IDIR=../CuTest
CC=gcc
CFLAGS=-I.

DEPS = ../CuTest/CuTest.h
OBJ = cli_test.o ../CuTest/CuTest.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cli_test: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o cli_test
