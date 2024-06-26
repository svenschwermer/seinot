#!/bin/sh

set -eu

if [ $# -ne 2 ]; then
    echo "ERROR: Expected 2 arguments: SSID PASSPHRASE" >&2
    exit 1
fi

wpa_passphrase "$1" "$2" > /etc/wpa_supplicant/wpa_supplicant-wlan0.conf

cat <<EOF > /etc/systemd/network/wlan0.network
[Match]
Name=wlan0

[Network]
DHCP=ipv4
EOF

systemctl stop wpa_supplicant@wlan0.service
systemctl restart systemd-networkd
