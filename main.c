#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ahttp_parser.h"

const char* mock_response = 
    "HTTP/1.1 200 OK\r\n"                               // Status Line
    "Date: Fri, 28 Jun 2025 10:30:00 GMT\r\n"           // Additional Header: Date
    "Server: MockWebServer/1.0 (Unix)\r\n"             // Additional Header: Server
    "Content-Type: text/html; charset=utf-8\r\n"        // Changed Content-Type to HTML
    "Content-Length: 153\r\n"                           // Updated Content-Length for new body
    "Connection: close\r\n"                             // Header 3
    "Cache-Control: public, max-age=3600\r\n"           // Additional Header: Cache-Control
    "Set-Cookie: session_id=abcde12345; Path=/; HttpOnly\r\n" // Additional Header: Set-Cookie
    "X-SSL-Nonsense:   -----BEGIN CERTIFICATE-----\r\n"
    "\tMIIFbTCCBFWgAwIBAgICH4cwDQYJKoZIhvcNAQEFBQAwcDELMAkGA1UEBhMCVUsx\r\n"
    "\tETAPBgNVBAoTCGVTY2llbmNlMRIwEAYDVQQLEwlBdXRob3JpdHkxCzAJBgNVBAMT\r\n"
    "\tAkNBMS0wKwYJKoZIhvcNAQkBFh5jYS1vcGVyYXRvckBncmlkLXN1cHBvcnQuYWMu\r\n"
    "\tdWswHhcNMDYwNzI3MTQxMzI4WhcNMDcwNzI3MTQxMzI4WjBbMQswCQYDVQQGEwJV\r\n"
    "\tSzERMA8GA1UEChMIZVNjaWVuY2UxEzARBgNVBAsTCk1hbmNoZXN0ZXIxCzAJBgNV\r\n"
    "\tBAcTmrsogriqMWLAk1DMRcwFQYDVQQDEw5taWNoYWVsIHBhcmQYJKoZIhvcNAQEB\r\n"
    "\tBQADggEPADCCAQoCggEBANPEQBgl1IaKdSS1TbhF3hEXSl72G9J+WC/1R64fAcEF\r\n"
    "\tW51rEyFYiIeZGx/BVzwXbeBoNUK41OK65sxGuflMo5gLflbwJtHBRIEKAfVVp3YR\r\n"
    "\tgW7cMA/s/XKgL1GEC7rQw8lIZT8RApukCGqOVHSi/F1SiFlPDxuDfmdiNzL31+sL\r\n"
    "\t0iwHDdNkGjy5pyBSB8Y79dsSJtCW/iaLB0/n8Sj7HgvvZJ7x0fr+RQjYOUUfrePP\r\n"
    "\tu2MSpFyf+9BbC/aXgaZuiCvSR+8Snv3xApQY+fULK/xY8h8Ua51iXoQ5jrgu2SqR\r\n"
    "\twgA7BUi3G8LFzMBl8FRCDYGUDy7M6QaHXx1ZWIPWNKsCAwEAAaOCAiQwggIgMAwG\r\n"
    "\tA1UdEwEB/wQCMAAwEQYJYIZIAYb4QgHTTPAQDAgWgMA4GA1UdDwEB/wQEAwID6DAs\r\n"
    "\tBglghkgBhvhCAQ0EHxYdVUsgZS1TY2llbmNlIFVzZXIgQ2VydGlmaWNhdGUwHQYD\r\n"
    "\tVR0OBBYEFDTt/sf9PeMaZDHkUIldrDYMNTBZMIGaBgNVHSMEgZIwgY+AFAI4qxGj\r\n"
    "\tloCLDdMVKwiljjDastqooXSkcjBwMQswCQYDVQQGEwJVSzERMA8GA1UEChMIZVNj\r\n"
    "\taWVuY2UxEjAQBgNVBAsTCUF1dGhvcml0eTELMAkGA1UEAxMCQ0ExLTArBgkqhkiG\r\n"
    "\t9w0BCQEWHmNhLW9wZXJhdG9yQGdyaWQtc3VwcG9ydC5hYy51a4IBADApBgNVHRIE\r\n"
    "\tIjAggR5jYS1vcGVyYXRvckBncmlkLXN1cHBvcnQuYWMudWswGQYDVR0gBBIwEDAO\r\n"
    "\tBgwrBgEEAdkvAQEBAQYwPQYJYIZIAYb4QgEEBDAWLmh0dHA6Ly9jYS5ncmlkLXN1\r\n"
    "\tcHBvcnQuYWMudmT4sopwqlBWsvcHViL2NybC9jYWNybC5jcmwwPQYJYIZIAYb4QgEDBDAWLmh0\r\n"
    "\tdHA6Ly9jYS5ncmlkLXN1cHBvcnQuYWMudWsvcHViL2NybC9jYWNybC5jcmwwPwYD\r\n"
    "\tVR0fBDgwNjA0oDKgMIYuaHR0cDovL2NhLmdyaWQt5hYy51ay9wdWIv\r\n"
    "\tY3JsL2NhY3JsLmNybDANBgkqhkiG9w0BAQUFAAOCAQEAS/U4iiooBENGW/Hwmmd3\r\n"
    "\tXCy6Zrt08YjKCzGNjorT98g8uGsqYjSxv/hmi0qlnlHs+k/3Iobc3LjS5AMYr5L8\r\n"
    "\tUO7OSkgFFlLHQyC9JzPfmLCAugvzEbyv4Olnsr8hbxF1MbKZoQxUZtMVu29wjfXk\r\n"
    "\thTeApBv7eaKCWpSp7MCbvgzm74izKhu3vlDk9w6qVrxePfGgpKPqfHiOoGhFnbTK\r\n"
    "\twTC6o2xq5y0qZ03JonF7OJspEd3I5zKY3E+ov7/ZhW6DqT8UFvsAdjvQbXyhV8Eu\r\n"
    "\tYhixw1aKEPzNjNowuIseVogKOLXxWI5vAi5HgXdS0/ES5gDGsABo4fqovUKlgop3\r\n"
    "\tRA==\r\n"
    "\t-----END CERTIFICATE-----\r\n"
    "\r\n"                                              // Empty line separating headers from body
    "<!DOCTYPE html>\n"                                 // More complex HTML body
    "<html lang=\"en\">\n"
    "<head><title>Welcome</title></head>\n"
    "<body>\n"
    "    <h1>Hello, Mock World!</h1>\n"
    "    <p>This is a more complex HTTP response for testing purposes.</p>\n"
    "</body>\n"
    "</html>\n";

