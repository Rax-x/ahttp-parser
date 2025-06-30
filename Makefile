all:
	gcc -std=c99 *.c -o ahttp

.PHONY: clean
clean:
	rm ahttp
