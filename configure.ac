AC_INIT([pimeter], [0.44])
AC_CONFIG_SRCDIR([src/pimeter.c])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

AC_CHECK_LIB([asound], [snd_pcm_open], , [AC_MSG_ERROR(You need libasound installed)], )


AC_OUTPUT(Makefile)
