enum src_type {
	SRC_STREAM,
	SRC_STRING
};

enum err_type {
	ERR_NONE,
	ERR_ILLEGAL,
	ERR_NO_MEM,
	ERR_EXPECTED
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
typedef enum err_type	err_t;
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
};

typedef struct cs_json_parser cs_json_parser;

const char *strtype(enum obj_type t);

cs_json_parser *cs_parser_create_fn(const char *file);

cs_json_parser *cs_parser_create_f(FILE *source);

cs_json_parser *cs_parser_create_s(const char *source);

void cs_parser_destroy(cs_json_parser *p);

cs_json_obj *cs_json_parse(cs_json_parser *p);