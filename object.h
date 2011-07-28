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

#include <stdint.h>
#include <stddef.h>

enum obj_type {
    TYPE_OBJECT,
    TYPE_ARRAY,
    TYPE_STRING,
    TYPE_NUMBER,
    TYPE_BOOL,
    TYPE_NULL
};

struct cs_json_obj {
    enum obj_type type;
    void *data;
};

typedef struct cs_json_obj cs_json_obj;

void cs_object_print(cs_json_obj *obj);

cs_json_obj *cs_object_create(void);

cs_json_obj *cs_array_create(void);

cs_json_obj *cs_bool_create(uint8_t val);

cs_json_obj *cs_string_create(char *val, uint8_t assign);

cs_json_obj *cs_number_create(double val);

void cs_object_destroy(cs_json_obj *o);

char *cs_string_get_val(cs_json_obj *string);
uint8_t cs_string_set_val(cs_json_obj *string, const char *value);
size_t cs_string_get_len(cs_json_obj *string);

double cs_number_get_val(cs_json_obj *number, uint8_t *success);
uint8_t cs_number_set_val(cs_json_obj *number, double value);

uint8_t cs_bool_get_val(cs_json_obj *boolean, uint8_t *success);
uint8_t cs_bool_set_val(cs_json_obj *boolean, uint8_t value);

cs_json_obj *cs_object_get_val(cs_json_obj *object, const char *key);
uint8_t cs_object_set_val(cs_json_obj *object, const char *key, cs_json_obj *value);

cs_json_obj *cs_array_get_val(cs_json_obj *array, uint32_t index);
uint8_t cs_array_set_val(cs_json_obj *array, uint32_t index, cs_json_obj *value);
size_t cs_array_get_len(cs_json_obj *array);