struct http_header {
    char* name;
    char* value;

    struct http_header* next;
};

struct http_header* new_header() {
    struct http_header* header = (struct http_header*)malloc(sizeof(*header));

    header->name = NULL;
    header->value = NULL;
    header->next = NULL;

    return header;
}

void destroy_header(struct http_header* header) {
    free(header->name);
    free(header->value);

    free(header);
}

struct http_response {

    int major_version;
    int minor_version;
    int status;

    struct http_header* headers;
    char* body;
};

void destroy_http_response(struct http_response* response) {
    struct http_header* header = response->headers;
    struct http_header* temp;

    while(header != NULL) {
        temp = header;
        header = header->next;

        destroy_header(temp);
    }

    free(response->body);
}

void on_new_header(struct http_parser* parser) {
    struct http_response* response = (struct http_response*)parser->data;

    struct http_header* header = new_header();
    header->next = response->headers;
    response->headers = header;
}

void on_headers_done(struct http_parser* parser) {
    puts("Headers finished");
}

void on_header_name(struct http_parser* parser, const char* name, int length) {

    struct http_response* response = (struct http_response*)parser->data;

    response->headers->name = (char*)malloc(sizeof(char) * length + 1);
    memcpy(response->headers->name, name, length);
    response->headers->name[length] = '\0';
}

void on_header_value(struct http_parser* parser, const char* name, int length) {

    struct http_response* response = (struct http_response*)parser->data;

    response->headers->value = (char*)malloc(sizeof(char) * length + 1);
    memcpy(response->headers->value, name, length);
    response->headers->value[length] = '\0';
}

void on_body(struct http_parser* parser, const char* name, int length) {
    struct http_response* response = (struct http_response*)parser->data;

    response->body = (char*)malloc(sizeof(char) * length + 1);
    memcpy(response->body, name, length);
    response->body[length] = '\0';
}

int main() {
    
    struct http_response response = {0};

    struct http_parser parser = http_parser_init(mock_response, 
                                                 strlen(mock_response));

    struct http_parser_settings settings = {NULL,
                                                 on_new_header, 
                                                 on_header_name, 
                                                 on_header_value,
                                                 on_headers_done,
                                                 on_body};

    http_parser_run(&parser, &response, &settings, HTTP_PARSER_RESPONSE);
    if(parser_had_error(&parser)) {
        printf("Error: %s\n", parser_get_error(&parser));
        exit(EXIT_FAILURE);
    }

                     
    puts(response.body);
    for(struct http_header* header = response.headers; 
        header != NULL; 
        header = header->next) {
        printf("%s: %s\n", header->name, header->value);
    }
    
    printf("Method: %d, major: %d, minor: %d\n",
           parser_http_method(&parser),
           parser_http_major_version(&parser),
           parser_http_minor_version(&parser));

    destroy_http_response(&response);

    return 0;
}
