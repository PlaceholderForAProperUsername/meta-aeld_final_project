SUMMARY = "Utility files to load the modules"
DESCRIPTION = "${SUMMARY}"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://aeldbme280-start-stop \
          "
          
S = "${WORKDIR}"
do_install() {
	install -d ${D}${sysconfdir}/init.d
	install -d ${D}${sysconfdir}/rcS.d
	install -m 0755 ${WORKDIR}/aeldbme280-start-stop ${D}${sysconfdir}/init.d/aeldbme280-start-stop
	
	ln -sf ../init.d/aeldbme280-start-stop	${D}${sysconfdir}/rcS.d/S98aeldbme280-start-stop
}
