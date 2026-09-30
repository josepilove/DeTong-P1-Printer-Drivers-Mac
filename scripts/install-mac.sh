#!/bin/sh
# Build and install the DeTong P1 driver, then add the printer queue.
# Needs Xcode Command Line Tools (xcode-select --install). Run from repo root.
set -e
[ "$(uname)" = Darwin ] || { echo "macOS only"; exit 1; }

make
sudo install -m 0755 -o root -g wheel build/rastertodetong /usr/libexec/cups/filter/rastertodetong
sudo mkdir -p /Library/Printers/PPDs/Contents/Resources
sudo install -m 0644 ppd/DeTong-P1.ppd /Library/Printers/PPDs/Contents/Resources/DeTong-P1.ppd

# Find the USB URI of the printer as CUPS sees it.
URI=$(lpinfo -v 2>/dev/null | awk '/usb:\/\/DeTong/ {print $2; exit}')
[ -n "$URI" ] || { echo "Printer not found; is it plugged in? (lpinfo -v)"; exit 1; }
echo "Using $URI"

sudo lpadmin -p DeTong_P1 -E -v "$URI" \
     -P /Library/Printers/PPDs/Contents/Resources/DeTong-P1.ppd \
     -o printer-is-shared=false
echo "Done. Test:  lp -d DeTong_P1 -o fit-to-page some.pdf"
