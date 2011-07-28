#include <stdio.h>
#include <string.h>
#include "eurysta.h"

// to compile:
// gcc parser.c c_data_structs/cs_hash_tab.c c_data_structs/cs_linked_list.c object.c -std=c99 test.c -o test -O3

int main(int argc, const char **argv) {
    cs_json_parser *parser = NULL;
    if (argc > 1 && strcmp(argv[1], "-") == 0)
        parser = cs_parser_create_f(stdin);
    else
        parser = cs_parser_create_fmm("twitter.json"); // parser = cs_parser_create_fn("twitter.json");
    
    cs_json_obj *root = cs_json_parse(parser);

    if (root != NULL) {
        // warning: terminal will be bombed if the root object is big
        cs_object_print(root);
        
        printf("\n\nNumber of tweets: %zu\n", cs_array_get_len(root));
        
        // tweets[5].text
        printf("Text of 6th tweet: \"%s\"\n", cs_string_get_val(cs_object_get_val(cs_array_get_val(root, 5), "text")));
        
        cs_object_destroy(root);
    }
    else {
        fprintf(stderr, "Error: %s\n", cs_strerror(parser->error));
    }
    
    cs_parser_destroy(parser);
}