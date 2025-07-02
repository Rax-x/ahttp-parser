#ifndef _AHTTP_PARSER_H_
#define _AHTTP_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum http_method {
    HTTP_INVALID = -1,

    HTTP_OPTIONS,
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_TRACE,
    HTTP_CONNECT
} http_method;

typedef struct http_parser http_parser;

typedef void (*ahttp_event_cb)(http_parser* parser);
typedef void (*ahttp_data_cb)(http_parser* parser, const char* at, int length);

typedef enum http_parser_type {
    HTTP_PARSER_RESPONSE,
    HTTP_PARSER_REQUEST
} http_parser_type;

struct http_parser {
    const char* source;
    int length;

    const char* start;
    const char* curr;

    uint8_t current_state;
    uint8_t prev_state;

    uint8_t http_major;
    uint8_t http_minor;

    int status; // only response
    http_method method; // only request

    uint8_t errno;

    void* data;
};

typedef struct http_parser_settings {
    ahttp_data_cb on_req_uri;

    ahttp_event_cb on_header;
    ahttp_data_cb on_header_name;
    ahttp_data_cb on_header_value;
    ahttp_event_cb on_headers_done;

    ahttp_data_cb on_body;
} http_parser_settings;

int parser_http_minor_version(http_parser* parser);
int parser_http_major_version(http_parser* parser);
short int parser_http_status_code(http_parser* parser);
http_method parser_http_method(http_parser* parser);

http_parser http_parser_init(const char* source, int length);

int http_parser_run(http_parser* parser, 
                    void* data,
                    http_parser_settings* settings,
                    http_parser_type type);

bool parser_had_error(http_parser* parser);
const char* parser_get_error(http_parser* parser);

#ifdef __cplusplus
}
#endif


#endif
