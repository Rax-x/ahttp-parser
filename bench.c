#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h>

#include "ahttp_parser.h"

/* 8 gb */
static const int64_t bytes = 8LL << 30;

static const char request[] =
    "POST /test/ahttp-parser HTTP/1.1\r\n"
    "Host: github.com\r\n"
    "DNT: 1\r\n"
    "Accept-Encoding: gzip, deflate, sdch\r\n"
    "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
    "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) "
    "AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/39.0.2171.65 Safari/537.36\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,"
    "image/webp,*/*;q=0.8\r\n"
    "Referer: https://github.com/joyent/http-parser\r\n"
    "Connection: keep-alive\r\n"
    "Transfer-Encoding: chunked\r\n"
    "Cache-Control: max-age=0\r\n\r\nb\r\nhello world\r\n0\r\n";

static const int request_length = sizeof(request);

static http_parser_settings default_settings = {0};

void run_bench(int iter_count) {
    
    int err;

    struct timeval start;
    struct timeval end;

    http_parser parser;
    int parsed;


    printf( "request length = %d\n", request_length);

    err = gettimeofday(&start, NULL);
    assert(err == 0);

    for (int i = 0; i < iter_count; i++) {
        parser = http_parser_init(request, request_length);

        parsed = http_parser_run(&parser, NULL, &default_settings, HTTP_PARSER_REQUEST);
        assert(parsed == request_length);
    }

    double elapsed;
    double bw;
    double total;

    err = gettimeofday(&end, NULL);
    assert(err == 0);

    printf( "Benchmark result:\n");

    elapsed = (double) (end.tv_sec - start.tv_sec) +
        (end.tv_usec - start.tv_usec) * 1e-6f;

    total = (double) iter_count * request_length;
    bw = (double) total / elapsed;

    printf("%.2f mb | %.2f mb/s | %.2f req/sec | %.2f s\n",
            (double) total / (1024 * 1024),
            bw / (1024 * 1024),
            (double) iter_count / elapsed,
            elapsed);

    fflush(stdout);
}

int main(void) {

    const int iterations = bytes / request_length;
    run_bench(iterations);

    return 0;
}
