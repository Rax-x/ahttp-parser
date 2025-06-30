#include "ahttp_parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct http_header* new_header() {
    
    struct http_header* header = (struct http_header*)malloc(sizeof(*header));
    if(header == NULL) {
        return NULL;
    }

    header->name = NULL;
    header->value = NULL;
    header->next = NULL;

    return header;
}

static void destroy_http_header(struct http_header* header) {

    if(header == NULL) return;

    free(header->name);
    free(header->value);
    free(header);
}

static void destroy_header_list(struct http_header* headers) {

    struct http_header* current = headers;
    struct http_header* header = NULL;

    while(current != NULL) {
        
        header = current;
        current = current->next;

        destroy_http_header(header);
    }
}

void destroy_http_request(struct http_request* request) {
    destroy_header_list(request->headers);
    free(request->body);

    free(request);
}

void destroy_http_response(struct http_response* response) {
    destroy_header_list(response->headers);
    free(response->body);

    free(response);
}

struct http_parser http_parser_init(const char* source, int length) {
    struct http_parser parser;

    parser.source = source;
    parser.length = length;

    parser.curr = source;
    parser.current_state = PARSER_START;

    return parser;
}

static int is_at_end(struct http_parser* parser) {
    return (parser->curr - parser->source) >= parser->length;
}

static char next_char(struct http_parser* parser) {
    return !is_at_end(parser)
        ? *parser->curr++
        : '\0';
}

static char peek(struct http_parser* parser) {
    return *parser->curr;
}

static int match(struct http_parser* parser, char c) {
    if(peek(parser) == c) {
        next_char(parser);
        return 1;
    }

    return 0;
}

static int match_chars(struct http_parser* parser, const char* chars) {

    int status = 1;
    
    while(*chars && status != 0) {
        if(!match(parser, *chars)) {
            status = 0;
        }

        chars++;
    }
    
    return status;
}

static void update_parser_state(struct http_parser* parser, 
                                enum http_parser_state state) {

    parser->prev_state = parser->current_state;
    parser->current_state = state;
}

static int parse_integer(struct http_parser* parser) {
    int number = 0;

    while(isdigit(peek(parser)) && !is_at_end(parser)) {
        number = (number * 10) + (next_char(parser)  - '0');
    }

    // TODO: check if is at the end

    return number;
}

static int is_valid_character(char c, int allow_all) {
    return allow_all 
        ? isprint(c)
        : (isalnum(c) || c == '-');
}

static void parse_string(struct http_parser* parser, int allow_all) {

    while(is_valid_character(peek(parser), allow_all) && !is_at_end(parser)) {
        next_char(parser);
    }
}

static char* allocate_string(struct http_parser* parser) {

    const char* end = parser->curr;
    const char* start = parser->start;

    const int length = (int)(end - start);

    char* string = (char*)malloc(sizeof(*string) * length + 1);
    if(string == NULL) {
        return NULL;
    }

    memcpy(string, start, length);
    string[length] = '\0';

    return string;
}

struct http_response* parse_response(struct http_parser* parser) {

    struct http_response* response = NULL;

