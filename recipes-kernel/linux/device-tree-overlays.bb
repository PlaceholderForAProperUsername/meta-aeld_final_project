SUMMARY = "Device tree overlays"
DESCRIPTION = "Test"

SRC_URI = "file://bme280-overlay.dts"

inherit devicetree

S = "${WORKDIR}"
