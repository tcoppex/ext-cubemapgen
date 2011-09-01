//--------------------------------------------------------------------------------------
// File: CEffectList.h
//
//  A list of all effects defined using this class is kept
//  in order to handle cleanup, and OnResetDevice, OnDestroyDevice, and 
//   OnLostDevice events with single function calls.
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------

#include "CEffectList.h"

static int32 sg_NumEffects=0;
static CEffect *sg_EffectPtrList[MAX_NUM_EFFECTS];


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
CEffect::CEffect(void)
{
    m_pEffect = NULL;
    m_EffectListIdx = sg_NumEffects;
    sg_EffectPtrList[sg_NumEffects]=this;

    sg_NumEffects++;
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
CEffect::~CEffect(void)
{
    int32 i;

//    OnLostDevice();
//    OnDestroyDevice();
    SAFE_RELEASE(m_pEffect);

    //compact list to remove entry
    for(i=m_EffectListIdx; i<sg_NumEffects-1; i++)
    {
        sg_EffectPtrList[i] = sg_EffectPtrList[i+1]; 
    }

    //decrease number of list entries to account for removed element
    sg_NumEffects--;

}


//--------------------------------------------------------------------------------------
//LoadShader
//--------------------------------------------------------------------------------------
HRESULT CEffect::LoadShader(WCHAR *a_Filename, IDirect3DDevice9 *a_pDevice)
{
   HRESULT hr;
   ID3DXBuffer *errorBuffer;
   WCHAR str[MAX_PATH];

   m_pDevice = a_pDevice;

   //delete old effect if multiple calls to load shader are made
   SAFE_RELEASE(m_pEffect);

   // Define DEBUG_VS and/or DEBUG_PS to debug vertex and/or pixel shaders with the 
   // shader debugger. Debugging vertex shaders requires either REF or software vertex 
   // processing, and debugging pixel shaders requires REF.  The 
   // D3DXSHADER_FORCE_*_SOFTWARE_NOOPT flag improves the debug experience in the 
   // shader debugger.  It enables source level debugging, prevents instruction 
   // reordering, prevents dead code elimination, and forces the compiler to compile 
   // against the next higher available software target, which ensures that the 
   // unoptimized shaders do not exceed the shader model limitations.  Setting these 
   // flags will cause slower rendering since the shaders will be unoptimized and 
   // forced into software.  See the DirectX documentation for more information about 
   // using the shader debugger.
   DWORD dwShaderFlags = 0;
   #ifdef DEBUG_VS
       dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
   #endif
   #ifdef DEBUG_PS
       dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
   #endif

   // Read the D3DX effect file
   hr = DXUTFindDXSDKMediaFileCch( str, MAX_PATH, a_Filename );

   // search DX search paths for .fx file
   if(FAILED(hr))
   {  
      _snwprintf_s(m_ErrorMsg, MAX_ERROR_MSG, MAX_ERROR_MSG, L"Error: Can't Find Shader %s in any search paths.", a_Filename);
      return STG_E_FILENOTFOUND;
   }


   //load and compile .fx file
   // If this fails, there should be debug output as to 
   // why the .fx file failed to compile
   hr = D3DXCreateEffectFromFile( m_pDevice, str, NULL, NULL, 
       dwShaderFlags, NULL, &m_pEffect, &errorBuffer );

   //return failure and record error message if failure
   if(FAILED(hr))
   {  //error is in 8-bit character format, so for swprintf to use the string, %S (capital S) is used.
      _snwprintf_s(m_ErrorMsg, MAX_ERROR_MSG, MAX_ERROR_MSG, L"Shader Compile Error: %S.", errorBuffer->GetBufferPointer() );
      errorBuffer->Release();
      return hr;
   }

   _snwprintf_s(m_ErrorMsg, MAX_ERROR_MSG, MAX_ERROR_MSG, L"FX file %s Compiled Successfully.", a_Filename);

   return S_OK;
}


//--------------------------------------------------------------------------------------
//LoadShaderFromResource
//
//loads shader given a resource id
//--------------------------------------------------------------------------------------
HRESULT CEffect::LoadShaderFromResource(uint32 a_ResourceID, IDirect3DDevice9 *a_pDevice)
{
   HRESULT hr;
   ID3DXBuffer *errorBuffer;
   HRSRC       resourceInfo;
   HGLOBAL     resourceData;
   char8       *shaderText;

   m_pDevice = a_pDevice;

   //delete old effect if multiple calls to load shader are made
   SAFE_RELEASE(m_pEffect);

   // Define DEBUG_VS and/or DEBUG_PS to debug vertex and/or pixel shaders with the 
   // shader debugger. Debugging vertex shaders requires either REF or software vertex 
   // processing, and debugging pixel shaders requires REF.  The 
   // D3DXSHADER_FORCE_*_SOFTWARE_NOOPT flag improves the debug experience in the 
   // shader debugger.  It enables source level debugging, prevents instruction 
   // reordering, prevents dead code elimination, and forces the compiler to compile 
   // against the next higher available software target, which ensures that the 
   // unoptimized shaders do not exceed the shader model limitations.  Setting these 
   // flags will cause slower rendering since the shaders will be unoptimized and 
   // forced into software.  See the DirectX documentation for more information about 
   // using the shader debugger.
   DWORD dwShaderFlags = 0;
   #ifdef DEBUG_VS
      dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
   #endif
   #ifdef DEBUG_PS
      dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
   #endif

   //load shadertext from resource
   resourceInfo = FindResource(NULL, MAKEINTRESOURCE(a_ResourceID), L"EFFECTFILE"); 

   if(resourceInfo == NULL)
   {
      _snwprintf_s(m_ErrorMsg, MAX_ERROR_MSG, MAX_ERROR_MSG, L"Resource %d of type ''EFFECTFILE'' not found.", a_ResourceID );   
   }

   resourceData = LoadResource(NULL, resourceInfo ); 

   shaderText = (char8 *)LockResource( resourceData);

   //Create effect from text loaded from resource
   // load and compile .fx file
   // If this fails, there should be debug output as to 
   // why the .fx file failed to compile
   hr = D3DXCreateEffect(
      m_pDevice,           //LPDIRECT3DDEVICE9 pDevice,
      shaderText,          //LPCVOID pSrcData,
      (uint32)SizeofResource(NULL, resourceInfo),  //UINT SrcDataLen,
      NULL,                //const D3DXMACRO *pDefines,
      NULL,                //LPD3DXINCLUDE pInclude,
      0,                   //DWORD Flags,
      NULL,                //LPD3DXEFFECTPOOL pPool,
      &m_pEffect,          //LPD3DXEFFECT *ppEffect,
      &errorBuffer         //LPD3DXBUFFER *ppCompilationErrors
      );
      
   UnlockResource( resourceData );
   FreeResource( resourceData );

   //return failure and record error message if failure
   if(FAILED(hr))
   {  //error is in 8-bit character format, so for swprintf to use the string, %S (capital S) is used.
      if(errorBuffer != NULL)
      {
         _snwprintf_s(m_ErrorMsg, MAX_ERROR_MSG, MAX_ERROR_MSG, L"Shader Compile Error: %S.", errorBuffer->GetBufferPointer() );
      }
      else
      {
         _snwprintf_s(m_ErrorMsg, MAX_ERROR_MSG, MAX_ERROR_MSG, L"Shader Compile Error: no output from D3DXCreateEffectFromResource." );      
      }

      SAFE_RELEASE(errorBuffer);
      return hr;
   }

   _snwprintf_s(m_ErrorMsg, MAX_ERROR_MSG, MAX_ERROR_MSG, L"FX file in Resource %d Compiled Successfully.", a_ResourceID);

   return S_OK;
}


//--------------------------------------------------------------------------------------
// Draw geometry using effect
//--------------------------------------------------------------------------------------
HRESULT CEffect::DrawGeom(char *a_TechniqueName, CGeom *a_Geom)
{
    UINT cPasses, iPass;
    HRESULT hr;

    V_RETURN( m_pEffect->SetTechnique( a_TechniqueName ) );
    V_RETURN( m_pEffect->Begin(&cPasses, 0) );

    for (iPass = 0; iPass < cPasses; iPass++)
    {
        V_RETURN( m_pEffect->BeginPass(iPass));

        //draw scene geometry
        V_RETURN(a_Geom->Draw());

        V_RETURN( m_pEffect->EndPass());
    }

    V_RETURN( m_pEffect->End());

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Draw geometry outline using effect
//--------------------------------------------------------------------------------------
HRESULT CEffect::DrawGeomOutline(char *a_TechniqueName, CGeom *a_Geom)
{
    UINT cPasses, iPass;
    HRESULT hr;

    V( m_pEffect->SetTechnique( a_TechniqueName ) );
    V( m_pEffect->Begin(&cPasses, 0) );

    for (iPass = 0; iPass < cPasses; iPass++)
    {
        V( m_pEffect->BeginPass(iPass));

        //draw scene geometry
        a_Geom->DrawOutline();

        V( m_pEffect->EndPass());
    }

    V( m_pEffect->End());

    return S_OK;
}



//--------------------------------------------------------------------------------------
//OnResetDevice
//--------------------------------------------------------------------------------------
void EffectListOnResetDevice(void)
{
   int32 i;

   //call reset device function for each effect
   for(i=0; i<sg_NumEffects; i++)
   {
      if(sg_EffectPtrList[i]->m_pEffect != NULL)
      {
         sg_EffectPtrList[i]->m_pEffect->OnResetDevice();
      }
   }
}

//--------------------------------------------------------------------------------------
//OnDestroyDevice
//--------------------------------------------------------------------------------------
void EffectListOnDestroyDevice(void)
{
    int32 i;

    //safe release effect
    for(i=0; i<sg_NumEffects; i++)
    {
        SAFE_RELEASE(sg_EffectPtrList[i]->m_pEffect);
    }
}

//--------------------------------------------------------------------------------------
//OnLostDevice
//--------------------------------------------------------------------------------------
void EffectListOnLostDevice(void)
{
   int32 i;

   //call lost device function for each effect
   for(i=0; i<sg_NumEffects; i++)
   {
      if(sg_EffectPtrList[i]->m_pEffect != NULL)
      {
         sg_EffectPtrList[i]->m_pEffect->OnLostDevice();
      }
   }
}





