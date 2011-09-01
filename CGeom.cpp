//--------------------------------------------------------------------------------------
// File: MeshUtils.cpp
//
// Mesh processing utilities
//  This file contains mesh load/creation routines 
//   in the future it will contain code to generate additional supporting geometry 
//   such as degenerate quads used for smoothies, or Haines style plateus.
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------


#include "CGeom.h"


//geometry point list keeps list of allocated geometry objects
static int32 sg_NumGeom=0;
static CGeom *sg_GeomPtrList[MAX_GEOM_OBJECTS];



//format for vertices of sphere object
const D3DVERTEXELEMENT9 SSphereVert::Decl[6] =
{
    { 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
    { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,    0 },  //vertex normal
    { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },  //texcoords
    { 0, 32, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  1 },  //tangent space x dir
    { 0, 44, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  2 },  //tangent space y dir
    D3DDECL_END()
};


//format for vertices taken from obj format
const D3DVERTEXELEMENT9 FROMOBJVERT::Decl[6] =
{
    { 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
    { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,    0 },  //vertex normal
    { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },  //texcoords
    { 0, 32, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  1 },  //tangent space x dir
    { 0, 44, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  2 },  //tangent space y dir
    D3DDECL_END()
};


//-----------------------------------------------------------------------------------------------
//compute face normal from positions
//-----------------------------------------------------------------------------------------------
D3DXVECTOR3 CalcFaceNorm(D3DXVECTOR3 *a_PosArray)
{
    D3DXVECTOR3 edgeVec[2];
    D3DXVECTOR3 xprod, out;

    edgeVec[0] = (a_PosArray[1]) - (a_PosArray[0]);
    edgeVec[1] = (a_PosArray[2]) - (a_PosArray[0]);

    D3DXVec3Cross( &xprod, &edgeVec[0], &edgeVec[1]);
    D3DXVec3Normalize(&out, &xprod);

    return out;
}



//-----------------------------------------------------------------------------------------------
//compute face normal given a face idx and a 32 bit index buffer, and raw vertex buffer data
//-----------------------------------------------------------------------------------------------
D3DXVECTOR3 CalcFaceNorm(DWORD a_FaceIdx, DWORD *a_pInIBData, uint8 *a_pRawVertData, int32 a_Stride, int32 a_Offset)
{
    D3DXVECTOR3 pos[3];
    DWORD       vertIdx;
    float32     *vertPosPtr;
    int32 i;

    for(i=0; i<3; i++)
    {
        vertIdx = a_pInIBData[(a_FaceIdx * 3) + i];
        vertPosPtr = (float32 *)(a_pRawVertData + a_Offset + (a_Stride * vertIdx) );
        pos[i] = D3DXVECTOR3(vertPosPtr);
    }

    return CalcFaceNorm(pos); 
}


//--------------------------------------------------------------------------------------
//Computes the offset of a given quantity in a vertex given its semantic, and the 
// vertex format decl
//--------------------------------------------------------------------------------------
int32 ComputeOffsetFromSemantic(D3DVERTEXELEMENT9 *aInputDecl, D3DDECLUSAGE aUsage, D3DDECLUSAGE aIndex)
{
    D3DVERTEXELEMENT9 endSentinel = D3DDECL_END();
    int32 i;
    int32 posOffset = -1;

    //Get position and surface normal offsets within streams
    i=0;
    while( (aInputDecl[i].Stream != endSentinel.Stream) && (i < MAX_FVF_DECL_SIZE) )
    {
        if( (aInputDecl[i].Usage == D3DDECLUSAGE_POSITION) && (aInputDecl[i].UsageIndex == 0) )
        {
            //offset in bytes
            posOffset = aInputDecl[i].Offset;
            return posOffset;
        }

        i++;
    }

    //If position offset or surface normal offset non existant give error
    if( posOffset == -1 )
    {
        DXUTOutputDebugString( L"Warning: vertex element with given semantic not found in input declaration. \n" );        
        
    }

    return posOffset;
}


//--------------------------------------------------------------------------------------
//constructor
//--------------------------------------------------------------------------------------
CGeom::CGeom()
{
    m_bInit = FALSE;
    m_pDevice = NULL;
    m_uNumVertices = 0;
    m_uNumIndices = 0;
    m_uNumOutlineIndices = 0;

    m_pMesh = NULL;
    m_pibIndices = NULL;
    m_pibOutlineIndices = NULL;
    m_pvbVertices = NULL;

    m_GeomListIdx = sg_NumGeom;
    sg_GeomPtrList[sg_NumGeom]= this;
    sg_NumGeom++;
}


//--------------------------------------------------------------------------------------
//destructor
//--------------------------------------------------------------------------------------
CGeom::~CGeom()
{
    int32 i;

    //release old geom data
    SafeRelease();

    //compact list to remove entry
    for(i=m_GeomListIdx; i<sg_NumGeom-1; i++)
    {
        sg_GeomPtrList[i] = sg_GeomPtrList[i+1]; 
    }

    //decrease number of list entries to account for removed element
    sg_NumGeom--;
}


//--------------------------------------------------------------------------------------
//initialize with 
//--------------------------------------------------------------------------------------
HRESULT CGeom::Init(IDirect3DDevice9 *a_pDevice)
{
    m_pDevice = a_pDevice;
    return S_OK;
}

//--------------------------------------------------------------------------------------
// generates a mesh of a cube with -1 to 1 range in X, Y, and Z
//
//   Outline IB is for rendering the cube in wireframe
//--------------------------------------------------------------------------------------
HRESULT CGeom::GenerateCube(void)
{
    HRESULT hr;

    //8 verts, 3 floats per vert
    float32 cubeVertices[24] = {
       1.000000, 1.000000,-1.000000,     // Vertex 0.
      -1.000000, 1.000000,-1.000000,     // Vertex 1.
      -1.000000, 1.000000, 1.000000,     // And so on.
       1.000000, 1.000000, 1.000000,
       1.000000,-1.000000,-1.000000,
      -1.000000,-1.000000,-1.000000,
      -1.000000,-1.000000, 1.000000,
       1.000000,-1.000000, 1.000000,
       };

    //12 tris, 3 idx per tri
    uint16 cubeIndices[36] = {
        0,1,2,
        0,2,3,                
        0,4,5,
        0,5,1,
        1,5,6,
        1,6,2,
        2,6,7,
        2,7,3,
        3,7,4,
        3,4,0,
        4,7,6,
        4,6,5,
    };

    //12 cube edges , 2 idx per edge
    uint16 cubeOutlineIndices[24] = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7,
    };

    //is device present??
    if(m_pDevice == NULL)
    {
        return E_FAIL;
    };

    //release old geom data
    SafeRelease();

    m_bInit = TRUE;

    m_uGeomSource = GEOM_SOURCE_MANUAL;
    m_uNumVertices = 8;
    m_uNumIndices = 36;
    m_uNumOutlineIndices = 24;

    m_uOffset = 0;
    m_uStride = sizeof(D3DXVECTOR3);
    m_FVF = D3DFVF_XYZ;

    // Create indices
    if(!m_pibIndices)
    {
        uint16 *pwIndices;
        if (FAILED (hr = m_pDevice->CreateIndexBuffer(m_uNumIndices * sizeof(uint16), D3DUSAGE_WRITEONLY, 
            D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pibIndices, NULL)))
        {
            return hr;
        }

        if (FAILED (hr = m_pibIndices->Lock(0, m_uNumIndices * sizeof(uint16), (void**) &pwIndices, 0)))
        {
            return hr;
        }

        memcpy(pwIndices, cubeIndices, m_uNumIndices * sizeof(uint16));
        m_pibIndices->Unlock();
    }

    // Create outline indices
    if(!m_pibOutlineIndices)
    {
        uint16 *pwOutlineIndices;
        if (FAILED (hr = m_pDevice->CreateIndexBuffer(m_uNumOutlineIndices * sizeof(uint16), D3DUSAGE_WRITEONLY, 
            D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pibOutlineIndices, NULL)))
        {
            return hr;
        }

        if (FAILED (hr = m_pibOutlineIndices->Lock(0, m_uNumOutlineIndices * sizeof(uint16), (void**) &pwOutlineIndices, 0)))
        {
            return hr;
        }

        memcpy(pwOutlineIndices, cubeOutlineIndices, m_uNumOutlineIndices * sizeof(uint16));
        m_pibOutlineIndices->Unlock();
    }

    
    // Create vertices
    if (!m_pvbVertices)
    {
        D3DXVECTOR3 *pVertices;

        if (FAILED (hr = m_pDevice->CreateVertexBuffer(m_uNumVertices * sizeof(D3DXVECTOR3), D3DUSAGE_WRITEONLY, 
          D3DFVF_XYZ, D3DPOOL_MANAGED, &m_pvbVertices, NULL)))
        {
            return hr;
        }

        if (FAILED (hr = m_pvbVertices->Lock(0, m_uNumVertices * sizeof(D3DXVECTOR3), (void**) &pVertices, 0)))
        {
          return hr;
        }

        memcpy(pVertices, cubeVertices, m_uNumVertices * sizeof(D3DXVECTOR3) );
        m_pvbVertices->Unlock();
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// generates a quad -1 to 1 range in X, Y at depth of 0.0
//
//   Outline IB is for rendering the outline of the quad.
//--------------------------------------------------------------------------------------
HRESULT CGeom::GenerateQuad(void)
{
    HRESULT hr;

    //release old geom data
    SafeRelease();

    //8 verts, 3 floats per vert
    float32 vertices[12] = {
       1.000000, 1.000000, 0.0,     
      -1.000000, 1.000000, 0.0, 
      -1.000000,-1.000000, 0.0,
       1.000000,-1.000000, 0.0
       };

    //12 tris, 3 idx per tri
    uint16 indices[6] = {
        0,1,2,
        0,2,3                
    };

    //12 cube edges , 2 idx per edge
    uint16 outlineIndices[8] = {
        0,1, 1,2, 2,3, 3,0
    };

    //is device present??
    if(m_pDevice == NULL)
    {
        return E_FAIL;
    };

    m_bInit = TRUE;

    m_uGeomSource = GEOM_SOURCE_MANUAL;
    m_uNumVertices = 4;
    m_uNumIndices = 6;
    m_uNumOutlineIndices = 8;

    m_uOffset = 0;
    m_uStride = sizeof(D3DXVECTOR3);
    m_FVF = D3DFVF_XYZ;

    // Create indices
    if(!m_pibIndices)
    {
        uint16 *pwIndices;
        if (FAILED (hr = m_pDevice->CreateIndexBuffer(m_uNumIndices * sizeof(uint16), D3DUSAGE_WRITEONLY, 
            D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pibIndices, NULL)))
        {
            return hr;
        }

        if (FAILED (hr = m_pibIndices->Lock(0, m_uNumIndices * sizeof(uint16), (void**) &pwIndices, 0)))
        {
            return hr;
        }

        memcpy(pwIndices, indices, m_uNumIndices * sizeof(uint16));
        m_pibIndices->Unlock();
    }

    // Create outline indices
    if(!m_pibOutlineIndices)
    {
        uint16 *pwOutlineIndices;
        if (FAILED (hr = m_pDevice->CreateIndexBuffer(m_uNumOutlineIndices * sizeof(uint16), D3DUSAGE_WRITEONLY, 
            D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pibOutlineIndices, NULL)))
        {
            return hr;
        }

        if (FAILED (hr = m_pibOutlineIndices->Lock(0, m_uNumOutlineIndices * sizeof(uint16), (void**) &pwOutlineIndices, 0)))
        {
            return hr;
        }

        memcpy(pwOutlineIndices, outlineIndices, m_uNumOutlineIndices * sizeof(uint16));
        m_pibOutlineIndices->Unlock();
    }

    
    // Create vertices
    if (!m_pvbVertices)
    {
        D3DXVECTOR3 *pVertices;

        if (FAILED (hr = m_pDevice->CreateVertexBuffer(m_uNumVertices * sizeof(D3DXVECTOR3), D3DUSAGE_WRITEONLY, 
          D3DFVF_XYZ, D3DPOOL_MANAGED, &m_pvbVertices, NULL)))
        {
            return hr;
        }

        if (FAILED (hr = m_pvbVertices->Lock(0, m_uNumVertices * sizeof(D3DXVECTOR3), (void**) &pVertices, 0)))
        {
          return hr;
        }

        memcpy(pVertices, vertices, m_uNumVertices * sizeof(D3DXVECTOR3) );
        m_pvbVertices->Unlock();
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
//GenerateSphere
//
//--------------------------------------------------------------------------------------
HRESULT CGeom::GenerateSphere(IDirect3DDevice9 *a_pDevice, float32 a_Radius, 
                              uint32 a_Slices, uint32 a_Stacks)
{
   uint32 *pIBData, *pIBWalkPtr;
   SSphereVert *pVBData;
   HRESULT hr;
   uint32 i, j, numFaces, numVerts;

   //release old geom data
   SafeRelease();

   m_uGeomSource = GEOM_SOURCE_D3DXMESH;

   //note this code need to be written since the D3DXCreateSphere function does not 
   // produce texture coordinates, nor does the geometry have the seam to transition
   // across texture coordinates

   //sphere is created using a grid of points 
   numFaces = (a_Slices) * (a_Stacks) * 2;
   numVerts = (a_Slices + 1) * (a_Stacks + 1);

   //create mesh
   hr = D3DXCreateMesh( 
      numFaces,      //number of faces 
      numVerts,      //number of vertices
      D3DXMESH_32BIT | D3DXMESH_MANAGED, 
      SSphereVert::Decl, 
      m_pDevice, 
      &m_pMesh );

   //lock VB and IB
   m_pMesh->LockVertexBuffer( 0, (LPVOID*)&pVBData );
   m_pMesh->LockIndexBuffer( 0, (LPVOID*)&pIBData );

   //build index buffer (grid defined by latitude and longitude lines)
   pIBWalkPtr = pIBData;
   for(j=0; j<a_Stacks; j++)
   {
      for(i=0; i<a_Slices; i++)
      {
         int32 vertIdx;

         vertIdx = (j * (a_Slices+1)) + i;

         pIBWalkPtr[0] = vertIdx;
         pIBWalkPtr[1] = vertIdx + 1;
         pIBWalkPtr[2] = vertIdx + 1 + (a_Slices+1);
         pIBWalkPtr[3] = vertIdx;
         pIBWalkPtr[4] = vertIdx + 1 + (a_Slices+1);
         pIBWalkPtr[5] = vertIdx + (a_Slices+1);

         pIBWalkPtr += 6;
      }
   }

   m_BBoxMin = D3DXVECTOR3(VM_LARGE_FLOAT, VM_LARGE_FLOAT, VM_LARGE_FLOAT);
   m_BBoxMax = D3DXVECTOR3(-VM_LARGE_FLOAT, -VM_LARGE_FLOAT, -VM_LARGE_FLOAT);

   //build vertex buffer (grid defined by latitude and longitude lines)
   for(j=0; j<a_Stacks+1; j++)
   {
      for(i=0; i<a_Slices+1; i++)
      {
         float32 u, v;
         int32 vertIdx;

         vertIdx = (j * (a_Slices+1)) + i;
         u = (float32)i / a_Slices;   //[0, 1] range inclusive over mesh
         v = (float32)j / a_Stacks;   //[0, 1] range inclusive over mesh

         pVBData[vertIdx].m_TexCoord.x = u;
         pVBData[vertIdx].m_TexCoord.y = v;

         //longitude
         pVBData[vertIdx].m_VertexNormal.x = cosf(u * 2.0f * D3DX_PI) * sinf(v * D3DX_PI);
         pVBData[vertIdx].m_VertexNormal.y = -sinf(u * 2.0f * D3DX_PI) * sinf(v * D3DX_PI);

         //latitude
         pVBData[vertIdx].m_VertexNormal.z = cos(v * D3DX_PI);
         pVBData[vertIdx].m_Position = pVBData[vertIdx].m_VertexNormal * a_Radius;

         //compute bbox
         m_BBoxMin.x = VM_MIN(pVBData[vertIdx].m_Position.x, m_BBoxMin.x);   
         m_BBoxMin.y = VM_MIN(pVBData[vertIdx].m_Position.y, m_BBoxMin.y);   
         m_BBoxMin.z = VM_MIN(pVBData[vertIdx].m_Position.z, m_BBoxMin.z);   
         m_BBoxMax.x = VM_MAX(pVBData[vertIdx].m_Position.x, m_BBoxMax.x);   
         m_BBoxMax.y = VM_MAX(pVBData[vertIdx].m_Position.y, m_BBoxMax.y);   
         m_BBoxMax.z = VM_MAX(pVBData[vertIdx].m_Position.z, m_BBoxMax.z);   

      }
   }

   //center point
   m_CenterPoint = (m_BBoxMin + m_BBoxMax) * 0.5f;

   m_pMesh->UnlockVertexBuffer();
   m_pMesh->UnlockIndexBuffer();

   return S_OK;
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
HRESULT CGeom::LoadMesh(IDirect3DDevice9 *a_pDevice, WCHAR *a_Filename)
{
    HRESULT hr;
    WCHAR *prefix;

    //find prefix
    m_pDevice = a_pDevice;
    prefix = wcsrchr(a_Filename, L'.');

    if(wcscmp(prefix, L".x") == 0)
    {
        V_RETURN(LoadDotX(a_Filename)); 
    }
    else if (wcscmp(prefix, L".obj") == 0)
    {
        V_RETURN(LoadObj(a_Filename));
    }

    return E_FAIL;
}


//--------------------------------------------------------------------------------------
//
//
//--------------------------------------------------------------------------------------
HRESULT CGeom::LoadDotX(WCHAR *a_Filename)
{
   HRESULT hr;
   WCHAR str[MAX_PATH];
   int32 j;

   //release old geom data
   SafeRelease();

   m_uGeomSource = GEOM_SOURCE_D3DXMESH;
   m_pMesh = NULL;

   // Load the mesh with D3DX and get back a ID3DXMesh*.  For this
   // sample we'll ignore the X file's embedded materials since we know 
   // exactly the model we're loading.  See the mesh samples such as
   // "OptimizedMesh" for a more generic mesh loading example.
   V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, a_Filename ) );
   V_RETURN( D3DXLoadMeshFromX(str, D3DXMESH_MANAGED, m_pDevice, NULL, NULL, NULL, NULL, &m_pMesh) );

   DWORD *rgdwAdjacency = NULL;

   // Make sure there are normals which are required for lighting
   if( !(m_pMesh->GetFVF() & D3DFVF_NORMAL) )
   {
       ID3DXMesh* pTempMesh;
       V( m_pMesh->CloneMeshFVF( m_pMesh->GetOptions(), 
                                 m_pMesh->GetFVF() | D3DFVF_NORMAL, 
                                 m_pDevice, &pTempMesh ) );
       V( D3DXComputeNormals( pTempMesh, NULL ) );

       SAFE_RELEASE( m_pMesh );
       m_pMesh = pTempMesh;
   }

   //bbox for object
   float32 *vBuffer;

   m_pMesh->LockVertexBuffer(D3DLOCK_READONLY, (LPVOID *)&vBuffer);

   m_BBoxMin = D3DXVECTOR3(VM_LARGE_FLOAT, VM_LARGE_FLOAT, VM_LARGE_FLOAT);
   m_BBoxMax = D3DXVECTOR3(-VM_LARGE_FLOAT, -VM_LARGE_FLOAT, -VM_LARGE_FLOAT);

   for(j=0; j<(int32)m_pMesh->GetNumVertices(); j++)
   {
      //note position is first element in FVF vertex format

      //compute center point
      m_BBoxMin.x = VM_MIN(vBuffer[0], m_BBoxMin.x);   
      m_BBoxMin.y = VM_MIN(vBuffer[1], m_BBoxMin.y);   
      m_BBoxMin.z = VM_MIN(vBuffer[2], m_BBoxMin.z);   
      m_BBoxMax.x = VM_MAX(vBuffer[0], m_BBoxMax.x);   
      m_BBoxMax.y = VM_MAX(vBuffer[1], m_BBoxMax.y);   
      m_BBoxMax.z = VM_MAX(vBuffer[2], m_BBoxMax.z);   

      //advance pointer to next vertex (float32 pointer takes 4 byte steps hence the divide by 4)
      vBuffer += (m_pMesh->GetNumBytesPerVertex() / 4 );
   }

   m_pMesh->UnlockVertexBuffer();
   
   //center point
   m_CenterPoint = (m_BBoxMin + m_BBoxMax) * 0.5f;

 

   return S_OK;
}


//--------------------------------------------------------------------------------------
// save the cuurent geom object as a DotX file.
//
//--------------------------------------------------------------------------------------
HRESULT CGeom::SaveDotX(WCHAR *a_Filename)
{
    HRESULT hr;

    //for later, convert VB/IB raw format to mesh, and then save off the mesh in the 
    //  case of GEOM_SOURCE_MANUAL
    if(m_uGeomSource == GEOM_SOURCE_D3DXMESH)
    {
        hr = D3DXSaveMeshToX( a_Filename, m_pMesh, NULL, NULL, NULL, 0, D3DXF_FILEFORMAT_TEXT );
    }

    return hr;
}


//--------------------------------------------------------------------------------------
// This function loads the mesh and ensures the mesh has normals; it also optimizes the 
// mesh for the graphics card's vertex cache, which improves performance by organizing 
// the internal triangle list for less cache misses.
//--------------------------------------------------------------------------------------
HRESULT CGeom::LoadObj(WCHAR *a_Filename)
{
   char8 filename[MAX_PATH];
   HRESULT      hr;
   ObjReader    objData;
   ID3DXMesh    *pNewMesh;
   FROMOBJVERT  *pNewVBData = NULL;
   DWORD        *pNewIBData = NULL;
   int32 i, j;
   size_t numBytesConverted = 0;

   //release old geom data
   SafeRelease();

   wcstombs_s(&numBytesConverted, filename, MAX_PATH * sizeof(char8), a_Filename, wcslen(a_Filename));

   //add end of string
   filename[wcslen(a_Filename)] = 0;

   if(objData.LoadObj(filename) != TRUE)
   {
      return E_FAIL;
   }

   //create mesh
   hr = D3DXCreateMesh( 
      objData.mNumIndex / 3,   //number of faces 
      objData.mNumVertex,      //number of vertices
      D3DXMESH_32BIT | D3DXMESH_MANAGED, 
      FROMOBJVERT::Decl, 
      m_pDevice, 
      &pNewMesh );

   //lock VB and IB
   pNewMesh->LockVertexBuffer( 0, (LPVOID*)&pNewVBData );
   pNewMesh->LockIndexBuffer( 0, (LPVOID*)&pNewIBData );

   //copy data to mesh
   for(i=0; i<(int32)objData.mNumIndex; i++)
   {
      pNewIBData[i] = objData.mIndex[i];
   }

   //bbox for object
   m_BBoxMin = D3DXVECTOR3(VM_LARGE_FLOAT, VM_LARGE_FLOAT, VM_LARGE_FLOAT);
   m_BBoxMax = D3DXVECTOR3(-VM_LARGE_FLOAT, -VM_LARGE_FLOAT, -VM_LARGE_FLOAT);

   for(j=0; j<(int32)objData.mNumVertex; j++)
   {
      pNewVBData[j].Position = D3DXVECTOR3(&objData.mPosition[j*3]);
      pNewVBData[j].VertexNormal = D3DXVECTOR3(&objData.mNormal[j*3]);
      pNewVBData[j].TexCoord = D3DXVECTOR2(&objData.mTexCoord[j*2]);
      pNewVBData[j].TangentU = D3DXVECTOR3(&objData.mTangentU[j*3]);
      pNewVBData[j].TangentV = D3DXVECTOR3(&objData.mTangentV[j*3]);

      //compute center point
      m_BBoxMin.x = VM_MIN(pNewVBData[j].Position.x, m_BBoxMin.x);   
      m_BBoxMin.y = VM_MIN(pNewVBData[j].Position.y, m_BBoxMin.y);   
      m_BBoxMin.z = VM_MIN(pNewVBData[j].Position.z, m_BBoxMin.z);   
      m_BBoxMax.x = VM_MAX(pNewVBData[j].Position.x, m_BBoxMax.x);   
      m_BBoxMax.y = VM_MAX(pNewVBData[j].Position.y, m_BBoxMax.y);   
      m_BBoxMax.z = VM_MAX(pNewVBData[j].Position.z, m_BBoxMax.z);   
   }
   
   //center point
   m_CenterPoint = (m_BBoxMin + m_BBoxMax) * 0.5f;
   

   //unlock VB and IB
   pNewMesh->UnlockVertexBuffer();
   pNewMesh->UnlockIndexBuffer();

   m_pMesh = pNewMesh;
   m_uGeomSource = GEOM_SOURCE_D3DXMESH;

   return S_OK;
}


//--------------------------------------------------------------------------------------
//Draw()
//
//--------------------------------------------------------------------------------------
HRESULT CGeom::Draw(void)
{
    HRESULT hr;

    switch(m_uGeomSource)
    {
        case GEOM_SOURCE_D3DXMESH:
        {
            DWORD i;
            DWORD numAttrib;

            m_pMesh->GetAttributeTable(NULL, &numAttrib);

            //always draw subset #0
            m_pMesh->DrawSubset(0);
            for(i=1; i<numAttrib; i++)
            {
                m_pMesh->DrawSubset(i);
            }
        }
        break;
        case GEOM_SOURCE_MANUAL:
        {        
            m_pDevice->SetFVF(m_FVF);
            m_pDevice->SetStreamSource(0, m_pvbVertices, m_uOffset, m_uStride);
            m_pDevice->SetIndices(m_pibIndices);

            if(FAILED(hr = m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 
                m_uNumVertices, 0, m_uNumIndices/3) ) )
            {
                return hr;
            }
        }
        break;
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
//draws outline as a series of line segments
//
//--------------------------------------------------------------------------------------
HRESULT CGeom::DrawOutline(void)
{
    HRESULT hr;

    if( m_uGeomSource == GEOM_SOURCE_MANUAL)
    {
        m_pDevice->SetFVF(m_FVF);
        m_pDevice->SetStreamSource(0, m_pvbVertices, m_uOffset, m_uStride);
        m_pDevice->SetIndices(m_pibOutlineIndices);

        if(FAILED(hr = m_pDevice->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, m_uNumVertices, 0, m_uNumOutlineIndices/2) ) )
        {
            return hr;
        }

    }

    return S_OK;
}




//--------------------------------------------------------------------------------------
// handle reset for POOL_DEFAULT objects 
//
//--------------------------------------------------------------------------------------
void CGeom::OnResetDevice(void)
{

}


//--------------------------------------------------------------------------------------
// handle cleanup for POOL_DEFAULT objects 
//
//--------------------------------------------------------------------------------------
void CGeom::OnLostDevice(void)
{

}

//--------------------------------------------------------------------------------------
// handle cleanup for POOL_MANAGED objects 
//
//--------------------------------------------------------------------------------------
void CGeom::OnDestroyDevice(void)
{
   SafeRelease();
}


//--------------------------------------------------------------------------------------
// safely releases mesh data
//--------------------------------------------------------------------------------------
void CGeom::SafeRelease(void)
{
    switch(m_uGeomSource)
    {
        case GEOM_SOURCE_D3DXMESH:
        {            
            SAFE_RELEASE( m_pMesh );
        }
        break;
        case GEOM_SOURCE_MANUAL:
        {
            SAFE_RELEASE( m_pibIndices );
            SAFE_RELEASE( m_pibOutlineIndices );
            SAFE_RELEASE( m_pvbVertices );
        }
        break;
    } 
}



//--------------------------------------------------------------------------------------
// handles OnResetDevice condition for all geometric objects 
//--------------------------------------------------------------------------------------
void GeomListOnResetDevice(void)
{
    int32 i;

    //call lost device function for each geometry member
    for(i=0; i<sg_NumGeom; i++)
    {
        sg_GeomPtrList[i]->OnResetDevice();
    }
}


//--------------------------------------------------------------------------------------
// handles OnLostDevice condition for all geometric objects 
//--------------------------------------------------------------------------------------
void GeomListOnLostDevice(void)
{
    int32 i;

    //call lost device function for each geometry member
    for(i=0; i<sg_NumGeom; i++)
    {
        sg_GeomPtrList[i]->OnLostDevice();
    }
}


//--------------------------------------------------------------------------------------
// handles OnDestroyDevice condition for all geometric objects 
//--------------------------------------------------------------------------------------
void GeomListOnDestroyDevice(void)
{
    int32 i;

    //call lost device function for each geometry member
    for(i=0; i<sg_NumGeom; i++)
    {
        sg_GeomPtrList[i]->OnDestroyDevice();
    }
}


//--------------------------------------------------------------------------------------
// Adapted from D3DShadowVolumeExample
//
// Takes an array of CEdgeMapping objects, then returns an index for the edge in the
// table if such entry exists, or returns an index at which a new entry for the edge
// can be written.
// nV1 and nV2 are the vertex indexes for the old edge.
// nCount is the number of elements in the array.
// The function returns -1 if an available entry cannot be found.  In reality,
// this should never happens as we should have allocated enough memory.
//--------------------------------------------------------------------------------------
int FindEdgeInMappingTable( int nV1, int nV2, CEdgeMapping *pMapping, int nCount )
{
    for( int i = 0; i < nCount; ++i )
    {
        // If both vertex indexes of the old edge in mapping entry are -1, then
        // we have searched every valid entry without finding a match.  Return
        // this index as a newly created entry.
        if( ( pMapping[i].m_anOldEdge[0] == -1 && pMapping[i].m_anOldEdge[1] == -1 ) ||

            // Or if we find a match, return the index.
            ( pMapping[i].m_anOldEdge[1] == nV1 && pMapping[i].m_anOldEdge[0] == nV2 ) )
        {
            return i;
        }
    }

    return -1;  // We should never reach this line
}


//--------------------------------------------------------------------------------------
// This function loads the mesh and ensures the mesh has normals; it also optimizes the 
// mesh for the graphics card's vertex cache, which improves performance by organizing 
// the internal triangle list for less cache misses.
//--------------------------------------------------------------------------------------
HRESULT LoadMesh( IDirect3DDevice9 *pd3dDevice, WCHAR *strFileName, ID3DXMesh **ppMesh )
{
    ID3DXMesh* pMesh = NULL;
    WCHAR str[MAX_PATH];
    HRESULT hr;

    // Load the mesh with D3DX and get back a ID3DXMesh*.  For this
    // sample we'll ignore the X file's embedded materials since we know 
    // exactly the model we're loading.  See the mesh samples such as
    // "OptimizedMesh" for a more generic mesh loading example.
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, strFileName ) );

    V_RETURN( D3DXLoadMeshFromX(str, D3DXMESH_MANAGED, pd3dDevice, NULL, NULL, NULL, NULL, &pMesh) );

    DWORD *rgdwAdjacency = NULL;

    // Make sure there are normals which are required for lighting
    if( !(pMesh->GetFVF() & D3DFVF_NORMAL) )
    {
        ID3DXMesh* pTempMesh;
        V( pMesh->CloneMeshFVF( pMesh->GetOptions(), 
                                  pMesh->GetFVF() | D3DFVF_NORMAL, 
                                  pd3dDevice, &pTempMesh ) );
        V( D3DXComputeNormals( pTempMesh, NULL ) );

        SAFE_RELEASE( pMesh );
        pMesh = pTempMesh;
    }

    // Optimize the mesh for this graphics card's vertex cache 
    // so when rendering the mesh's triangle list the vertices will 
    // cache hit more often so it won't have to re-execute the vertex shader 
    // on those vertices so it will improve perf.     
    rgdwAdjacency = new DWORD[pMesh->GetNumFaces() * 3];
    if( rgdwAdjacency == NULL )
        return E_OUTOFMEMORY;
    V( pMesh->ConvertPointRepsToAdjacency(NULL, rgdwAdjacency) );
    V( pMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE, rgdwAdjacency, NULL, NULL, NULL) );
    delete []rgdwAdjacency;

    *ppMesh = pMesh;

    return S_OK;
}





