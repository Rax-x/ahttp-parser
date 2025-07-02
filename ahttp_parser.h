#ifndef _AHTTP_PARSER_H_
#define _AHTTP_PARSER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum http_method {
    HTTP_INVALID = -1,

    HTTP_OPTIONS,
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_TRACE,
    HTTP_CONNECT
};

enum http_parser_state {
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

    PARSER_ERROR,
    PARSER_END,
};

struct http_parser;

typedef void (*ahttp_event_cb)(struct http_parser*);
typedef void (*ahttp_data_cb)(struct http_parser*, const char* at, int length);

enum http_parser_type {
    HTTP_PARSER_RESPONSE,
    HTTP_PARSER_REQUEST
};

struct http_parser {
    const char* source;
    int length;

    const char* start;
    const char* curr;

    enum http_parser_state current_state;
    enum http_parser_state prev_state;

    short int http_major;
    short int http_minor;

    int status; // only response
    enum http_method method; // only request

    unsigned int error: 1;

    void* data;
};

struct http_parser_settings {
    ahttp_data_cb on_req_uri;

    ahttp_event_cb on_header;
    ahttp_data_cb on_header_name;
    ahttp_data_cb on_header_value;
    ahttp_event_cb on_headers_done;

    ahttp_data_cb on_body;
};

int http_parser_minor_version(struct http_parser* parser);
int http_parser_major_version(struct http_parser* parser);
short int http_parser_status_code(struct http_parser* parser);
enum http_method http_parser_method(struct http_parser* parser);

struct http_parser http_parser_init(const char* source, int length);

int http_parser_run(struct http_parser* parser, 
                    void* data,
                    struct http_parser_settings* settings,
                    enum http_parser_type type);

#ifdef __cplusplus
}
#endif


#endif
