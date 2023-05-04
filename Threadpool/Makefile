.PHONY: all
all: task stdinExample

task: codec.h basic_main.c
	gcc basic_main.c -L. -l Codec -o encoder

stdinExample: codec.h stdin_main.c
	gcc stdin_main.c -L. -l Codec -o tester

.PHONY: clean
clean:
	-rm encoder tester 2>/dev/null
