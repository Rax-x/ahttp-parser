# ahttp-parser

I built this library because sometimes I work with **Arduino** and often there's a need to parse **HTTP** messages.
Many existing libraries on **the internet** either offer too many features (**too much overhead**), 
or they are simply not easy to use or integrate **into** projects.

This library is written in **C99** and **implements** just the basic features, following **RFC 2616**.

>[!WARNING]
> This library is still **WORKING IN PROGRESS**!

## Features
- Parses both HTTP **Response** and **Request** messages.
- **Event-driven** design using callbacks for flexible handling of parsed data (headers, body, etc.).
- Small footprint and minimal dependencies.
- Follows the **RFC 2616** standard.

## Example
```c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ahttp_parser.h"

const char* mock_response =
    "HTTP/1.1 200 OK\r\n"
    "Date: Wed, 23 Jun 2024 12:00:00 GMT\r\n"
    "Server: Apache\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Length: 1234\r\n"
    "\r\n" // Empty line separating headers from body
    "<html>\r\n"
    "<body>\r\n"
    "  <h1>Hello, World!</h1>\r\n"
    "</body>\r\n"
    "</html>";

typedef struct http_header {
  char* name;
  char* value;

  struct http_header* next;
} http_header;

http_header* new_header() { ... }
void destroy_header(http_header* header) { ... }

typedef struct http_response {
  http_header* headers;
  char* body;
} http_response;

void destroy_http_response(http_response* response) { ... }

void on_new_header(http_parser* parser) {
  http_response* response = (http_response*)parser->data;

  http_header* header = new_header();

  header->next = response->headers;
  response->headers = header;
}

void on_headers_done(http_parser* parser) {
  puts("Headers finished");
}

void on_header_name(struct http_parser* parser, const char* at, int length) {

  http_response* response = (http_response*)parser->data;

  response->headers->name = (char*)malloc(sizeof(char) * length + 1);
  memcpy(response->headers->name, at, length);
  response->headers->name[length] = '\0';
}

void on_header_value(http_parser* parser, const char* at, int length) {

  http_response* response = (http_response*)parser->data;

  response->headers->value = (char*)malloc(sizeof(char) * length + 1);
  memcpy(response->headers->value, at, length);
  response->headers->value[length] = '\0';
}

void on_body(http_parser* parser, const char* at, int length) {
  http_response* response = (http_response*)parser->data;

  response->body = (char*)malloc(sizeof(char) * length + 1);
  memcpy(response->body, at, length);
  response->body[length] = '\0';
}

int main() {
    
  http_response response = {0};

  http_parser parser = http_parser_init(mock_response, strlen(mock_response));
  http_parser_settings settings = {
    NULL,
    on_new_header,
    on_header_name,
    on_header_value,
    on_headers_done,
    on_body
  };

  http_parser_run(&parser, &response, &settings, HTTP_PARSER_RESPONSE);
  if(parser_had_error(&parser)) {
    printf("Error: %s\n", parser_get_error(&parser));
    exit(EXIT_FAILURE);
  }

  puts(response.body);
  for(http_header* header = response.headers; 
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

```

# API

## Data Types

```c 
  typedef enum http_method { ... } http_method;
```
Represents the standard HTTP request methods.

- `HTTP_INVALID` (Indicates an unrecognized or invalid method)
- `HTTP_OPTIONS`
- `HTTP_GET`
- `HTTP_HEAD`
- `HTTP_POST`
- `HTTP_PUT`
- `HTTP_DELETE`
- `HTTP_TRACE`
- `HTTP_CONNECT`

---

```c
  typedef void (*ahttp_event_cb)(http_parser* parser);
```

Callback function type for events that don't involve data.

- `parser`: A pointer to the current http_parser instance.

---

```c
  typedef void (*ahttp_data_cb)(http_parser* parser, const char* at, int length);
```

Callback function type for events that provide a segment of parsed data (e.g., header name, body chunk).

**Parameters**:
- ```parser```: A pointer to the current ```http_parser``` instance.
- ```at```: A pointer to the beginning of the parsed data segment within the source buffer.
- ```length```: The length of the parsed data segment.

