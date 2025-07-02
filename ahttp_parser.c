#include "ahttp_parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct http_parser http_parser_init(const char* source, int length) {

    struct http_parser parser;

    parser.source = source;
    parser.length = length;

    parser.curr = source;
    parser.current_state = PARSER_START;

    parser.http_major = 0;
    parser.http_minor = 0;

    parser.status = -1;
    parser.method = HTTP_INVALID;

    parser.data = NULL;
    parser.error = 0;

    return parser;
}

inline int http_parser_minor_version(struct http_parser* parser) {
    return parser->http_minor;
}

inline int http_parser_major_version(struct http_parser* parser) {
    return parser->http_major;
}

inline short int http_parser_status_code(struct http_parser* parser) {
    return parser->status;
}

inline enum http_method http_parser_method(struct http_parser* parser) {
    return parser->method;
}

static inline int is_at_end(struct http_parser* parser) {
    return (parser->curr - parser->source) >= parser->length;
}

static char next_char(struct http_parser* parser) {
    return !is_at_end(parser)
        ? *parser->curr++
        : '\0';
}

static inline char peek(struct http_parser* parser) {
    return *parser->curr;
}

static inline int match(struct http_parser* parser, char c) {
    if(peek(parser) == c) {
        next_char(parser);
        return 1;
    }

    return 0;
}

static int match_chars(struct http_parser* parser, const char* chars) {

    while(*chars) {
        if(!match(parser, *chars)) {
            return 0;
        }

        chars++;
    }
    
    return 1;
}

static inline void update_parser_state(struct http_parser* parser, 
                                       enum http_parser_state state) {
    parser->prev_state = parser->current_state;
    parser->current_state = state;
}

static int parse_integer(struct http_parser* parser) {
    int number = 0;

    while(isdigit(peek(parser)) && !is_at_end(parser)) {
        number = (number * 10) + (next_char(parser)  - '0');
    }

    return number;
}

static inline int is_valid_character(char c, int allow_all) {
    return allow_all 
        ? isprint(c)
        : (isalnum(c) || c == '-');
}

static void parse_string(struct http_parser* parser, int allow_all) {

    while(is_valid_character(peek(parser), allow_all) && !is_at_end(parser)) {
        next_char(parser);
    }
}

#define SET_ERROR(p) (update_parser_state((p), PARSER_ERROR))
#define MARK_START(parser) (parser->start = parser->curr)
#define CALC_DATA_LENGTH(parser) ((int)((parser)->curr - (parser)->start))

