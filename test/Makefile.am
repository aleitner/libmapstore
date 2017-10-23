noinst_PROGRAMS = tests
tests_SOURCES = tests.c tests.h $(top_builddir)/src/mapstore.h leitner_test.h
tests_LDADD = $(top_builddir)/src/libmapstore.la
tests_LDFLAGS = -Wall -g

if BUILD_MAPSTORE_DLL
tests_LDFLAGS +=
else
tests_LDFLAGS += -static
endif

TESTS = tests
