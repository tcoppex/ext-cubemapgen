//--------------------------------------------------------------------------------------
// File: CTextureList.h
//
//  Class to allocate, load, and keep track of textures for the app
//  
//  A list of all textures defined using this class is kept
//  in order to handle cleanup, and OnResetDevice, OnDestroyDevice, and 
//   OnLostDevice events with single function calls.
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------

#include "CTexture.h"

static IDirect3DDevice9 *sg_pDevice = NULL;
static int32 sg_NumTextures = 0;
static CTexture *sg_TexturePtrList[MAX_NUM_TEXTURES];

//--------------------------------------------------------------------------------------
// init texture class
//--------------------------------------------------------------------------------------
CTexture::CTexture()
{
    m_TextureListIdx = sg_NumTextures;          //Index of this texture in the texture list
    sg_TexturePtrList[sg_NumTextures] = this;
    sg_NumTextures++;                           //Increment number of textures in texture list

    m_SurfaceType = IMG_TYPE_NONE;              //Type of surface

    m_pTexture = NULL;                          //Texture 
    m_pCubeTexture = NULL;                      //Cube Texture 
    m_pSurface = NULL;                          //Surface

    m_ImageSource = IMG_SOURCE_NONE;            //Image source none

    m_Format = D3DFMT_UNKNOWN;
    m_Pool = D3DPOOL_DEFAULT;

    wcscpy_s(m_pFilename, 4097, L"");                   //No filename yet
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
CTexture::~CTexture()
{
    int32 i;

    //release textures
    OnDestroyDevice();

    //remove entry and compact list
    for(i=m_TextureListIdx; i<sg_NumTextures-1; i++)
    {
        sg_TexturePtrList[i] = sg_TexturePtrList[i+1]; 
    }

    //decrease number of list entries to account for removed element
    sg_NumTextures--;
}


//--------------------------------------------------------------------------------------
//creates a texture using the given parameters
//--------------------------------------------------------------------------------------
HRESULT CTexture::CreateTexture(UINT a_Width, UINT a_Height, 
    UINT a_Levels, DWORD a_Usage, D3DFORMAT a_Format, D3DPOOL a_Pool)
{
   HRESULT hr;    
   D3DCAPS9 deviceCaps;
   D3DDISPLAYMODE displayMode;
   IDirect3D9 *pD3D; 

   m_SurfaceType = IMG_TYPE_TEXTURE;
   m_Width = a_Width;
   m_Height = a_Height;
   m_Pool = a_Pool;
   m_Format = a_Format;

   SAFE_RELEASE( m_pTexture );
   SAFE_RELEASE( m_pSurface );
   SAFE_RELEASE( m_pCubeTexture );

   sg_pDevice->GetDeviceCaps(&deviceCaps);
   sg_pDevice->GetDisplayMode(0, &displayMode);
   sg_pDevice->GetDirect3D(&pD3D);

   hr = pD3D->CheckDeviceFormat(
      deviceCaps.AdapterOrdinal,   
      deviceCaps.DeviceType,     
      displayMode.Format,
      0, 
      D3DRTYPE_TEXTURE,
      a_Format);

   SAFE_RELEASE(pD3D);

   //is texture supported
   if( FAILED(hr))
   {
      return E_FAIL;
   }

   V_RETURN( sg_pDevice->CreateTexture( a_Width, a_Height, a_Levels, a_Usage, a_Format,
                              a_Pool, &m_pTexture, NULL ));

   m_NumMipLevels = m_pTexture->GetLevelCount();

   return S_OK;
}


//--------------------------------------------------------------------------------------
//FillTexture  fill a texture with a given initial color
//
// only for textures of type A8R8G8B8 or X8R8G8B8
//--------------------------------------------------------------------------------------
void CTexture::FillTexture(D3DCOLOR a_Color )
{
    uint32 i, j;
    int32 m; 
    HRESULT hr;
    D3DLOCKED_RECT rect;

    //texture walking pointer
    DWORD *pPixel;

    if( (m_Format == D3DFMT_A8R8G8B8) || (m_Format == D3DFMT_X8R8G8B8) )
    {
        for(m=0; m<m_NumMipLevels; m++)
        {
            D3DSURFACE_DESC desc;

            m_pTexture->GetLevelDesc(m, &desc);

            V( m_pTexture->LockRect(m, &rect, NULL, 0) );

            //texture walking pointer
            pPixel = (DWORD *)rect.pBits;

            for(j=0; j<desc.Height ; j++)
            {
                for(i=0; i<desc.Width; i++)
                {                
                    *(pPixel + i) = a_Color;
                }

                pPixel += (rect.Pitch/4);  //pitch is in bytes, but pointer is a dword pointer
            }

            V( m_pTexture->UnlockRect(m) );
        }
    }
}



//--------------------------------------------------------------------------------------
//CreateDepthStencilSurface
//--------------------------------------------------------------------------------------
HRESULT CTexture::CreateDepthStencilSurface(UINT a_Width, 
    UINT a_Height, D3DFORMAT a_Format, D3DMULTISAMPLE_TYPE a_MultiSampleType, DWORD a_MultisampleQuality,
    BOOL a_bDiscard ) 
{
    HRESULT hr;    

    m_Pool = D3DPOOL_DEFAULT;
    m_SurfaceType = IMG_TYPE_DEPTH_STENCIL;
    m_Width = a_Width;
    m_Height = a_Height;
    m_Format = a_Format;

    SAFE_RELEASE(m_pTexture);
    SAFE_RELEASE( m_pCubeTexture );
    SAFE_RELEASE(m_pSurface);

    V_RETURN( sg_pDevice->CreateDepthStencilSurface(a_Width, a_Height, a_Format, a_MultiSampleType, 
        a_MultisampleQuality, a_bDiscard, &m_pSurface, NULL));

    return S_OK;
}


//--------------------------------------------------------------------------------------
//creates a texture using the given parameters
//--------------------------------------------------------------------------------------
HRESULT CTexture::CreateCubeTexture(UINT a_Size, 
    UINT a_Levels, DWORD a_Usage, D3DFORMAT a_Format, D3DPOOL a_Pool)
{
   HRESULT hr;    
   D3DCAPS9 deviceCaps;
   D3DDISPLAYMODE displayMode;
   IDirect3D9 *pD3D; 

   m_SurfaceType = IMG_TYPE_CUBEMAP;
   m_Width = a_Size;
   m_Height = a_Size;
   m_Pool = a_Pool;
   m_Format = a_Format;

   SAFE_RELEASE( m_pTexture );
   SAFE_RELEASE( m_pSurface );
   SAFE_RELEASE( m_pCubeTexture );

   
   sg_pDevice->GetDeviceCaps(&deviceCaps);
   sg_pDevice->GetDisplayMode(0, &displayMode);
   sg_pDevice->GetDirect3D(&pD3D);

   hr = pD3D->CheckDeviceFormat(
      deviceCaps.AdapterOrdinal,   
      deviceCaps.DeviceType,     
      displayMode.Format,
      0, 
      D3DRTYPE_CUBETEXTURE,
      a_Format);

   SAFE_RELEASE(pD3D);


   //is texture supported
   if( FAILED(hr))
   {
      return E_FAIL;
   }

   //create texture
   if(FAILED( sg_pDevice->CreateCubeTexture(a_Size, a_Levels, a_Usage, 
      a_Format, a_Pool, &m_pCubeTexture, NULL )))
   {
     return E_FAIL;
   }

   m_NumMipLevels = m_pCubeTexture->GetLevelCount();

   return S_OK;
}


//--------------------------------------------------------------------------------------
//FillCubeTexture  set faces in cube map to per-face colors spacified in a_FaceColors
//  array
//
// only for textures of type A8R8G8B8 or X8R8G8B8
//--------------------------------------------------------------------------------------
void CTexture::FillCubeTexture(D3DCOLOR *a_FaceColors )
{
    uint32 i, j, iFaceIdx;
    int32 m; 
    HRESULT hr;
    D3DLOCKED_RECT rect;

    //texture walking pointer
    DWORD *pPixel;

    if( (m_Format == D3DFMT_A8R8G8B8) || (m_Format == D3DFMT_X8R8G8B8) )
    {
        for(iFaceIdx=0; iFaceIdx<6; iFaceIdx++)
        {
            for(m=0; m<m_NumMipLevels; m++)
            {
                D3DSURFACE_DESC desc;

                V( m_pCubeTexture->GetLevelDesc(m, &desc) );

                V( m_pCubeTexture->LockRect((D3DCUBEMAP_FACES)iFaceIdx, m, &rect, NULL, 0) );

                //texture walking pointer
                pPixel = (DWORD *)rect.pBits;

                for(j=0; j<desc.Height ; j++)
                {
                    for(i=0; i<desc.Width; i++)
                    {                
                        *(pPixel + i) = a_FaceColors[iFaceIdx];
                    }

                    pPixel += (rect.Pitch/4);  //pitch is in bytes, but pointer is a dword pointer
                }

                V( m_pCubeTexture->UnlockRect((D3DCUBEMAP_FACES)iFaceIdx, m) );
            }
        }
    }
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
HRESULT CTexture::LoadTexture(WCHAR *a_Filename)
{
    m_SurfaceType = IMG_TYPE_TEXTURE;
    m_ImageSource = IMG_SOURCE_FILE;

    SAFE_RELEASE(m_pTexture);
    SAFE_RELEASE(m_pSurface);
    SAFE_RELEASE(m_pCubeTexture);

    m_Pool =  D3DPOOL_MANAGED;
    
    if( FAILED( D3DXCreateTextureFromFile( sg_pDevice, a_Filename, &m_pTexture) ))
    {
        OutputMessage(L"Error loading texture %s, file is either non-existant, invalid, or of an unsupported format.", a_Filename);
        return(E_FAIL);
    }

    D3DSURFACE_DESC desc;
    m_pTexture->GetLevelDesc(0, &desc);

    m_Width = desc.Width;
    m_Height = desc.Height;
    m_Format = desc.Format;
    m_NumMipLevels = m_pTexture->GetLevelCount();

    wcscpy_s(m_pFilename, 4097, a_Filename);

    return S_OK;
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
HRESULT CTexture::LoadTextureFromResource(uint32 a_ResourceID)
{
   HRESULT     hr;
   HRSRC       resourceInfo;
   HGLOBAL     resourceData;
   uint8       *rawFileData;

   m_SurfaceType = IMG_TYPE_TEXTURE;
   m_ImageSource = IMG_SOURCE_RESOURCE;

   SAFE_RELEASE(m_pTexture);
   SAFE_RELEASE(m_pSurface);
   SAFE_RELEASE(m_pCubeTexture);

   m_Pool = D3DPOOL_MANAGED;
   
   //load raw file data from resource
   resourceInfo = FindResource(NULL, MAKEINTRESOURCE(a_ResourceID), L"TEXTUREDDS"); 

   if(resourceInfo == NULL)
   {
      OutputMessage(L"Resource %d of type ''TEXTUREDDS'' not found.", a_ResourceID );   
   }

   resourceData = LoadResource(NULL, resourceInfo ); 
   rawFileData = (uint8 *)LockResource( resourceData);

   hr = D3DXCreateTextureFromFileInMemory(          
      sg_pDevice,                    //LPDIRECT3DDEVICE9 pDevice,
      rawFileData,                  // LPCVOID pSrcData,
      SizeofResource(NULL, resourceInfo), // UINT SrcDataSize,
      &m_pTexture);                  // LPDIRECT3DTEXTURE9 *ppTexture
   
      
   UnlockResource( resourceData );
   FreeResource( resourceData );

   
   if( FAILED( hr ))
   {
      OutputMessage(L"Error loading texture from resource %d of type ''TEXTUREDDS''", a_ResourceID);
      return(E_FAIL);
   }

   D3DSURFACE_DESC desc;
   m_pTexture->GetLevelDesc(0, &desc);

   m_Width = desc.Width;
   m_Height = desc.Height;
   m_Format = desc.Format;
   m_NumMipLevels = m_pTexture->GetLevelCount();

   wcscpy_s( m_pFilename, 4097, L"" );

   return S_OK;
}


//--------------------------------------------------------------------------------------
//LoadCubeTexture
//--------------------------------------------------------------------------------------
HRESULT CTexture::LoadCubeTexture(WCHAR *a_Filename)
{
    m_SurfaceType = IMG_TYPE_CUBEMAP;
    m_ImageSource = IMG_SOURCE_FILE;

    SAFE_RELEASE(m_pTexture);
    SAFE_RELEASE(m_pSurface);
    SAFE_RELEASE(m_pCubeTexture);

    m_Pool =  D3DPOOL_MANAGED;
    if(FAILED(D3DXCreateCubeTextureFromFile( sg_pDevice, a_Filename, &m_pCubeTexture)))
    {
        OutputMessage(L"Error loading texture %s, file is either non-existant, invalid, or of an unsupported format.", a_Filename);
        return(E_FAIL);
    }

    D3DSURFACE_DESC desc;
    m_pCubeTexture->GetLevelDesc(0, &desc);

    m_Width = desc.Width;
    m_Height = desc.Height;
    m_Format = desc.Format;
    m_NumMipLevels = m_pCubeTexture->GetLevelCount();

    wcscpy_s( m_pFilename, 4097, a_Filename );

    return S_OK;
}





//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void CTexture::OnResetDevice(void)
{

}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void CTexture::OnLostDevice(void)
{
    //if allocated in default pool, safe release the texture here.
    if(m_Pool == D3DPOOL_DEFAULT)
    {
        switch(m_SurfaceType)
        {
            case IMG_TYPE_NONE:       
            break;
            case IMG_TYPE_TEXTURE:
            case IMG_TYPE_CUBEMAP:
            case IMG_TYPE_VOLUMEMAP:
                SAFE_RELEASE(m_pTexture);
            break;
            case IMG_TYPE_SURFACE:       
            case IMG_TYPE_DEPTH_STENCIL:
                SAFE_RELEASE(m_pSurface);
            break;
        }
    }

}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void CTexture::OnDestroyDevice(void)
{

    SAFE_RELEASE(m_pTexture);
    SAFE_RELEASE(m_pCubeTexture);
    SAFE_RELEASE(m_pSurface);

}


//--------------------------------------------------------------------------------------
//set D3D Device for CTexture class member to use
//--------------------------------------------------------------------------------------
void TextureListSetDevice(IDirect3DDevice9 *a_pDevice)
{
    sg_pDevice = a_pDevice;

}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void TextureListOnResetDevice(void)
{
    int32 i;

    //call lost device function for each effect
    for(i=0; i<sg_NumTextures; i++)
    {
        sg_TexturePtrList[i]->OnResetDevice();
    }
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void TextureListOnLostDevice(void)
{
    int32 i;

    //call lost device function for each effect
    for(i=0; i<sg_NumTextures; i++)
    {
        sg_TexturePtrList[i]->OnLostDevice();
    }
}


//--------------------------------------------------------------------------------------
// call OnDestroyDevice for all textures in texture list
//--------------------------------------------------------------------------------------
void TextureListOnDestroyDevice(void)
{
    int32 i;

    //call lost device function for each effect
    for(i=0; i<sg_NumTextures; i++)
    {
        sg_TexturePtrList[i]->OnDestroyDevice();
    }
}


//--------------------------------------------------------------------------------------
//round number up to next highest power of two if not already a power of two
// useful for choosing next largest texture size that is a power of two
//--------------------------------------------------------------------------------------
uint32 RoundupToNextPow2(uint32 x)
{
    x--;

    x |= x>>1;  //replicate highest order set bit to next bit
    x |= x>>2;  //replicate 2 highest order set bits to next 2 bits
    x |= x>>4;  //replicate 4 highest order set bits to next 4 bits
    x |= x>>8;  //replicate 8 highest order set bits to next 8 bits
    x |= x>>16; //replicate 16 highest order set bits to next 16 bits

    x++;

    return(x);
}






