//--------------------------------------------------------------------------------------
// File: CCubeGenApp.cpp
//
// CubeGen application class and member functions
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------
#include "CCubeGenApp.h"


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
CCubeGenApp::CCubeGenApp(void)
{
   m_bInitialized = FALSE;

   m_bErrorOccurred = FALSE;

   //no filtering so output cubemap not valid yet
   m_bValidOutputCubemap = FALSE;   

   //default settings
   m_FramesSinceLastRefresh = 0;
   m_bOutputPeriodicRefresh = TRUE;

   m_InputCubeMapSize = 128;
   m_InputCubeMapFormat = D3DFMT_A8R8G8B8;
   m_OutputCubeMapSize = 128;
   //m_OutputCubeMapFormat = D3DFMT_A32B32G32R32F;
   //m_OutputCubeMapFormat = D3DFMT_A16B16G16R16;
   m_OutputCubeMapFormat = D3DFMT_A8R8G8B8;


   //default display options
   m_RenderTechnique = CG_RENDER_NORMAL;
   m_DisplayCubeSource = CG_CUBE_DISPLAY_INPUT;
   m_bMipLevelSelectEnable = FALSE;
   m_MipLevelDisplayed = 0;
   m_bShowAlpha = FALSE;
   m_bDrawSkySphere = FALSE;

   //clamp max mip level using sampler state MaxMipLevel
   m_bMipLevelClampEnable = FALSE;

   // SL BEGIN
   m_bUseMultithread = TRUE;
   // SL END

   //write mip level to alpha so the cube maps mip level can be determined by ps.2.0 or ps.2.b shaders
   m_bWriteMipLevelIntoAlpha = TRUE;

   //selected cube face to load/manipulate
   m_SelectedCubeFace = CG_CUBE_SELECT_FACE_XPOS;

   //default filtering options
   // SL BEGIN
   m_FilterTech = CP_FILTER_TYPE_COSINE_POWER;
   // SL END
   m_BaseFilterAngle = 0.0f;
   m_MipInitialFilterAngle = 1.0f;
   m_MipFilterAngleScale = 2.0f; 
   m_bUseSolidAngleWeighting = TRUE;
   // SL BEGIN
   m_SpecularPower = 2048.0f; // Default to 2048 because it is a fast computation when you swtich to cosinus power filter
   m_CosinePowerDropPerMip = 0.25;
   m_NumMipmap = 0;
   m_CosinePowerMipmapChainMode = CP_COSINEPOWER_CHAIN_DROP;
   m_bExcludeBase = FALSE;
   m_bIrradianceCubemap = FALSE;
   m_LightingModel = FALSE;
   m_GlossScale = 10.0f;
   m_GlossBias	= 1.0f;
   // SL END

   // SL BEGIN
   m_EdgeFixupTech = CP_FIXUP_WARP;
   // SL END
   m_bCubeEdgeFixup = TRUE;
   m_EdgeFixupWidth = 1;

   m_bWriteMipLevelIntoAlpha = FALSE;
   m_bFlipFaceSurfacesOnImport = FALSE;
   m_bFlipFaceSurfacesOnExport = FALSE;
   m_bExportMipChain = TRUE;                 //Default export mipchain is true

   m_InputScaleFactor = 1.0f;
   m_InputDegamma = 1.0f;     
   m_InputMaxClamp = CG_DEFAULT_MAX_CLAMP;

   m_OutputScaleFactor = 1.0f;
   m_OutputGamma = 1.0f;     

   //initial scene filename
   wcscpy_s( m_SceneFilename, CG_MAX_FILENAME_LENGTH, L"" );

   //initial scene filename
   wcscpy_s( m_BaseMapFilename, CG_MAX_FILENAME_LENGTH, L"" );
   // wcscpy(m_SceneFilename, L"");

   //initial input cubemap filename blank, so no loading cubemaps
   wcscpy_s( m_InputCubeMapFilename, CG_MAX_FILENAME_LENGTH, L"" ); 

   //initial output cubemap filename
   wcscpy_s( m_OutputCubeMapFilename, CG_MAX_FILENAME_LENGTH, CG_INITIAL_OUTPUT_CUBEMAP_FILENAME );
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
CCubeGenApp::~CCubeGenApp()
{
    m_CubeMapProcessor.Clear();
    OnDestroyDevice();
}


//--------------------------------------------------------------------------------------
//Initalizes ShadowMap App
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::Init(IDirect3DDevice9 *a_pDevice)
{
    HRESULT hr;

    //set device
    m_pDevice = a_pDevice;

    //get caps
    m_pDevice->GetDeviceCaps(&m_DeviceCaps);
        
    TextureListSetDevice(m_pDevice);

    V_RETURN( InitShaders() );
    V_RETURN( LoadBaseMap() );
    V_RETURN( LoadInputCubeMap() );
    V_RETURN( CreateOutputCubeMap(0) );

    LoadObjects();

    m_bInitialized = TRUE;

    //invalidate current output cubemap
    m_bValidOutputCubemap = FALSE;

    return S_OK;
}


//--------------------------------------------------------------------------------------
//init shaders
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::InitShaders(void)
{
   HRESULT hr;

   hr = m_pEffect.LoadShader( CG_EFFECT_FILENAME, m_pDevice);

   if(FAILED(hr))
   {
      //if compile error, output compile error message
      if(hr != STG_E_FILENOTFOUND)
      {
         OutputMessage(L"Error creating Effect, Loading effects file from resource. \n\n %s", m_pEffect.m_ErrorMsg);
      }
     
      hr = m_pEffect.LoadShaderFromResource( CG_EFFECT_RESOURCE_ID, m_pDevice);

      OutputFatalMessageOnFail(hr, m_pEffect.m_ErrorMsg );    
   }

   return hr;
}


//--------------------------------------------------------------------------------------
//load basemap to apply to texture
//
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::LoadBaseMap(void)
{

    if( wcscmp(m_BaseMapFilename, L"") == 0)
    {   //if empty filename, create 64x64 white basemap
        OutputFatalMessageOnFail(m_pBaseMap.CreateTexture(64, 64, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED),
            L"Exiting, cannot create default basemap.");
        m_pBaseMap.FillTexture( D3DCOLOR_RGBA(255, 255, 255, 255 ) );
        //m_pBaseMap.LoadTextureFromResource(IDR_UITEXTURE  );
    }
    else
    {
        //else attempt to load basemap 
        if(FAILED(m_pBaseMap.LoadTexture(m_BaseMapFilename)))
        {
            m_bErrorOccurred = TRUE;
            OutputMessage(L"Error: Cannot Load Texture %s", m_BaseMapFilename);
            wcscpy_s( m_BaseMapFilename, CG_MAX_FILENAME_LENGTH, L"" );
            return(LoadBaseMap());
        }
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
//Loads entire cubemap + mipchain for the Cubemap
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::LoadInputCubeMap(void)
{
    if( wcscmp(m_InputCubeMapFilename, L"") == 0)
    {   
        D3DCOLOR faceColors[6] = {
            D3DCOLOR_RGBA(255, 0, 0, 255 ),     //red
            D3DCOLOR_RGBA(0, 255, 255, 255 ),   //cyan
            D3DCOLOR_RGBA(0, 255, 0, 255 ),     //green
            D3DCOLOR_RGBA(255, 0, 255, 255 ),   //magenta
            D3DCOLOR_RGBA(0, 0, 255, 255 ),     //blue
            D3DCOLOR_RGBA(255, 255, 0, 255 )    //yellow        
            };

        //if no filename, create blank cubemap
        OutputFatalMessageOnFail(
            m_pInputCubeMap.CreateCubeTexture(m_InputCubeMapSize, 0, 0, 
                m_InputCubeMapFormat, D3DPOOL_MANAGED ),
            L"Fatal Error: cannot create cubemap of size %d of format %s", m_InputCubeMapSize,
                DXUTD3DFormatToString(m_InputCubeMapFormat, false) );

        m_pInputCubeMap.FillCubeTexture(faceColors);
    }
    else
    {
        //else load cubemap from disk
        if(FAILED(m_pInputCubeMap.LoadCubeTexture(m_InputCubeMapFilename)))
        {
            m_bErrorOccurred = TRUE;
            OutputMessage(L"Error: Cannot Load Texture %s", m_InputCubeMapFilename);
            wcscpy_s( m_InputCubeMapFilename, CG_MAX_FILENAME_LENGTH, L"" );
            return(LoadInputCubeMap());
        }
    }

    m_InputCubeMapSize = m_pInputCubeMap.m_Width;
    m_InputCubeMapFormat = m_pInputCubeMap.m_Format;

    //use cube map processor to flip faces surfaces horizonally on import (if needed)
    /*
    if(m_bFlipFaceSurfacesOnImport)
    {
        CCubeMapProcessor cmProc;

        //generate full mip chain and 4 color channels 
        cmProc.Init(m_InputCubeMapSize, m_InputCubeMapSize, 0, 4);

        //load cube map processor with input cube map
        SetCubeMapProcessorInputCubeMap(&cmProc);

        //flip faces
        cmProc.FlipInputCubemapFaces();

        //retrieve cube map from cube map processor
        GetCubeMapProcessorInputCubeMap(&cmProc);

        cmProc.Clear();
    }
    */

    //invalidate current output cubemap
    m_bValidOutputCubemap = FALSE;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// LoadSelectedCubeMapFace
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::LoadSelectedCubeMapFace(WCHAR *aCubeFaceFilename)
{
    LPDIRECT3DSURFACE9 pD3DSrcSurface, pD3DDstSurface;
    CTexture tempTex;

    if(m_SelectedCubeFace == CG_CUBE_SELECT_NONE)
    {//no cube face selected, do nothing
        return S_OK;
    }

    //attempt to load image
    if(FAILED(tempTex.LoadTexture(aCubeFaceFilename)))
    {
        m_bErrorOccurred = TRUE;
        OutputMessage(L"Error: Cannot Load Image %s", aCubeFaceFilename);
        return E_FAIL;
    }

    //V_RETURN( tempTex.LoadTexture(aCubeFaceFilename) );
    if( tempTex.m_Width != tempTex.m_Height )
    {
        m_bErrorOccurred = TRUE;
        //texture width and height not equal, cant load into cube face
        OutputMessage(L"Error: Cubemap face must be square and power of 2 in size, but image contained in %s is %dx%d", 
            aCubeFaceFilename, tempTex.m_Width, tempTex.m_Height);
        return E_FAIL;
    }

    D3DFORMAT derivedFormat;
    int32 derivedSize;

    derivedFormat = tempTex.m_Format;
    derivedSize = tempTex.m_Width;

    // make sure the face is the same dimension as the current cubemap
    if ( derivedSize != m_InputCubeMapSize || derivedFormat != m_InputCubeMapFormat )
    {
       
       if ( !OutputQuestion( L"The cube map face being loaded has dimensions %dx%d with format %s,\n but the current input cubemap is %dx%d and format %s.\n\n Would you like to switch the size and format to that of the new texture?",
                      tempTex.m_Width, tempTex.m_Height, DXUTD3DFormatToString( tempTex.m_Format, false ), m_InputCubeMapSize, m_InputCubeMapSize, DXUTD3DFormatToString( m_InputCubeMapFormat, false ) ) )
       {
          derivedSize = m_pInputCubeMap.m_Width;
          derivedFormat = m_InputCubeMapFormat;
       }
    }

    //if input cubemap is not same width or format, re-create input cubemap of same format
    if((m_pInputCubeMap.m_Width != derivedSize) || (m_pInputCubeMap.m_Format != derivedFormat) )
    {

        //Create cube texture: automatically releases old texture if successful
        if(FAILED(m_pInputCubeMap.CreateCubeTexture(derivedSize, 0, 0, derivedFormat, D3DPOOL_MANAGED )))
        {
            m_bErrorOccurred = TRUE;
            OutputMessage(L"Error: Cannot create cubemap of size %d and of format %s", derivedSize,
                DXUTD3DFormatToString(derivedFormat, false) );
            return E_FAIL;
        }

        m_InputCubeMapSize = m_pInputCubeMap.m_Width;
        m_InputCubeMapFormat = m_pInputCubeMap.m_Format;
    }

    //copy image to cube map surface
    m_pInputCubeMap.m_pCubeTexture->GetCubeMapSurface( (D3DCUBEMAP_FACES) m_SelectedCubeFace, 0, &pD3DDstSurface);
    tempTex.m_pTexture->GetSurfaceLevel(0, &pD3DSrcSurface);

    //copy one surface to the next
    HRESULT hRes = D3DXLoadSurfaceFromSurface(pD3DDstSurface, NULL, NULL, pD3DSrcSurface, NULL, NULL, D3DX_FILTER_NONE, 0);

    //release old surfaces
    SAFE_RELEASE(pD3DSrcSurface);
    SAFE_RELEASE(pD3DDstSurface);

    if ( hRes == D3D_OK )
    {
       //generate mip sublevels
       D3DXFilterTexture(m_pInputCubeMap.m_pCubeTexture, NULL, 0, D3DTEXF_LINEAR);

       //invalidate current output cubemap
       m_bValidOutputCubemap = FALSE;

       return S_OK;
    }
    else
    {
       return hRes;
    }
}


//--------------------------------------------------------------------------------------
// load cubemap cross in HDR shop cube cross format
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::LoadInputCubeCross(WCHAR *aFilename)
{
    uint32 iCubeFace;
    LPDIRECT3DSURFACE9 pD3DSrcSurface, pD3DDstSurface;
    int32 cubeMapSize;
    D3DXIMAGE_INFO imgInfo;
    CImageSurface cImg; //image surface proxy used to copy face image data out of tempTex into the cube faces
                        // also used to flip Z- face vertically and horizonally

    //u, v offsets of each face image in the cross, in units of faces (cross is layed out in a 3x4 grid)
    //      ___
    //     |Y+ |
    //  ___|___|___
    // |X- |Z+ |X+ |
    // |___|___|___|
    //     |Y- |
    //     |___|
    //     |Z- |
    //     |___|
    //
    //  note Z- face needs to be flipped vertically and horizonally to import into D3D
    int32 crossOffsets[6][2] = {
        {2, 1},  //X+ face
        {0, 1},  //X- face
        {1, 0},  //Y+ face
        {1, 2},  //Y- face
        {1, 1},  //Z+ face
        {1, 3},  //Z- face
        };

    //load cross into temporary texture
    if( FAILED( D3DXGetImageInfoFromFile(aFilename, &imgInfo) ) )
    {
        m_bErrorOccurred = TRUE;

        OutputMessage(L"Error: cannot retrieve image information from file %s. " //no comma, string continues on next line
            L"File is either invalid, non-existant, or of a nonsupported format",
            aFilename);
        return E_FAIL;                
    }

    //make sure cross image is 3:4 ratio
    if( (imgInfo.Width / 3) != (imgInfo.Height / 4) )
    {
        m_bErrorOccurred = TRUE;

        //texture width/height ratio must be 3/4 for cube map cross
        OutputMessage(L"Error: texture width:height ratio must be 3:4 for cube map cross." //no comma, string continues on next line
            L" Image Contained in %s is %dx%d.", aFilename, imgInfo.Width, imgInfo.Height );

        return E_FAIL;
    }


    //create temp surface to hold image to read other surfaces off of
    if(FAILED( m_pDevice->CreateOffscreenPlainSurface(imgInfo.Width, imgInfo.Height,
        imgInfo.Format, D3DPOOL_SCRATCH, &pD3DSrcSurface, NULL)))
    {
        m_bErrorOccurred = TRUE;

        OutputMessage(L"Error: Cannot create offscreen, system memory buffer of size %dx%d and format %s", 
            imgInfo.Width, imgInfo.Height,  DXUTD3DFormatToString(imgInfo.Format, false) );

        return E_FAIL;                    
    }


    if(FAILED( D3DXLoadSurfaceFromFile(pD3DSrcSurface, NULL, NULL, aFilename, NULL, D3DX_FILTER_NONE, 0, &imgInfo) ) )
    {
        m_bErrorOccurred = TRUE;

        OutputMessage(L"Error: cannot load image file %s. " //no comma, string continues on next line
            L"File is either invalid, non-existant, or of an unsupported format",
            aFilename);

        SAFE_RELEASE(pD3DSrcSurface);
        return E_FAIL;                
    }

    cubeMapSize = imgInfo.Width / 3;

    //Create input new cube texture (automatically releases old texture)
    if(FAILED(m_pInputCubeMap.CreateCubeTexture(cubeMapSize, 0, 0, imgInfo.Format, D3DPOOL_MANAGED )))
    {
        m_bErrorOccurred = TRUE;

        OutputMessage(L"Error: Cannot create input cubemap of size %d and format %s", 
            cubeMapSize, DXUTD3DFormatToString(imgInfo.Format, false) );        

        SAFE_RELEASE(pD3DSrcSurface);
        return E_FAIL;                
    }

    m_InputCubeMapFormat = m_pInputCubeMap.m_Format;
    m_InputCubeMapSize = cubeMapSize;

    //Create CImageSurface to copy image data through
    cImg.Init(cubeMapSize, cubeMapSize, 4);

    //iterate over cube faces
    for(iCubeFace=0; iCubeFace<6; iCubeFace++)
    {
        RECT faceRect;

        faceRect.left = crossOffsets[iCubeFace][0] * cubeMapSize;
        faceRect.right = (crossOffsets[iCubeFace][0] + 1) * cubeMapSize;
        faceRect.top = crossOffsets[iCubeFace][1] * cubeMapSize;
        faceRect.bottom = (crossOffsets[iCubeFace][1] + 1) * cubeMapSize;

        //copy image data from surface into CSurfaceImage
        InitCSurfaceImageUsingD3DSurface(&cImg, 4, pD3DSrcSurface, &faceRect);

        //flip Z- Cube Face
        if( (D3DCUBEMAP_FACES)iCubeFace == D3DCUBEMAP_FACE_NEGATIVE_Z)
        {
            cImg.InPlaceHorizonalFlip();
            cImg.InPlaceVerticalFlip();
        }

        //get cube map surface
        m_pInputCubeMap.m_pCubeTexture->GetCubeMapSurface((D3DCUBEMAP_FACES)iCubeFace, 0, &pD3DDstSurface);

        //copy image data from CSurfaceImage into cube face surface
        CopyCSurfaceImageToD3DSurface(&cImg, pD3DDstSurface );
        

        //release cube face surface
        SAFE_RELEASE(pD3DDstSurface);
    }
    
    SAFE_RELEASE(pD3DSrcSurface);

    //generate mip sublevels
    D3DXFilterTexture(m_pInputCubeMap.m_pCubeTexture, NULL, 0, D3DTEXF_LINEAR);

    //invalidate current output cubemap
    m_bValidOutputCubemap = FALSE;

    return S_OK;
}


/*
//--------------------------------------------------------------------------------------
//Loads Input Cube map as a series of textures for the app
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::LoadInputCubeMapFromFiles (WCHAR m_InputCubeMapFilenamePrefix(void)
{
    HRESULT hr;

    WCHAR m_InputCubeMapFilenamePrefix[CG_MAX_FILENAME_LENGTH];
    D3DXIMAGE_FILEFORMAT m_InputCubeMapFilenameFileType;

    WCHAR m_InputCubeMapFilenamePrefix[CG_MAX_FILENAME_LENGTH];
    D3DXIMAGE_FILEFORMAT m_InputCubeMapFilenameFileType;

    //load texture
    //V_RETURN(m_pInputCubeMap.LoadCubeTexture(m_InputCubeMapFilename));

    m_InputCubeMapSize = m_pInputCubeMap.m_Width;
    m_InputCubeMapFormat = m_pInputCubeMap.m_Format;

    return S_OK;
}
*/


//--------------------------------------------------------------------------------------
//save currently displayed cubemap to disk .DDS (includes cube map and all mip chains)
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::SaveOutputCubeMap(void)
{

   //if output cubemap is not invalid prompt user to see if he/she/it wants to copy the 
   // input cubemap and save it instead
   if( HandleInvalidOutputCubemapBeforeSaving() == FALSE)
   {
      return S_OK; //no save, but thats what the user prompted to do
   }

   //if no extension, add .dds to file name
   if(wcsrchr(m_OutputCubeMapFilename, L'.') == NULL)
   {
      wcscat_s( m_OutputCubeMapFilename, CG_MAX_FILENAME_LENGTH, L".dds" );
   }

   if(m_bExportMipChain == TRUE)
   {
      //re-retreive cube map from cube map processor
      GetCubeMapProcessorOutputCubeMap(&m_CubeMapProcessor);

      if(FAILED(D3DXSaveTextureToFile(m_OutputCubeMapFilename,
               D3DXIFF_DDS, // D3DXIMAGE_FILEFORMAT
               m_pOutputCubeMap.m_pCubeTexture,
               NULL)))
      {
         m_bErrorOccurred = TRUE;

         OutputMessage(L"Error: cannot save cubemap file %s. "     //no comma, string continues on next line
               L" Please check free diskspace, write permission, and validity of the filename.",
               m_OutputCubeMapFilename);

         return E_FAIL;                        
      }
   }
   else
   {  //reallocate output cube map without mip chain
      CreateOutputCubeMap(1);

      //re-retreive cube map from cube map processor
      GetCubeMapProcessorOutputCubeMap(&m_CubeMapProcessor);

      if(FAILED(D3DXSaveTextureToFile(m_OutputCubeMapFilename,
               D3DXIFF_DDS, // D3DXIMAGE_FILEFORMAT
               m_pOutputCubeMap.m_pCubeTexture,
               NULL)))
      {
         m_bErrorOccurred = TRUE;

         OutputMessage(L"Error: cannot save cubemap file %s. "     //no comma, string continues on next line
               L" Please check free diskspace, write permission, and validity of the filename.",
               m_OutputCubeMapFilename);

         return E_FAIL;                        
      }
   
      //reallocate output cube map with full mipchain
      CreateOutputCubeMap(0);

      //re-retreive cube map from cube map processor
      GetCubeMapProcessorOutputCubeMap(&m_CubeMapProcessor);
   }


   return S_OK;
}


//--------------------------------------------------------------------------------------
//save output cubemap surfaces to disk in any D3D compatible formats
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::SaveOutputCubeMapToFiles(WCHAR *aFilenamePrefix, D3DXIMAGE_FILEFORMAT aDestFormat)
{
   IDirect3DSurface9 *pCubeMapSurface;
   D3DSURFACE_DESC surfDesc;
   uint32 i, j;
   uint32 lowestMipLevelToSave;     //lowest miplevel to save
   WCHAR aFullFilename[4096];
   WCHAR *fileExtension[]={
      {L".bmp"},
      {L".jpg"},
      {L".tga"},
      {L".png"},
      {L".dds"},
      {L".ppm"},
      {L".dib"},
      {L".hdr"},
      {L".pfm"} };

   HRESULT hr;

   //if output cubemap is not invalid prompt user to see if he/she/it wants to copy the 
   // input cubemap and save it instead
   if( HandleInvalidOutputCubemapBeforeSaving() == FALSE)
   {
      return S_OK; //no save, but thats what the user prompted to do
   }

   //peel off extension
   WCHAR *extStart = wcsrchr(aFilenamePrefix, L'.');
   if(extStart != NULL)
   {
      //if portion of string after extension is 4 characters long, then remove it
      // otherwise.. keep it
      if( wcslen(extStart) == 4 )
      {
         *extStart = L'\0';
      }
   }

   //given export face layout, manipulate cube map faces to be exported directly from surfaces
   switch(m_ExportFaceLayout)
   {
      case CG_EXPORT_FACE_LAYOUT_D3D:
          //no swap needed
      break;
      case CG_EXPORT_FACE_LAYOUT_SUSHI:
      case CG_EXPORT_FACE_LAYOUT_OPENGL:
          
          //flip output faces
          m_CubeMapProcessor.FlipOutputCubemapFaces();
      break;
      default:
      break;
   }

   //re-retreive cube map from cube map processor
   GetCubeMapProcessorOutputCubeMap(&m_CubeMapProcessor);

   //whether or not to export the mipchain
   if( m_bExportMipChain == TRUE )
   {
      lowestMipLevelToSave = m_pOutputCubeMap.m_pCubeTexture->GetLevelCount() - 1;
   }
   else
   {
      lowestMipLevelToSave = 0;
   }


   //save filtered cube faces out to individual image files for all miplevels required
   for(j=0; j<=lowestMipLevelToSave; j++)
   {
      for(i=0; i<6; i++)
      {
         m_pOutputCubeMap.m_pCubeTexture->GetCubeMapSurface((D3DCUBEMAP_FACES)i, j, &pCubeMapSurface);

         //add _m?? to filename to specify miplevel if exporting entire mip chain 
         if( m_bExportMipChain == TRUE )
         {
             swprintf_s( aFullFilename, CG_MAX_FILENAME_LENGTH, L"%s_m%02d_c%02d%s", aFilenamePrefix, j, i, fileExtension[(int32)aDestFormat] );
         }
         else
         {
             swprintf_s( aFullFilename, CG_MAX_FILENAME_LENGTH, L"%s_c%02d%s", aFilenamePrefix, i, fileExtension[(int32)aDestFormat] );            
         }

         pCubeMapSurface->GetDesc(&surfDesc);

         // (the cast to uint32 allows comparison of enumerated types)
#ifdef CG_HDR_FILE_SUPPORT
         if( ((uint32)aDestFormat == (uint32)D3DXIFF_HDR) && (surfDesc.Width <=4) )
         {
             //workaround for bug in D3DX libraries in which .hdr files 4 pixels or smaller in width get written 
             // out incorrectly 

             CImageSurface surfImg;

             //HDR writer requires 3 channel CSurfaceImage
             InitCSurfaceImageUsingD3DSurface(&surfImg, 3, pCubeMapSurface, NULL);

             //write out .hdr file
             surfImg.WriteHDRFile(aFullFilename);

             //clear surface image
             surfImg.Clear();
         }
         else 
#endif
         if( ((uint32)aDestFormat == (uint32)D3DXIFF_BMP) && (surfDesc.Format ==  D3DFMT_X8R8G8B8) )
         { 
            //workaround for issue in D3DX libraries in which .bmp files get written as a 32-bit BMP 
            // even for D3DFMT_X8R8G8B8 Instead, we would like to write out a 24-bit BMP
            // ( convert to 24-bit RGB internally, then write out the 24-bit file)
            IDirect3DSurface9 *pD3DDstSurface;

            //create temp R8G8B8 surface to hold output images
            if(FAILED( m_pDevice->CreateOffscreenPlainSurface(surfDesc.Width, surfDesc.Height,
               D3DFMT_R8G8B8, D3DPOOL_SCRATCH, &pD3DDstSurface, NULL)))
            {
               m_bErrorOccurred = TRUE;
               OutputMessage(L"Error: Cannot create offscreen, system memory buffer of size %dx%d and format %s", 
                  surfDesc.Width, surfDesc.Height, DXUTD3DFormatToString(D3DFMT_R8G8B8, false) );

               return E_FAIL;                    
            }

            //copy face into system memory R8G8B8 surface
            V( D3DXLoadSurfaceFromSurface(                
               pD3DDstSurface,
               NULL,       //no palette
               NULL,       //use full surface as dst rect
               pCubeMapSurface,
               NULL,       //no palette
               NULL,       //use full surface src rect
               D3DX_FILTER_POINT,
               NULL        //no color key
               ) );

            //save out R8G8B8 surface as 24-bit bmp
            if(FAILED(D3DXSaveSurfaceToFile(aFullFilename, aDestFormat, pD3DDstSurface, NULL, NULL )))
            {
               m_bErrorOccurred = TRUE;

               OutputMessage(L"Error: cannot save cubemap image file %s. " //no comma, string continues on next line
                  L" Please check free diskspace, write permissions, and validity of the filename.",
                  aFullFilename);
               return E_FAIL;
            }

            //release created surface
            SAFE_RELEASE(pD3DDstSurface);     
         }
         else
         {
             if(FAILED(D3DXSaveSurfaceToFile(aFullFilename, aDestFormat, pCubeMapSurface, NULL, NULL )))
             {
                 m_bErrorOccurred = TRUE;
                 OutputMessage(L"Error: cannot save cubemap image file %s. " //no comma, string continues on next line
                     L" Please check free diskspace, write permissions, and validity of the filename.",
                     aFullFilename);
                 return E_FAIL;
             }
         }

         SAFE_RELEASE(pCubeMapSurface);
      }
   }


   //given export face layout, re-flip faces to normal orientation
   switch(m_ExportFaceLayout)
   {
      case CG_EXPORT_FACE_LAYOUT_D3D:
          //no swap needed
      break;
      case CG_EXPORT_FACE_LAYOUT_SUSHI:
      case CG_EXPORT_FACE_LAYOUT_OPENGL:
          //save old cube face
          
          //flip output faces
          m_CubeMapProcessor.FlipOutputCubemapFaces();
      break;
      default:
      break;
   }

   //re-retreive cube map from cube map processor
   GetCubeMapProcessorOutputCubeMap(&m_CubeMapProcessor);

   return S_OK;
}


//--------------------------------------------------------------------------------------
//save each miplevel of the output cube map to HDRShop layout cube crosses
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::SaveOutputCubeMapToCrosses(WCHAR *aFilenamePrefix, D3DXIMAGE_FILEFORMAT aDestFormat)
{
   HRESULT hr;
   uint32 iCubeFace, j;
   LPDIRECT3DSURFACE9 pD3DSrcSurface, pD3DDstSurface;
   //Output width, height, and format
   D3DSURFACE_DESC surfDesc;
   int32 crossImageWidth;
   int32 crossImageHeight;
   uint32 lowestMipLevelToSave;     //lowest miplevel to save
   CImageSurface cImg;             //image surface proxy used to copy face image data out of tempTex into 
                                   // the cube faces also used to flip Z- face vertically and horizonally

   WCHAR aFullFilename[4096];
   WCHAR *fileExtension[]={
       {L".bmp"},
       {L".jpg"},
       {L".tga"},
       {L".png"},
       {L".dds"},
       {L".ppm"},
       {L".dib"},
       {L".hdr"},
       {L".pfm"} };

   //if output cubemap is not invalid prompt user to see if he/she/it wants to copy the 
   // input cubemap and save it instead
   if( HandleInvalidOutputCubemapBeforeSaving() == FALSE)
   {
      return S_OK; //no save, but thats what the user prompted to do
   }

   //u, v offsets of each face image in the cross, in units of faces (cross is layed out in a 3x4 grid)
   //      ___
   //     |Y+ |
   //  ___|___|___
   // |X- |Z+ |X+ |
   // |___|___|___|
   //     |Y- |
   //     |___|
   //     |Z- |
   //     |___|
   //
   //  note Z- face needs to be flipped vertically and horizonally from D3D cube face layout to cubecross layout
   int32 crossOffsets[6][2] = {
       {2, 1},  //X+ face
       {0, 1},  //X- face
       {1, 0},  //Y+ face
       {1, 2},  //Y- face
       {1, 1},  //Z+ face
       {1, 3},  //Z- face
       };

   //peel off extension
   WCHAR *extStart = wcsrchr(aFilenamePrefix, L'.');
   if(extStart != NULL)
   {
      //if portion of string after extension is 4 characters long, then remove it
      // otherwise.. keep it
      if( wcslen(extStart) == 4 )
      {
         *extStart = L'\0';
      }
   }


   //flip Z-NEG face for cube cross export in cubemap processor
   for(j=0; j<m_pOutputCubeMap.m_pCubeTexture->GetLevelCount(); j++)
   {
       m_CubeMapProcessor.m_OutputSurface[j][CP_FACE_Z_NEG].InPlaceHorizonalFlip();
       m_CubeMapProcessor.m_OutputSurface[j][CP_FACE_Z_NEG].InPlaceVerticalFlip();
   }

   //retreive cube map from cube map processor
   GetCubeMapProcessorOutputCubeMap(&m_CubeMapProcessor);

   //whether or not to export the mipchain
   if( m_bExportMipChain == TRUE )
   {
       lowestMipLevelToSave = m_pOutputCubeMap.m_pCubeTexture->GetLevelCount() - 1;
   }
   else
   {
       lowestMipLevelToSave = 0;
   }

   //read filtered cube levels back out
   for(j=0; j<=lowestMipLevelToSave; j++)
   {
       m_pOutputCubeMap.m_pCubeTexture->GetLevelDesc(j, &surfDesc);

       crossImageWidth = 3 * surfDesc.Width;
       crossImageHeight = 4 * surfDesc.Height;

       //create temp surface to hold output cube cross
       if(FAILED( m_pDevice->CreateOffscreenPlainSurface(crossImageWidth, crossImageHeight,
           surfDesc.Format, D3DPOOL_SCRATCH, &pD3DDstSurface, NULL)))
       {
           m_bErrorOccurred = TRUE;

           OutputMessage(L"Error: Cannot create offscreen, system memory buffer of size %dx%d and format %s", 
               crossImageWidth, crossImageHeight, DXUTD3DFormatToString(surfDesc.Format, false) );

           return E_FAIL;                    
       }

       //copy cube faces into cubecross surface
       for(iCubeFace=0; iCubeFace<6; iCubeFace++)
       {
           //grab cubeface surface
           m_pOutputCubeMap.m_pCubeTexture->GetCubeMapSurface((D3DCUBEMAP_FACES)iCubeFace, j, &pD3DSrcSurface);
         
           //compute region in destination cross image to copy cube face into
           RECT dstRect;

           dstRect.left   = crossOffsets[iCubeFace][0] * surfDesc.Width;
           dstRect.right  = (crossOffsets[iCubeFace][0] + 1) * surfDesc.Width;
           dstRect.top    = crossOffsets[iCubeFace][1] * surfDesc.Height;
           dstRect.bottom = (crossOffsets[iCubeFace][1] + 1) * surfDesc.Height;

           //copy face into cube cross
           V( D3DXLoadSurfaceFromSurface(                
               pD3DDstSurface,
               NULL,       //no palette
               &dstRect,   //
               pD3DSrcSurface,
               NULL,       //no palette
               NULL,       //use full surface src rect
               D3DX_FILTER_POINT,
               NULL        //no color key
               ) );

           SAFE_RELEASE(pD3DSrcSurface);
       }

       //build filename from prefix and file-extension
       if( m_bExportMipChain == TRUE )
       {   //add _m?? after prefix to filename to specify miplevel for each cross saved
           swprintf_s(aFullFilename, CG_MAX_FILENAME_LENGTH, L"%s_m%02d%s", aFilenamePrefix, j, fileExtension[(int32)aDestFormat] );
       }
       else
       {   //save out filename with solely
           swprintf_s(aFullFilename, CG_MAX_FILENAME_LENGTH, L"%s%s", aFilenamePrefix, fileExtension[(int32)aDestFormat] );        
       }

#ifdef CG_HDR_FILE_SUPPORT
	   //workaround for bug in D3DX libraries in which .hdr files 4 pixels or smaller in width get written 
       // out incorrectly 
       //   (casting to uint32 allows comparison of enumerated type variables) 
       if( ((uint32)aDestFormat == (uint32)D3DXIFF_HDR) && (surfDesc.Width <=4) )
       {
           CImageSurface surfImg;

           //HDR writer requires 3 channel CSurfaceImage
           InitCSurfaceImageUsingD3DSurface(&surfImg, 3, pD3DDstSurface, NULL);

           //write out .hdr file
           surfImg.WriteHDRFile(aFullFilename);

           //clear surface image
           surfImg.Clear();
       }
       else
#endif
	   {
           if(FAILED(D3DXSaveSurfaceToFile(aFullFilename, aDestFormat, pD3DDstSurface, NULL, NULL )))
           {
               m_bErrorOccurred = TRUE;

               OutputMessage(L"Error: cannot save cubemap cross image file %s." //no comma, string continues on next line
                   L"Please check free diskspace, write permissions, and validity of the filename.",
                   aFullFilename);

               return E_FAIL;
           }
       }

       //release temp surface used to assemble and store the cube cross.
       SAFE_RELEASE(pD3DDstSurface);
   }
   
   SAFE_RELEASE(pD3DSrcSurface);

   //re-flip Z-NEG face to return cubemap in cubemap processor to normal D3D layout
   for(j=0; j<m_pOutputCubeMap.m_pCubeTexture->GetLevelCount(); j++)
   {
       m_CubeMapProcessor.m_OutputSurface[j][CP_FACE_Z_NEG].InPlaceHorizonalFlip();
       m_CubeMapProcessor.m_OutputSurface[j][CP_FACE_Z_NEG].InPlaceVerticalFlip();
   }

   //retreive output cube map from cube map processor
   GetCubeMapProcessorOutputCubeMap(&m_CubeMapProcessor);

   return S_OK;
}


//--------------------------------------------------------------------------------------
// Creates the output cube map texture
//   //using zero as teh number of miplevels means that a full mip chain is created
//--------------------------------------------------------------------------------------
HRESULT CCubeGenApp::CreateOutputCubeMap(int32 a_NumMipLevels)
{
    //note create calls automatically destroy the old texture stored in the CTexture class
    if(FAILED(m_pOutputCubeMap.CreateCubeTexture(m_OutputCubeMapSize, a_NumMipLevels, 
        0, m_OutputCubeMapFormat, D3DPOOL_MANAGED )))
    {
        m_bErrorOccurred = TRUE;

        OutputMessage(L"Error: Cannot create output cubemap of size %d and format %s.", 
            m_OutputCubeMapSize, DXUTD3DFormatToString(m_OutputCubeMapFormat, false) );        

        return E_FAIL;                
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------------------
void CCubeGenApp::SetOutputCubeMapSize(int32 a_Size)
{
   uint32 oldSize;

   oldSize = m_OutputCubeMapSize;
   m_OutputCubeMapSize = a_Size;

   if(FAILED(CreateOutputCubeMap(0)))
   {
      m_OutputCubeMapSize = oldSize;
      CreateOutputCubeMap(0);
   }

   m_CubeMapProcessor.Clear();

   //invalidate current output cubemap
   m_bValidOutputCubemap = FALSE;
}


//--------------------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------------------
void CCubeGenApp::SetOutputCubeMapTexelFormat(D3DFORMAT a_Format)
{   
   D3DFORMAT oldOutputCubeFormat; 
   
   oldOutputCubeFormat = m_OutputCubeMapFormat;   
   m_OutputCubeMapFormat = a_Format;

   //if new format is not supported, return to old format
   if(FAILED(CreateOutputCubeMap(0)))
   {
      m_OutputCubeMapFormat = oldOutputCubeFormat;
      CreateOutputCubeMap(0);
   }

   GetCubeMapProcessorOutputCubeMap(&m_CubeMapProcessor);
}


//------------------------------------------------------------------------------------------
// refresh output cube map if cubemap in cubemap processor is compatibe with output cube map
//------------------------------------------------------------------------------------------
void CCubeGenApp::RefreshOutputCubeMap(void)
{
    if(m_CubeMapProcessor.m_OutputSize == m_OutputCubeMapSize)
    {
        GetCubeMapProcessorOutputCubeMap(&m_CubeMapProcessor);
    }
}


//--------------------------------------------------------------------------------------
//Rendering technique used
//--------------------------------------------------------------------------------------
void CCubeGenApp::SetRenderTechnique(int32 a_RenderTechnique)
{
    m_RenderTechnique = a_RenderTechnique;
}


//--------------------------------------------------------------------------------------
//Load objects
//--------------------------------------------------------------------------------------
void CCubeGenApp::LoadObjects(void)
{
    //setup geometry, note subsequent Generate/Load calls release any previously existing meshes
    m_GeomCube.Init(m_pDevice);
    m_GeomCube.GenerateCube();

    m_GeomQuad.Init(m_pDevice);
    m_GeomQuad.GenerateQuad();

    //sky sphere
    m_GeomSkySphere.Init(m_pDevice);
    m_GeomSkySphere.GenerateSphere(m_pDevice, 5000, 30, 30);


    if(wcscmp(m_SceneFilename, L"") == 0 )
    {   //generate sphere if filename is empty string        
        m_GeomScene.Init(m_pDevice);
        m_GeomScene.GenerateSphere(m_pDevice, 10, 100, 100);
    }
    else  
    {   //load scene from file
        m_GeomScene.Init(m_pDevice);
        m_GeomScene.LoadMesh(m_pDevice, m_SceneFilename);
    }
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void CCubeGenApp::SetEyeFrustumProj(D3DXMATRIX *aProjMX)
{
    m_EyeFrustum.SetProjMatrix(aProjMX);
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void CCubeGenApp::SetEyeFrustumView(D3DXMATRIX *aViewMX)
{
    m_EyeFrustum.SetViewMatrix(aViewMX);
}


//--------------------------------------------------------------------------------------
//set world matrix for object
//--------------------------------------------------------------------------------------
void CCubeGenApp::SetObjectWorldMatrix(D3DXMATRIX *aWorldMX)
{
    m_ObjectWorldMatrix = *aWorldMX;
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void CCubeGenApp::UpdateEyeEffectParams(void)
{
   HRESULT hr;
   D3DXMATRIX worldMX, viewMX, projMX;

   worldMX = m_ObjectWorldMatrix;
   viewMX =  *m_EyeFrustum.GetViewMatrix();
   projMX =  *m_EyeFrustum.GetProjMatrix();

   //multiply by bbox
   if( m_bCenterObject == TRUE)
   {
      D3DXMATRIX bboxXform;  //matrix to center and scale model by bbox
      D3DXVECTOR3 bboxSize;
      float32 scaleFactor;

      bboxSize = m_GeomScene.m_BBoxMax - m_GeomScene.m_BBoxMin;

      //inverse of largest axis of bbox
      scaleFactor = VM_MAX( bboxSize.x, bboxSize.y); 
      scaleFactor = VM_MAX( scaleFactor, bboxSize.z); 
      scaleFactor = 1.0 / scaleFactor;

      //build matrix to center and scale model for display based on bbox
      D3DXMatrixIdentity(&bboxXform);
      bboxXform._11 = 20 * scaleFactor;
      bboxXform._22 = 20 * scaleFactor;
      bboxXform._33 = 20 * scaleFactor;

      bboxXform._41 = -m_GeomScene.m_CenterPoint.x * (20 * scaleFactor);
      bboxXform._42 = -m_GeomScene.m_CenterPoint.y * (20 * scaleFactor);
      bboxXform._43 = -m_GeomScene.m_CenterPoint.z * (20 * scaleFactor);

      worldMX = bboxXform * worldMX;
   }

   m_EyeSceneVP = viewMX * projMX;
   m_EyeSceneWVP = worldMX * viewMX * projMX;

   V( m_pEffect.m_pEffect->SetMatrix("g_mWorld",    &worldMX ) );
   V( m_pEffect.m_pEffect->SetMatrix("g_mViewProj", &m_EyeSceneVP ) );
   V( m_pEffect.m_pEffect->SetMatrix("g_mWorldViewProj", &m_EyeSceneWVP ) ); 
   V( m_pEffect.m_pEffect->SetVector("g_vWorldCamPos", &(m_EyeFrustum.m_FrustumApex) ) );
}


//--------------------------------------------------------------------------------------
//draw 
//--------------------------------------------------------------------------------------
void CCubeGenApp::Draw(void)
{
    //
    UpdateEyeEffectParams();

    DrawScene(m_RenderTechnique);
}


//--------------------------------------------------------------------------------------
// setup associated textures and draw scene geometry
//--------------------------------------------------------------------------------------
void CCubeGenApp::DrawScene(int32 a_RenderTechnique)
{
    m_FramesSinceLastRefresh++;

   //if cubemap processor is currently processing, then have main application thread sleep to 
   // give filtering process more time
   if(m_CubeMapProcessor.GetStatus() == CP_STATUS_PROCESSING)
   {
      Sleep(100);  //sleep for 100 milliseconds (max framerate is 10fps now..)     

      if(m_bOutputPeriodicRefresh &&
         (m_FramesSinceLastRefresh > CG_PROCESSING_FRAME_REFRESH_INTERVAL )
         )
      {  
         RefreshOutputCubeMap();
         m_FramesSinceLastRefresh = 0;
      }

   }
   else if( (m_CubeMapProcessor.GetStatus() == CP_STATUS_FILTER_TERMINATED) || //if processing has been terminated
            (m_CubeMapProcessor.GetStatus() == CP_STATUS_FILTER_COMPLETED) 
            )
   {
      //reead output cubemap from cubemap processor
      RefreshOutputCubeMap();

      //refresh cubemap processor status to READY state
      m_CubeMapProcessor.RefreshStatus();

      //cubemap output is valid now
      m_bValidOutputCubemap = TRUE;
   }
   

   //draw skysphere
   if(m_bDrawSkySphere)
   {
      m_pEffect.m_pEffect->SetTexture("g_tCubeMap", m_pInputCubeMap.m_pCubeTexture);
      m_pEffect.m_pEffect->SetBool("g_bShowAlpha", false);  
      m_pEffect.m_pEffect->SetFloat("g_fMipLevelClamp", 0.0f);    
      m_pEffect.m_pEffect->SetFloat("g_fMipLODBias", 0.0f);    
      m_pEffect.DrawGeom("CubeMapNorm", &m_GeomSkySphere );
   }


   m_pEffect.m_pEffect->SetFloat("g_fScaleFactor", 1.0f);     

   //setup any textures needed
   if(m_DisplayCubeSource == CG_CUBE_DISPLAY_INPUT)
   {
      m_pEffect.m_pEffect->SetTexture("g_tCubeMap", m_pInputCubeMap.m_pCubeTexture);

      m_pEffect.m_pEffect->SetFloat("g_fNumMipLevels", (float32) m_pInputCubeMap.m_NumMipLevels );
   }
   else // (m_DisplayCubeSource == CG_CUBE_DISPLAY_OUTPUT)
   {
      m_pEffect.m_pEffect->SetTexture("g_tCubeMap", m_pOutputCubeMap.m_pCubeTexture);    

      m_pEffect.m_pEffect->SetFloat("g_fNumMipLevels", (float32) m_pOutputCubeMap.m_NumMipLevels );
   }

   //if displying only a single mip level, setup texCUBEBias amounts
   if(m_bMipLevelSelectEnable)
   {
      m_pEffect.m_pEffect->SetFloat("g_fMipLODBias", -16.0f);    
   }
   else
   {
      m_pEffect.m_pEffect->SetFloat("g_fMipLODBias", 0);
   }

   //if displying only a single mip level, or clamping the max mip level, 
   //  setup mip level select/clamp using maxMipLOD settings
   if(m_bMipLevelClampEnable || m_bMipLevelSelectEnable)
   {
      m_pEffect.m_pEffect->SetFloat("g_fMipLevelClamp", (float32)m_MipLevelDisplayed);    
   }
   else
   {
      m_pEffect.m_pEffect->SetFloat("g_fMipLevelClamp", 0);    
   }


   m_pEffect.m_pEffect->SetTexture("g_tBaseMap", m_pBaseMap.m_pTexture);

   //set show alpha option
   m_pEffect.m_pEffect->SetBool("g_bShowAlpha", m_bShowAlpha);    


   //draw
   switch( m_RenderTechnique)
   {
      case CG_RENDER_NORMAL:
         m_pEffect.DrawGeom("BaseCubeMapNorm", &m_GeomScene );
      break;
      case CG_RENDER_REFLECT_PER_VERTEX:
         m_pEffect.DrawGeom("BaseCubeMapReflectPerVertex", &m_GeomScene );
      break;
      case CG_RENDER_REFLECT_PER_PIXEL:
         m_pEffect.DrawGeom("BaseCubeMapReflectPerPixel", &m_GeomScene );
      break;
      case CG_RENDER_MIP_ALPHA_LOD:
         m_pEffect.DrawGeom("BaseCubeMapMipAlphaLOD", &m_GeomScene );
      break;
   }

}


//--------------------------------------------------------------------------------------
//load faces from input cubemap texture into cube map processor
//--------------------------------------------------------------------------------------
void CCubeGenApp::SetCubeMapProcessorInputCubeMap(CCubeMapProcessor *aCMProc)
{
   int32 i;
   D3DLOCKED_RECT lockedRect[6];
   uint32 cImgSurfFormat;

   //invalidate current output cubemap
   m_bValidOutputCubemap = FALSE;

   //read in top cube level into cube map processor
   for(i=0; i<6; i++)
   {
      //params:  a_FaceIdx, a_SrcType, a_SrcNumChannels, a_SrcPitch,  *a_SrcDataPtr

      switch(m_InputCubeMapFormat)
      {
         case D3DFMT_A8R8G8B8:
         case D3DFMT_X8R8G8B8:
            cImgSurfFormat = CP_VAL_UNORM8_BGRA;
         break;
         case D3DFMT_A16B16G16R16:
            cImgSurfFormat = CP_VAL_UNORM16;
         break;
         case D3DFMT_A16B16G16R16F:
            cImgSurfFormat = CP_VAL_FLOAT16;
         break;
         case D3DFMT_A32B32G32R32F:
            cImgSurfFormat = CP_VAL_FLOAT32;
         break;
      }

      //now copy the surface depending on the type
      switch(m_InputCubeMapFormat)
      {
         case D3DFMT_A8R8G8B8:
         case D3DFMT_X8R8G8B8:
         case D3DFMT_A16B16G16R16:
         case D3DFMT_A16B16G16R16F:
         case D3DFMT_A32B32G32R32F:
         {
            //lock rect, copy data then unlock the rect
            m_pInputCubeMap.m_pCubeTexture->LockRect((D3DCUBEMAP_FACES)i, 0, &lockedRect[i], NULL, NULL );
            aCMProc->SetInputFaceData(i, cImgSurfFormat, 4, lockedRect[i].Pitch, lockedRect[i].pBits, 
               m_InputMaxClamp, m_InputDegamma, m_InputScaleFactor);
            m_pInputCubeMap.m_pCubeTexture->UnlockRect((D3DCUBEMAP_FACES)i, 0);
         }
         break;
         default:    //otherwise, use D3D routines to convert surface to type CG_INTERMEDIATE_SURFACE_TYPE and 
         {           // load that into cube map processor
            IDirect3DSurface9 *pD3DSrcSurface = NULL;
            IDirect3DSurface9 *pD3DDstSurface = NULL;
            HRESULT hr;

            m_pInputCubeMap.m_pCubeTexture->GetCubeMapSurface((D3DCUBEMAP_FACES)i, 0, &pD3DSrcSurface);            

            //create temp offscreen surface of type CG_INTERMEDIATE_SURFACE_FORMAT to store converted texture info
            if(FAILED( m_pDevice->CreateOffscreenPlainSurface(
               m_pInputCubeMap.m_Width,   //only loads topmost level into input cubemap, so use full cubemap size
               m_pInputCubeMap.m_Height,
               CG_INTERMEDIATE_SURFACE_FORMAT , 
               D3DPOOL_SCRATCH, 
               &pD3DDstSurface, 
               NULL)))
            {               
               m_bErrorOccurred = TRUE;

               OutputMessage(L"Error: Cannot create offscreen, system memory buffer of size %dx%d and format %s", 
                     m_pInputCubeMap.m_Width, 
                     m_pInputCubeMap.m_Height, 
                     DXUTD3DFormatToString(CG_INTERMEDIATE_SURFACE_FORMAT , false) );

               return;                    
            }

            //copy and convert image data from cubemap surface into temp offscreen surface
            hr = D3DXLoadSurfaceFromSurface(
               pD3DDstSurface,   //LPDIRECT3DSURFACE9 pDestSurface,
               NULL,             //CONST PALETTEENTRY *pDestPalette,
               NULL,             //CONST RECT *pDestRect,
               pD3DSrcSurface,   //LPDIRECT3DSURFACE9 pSrcSurface,
               NULL,             //CONST PALETTEENTRY *pSrcPalette,
               NULL,             //CONST RECT *pSrcRect,
               D3DX_DEFAULT,     //DWORD Filter,
               NULL              //D3DCOLOR ColorKey
            );

            //copy data out of temp surface
            InitCSurfaceImageUsingD3DSurface(&(aCMProc->m_InputSurface[i]), 4, pD3DDstSurface, NULL);

            //cImgSurfFormat = CP_VAL_FLOAT32;  //
            //pD3DDstSurface->LockRect(&lockedRect[i], NULL, 0);
            //aCMProc->SetInputFaceData(i, cImgSurfFormat, 4, lockedRect[i].Pitch, lockedRect[i].pBits, 
            //   m_InputMaxClamp, m_InputDegamma, m_InputScaleFactor);
            //pD3DDstSurface->UnlockRect();

            SAFE_RELEASE(pD3DDstSurface);
            SAFE_RELEASE(pD3DSrcSurface);
         }
         break;
      }
   }



}


//--------------------------------------------------------------------------------------
//retrieve input cubemap texture faces from cube map processor
//--------------------------------------------------------------------------------------
/*
void CCubeGenApp::GetCubeMapProcessorInputCubeMap(CCubeMapProcessor *aCMProc)
{
    int32 i;
    D3DLOCKED_RECT lockedRect[6];
    uint32 cImgSurfFormat;

    //read in top cube level into cube map processor
    for(i=0; i<6; i++)
    {
        m_pInputCubeMap.m_pCubeTexture->LockRect((D3DCUBEMAP_FACES)i, 0, &lockedRect[i], NULL, NULL );
        //params:  a_FaceIdx, a_SrcType, a_SrcNumChannels, a_SrcPitch,  *a_SrcDataPtr

        switch(m_InputCubeMapFormat)
        {
            case D3DFMT_A8R8G8B8:
            case D3DFMT_X8R8G8B8:
                cImgSurfFormat = CP_VAL_UNORM8_BGRA;
            break;
            case D3DFMT_A16B16G16R16:
                cImgSurfFormat = CP_VAL_UNORM16;
            break;
            case D3DFMT_A16B16G16R16F:
                cImgSurfFormat = CP_VAL_FLOAT16;
            break;
            case D3DFMT_A32B32G32R32F:
                cImgSurfFormat = CP_VAL_FLOAT32;
            break;
        }

        //retrieve cube faces undoing scale and gamma effects
        aCMProc->GetInputFaceData(i, cImgSurfFormat, 4, lockedRect[i].Pitch, lockedRect[i].pBits,
            m_InputScaleFactor, (1.0f / m_InputDegamma) );

        m_pInputCubeMap.m_pCubeTexture->UnlockRect((D3DCUBEMAP_FACES)i, 0);
    }
}
*/

//--------------------------------------------------------------------------------------
//retrieve output cubemap texture faces for all mip chains from the cube map processor
//--------------------------------------------------------------------------------------
void CCubeGenApp::GetCubeMapProcessorOutputCubeMap(CCubeMapProcessor *aCMProc)
{
    uint32 i, j;
    D3DLOCKED_RECT lockedRect[6];
    uint32 cImgSurfFormat;

    //set hourglass         
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    //write miplevel into alpha
    if( m_bWriteMipLevelIntoAlpha == TRUE)
    {
       m_CubeMapProcessor.WriteMipLevelIntoAlpha();   
    }

    //read filtered cube levels back out
    for(j=0; j<m_pOutputCubeMap.m_pCubeTexture->GetLevelCount(); j++)
    {
        for(i=0; i<6; i++)
        {
            m_pOutputCubeMap.m_pCubeTexture->LockRect((D3DCUBEMAP_FACES)i, j, &lockedRect[i], NULL, NULL );

            switch(m_OutputCubeMapFormat)
            {
                case D3DFMT_R8G8B8:
                case D3DFMT_A8R8G8B8:
                case D3DFMT_X8R8G8B8:
                    cImgSurfFormat = CP_VAL_UNORM8_BGRA;
                break;
                case D3DFMT_A16B16G16R16:
                    cImgSurfFormat = CP_VAL_UNORM16;
                break;
                case D3DFMT_A16B16G16R16F:
                    cImgSurfFormat = CP_VAL_FLOAT16;
                break;
                case D3DFMT_A32B32G32R32F:
                    cImgSurfFormat = CP_VAL_FLOAT32;
                break;
            }

            //apply scale factor, then gamma, then retrieve image data 
            if(m_OutputCubeMapFormat == D3DFMT_R8G8B8)
            {  //if using a 3 channel format
               m_CubeMapProcessor.GetOutputFaceData(i, j, cImgSurfFormat, 3, lockedRect[i].Pitch, lockedRect[i].pBits,
                  m_OutputScaleFactor, m_OutputGamma);
            }
            else
            {
               m_CubeMapProcessor.GetOutputFaceData(i, j, cImgSurfFormat, 4, lockedRect[i].Pitch, lockedRect[i].pBits,
                  m_OutputScaleFactor, m_OutputGamma);
            }

            m_pOutputCubeMap.m_pCubeTexture->UnlockRect((D3DCUBEMAP_FACES)i, j);
        }    
    }

    //set arrow
    SetCursor(LoadCursor(NULL, IDC_ARROW));

}


//--------------------------------------------------------------------------------------
//flips the selected face about the line u = v
//--------------------------------------------------------------------------------------
void CCubeGenApp::FlipSelectedFaceDiagonal(void)
{
   HRESULT hr;
   int i;

   LPDIRECT3DSURFACE9 pD3DSurface;
   CImageSurface surfaceImage;

   if(m_SelectedCubeFace == CG_CUBE_SELECT_NONE)
   {
      return;  //no cube face selected, do nothing
   }

   for(i=0; i<m_pInputCubeMap.m_NumMipLevels; i++)
   {
      V(m_pInputCubeMap.m_pCubeTexture->GetCubeMapSurface( (D3DCUBEMAP_FACES) m_SelectedCubeFace, i, &pD3DSurface ));    
      InitCSurfaceImageUsingD3DSurface(&surfaceImage, 4, pD3DSurface, NULL);

      surfaceImage.InPlaceDiagonalUVFlip();

      CopyCSurfaceImageToD3DSurface(&surfaceImage, pD3DSurface);
      SAFE_RELEASE(pD3DSurface);
   }
}


//--------------------------------------------------------------------------------------
//flips the selected face vertically
//--------------------------------------------------------------------------------------
void CCubeGenApp::FlipSelectedFaceVertical(void)
{
   HRESULT hr;
   int32 i;

   LPDIRECT3DSURFACE9 pD3DSurface;
   CImageSurface surfaceImage;

   if(m_SelectedCubeFace == CG_CUBE_SELECT_NONE)
   {
      return;
   }

   //invalidate current output cubemap
   m_bValidOutputCubemap = FALSE;

   for(i=0; i<m_pInputCubeMap.m_NumMipLevels; i++)
   {
      V(m_pInputCubeMap.m_pCubeTexture->GetCubeMapSurface( (D3DCUBEMAP_FACES) m_SelectedCubeFace, i, &pD3DSurface ));
      InitCSurfaceImageUsingD3DSurface(&surfaceImage, 4, pD3DSurface, NULL);

      surfaceImage.InPlaceVerticalFlip();

      CopyCSurfaceImageToD3DSurface(&surfaceImage, pD3DSurface);
      SAFE_RELEASE(pD3DSurface);
   }
}


//--------------------------------------------------------------------------------------
//flips the selected face horizontally
//--------------------------------------------------------------------------------------
void CCubeGenApp::FlipSelectedFaceHorizontal(void)
{
   HRESULT hr;
   int32 i;

   LPDIRECT3DSURFACE9 pD3DSurface;
   CImageSurface surfaceImage;

   if(m_SelectedCubeFace == CG_CUBE_SELECT_NONE)
   {
      return;
   }

   //invalidate current output cubemap
   m_bValidOutputCubemap = FALSE;

   for(i=0; i<m_pInputCubeMap.m_NumMipLevels; i++)
   {
      V(m_pInputCubeMap.m_pCubeTexture->GetCubeMapSurface( (D3DCUBEMAP_FACES) m_SelectedCubeFace, i, &pD3DSurface ));
      InitCSurfaceImageUsingD3DSurface(&surfaceImage, 4,  pD3DSurface, NULL);

      surfaceImage.InPlaceHorizonalFlip();

      CopyCSurfaceImageToD3DSurface(&surfaceImage, pD3DSurface);
      SAFE_RELEASE(pD3DSurface);
   }
}


//--------------------------------------------------------------------------------------
//initialize the CSurfaceImage using the D3D Surface, and copy the image data over from 
// the D3D Surface to the CSurfaceImage note that the number of channels can be 
// specified.
//
// the arguement a_Rect allows you to specify a subregion of the surface to be used.
//  setting it to NULL specifies the entire surface
//--------------------------------------------------------------------------------------
void CCubeGenApp::InitCSurfaceImageUsingD3DSurface(CImageSurface *a_pSurfImg, int32 a_NumChannels, 
    LPDIRECT3DSURFACE9 a_pD3DSurface, RECT *a_Rect )
{
   D3DSURFACE_DESC desc;
   D3DLOCKED_RECT  lockedRect;
   uint32 cImgSurfValType;
   IDirect3DSurface9 *newSurface = NULL, *srcSurface = NULL;
   HRESULT hr;

   a_pD3DSurface->GetDesc( &desc );

   //if surface format is unsupported by CSurfaceImage then convert it to A8R8G8B8
   switch( desc.Format )
   {
      case D3DFMT_A8R8G8B8:
      case D3DFMT_X8R8G8B8:
      case D3DFMT_A16B16G16R16:
      case D3DFMT_A16B16G16R16F:
      case D3DFMT_A32B32G32R32F:    
         //format is supported, use surface as is
         srcSurface = a_pD3DSurface;
      break;
      default: //unsupported format, create new temp surface convert to CG_INTERMEDIATE_SURFACE_FORMAT 
         //create temp offscreen surface of type CG_INTERMEDIATE_SURFACE_FORMAT  to store converted texture info
         if(FAILED( m_pDevice->CreateOffscreenPlainSurface(
            desc.Width,  
            desc.Height,
            CG_INTERMEDIATE_SURFACE_FORMAT , 
            D3DPOOL_SCRATCH, 
            &newSurface, 
            NULL)))
         {
            m_bErrorOccurred = TRUE;

            OutputMessage(L"Error: Cannot create offscreen, system memory buffer of size %dx%d and format %s", 
                  desc.Width, 
                  desc.Height, 
                  DXUTD3DFormatToString(CG_INTERMEDIATE_SURFACE_FORMAT , false) );

            return;                    
         }

         //copy and convert image data from surface into temp offscreen surface
         hr = D3DXLoadSurfaceFromSurface(
            newSurface,   //LPDIRECT3DSURFACE9 pDestSurface,
            NULL,             //CONST PALETTEENTRY *pDestPalette,
            NULL,             //CONST RECT *pDestRect,
            a_pD3DSurface,   //LPDIRECT3DSURFACE9 pSrcSurface,
            NULL,             //CONST PALETTEENTRY *pSrcPalette,
            NULL,             //CONST RECT *pSrcRect,
            D3DX_DEFAULT,     //DWORD Filter,
            NULL              //D3DCOLOR ColorKey
         );

         srcSurface = newSurface;
      break;
   }

   srcSurface->GetDesc( &desc );

   a_pSurfImg->Clear();

   if(a_Rect == NULL)
   {  //if using entire surface, use width and height from surface desc
      a_pSurfImg->Init(desc.Width, desc.Height, a_NumChannels );
   }
   else
   {  //otherwise get description from the subrect to be copied
      a_pSurfImg->Init(a_Rect->right - a_Rect->left, a_Rect->bottom - a_Rect->top, a_NumChannels );
   }

   srcSurface->LockRect(&lockedRect, a_Rect, NULL );

   switch( desc.Format )
   {
      case D3DFMT_A8R8G8B8:
      case D3DFMT_X8R8G8B8:
          cImgSurfValType = CP_VAL_UNORM8_BGRA;
      break;
      case D3DFMT_A16B16G16R16:
          cImgSurfValType = CP_VAL_UNORM16;
      break;
      case D3DFMT_A16B16G16R16F:
          cImgSurfValType = CP_VAL_FLOAT16;
      break;
      case D3DFMT_A32B32G32R32F:
          cImgSurfValType = CP_VAL_FLOAT32;
      break;
   }

   a_pSurfImg->SetImageData(cImgSurfValType, 4, lockedRect.Pitch, lockedRect.pBits);

   srcSurface->UnlockRect();

   //release new surface if needed
   SAFE_RELEASE(newSurface);
}


//--------------------------------------------------------------------------------------
// copy data from CSurfaceImage to D3DSurface
//--------------------------------------------------------------------------------------
void CCubeGenApp::CopyCSurfaceImageToD3DSurface(CImageSurface *a_pSurfImg, LPDIRECT3DSURFACE9 a_pD3DSurface )
{
   D3DSURFACE_DESC desc;
   D3DLOCKED_RECT  lockedRect;
   uint32 cImgSurfValType;
   IDirect3DSurface9 *newSurface = NULL, *dstSurface = NULL;
   bool8 bUseIntermediateSurface = FALSE;
   HRESULT hr;

   a_pD3DSurface->GetDesc( &desc );

   //if surface type not supported by CSurfaceImage then create intermediate surface of 
   // type A8R8G8B to convert from
   switch( desc.Format )
   {
      case D3DFMT_A8R8G8B8:
      case D3DFMT_X8R8G8B8:
      case D3DFMT_A16B16G16R16:
      case D3DFMT_A16B16G16R16F:
      case D3DFMT_A32B32G32R32F:    
         //format is supported, use surface as is
         dstSurface = a_pD3DSurface;
      break;
      default: //unsupported format, create new temp surface convert to CG_INTERMEDIATE_SURFACE_FORMAT 
         //create temp offscreen surface of type CG_INTERMEDIATE_SURFACE_FORMAT  to store converted texture info
         if(FAILED( m_pDevice->CreateOffscreenPlainSurface(
            desc.Width,  
            desc.Height,
            CG_INTERMEDIATE_SURFACE_FORMAT , 
            D3DPOOL_SCRATCH, 
            &newSurface, 
            NULL)))
         {
            m_bErrorOccurred = TRUE;

            OutputMessage(L"Error: Cannot create offscreen, system memory buffer of size %dx%d and format %s", 
                  desc.Width, 
                  desc.Height, 
                  DXUTD3DFormatToString(CG_INTERMEDIATE_SURFACE_FORMAT , false) );

            return;                    
         }

         bUseIntermediateSurface = true;
         dstSurface = newSurface;
      break;
   }


   dstSurface->GetDesc( &desc );
   dstSurface->LockRect(&lockedRect, NULL, NULL );

   switch( desc.Format )
   {
       case D3DFMT_A8R8G8B8:
       case D3DFMT_X8R8G8B8:
           cImgSurfValType = CP_VAL_UNORM8_BGRA;
       break;
       case D3DFMT_A16B16G16R16:
           cImgSurfValType = CP_VAL_UNORM16;
       break;
       case D3DFMT_A16B16G16R16F:
           cImgSurfValType = CP_VAL_FLOAT16;
       break;
       case D3DFMT_A32B32G32R32F:
           cImgSurfValType = CP_VAL_FLOAT32;
       break;
   }

   a_pSurfImg->GetImageData(cImgSurfValType, 4, lockedRect.Pitch, lockedRect.pBits );
   dstSurface->UnlockRect();

   //if intermediate surface was used, copy data from intermediate surface to a_pD3DSurface
   if(bUseIntermediateSurface == TRUE)
   {
      //copy and convert image from temp offscreen surface into a_pD3DSurface
      hr = D3DXLoadSurfaceFromSurface(
         a_pD3DSurface,    //LPDIRECT3DSURFACE9 pDestSurface,
         NULL,             //CONST PALETTEENTRY *pDestPalette,
         NULL,             //CONST RECT *pDestRect,
         dstSurface,       //LPDIRECT3DSURFACE9 pSrcSurface,
         NULL,             //CONST PALETTEENTRY *pSrcPalette,
         NULL,             //CONST RECT *pSrcRect,
         D3DX_DEFAULT,     //DWORD Filter,
         NULL              //D3DCOLOR ColorKey
      );  

      SAFE_RELEASE( newSurface );
   }

}


//--------------------------------------------------------------------------------------
//filter cube map
//--------------------------------------------------------------------------------------
void CCubeGenApp::FilterCubeMap(void)
{
   int32 fixupWidth;

   //invalidate current output cubemap
   m_bValidOutputCubemap = FALSE;

   //init cubemap processor
   m_CubeMapProcessor.Init(m_InputCubeMapSize, m_OutputCubeMapSize, m_pOutputCubeMap.m_pCubeTexture->GetLevelCount(), 4 ); 

   //copy input cubemap into cube map processor
   SetCubeMapProcessorInputCubeMap(&m_CubeMapProcessor);

   //cube edge fixup
   // SL BEGIN
   int32 EdgeFixupTech = CP_FIXUP_NONE;
   // SL END
   if(m_bCubeEdgeFixup == TRUE)
   {
      fixupWidth = m_EdgeFixupWidth;
	  // SL BEGIN
	  EdgeFixupTech = m_EdgeFixupTech;
	  // SL END
   }
   else
   {
      fixupWidth = 0;
   }

   //filter the cube map mip chain
   //m_CubeMapProcessor.FilterCubeMapMipChain(m_BaseFilterAngle, m_MipInitialFilterAngle, m_MipFilterAngleScale, 
   //    m_FilterTech, m_EdgeFixupTech, fixupWidth, m_bUseSolidAngleWeighting);
   // SL BEGIN
   //begin filtering, if one or more filtereing threads is enabled, initiate the filtering threads, and return 
   // from the function with the threads running in the background.
   m_CubeMapProcessor.InitiateFiltering(m_BaseFilterAngle, m_MipInitialFilterAngle, m_MipFilterAngleScale, 
	  m_FilterTech, EdgeFixupTech, fixupWidth, m_bUseSolidAngleWeighting, m_bUseMultithread, m_SpecularPower, m_CosinePowerDropPerMip, m_NumMipmap, m_CosinePowerMipmapChainMode,
	  m_bExcludeBase, m_bIrradianceCubemap, m_LightingModel, m_GlossScale, m_GlossBias);
   // SL END

   m_FramesSinceLastRefresh = 0;


   //retreive output cubemap from cubemap processor
   //GetCubeMapProcessorOutputCubeMap(&m_CubeMapProcessor);
}


//--------------------------------------------------------------------------------------
//if the output cubemap is invalid, the user is prompted as to whether or not
// they would like to copy the input cubemap to the output cubemap.
//
//if the user selects NO, the function returns false
//
//--------------------------------------------------------------------------------------
bool8 CCubeGenApp::HandleInvalidOutputCubemapBeforeSaving(void)
{
   //output cubemap is not valid, so copy input cubemap to output cubemap
   if(m_bValidOutputCubemap == FALSE)
   {
      int32 userResponse;
      float32 mipInitialFilterAngle;

      SetCursor( LoadCursor(NULL, IDC_ARROW) );

      userResponse = MessageBox(NULL, 
         L"Filtering has not yet been performed or is incomplete using the current filtering settings." //string continues
         L"Would you like to copy the current input cubemap to the output cubemap and then save?" //string continues
         L"  Press NO to abort saving and return to CubeMapGen.",
         L"No Output Cubemap",  //title
         MB_YESNO);

      if(userResponse == IDNO)
      {
         return FALSE;            
      }

      SetCursor( LoadCursor(NULL, IDC_WAIT) );

      m_OutputCubeMapSize = m_InputCubeMapSize;
      m_OutputCubeMapFormat = m_InputCubeMapFormat;
      CreateOutputCubeMap(0);

      mipInitialFilterAngle = 2.0f * (360.0f / (m_OutputCubeMapSize * 4.0f));

      //base filter angle of 0.0 just copies input cubemap to output cubemap
      // then build mip chain using simple filtering algorithm
      m_BaseFilterAngle = 0.0;
      m_MipInitialFilterAngle = mipInitialFilterAngle;
      m_MipFilterAngleScale = 2.0;
      m_bCubeEdgeFixup = FALSE;

      FilterCubeMap();

      //sleep until filtering thread returns
      while(m_CubeMapProcessor.GetStatus() == CP_STATUS_PROCESSING)
      {
         Sleep(1000);
      }

      //copy output cubemap from cubemap processor
      RefreshOutputCubeMap();

      //now its valid!!
      m_bValidOutputCubemap = TRUE;
   }

   return TRUE;
}




//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void CCubeGenApp::OnResetDevice(IDirect3DDevice9 *a_pDevice)
{
    //reinitialize/reload CCubeGenApp class data    
    if(m_bInitialized == FALSE)
    {
      Init(a_pDevice);
    }

    TextureListOnResetDevice();
    GeomListOnResetDevice();
    EffectListOnResetDevice();

    //refresh output cubemap if possible
    //RefreshOutputCubeMap();
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void CCubeGenApp::OnLostDevice(void)
{
    EffectListOnLostDevice();
    GeomListOnLostDevice();
    TextureListOnLostDevice();
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void CCubeGenApp::OnDestroyDevice(void)
{
    EffectListOnDestroyDevice();
    GeomListOnDestroyDevice();
    TextureListOnDestroyDevice();

    //device is destroyed, so will need to reinitialize
    m_bInitialized = FALSE;

}




