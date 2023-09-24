SUMMARY = "Minimal kernel driver to use a HD44780 LCD"
DESCRIPTION = "${SUMMARY}"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module

SRC_URI = "file://Makefile \
           file://aeld_hd44780.c \
           file://COPYING \
          "

PV = "1.0"

S = "${WORKDIR}"

RPROVIDES_${PN} += "kernel-module-aeld_hd44780" 
