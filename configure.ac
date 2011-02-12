dnl Process this file with autoconf to create configure.

AC_PREREQ([2.57])
AC_INIT(wndo,[0.7], [http://wijjo.com/wndo],wndo)
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_INSTALL
AM_PATH_PYTHON(2.4)
AC_PYTHON_MODULE(sys)
AC_PYTHON_MODULE(os)
AC_PYTHON_MODULE(os.path)
AC_PYTHON_MODULE(re)
AC_PYTHON_MODULE(shutil)
AC_PYTHON_MODULE(inspect)
AC_PYTHON_MODULE(traceback)
AC_PYTHON_MODULE(types)
AC_PYTHON_MODULE(copy)

# Checks for X11
PKG_CHECK_MODULES(X, x11)
CFLAGS="$CFLAGS $X_CFLAGS"
LDFLAGS="$LDFLAGS $X_LIBS"

# Checks for X11/Xmu
PKG_CHECK_MODULES(XMU, xmu)
CFLAGS="$CFLAGS $XMU_CFLAGS"
LDFLAGS="$LDFLAGS $XMU_LIBS"

# Checks for X11/XTest
PKG_CHECK_MODULES(XTEST, xtst)
CFLAGS="$CFLAGS $XTEST_CFLAGS"
LDFLAGS="$LDFLAGS $XTEST_LIBS"

AC_OUTPUT(Makefile
          bash_completion/Makefile
          bin/Makefile
          debian/Makefile
          docgen/Makefile
          lib/Makefile
          test/Makefile
          wndo.d/Makefile
          wndo-ctrl/Makefile
          wndo-pick/Makefile
          wndo-spy/Makefile)
