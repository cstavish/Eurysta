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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "eurysta.h"

// global "null" object
cs_json_obj null_ = { TYPE_NULL, NULL };

static inline void spec_destructor_(const char *k, void *v) {
    cs_json_obj *obj = (cs_json_obj *)v;
    switch (obj->type) {
        case TYPE_OBJECT:
            cs_hash_destroy(obj->data);
            break;
        case TYPE_ARRAY:
            cs_dll_destroy(obj->data);
            break;
        case TYPE_STRING:
            free(obj->data);
            break;
        case TYPE_NUMBER:
            free(obj->data);
            break;
        case TYPE_NULL:
            return;
    }
    free(obj);
}

static void generic_destructor_(void *v) {
    spec_destructor_(NULL, v);
}

static inline void print_string_(const char *s, FILE *f) {
    putc('"', f);
    while (*s) {
        if (*s == '"')
            putc('\\', f);
        putc(*s++, f);
    }
    putc('"', f);
}

static inline void print_hash_(cs_hash_tab *t, FILE *f) {
    putc('{', f);
    for (size_t i = 0, c = 0; i < t->size, c < t->count; i++) {
        for (cs_knode *n = t->buckets[i]; n != NULL; n = n->next) {
            print_string_(n->key, f);
            putc(':', f);
            cs_object_print(n->val, f);
            if (t->count - c++ > 1)
                putc(',', f);
        }
    }
    putc('}', f);
}

static inline void print_array_(cs_dll *list, FILE *f) {
    putc('[', f);
    cs_dll_node *n = list->start;
    for (int i = 0; n != NULL; i++, n = n->next) {
        cs_object_print((cs_json_obj *)n->data, f);
        if (i < list->size - 1)
            putc(',', f);
    }
    putc(']', f);
}

void cs_object_destroy(cs_json_obj *o) {
    generic_destructor_(o);
}

void cs_object_print(cs_json_obj *obj, FILE *f) {
    switch (obj->type) {
        case TYPE_STRING:
            print_string_((const char *)obj->data, f);
            break;
        case TYPE_NUMBER:
            fprintf(f, "%g", *(double *)obj->data);
            break;
        case TYPE_OBJECT:
            print_hash_((cs_hash_tab *)obj->data, f);
            break;
        case TYPE_ARRAY:
            print_array_((cs_dll *)obj->data, f);
            break;
        case TYPE_NULL:
            fprintf(f, "null");
            break;
        case TYPE_BOOL:
            fprintf(f, "%s", ((uintptr_t)obj->data & 1) ? "true" : "false");
    }
}

cs_json_obj *cs_object_create(void) {
    cs_json_obj *obj = malloc(sizeof(cs_json_obj));
    if (obj == NULL)
        return NULL;
        
    obj->type = TYPE_OBJECT;
    if ((obj->data = cs_hash_create_opt(32, 0.75f, 0.25f)) == NULL) {
        free(obj);
        return NULL;
    }

    ((cs_hash_tab *)(obj->data))->cleanup = spec_destructor_;
    ((cs_hash_tab *)(obj->data))->copy_keys = 0;

    return obj;
}

cs_json_obj *cs_array_create(void) {
    cs_json_obj *obj = malloc(sizeof(cs_json_obj));
    if (obj == NULL)
        return NULL;
    
    obj->type = TYPE_ARRAY;
    if ((obj->data = cs_dll_create(generic_destructor_, NULL)) == NULL) {
        free(obj);
        return NULL;
    }
    
    return obj;
}

cs_json_obj *cs_bool_create(uint8_t val) {
    cs_json_obj *obj = malloc(sizeof(cs_json_obj));
    if (obj == NULL)
        return NULL;
    
    obj->type = TYPE_BOOL;
    obj->data = (void *)(uintptr_t)(0 | val & 1);
    
    return obj;
}

cs_json_obj *cs_string_create(char *val, uint8_t assign) {
    if (val == NULL)
        return NULL;

    cs_json_obj *str = malloc(sizeof(cs_json_obj));
    if (str == NULL)
        return NULL;
        
    str->type = TYPE_STRING;
    str->data = (assign) ? val : strdup(val);
    
    return str;
}

cs_json_obj *cs_number_create(double val) {
    cs_json_obj *num = malloc(sizeof(cs_json_obj));
    if (num == NULL)
        return NULL;
    
    num->type = TYPE_NUMBER;
    if ((num->data = malloc(sizeof(double))) == NULL) {
        free(num);
        return NULL;
    }
    
    *(double *)(num->data) = val;
    
    return num;
}

char *cs_string_get_val(cs_json_obj *string) {
    if (string != NULL && string->type == TYPE_STRING) {
        return string->data;
    }
    return NULL;
}

uint8_t cs_string_set_val(cs_json_obj *string, const char *value) {
    if (string != NULL && string->type == TYPE_STRING) {
        // FIXME: it's unsafe to assume the previous value was dynamically allocated
        free(string->data);
        string->data = strdup(value);
        return 1;
    }
    return 0;
}

inline size_t cs_string_get_len(cs_json_obj *string) {
    return strlen((char *)string->data);
}

double cs_number_get_val(cs_json_obj *number, uint8_t *success) {
    uint8_t s = 0;
    double v = 0;
    if (number != NULL && number->type == TYPE_NUMBER) {
        s = 1;
        v = *(double *)number->data;
    }
    if (success != NULL)
        *success = s;
    return v;
}

uint8_t cs_number_set_val(cs_json_obj *number, double value) {
    if (number != NULL && number->type == TYPE_NUMBER) {
        free(number->data);
        if ((number->data = malloc(sizeof(double))) != NULL) {
            *(double *)number->data = value;
            return 1;
        }
    }
    return 0;
}

uint8_t cs_bool_get_val(cs_json_obj *boolean, uint8_t *success) {
    uint8_t s = 0, v = 0;
    if (boolean != NULL) {
        s = 1;
        v = (uintptr_t)boolean->data & 1;
    }
    if (success != NULL)
        *success = s;
    return v;
}

uint8_t cs_bool_set_val(cs_json_obj *boolean, uint8_t value) {
    if (boolean != NULL && boolean->type == TYPE_BOOL) {
        boolean->data = (void *)((uintptr_t)value & 1);
        return 1;
    }
    return 0;
}

cs_json_obj *cs_object_get_val(cs_json_obj *object, const char *key) {
    if (object != NULL && key != NULL && object->type == TYPE_OBJECT) {
        return cs_hash_get((cs_hash_tab *)object->data, key);
    }
    return NULL;
}

uint8_t cs_object_set_val(cs_json_obj *object, const char *key, cs_json_obj *value) {
    if (object != NULL && object->type == TYPE_OBJECT) {
        if (object->data != NULL) {
            cs_hash_set((cs_hash_tab *)object->data, key, value);
            return 1;
        }
    }
    return 0;
}

cs_json_obj *cs_array_get_val(cs_json_obj *array, uint32_t index) {
    if (array != NULL && array->type == TYPE_ARRAY) {
        return cs_dll_get((cs_dll *)array->data, index);
    }
    return NULL;
}

uint8_t cs_array_set_val(cs_json_obj *array, uint32_t index, cs_json_obj *value) {
    if (array != NULL && array->type == TYPE_ARRAY) {
        if (array->data) {
            return cs_dll_set((cs_dll *)array->data, value, index);
        }
    }
    return 0;
}

inline size_t cs_array_get_len(cs_json_obj *array) {
    return ((cs_dll *)array->data)->size;
}
