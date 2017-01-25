AM_CFLAGS = -g -O2 -W -Wall -lwiringPi `sdl-config --cflags` -DWITH_DEVICE_BLINKT -DWITH_DEVICE_SPEAKER_PHAT
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libpimeter.la

libpimeter_la_SOURCES = src/pimeter.c src/devices/blinkt.c src/devices/speaker-phat.c
