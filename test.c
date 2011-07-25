#include <stdio.h>
#include "eurysta.h"

// to compile:
// gcc parser.c c_data_structs/cs_hash_tab.c c_data_structs/cs_linked_list.c object.c -std=c99 test.c -o test -O2

int main(int argc, const char **argv) {
    cs_json_parser *parser = cs_parser_create_fn("twitter.json");
    cs_json_obj *root = cs_json_parse(parser);
    if (root)
        cs_object_print(root);
    cs_parser_destroy(parser);
}