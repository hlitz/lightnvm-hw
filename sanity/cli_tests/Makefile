IDIR=../CuTest
CC=gcc
CFLAGS=-I.

DEPS = ../CuTest/CuTest.h
OBJ = cli_test.o ../CuTest/CuTest.o
LIBOBJ = lib_test.o ../CuTest/CuTest.o

all: cli_test lib_test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cli_test: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

lib_test: $(LIBOBJ)
	gcc -o $@ $^ $(CFLAGS) -llightnvm

clean:
	rm -f *.o cli_test lib_test
