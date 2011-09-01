//---------------------------------------------------------------------------------------------
// A simple example for using the CubeMapGen library
//
//---------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <tchar.h>
#include "CCubeMapProcessor.h"
#include "ErrorMsg.h"
#include "TGAReadWrite.h"

//filter settings
float32  g_BaseFilterAngle = 20.0;
float32  g_InitialMipAngle = 1.0;
float32  g_MipAnglePerLevelScale = 2.0;
int32    g_FilterType = CP_FILTER_TYPE_ANGULAR_GAUSSIAN;
int32    g_FixupType = CP_FIXUP_PULL_LINEAR; 
int32    g_FixupWidth = 3;
bool8    g_bUseSolidAngle = TRUE;      

char *gInputCubeMapFilenames[6] = 
{
   "InputCubemap_c00.tga",
   "InputCubemap_c01.tga",
   "InputCubemap_c02.tga",
   "InputCubemap_c03.tga",
   "InputCubemap_c04.tga",
   "InputCubemap_c05.tga"
};

char gOutputCubeMapFilenamePrefix[] = "OutputCubemap";



void ErrorOutputCallback(WCHAR *msg, WCHAR *title)
{
   wprintf(L"\n%s:%s\n", title, msg);
   wprintf(L"Press a key to continue\n");

   _getch();
}

int _tmain(int argc, _TCHAR* argv[])
{   
   //cubemap processor
   CCubeMapProcessor cubeMapProcessor;

   //image surface used to load and save image files.
   CImageSurface *cImg = NULL;

   int32 i, j;
   bool8 bLoadOK;

   //---------------------------------------------------------------------------------------------
   //Set Error Reporting Output Callback
   //---------------------------------------------------------------------------------------------
   SetErrorMessageCallback( ErrorOutputCallback );

   //---------------------------------------------------------------------------------------------
   //Create cubemap processor for filtering cubemap
   //---------------------------------------------------------------------------------------------
   cubeMapProcessor.Clear();

   //read face image
   bLoadOK = LoadTGA(gInputCubeMapFilenames[0], &cImg);

   if(bLoadOK == FALSE)
   {
      wprintf(L"\nError Loading: %S\n", gInputCubeMapFilenames[0]);
      wprintf(L"Press a key to continue\n");

      _getch();

      exit(EM_FATAL_ERROR );
   }
   

   //input and output cubemap set to have save dimensions, 
   // Uses up to 16 miplevels, and up to 4 channels.
   cubeMapProcessor.Init(cImg->m_Width, cImg->m_Width, 16, 4);

   //uncomment to use 2 filtering threads for dual, or multicore CPU
   //cubeMapProcessor.m_NumFilterThreads = 2;

   //---------------------------------------------------------------------------------------------
   //Load the 6 faces of the input cubemap and copy them into the cubemap processor
   //---------------------------------------------------------------------------------------------
   for(i=0; i<6; i++)
   {  
      //read face image
      LoadTGA(gInputCubeMapFilenames[i], &cImg);

      if(bLoadOK == FALSE)
      {
         wprintf(L"\nError Loading: %S\n", gInputCubeMapFilenames[i]);
         wprintf(L"Press a key to continue\n");

         _getch();

         exit(EM_FATAL_ERROR );
      }

      //Copy into input of cubemap processor
      // note that this example compies data out a CSurfaceImage
      // for convienience, but the SetInputFaceData routines has the 
      // capability of reading buffers in a variety of formats. 
      // See  CCubeMapProcessor.h  for more detail.
      cubeMapProcessor.SetInputFaceData(
         i,                                        // a_FaceIdx, 
         CP_VAL_FLOAT32,                           // a_SrcType, 
         cImg->m_NumChannels,                      // a_SrcNumChannels, 
         cImg->m_Width * cImg->m_NumChannels * 4,  // a_SrcPitch,   (4 bytes per float)
         cImg->m_ImgData,                          //*a_SrcDataPtr, 
         1000000.0f,                               // a_MaxClamp, 
         1.0f,                                     // a_Degamma, 
         1.0f                                      // a_Scale 
         );
   }


   //---------------------------------------------------------------------------------------------
   //Filter cubemap
   //---------------------------------------------------------------------------------------------
   cubeMapProcessor.InitiateFiltering(
      g_BaseFilterAngle,         //float32 a_BaseFilterAngle, 
      g_InitialMipAngle,         //float32 a_InitialMipAngle, 
      g_MipAnglePerLevelScale,   //float32 a_MipAnglePerLevelScale, 
      g_FilterType,              //int32 a_FilterType, 
      g_FixupType,               //int32 a_FixupType, 
      g_FixupWidth,              //int32 a_FixupWidth, 
      g_bUseSolidAngle           //bool8 a_bUseSolidAngle );          
      );
   
   wprintf(L"\nFiltering progress:\n");



   //---------------------------------------------------------------------------------------------
   //Report status of filtering , and loop until filtering is complete
   //---------------------------------------------------------------------------------------------
   while( cubeMapProcessor.GetStatus() == CP_STATUS_PROCESSING)
   {
	  Sleep(200);

      wprintf(L"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
      wprintf(L"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
      wprintf(L"%s        ", cubeMapProcessor.GetFilterProgressString() );

      
   }


   //---------------------------------------------------------------------------------------------
   //Save destination cubemap
   //---------------------------------------------------------------------------------------------
   for(j=0; j<cubeMapProcessor.m_NumMipLevels; j++)
   {
      for(i=0; i<6; i++)
      {
         char outputFilename[4096];

         sprintf_s(outputFilename, 4096, "%s_m%02d_c%02d.tga", gOutputCubeMapFilenamePrefix, j, i );

         //save face image
         SaveTGA(outputFilename, &(cubeMapProcessor.m_OutputSurface[j][i]) );
      }
   }

   wprintf(L"\n\nCompleted Filtering: Press a key to continue\n");

   _getch();

	return 0;
}

