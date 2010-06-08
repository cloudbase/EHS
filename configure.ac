dnl Process this file with autoconf to produce a configure script.
AC_INIT([ehs],[1.4.6],[ehsdev@fritz-elfert.de])
AC_REVISION([$Id$])
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

dnl Replace `main' with a function in -lpcre:
BFDLIB1=
BFDLIB2=
DWLIB=
AC_CHECK_LIB(pcrecpp, main)
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(sokt, socket)
AC_CHECK_LIB(ssl, SSL_library_init)
AC_CHECK_LIB(dw, dwfl_begin, DWLIB="-ldw")
AC_CHECK_LIB(bfd, bfd_openr, [BFDLIB1="-lbfd"])
AC_CHECK_LIB(iberty, cplus_demangle, [BFDLIB2="-liberty"])
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

AC_MSG_CHECKING([whether to prefer BFD over DWARF])
enableval=NO
PREFER_BFD=NO
AC_ARG_ENABLE(bfd,AS_HELP_STRING([--enable-bfd],[prefer BFD over DWARF (use for ehs_exception sample only)]))
AC_MSG_RESULT([$enableval])
case "$enableval" in
    [[yY][eE][sS]])
        PREFER_BFD=YES
        ;;
esac

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h demangle.h dwarf.h])

dnl DWARF vs. BFD
DW_CPPFLAGS=
AC_CHECK_HEADERS([libdwfl.h])
if test "$ac_cv_header_libdwfl_h" = "no" ; then
    AC_CHECK_HEADERS([elfutils/libdwfl.h],[DW_CPPFLAGS="-I/usr/include/elfutils"])
fi

if test -n "$DWLIB" -a "$PREFER_BFD" = "NO" ; then
    TRACE_LIBS="$DWLIB"
    TRACE_CPPFLAGS="$DW_CPPFLAGS"
    AC_DEFINE_UNQUOTED([USE_DWARF],1,[Define, if using libdw for resolving symbols])
    AC_MSG_NOTICE([Using DWARF for resolving symbols])
    if test -n "$BFDLIB2" ; then
        TRACE_LIBS="$DWLIB $BFDLIB2"
        AC_DEFINE_UNQUOTED([HAVE_LIBIBERTY],1,[Define, if libiberty is available])
    fi
else
    if test -n "$BFDLIB1" -a -n "$BFDLIB2" ; then
        TRACE_LIBS="$BFDLIB1 $BFDLIB2"
        AC_DEFINE_UNQUOTED([USE_BFD],1,[Define, if using bfd for resolving symbols])
        AC_MSG_NOTICE([Using BFD for resolving symbols])
    else
        AC_MSG_NOTICE([Disabling backtracing])
    fi
fi
AC_SUBST(TRACE_LIBS)
AC_SUBST(TRACE_CPPFLAGS)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_C_CONST

dnl Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memset select setlocale socket strcasecmp strtoul])
tmp_LIBS="$LIBS"
LIBS="$LIBS $BFDLIB1 $BFDLIB2"
AC_CHECK_FUNCS([bfd_demangle])
LIBS="$tmp_LIBS"

FE_SVNREV([SVNREV])
AC_DEFINE_UNQUOTED(SVNREV,"$SVNREV",[SVN revision])
FE_RCVERSION

dnl Use modern -pthread gcc option if available.
fe_CHECK_FLAG([C++],[CXXFLAGS],[-pthread])

AC_ARG_ENABLE([warn],
	AS_HELP_STRING([--enable-warn],
		[Enable additional warnings (Default: NO)]),
	[
		AC_MSG_NOTICE([will enable additional warnings])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-pedantic])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wall])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wextra])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wchar-subscripts])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wcomment])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wformat])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wformat-security])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wimplicit])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wmissing-braces])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wparentheses])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wreturn-type])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wswitch])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wuninitialized])
        CPPFLAGS="$CPPFLAGS $ac_checked_CPPFLAGS"
        fe_CHECK_FLAG([C++],[CXXFLAGS],[-Wctor-dtor-privacy])
        fe_CHECK_FLAG([C++],[CXXFLAGS],[-Wnon-virtual-dtor])
        fe_CHECK_FLAG([C++],[CXXFLAGS],[-Woverloaded-virtual])
        fe_CHECK_FLAG([C++],[CXXFLAGS],[-Weffc++])
        CXXFLAGS="$CXXFLAGS $ac_checked_CXXFLAGS"
	]
)
LIBVERSION="`echo $VERSION|tr . :`"
AC_SUBST(LIBVERSION)

dnl
dnl doxygen setup
dnl
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([EHS], [conf/doxygen.cfg], [doc])

AC_CONFIG_FILES([Makefile samples/Makefile conf/doxygen.cfg doc/manual.dox])
AC_OUTPUT
