CC := cc
AR := ar

.PHONY: all

all: mwad libwad.a

%.o: %.c
	$(CC) -o $@ -c $<

libwad.a: wad.o
	$(AR) -crs $@ $^

mwad: libwad.a main.c
	$(CC) -o $@ $(filter %.c,$^) -L. $(patsubst lib%.a,-l%,$(filter %.a,$^)) $(shell pkg-config fuse --cflags --libs)