int http_parser_run(struct http_parser* parser, void* data,
                    struct http_parser_settings* settings, 
                    enum http_parser_type type) {

    const int is_request = (type == HTTP_PARSER_REQUEST);
    parser->data = data;

    while (parser->current_state != PARSER_END) {

        switch (parser->current_state) {
            case PARSER_START:
                update_parser_state(parser, is_request
                                    ? PARSER_START_REQ
                                    : PARSER_START_RES);
                break;
            case PARSER_SP:
                
                if(match(parser, ' ')) {

                    enum http_parser_state next_state;

                    switch(parser->prev_state) {
                        case PARSER_HTTP_MINOR:
                            next_state = PARSER_RES_STATUS;
                            break;
                        case PARSER_RES_STATUS:
                            next_state = PARSER_RES_REASON;
                            break;
                        case PARSER_REQ_METHOD:
                            next_state = PARSER_REQ_URI;
                            break;
                        case PARSER_REQ_URI:
                            next_state = PARSER_HTTP;
                            break;
                        case PARSER_HEADER_VALUE_END:
                            next_state = PARSER_HEADER_START;
                            break;
                        case PARSER_HEADERS_DONE: // TODO(Rax): useless
                            next_state = PARSER_CRLF;
                            break;
                        default:
                            next_state = PARSER_ERROR;
                            break;
                    }

                    update_parser_state(parser, next_state);
                } else {
                    SET_ERROR(parser);
                }

                break;
            case PARSER_CRLF:
                if(match_chars(parser, "\r\n")) {
                    enum http_parser_state next_state;

                    switch(parser->prev_state) {
                        case PARSER_CRLF:
                            next_state = PARSER_BODY_START;
                            break;
                        case PARSER_HTTP_MINOR:
                            // fallthrought
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
                    SET_ERROR(parser);
                }
                break;
            case PARSER_START_RES:
                update_parser_state(parser, PARSER_HTTP);
                break;
            case PARSER_START_REQ:
                update_parser_state(parser, PARSER_REQ_METHOD);
                break;
            case PARSER_HTTP:

                if(match_chars(parser, "HTTP")) {
                    update_parser_state(parser, PARSER_HTTP_SLASH);
                } else {
                    SET_ERROR(parser);
                }

                break;
            case PARSER_HTTP_SLASH:
                update_parser_state(parser, match(parser, '/')
                                    ? PARSER_HTTP_MAJOR
                                    : PARSER_ERROR);
                break;
            case PARSER_HTTP_MAJOR:
                parser->http_major = parse_integer(parser); // TODO(Rax): error checking
                update_parser_state(parser, PARSER_HTTP_DOT);
                break;
            case PARSER_HTTP_DOT:
                update_parser_state(parser, match(parser, '.')
                                    ? PARSER_HTTP_MINOR
                                    : PARSER_ERROR);
                break;
            case PARSER_HTTP_MINOR:
                parser->http_minor = parse_integer(parser); // TODO(Rax): error checking
                update_parser_state(parser, is_request ? PARSER_CRLF : PARSER_SP);
                break;
            case PARSER_RES_STATUS:
                parser->status = parse_integer(parser); // TODO(Rax): error checking
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
            case PARSER_REQ_METHOD: {
                char c = next_char(parser);

                // TODO(Rax): this could be faster

                switch(c) {
                    case 'C':
                        parser->method = match_chars(parser, "ONNECT")
                            ? HTTP_CONNECT
                            : HTTP_INVALID;
                        break;
                    case 'D':
                        parser->method = match_chars(parser, "ELETE")
                            ? HTTP_DELETE
                            : HTTP_INVALID;

                        break;
                    case 'G':
                        parser->method = match_chars(parser, "ET")
                            ? HTTP_GET
                            : HTTP_INVALID;

                        break;
                    case 'H':
                        parser->method = match_chars(parser, "EAD")
                            ? HTTP_HEAD
                            : HTTP_INVALID;
                        break;
                    case 'O':
                        parser->method = match_chars(parser, "PTIONS")
                            ? HTTP_OPTIONS
                            : HTTP_INVALID;
                        break;
                    case 'P':
                        c = next_char(parser);
                        
                        if(c == 'U') {
                            parser->method = match(parser, 'T')
                                ? HTTP_PUT
                                : HTTP_INVALID;
                        } else {
                            parser->method = match_chars(parser, "OST")
                                ? HTTP_POST
                                : HTTP_INVALID;
                        }

                        break;
                    case 'T':
                        parser->method = match_chars(parser, "RACE")
                            ? HTTP_TRACE
                            : HTTP_INVALID;
                        break;
                    default:
                        parser->method = HTTP_INVALID;
                        break;
                }

                if(parser->method != HTTP_INVALID) {
                    update_parser_state(parser, PARSER_SP);
                } else {
                    SET_ERROR(parser);
                }

                break;
            }
            case PARSER_REQ_URI:

                MARK_START(parser);
                while(peek(parser) != ' ') {
                    next_char(parser);
                }

                if(settings->on_req_uri != NULL) {
                    settings->on_req_uri(parser, parser->start, CALC_DATA_LENGTH(parser));
                }

                update_parser_state(parser, PARSER_SP);
                break;
            case PARSER_HEADERS:
                update_parser_state(parser, PARSER_HEADER_START);
                break;
            case PARSER_HEADER_START: {

                if(peek(parser) == '\r') {
                    update_parser_state(parser, PARSER_HEADERS_DONE);
                    break;
                }

                if(settings->on_header != NULL) {
                    settings->on_header(parser);
                }

                update_parser_state(parser, PARSER_HEADER_NAME_START);
                break;
            }
            case PARSER_HEADER_NAME_START:
                MARK_START(parser);
                update_parser_state(parser, PARSER_HEADER_NAME);
                break;
            case PARSER_HEADER_NAME:
                parse_string(parser, 0);                
                update_parser_state(parser, PARSER_HEADER_NAME_END);
                break;
            case PARSER_HEADER_NAME_END: {

                const int header_name_length = CALC_DATA_LENGTH(parser);
                
                if(settings->on_header_name != NULL) {
                    settings->on_header_name(parser, parser->start, header_name_length);
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

                MARK_START(parser);
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
                    SET_ERROR(parser);
                }

                break;
            }
            case PARSER_HEADER_VALUE_END: {

                const int header_value_length = CALC_DATA_LENGTH(parser) - 2; // exclude \r\n

                if(settings->on_header_value != NULL) {
                    settings->on_header_value(parser, parser->start, header_value_length);
                }

                update_parser_state(parser, PARSER_HEADER_START);
                break;
            }
            case PARSER_HEADERS_DONE:

                if(settings->on_headers_done != NULL) {
                    settings->on_headers_done(parser);
                }

                update_parser_state(parser, PARSER_CRLF);
                break;
            case PARSER_BODY_START:
                MARK_START(parser);
                update_parser_state(parser, PARSER_BODY);
                break;
            case PARSER_BODY: {

                const int body_length = parser->length - (int)(parser->start - parser->source);
                parser->curr += body_length; // it reaches the end of the buffer

                if(settings->on_body != NULL) {
                    settings->on_body(parser, parser->start, body_length);
                }

                update_parser_state(parser, PARSER_END);
                break;
            }
            case PARSER_ERROR:
                fprintf(stderr, "An error occurred!\n");
                parser->error = 1;
                update_parser_state(parser, PARSER_END);
                break;
            case PARSER_END:
                break;
        }
    }
   
    return (int)(parser->curr - parser->source);
}
