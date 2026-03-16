#include "jsmn.h"
#include <stddef.h>

static jsmntok_t* jsmn_alloc_token(jsmn_parser* parser, jsmntok_t* tokens, size_t num_tokens) {
    if (parser->toknext >= num_tokens) {
        return NULL;
    }
    jsmntok_t* token = &tokens[parser->toknext++];
    token->start = token->end = -1;
    token->size = 0;
    token->parent = -1;
    token->type = JSMN_UNDEFINED;
    return token;
}

static void jsmn_fill_token(jsmntok_t* token, jsmntype_t type, int start, int end) {
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

void jsmn_init(jsmn_parser* parser) {
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}

static int jsmn_parse_primitive(jsmn_parser* parser, const char* js, size_t len, jsmntok_t* tokens, size_t num_tokens) {
    int start = (int)parser->pos;
    for (; parser->pos < len; parser->pos++) {
        switch (js[parser->pos]) {
            case '\t': case '\r': case '\n': case ' ':
            case ',': case ']': case '}':
                goto found;
            default:
                break;
        }
    }
found:
    if (tokens == NULL) {
        parser->pos--;
        return 0;
    }
    jsmntok_t* token = jsmn_alloc_token(parser, tokens, num_tokens);
    if (token == NULL) {
        parser->pos = start;
        return JSMN_ERROR_NOMEM;
    }
    jsmn_fill_token(token, JSMN_PRIMITIVE, start, (int)parser->pos);
    token->parent = parser->toksuper;
    parser->pos--;
    return 0;
}

static int jsmn_parse_string(jsmn_parser* parser, const char* js, size_t len, jsmntok_t* tokens, size_t num_tokens) {
    int start = (int)parser->pos + 1;
    parser->pos++;
    for (; parser->pos < len; parser->pos++) {
        char c = js[parser->pos];
        if (c == '\"') {
            if (tokens == NULL) {
                return 0;
            }
            jsmntok_t* token = jsmn_alloc_token(parser, tokens, num_tokens);
            if (token == NULL) {
                parser->pos = start - 1;
                return JSMN_ERROR_NOMEM;
            }
            jsmn_fill_token(token, JSMN_STRING, start, (int)parser->pos);
            token->parent = parser->toksuper;
            return 0;
        }
        if (c == '\\' && parser->pos + 1 < len) {
            parser->pos++;
        }
    }
    parser->pos = start - 1;
    return JSMN_ERROR_PART;
}

int jsmn_parse(jsmn_parser* parser, const char* js, unsigned int len, jsmntok_t* tokens, unsigned int num_tokens) {
    int result;
    for (; parser->pos < len; parser->pos++) {
        char c = js[parser->pos];
        jsmntok_t* token;
        switch (c) {
            case '{':
            case '[':
                token = jsmn_alloc_token(parser, tokens, num_tokens);
                if (token == NULL) {
                    return JSMN_ERROR_NOMEM;
                }
                if (parser->toksuper != -1) {
                    tokens[parser->toksuper].size++;
                    token->parent = parser->toksuper;
                }
                token->type = (c == '{') ? JSMN_OBJECT : JSMN_ARRAY;
                token->start = (int)parser->pos;
                parser->toksuper = (int)(parser->toknext - 1);
                break;
            case '}':
            case ']':
                if (tokens == NULL) {
                    break;
                }
                for (int i = (int)parser->toknext - 1; i >= 0; i--) {
                    token = &tokens[i];
                    if (token->start != -1 && token->end == -1) {
                        if ((token->type == JSMN_OBJECT && c == '}') || (token->type == JSMN_ARRAY && c == ']')) {
                            token->end = (int)parser->pos + 1;
                            parser->toksuper = token->parent;
                            break;
                        }
                        return JSMN_ERROR_INVAL;
                    }
                }
                break;
            case '\"':
                result = jsmn_parse_string(parser, js, len, tokens, num_tokens);
                if (result < 0) {
                    return result;
                }
                if (parser->toksuper != -1) {
                    tokens[parser->toksuper].size++;
                }
                break;
            case '\t':
            case '\r':
            case '\n':
            case ' ':
            case ':':
            case ',':
                break;
            default:
                result = jsmn_parse_primitive(parser, js, len, tokens, num_tokens);
                if (result < 0) {
                    return result;
                }
                if (parser->toksuper != -1) {
                    tokens[parser->toksuper].size++;
                }
                break;
        }
    }

    for (unsigned int i = 0; i < parser->toknext; i++) {
        if (tokens[i].start != -1 && tokens[i].end == -1) {
            return JSMN_ERROR_PART;
        }
    }
    return (int)parser->toknext;
}
