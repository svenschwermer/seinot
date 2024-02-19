SUMMARY = "NFC-activated jukebox app"
DESCRIPTION = "Don't read this backwards..."
LICENSE = "GPL-3.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-3.0-only;md5=c79ff39f19dfec6d293b95dea7b07891"

inherit pkgconfig

DEPENDS += "\
    libnfc \
    alsa-lib \
    mpg123 \
    "
# libout123

SRC_URI = "\
    file://led.c \
    file://led.h \
    file://log.h \
    file://main.c \
    file://Makefile \
    file://mifare.c \
    file://mifare.h \
    file://nfc.c \
    file://nfc.h \
    file://playback.c \
    file://playback.h \
    file://soundctrl.c \
    file://soundctrl.h \
    "
S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/seinot ${D}${bindir}/seinot
}
