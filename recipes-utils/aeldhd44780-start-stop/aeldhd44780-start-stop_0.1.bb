SUMMARY = "Utility files to load the modules"
DESCRIPTION = "${SUMMARY}"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://aeldhd44780-start-stop \
          "

inherit update-rc.d
INITSCRIPT_NAME = "aeldhd44780-start-stop"
INITSCRIPT_PARAMS = "90 20"

S = "${WORKDIR}"
do_install() {
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/aeldhd44780-start-stop ${D}${sysconfdir}/init.d/
}
