SUMMARY = "A console-only image that fully supports the target device \
hardware."

IMAGE_INSTALL:append = " aeld_hd44780"

IMAGE_FEATURES += "splash"

LICENSE = "MIT"

inherit core-image
