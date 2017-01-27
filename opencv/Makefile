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

TARGETS = camera

default: all

all: $(TARGETS)

camera.o: camera.C
	${C++} ${CFLAGS} -c $< -o $@;

camera: camera.o
	${C++} ${LDFLAGS} -o $@ $^ -lopencv_core -lopencv_imgproc -lopencv_highgui;

clean:
	rm -f *.o camera

