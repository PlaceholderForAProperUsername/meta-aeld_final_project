SRC_URI += "file://bme280-overlay.dts;subdir=git/arch/${ARCH}/boot/dts/overlays"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

PACKAGE_ARCH = "${MACHINE_ARCH}"

inherit devicetree

S = "${WORKDIR}"

MACHINE_PREFIX = "${MACHINE}"

DT_FILES_PATH = "${WORKDIR}"

do_deploy:append () {
	install -d ${DEPLOYDIR}/overlays
	
	echo "fdt_overlays=bme280.dtbo" > ${DEPLOYDIR}/overlays.txt
}

