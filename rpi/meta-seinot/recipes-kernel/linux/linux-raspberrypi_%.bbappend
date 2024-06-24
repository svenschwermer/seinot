FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += "\
    file://0001-slab-Remove-__malloc-attribute-from-realloc-function.patch \
    file://0002-devres-Provide-krealloc_array.patch \
    file://0003-leds-Provide-devm_of_led_get_optional.patch \
    file://0004-led-led-class-Read-max-brightness-from-devicetree.patch \
    file://0005-leds-class-Store-the-color-index-in-struct-led_class.patch \
    file://0006-leds-rgb-Add-a-multicolor-LED-driver-to-group-monoch.patch \
    file://seinot-overlay.patch \
    file://seinot-overlay.dts \
    file://${MACHINE}/defconfig \
    "

#KBUILD_DEFCONFIG:seinot-rpi0w ?= "bcmrpi_defconfig"
#KBUILD_DEFCONFIG:seinot-rpi3 ?= "bcmrpi3_defconfig"
KBUILD_DEFCONFIG:seinot-rpi3 = ""

do_copy_seinot_overlay() {
    cp ${WORKDIR}/seinot-overlay.dts ${S}/arch/${ARCH}/boot/dts/overlays/
}
addtask copy_seinot_overlay after do_patch before do_configure
