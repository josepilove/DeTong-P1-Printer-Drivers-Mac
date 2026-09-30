# DeTong P1 Label Printer driver for macOS

An open-source macOS (CUPS) driver for the **DeTong "P1 Label Printer"**, a
small USB thermal printer that macOS detects but has no driver for. Once
installed it appears in **System Settings → Printers & Scanners** and prints
from any app's Print dialog (PDFs, images, Amazon return QR codes and so on).

> Not affiliated with DeTong. Tested on **one unit** (below). Reports from other
> units and macOS versions are very welcome.

## Is this your printer?
Plug it in and run:

    system_profiler SPUSBDataType | grep -A12 "P1 Label Printer"

You should see **Vendor ID `0x3533`**, **Product ID `0x5a11`**, manufacturer
"DeTong". Its USB ID string says `CMD:LPAPI`, but it actually accepts plain
ESC/POS raster commands, which is what this driver sends. Other DeTong models
may work if they share that behaviour; that is untested.

## Install
Requires macOS and the Xcode Command Line Tools (a C compiler).

    git clone https://github.com/josepilove/detong-p1-printer-drivers-mac.git
    cd detong-p1-printer-drivers-mac
    xcode-select --install        # skip if already installed
    ./scripts/install-mac.sh

The script builds the filter, installs it and the PPD (it asks for your
password via `sudo`), and creates a printer called **DeTong_P1**. Uninstall
with `./scripts/uninstall-mac.sh`.

### Adding the printer by hand (Add Printer menu)
After running the install script once, the driver is available system-wide:
1. System Settings → Printers & Scanners → **Add Printer, Scanner or Fax…**
2. Select **P1 Label Printer**.
3. "Use" should choose **DeTong P1 Label Printer** automatically. If not,
   pick *Select Software…* and choose it from the list.

## Printing
- **Images / PDFs:** Print from Preview or any app and choose the DeTong P1.
  Use *Scale to fit* and a paper size like **42mm x 70mm**; the printable width
  is about 42 mm.
- **Command line:** `lp -d DeTong_P1 -o fit-to-page file.pdf`
- **Plain text:** `lp -d DeTong_P1 -o cpi=20 -o lpi=10 notes.txt`

### Paper sizes and options
The roll is treated as continuous paper. Available in the Print dialog / `lp -o`:

| Option | Values |
|---|---|
| Paper size | 42 mm wide, several heights, or a custom size |
| Wide ×2 / ×3 sizes | Wider virtual pages that are shrunk to fit (see below) |
| `DeTongDither` | `Threshold` (sharp, best for text and QR codes) or `Diffuse` (photos) |
| `DeTongThreshold` | `96` lighter, `128` normal, `160` darker |
| `DeTongGapTrim` | Blank dots removed between pages (default 16, about 2 mm) |

**Text editors** (TextEdit, CotEditor) add roughly 1 inch page margins, which
leaves almost no room on a 42 mm page. The Wide sizes are a workaround, but
results are mediocre. Printing images/PDFs, or plain text with `lp`, works
much better.

## How it works
`ppd/DeTong-P1.ppd` describes the printer to macOS. `src/rastertodetong.c`
is a CUPS raster filter that converts each page to 1-bit, fits it into the
384-dot head at 203 dpi, and sends it as ESC/POS `GS v 0` bands. Details of
the hardware quirks it works around are in [docs/PRINTER-NOTES.md](docs/PRINTER-NOTES.md).

- Big jobs are silently dropped by the printer, so data is sent in 64-row
  bands with a short pause between them (`DETONG_BAND_MS`, default 400).
- The head is off-centre: 11 dots on the left and 33 on the right can't print.
- A 112-dot feed ends each job so the last lines clear the tear bar.

## Known limitations
- Tested on one printer, on Apple Silicon. Intel Macs should work but are untested.
- No status readback (out of paper, cover open, etc.).
- The printer can occasionally hang until it is power-cycled. A custom CUPS
  backend that resets it before each job is a possible future fix.
- Text-editor printing is awkward (see above).

## Troubleshooting
- **Nothing prints / job vanishes:** check `lpstat -p DeTong_P1` and
  `/var/log/cups/error_log`. Power-cycle the printer, then try again.
- **Long jobs stop partway:** increase the pause, e.g. by setting
  `DETONG_BAND_MS` higher in the filter's environment, or open an issue.
- **Cancel jobs:** `cancel -a DeTong_P1`
- **Printer not in Add Printer list:** unplug and replug, and check that the
  `system_profiler` command above sees it.

## Development
    make          # build
    make test     # run the filter on a generated test image

`tests/mkraster.c` generates a sample CUPS raster stream. There are no hardware
tests; the output byte stream is checked by decoding it.

## Contributing
Issues and pull requests are welcome, especially: results from other DeTong
models, a `.pkg` installer, Intel/older-macOS testing, and status readback.

## License
[MIT](LICENSE)
