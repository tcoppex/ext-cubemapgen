/********************************************************************
super simple routines for reading and writing uncompressed 
24 or 32-bit targas

********************************************************************/
#include "TGAReadWrite.h"


/********************************************************************
Loads targa file. If aImg is non-null, and the dimensions match the 

********************************************************************/
bool8 LoadTGA(char *aFilename, CImageSurface **aImg )
{
   TGAHeaderInfo header;
   FILE   *ifp;
   uint32 tgaImgDataSize;
   uint8  *tmpImgBuffer;

   errno_t result = fopen_s( &ifp, aFilename, "rb");

   if( result != 0 )
   {
      return FALSE;
   }

   fread(&header, sizeof(TGAHeaderInfo), 1, ifp);

   //If image type not supported
   if(header.imtype != 2 )
   {
      fclose(ifp);
      return FALSE;
   }

   //Exit if a color map is found (color map not supported)
   if(header.cmtype != 0 )
   {
      fclose(ifp);
      return FALSE;
   }

   //16-bit image not supported
   if(header.imdepth == 16 )
   {
      fclose(ifp);
      return FALSE;
   }

   if((*aImg) == NULL)
   {
      (*aImg) = new CImageSurface();
   }


   (*aImg)->Clear();
   (*aImg)->Init(header.imwidth, header.imheight, header.imdepth /8 );

   //read past header additional info
   fseek(ifp, header.idlen, SEEK_CUR);

   //file temp buffer to copy into CImageSurface 
   //  divide by 8 since imDepth specifies file size as number of bits
   tgaImgDataSize = (header.imwidth * header.imheight * header.imdepth) / 8;
   tmpImgBuffer = new uint8[ tgaImgDataSize ];  

   if(tmpImgBuffer == NULL)
   {
      fclose(ifp);
      return FALSE;
   }

   fread(tmpImgBuffer, 1, tgaImgDataSize, ifp);
   
   //load data into surface image
   (*aImg)->SetImageData( CP_VAL_UNORM8, header.imdepth / 8,  header.imwidth * header.imdepth / 8, tmpImgBuffer);

   delete [] tmpImgBuffer;

   fclose(ifp);

   return TRUE;
}


/********************************************************************
Saves targa file. If aImg is non-null, and the dimensions match the 

********************************************************************/
bool8 SaveTGA(char *aFilename, CImageSurface *aImg )
{
   TGAHeaderInfo header;
   FILE *ofp;
   uint32 tgaImgDataSize;
   uint8  *tmpImgBuffer;
   
   header.cmcnt = 0;
   header.cmorg = 0;
   header.cmsize = 0;
   header.cmtype = 0;
   header.idlen = 0;
   header.imdepth = aImg->m_NumChannels * 8;
   header.imdesc = 0;
   header.imheight = aImg->m_Height;
   header.imtype = 2;
   header.imwidth = aImg->m_Width;
   header.imxorg = 0;
   header.imyorg = 0;

   errno_t result = fopen_s( &ofp, aFilename, "wb" );

   if( result != 0 )
   {
      return FALSE;
   }

   fwrite(&header, sizeof(TGAHeaderInfo), 1, ofp);

   //File temp buffer to copy out of CImageSurface 
   //  divide by 8 since imDepth specifies file size as number of bits   
   tgaImgDataSize = (header.imwidth * header.imheight * header.imdepth) / 8;
   tmpImgBuffer = new uint8[ tgaImgDataSize ];  

   if(tmpImgBuffer == NULL)
   {
      fclose(ofp);
      return FALSE;
   }
   
   //copy data into temp buffer
   aImg->GetImageData( CP_VAL_UNORM8, header.imdepth / 8,  header.imwidth * header.imdepth / 8, tmpImgBuffer);

   //write data to disk
   fwrite(tmpImgBuffer, 1, tgaImgDataSize, ofp);
   delete [] tmpImgBuffer;

   fclose(ofp);

   return TRUE;
}
