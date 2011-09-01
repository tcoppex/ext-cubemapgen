/********************************************************************
simple routines for reading and writing uncompressed 32-bit targas
into CSurfaceImage  which stores image data internally using float32


********************************************************************/
#include <stdio.h>
#include "CImageSurface.h"


#pragma warning(disable :4103)  //don't complain about #pragma pack(1)
#pragma pack(1)         // targa header has non-aligned uint16 data and needs tight packing

struct TGAHeaderInfo
{
   uint8  idlen;    //length of optional identification sequence
   uint8  cmtype;   //indicates whether a palette is present
   uint8  imtype;   //image data type (e.g., uncompressed RGB)
   uint16 cmorg;    //first palette index, if present
   uint16 cmcnt;    //number of palette entries, if present
   uint8  cmsize;   //number of bits per palette entry
   uint16 imxorg;   //horiz pixel coordinate of lower left of image
   uint16 imyorg;   //vert pixel coordinate of lower left of image
   uint16 imwidth;  //image width in pixels
   uint16 imheight; //image height in pixels
   uint8  imdepth;  //image color depth (bits per pixel)
   uint8  imdesc;   //image attribute flags
};


bool8 LoadTGA(char *aFilename, CImageSurface **aImg );
bool8 SaveTGA(char *aFilename, CImageSurface *aImg );

