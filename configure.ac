AC_INIT([servicehisilicon], [0.1], [])
AM_INIT_AUTOMAKE([subdir-objects foreign dist-bzip2 no-define tar-pax])
AC_CONFIG_MACRO_DIR([m4])
LT_INIT

AC_PROG_CC
AC_PROG_CXX
AM_PATH_PYTHON
AX_PYTHON_DEVEL
AC_LANG(C++)

AX_CXX_COMPILE_STDCXX_17([ext],[mandatory])

PKG_CHECK_MODULES(ENIGMA2, enigma2)

AC_DEFINE([DEBUG])

AC_ARG_WITH(binarycompatibledebug,
	AC_HELP_STRING([--with-binarycompatibledebug], [use e2 independent debug output]),
	[[with_binarycompatibledebug=$withval]],
	[[with_binarycompatibledebug=no]]
)

if test x"$with_binarycompatibledebug" = xyes ; then
	AC_DEFINE([BINARY_COMPATIBLE_DEBUGLOG])
fi

AC_CONFIG_FILES([
Makefile
servicehisilicon/Makefile
plugin/Makefile
])
AC_OUTPUT
