AC_INIT([pimeter], [0.44])
AC_CONFIG_SRCDIR([src/pivumeter.c])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

AC_CHECK_LIB([wiringPi], [wiringPiSetup], , [AC_MSG_ERROR(You need libwiringPi installed)], )
AC_CHECK_LIB([asound], [snd_pcm_open], , [AC_MSG_ERROR(You need libasound installed)], )
AC_CHECK_LIB([fftw3], [fftw_plan_dft_r2c_1d], , [AC_MSG_ERROR(You need libfftw3 installed)], )
AC_CHECK_LIB([m], [log])

AC_OUTPUT(Makefile)
