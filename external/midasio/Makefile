# Makefile

CFLAGS := -O2 -g -Wall -Wuninitialized

all:: midasio.o
all:: test_midasio.o
all:: test_midasio.exe

clean::
	/bin/rm -f *.o *.exe

test_midasio.exe: %.exe: %.o midasio.o lz4.o lz4frame.o lz4hc.o xxhash.o
	g++ -o $@ $^ $(CFLAGS) -lz

%.o: %.cxx
	g++ -c -o $@ $^ $(CFLAGS) -I.

# end

