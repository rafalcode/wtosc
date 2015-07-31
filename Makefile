CC=gcc
CFLAGS=-g -Wall

EXECUTABLES=symyf symymf wsymymf

# symyf: SYnthesise MY Frequency. Accepts a frequency vlaue on the command line, and synthesizes a sine wave on it, using a wavetable.
symyf: symyf.c
	${CC} ${CFLAGS} $^ -o $@ -lm

# in this variation, there's an m before the f, which stands for multi.
# so we have 8 frequencies we'd like built from the wavetable, all output to one wav file.
# there's an extra useful data struct in this one, an srp_t, which is a point to ring struct, with an added sz value.
# indicating the number of sample points in the ring.
symymf: symymf.c
	${CC} ${CFLAGS} $^ -o $@ -lm

# prototype for reading in a data from a wav file from a certain time point: used later in wsymymf.c
samh: samh.c
	${CC} ${CFLAGS} $^ -o $@

# wnums: simpy extract essential numbers from a wav file, including actual values from
# a certain timepoint
wnums: wnums.c
	${CC} ${CFLAGS} $^ -o $@
wnums_d: wnums.c
	${CC} ${CFLAGS} -DDBG $^ -o $@

# wsymymf: the extra w here is for WAV. The idea is to populate the wavetable from samples in a wav file
wsymymf: wsymymf.c
	${CC} ${CFLAGS} $^ -o $@

.PHONY: clean

clean:
	rm -f ${EXECUTABLES} a.out
