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

    PARSER_ERROR,
    PARSER_END,
};

struct http_parser;

typedef void (*ahttp_event_cb)(struct http_parser*);
typedef void (*ahttp_data_cb)(struct http_parser*, const char* at, int length);

struct http_parser {
    const char* source;
    int length;

    const char* start;
    const char* curr;

    enum http_parser_state current_state;
    enum http_parser_state prev_state;

    ahttp_event_cb on_header;
    ahttp_event_cb on_headers_done;

    ahttp_data_cb on_header_name;
    ahttp_data_cb on_header_value;
    ahttp_data_cb on_body;

    short int http_major;
    short int http_minor;
    short int status;

    unsigned int error: 1;

    void* data;
};

int http_parser_minor_version(struct http_parser* parser);
int http_parser_major_version(struct http_parser* parser);

struct http_parser http_parser_init(const char* source, 
                                    int length,
                                    void* data,
                                    ahttp_event_cb on_header,
                                    ahttp_data_cb on_header_name,
                                    ahttp_data_cb on_header_value,
                                    ahttp_event_cb on_headers_done,
                                    ahttp_data_cb on_body);

int http_parser_run(struct http_parser* parser);

#ifdef __cplusplus
}
#endif


#endif
