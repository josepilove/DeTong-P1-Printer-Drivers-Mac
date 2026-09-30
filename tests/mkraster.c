/* Writes a CUPS raster stream: 340x400 8-bit gray, box border + diagonal. */
#include <cups/raster.h>
#include <string.h>
#include <stdlib.h>
int main(void){
  cups_raster_t *r = cupsRasterOpen(1, CUPS_RASTER_WRITE);
  cups_page_header2_t h; memset(&h,0,sizeof h);
  strcpy(h.MediaClass,"PwgRaster");
  h.HWResolution[0]=h.HWResolution[1]=203;
  h.cupsWidth=340; h.cupsHeight=400; h.cupsBitsPerColor=8; h.cupsBitsPerPixel=8;
  h.cupsBytesPerLine=340; h.cupsColorSpace=CUPS_CSPACE_W; h.cupsNumColors=1;
  h.cupsColorOrder=CUPS_ORDER_CHUNKED; h.NumCopies=1;
  h.PageSize[0]=120; h.PageSize[1]=142;
  cupsRasterWriteHeader2(r,&h);
  unsigned char l[340];
  for(int y=0;y<400;y++){ memset(l,255,340);
    if(y<3||y>=397) memset(l,0,340); else { l[0]=l[1]=l[338]=l[339]=0; l[(y*339)/399]=0; }
    cupsRasterWritePixels(r,l,340); }
  cupsRasterClose(r); return 0; }
