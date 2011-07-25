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

static void spec_destructor_(const char *k, void *v) {
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

void generic_destructor_(void *v) {
    spec_destructor_(NULL, v);
}

static void print_hash_(cs_hash_tab *t, const char *k, void *v, uint32_t i) {
    if (i == 0)
        printf(" "); // insert space after {
    printf("\"%s\": ", k);
    cs_object_print((cs_json_obj *)v);
    if (i < t->count - 1)
        printf(", ");
    else if (t->count - i == 1)
        printf(" "); // insert space before }
}

static void print_array_(cs_dll *list) {
    cs_dll_node *n = list->start;
    for (int i = 0; n != NULL; i++, n = n->next) {
        if (i == 0)
            printf(" ");
        cs_object_print((cs_json_obj *)n->data);
        if (i < list->size - 1)
            printf(", ");
        else if (list->size - i == 1)
            printf(" ");
    }
}

void cs_object_print(cs_json_obj *obj) {
    switch (obj->type) {
        case TYPE_STRING:
            printf("\"%s\"", (char *)obj->data);
            break;
        case TYPE_NUMBER:
            printf("%.0f", *(double *)obj->data);
            break;
        case TYPE_OBJECT:
            printf("{");
            cs_hash_iterate((cs_hash_tab *)obj->data, print_hash_);
            printf("}");
            break;
        case TYPE_ARRAY:
            printf("[");
            print_array_((cs_dll *)obj->data);
            printf("]");
            break;
        case TYPE_NULL:
            printf("null");
            break;
        case TYPE_BOOL:
            printf("%s", (obj->data == (void *)1) ? "true" : "false");
    }
}

cs_json_obj *cs_object_create(void) {
    cs_json_obj *obj = malloc(sizeof(cs_json_obj));
    if (obj == NULL)
        return NULL;
        
    obj->type = TYPE_OBJECT;
    if ((obj->data = cs_hash_create()) == NULL) {
        free(obj);
        return NULL;
    }
    ((cs_hash_tab *)(obj->data))->cleanup = spec_destructor_;

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
    obj->data  = (val) ? (void *)1 : (void *)0;
    
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