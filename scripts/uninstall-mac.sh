#!/bin/sh
# Remove the DeTong P1 queue, filter and PPD.
[ "$(uname)" = Darwin ] || { echo "macOS only"; exit 1; }
sudo lpadmin -x DeTong_P1 2>/dev/null
sudo rm -f /usr/libexec/cups/filter/rastertodetong \
           /Library/Printers/PPDs/Contents/Resources/DeTong-P1.ppd
echo "Removed."
