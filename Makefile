CC=gcc
CFLAGS=-g -Wall

EXECUTABLES=sr3

sr1: sr1.c
	${CC} ${CFLAGS} $^ -o $@

sr2: sr2.c
	${CC} ${CFLAGS} $^ -o $@ -lm

sr3: sr3.c
	${CC} ${CFLAGS} $^ -o $@ -lm

.PHONY: clean

clean:
	rm -f ${EXECUTABLES}
