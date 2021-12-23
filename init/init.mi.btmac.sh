#!/vendor/bin/sh
# Copyright (C) 2021 KudProject Development
# SPDX-License-Identifier: GPL-3.0-or-later OR Apache-2.0

LOG_TAG="MiSetBtMac"
logi () { log -t "$LOG_TAG" -p i "$@"; }

# hex binary containing mac address
BT_MAC_HEX_PATH="/data/vendor/mac_addr/bt.mac";
if [ ! -f "$BT_MAC_HEX_PATH" ]; then
    logi "bt.mac file not found, exiting"
    exit
fi

# raw mac address without colons
RAW_MAC=$(xxd -p "$BT_MAC_HEX_PATH");
# convert it into format recognized by bluetooth hal
DEC_MAC=$(echo "$RAW_MAC" | sed 's!^M$!!;s!\-!!g;s!\.!!g;s!\(..\)!\1:!g;s!:$!!')
# set the mac address using persist property
setprop persist.vendor.service.bdroid.bdaddr "$DEC_MAC"
logi "bt.mac file found, setting mac addr"
