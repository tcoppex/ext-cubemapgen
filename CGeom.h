//--------------------------------------------------------------------------------------
// File: MeshUtils.cpp
//
// Mesh processing utilities
//  this file contains mesh load routines 
//   in the future it will contain code to generate additional supporting geometry 
//   such as degenerate quads used for smoothies, or Haines style plateus.
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef CGEOM_H
#define CGEOM_H

#include "ObjReader.h"
#include "LocalDXUT\\dxstdafx.h"
#include "types.h"
#include "VectorMacros.h"


//do not allow "dxstdafx.h" to depricate any core string functions
#pragma warning( disable : 4995 )


#define MAX_GEOM_OBJECTS 1000
#define ADJACENCY_EPSILON 0.0001f


//IB/VB created manually by the application
#define GEOM_SOURCE_MANUAL 0 

//IB/VB stored in an ID3DXMesh
#define GEOM_SOURCE_D3DXMESH 1 

//CGeom stores object information as either a ID3DXMesh or as explicit IB/VB
//  the resons for supporting explicit VB/IB is to allow for rendering of line based objects 
//  such as a cube outline, or a frustum outline
class CGeom
{
public:
    bool8                   m_bInit;
    IDirect3DDevice9        *m_pDevice;

    uint32                  m_uGeomSource;   //source of geometry
    uint32                  m_GeomListIdx;   //index of this object in the geometry list

    ID3DXMesh               *m_pMesh;        //mesh structure to store geometry 

    uint32                  m_uNumVertices;
    uint32                  m_uNumIndices;
    uint32                  m_uNumOutlineIndices;
    uint32                  m_uOffset;       //offset into vertex buffer
    uint32                  m_uStride;       //stride between vertex buffer elements
    DWORD                   m_FVF; 

    D3DXVECTOR3             m_BBoxMin;       //bounding box min
    D3DXVECTOR3             m_BBoxMax;       //bounding box max
    D3DXVECTOR3             m_CenterPoint;   //centerpoint


    LPDIRECT3DINDEXBUFFER9  m_pibIndices;
    LPDIRECT3DINDEXBUFFER9  m_pibOutlineIndices;
    LPDIRECT3DVERTEXBUFFER9 m_pvbVertices;

    CGeom(void);
    ~CGeom();
    HRESULT Init(IDirect3DDevice9 *a_pDevice);

    HRESULT LoadMesh(IDirect3DDevice9 *a_pDevice, WCHAR* a_Filename);
    HRESULT LoadDotX(WCHAR *a_Filename);
    HRESULT SaveDotX(WCHAR *a_Filename);
    HRESULT LoadObj(WCHAR *a_Filename);

    HRESULT GenerateQuad(void);
    HRESULT GenerateCube(void);
    HRESULT GenerateSphere(IDirect3DDevice9 *a_pDevice, float32 a_Radius, uint32 a_Slices, uint32 a_Stacks);

    HRESULT GenerateShadowVolumeMesh(CGeom *a_InputMesh);
    HRESULT GenerateDegenEdgeQuads(CGeom *a_InputMesh);
    HRESULT GenerateSmoothieQuads(CGeom *a_InputMesh);
    HRESULT GenerateSmoothieQuadsTex(CGeom *a_InputMesh);
    HRESULT GenerateBaryTestMesh(CGeom *a_InputMesh);
    HRESULT GenerateDegenQuadsDualPos(CGeom *a_InputMesh);

    HRESULT AddCompPRTVertexData(LPD3DXPRTCOMPBUFFER a_CompPrtBuffer);

    HRESULT GenerateCPCAPRTMesh(CGeom *a_InputMesh, LPD3DXPRTCOMPBUFFER a_CompPRTBuffer);
    HRESULT GenerateLDPRTMesh(CGeom *a_InputMesh, LPD3DXPRTBUFFER a_PRTBuffer);

    HRESULT Draw(void);
    HRESULT DrawOutline(void);

    void OnResetDevice(void);
    void OnLostDevice(void);
    void OnDestroyDevice(void);
    void SafeRelease(void);

};


//edge structure : adapted from D3D ShadowVolumes Example
struct CEdgeMapping
{
public:
    int m_anOldEdge[2];     // vertex index of the original edge
    int m_aanNewEdge[2][2]; // vertex indexes of the new edge
                            // First subscript = index of the new edge
                            // Second subscript = index of the vertex for the edge
    CEdgeMapping()
    {
        FillMemory( m_anOldEdge, sizeof(m_anOldEdge), -1 );
        FillMemory( m_aanNewEdge, sizeof(m_aanNewEdge), -1 );
    }
};


//from the .obj file
struct SSphereVert
{
    D3DXVECTOR3 m_Position;           //Position
    D3DXVECTOR3 m_VertexNormal;       //Vertex normal
    D3DXVECTOR2 m_TexCoord;           //Tex coords  
    D3DXVECTOR3 m_TangentU;           //TangentU
    D3DXVECTOR3 m_TangentV;           //TangentV
    const static D3DVERTEXELEMENT9 Decl[6];
};


//from the .obj file
struct FROMOBJVERT
{
    D3DXVECTOR3 Position;           //Position
    D3DXVECTOR3 VertexNormal;       //Vertex normal
    D3DXVECTOR2 TexCoord;           //Tex coords  
    D3DXVECTOR3 TangentU;           //TangentU
    D3DXVECTOR3 TangentV;           //TangentV
    const static D3DVERTEXELEMENT9 Decl[6];
};



void GeomListOnResetDevice(void);
void GeomListOnLostDevice(void);
void GeomListOnDestroyDevice(void);

//used for generating degenerate edge quads
int FindEdgeInMappingTable(int nV1, int nV2, CEdgeMapping *pMapping, int nCount );



#endif //CGEOM_H

