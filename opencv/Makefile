CC  = gcc
C++ = g++
WARNINGS = -Wall
C_DEBUG = -pg -fno-omit-frame-pointer -ggdb -g
LD_DEBUG = -g -p

# Libraries to link with
CV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui

# Linker and compiler flags
CFLAGS = ${OPT} ${WARNINGS}
ifdef DEBUG
	CFLAGS += ${C_DEBUG}
	LDFLAGS += ${LD_DEBUG}
endif

TARGETS = cam-pthread 

default: all

all: $(TARGETS)

cam-pthread.o: cam-pthread.cpp cam-pthread.h
	${C++} ${CFLAGS} -c $< -o $@;

jetsonGPIO.o: jetsonGPIO.c jetsonGPIO.h
	${C++} ${CFLAGS} -c $< -o $@;

cam-pthread: jetsonGPIO.o cam-pthread.o
	${C++} ${LDFLAGS} -o $@ $^ -pthread -lopencv_core -lopencv_imgproc -lopencv_highgui;

clean:
	rm -f *.o $(TARGETS)

