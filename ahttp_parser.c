#include "ahttp_parser.h"

#ifdef __cplusplus
extern "C" {
#endif


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum http_parser_state {
    PARSER_START,

    PARSER_SP,
    PARSER_CRLF,

    PARSER_START_RES,
    PARSER_START_REQ,

    PARSER_HTTP,
    PARSER_HTTP_SLASH,
    PARSER_HTTP_MAJOR,
    PARSER_HTTP_DOT,
    PARSER_HTTP_MINOR,

    PARSER_RES_STATUS,
    PARSER_RES_REASON,

    PARSER_REQ_METHOD,
    PARSER_REQ_URI,

    PARSER_HEADERS,
    PARSER_HEADER_START,
    PARSER_HEADER_NAME_START,
    PARSER_HEADER_NAME,
    PARSER_HEADER_NAME_END,
    PARSER_HEADER_COLON,
    PARSER_HEADER_VALUE_START,
    PARSER_HEADER_VALUE,
    PARSER_HEADER_VALUE_LWS,
    PARSER_HEADER_VALUE_END,
    PARSER_HEADERS_DONE,

    PARSER_BODY_START,
    PARSER_BODY,

    PARSER_END,
} http_parser_state;

enum http_parser_error {
    PARSER_NO_ERROR,

    PARSER_EXPECT_SPACE,
    PARSER_EXPECT_CRLF,
    PARSER_INVALID_STATE,
    PARSER_MALFORMED_HTTP_VERSION,
    PARSER_INVALID_STATUS_CODE,
    PARSER_INVALID_HTTP_METHOD,
    PARSER_EXPECT_NUMBER,
    PARSER_EXPECT_COLON,
    PARSER_EXPECT_HEADER_VALUE
};

http_parser http_parser_init(const char* source, int length) {

    http_parser parser;

    parser.source = source;
    parser.length = length;

    parser.curr = source;
    parser.current_state = PARSER_START;

    parser.http_major = 0;
    parser.http_minor = 0;

    parser.status = -1;
    parser.method = HTTP_INVALID;

    parser.data = NULL;
    parser.errno = PARSER_NO_ERROR;

    return parser;
}

uint8_t parser_http_minor_version(const http_parser* restrict parser) {
    return parser->http_minor;
}

uint8_t parser_http_major_version(const http_parser* restrict parser) {
    return parser->http_major;
}

int parser_http_status_code(const http_parser* restrict parser) {
    return parser->status;
}

http_method parser_http_method(const http_parser* restrict parser) {
    return parser->method;
}

bool parser_had_error(const http_parser* restrict parser) {
    return parser->errno != PARSER_NO_ERROR;
}

const char* parser_get_error(const http_parser* restrict parser) {

    static const char *const http_parser_error_strings[] = {
        [PARSER_NO_ERROR] = "No error",
        [PARSER_EXPECT_SPACE] = "Expected a space character",
        [PARSER_EXPECT_CRLF] = "Expected carriage return (CR) or line feed (LF)",
        [PARSER_INVALID_STATE] = "Parser is in an invalid state",
        [PARSER_MALFORMED_HTTP_VERSION] = "HTTP version string is malformed (e.g., 'HTTP/x.y' expected)",
        [PARSER_INVALID_STATUS_CODE] = "Invalid HTTP status code",
        [PARSER_INVALID_HTTP_METHOD] = "Invalid HTTP method",
        [PARSER_EXPECT_NUMBER] = "Expected a number",
        [PARSER_EXPECT_COLON] = "Expected a colon character (':')",
        [PARSER_EXPECT_HEADER_VALUE] = "Expected a header value"
    };

    return http_parser_error_strings[parser->errno];
}

/* >>> Parser related functions  */

static inline bool is_at_end(const http_parser* restrict parser) {
    return (parser->curr - parser->source) >= parser->length;
}

static inline char next_char(http_parser* restrict parser) {
    return !is_at_end(parser)
        ? *parser->curr++
        : '\0';
}

static inline char peek(const http_parser* restrict parser) {
    return *parser->curr;
}

static inline bool match(http_parser* restrict parser, char c) {
    if(peek(parser) == c) {
        next_char(parser);
        return true;
    }

    return false;
}

static bool match_chars(http_parser* restrict parser, const char* chars) {

    while(*chars) {
        if(!match(parser, *chars)) {
            return false;
        }

        chars++;
    }
    
    return true;
}

static inline void update_parser_state(http_parser* restrict parser,
                                       http_parser_state state) {
    parser->prev_state = parser->current_state;
    parser->current_state = state;
}

static int parse_integer(http_parser* restrict parser, int* result) {

    parser->start = parser->curr;

    while(isdigit(peek(parser)) && !is_at_end(parser)) {
        *result = (*result * 10) + (next_char(parser)  - '0');
    }

    return parser->start != parser->curr;
}

static inline bool is_valid_character(char c, bool allow_all) {
    return allow_all 
        ? isprint(c)
        : (isalnum(c) || c == '-');
}

static void parse_string(http_parser* restrict parser, bool allow_all) {

    while(is_valid_character(peek(parser), allow_all) && !is_at_end(parser)) {
        next_char(parser);
    }
}

/* <<< End Parser related functions */

#define GET_PARSED_BYTES(parser) ((int)((parser)->curr - (parser)->source))
#define THROW_ERROR(parser, err) do {           \
        (parser)->errno = (err);              \
        return GET_PARSED_BYTES(parser);        \
    } while(0)

