
lib_LTLIBRARIES = libmapstore.la
libmapstore_la_SOURCES = mapstore.c mapstore_helpers.c utils.c utils.h database_utils.c database_utils.h
libmapstore_la_LIBADD = -ljson-c -luv -lsqlite3 -lm -lnettle
libmapstore_la_LDFLAGS = -Wall
if BUILD_MAPSTORE_DLL
libmapstore_la_LDFLAGS += -no-undefined
endif
include_HEADERS = mapstore.h

bin_PROGRAMS = mapstore
mapstore_SOURCES = cli.c cli_helper.c cli_helper.h mapstore.h
mapstore_LDADD = libmapstore.la
if BUILD_MAPSTORE_DLL
mapstore_LDFLAGS = -Wall
else
mapstore_LDFLAGS = -Wall -static
endif
