SUMMARY = "A console-only image that fully supports the target device \
hardware."

IMAGE_INSTALL:append = " hello-mod"

IMAGE_FEATURES += "splash"

LICENSE = "MIT"

inherit core-image