---

```c
  typedef enum http_parser_type { ... } http_parser_type;
```

Specifies the type of HTTP message the parser should expect.

- ```HTTP_PARSER_RESPONSE```: Parse an HTTP response (e.g., HTTP/1.1 200 OK...).
- ```HTTP_PARSER_REQUEST```: Parse an HTTP request (e.g., GET / HTTP/1.1...).

--- 

```c
  typedef struct http_parser_settings { ... } http_parser_settings;
```

A structure to hold all callback pointers for a parser instance. This allows for sharing callback configurations.

- `ahttp_data_cb on_req_uri`: Called when the request URI (e.g., /index.html) is parsed. **(Request only)**
- `ahttp_event_cb on_header`: Called at the start of any header.
- `ahttp_data_cb on_header_name`: Called when a header field name (e.g., Content-Type) is parsed.
- `ahttp_data_cb on_header_value`: Called when a header field value (e.g., text/html) is parsed.
- `ahttp_event_cb on_headers_done`: Called when all headers have been parsed.
- `ahttp_data_cb on_body`: Called when the body is parsed.

## Functions

```c
  http_parser http_parser_init(const char* source, int length);
``` 
Initializes a new `http_parser` instance.

**Parameters**:
- `source`: A pointer to the beginning of the raw HTTP message data. This buffer must remain valid throughout the parsing process.
- `length`: The total length of the source buffer.

**Returns**: An initialized `http_parser` struct.

---

```c
  int http_parser_run(http_parser* parser, void* data, http_parser_settings* settings, http_parser_type type);
```

Executes the parsing process.

**Parameters**:
- `parser`: A pointer to the `http_parser` instance to run.
- `data`: An optional `void*` pointer to user-defined data. This pointer will be accessible within your callbacks via `parser->data`.
- `settings`: A pointer to the `http_parser_settings` struct containing your callback functions.
- `type`: Specifies whether to parse a `HTTP_PARSER_RESPONSE` or `HTTP_PARSER_REQUEST`.

**Returns**: The numbers of parsed bytes.

---

```c
  bool parser_had_error(http_parser* parser);
```

Checks if the parser encountered an error during its last `http_parser_run` call.

**Parameters**:
- `parser`: A pointer to the `http_parser` instance.

**Returns**: true if an error occurred, false otherwise.

---

```c
  const char* parser_get_error(http_parser* parser);
```
Retrieves a human-readable string representation of the last error that occurred in the parser.

**Parameters**:
- `parser`: A pointer to the `http_parser` instance.

**Returns**: A C-string describing the error. Returns "No error" if `parser_had_error()` is false.

---

```c
  int parser_http_major_version(http_parser* parser);
```

Retrieves the HTTP major version number from a parsed message.

**Parameters**:
- `parser`: A pointer to the `http_parser` instance.

**Returns**: The HTTP major version (e.g., 1 for HTTP/1.1).

---

```c
  int parser_http_minor_version(http_parser* parser);
```

Retrieves the HTTP minor version number from a parsed message.

**Parameters**:
- `parser`: A pointer to the `http_parser` instance.

**Returns**: The HTTP minor version (e.g., 1 for HTTP/1.1).

---

```
  short int parser_http_status_code(http_parser* parser);
```
Retrieves the HTTP status code from a parsed response message.

**Parameters**:
`parser`: A pointer to the `http_parser` instance.

**Returns**: The HTTP status code (e.g., 200, 404). **(Only valid for `HTTP_PARSER_RESPONSE` type)**

---

```c
  http_method parser_http_method(http_parser* parser);
```

Retrieves the HTTP method from a parsed request message.

**Parameters**:
- `parser`: A pointer to the `http_parser` instance.

**Returns**: An `http_method` enum value. **(Only valid for `HTTP_PARSER_REQUEST` type)**

# References

- [RFC 2616](https://datatracker.ietf.org/doc/html/rfc2616#section-14.7)
- [NodeJS http-parser](https://github.com/nodejs/http-parser)
- [llhttp](https://github.com/nodejs/llhttp)
