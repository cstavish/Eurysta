#include <stdio.h>
#include <string.h>
#include "eurysta.h"

// to compile:
// gcc parser.c c_data_structs/cs_hash_tab.c c_data_structs/cs_linked_list.c object.c -std=c99 bench.c -o bench -O2

int main(int argc, const char **argv) {
    cs_json_parser *parser = cs_parser_create_fmm("twitter.json");
    
    int i = 0;
    while (i++ < 3000) {
        cs_json_obj *root = cs_json_parse(parser);
        cs_object_destroy(root);
        
        // reset parser
        parser->position = 0;
        parser->current = 0;
    }
    
    cs_parser_destroy(parser);
}