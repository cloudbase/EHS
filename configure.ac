dnl Process this file with autoconf to produce a configure script.
AC_INIT([ehs],[1.3.1],[ehsdev@fritz-elfert.de])
AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([ehs.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign no-installinfo no-installman])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AM_PROG_LIBTOOL

dnl Checks for libraries.

AC_ARG_WITH([pcre],AS_HELP_STRING([--with-pcre=DIR],[Specifies the location of the PCRE library]),
    [CPPFLAGS="$CPPFLAGS -I${withval}/include"
     CXXFLAGS="$CXXFLAGS -I${withval}/include"
     LDFLAGS="$LDFLAGS -L${withval}/lib"],[])

AC_ARG_WITH([pme], AS_HELP_STRING([--with-pme=DIR], [Specifies the location of the PME library]),
    [CPPFLAGS="$CPPFLAGS -I${withval}/include"
     CXXFLAGS="$CXXFLAGS -I${withval}/include"
     LDFLAGS="$LDFLAGS -L${withval}/lib"],[])

dnl Replace `main' with a function in -lpcre:
AC_CHECK_LIB(pcre, main)
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(sokt, socket)
AC_CHECK_LIB(pme,main,,AC_MSG_ERROR(EHS requires libpme from http://xaxxon.slackworks.com/pme/))

COMPILE_WITH_SSL=0
ac_cv_withssl=0
AC_ARG_WITH([ssl],AS_HELP_STRING([--with-ssl],[compile with SSL support (requires OpenSSL)]),[ac_cv_withssl=1])
if test "$ac_cv_withssl" = "1" ; then
dnl Replace `main' with a function in -lssl:
    AC_CHECK_LIB(ssl,main)
    COMPILE_WITH_SSL=1
    AC_DEFINE([COMPILE_WITH_SSL],[1],[Whether to compile with SSL support or not])
fi

EHS_DEBUG=0
AC_ARG_WITH([debug],AS_HELP_STRING([--with-debug],[compile with debugging information]),[EHS_DEBUG=1])
if test "$EHS_DEBUG" = "1" ; then
    AC_DEFINE([EHS_DEBUG],,[Whether to compile with debugging information])
fi

EHS_MEMORY=0
AC_ARG_WITH(memory,[  --with-memory           compile with memory tracking information],EHS_MEMORY=1)
if test "$EHS_MEMORY" = "1" ; then
    AC_DEFINE([EHS_MEMORY],,[Whether to compile with memory tracking information])
fi
AC_SUBST(COMPILE_WITH_SSL)
AC_SUBST(EHS_DEBUG)
AC_SUBST(EHS_MEMORY)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_C_CONST

dnl Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memset select setlocale socket strcasecmp])

CPPFLAGS="$CPPFLAGS -DIN_EHSBUILD"

AC_OUTPUT([Makefile ehs-config.h])
