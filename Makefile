CC := cc
AR := ar

.PHONY: all

all: mwad libwad.a

%.o: %.c
	$(CC) -o $@ -c $<

libwad.a: wad.o
	$(AR) -crs $@ $^

mwad: libwad.a main.o
	$(CC) -o $@ $(filter %.o,$^) -L. $(patsubst lib%.a,-l%,$(filter %.a,$^))
