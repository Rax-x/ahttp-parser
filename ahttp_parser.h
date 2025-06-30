#ifndef _AHTTP_PARSER_H_
#define _AHTTP_PARSER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum http_method {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
};

struct http_header {
    char* name;
    char* value;

    struct http_header* next;
};

struct http_request {
    enum http_method method;

    uint8_t major_version;
    uint8_t minor_version;

    struct http_header* headers;

    char* body;
};

struct http_response {
    int status;

    uint8_t major_version;
    uint8_t minor_version;

    struct http_header* headers;

    char* body;
};

enum http_parser_state {
    PARSER_START,

    PARSER_SP,
    PARSER_CRLF,

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
    PARSER_BODY_END,

    PARSER_ERROR,
    PARSER_END,
};

struct http_parser {
    const char* source;
    int length;

    const char* start;
    const char* curr;

    enum http_parser_state current_state;
    enum http_parser_state prev_state;
};

struct http_parser http_parser_init(const char* source, int length);

struct http_response* parse_response(struct http_parser* parser);

void destroy_http_request(struct http_request* request);
void destroy_http_response(struct http_response* response);

#ifdef __cplusplus
}
#endif


#endif
