ARCH := $(shell uname -m | sed -e s/i686/x86/)

ifeq (${ARCH}, x86_64)
	COMPILER = gcc
else
	$(error ${ARCH} is not supported))
endif

ifeq (${COMPILER}, gcc)
	CC  = /usr/bin/gcc
	C++ = /usr/bin/g++
	WARNINGS = -Wall
	C_DEBUG = -pg -fno-omit-frame-pointer -ggdb -g
	LD_DEBUG = -g -p
	OPT = -msse4.2
else
	$(error ${COMPILER} is not supported))
endif

# Install directories
DESTDIR =
PREFIX = $(DESTDIR)/usr/local
INSTALL_DIR = $(PREFIX)/bin

# Library Path
LIBPATH = -L/usr/lib64/HCNetSDKCom -L/opt/gst-1.8.3/lib

# Includes
GST_INC = -I/opt/gst-1.8.3/include/gstreamer-1.0 -I/opt/gst-1.8.3/lib/gstreamer-1.0/include
XML_INC = -I/usr/include/libxml2
GL2_INC = -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include

# Libraries to link with
HIK_LIBS = -lhcnetsdk -lHCCoreDevCfg -lHCPreview
GL2_LIBS = -lglib-2.0 -lgthread-2.0 -lgobject-2.0
OCV_LIBS = -lopencv_core -lopencv_highgui -lopencv_imgproc
GST_LIBS = -lgstreamer-1.0 -lgstapp-1.0
XML_LIBS = -lxml2
SHM_LIBS = -lrt -lpthread
JSON_LIBS = -ljson-c
ZBAR_LIBS = -lzbar

# Linker and compiler flags
CFLAGS = ${OPT} ${WARNINGS}
ifdef DEBUG
	CFLAGS += ${C_DEBUG}
	LDFLAGS += ${LD_DEBUG}
endif

TARGETS = hik_camerad test camerad net_camerad net_client

default: all

all: $(TARGETS)

test.o: test.c
	${C++} ${CFLAGS} -c $< -o $@;

elk.o: elk.c elk.h
	${C++} ${CFLAGS} -c $< -o $@;

net_common.o: net_common.c net_common.h
	${C++} ${CFLAGS} -c $< -o $@;

net_camerad.o: net_camerad.c net_camerad.h
	${C++} ${CFLAGS} -c $< -o $@;

camerad.o: camerad.c camerad.h
	${C++} ${CFLAGS} -c $< -o $@;

video_shm.o: video_shm.c video_shm.h
	${C++} ${CFLAGS} -c $< -o $@;

net_client.o: net_client.c
	${C++} ${CFLAGS} -c $< -o $@;

hik_camerad.o: hik_camerad.c hik_camerad.h
	${C++} ${CFLAGS} ${GST_INC} ${XML_INC} ${GL2_INC} -c $< -o $@;

net_client: net_client.o net_common.o video_shm.o
	${C++} ${LDFLAGS} -o $@ $^ ${SHM_LIBS} ${JSON_LIBS};

test: test.o video_shm.o
	${C++} ${LDFLAGS} -o $@ $^ ${SHM_LIBS} ${OCV_LIBS};

camerad: camerad.o video_shm.o elk.o
	${C++} ${LDFLAGS} ${LIBPATH} -o $@ $^ ${OCV_LIBS} ${SHM_LIBS} ${ZBAR_LIBS} ${JSON_LIBS};

net_camerad: net_camerad.o net_common.o video_shm.o elk.o
	${C++} ${LDFLAGS} ${LIBPATH} -o $@ $^ ${SHM_LIBS} ${JSON_LIBS};

hik_camerad: hik_camerad.o video_shm.o elk.o
	${C++} ${LDFLAGS} ${LIBPATH} -o $@ $^ ${GL2_LIBS} ${XML_LIBS} ${GST_LIBS} ${HIK_LIBS} ${SHM_LIBS} ${JSON_LIBS};

install: $(TARGETS)
	install -m 755 -d $(INSTALL_DIR)
	install -m 755 $(TARGETS) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)
	rmdir --ignore-fail-on-non-empty $(PREFIX)

clean:
	rm -f *.o $(TARGETS)
