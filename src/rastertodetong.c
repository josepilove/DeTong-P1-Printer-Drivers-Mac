/*
 * rastertodetong - CUPS raster filter for the DeTong "P1 Label Printer"
 * (USB 3533:5a11, ESC/POS raster, 384-dot head at 203 dpi).
 *
 * Quirks handled here (measured on real hardware, see docs/PRINTER-NOTES.md):
 *   - jobs larger than ~25 KB are silently dropped, so the image is sent as
 *     64-row GS v 0 bands, 4096-byte writes, with a pause after each band;
 *   - the last lines sit behind the tear bar, so every page ends with a
 *     112-dot feed;
 *   - the head is off-centre: dots 0..10 and 351..383 are unprintable.
 *
 * Usage: rastertodetong job user title copies options [file]
 * Env:   DETONG_BAND_MS  pause after each band in ms (default 400)
 */
#include <cups/cups.h>
#include <cups/raster.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>

#define HEAD_DOTS   384
#define HEAD_BYTES  (HEAD_DOTS / 8)
#define LEFT_MARGIN 11
#define RIGHT_MARGIN 33
#define PRINTABLE   (HEAD_DOTS - LEFT_MARGIN - RIGHT_MARGIN) /* 340 */
#define BAND_ROWS   64
#define WRITE_CHUNK 4096
#define TRAILING_FEED 112

static void out(const unsigned char *p, size_t n)
{
  while (n) {
    size_t c = n > WRITE_CHUNK ? WRITE_CHUNK : n;
    fwrite(p, 1, c, stdout);
    fflush(stdout);
    p += c; n -= c;
  }
}

static void feed(int dots)
{
  while (dots > 0) {
    unsigned char n = dots > 255 ? 255 : dots;
    unsigned char cmd[3] = { 0x1B, 0x4A, n };
    out(cmd, 3);
    dots -= n;
  }
}

static void send_band(const unsigned char *rows, int nrows, int pause_ms)
{
  unsigned char hdr[8] = { 0x1D, 0x76, 0x30, 0x00,
                           HEAD_BYTES & 255, HEAD_BYTES >> 8,
                           nrows & 255, nrows >> 8 };
  out(hdr, 8);
  out(rows, (size_t)nrows * HEAD_BYTES);
  if (pause_ms > 0) usleep((useconds_t)pause_ms * 1000);
}

/* Append one row to the band, sending the band when full. */
static void add_row(unsigned char *band, int *band_rows, const unsigned char *row, int pause_ms)
{
  memcpy(band + (size_t)*band_rows * HEAD_BYTES, row, HEAD_BYTES);
  if (++*band_rows == BAND_ROWS) {
    send_band(band, *band_rows, pause_ms);
    *band_rows = 0;
  }
}

