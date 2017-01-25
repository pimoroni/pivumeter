AC_INIT([pimeter], [0.44])
AC_CONFIG_SRCDIR([src/pimeter.c])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

AC_CHECK_LIB([SDL], [SDL_Init], , [AC_MSG_ERROR(You need libSDL installed)], -lpthread)


AC_OUTPUT(Makefile)
