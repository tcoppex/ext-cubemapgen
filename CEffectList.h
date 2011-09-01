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

#pragma once
#ifndef CEFFECT_LIST_H
#define CEFFECT_LIST_H

#include "CGeom.h"
#include "LocalDXUT\\dxstdafx.h"
#include "types.h"


#define MAX_NUM_EFFECTS 1000

#define MAX_ERROR_MSG  16384

class CEffect
{
public:
    IDirect3DDevice9    *m_pDevice;

    int32               m_EffectListIdx;  //index of effect in effect list
    ID3DXEffect         *m_pEffect;  

    WCHAR               m_ErrorMsg[MAX_ERROR_MSG];  //shader error message


    CEffect(void);
    virtual ~CEffect(void);
    HRESULT LoadShader(WCHAR *a_Filename, IDirect3DDevice9 *a_pDevice);
    HRESULT LoadShaderFromResource(uint32 a_ResourceID, IDirect3DDevice9 *a_pDevice);

    HRESULT DrawGeom(char *a_TechniqueName, CGeom *a_Geom);
    HRESULT DrawGeomOutline(char *a_TechniqueName, CGeom *a_Geom);

    //in base class
    //void OnResetDevice(void);
    //void OnLostDevice(void);

    //effect class does not contain an on destroy device, just adding an empty function for 
    // completeness
    void OnDestroyDevice(void);

};

void EffectListOnResetDevice(void);
void EffectListOnDestroyDevice(void);
void EffectListOnLostDevice(void);


#endif //CEFFECT_LIST_H