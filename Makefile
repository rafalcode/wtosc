CC=gcc
CFLAGS=-g -Wall
SPECLIBS=-lcairo -lm

EXECUTABLES=sy0 symyf symymf wsymymf wsymymf_d wnums wnums_d lnums seerold gwa gwa2 wtout wtout2 wtbank0 gri1 sinevals tsinevals natf svs0 gwa3

# symyf: SYnthesise MY Frequency. Accepts a frequency vlaue on the command line, and synthesizes a sine wave on it, using a wavetable.
symyf: symyf.c
	${CC} ${CFLAGS} $^ -o $@ -lm
# a precursor to symyf, which takes a list of shorts (one per line) and uses them to create ring and desired frequency
sy0: sy0.c
	${CC} ${CFLAGS} $^ -o $@ -lm
natf: natf.c
	${CC} ${CFLAGS} $^ -o $@ -lm
svs0: svs0.c
	${CC} ${CFLAGS} $^ -o $@ -lm


# in this variation, there's an m before the f, which stands for multi.
# so we have 8 frequencies we'd like built from the wavetable, all output to one wav file.
# there's an extra useful data struct in this one, an srp_t, which is a point to ring struct, with an added sz value.
# indicating the number of sample points in the ring.
symymf: symymf.c
	${CC} ${CFLAGS} $^ -o $@ -lm
gwa: gwa.c
	${CC} ${CFLAGS} $^ -o $@ -lm
# an easier to read gwa:
gwa2: gwa2.c
	${CC} ${CFLAGS} $^ -o $@ -lm
gwa2_d: gwa2.c
	${CC} ${CFLAGS} -DDBG $^ -o $@ -lm
# a multiharmonic version of gwa2
gwa3: gwa3.c
	${CC} ${CFLAGS} $^ -o $@ -lm
wtout: wtout.c
	${CC} ${CFLAGS} $^ -o $@ -lm
#returning to wtout.c after a while had me confused. Some steps I took were'nt exactly logical.
# wtout2.c is an attempt to clean up more or less.
wtout2: wtout2.c
	${CC} ${CFLAGS} $^ -o $@ -lm

# wtbank0 base on seerold.c
# # does same thing, but it's been robustified
wtbank0: wtbank0.c
	${CC} ${CFLAGS} -o $@ $^ ${SPECLIBS} -lm

# prototype for reading in a data from a wav file from a certain time point: used later in wsymymf.c
samh: samh.c
	${CC} ${CFLAGS} $^ -o $@

# wnums: simpy extract essential numbers from a wav file, including actual values from
# a certain timepoint
wnums: wnums.c
	${CC} ${CFLAGS} $^ -o $@
wnums_d: wnums.c
	${CC} ${CFLAGS} -DDBG $^ -o $@

# control a certain level
lnums: lnums.c
	${CC} ${CFLAGS} $^ -o $@

# control a certain level
lnums_d: lnums.c
	${CC} ${CFLAGS} -DDBG $^ -o $@

# based on wnums
magbit: magbit.c
	${CC} ${CFLAGS} $^ -o $@

# wsymymf: the extra w here is for WAV. The idea is to populate the wavetable from samples in a wav file
wsymymf: wsymymf.c
	${CC} ${CFLAGS} $^ -o $@

# wsymymf_d: need the debug version to discover where the distortion is coming from
wsymymf_d: wsymymf.c
	${CC} ${CFLAGS} -DDBG $^ -o $@

gri1: gri1.c
	${CC} ${CFLAGS} -o $@ $^ ${SPECLIBS} -lm

sinevals: sinevals.c
	${CC} ${CFLAGS} -o $@ $^ -lm


.PHONY: clean

clean:
	rm -f ${EXECUTABLES} a.out
