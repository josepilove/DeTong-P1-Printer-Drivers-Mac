# DeTong P1 Label Printer: measured behaviour

USB 3533:5a11, 1284 ID `CMD:LPAPI` (misleading: it accepts plain ESC/POS raster).
Bulk OUT ep 0x01, printer-class interface 0. Status readback untested.

| Purpose | Bytes |
|---|---|
| Init | `1B 40` |
| Raster | `1D 76 30 00 xL xH yL yH data` (1 bpp, MSB first, 1 = black) |
| Feed | `1B 4A n` (n <= 255) |

Head: 384 dots @ 203 dpi. Unprintable: dots 0-10 and 351-383, so 340 usable.
Tear bar is ~18 mm past the last row: end each page with a 112-dot feed.

Quirks: jobs > ~25 KB vanish silently -> send 64-row bands, 4096-byte writes,
0.4 s pause per band. The printer can hang until USB-reset (not yet handled
by this driver; would need a custom backend).
