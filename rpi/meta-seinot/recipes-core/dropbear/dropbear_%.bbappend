FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "\
    file://authorized_keys \
    "

do_install:append() {
    install -d ${D}${ROOT_HOME}/.ssh
    install -m 0600 ${WORKDIR}/authorized_keys ${D}${ROOT_HOME}/.ssh/authorized_keys
}

FILES:${PN} += "${ROOT_HOME}/.ssh"
