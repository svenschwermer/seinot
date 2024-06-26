LICENSE = "CLOSED"

SRC_URI = "\
    file://wifi.sh \
    "

RDEPENDS:${PN} = "\
    systemd \
    wpa-supplicant \
    "

inherit allarch

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/wifi.sh ${D}${bindir}/wifi
}
