#define JSMN_HEADER
#include "json_extraction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_SIZE 128

/* [start]
struct json_data {
    char* original;
    int json_len;
    jsmntok_t t[TOKEN_SIZE];
    int nb_tokens;
};
[end] */

char* file_content_to_string(char* filename) {
    char* buffer = 0;
    long length;
    FILE* f = fopen(filename, "rb");

    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer) {
            fread(buffer, 1, length, f);
        }
        fclose(f);
    }
    return buffer;
}
struct json_data* extract_file(char* filename) {
    struct json_data* data = malloc(sizeof(struct json_data));
    data->original = file_content_to_string(filename);
    data->json_len = strlen(data->original);

    jsmn_parser p;
    jsmn_init(&p);
    data->nb_tokens =
        jsmn_parse(&p, data->original, data->json_len, data->t, TOKEN_SIZE);

    return data;
}

void free_json_data(struct json_data* data) {
    free(data->original);
    free(data);
}

void extract_string(struct json_data* data,
                    int token_index,
                    int len,
                    char* extracted) {
    jsmntok_t current_token = data->t[token_index];
    // long len = current_token.end - current_token.start;
    long actual_len = (current_token.start + len > data->json_len)
                          ? data->json_len - current_token.start - 1
                          : len;
    for (long i = 0; i < actual_len; i++) {
        extracted[i] = data->original[current_token.start + i];
    }
    extracted[actual_len] = '\0';
}

int extract_attribute(struct json_data* data, char* attribute) {
    int max_len = strlen(attribute);
    char* extracted = malloc(sizeof(char) * max_len);
    int index = -1;
    for (int i = 0; i < data->nb_tokens; i++) {
        extract_string(data, i, max_len, extracted);
        if (strcmp(extracted, attribute) == 0) {
            index = i + 1;
            break;
        }
    }
    free(extracted);
    return index;
}

char* extract_exec_file(struct json_data* data) {
    int tok_index = extract_attribute(data, "exec_file");
    jsmntok_t tok = data->t[tok_index];
    long len = tok.end - tok.start;
    char* path = malloc(sizeof(char) * (len + 1));
    extract_string(data, tok_index, len, path);
    return path;
}

char* extract_prologue_epilogue(struct json_data* data, char* to_extract) {
    int prologue_index = extract_attribute(data, to_extract);
    jsmntok_t tok = data->t[prologue_index];
    int size = tok.size;
    int len = 0;
    for (int i = 0; i < size; i++) {
        len = len + data->t[prologue_index + 1 + i].end +
              data->t[prologue_index + 1 + i].start;
    }

    // len + size because we want to add ; between commands and end with a \0
    char* prologue_command = malloc(sizeof(char) * (len + size));
    int position = 0;
    for (int i = 0; i < size; i++) {
        jsmntok_t current = data->t[prologue_index + 1 + i];
        extract_string(data, prologue_index + 1 + i,
                       current.end - current.start,
                       prologue_command + position);
        position = position + current.end - current.start;
        if (i != size - 1) {
            prologue_command[position++] = ';';
        }
    }
    return prologue_command;
}

char* extract_prologue(struct json_data* data) {
    return extract_prologue_epilogue(data, "prologue");
}

char* extract_epilogue(struct json_data* data) {
    return extract_prologue_epilogue(data, "epilogue");
}
