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

AC_ARG_WITH([pme],AS_HELP_STRING([--with-pme=DIR], [Specifies the location of the PME library]),
    [CPPFLAGS="$CPPFLAGS -I${withval}/include"
     CXXFLAGS="$CXXFLAGS -I${withval}/include"
     LDFLAGS="$LDFLAGS -L${withval}/lib"],[])

dnl Replace `main' with a function in -lpcre:
AC_CHECK_LIB(pcre, main)
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(sokt, socket)
AC_CHECK_LIB(pme,main,,AC_MSG_ERROR(EHS requires libpme from http://xaxxon.slackworks.com/pme/))
AC_CHECK_LIB(ssl,SSL_library_init)
havessl=$ac_cv_lib_ssl_SSL_library_init

AC_MSG_CHECKING([whether to build with SSL support])
withval=AUTO
AC_ARG_WITH([ssl],AS_HELP_STRING([--with-ssl],[compile with SSL support (requires OpenSSL)]))
AC_MSG_RESULT([$withval])
case "$withval" in
    [[yY][eE][sS]])
        test "$havessl" = "yes" || AC_MSG_ERROR([For compiling with SSL support, OpenSSL is required])
        AC_DEFINE([COMPILE_WITH_SSL],[1],[Whether to compile with SSL support or not])
        ;;
    AUTO)
        if test "$havessl" = "yes" ; then
            AC_DEFINE([COMPILE_WITH_SSL],[1],[Whether to compile with SSL support or not])
            AC_MSG_RESULT([configure: libssl is available, enabled SSL support])
        else
            AC_MSG_RESULT([configure: libssl not available, disabled SSL support])
        fi
        ;;
esac

AC_MSG_CHECKING([whether to build with debugging information])
enableval=NO
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[compile with debugging information]))
AC_MSG_RESULT([$enableval])
case "$enableval" in
    [[yY][eE][sS]])
        AC_DEFINE([EHS_DEBUG],[1],[Whether to compile with debugging information])
        ;;
esac

AC_MSG_CHECKING([whether to build with memory tracking information])
enableval=NO
AC_ARG_ENABLE(memory,AS_HELP_STRING([--enable-memory],[compile with memory tracking information]))
AC_MSG_RESULT([$enableval])
case "$enableval" in
    [[yY][eE][sS]])
        AC_DEFINE([EHS_MEMORY],[1],[Whether to compile with memory tracking information])
        ;;
esac

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

AC_OUTPUT([Makefile])
