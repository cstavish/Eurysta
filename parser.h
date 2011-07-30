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

#ifndef PARSER_H
#define PARSER_H

#include <sys/types.h>

enum src_type {
    SRC_STREAM,
    SRC_STRING,
    SRC_MMAP
};

enum err_type {
    ERR_NONE,
    ERR_ILLEGAL,
    ERR_NO_MEM,
    ERR_EXPECTED_COLON,
    ERR_EXPECTED_RCURLY,
    ERR_EXPECTED_RSQUARE,
    ERR_EXPECTED_KEY,
    ERR_EXPECTED_VALUE,
    ERR_EXPECTED_TRUE,
    ERR_EXPECTED_FALSE,
    ERR_EXPECTED_NULL,
    ERR_INVALID_ESCAPE
};

enum tok_type {
    TOK_LSQUARE = '[',
    TOK_RSQUARE = ']',
    TOK_LCURLY  = '{',
    TOK_RCURLY  = '}',
    TOK_COMMA   = ',',
    TOK_COLON   = ':',
    TOK_NULL    = 'n',
    TOK_TRUE    = 't',
    TOK_FALSE   = 'f',
    TOK_STRING  = 's',
    TOK_NUMBER  = '#',
    TOK_END     = 'e'
};

typedef enum src_type src_t;
typedef enum err_type err_t;
typedef enum tok_type tok_t;

struct cs_json_parser {
    uint32_t position;
    union {
        FILE *stream;
        const char *string;
    } source;
    src_t whence;
    err_t error;
    tok_t current;
    // for mmap
    int file_des;
    size_t input_size;
};

typedef struct cs_json_parser cs_json_parser;

cs_json_parser *cs_parser_create_fn(const char *file);

cs_json_parser *cs_parser_create_fmm(const char *file);

cs_json_parser *cs_parser_create_f(FILE *source);

cs_json_parser *cs_parser_create_s(const char *source);

void cs_parser_destroy(cs_json_parser *p);

cs_json_obj *cs_json_parse(cs_json_parser *p);

const char *cs_strtype(enum obj_type t);
const char *cs_strerror(enum err_type e);

#endif
