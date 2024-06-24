FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += "\
    file://show-status.conf \
    file://etc-systemd-network.mount \
    file://systemd-rfkill-override.conf \
    "

PACKAGECONFIG = "\
    kmod \
    networkd \
    resolved \
    timesyncd \
    rfkill \
    link-udev-shared \
    "

RRECOMMENDS:${PN}:remove = "udev-hwdb"

do_install:append () {
    install -m 0644 -D -t ${D}${sysconfdir}/systemd/system.conf.d ${WORKDIR}/show-status.conf
    install -m 644 ${WORKDIR}/etc-systemd-network.mount ${D}${systemd_system_unitdir}
    sed -Ei \
        -e 's/(After=.+)/\1 etc-systemd-network.mount/' \
        -e 's/(Wants=.+)/\1 wpa_supplicant@wlan0.service/' \
        -e '/^Wants=/a Requires=etc-systemd-network.mount' \
        ${D}${systemd_system_unitdir}/systemd-networkd.service
    install -d ${D}${systemd_system_unitdir}/systemd-rfkill.service.d
    install -m 0644 ${WORKDIR}/systemd-rfkill-override.conf ${D}${systemd_system_unitdir}/systemd-rfkill.service.d/override.conf
}
