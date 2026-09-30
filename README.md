# DeTong P1 Label Printer: macOS (CUPS) driver

A CUPS raster filter + PPD for the DeTong P1 (USB 3533:5a11).

- `src/rastertodetong.c`: filter (CUPS raster -> ESC/POS, banded and paced)
- `ppd/DeTong-P1.ppd`: 42 mm wide continuous roll, 203 dpi, threshold/dither options
- `scripts/install-mac.sh`: build + install + create the `DeTong_P1` queue
- `docs/PRINTER-NOTES.md`: hardware findings

## Install (on the Mac)
    xcode-select --install     # once
    ./scripts/install-mac.sh

## Status
Filter output is verified in software only (decoded byte stream); **not yet
tested on hardware**. Known risks: macOS filter sandbox, stock `usb` backend
has no reset-before-job, pacing relies on the backend forwarding data as it
arrives. Tune pause with `DETONG_BAND_MS`.
