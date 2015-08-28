IDIR=../CuTest
CC=gcc
CFLAGS=-I.

DEPS = ../CuTest/CuTest.h
OBJ = unit_bm.o ../CuTest/CuTest.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

bm_tests: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o bm_tests
