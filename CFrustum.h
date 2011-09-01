//--------------------------------------------------------------------------------------
// File: CFrustum.h
//
// frustum class
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef CFRUSTUM_H
#define CFRUSTUM_H


#include "LocalDXUT\\dxstdafx.h"
#include "types.h"

//do not allow "dxstdafx.h" to depricate any core string functions
#pragma warning( disable : 4995 )


#define PROJ_PERSPECTIVE_LH 0
#define PROJ_ORTHOGRAPHIC 2

class CFrustum
{
public:
    bool8 m_ProjType;      //projection type
    D3DXVECTOR4 m_FrustumApex;  //apex of frustum in world space
    float32 m_FOV;         // field of view
    float32 m_Aspect;      // aspect ratio (width/ height)
    float32 m_NearClip;    // near clip plane
    float32 m_FarClip;     // far clip plane
    D3DXMATRIX m_ProjMX;   // projection matrix
    D3DXMATRIX m_ViewMX;   // view matrix

    CFrustum(float32 a_FOV=60.0f, float32 a_Aspect=1.0f, float32 a_NearClip=0.1f, float32 a_FarClip=0.1f);
    ~CFrustum();

    void UpdateProjMatrix(void);
    D3DXMATRIX *GetProjMatrix(void);
    void SetProjMatrix(D3DXMATRIX *a_ProjMX);
    D3DXMATRIX *GetViewMatrix(void);
    void SetViewMatrix(D3DXMATRIX *a_ViewMX);
    D3DXVECTOR4 *GetFrustumApex(void);   
};

#endif //CFRUSTUM_H
