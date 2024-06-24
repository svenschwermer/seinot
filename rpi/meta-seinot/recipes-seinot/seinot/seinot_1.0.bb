SUMMARY = "NFC-activated jukebox app"
DESCRIPTION = "Don't read this backwards..."
LICENSE = "GPL-3.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-3.0-only;md5=c79ff39f19dfec6d293b95dea7b07891"

inherit pkgconfig systemd

DEPENDS += "\
    libnfc \
    alsa-lib \
    mpg123 \
    "

# libgcc is needed for pthread_cancel to work
RDEPENDS:${PN} += "libgcc"

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
    file://seinot.service \
    "
S = "${WORKDIR}"

SYSTEMD_SERVICE:${PN} = "seinot.service"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/seinot ${D}${bindir}/seinot
    
    install -d ${D}${systemd_system_unitdir}
    install -m 644 ${WORKDIR}/seinot.service ${D}${systemd_system_unitdir}
    sed -i 's:@bindir@:${bindir}:' ${D}${systemd_system_unitdir}/seinot.service
}
FILES:${PN} += "${systemd_system_unitdir}"
