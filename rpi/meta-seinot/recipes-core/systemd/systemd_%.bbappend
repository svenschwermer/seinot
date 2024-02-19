FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += "\
    file://show-status.conf \
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
}
