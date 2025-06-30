all:
	gcc *.c -o ahttp

.PHONY: clean
clean:
	rm ahttp
