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
Verified on hardware: short and long jobs print.
has no reset-before-job, pacing relies on the backend forwarding data as it
arrives. Tune pause with `DETONG_BAND_MS`.

## Printing text
`lp` uses 10 chars/inch by default, which is huge on a 42 mm strip. Try:

    lp -d DeTong_P1 -o cpi=20 -o lpi=10 README.md

Make it the default: `lpoptions -p DeTong_P1 -o cpi=20 -o lpi=10`

## Page gap
The filter drops 2 mm (16 dots) of blank rows at each page boundary. Change it
with `-o DeTongGapTrim=<dots>` (203 dots = 1 inch; 0 disables).
