SUMMARY = "Utility files to load the modules"
DESCRIPTION = "${SUMMARY}"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://aeldd-start-stop \
	   file://Makefile \
	   file://aeldd.c \
          "

S = "${WORKDIR}"

inherit update-rc.d
INITSCRIPT_NAME = "aeldd-start-stop"
INITSCRIPT_PARAMS = "99 5"

do_compile () {
	oe_runmake
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${S}/aeldd ${D}${bindir}/
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/aeldd-start-stop ${D}${sysconfdir}/init.d/
}