#define MARK_START(parser) (parser->start = parser->curr)
#define CALC_DATA_LENGTH(parser) ((int)((parser)->curr - (parser)->start))

int http_parser_run(http_parser* restrict parser,
                    void* data,
                    http_parser_settings* settings,
                    http_parser_type type) {

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

                    http_parser_state next_state;

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
                        default:
                            THROW_ERROR(parser, PARSER_INVALID_STATE);
                            break;
                    }

                    update_parser_state(parser, next_state);
                } else {
                    THROW_ERROR(parser, PARSER_EXPECT_SPACE);
                }

                break;
            case PARSER_CRLF:
                if(match_chars(parser, "\r\n")) {
                    http_parser_state next_state;

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
                            THROW_ERROR(parser, PARSER_INVALID_STATE);
                            break;
                    }

                    update_parser_state(parser, next_state);
                } else {
                    THROW_ERROR(parser, PARSER_EXPECT_CRLF);
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
                    THROW_ERROR(parser, PARSER_MALFORMED_HTTP_VERSION);
                }

                break;
            case PARSER_HTTP_SLASH:
                if(match(parser, '/')) {
                    update_parser_state(parser, PARSER_HTTP_MAJOR);
                } else {
                    THROW_ERROR(parser, PARSER_MALFORMED_HTTP_VERSION);
                }

                break;
            case PARSER_HTTP_MAJOR:

                if(!isdigit(peek(parser))) {
                    THROW_ERROR(parser, PARSER_MALFORMED_HTTP_VERSION);
                }

                parser->http_major = next_char(parser) - '0';
                update_parser_state(parser, PARSER_HTTP_DOT);
                break;
            case PARSER_HTTP_DOT:
                if(match(parser, '.')){
                    update_parser_state(parser, PARSER_HTTP_MINOR);
                } else {
                    THROW_ERROR(parser, PARSER_MALFORMED_HTTP_VERSION);
                }

                break;
            case PARSER_HTTP_MINOR:

                if(!isdigit(peek(parser))) {
                    THROW_ERROR(parser, PARSER_MALFORMED_HTTP_VERSION);
                }

                parser->http_minor = next_char(parser) - '0';
                update_parser_state(parser, is_request ? PARSER_CRLF : PARSER_SP);
                break;
            case PARSER_RES_STATUS:
                if(parse_integer(parser, &parser->status)) {
                    update_parser_state(parser, PARSER_SP);
                } else {
                    THROW_ERROR(parser, PARSER_INVALID_STATUS_CODE);
                }

                break;
            case PARSER_RES_REASON:
                while(peek(parser) != '\r' && !is_at_end(parser)) {
                    next_char(parser);
                }

                if(is_at_end(parser)) {
                    THROW_ERROR(parser, PARSER_EXPECT_CRLF);
                }

                update_parser_state(parser, PARSER_CRLF);
                break;
            case PARSER_REQ_METHOD: {
                char c = next_char(parser);

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
                            parser->method = match_chars(parser, "ST")
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
                    THROW_ERROR(parser, PARSER_INVALID_HTTP_METHOD);
                }

                break;
            }
            case PARSER_REQ_URI:

                MARK_START(parser);
                while(peek(parser) != ' ' && !is_at_end(parser)) {
                    next_char(parser);
                }

                if(is_at_end(parser)) {
                    THROW_ERROR(parser, PARSER_EXPECT_SPACE);
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
                parse_string(parser, /* allow_all */ false);
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
                if(match(parser, ':')) {
                    update_parser_state(parser, PARSER_HEADER_VALUE_START);
                } else {
                    THROW_ERROR(parser, PARSER_EXPECT_COLON);
                }

                break;
            case PARSER_HEADER_VALUE_START:

                while(isspace(peek(parser)) && !is_at_end(parser)) {
                    next_char(parser);
                }


                if(is_at_end(parser)) {
                    THROW_ERROR(parser, PARSER_EXPECT_HEADER_VALUE);
                }

                MARK_START(parser);
                update_parser_state(parser, PARSER_HEADER_VALUE);
                break;
            case PARSER_HEADER_VALUE:
                parse_string(parser, /* allow_all */ true);
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
                    THROW_ERROR(parser, PARSER_EXPECT_CRLF);
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
            case PARSER_END:
                break;
        }
    }
   
    return GET_PARSED_BYTES(parser);
}

#ifdef __cplusplus
}
#endif
