
lib_LTLIBRARIES = libmapstore.la
libmapstore_la_SOURCES = mapstore.c shareshare.h utils.c utils.h server.c server.h
libmapstore_la_LIBADD = -lcurl -ljson-c -luv -lmicrohttpd -lsqlite3 -lm -lnettle
libmapstore_la_LDFLAGS = -Wall
if BUILD_MAPSTORE_DLL
libmapstore_la_LDFLAGS += -no-undefined
endif
include_HEADERS = mapstore.h

bin_PROGRAMS = mapstore
mapstore_SOURCES = cli.c mapstore.h
mapstore_LDADD = libmapstore.la
if BUILD_MAPSTORE_DLL
mapstore_LDFLAGS = -Wall
else
mapstore_LDFLAGS = -Wall -static
endif
