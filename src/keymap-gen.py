#!/usr/bin/env python

# Copyright (C) 2017 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

# A script for creating conversion maps for various keyboard layouts.
# Based on keymap-gen.pl
#
# Base data sources:
#
#  linux:     Linux: linux/input.h                         (master set)
#    osx:      OS-X: Carbon/HIToolbox/Events.h             (manually mapped)
# atset1:  AT Set 1: linux/drivers/input/keyboard/atkbd.c
#        (atkbd_set2_keycode + atkbd_unxlate_table)
# atset2:  AT Set 2: linux/drivers/input/keyboard/atkbd.c  (atkbd_set2_keycode)
# atset3:  AT Set 3: linux/drivers/input/keyboard/atkbd.c  (atkbd_set3_keycode)
#     xt:        XT: linux/drivers/input/keyboard/xt.c     (xtkbd_keycode)
#  xtkbd: Linux RAW: linux/drivers/char/keyboard.c         (x86_keycodes)
#    usb:   USB HID: linux/drivers/hid/usbhid/usbkbd.c     (usb_kbd_keycode)
#  win32:     Win32: mingw32/winuser.h                     (manually mapped)
# xwinxt:   XWin XT: xorg-server/hw/xwin/{winkeybd.c,winkeynames.h}
#        (xt + manually transcribed)
# xkbdxt:   XKBD XT: xf86-input-keyboard/src/at_scancode.c
#        (xt + manually transcribed)
#    x11: X11 keysyms:
#        http://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h
#
# Derived data sources
#
#    xorgevdev: Xorg +  evdev: linux + an offset
#      xorgkbd: Xorg +    kbd: xkbdxt + an offset
#  xorgxquartz: Xorg +   OS-X: osx + an offset
#     xorgxwin: Xorg + Cygwin: xwinxt + an offset
#          rfb:   XT over RFB: xtkbd + special re-encoding of high bit

import sys
import csv

maps = frozenset(["linux", "osx", "atset1", "atset2", "atset3", "xt", "xtkbd",
                  "usb", "win32", "xwinxt", "xkbdxt", "x11", "xorgevdev",
                  "xorgkbd", "xorgxquartz", "xorgxwin", "rfb"])

# csvColumns = frozenset(["Linux Name", "Linux Keycode", "OS-X Name",
#                         "OS-X Keycode", "AT set1 keycode", "AT set2 keycode",
#                         "AT set3 keycode", "XT", "XT KBD", "USB Keycodes",
#                         "Win32 Name", "Win32 Keycode", "Xwin XT",
#                         "Xfree86 KBD XT", "X11 keysym", "X11 keycode"])


def usage(exitcode=0):
    msg = "usage: " + sys.argv[0] + "KEYMAPS SRCMAP DSTMAP\n\n"
    msg + "Valid keymaps are:\n"
    msg += "\n".join(maps)
    print(msg)
    sys.exit(exitcode)


def get_name_and_code_for_map(row, mapname):
    map_to_dict = {
        "linux": ["Linux Name", "Linux Keycode"],
        "osx": ["OS-X Name", "OS-X Keycode"],
        "win32": ["Win32 Name", "Win32 Keycode"],
        "x11": ["X11 keysym", "X11 keycode"]
    }
    base_lambda = lambda val: int(val)
    hex_or_dec = lambda val: int(val, 16) if val.startswith("0x") else int(val)
    map_to_keycode = {
        "atset1": ["AT set1 keycode", base_lambda],
        "atset2": ["AT set2 keycode", base_lambda],
        "atset3": ["AT set3 keycode", base_lambda],
        "xt": ["XT", lambda val: int(val)],
        "xtkbd": ["XT KBD", lambda val: int(val)],
        "xwinxt": ["Xwin XT", lambda val: int(val)],
        "xkbdxt": ["Xfree86 KBD XT", hex_or_dec],
        "usb": ["USB Keycodes", lambda val: int(val)],
        "xorgevdev": ["Linux Keycode", lambda val: int(val, 16) + 8],
        "xorgkbd": ["Xfree86 KBD XT", lambda val: hex_or_dec(val) + 8],
        "xorgxquartz": ["OS-X Keycode", lambda val: int(val, 16) + 8],
        "xorgxwin": ["Xwin XT", lambda val: int(val, 16) + 8],
        "rfb": ["XT KBD", lambda val: (val & 0x100) >> 1 | (val & 0x7f)]
    }
    if map_to_dict.has_key(mapname):
        try:
            name = row[map_to_dict[mapname][0]]
            code = int(row[map_to_dict[mapname][1]], 16)
            return name, code
        except:
            return "", 0
    elif map_to_keycode.has_key(mapname):
        try:
            code = map_to_keycode[mapname][1](row[map_to_keycode[mapname][0]])
            return "", code
        except:
            return "", 0


def map_from_csv(file, srcmap, dstmap):
    with open(file, 'rb') as csvfile:
        i = 0
        reader = csv.DictReader(csvfile)
        mapping = dict()
        for row in reader:
            srcname, i = get_name_and_code_for_map(row, srcmap)
            dstname, j = get_name_and_code_for_map(row, dstmap)
            if i == 0 or j == 0 or mapping.has_key(i):
                continue
            if srcname is not "":
                srcname = " (%s)" % srcname
            if dstname is not "":
                dstname = " (%s)" % dstname
            data = "[0x%(from)x] = 0x%(to)x," % {"from": i, "to": j}
            comment = "%(from)d%(srcname)s => %(to)d%(dstname)s" % {
                      "from": i, "srcname": srcname,
                      "to": j, "dstname": dstname}
            if "linux" not in [srcmap, dstmap]:
                comment += " via %(linux)d (%(via)s)" % {
                           "linux": int(row["Linux Keycode"]),
                           "via": row["Linux Name"]}
            mapping[i] = [j, comment]
        return mapping
    usage(1)


def main():
    if len(sys.argv) != 4:
        usage(1)
    if "-h" in sys.argv or "--help" in sys.argv:
        usage()
    for keymap in sys.argv[2:]:
        if keymap not in maps:
            print("Unknown keymap " + keymap)
            usage(1)
    mapping = map_from_csv(*sys.argv[1:])
    srcmap, dstmap = sys.argv[2:]
    print("static const guint16 keymap_%(src)s2%(dst)s[] = {" %
          {"src": srcmap, "dst": dstmap})
    for key, (value, comment) in mapping.iteritems():
        data = "[0x%(from)x] = 0x%(to)x," % {"from": key, "to": value}
        print("  %(data)-20s /* %(comment)s */" %
              {"data": data, "comment": comment})
    print("};")


if __name__ == "__main__":
    main()
