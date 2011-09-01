//--------------------------------------------------------------------------------------
// File: CFrustum.cpp
//
// frustum class
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------
#include "CFrustum.h"


//--------------------------------------------------------------------------------------
//setup initial frustum
//--------------------------------------------------------------------------------------
CFrustum::CFrustum(float32 a_FOV, float32 a_Aspect, float32 a_NearClip, float32 a_FarClip)
{
    m_ProjType = PROJ_PERSPECTIVE_LH;
    m_FOV = a_FOV;
    m_Aspect = a_Aspect;
    m_NearClip = a_NearClip;
    m_FarClip = a_FarClip;

    UpdateProjMatrix();
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
CFrustum::~CFrustum()
{

}


//--------------------------------------------------------------------------------------
// update internal projection matrix based on settings
//--------------------------------------------------------------------------------------
void CFrustum::UpdateProjMatrix(void)
{

    D3DXMatrixPerspectiveFovLH(&m_ProjMX, m_FOV, m_Aspect, m_NearClip, m_FarClip);
    /*
    switch(m_ProjType)
    {
        case PROJ_PERSPECTIVE_LH:
            D3DXMatrixPerspectiveFovLH(&m_ProjMX, m_FOV, m_Aspect, m_NearClip, m_FarClip);
        break;
    };
    */
}


//--------------------------------------------------------------------------------------
// get frustum projection matrix
//--------------------------------------------------------------------------------------
D3DXMATRIX *CFrustum::GetProjMatrix(void)
{
    return &m_ProjMX;
}


//--------------------------------------------------------------------------------------
// set frustum projection matrix 
// 
//--------------------------------------------------------------------------------------
void CFrustum::SetProjMatrix(D3DXMATRIX *a_ProjMX)
{
    //extract fov info from matrix
    //assume Perspective LH for now
    m_ProjMX = *a_ProjMX;
    m_FOV = 2.0f * tanf(m_ProjMX._22);
    m_Aspect = m_ProjMX._11 / m_ProjMX._22; 
    m_NearClip = -m_ProjMX._43 / m_ProjMX._33; 
    m_FarClip = -(m_NearClip * m_ProjMX._33) / (1.0f - m_ProjMX._33); 

}


//--------------------------------------------------------------------------------------
// set frustum projection matrix 
// 
//--------------------------------------------------------------------------------------
D3DXMATRIX *CFrustum::GetViewMatrix(void)
{
    return(&m_ViewMX);

}    


//--------------------------------------------------------------------------------------
// set frustum projection matrix 
// 
//--------------------------------------------------------------------------------------
void CFrustum::SetViewMatrix(D3DXMATRIX *a_ViewMX)
{
    m_ViewMX = *a_ViewMX;
    
    //initialize frustum apex
    GetFrustumApex();
}


//--------------------------------------------------------------------------------------
// get apex of frustum in world coords
// 
//--------------------------------------------------------------------------------------
D3DXVECTOR4 *CFrustum::GetFrustumApex(void)
{
    D3DXVECTOR4 origin = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 1.0f );
    D3DXMATRIX invViewMatrix;
    float32 det;

    D3DXMatrixInverse(&invViewMatrix, &det, &m_ViewMX);
    D3DXVec4Transform(&m_FrustumApex, &origin, &invViewMatrix );
    
    return &m_FrustumApex;
}


