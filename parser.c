/*
Copyright (c) 2011, Coleman Stavish
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
	notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.
  * Neither the name of Coleman Stavish nor the
	names of contributors may be used to endorse or promote products
	derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL COLEMAN STAVISH BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include "eurysta.h"

static inline void putback_(cs_json_parser *p, char c) {
    if (p->whence == SRC_STREAM)
        ungetc(c, p->source.stream);
    if (p->position > 0)
        p->position--;
}

static inline char next_(cs_json_parser *p) {
    int ch = 0;
    if (p->whence == SRC_STREAM) {
        p->position++;
        ch = fgetc(p->source.stream);
        if (ch == EOF)
            return '\0';
    }
    // SRC_STRING or SRC_MMAP
    else {
        if (p->position >= p->input_size)
            return '\0';
        ch = p->source.string[p->position++];
    }
    return (char)ch;
}

static inline uint8_t match_str_(cs_json_parser *p, const char *s, uint32_t l) {
    char c = '\0', *n = (char *)s;
    while (*n && (c = next_(p)) && c == *n++)
        ;
    return n - l == s;
}

// inline isdigit/isspace > dynamically linked library isdigit/isspace
static inline uint8_t my_isdigit_(char c) {
    return c >= '0' && c <= '9';
}

static inline uint8_t my_isspace_(char c) {
    uint8_t s = 0;
    switch (c) {
        case  ' ': case '\n': case '\t':
        case '\v': case '\f': case '\r':
            s = 1;
    }
    return s;
}

static tok_t get_tok_(cs_json_parser *p) {
    char ch = '\0';

    // skip whitespace
    while (my_isspace_(ch = next_(p)))
        ;

    if (ch == '\0')
        return p->current = TOK_END;
    
    switch (ch) {
        case '[': case ']': case ':':
        case '{': case '}': case ',':
            return p->current = ch;

        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        case '8': case '9': case '-':
            putback_(p, ch);
            return p->current = TOK_NUMBER;

        case '"':
            return p->current = TOK_STRING;
        
        // expect literal 'null'
        case 'n':
            if (match_str_(p, "ull", 3))
                return p->current = TOK_NULL;

            p->error = ERR_EXPECTED_NULL;
            break;
        
        // expect literal 'true'
        case 't':
            if (match_str_(p, "rue", 3))
                return p->current = TOK_TRUE;

            p->error = ERR_EXPECTED_TRUE;
            break;
        
        // expect literal 'false'
        case 'f':
            if (match_str_(p, "alse", 4))
                return p->current = TOK_FALSE;

            p->error = ERR_EXPECTED_FALSE;
            break;
        
        default:
            p->error = ERR_ILLEGAL;
            return TOK_END;
    }
}

static char *string_raw_(cs_json_parser *p) {
    uint32_t buf_size = 2048;
    char *buffer = malloc(buf_size);
    if (buffer == NULL) {
        p->error = ERR_NO_MEM;
        goto fail;
    }
    
    // "embedded" state machine
    uint32_t len = 0;
    uint8_t in_esc = 0,  // in escape sequence initiated by \ (reverse solidus)
            in_uni = 0,  // in Unicode escape sequence initiated by \u
            uni_len = 0; // position in Unicode escape sequence (e.g. '\u5c5c'): 0-3 inclusive

    uint32_t uni_code = 0;
    char c = '\0';
    while ((c = next_(p))) {
    
        // buffer must have at least 4 free bytes:
        // potentially 3 for high Unicode sequences, and a terminating 0 byte
        if (len > buf_size - 4) {
            char *new = realloc(buffer, buf_size += 2048);
            if (new == NULL) {
                p->error = ERR_NO_MEM;
                goto fail;
            }
            buffer = new;
        }
    
        if (in_esc && !in_uni) {
            switch (c) {
                // note the start of a Unicode escape
                case 'u': in_uni = 1; break;
                // single character escape sequences
                case 'b':  buffer[len++] = '\b'; break;
                case 'f':  buffer[len++] = '\f'; break;
                case 'n':  buffer[len++] = '\n'; break;
                case 'r':  buffer[len++] = '\r'; break;
                case 't':  buffer[len++] = '\t'; break;
                case '\\': buffer[len++] = '\\'; break;
                case '/':  buffer[len++] = '/'; break;
                case '"':  buffer[len++] = '"'; break;
                default:
                    p->error = ERR_INVALID_ESCAPE;
                    goto fail;
            }
            in_esc = 0;
        }
        else if (in_uni) {
            // a character in [0-9A-Fa-f] must be converted to a 4 bit integer
            uint8_t half = 0;
            if (my_isdigit_(c)) {
                half = c - '0'; // 0-9
            }
            else if (c >= 'A' && c <= 'F') {
                half = c - 55; // 10-15
            }
            else if (c >= 'a' && c <= 'f') {
                half = c - 87; // 10-15
            }
            else {
                // not /[0-9A-Fa-f]/
                p->error = ERR_INVALID_ESCAPE;
                goto fail;
            }
            
            // pack in the latest 4 bits
            uni_code <<= 4;
            uni_code |= half;

            // UTF-8 <3
            if (++uni_len == 4) {
                // just a 7-bit ASCII char, no big deal
                if (uni_code <= 0x7F) {
                    buffer[len++] = (char)uni_code;
                }
                // now we're going dual-byte
                else if (uni_code <= 0x07FF) {                    
                    // prefix with '110', then select the 5 most significant bits
                    //  of the char code, shift them down and combine
                    buffer[len++] = 0xC0 | ((uni_code & 0x07C0) >> 6);
                    
                    // prefix with '10', then select the 6 lowest bits of the char code and combine
                    buffer[len++] = 0x80 | (uni_code & 0x3F);
                    
                    // result = 0b110xxxxx 0b10xxxxxx, where x refers to a bit of the char code
                }
                // and beyond...
                else if (uni_code <= 0xFFFF) {
                    // prefix with '1110', indicating a 3 byte sequence, select the 4 most
                    //  significant bits of the char code, shift, etc.
                    buffer[len++] = 0xE0 | ((uni_code & 0xF000) >> 12);
                    
                    // prefix with '10', followed by the next 6 most significant bits of the char code
                    buffer[len++] = 0x80 | ((uni_code & 0x0FC0) >> 6);
                    
                    // same situation as with 2 byte sequences
                    buffer[len++] = 0x80 | (uni_code & 0x3F);
                    
                    // result = 0b1110xxxx 0b10xxxxxx 0b10xxxxxx
                }
                in_uni = in_esc = uni_len = uni_code = 0;
            }
        }
        else {
            switch (c) {
                case '"':
                    // the unescaped final double quote--at last!
                    goto win;
                case '\\':
                    // note the start of an escape sequence
                    in_esc = 1;
                    break;
                default:
                    // character is nothing special, buffer it
                    buffer[len++] = c;
                    break;
            }
        }
    }

fail:
    free(buffer);
    return NULL;

win:
    buffer[len] = '\0';
    char *final = malloc(len);
    if (final != NULL)
        memcpy(final, buffer, len);

    free(buffer);
    return final;
}

static cs_json_obj *string_(cs_json_parser *p) {
    return cs_string_create(string_raw_(p), 1);
}

extern cs_json_obj null_;

static cs_json_obj *number_(cs_json_parser *p) {
    char buffer[256];
    char c = '\0';
    uint32_t len = 0;

    // "embedded" state machine makes a comeback, albeit with less grandeur
    uint8_t in_expo = 0, // in the optional exponent part of the number (following 'e' or 'E')
            in_frac = 0; // in the optinal fraction part of the number (folowwing '.')
    
    while (len < sizeof(buffer) - 1 && (c = next_(p))) {
        switch (c) {
            case '.':
                if (in_frac)
                    goto fail;
                in_frac = 1;
                break;
            case 'e': case 'E':
                if (in_expo)
                    goto fail;
                in_expo = 1;
                break;
            case '+': case '-':
                // '+' or '-' may appear within a number iff it immediately follows 'e' or 'E'
                if (len > 0 && (buffer[len - 1] != 'e' && buffer[len - 1] != 'E'))
                    goto fail;
                break;
            default:
                // end of the number
                if (!my_isdigit_(c)) {
                    putback_(p, c);
                    goto win;
                }
                break;
        }
        buffer[len++] = c;
    }

fail:
    p->error = ERR_ILLEGAL;
    return NULL;

win:
    buffer[len] = '\0';
    errno = 0;
    double val = strtod(buffer, NULL);
    if (errno == ERANGE || errno == EINVAL)
        return &null_;
    return cs_number_create(val);
}

static cs_json_obj *array_(cs_json_parser *p) {
    cs_json_obj *array = cs_array_create();
    if (array == NULL)
        return NULL;
    
    do {    
        cs_json_obj *obj = cs_json_parse(p);
        if (obj == NULL) {
            if (p->current == TOK_RSQUARE) // [ ]
                return array;
            // only set error if one was not assigned previously
            if (p->error == ERR_NONE)
                p->error = ERR_EXPECTED_VALUE;
            goto fail;
        }
        
        // success--appened object
        cs_dll_app((cs_dll *)(array->data), obj);

    } while (get_tok_(p) == TOK_COMMA);
    
    if (p->current == TOK_RSQUARE)
        return array;
    
    p->error = ERR_EXPECTED_RSQUARE;
    
fail:
    cs_json_obj_destroy(array);
    return NULL;
}

static cs_json_obj *object_(cs_json_parser *p) {
    cs_json_obj *object = cs_object_create();
    if (object == NULL)
        return NULL;
        
    do {
        // try to match first double quote
        if (get_tok_(p) != TOK_STRING) {
            if (p->current == TOK_RCURLY)
                return object;
            p->error = ERR_EXPECTED_KEY;
            goto fail;
        }

        char *key = string_raw_(p);
        if (key == NULL) {
            goto fail;
        }
        
        // try to match key-value separator :
        if (get_tok_(p) != TOK_COLON) {
            p->error = ERR_EXPECTED_COLON;
            goto fail;
        }
        
        cs_json_obj *val = cs_json_parse(p);
        if (val == NULL) {
            if (p->error == ERR_NONE)
                p->error = ERR_EXPECTED_VALUE;
            free(key);
            goto fail;
        }
        
        cs_hash_set((cs_hash_tab *)(object->data), key, val);
        
    } while (get_tok_(p) == TOK_COMMA);
    
    // match closing }
    if (p->current == TOK_RCURLY)
        return object;

    p->error = ERR_EXPECTED_RCURLY;

fail:
    cs_json_obj_destroy(object);
    return NULL;
}

cs_json_obj *cs_json_parse(cs_json_parser *p) {
    switch (get_tok_(p)) {
        case TOK_LCURLY:  return object_(p);
        case TOK_LSQUARE: return array_(p);
        case TOK_STRING:  return string_(p);
        case TOK_NUMBER:  return number_(p);
        case TOK_TRUE:    return cs_bool_create(1);
        case TOK_FALSE:   return cs_bool_create(0);
        case TOK_NULL:    return &null_;
    }
    return NULL;
}

cs_json_parser *cs_parser_create_fn(const char *file) {
    FILE *f = fopen(file, "r");
    return cs_parser_create_f(f);
}

cs_json_parser *cs_parser_create_fmm(const char *file) {
    cs_json_parser *p = cs_parser_create_s(NULL);
    if (p == NULL)
        return NULL;
    
    int fd = -1;
    if ((fd = open(file, O_RDWR)) > 0) {
        struct stat s;
        if (fstat(fd, &s) != -1) {
            if ((p->source.string = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0)) != MAP_FAILED) {
                p->file_des = fd;
                p->input_size = s.st_size;
                p->whence = SRC_MMAP;
                return p;
            }
            else {
                close(fd);
            }
        }
    }
    
    // err
    free(p);
    return NULL;
}

cs_json_parser *cs_parser_create_f(FILE *source) {
    if (source == NULL)
        return NULL;

    cs_json_parser *p = malloc(sizeof(cs_json_parser));
    if (p == NULL)
        return NULL;

    p->whence = SRC_STREAM;
    p->source.stream = source;
    p->position = 0;
    p->error = ERR_NONE;
    
    return p;
}

cs_json_parser *cs_parser_create_s(const char *source) {
    cs_json_parser *p = malloc(sizeof(cs_json_parser));
    if (p == NULL)
        return NULL;

    p->whence = SRC_STRING;
    p->source.string = source;
    p->position = 0;
    p->input_size = UINT_MAX;
    p->error = ERR_NONE;
    
    return p;
}

void cs_parser_destroy(cs_json_parser *p) {
    if (p->whence == SRC_STREAM && p->source.stream != stdin) {
        fclose(p->source.stream);
    }
    else if (p->whence == SRC_MMAP) {
        munmap((void *)p->source.string, p->input_size);
        close(p->file_des);
    }
    free(p);
}

const char *cs_strtype(enum obj_type t) {
    static const char *names[] = { "object", "array", "string", "number", "boolean", "null" };
    if (t < sizeof(names))
        return names[t];
    return NULL;
}

const char *cs_strerror(enum err_type e) {
    static const char *errors[] = {
        "No error",
        "Illegal",
        "Could not allocate memory",
        "Expected ':'",
        "Expected '}'",
        "Expected ']'",
        "Expected key",
        "Expected value",
        "Expected 'true'",
        "Expected 'false'",
        "Expected 'null'",
        "Invalid escape"
    };
    if (e < sizeof(errors))
        return errors[e];
    return NULL;
}