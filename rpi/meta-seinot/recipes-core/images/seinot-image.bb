SUMMARY = "A small image catered to be used in the seinot jukebox"

IMAGE_FEATURES = "\
    allow-root-login \
    read-only-rootfs \
    ssh-server-dropbear \
    ${EXTRA_IMAGE_FEATURES} \
    "
IMAGE_INSTALL = "\
    alsa-utils \
    packagegroup-core-boot \
    seinot \
    systemd-analyze \
    wifi \
    wpa-supplicant \
    ${CORE_IMAGE_EXTRA_INSTALL} \
    ${MACHINE_EXTRA_RRECOMMENDS} \
    "

IMAGE_LINGUAS = " "

LICENSE = "MIT"

inherit core-image

IMAGE_ROOTFS_SIZE ?= "8192"
IMAGE_ROOTFS_EXTRA_SPACE:append = "${@bb.utils.contains("DISTRO_FEATURES", "systemd", " + 4096", "", d)}"

make_seinot_mount_dirs () {
    mkdir -p ${IMAGE_ROOTFS}/mnt/data ${IMAGE_ROOTFS}/mnt/state
}
ROOTFS_POSTPROCESS_COMMAND += 'make_seinot_mount_dirs;'
