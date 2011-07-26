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
#include <ctype.h>
#include <errno.h>
#include "eurysta.h"

/*
 * TODO:
 *
 * 1. Error reporting
 * 2. Solid interface (essentially a few wrapper functions)
 * 3. DOCUMENTATION and more insightful comments
 *
 * - Coleman
 *
 */

static inline void putback_(cs_json_parser *p, char c) {
    if (p->whence == SRC_STREAM) {
        ungetc(c, p->source.stream);
    }
    else if (p->whence == SRC_STRING) {
        p->position--;
    }
}

static inline char next_(cs_json_parser *p) {
    int ch = 0;
    if (p->whence == SRC_STREAM) {
        ch = fgetc(p->source.stream);
        if (ch == EOF)
            return '\0';
    }
    else if (p->whence == SRC_STRING) {
        if (p->source.string[p->position] == '\0')
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

static tok_t get_tok_(cs_json_parser *p) {
    int ch = 0;
    // skip whitespace
    while (isspace(ch = next_(p)))
        ;
    if (ch == 0) {
        // cleanup
        return p->current = TOK_END;
    }
    
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
            
        case 'n':
            if (match_str_(p, "ull", 3))
                return p->current = TOK_NULL;
            break;
            
        case 't':
            if (match_str_(p, "rue", 3))
                return p->current = TOK_TRUE;
            break;
            
        case 'f':
            if (match_str_(p, "alse", 4))
                return p->current = TOK_FALSE;
            break;
            
        default:
            break;
    }
    return TOK_END;
}

static char *string_raw_(cs_json_parser *p) {
    uint32_t buf_size = 4096;
    char *buffer = malloc(buf_size);
    if (buffer == NULL) {
        // err = insufficient mem
        goto fail;
    }
    
    // "embedded" state machine
    uint32_t len = 0;
    uint8_t in_esc = 0, in_uni = 0, uni_len = 0;
    uint32_t uni_code = 0;
    char c = '\0';
    while ((c = next_(p))) {
    
        if (len > buf_size - 4) {
            char *new = realloc(buffer, buf_size += 4096);
            if (new == NULL) {
                goto fail;
            }
            buffer = new;
        }
    
        if (in_esc && !in_uni) {
            switch (c) {
                case 'b':  buffer[len++] = '\b'; break;
                case 'f':  buffer[len++] = '\f'; break;
                case 'n':  buffer[len++] = '\n'; break;
                case 'r':  buffer[len++] = '\r'; break;
                case 't':  buffer[len++] = '\t'; break;
                case '\\': buffer[len++] = '\\'; break;
                case '/':  buffer[len++] = '/'; break;
                case '"':  buffer[len++] = '/'; break;
                case 'u': in_uni = 1; break;
                default: goto fail; // invalid escape char
            }
            in_esc = 0;
        }
        else if (in_uni) {
            uint8_t half = 0;
            if (isdigit(c)) {
                half = c - '0';
            }
            else if (c >= 'A' && c <= 'F') {
                half = c - 55; // 10-15
            }
            else if (c >= 'a' && c <= 'f') {
                half = c - 87; // 10-15
            }
            else {
                // not /[0-9A-Fa-f]/
                // set err
                goto fail;
            }
            
            static uint16_t pow_table[] = { 0x1000, 0x0100, 0x10, 0x01 };
            uni_code += pow_table[uni_len] * half;

            // UTF-8 <3
            if (++uni_len == 4) {
                if (uni_code <= 0x7F) {
                    buffer[len++] = (char)uni_code;
                }
                else if (uni_code <= 0x07FF) {
                    // 0b110xxxxx 0b10xxxxxx
                    buffer[len++] = 0xC0 | ((uni_code & 0x07C0) >> 6);
                    buffer[len++] = 0x80 | (uni_code & 0x3F);
                }
                else if (uni_code <= 0xFFFF) {
                    // 0b1110xxxx 0b10xxxxxx 0b10xxxxxx
                    buffer[len++] = 0xE0 | ((uni_code & 0xF000) >> 12);
                    buffer[len++] = 0x80 | ((uni_code & 0x0FC0) >> 6);
                    buffer[len++] = 0x80 | (uni_code & 0x3F);
                }
                else {
                    // invalid escape sequence
                    goto fail;
                }
                in_uni = in_esc = uni_len = uni_code = 0;
            }
        }
        else {
            switch (c) {
                case '"':
                    if (!in_esc) // at last!
                        goto win;
                    in_esc = 0;
                    break;
                case '\\':
                    in_esc = 1;
                    break;
                default:
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
    char *final = strdup(buffer);
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
    // "embedded" state machine makes a comeback, albeit in a state of lesser grandeur
    uint8_t in_expo = 0, in_frac = 0;
    
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
                if (len > 0 && tolower(buffer[len - 1]) != 'e')
                    goto fail;
                break;
            default:
                if (!isdigit(c)) {
                    goto win;
                }   
                break;
        }
        buffer[len++] = c;
    }

fail:
    // set error
    return NULL;

win:
    putback_(p, c);
    buffer[len] = '\0';
    char *end = NULL;
    errno = 0;
    double val = strtod(buffer, &end);
    if (errno == ERANGE || errno == EINVAL)
        return &null_;
    return cs_number_create(val);
}

cs_json_obj *cs_json_parse(cs_json_parser *p);

static cs_json_obj *array_(cs_json_parser *p) {
    cs_json_obj *array = cs_array_create();
    if (array == NULL)
        return NULL;
    
    do {    
        cs_json_obj *obj = cs_json_parse(p);
        if (obj == NULL) {
            if (p->current == TOK_RSQUARE) // [ ]
                return array;
            goto fail;
        }
        
        // success!
        cs_dll_app((cs_dll *)(array->data), obj);

    } while (get_tok_(p) == TOK_COMMA);
    
    if (p->current == TOK_RSQUARE)
        return array;
    
fail:
    cs_json_obj_destroy(array);
    return NULL;
}

static cs_json_obj *object_(cs_json_parser *p) {
    cs_json_obj *object = cs_object_create();
    if (object == NULL)
        return NULL;
        
    do {
        if (get_tok_(p) != TOK_STRING) {
            if (p->current == TOK_RCURLY)
                return object;
            goto fail;
        }
        
        char *key = string_raw_(p);
        if (key == NULL) {
            goto fail;
        }
        
        // couldn't match key-val separator
        if (get_tok_(p) != TOK_COLON) {
            goto fail;
        }
        
        cs_json_obj *val = cs_json_parse(p);
        if (val == NULL) {
            free(key);
            goto fail;
        }
        
        cs_hash_set((cs_hash_tab *)(object->data), key, val);
        
    } while (get_tok_(p) == TOK_COMMA);
    
    if (p->current == TOK_RCURLY)
        return object;

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
    p->error = ERR_NONE;
    
    return p;
}

void cs_parser_destroy(cs_json_parser *p) {
    if (p->whence == SRC_STREAM && p->source.stream != stdin)
        fclose(p->source.stream);
    free(p);
}

const char *strtype(enum obj_type t) {
    static const char *names[] = { "object", "array", "string", "number", "boolean", "null" };
    if (t >= 0 && t < sizeof(names))
        return names[t];
    return NULL;
}
