AC_INIT([libmapstore],[1.0.0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])
AM_INIT_AUTOMAKE([foreign])
LT_INIT([win32-dll])

AC_PATH_PROG(HEXDUMP,hexdump)

if test x$HEXDUMP = x; then
AC_MSG_ERROR(hexdump is required for tests)
fi

PKG_PROG_PKG_CONFIG
if test -z "$PKG_CONFIG"; then
   AC_MSG_ERROR([pkg-config not found. pkg-config is required to check for some dependencies.])
fi

PKG_CHECK_MODULES([JSON], [json-c >= 0.11],, [AC_MSG_ERROR([libjson-c 0.11 or greater was not found.])])
AC_CHECK_HEADERS([json-c/json.h], [json_found_headers=yes; break;])
AS_IF([test "x$json_found_headers" != "xyes"],
        [AC_MSG_ERROR([Unable to find json-c headers.])])

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])
AC_CONFIG_FILES([libmapstore.pc:libmapstore.pc.in])

AC_CHECK_FUNCS([aligned_alloc posix_memalign posix_fallocate])

AM_CONDITIONAL([BUILD_MAPSTORE_DLL], [test "x${CFLAGS/"MAPSTOREDLL"}" != x"$CFLAGS"])

AC_ARG_ENABLE([debug],
        [AS_HELP_STRING([--enable-debug],
        [use debug compiler flags and macros (default is no)])],
        [enable_debug=$enableval],
        [enable_debug=no])

if test "x$enable_debug" = xyes; then
   CFLAGS="$CFLAGS -DDEBUG -g3 -O0"
else
   CFLAGS="$CFLAGS -O3"
fi

AC_OUTPUT
