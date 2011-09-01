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

#pragma once
#ifndef CTEXTURE_LIST_H
#define CTEXTURE_LIST_H

#include "LocalDXUT\\dxstdafx.h"
#include "types.h"
//#include "Random.h"
#include "CCubeMapProcessor.h"
#include "ErrorMsg.h"

//do not allow "dxstdafx.h" to depricate any core string functions
#pragma warning( disable : 4995 )


#define MAX_NUM_TEXTURES 1000

#define IMG_TYPE_NONE 0
#define IMG_TYPE_TEXTURE 1
#define IMG_TYPE_SURFACE 2
#define IMG_TYPE_DEPTH_STENCIL 3
#define IMG_TYPE_CUBEMAP 4
#define IMG_TYPE_VOLUMEMAP 5

#define IMG_SOURCE_NONE 0
#define IMG_SOURCE_FILE 1
#define IMG_SOURCE_RESOURCE 2

class CTexture
{
public:
    int32                   m_Width;            //texture width
    int32                   m_Height;           //texture height
    int32                   m_NumMipLevels;     // number of miplevels
    int32                   m_TextureListIdx;   //Index of this texture in the texture list
    int32                   m_SurfaceType;      //Type of surface
    D3DPOOL                 m_Pool;
    D3DFORMAT               m_Format;           //format of texture
    LPDIRECT3DTEXTURE9      m_pTexture;         //Texture 
    LPDIRECT3DCUBETEXTURE9  m_pCubeTexture;     //Cube texture
    LPDIRECT3DSURFACE9      m_pSurface;         //Surface
    WCHAR                   m_pFilename[4097];  //texture filename
    int32                   m_ImageSource;      //source of image used for texture e.g. file/create call

    //bool8                   m_bUseBackingStore;             //does texture use backing store?
    //LPDIRECT3DSURFACE9      *m_pBackingStoreSurfaceArray;   //temporary backing store to copy texture contents to handle lost device calls
    
    CTexture();
    ~CTexture();

    void InitTexture();
    HRESULT CreateTexture(UINT a_Width, UINT a_Height, UINT a_Levels, DWORD a_Usage, D3DFORMAT a_Format, D3DPOOL a_Pool);
    void FillTexture(D3DCOLOR a_Color);

    HRESULT CreateCubeTexture(UINT a_Size, UINT a_Levels, DWORD a_Usage, D3DFORMAT a_Format, D3DPOOL a_Pool);
    void FillCubeTexture(D3DCOLOR *a_FaceColors);
    HRESULT CreateDepthStencilSurface(UINT a_Width, UINT a_Height, D3DFORMAT a_Format, 
        D3DMULTISAMPLE_TYPE a_MultiSampleType, DWORD a_MultisampleQuality, BOOL a_bDiscard );
    HRESULT LoadTexture(WCHAR *a_Filename);
    HRESULT LoadTextureFromResource(uint32 a_ResourceID);

    HRESULT LoadCubeTexture(WCHAR *a_Filename);

    void OnResetDevice(void);
    void OnLostDevice(void);
    void OnDestroyDevice(void);

};

//sets the device used by the CTexture class for creating textures
void TextureListSetDevice(IDirect3DDevice9 *sg_pDevice);

void TextureListOnResetDevice(void);
void TextureListOnLostDevice(void);
void TextureListOnDestroyDevice(void);

//round number up to the next power of two if not already a power of two
uint32 RoundupToNextPow2(uint32 x);



#endif //CTEXTURE_LIST_H