    while (parser->current_state != PARSER_END) {

        switch (parser->current_state) {
            case PARSER_START:
                response = (struct http_response*)malloc(sizeof(*response));

                if(response == NULL) {
                    update_parser_state(parser, PARSER_ERROR);
                } else {
                    update_parser_state(parser, PARSER_HTTP);
                }

                break;
            case PARSER_SP:
                
                if(isspace(peek(parser))) {
                    next_char(parser);

                    enum http_parser_state next_state;

                    switch(parser->prev_state) {
                        case PARSER_HTTP_MINOR:
                            next_state = PARSER_RES_STATUS;
                            break;
                        case PARSER_RES_STATUS:
                            next_state = PARSER_RES_REASON;
                            break;
                        case PARSER_HEADER_VALUE_END:
                            next_state = PARSER_HEADER_START;
                            break;
                        case PARSER_HEADERS_DONE: // TODO: useless
                            next_state = PARSER_CRLF;
                            break;
                        default:
                            next_state = PARSER_ERROR;
                            break;
                    }

                    update_parser_state(parser, next_state);
                } else {
                    update_parser_state(parser, PARSER_ERROR);
                }

                break;
            case PARSER_CRLF:
                if(match_chars(parser, "\r\n")) {
                    enum http_parser_state next_state;

                    switch(parser->prev_state) {
                        case PARSER_CRLF:
                            next_state = PARSER_BODY_START;
                            break;
                        case PARSER_RES_REASON:
                            next_state = PARSER_HEADERS;
                            break;
                        case PARSER_HEADERS_DONE:
                            next_state = PARSER_BODY_START;
                            break;
                        default:
                            next_state = PARSER_ERROR;
                            break;
                    }

                    update_parser_state(parser, next_state);
                } else {
                    update_parser_state(parser, PARSER_ERROR);
                }
                break;
            case PARSER_HTTP:

                if(match_chars(parser, "HTTP")) {
                    update_parser_state(parser, PARSER_HTTP_SLASH);
                } else {
                    update_parser_state(parser, PARSER_ERROR);
                }

                break;
            case PARSER_HTTP_SLASH:
                update_parser_state(parser, match(parser, '/')
                                    ? PARSER_HTTP_MAJOR
                                    : PARSER_ERROR);
                break;
            case PARSER_HTTP_MAJOR:
                response->major_version = parse_integer(parser); // TODO: error checking
                update_parser_state(parser, PARSER_HTTP_DOT);
                break;
            case PARSER_HTTP_DOT:
                update_parser_state(parser, match(parser, '.')
                                    ? PARSER_HTTP_MINOR
                                    : PARSER_ERROR);
                break;
            case PARSER_HTTP_MINOR:
                response->minor_version = parse_integer(parser); // TODO: error checking
                update_parser_state(parser, PARSER_SP);
                break;
            case PARSER_RES_STATUS:
                response->status = parse_integer(parser); // TODO: error checking
                update_parser_state(parser, PARSER_SP);
                break;
            case PARSER_RES_REASON:
                while(peek(parser) != '\r' && !is_at_end(parser)) {
                    next_char(parser);
                }

                update_parser_state(parser, is_at_end(parser)
                                    ? PARSER_ERROR
                                    : PARSER_CRLF);
                break;
            case PARSER_REQ_METHOD:
                break;
            case PARSER_REQ_URI:
                break;
            case PARSER_HEADERS:
                update_parser_state(parser, PARSER_HEADER_START);
                break;
            case PARSER_HEADER_START: {

                if(peek(parser) == '\r') {
                    update_parser_state(parser, PARSER_HEADERS_DONE);
                    break;
                }

                struct http_header* header = new_header();

                if(header == NULL) {
                    update_parser_state(parser, PARSER_ERROR);
                    break;
                }


                // TODO: the headers will be saved in reversed order
                header->next = response->headers;
                response->headers = header;

                update_parser_state(parser, PARSER_HEADER_NAME_START);
                break;
            }
            case PARSER_HEADER_NAME_START:
                parser->start = parser->curr;
                update_parser_state(parser, PARSER_HEADER_NAME);
                break;
            case PARSER_HEADER_NAME:
                parse_string(parser, 0);                
                update_parser_state(parser, PARSER_HEADER_NAME_END);
                break;
            case PARSER_HEADER_NAME_END: {

                char* name = allocate_string(parser);

                if(name == NULL) {
                    update_parser_state(parser, PARSER_ERROR);
                } else {
                    response->headers->name = name;
                }

                update_parser_state(parser, PARSER_HEADER_COLON);
                break;
            }
            case PARSER_HEADER_COLON:
                update_parser_state(parser, match(parser, ':')
                                    ? PARSER_HEADER_VALUE_START
                                    : PARSER_ERROR);
                break;
            case PARSER_HEADER_VALUE_START:

                while(isspace(peek(parser)) && !is_at_end(parser)) {
                    next_char(parser);
                }

                parser->start = parser->curr;
                update_parser_state(parser, is_at_end(parser) 
                                    ? PARSER_ERROR 
                                    : PARSER_HEADER_VALUE);
                break;
            case PARSER_HEADER_VALUE:
                
                parse_string(parser, 1);
                update_parser_state(parser, PARSER_HEADER_VALUE_LWS);
                break;
            case PARSER_HEADER_VALUE_LWS: {
                
                if(match_chars(parser, "\r\n")) {
                    if(match(parser, ' ') || match(parser, '\t')) {
                        update_parser_state(parser, PARSER_HEADER_VALUE);
                    } else {
                        update_parser_state(parser, PARSER_HEADER_VALUE_END);
                    }
                } else {
                    update_parser_state(parser, PARSER_ERROR);
                }

                break;
            }
            case PARSER_HEADER_VALUE_END: {

                char* value = allocate_string(parser);

                if(value == NULL) {
                    update_parser_state(parser, PARSER_ERROR);
                } else {
                    response->headers->value = value;
                }

                update_parser_state(parser, PARSER_HEADER_START);
                break;
            }
            case PARSER_HEADERS_DONE:
                update_parser_state(parser, PARSER_CRLF);
                break;
            case PARSER_BODY_START:
                update_parser_state(parser, PARSER_BODY);
                break;
            case PARSER_BODY:

                parser->start = parser->curr;

                while(!is_at_end(parser)) {
                    next_char(parser);
                }

                update_parser_state(parser, PARSER_BODY_END);
                break;
            case PARSER_BODY_END: {

                char* body = allocate_string(parser);

                if(body == NULL) {
                    update_parser_state(parser, PARSER_ERROR);
                } else {
                    response->body = body;
                }

                update_parser_state(parser, PARSER_END);
                break;
            }
            case PARSER_ERROR:
                fprintf(stderr, "An error occurred!\n");
                if(response != NULL) {
                    destroy_http_response(response);
                    response = NULL;
                }

                update_parser_state(parser, PARSER_END);
                break;
            case PARSER_END:
                break;
        }
    }
   
   return response;
}
