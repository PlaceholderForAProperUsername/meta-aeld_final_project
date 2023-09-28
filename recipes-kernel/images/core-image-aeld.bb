SUMMARY = "Image for the advanced embedded linux development specialization."

IMAGE_FEATURES += "splash"

LICENSE = "MIT"

inherit core-image

KERNEL_DEVICE_TREE += " overlays/bme280.dtbo"
