CC = gcc
BENCH_CFLAGS = -O3 -std=c99 -Wall -pedantic

.PHONY: bench

bench:
	$(CC) $(BENCH_CFLAGS) bench.c ahttp_parser.c -o benchmark
	./benchmark
	@rm -rf benchmark
