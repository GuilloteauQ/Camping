#ifndef _JSON_EXTRACTION_H_
#define _JSON_EXTRACTION_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsmn.h"
#define TOKEN_SIZE 128
struct json_data {
    char* original;
    int json_len;
    jsmntok_t t[TOKEN_SIZE];
    int nb_tokens;
};
char* file_content_to_string(char* filename);

struct json_data* extract_file(char* filename);

void free_json_data(struct json_data* data);

void extract_string(struct json_data* data,
                    int token_index,
                    int len,
                    char* extracted);

int extract_attribute(struct json_data* data, char* attribute);

char* extract_exec_file(struct json_data* data);

char* extract_prologue_epilogue(struct json_data* data, char* to_extract);

char* extract_prologue(struct json_data* data);

char* extract_epilogue(struct json_data* data);

#endif

