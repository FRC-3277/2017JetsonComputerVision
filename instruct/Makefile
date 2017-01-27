TARGETS = sizeof arrays pointers malloc

default: all

all: $(TARGETS)

sizeof: sizeof.cpp
	/usr/bin/g++ -Wall -o $@ $<

arrays: arrays.cpp
	/usr/bin/g++ -Wall -o $@ $<

pointers: pointers.cpp
	/usr/bin/g++ -Wall -o $@ $<

malloc: malloc.cpp
	/usr/bin/g++ -Wall -o $@ $<

clean:
	rm -f $(TARGETS)
