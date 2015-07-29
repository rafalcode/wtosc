CC=gcc
CFLAGS=-g -Wall

EXECUTABLES=sr3

sr3: sr3.c
	${CC} ${CFLAGS} $^ -o $@

.PHONY: clean

clean:
	rm -f ${EXECUTABLES}
