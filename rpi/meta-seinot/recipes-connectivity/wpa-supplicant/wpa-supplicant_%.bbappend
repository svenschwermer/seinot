FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += "\
    file://etc-wpa_supplicant.mount \
    "

do_install:append() {
    install -d ${D}${sysconfdir}/wpa_supplicant
    install -m 644 ${WORKDIR}/etc-wpa_supplicant.mount ${D}/${systemd_system_unitdir}
    sed -Ei \
        -e 's/(Requires=.+)/\1 etc-wpa_supplicant.mount/' \
        -e 's/(After=.+)/\1 etc-wpa_supplicant.mount/' \
        ${D}/${systemd_system_unitdir}/wpa_supplicant@.service
}
