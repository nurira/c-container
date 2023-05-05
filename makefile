CFLAGS = -std=c++11 -Wall -Wextra -pedantic -O4

all: tests

tests: test1 test2 test3 test4 test5

test1: test-kec.cpp Map.hpp
	g++ $(CFLAGS) -o test1 test-kec.cpp

test2: test.cpp Map.hpp
	g++ $(CFLAGS) -o test2 test.cpp

test3: minimal.cpp Map.hpp
	g++ $(CFLAGS) -o test3 minimal.cpp

test4: morseex.cpp Map.hpp
	g++ $(CFLAGS) -o test4 morseex.cpp

test5: test-scaling.cpp Map.hpp
	g++ $(CFLAGS) -o test5 test-scaling.cpp

clean:
	rm -f *.o
	rm -f test1 test2 test3 test4 test5