/* 8-bit luminance, 0 = black, 255 = white, for one raster line. */
static void to_gray(const unsigned char *in, unsigned w,
                    cups_cspace_t cs, unsigned bpp_bytes, unsigned char *g)
{
  for (unsigned x = 0; x < w; x++) {
    const unsigned char *p = in + x * bpp_bytes;
    switch (cs) {
      case CUPS_CSPACE_K:  g[x] = 255 - p[0]; break;
      case CUPS_CSPACE_RGB:
      case CUPS_CSPACE_SRGB:
      case CUPS_CSPACE_ADOBERGB:
        g[x] = (unsigned char)((p[0] * 30 + p[1] * 59 + p[2] * 11) / 100); break;
      default:             g[x] = p[0]; break;      /* W / SW */
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc < 6 || argc > 7) {
    fprintf(stderr, "ERROR: %s job-id user title copies options [file]\n", argv[0]);
    return 1;
  }

  int fd = 0;
  if (argc == 7 && (fd = open(argv[6], O_RDONLY)) < 0) {
    perror("ERROR: Unable to open raster file");
    return 1;
  }

  cups_option_t *opts = NULL;
  int nopts = cupsParseOptions(argv[5], 0, &opts);
  const char *dither_opt = cupsGetOption("DeTongDither", nopts, opts);
  int dither = dither_opt && !strcasecmp(dither_opt, "Diffuse");
  const char *thr_opt = cupsGetOption("DeTongThreshold", nopts, opts);
  int threshold = thr_opt ? atoi(thr_opt) : 128;
  const char *ms_env = getenv("DETONG_BAND_MS");
  int pause_ms = ms_env ? atoi(ms_env) : 400;
  /* Blank dot rows dropped at each page boundary (16 dots = 2 mm). */
  const char *trim_opt = cupsGetOption("DeTongGapTrim", nopts, opts);
  int gap_trim = trim_opt ? atoi(trim_opt) : 16;
  unsigned char *band = calloc(BAND_ROWS, HEAD_BYTES);
  unsigned char rowbuf[HEAD_BYTES], blank[HEAD_BYTES] = { 0 };
  int band_rows = 0;
  long pending_blank = 0;   /* blank rows held back until content follows */

  cups_raster_t *ras = cupsRasterOpen(fd, CUPS_RASTER_READ);
  cups_page_header2_t h;
  int page = 0;
  int started = 0;

  while (cupsRasterReadHeader2(ras, &h)) {
    if (h.cupsBitsPerColor != 8 || h.cupsNumColors < 1) {
      fprintf(stderr, "ERROR: Need 8-bit raster, got %u bits/colour\n", h.cupsBitsPerColor);
      return 1;
    }
    if (!started) {
      static const unsigned char init[2] = { 0x1B, 0x40 };
      out(init, 2);
      started = 1;
    }
    page++;
    fprintf(stderr, "PAGE: %d 1\n", page);
    fprintf(stderr, "INFO: Printing page %d (%ux%u px)\n", page, h.cupsWidth, h.cupsHeight);

    unsigned w = h.cupsWidth;
    unsigned char *line = malloc(h.cupsBytesPerLine);
    unsigned char *gray = malloc(w + 2);
    int *err_cur = calloc(w + 2, sizeof(int)), *err_nxt = calloc(w + 2, sizeof(int));
    unsigned bpp_bytes = h.cupsBytesPerLine / (w ? w : 1);
    if (page > 1) {           /* shorten the gap between pages */
      pending_blank -= gap_trim;
      if (pending_blank < 0) pending_blank = 0;
    }

    /* Wide pages (for apps that force ~1" margins) are shrunk by an integer
     * factor n so that the page width maps onto the printable dots. */
    unsigned n = (w >= PRINTABLE * 3 / 2) ? (w + PRINTABLE / 2) / PRINTABLE : 1;
    unsigned wo = w / n;
    unsigned char *rawg = malloc(w + 2);
    unsigned *acc = calloc(wo + 1, sizeof(unsigned));
    int eof = 0;

    for (unsigned y = 0; y < h.cupsHeight && !eof; y += n) {
      unsigned rk = 0;
      memset(acc, 0, (wo + 1) * sizeof(unsigned));
      for (unsigned k = 0; k < n && y + k < h.cupsHeight; k++) {
        if (cupsRasterReadPixels(ras, line, h.cupsBytesPerLine) != h.cupsBytesPerLine) {
          eof = 1;
          break;
        }
        to_gray(line, w, h.cupsColorSpace, bpp_bytes, rawg);
        for (unsigned x = 0; x < wo * n; x++) acc[x / n] += rawg[x];
        rk++;
      }
      if (!rk) break;
      for (unsigned x = 0; x < wo; x++) gray[x] = (unsigned char)(acc[x] / (n * rk));

      unsigned char *row = rowbuf;
      memset(row, 0, HEAD_BYTES);
      for (unsigned x = 0; x < wo && x < PRINTABLE; x++) {
        int v = gray[x];
        int black;
        if (dither) {
          v += err_cur[x + 1];
          black = v < 128;
          int e = v - (black ? 0 : 255);
          err_cur[x + 2] += e * 7 / 16;
          err_nxt[x]     += e * 3 / 16;
          err_nxt[x + 1] += e * 5 / 16;
          err_nxt[x + 2] += e * 1 / 16;
        } else {
          black = v < threshold;
        }
        if (black) {
          unsigned dot = x + LEFT_MARGIN;
          row[dot >> 3] |= 0x80 >> (dot & 7);
        }
      }
      if (dither) {
        int *t = err_cur; err_cur = err_nxt; err_nxt = t;
        memset(err_nxt, 0, (w + 2) * sizeof(int));
      }
      if (memcmp(row, blank, HEAD_BYTES) == 0) {
        pending_blank++;
      } else {
        for (; pending_blank > 0; pending_blank--)
          add_row(band, &band_rows, blank, pause_ms);
        add_row(band, &band_rows, row, pause_ms);
      }
    }

    free(line); free(gray); free(rawg); free(acc); free(err_cur); free(err_nxt);
  }

  for (; pending_blank > 0; pending_blank--)
    add_row(band, &band_rows, blank, pause_ms);
  if (band_rows) send_band(band, band_rows, pause_ms);
  free(band);

  /* One tear feed per job: pages are slices of a continuous roll. */
  if (started) feed(TRAILING_FEED);
  cupsRasterClose(ras);
  if (fd) close(fd);
  cupsFreeOptions(nopts, opts);
  return page ? 0 : 1;
}
