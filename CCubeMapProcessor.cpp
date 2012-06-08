//--------------------------------------------------------------------------------------
//CCubeMapFilter
// Classes for filtering and processing cubemaps
//
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------
#include "CCubeMapProcessor.h"

#define CP_PI   3.14159265358979323846

//Filtering options for each thread
SThreadOptionsThread0 sg_FilterOptionsCPU0;

//Thread functions used to run filtering routines in the CPU0 process
// note that these functions can not be member functions since thread initiation
// must start from a global or static global function
DWORD WINAPI ThreadProcCPU0Filter(LPVOID a_NothingToPassToThisFunction)
{   
   //filter cube map mipchain
   sg_FilterOptionsCPU0.m_cmProc->FilterCubeMapMipChain(
	  sg_FilterOptionsCPU0.m_BaseFilterAngle,
	  sg_FilterOptionsCPU0.m_InitialMipAngle,
	  sg_FilterOptionsCPU0.m_MipAnglePerLevelScale,
	  sg_FilterOptionsCPU0.m_FilterType,
	  // SL BEGIN
	  // sg_FilterOptionsCPU0.m_FixupType,
	  // SL END
	  sg_FilterOptionsCPU0.m_FixupWidth,
	  sg_FilterOptionsCPU0.m_bUseSolidAngle 
	  // SL BEGIN
	  ,sg_FilterOptionsCPU0.m_MCO
	  // SL END
  );

   return(CP_THREAD_COMPLETED);
}

// SL BEGIN
DWORD WINAPI ThreadProcCPU0FilterMultithread(LPVOID a_NothingToPassToThisFunction)
{   

   //filter cube map mipchain
  sg_FilterOptionsCPU0.m_cmProc->FilterCubeMapMipChainMultithread(
	  sg_FilterOptionsCPU0.m_BaseFilterAngle,
	  sg_FilterOptionsCPU0.m_InitialMipAngle,
	  sg_FilterOptionsCPU0.m_MipAnglePerLevelScale,
	  sg_FilterOptionsCPU0.m_FilterType,
	  sg_FilterOptionsCPU0.m_FixupWidth,
	  sg_FilterOptionsCPU0.m_bUseSolidAngle,
	  sg_FilterOptionsCPU0.m_MCO
	  );

   return(CP_THREAD_COMPLETED);
}
// SL END

//Filtering options for each thread
SThreadOptionsThread1 sg_FilterOptionsCPU1;

//Thread functions used to run filtering routines in the process for CPU1
// note that these functions can not be member functions since thread initiation
// must start from a global or static global function

//This thread entry point function is called by thread 1  which is initiated
// by thread 0
DWORD WINAPI ThreadProcCPU1Filter(LPVOID a_NothingToPassToThisFunction)
{   
   //filter subset of faces 
   sg_FilterOptionsCPU1.m_cmProc->FilterCubeSurfaces(
      sg_FilterOptionsCPU1.m_SrcCubeMap, 
      sg_FilterOptionsCPU1.m_DstCubeMap, 
      sg_FilterOptionsCPU1.m_FilterConeAngle, 
      sg_FilterOptionsCPU1.m_FilterType, 
      sg_FilterOptionsCPU1.m_bUseSolidAngle, 
      sg_FilterOptionsCPU1.m_FaceIdxStart,
      sg_FilterOptionsCPU1.m_FaceIdxEnd,
      sg_FilterOptionsCPU1.m_ThreadIdx 
	  // SL BEGIN
	  ,sg_FilterOptionsCPU1.m_MCO.SpecularPower
	  ,sg_FilterOptionsCPU1.m_MCO.LightingModel
	  ,sg_FilterOptionsCPU1.m_MCO.FixupType
	  // SL END
	  ); 
      
   return(CP_THREAD_COMPLETED);
}


//------------------------------------------------------------------------------
// D3D cube map face specification
//   mapping from 3D x,y,z cube map lookup coordinates 
//   to 2D within face u,v coordinates
//
//   --------------------> U direction 
//   |                   (within-face texture space)
//   |         _____
//   |        |     |
//   |        | +Y  |
//   |   _____|_____|_____ _____
//   |  |     |     |     |     |
//   |  | -X  | +Z  | +X  | -Z  |
//   |  |_____|_____|_____|_____|
//   |        |     |
//   |        | -Y  |
//   |        |_____|
//   |
//   v   V direction
//      (within-face texture space)
//------------------------------------------------------------------------------

//Information about neighbors and how texture coorrdinates change across faces 
//  in ORDER of left, right, top, bottom (e.g. edges corresponding to u=0, 
//  u=1, v=0, v=1 in the 2D coordinate system of the particular face.
//Note this currently assumes the D3D cube face ordering and orientation
CPCubeMapNeighbor sg_CubeNgh[6][4] =
{
    //XPOS face
    {{CP_FACE_Z_POS, CP_EDGE_RIGHT },
     {CP_FACE_Z_NEG, CP_EDGE_LEFT  },
     {CP_FACE_Y_POS, CP_EDGE_RIGHT },
     {CP_FACE_Y_NEG, CP_EDGE_RIGHT }},
    //XNEG face
    {{CP_FACE_Z_NEG, CP_EDGE_RIGHT },
     {CP_FACE_Z_POS, CP_EDGE_LEFT  },
     {CP_FACE_Y_POS, CP_EDGE_LEFT  },
     {CP_FACE_Y_NEG, CP_EDGE_LEFT  }},
    //YPOS face
    {{CP_FACE_X_NEG, CP_EDGE_TOP },
     {CP_FACE_X_POS, CP_EDGE_TOP },
     {CP_FACE_Z_NEG, CP_EDGE_TOP },
     {CP_FACE_Z_POS, CP_EDGE_TOP }},
    //YNEG face
    {{CP_FACE_X_NEG, CP_EDGE_BOTTOM},
     {CP_FACE_X_POS, CP_EDGE_BOTTOM},
     {CP_FACE_Z_POS, CP_EDGE_BOTTOM},
     {CP_FACE_Z_NEG, CP_EDGE_BOTTOM}},
    //ZPOS face
    {{CP_FACE_X_NEG, CP_EDGE_RIGHT  },
     {CP_FACE_X_POS, CP_EDGE_LEFT   },
     {CP_FACE_Y_POS, CP_EDGE_BOTTOM },
     {CP_FACE_Y_NEG, CP_EDGE_TOP    }},
    //ZNEG face
    {{CP_FACE_X_POS, CP_EDGE_RIGHT  },
     {CP_FACE_X_NEG, CP_EDGE_LEFT   },
     {CP_FACE_Y_POS, CP_EDGE_TOP    },
     {CP_FACE_Y_NEG, CP_EDGE_BOTTOM }}
};


//3x2 matrices that map cube map indexing vectors in 3d 
// (after face selection and divide through by the 
//  _ABSOLUTE VALUE_ of the max coord)
// into NVC space
//Note this currently assumes the D3D cube face ordering and orientation
#define CP_UDIR     0
#define CP_VDIR     1
#define CP_FACEAXIS 2

float32 sgFace2DMapping[6][3][3] = {
    //XPOS face
    {{ 0,  0, -1},   //u towards negative Z
     { 0, -1,  0},   //v towards negative Y
     {1,  0,  0}},  //pos X axis  
    //XNEG face
     {{0,  0,  1},   //u towards positive Z
      {0, -1,  0},   //v towards negative Y
      {-1,  0,  0}},  //neg X axis       
    //YPOS face
    {{1, 0, 0},     //u towards positive X
     {0, 0, 1},     //v towards positive Z
     {0, 1 , 0}},   //pos Y axis  
    //YNEG face
    {{1, 0, 0},     //u towards positive X
     {0, 0 , -1},   //v towards negative Z
     {0, -1 , 0}},  //neg Y axis  
    //ZPOS face
    {{1, 0, 0},     //u towards positive X
     {0, -1, 0},    //v towards negative Y
     {0, 0,  1}},   //pos Z axis  
    //ZNEG face
    {{-1, 0, 0},    //u towards negative X
     {0, -1, 0},    //v towards negative Y
     {0, 0, -1}},   //neg Z axis  
};


//The 12 edges of the cubemap, (entries are used to index into the neighbor table)
// this table is used to average over the edges.
int32 sg_CubeEdgeList[12][2] = {
   {CP_FACE_X_POS, CP_EDGE_LEFT},
   {CP_FACE_X_POS, CP_EDGE_RIGHT},
   {CP_FACE_X_POS, CP_EDGE_TOP},
   {CP_FACE_X_POS, CP_EDGE_BOTTOM},

   {CP_FACE_X_NEG, CP_EDGE_LEFT},
   {CP_FACE_X_NEG, CP_EDGE_RIGHT},
   {CP_FACE_X_NEG, CP_EDGE_TOP},
   {CP_FACE_X_NEG, CP_EDGE_BOTTOM},

   {CP_FACE_Z_POS, CP_EDGE_TOP},
   {CP_FACE_Z_POS, CP_EDGE_BOTTOM},
   {CP_FACE_Z_NEG, CP_EDGE_TOP},
   {CP_FACE_Z_NEG, CP_EDGE_BOTTOM}
};


//Information about which of the 8 cube corners are correspond to the 
//  the 4 corners in each cube face
//  the order is upper left, upper right, lower left, lower right
int32 sg_CubeCornerList[6][4] = {
   { CP_CORNER_PPP, CP_CORNER_PPN, CP_CORNER_PNP, CP_CORNER_PNN }, // XPOS face
   { CP_CORNER_NPN, CP_CORNER_NPP, CP_CORNER_NNN, CP_CORNER_NNP }, // XNEG face
   { CP_CORNER_NPN, CP_CORNER_PPN, CP_CORNER_NPP, CP_CORNER_PPP }, // YPOS face
   { CP_CORNER_NNP, CP_CORNER_PNP, CP_CORNER_NNN, CP_CORNER_PNN }, // YNEG face
   { CP_CORNER_NPP, CP_CORNER_PPP, CP_CORNER_NNP, CP_CORNER_PNP }, // ZPOS face
   { CP_CORNER_PPN, CP_CORNER_NPN, CP_CORNER_PNN, CP_CORNER_NNN }  // ZNEG face
};


//--------------------------------------------------------------------------------------
//Error handling for cube map processor
//  Pop up dialog box, and terminate application
//--------------------------------------------------------------------------------------
void CPFatalError(WCHAR *a_Msg)
{
   MessageBoxW(NULL, a_Msg, L"Error: Application Terminating", MB_OK);
   exit(EM_FATAL_ERROR);
}

// SL BEGIN
void slerp(float32* res, float32* a, float32* b, float32 t)
{
	float32 angle = acosf(VM_DOTPROD3(a, b));

	if (0.0f == angle)
	{
		res[0] = a[0];
		res[1] = a[1];
		res[2] = a[2];
	}
	else if (CP_PI == angle)
	{
		// Can't recovert!
		res[0] = 0;
		res[1] = 0;
		res[2] = 0;
	}
	else
	{
		res[0] = (sinf((1.0-t)*angle)/sinf(angle))*a[0] + (sinf(t*angle)/sinf(angle))*b[0];
		res[1] = (sinf((1.0-t)*angle)/sinf(angle))*a[1] + (sinf(t*angle)/sinf(angle))*b[1];
		res[2] = (sinf((1.0-t)*angle)/sinf(angle))*a[2] + (sinf(t*angle)/sinf(angle))*b[2];
	}
}

#define LERP(A, B, FACTOR) ( (A) + (FACTOR)*((B) - (A)) )	
// SL END

//--------------------------------------------------------------------------------------
// Convert cubemap face texel coordinates and face idx to 3D vector
// note the U and V coords are integer coords and range from 0 to size-1
//  this routine can be used to generate a normalizer cube map
//--------------------------------------------------------------------------------------
// SL BEGIN
void TexelCoordToVect(int32 a_FaceIdx, float32 a_U, float32 a_V, int32 a_Size, float32 *a_XYZ, int32 a_FixupType)
{
	float32 nvcU, nvcV;
	float32 tempVec[3];

	// Change from original AMD code
	// transform from [0..res - 1] to [- (1 - 1 / res) .. (1 - 1 / res)]
	// + 0.5f is for texel center addressing
	nvcU = (2.0f * ((float32)a_U + 0.5f) / (float32)a_Size ) - 1.0f;
	nvcV = (2.0f * ((float32)a_V + 0.5f) / (float32)a_Size ) - 1.0f;
   
	if (a_FixupType == CP_FIXUP_WARP && a_Size > 1)
	{
		// Code from Nvtt : http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/src/nvtt/CubeSurface.cpp
		float32 a = powf(float32(a_Size), 2.0f) / powf(float32(a_Size - 1), 3.0f);
		nvcU = a * powf(nvcU, 3) + nvcU;
		nvcV = a * powf(nvcV, 3) + nvcV;

		// Get current vector
		//generate x,y,z vector (xform 2d NVC coord to 3D vector)
		//U contribution
		VM_SCALE3(a_XYZ, sgFace2DMapping[a_FaceIdx][CP_UDIR], nvcU);    
		//V contribution
		VM_SCALE3(tempVec, sgFace2DMapping[a_FaceIdx][CP_VDIR], nvcV);
		VM_ADD3(a_XYZ, tempVec, a_XYZ);
		//add face axis
		VM_ADD3(a_XYZ, sgFace2DMapping[a_FaceIdx][CP_FACEAXIS], a_XYZ);
		//normalize vector
		VM_NORM3(a_XYZ, a_XYZ);
	}
	else if (a_FixupType == CP_FIXUP_BENT && a_Size > 1)
	{
		// Method following description of Physically based rendering slides from CEDEC2011 of TriAce

		// Get vector at edge
		float32 EdgeNormalU[3];
		float32 EdgeNormalV[3];
		float32 EdgeNormal[3];
		float32 EdgeNormalMinusOne[3];

		// Recover vector at edge
		//U contribution
		VM_SCALE3(EdgeNormalU, sgFace2DMapping[a_FaceIdx][CP_UDIR], nvcU < 0.0 ? -1.0f : 1.0f);    
		//V contribution
		VM_SCALE3(EdgeNormalV, sgFace2DMapping[a_FaceIdx][CP_VDIR], nvcV < 0.0 ? -1.0f : 1.0f);
		VM_ADD3(EdgeNormal, EdgeNormalV, EdgeNormalU);
		//add face axis
		VM_ADD3(EdgeNormal, sgFace2DMapping[a_FaceIdx][CP_FACEAXIS], EdgeNormal);
		//normalize vector
		VM_NORM3(EdgeNormal, EdgeNormal);

		// Get vector at (edge - 1)
		float32 nvcUEdgeMinus1 = (2.0f * ((float32)(nvcU < 0.0f ? 0 : a_Size-1) + 0.5f) / (float32)a_Size ) - 1.0f;
		float32 nvcVEdgeMinus1 = (2.0f * ((float32)(nvcV < 0.0f ? 0 : a_Size-1) + 0.5f) / (float32)a_Size ) - 1.0f;

		// Recover vector at (edge - 1)
		//U contribution
		VM_SCALE3(EdgeNormalU, sgFace2DMapping[a_FaceIdx][CP_UDIR], nvcUEdgeMinus1);    
		//V contribution
		VM_SCALE3(EdgeNormalV, sgFace2DMapping[a_FaceIdx][CP_VDIR], nvcVEdgeMinus1);
		VM_ADD3(EdgeNormalMinusOne, EdgeNormalV, EdgeNormalU);
		//add face axis
		VM_ADD3(EdgeNormalMinusOne, sgFace2DMapping[a_FaceIdx][CP_FACEAXIS], EdgeNormalMinusOne);
		//normalize vector
		VM_NORM3(EdgeNormalMinusOne, EdgeNormalMinusOne);

		// Get angle between the two vector (which is 50% of the two vector presented in the TriAce slide)
		float32 AngleNormalEdge = acosf(VM_DOTPROD3(EdgeNormal, EdgeNormalMinusOne));
		
		// Here we assume that high resolution required less offset than small resolution (TriAce based this on blur radius and custom value) 
		// Start to increase from 50% to 100% target angle from 128x128x6 to 1x1x6
		float32 NumLevel = (logf(min(a_Size, 128))  / logf(2)) - 1;
		AngleNormalEdge = LERP(0.5 * AngleNormalEdge, AngleNormalEdge, 1.0f - (NumLevel/6) );

		float32 factorU = abs((2.0f * ((float32)a_U) / (float32)(a_Size - 1) ) - 1.0f);
		float32 factorV = abs((2.0f * ((float32)a_V) / (float32)(a_Size - 1) ) - 1.0f);
		AngleNormalEdge = LERP(0.0f, AngleNormalEdge, max(factorU, factorV) );

		// Get current vector
		//generate x,y,z vector (xform 2d NVC coord to 3D vector)
		//U contribution
		VM_SCALE3(a_XYZ, sgFace2DMapping[a_FaceIdx][CP_UDIR], nvcU);    
		//V contribution
		VM_SCALE3(tempVec, sgFace2DMapping[a_FaceIdx][CP_VDIR], nvcV);
		VM_ADD3(a_XYZ, tempVec, a_XYZ);
		//add face axis
		VM_ADD3(a_XYZ, sgFace2DMapping[a_FaceIdx][CP_FACEAXIS], a_XYZ);
		//normalize vector
		VM_NORM3(a_XYZ, a_XYZ);

		float32 RadiantAngle = AngleNormalEdge;
		// Get angle between face normal and current normal. Used to push the normal away from face normal.
		float32 AngleFaceVector = acosf(VM_DOTPROD3(sgFace2DMapping[a_FaceIdx][CP_FACEAXIS], a_XYZ));

		// Push the normal away from face normal by an angle of RadiantAngle
		slerp(a_XYZ, sgFace2DMapping[a_FaceIdx][CP_FACEAXIS], a_XYZ, 1.0f + RadiantAngle / AngleFaceVector);
	}
	else
	{
		//generate x,y,z vector (xform 2d NVC coord to 3D vector)
		//U contribution
		VM_SCALE3(a_XYZ, sgFace2DMapping[a_FaceIdx][CP_UDIR], nvcU);    
		//V contribution
		VM_SCALE3(tempVec, sgFace2DMapping[a_FaceIdx][CP_VDIR], nvcV);
		VM_ADD3(a_XYZ, tempVec, a_XYZ);
		//add face axis
		VM_ADD3(a_XYZ, sgFace2DMapping[a_FaceIdx][CP_FACEAXIS], a_XYZ);

		//normalize vector
		VM_NORM3(a_XYZ, a_XYZ);
	}
}
// SL END

//--------------------------------------------------------------------------------------
// Convert 3D vector to cubemap face texel coordinates and face idx 
// note the U and V coords are integer coords and range from 0 to size-1
//  this routine can be used to generate a normalizer cube map
//
// returns face IDX and texel coords
//--------------------------------------------------------------------------------------
// SL BEGIN
/*
Mapping Texture Coordinates to Cube Map Faces
Because there are multiple faces, the mapping of texture coordinates to positions on cube map faces
is more complicated than the other texturing targets.  The EXT_texture_cube_map extension is purposefully
designed to be consistent with DirectX 7's cube map arrangement.  This is also consistent with the cube
map arrangement in Pixar's RenderMan package. 
For cube map texturing, the (s,t,r) texture coordinates are treated as a direction vector (rx,ry,rz)
emanating from the center of a cube.  (The q coordinate can be ignored since it merely scales the vector
without affecting the direction.) At texture application time, the interpolated per-fragment (s,t,r)
selects one of the cube map face's 2D mipmap sets based on the largest magnitude coordinate direction 
the major axis direction). The target column in the table below explains how the major axis direction
maps to the 2D image of a particular cube map target. 

major axis 
direction     target                              sc     tc    ma 
----------    ---------------------------------   ---    ---   --- 
+rx          GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT   -rz    -ry   rx 
-rx          GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT   +rz    -ry   rx 
+ry          GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT   +rx    +rz   ry 
-ry          GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT   +rx    -rz   ry 
+rz          GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT   +rx    -ry   rz 
-rz          GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT   -rx    -ry   rz

Using the sc, tc, and ma determined by the major axis direction as specified in the table above,
an updated (s,t) is calculated as follows 
s   =   ( sc/|ma| + 1 ) / 2 
t   =   ( tc/|ma| + 1 ) / 2
If |ma| is zero or very nearly zero, the results of the above two equations need not be defined
(though the result may not lead to GL interruption or termination).  Once the cube map face's 2D mipmap
set and (s,t) is determined, texture fetching and filtering proceeds like standard OpenGL 2D texturing. 
*/
// Note this method return U and V in range from 0 to size-1
// SL END
void VectToTexelCoord(float32 *a_XYZ, int32 a_Size, int32 *a_FaceIdx, int32 *a_U, int32 *a_V )
{
   float32 nvcU, nvcV;
   float32 absXYZ[3];
   float32 maxCoord;
   float32 onFaceXYZ[3];
   int32   faceIdx;
   int32   u, v;

   //absolute value 3
   VM_ABS3(absXYZ, a_XYZ);

   if( (absXYZ[0] >= absXYZ[1]) && (absXYZ[0] >= absXYZ[2]) )
   {
      maxCoord = absXYZ[0];

      if(a_XYZ[0] >= 0) //face = XPOS
      {
         faceIdx = CP_FACE_X_POS;            
      }    
      else
      {
         faceIdx = CP_FACE_X_NEG;                    
      }
   }
   else if ( (absXYZ[1] >= absXYZ[0]) && (absXYZ[1] >= absXYZ[2]) )
   {
      maxCoord = absXYZ[1];

      if(a_XYZ[1] >= 0) //face = XPOS
      {
         faceIdx = CP_FACE_Y_POS;            
      }    
      else
      {
         faceIdx = CP_FACE_Y_NEG;                    
      }    
   }
   else  // if( (absXYZ[2] > absXYZ[0]) && (absXYZ[2] > absXYZ[1]) )
   {
      maxCoord = absXYZ[2];

      if(a_XYZ[2] >= 0) //face = XPOS
      {
         faceIdx = CP_FACE_Z_POS;            
      }    
      else
      {
         faceIdx = CP_FACE_Z_NEG;                    
      }    
   }

   //divide through by max coord so face vector lies on cube face
   VM_SCALE3(onFaceXYZ, a_XYZ, 1.0f/maxCoord);
   nvcU = VM_DOTPROD3(sgFace2DMapping[ faceIdx ][CP_UDIR], onFaceXYZ );
   nvcV = VM_DOTPROD3(sgFace2DMapping[ faceIdx ][CP_VDIR], onFaceXYZ );

   // SL BEGIN
   // Modify original AMD code to return value from 0 to Size - 1
   u = (int32)floor( (a_Size - 1) * 0.5f * (nvcU + 1.0f) );
   v = (int32)floor( (a_Size - 1) * 0.5f * (nvcV + 1.0f) );
   // SL END

   *a_FaceIdx = faceIdx;
   *a_U = u;
   *a_V = v;
}


//--------------------------------------------------------------------------------------
// gets texel ptr in a cube map given a direction vector, and an array of 
//  CImageSurfaces that represent the cube faces.
//   
//--------------------------------------------------------------------------------------
CP_ITYPE *GetCubeMapTexelPtr(float32 *a_XYZ, CImageSurface *a_Surface)
{
   int32 u, v, faceIdx;    

   //get face idx and u, v texel coordinate in face
   VectToTexelCoord(a_XYZ, a_Surface[0].m_Width, &faceIdx, &u, &v );

   return( a_Surface[faceIdx].GetSurfaceTexelPtr(u, v) );
}


//--------------------------------------------------------------------------------------
//  Compute solid angle of given texel in cubemap face for weighting taps in the 
//   kernel by the area they project to on the unit sphere.
//
//  Note that this code uses an approximation to the solid angle, by treating the 
//   two triangles that make up the quad comprising the texel as planar.  If more
//   accuracy is required, the solid angle per triangle lying on the sphere can be 
//   computed using the sum of the interior angles - PI.
//
//--------------------------------------------------------------------------------------
// SL BEGIN
// Replace the calcul of the solidAngle by a better approximation.
#if 0
float32 TexelCoordSolidAngle(int32 a_FaceIdx, float32 a_U, float32 a_V, int32 a_Size)
{
   float32 cornerVect[4][3];
   float64 cornerVect64[4][3];

   float32 halfTexelStep = 0.5f;  //note u, and v are in texel coords (where each texel is one unit)  
   float64 edgeVect0[3];
   float64 edgeVect1[3];
   float64 xProdVect[3];
   float64 texelArea;

   //compute 4 corner vectors of texel
   TexelCoordToVect(a_FaceIdx, a_U - halfTexelStep, a_V - halfTexelStep, a_Size, cornerVect[0] );
   TexelCoordToVect(a_FaceIdx, a_U - halfTexelStep, a_V + halfTexelStep, a_Size, cornerVect[1] );
   TexelCoordToVect(a_FaceIdx, a_U + halfTexelStep, a_V - halfTexelStep, a_Size, cornerVect[2] );
   TexelCoordToVect(a_FaceIdx, a_U + halfTexelStep, a_V + halfTexelStep, a_Size, cornerVect[3] );
   
   VM_NORM3_UNTYPED(cornerVect64[0], cornerVect[0] );
   VM_NORM3_UNTYPED(cornerVect64[1], cornerVect[1] );
   VM_NORM3_UNTYPED(cornerVect64[2], cornerVect[2] );
   VM_NORM3_UNTYPED(cornerVect64[3], cornerVect[3] );

   //area of triangle defined by corners 0, 1, and 2
   VM_SUB3_UNTYPED(edgeVect0, cornerVect64[1], cornerVect64[0] );
   VM_SUB3_UNTYPED(edgeVect1, cornerVect64[2], cornerVect64[0] );    
   VM_XPROD3_UNTYPED(xProdVect, edgeVect0, edgeVect1 );
   texelArea = 0.5f * sqrt( VM_DOTPROD3_UNTYPED(xProdVect, xProdVect ) );

   //area of triangle defined by corners 1, 2, and 3
   VM_SUB3_UNTYPED(edgeVect0, cornerVect64[2], cornerVect64[1] );
   VM_SUB3_UNTYPED(edgeVect1, cornerVect64[3], cornerVect64[1] );
   VM_XPROD3_UNTYPED(xProdVect, edgeVect0, edgeVect1 );
   texelArea += 0.5f * sqrt( VM_DOTPROD3_UNTYPED(xProdVect, xProdVect ) );

   return texelArea;
}
#else

/** Original code from Ignacio Castaño
* This formula is from Manne Öhrström's thesis.
* Take two coordiantes in the range [-1, 1] that define a portion of a
* cube face and return the area of the projection of that portion on the
* surface of the sphere.
**/

static float32 AreaElement( float32 x, float32 y )
{
	return atan2(x * y, sqrt(x * x + y * y + 1));
}

float32 TexelCoordSolidAngle(int32 a_FaceIdx, float32 a_U, float32 a_V, int32 a_Size)
{
   // transform from [0..res - 1] to [- (1 - 1 / res) .. (1 - 1 / res)]
   // (+ 0.5f is for texel center addressing)
   float32 U = (2.0f * ((float32)a_U + 0.5f) / (float32)a_Size ) - 1.0f;
   float32 V = (2.0f * ((float32)a_V + 0.5f) / (float32)a_Size ) - 1.0f;

   // Shift from a demi texel, mean 1.0f / a_Size with U and V in [-1..1]
   float32 InvResolution = 1.0f / a_Size;

	// U and V are the -1..1 texture coordinate on the current face.
	// Get projected area for this texel
	float32 x0 = U - InvResolution;
	float32 y0 = V - InvResolution;
	float32 x1 = U + InvResolution;
	float32 y1 = V + InvResolution;
	float32 SolidAngle = AreaElement(x0, y0) - AreaElement(x0, y1) - AreaElement(x1, y0) + AreaElement(x1, y1);

	return SolidAngle;
}
#endif
// SL END


//--------------------------------------------------------------------------------------
//Builds a normalizer cubemap
//
// Takes in a cube face size, and an array of 6 surfaces to write the cube faces into
//
// Note that this normalizer cube map stores the vectors in unbiased -1 to 1 range.
//  if _bx2 style scaled and biased vectors are needed, uncomment the SCALE and BIAS
//  below
//--------------------------------------------------------------------------------------
// SL BEGIN
void CCubeMapProcessor::BuildNormalizerCubemap(int32 a_Size, CImageSurface *a_Surface, int32 a_FixupType)
// SL END
{
   int32 iCubeFace, u, v;

   //iterate over cube faces
   for(iCubeFace=0; iCubeFace<6; iCubeFace++)
   {
      a_Surface[iCubeFace].Clear();
      a_Surface[iCubeFace].Init(a_Size, a_Size, 3);

      //fast texture walk, build normalizer cube map
      CP_ITYPE *texelPtr = a_Surface[iCubeFace].m_ImgData;

      for(v=0; v < a_Surface[iCubeFace].m_Height; v++)
      {
         for(u=0; u < a_Surface[iCubeFace].m_Width; u++)
         {
			 // SL_BEGIN
            TexelCoordToVect(iCubeFace, (float32)u, (float32)v, a_Size, texelPtr, a_FixupType);
			// SL END

            //VM_SCALE3(texelPtr, texelPtr, 0.5f);
            //VM_BIAS3(texelPtr, texelPtr, 0.5f);

            texelPtr += a_Surface[iCubeFace].m_NumChannels;
         }         
      }
   }
}


//--------------------------------------------------------------------------------------
//Builds a normalizer cubemap, with the texels solid angle stored in the fourth component
//
//Takes in a cube face size, and an array of 6 surfaces to write the cube faces into
//
//Note that this normalizer cube map stores the vectors in unbiased -1 to 1 range.
// if _bx2 style scaled and biased vectors are needed, uncomment the SCALE and BIAS
// below
//--------------------------------------------------------------------------------------
// SL BEGIN
void CCubeMapProcessor::BuildNormalizerSolidAngleCubemap(int32 a_Size, CImageSurface *a_Surface, int32 a_FixupType)
// SL END
{
   int32 iCubeFace, u, v;

   //iterate over cube faces
   for(iCubeFace=0; iCubeFace<6; iCubeFace++)
   {
      a_Surface[iCubeFace].Clear();
      a_Surface[iCubeFace].Init(a_Size, a_Size, 4);  //First three channels for norm cube, and last channel for solid angle

      //fast texture walk, build normalizer cube map
      CP_ITYPE *texelPtr = a_Surface[iCubeFace].m_ImgData;

      for(v=0; v<a_Surface[iCubeFace].m_Height; v++)
      {
         for(u=0; u<a_Surface[iCubeFace].m_Width; u++)
         {
			// SL_BEGIN
            TexelCoordToVect(iCubeFace, (float32)u, (float32)v, a_Size, texelPtr, a_FixupType);
			// SL END
            //VM_SCALE3(texelPtr, texelPtr, 0.5f);
            //VM_BIAS3(texelPtr, texelPtr, 0.5f);

            *(texelPtr + 3) = TexelCoordSolidAngle(iCubeFace, (float32)u, (float32)v, a_Size);

            texelPtr += a_Surface[iCubeFace].m_NumChannels;
         }         
      }
   }
}


//--------------------------------------------------------------------------------------
//Clear filter extents for the 6 cube map faces
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::ClearFilterExtents(CBBoxInt32 *aFilterExtents)
{
   int32 iCubeFaces;

   for(iCubeFaces=0; iCubeFaces<6; iCubeFaces++)
   {
      aFilterExtents[iCubeFaces].Clear();    
   }
}


//--------------------------------------------------------------------------------------
//Define per-face bounding box filter extents
//
// These define conservative texel regions in each of the faces the filter can possibly 
// process.  When the pixels in the regions are actually processed, the dot product  
// between the tap vector and the center tap vector is used to determine the weight of 
// the tap and whether or not the tap is within the cone.
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::DetermineFilterExtents(float32 *a_CenterTapDir, int32 a_SrcSize, int32 a_BBoxSize, 
                                               CBBoxInt32 *a_FilterExtents )
{
   int32 u, v;
   int32 faceIdx;
   int32 minU, minV, maxU, maxV;
   int32 i;

   //neighboring face and bleed over amount, and width of BBOX for
   // left, right, top, and bottom edges of this face
   int32 bleedOverAmount[4];
   int32 bleedOverBBoxMin[4];
   int32 bleedOverBBoxMax[4];

   int32 neighborFace;
   int32 neighborEdge;

   //get face idx, and u, v info from center tap dir
   VectToTexelCoord(a_CenterTapDir, a_SrcSize, &faceIdx, &u, &v );

   //define bbox size within face
   a_FilterExtents[faceIdx].Augment(u - a_BBoxSize, v - a_BBoxSize, 0);
   a_FilterExtents[faceIdx].Augment(u + a_BBoxSize, v + a_BBoxSize, 0);

   a_FilterExtents[faceIdx].ClampMin(0, 0, 0);
   a_FilterExtents[faceIdx].ClampMax(a_SrcSize-1, a_SrcSize-1, 0);

   //u and v extent in face corresponding to center tap
   minU = a_FilterExtents[faceIdx].m_minCoord[0];
   minV = a_FilterExtents[faceIdx].m_minCoord[1];
   maxU = a_FilterExtents[faceIdx].m_maxCoord[0];
   maxV = a_FilterExtents[faceIdx].m_maxCoord[1];

   //bleed over amounts for face across u=0 edge (left)    
   bleedOverAmount[0] = (a_BBoxSize - u);
   bleedOverBBoxMin[0] = minV;
   bleedOverBBoxMax[0] = maxV;

   //bleed over amounts for face across u=1 edge (right)    
   bleedOverAmount[1] = (u + a_BBoxSize) - (a_SrcSize-1);
   bleedOverBBoxMin[1] = minV;
   bleedOverBBoxMax[1] = maxV;

   //bleed over to face across v=0 edge (up)
   bleedOverAmount[2] = (a_BBoxSize - v);
   bleedOverBBoxMin[2] = minU;
   bleedOverBBoxMax[2] = maxU;

   //bleed over to face across v=1 edge (down)
   bleedOverAmount[3] = (v + a_BBoxSize) - (a_SrcSize-1);
   bleedOverBBoxMin[3] = minU;
   bleedOverBBoxMax[3] = maxU;

   //compute bleed over regions in neighboring faces
   for(i=0; i<4; i++)
   {
      if(bleedOverAmount[i] > 0)
      {
         neighborFace = sg_CubeNgh[faceIdx][i].m_Face;
         neighborEdge = sg_CubeNgh[faceIdx][i].m_Edge;

         //For certain types of edge abutments, the bleedOverBBoxMin, and bleedOverBBoxMax need to 
         //  be flipped: the cases are 
         // if a left   edge mates with a left or bottom  edge on the neighbor
         // if a top    edge mates with a top or right edge on the neighbor
         // if a right  edge mates with a right or top edge on the neighbor
         // if a bottom edge mates with a bottom or left  edge on the neighbor
         //Seeing as the edges are enumerated as follows 
         // left   =0 
         // right  =1 
         // top    =2 
         // bottom =3            
         // 
         // so if the edge enums are the same, or the sum of the enums == 3, 
         //  the bbox needs to be flipped
         if( (i == neighborEdge) || ((i+neighborEdge) == 3) )
         {
            bleedOverBBoxMin[i] = (a_SrcSize-1) - bleedOverBBoxMin[i];
            bleedOverBBoxMax[i] = (a_SrcSize-1) - bleedOverBBoxMax[i];
         }


         //The way the bounding box is extended onto the neighboring face
         // depends on which edge of neighboring face abuts with this one
         switch(sg_CubeNgh[faceIdx][i].m_Edge)
         {
            case CP_EDGE_LEFT:
               a_FilterExtents[neighborFace].Augment(0, bleedOverBBoxMin[i], 0);
               a_FilterExtents[neighborFace].Augment(bleedOverAmount[i], bleedOverBBoxMax[i], 0);
            break;
            case CP_EDGE_RIGHT:                
               a_FilterExtents[neighborFace].Augment( (a_SrcSize-1), bleedOverBBoxMin[i], 0);
               a_FilterExtents[neighborFace].Augment( (a_SrcSize-1) - bleedOverAmount[i], bleedOverBBoxMax[i], 0);
            break;
            case CP_EDGE_TOP:   
               a_FilterExtents[neighborFace].Augment(bleedOverBBoxMin[i], 0, 0);
               a_FilterExtents[neighborFace].Augment(bleedOverBBoxMax[i], bleedOverAmount[i], 0);
            break;
            case CP_EDGE_BOTTOM:   
               a_FilterExtents[neighborFace].Augment(bleedOverBBoxMin[i], (a_SrcSize-1), 0);
               a_FilterExtents[neighborFace].Augment(bleedOverBBoxMax[i], (a_SrcSize-1) - bleedOverAmount[i], 0);            
            break;
         }

         //clamp filter extents in non-center tap faces to remain within surface
         a_FilterExtents[neighborFace].ClampMin(0, 0, 0);
         a_FilterExtents[neighborFace].ClampMax(a_SrcSize-1, a_SrcSize-1, 0);
      }

      //If the bleed over amount bleeds past the adjacent face onto the opposite face 
      // from the center tap face, then process the opposite face entirely for now. 
      //Note that the cases in which this happens, what usually happens is that 
      // more than one edge bleeds onto the opposite face, and the bounding box 
      // encompasses the entire cube map face.
      if(bleedOverAmount[i] > a_SrcSize)
      {
         uint32 oppositeFaceIdx; 

         //determine opposite face 
         switch(faceIdx)
         {
            case CP_FACE_X_POS:
               oppositeFaceIdx = CP_FACE_X_NEG;
            break;
            case CP_FACE_X_NEG:
               oppositeFaceIdx = CP_FACE_X_POS;
            break;
            case CP_FACE_Y_POS:
               oppositeFaceIdx = CP_FACE_Y_NEG;
            break;
            case CP_FACE_Y_NEG:
               oppositeFaceIdx = CP_FACE_Y_POS;
            break;
            case CP_FACE_Z_POS:
               oppositeFaceIdx = CP_FACE_Z_NEG;
            break;
            case CP_FACE_Z_NEG:
               oppositeFaceIdx = CP_FACE_Z_POS;
            break;
            default:
            break;
         }
   
         //just encompass entire face for now
         a_FilterExtents[oppositeFaceIdx].Augment(0, 0, 0);
         a_FilterExtents[oppositeFaceIdx].Augment((a_SrcSize-1), (a_SrcSize-1), 0);            
      }
   }

   minV=minV;
}


//--------------------------------------------------------------------------------------
//ProcessFilterExtents 
//  Process bounding box in each cube face 
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::ProcessFilterExtents(float32 *a_CenterTapDir, float32 a_DotProdThresh, 
    CBBoxInt32 *a_FilterExtents, CImageSurface *a_NormCubeMap, CImageSurface *a_SrcCubeMap, 
    CP_ITYPE *a_DstVal, uint32 a_FilterType, bool8 a_bUseSolidAngleWeighting
	// SL BEGIN
	,float32 a_SpecularPower
	,int32 a_LightingModel
	// SL END
	)
{
   int32 iFaceIdx, u, v;
   int32 faceWidth;
   int32 k;

   //pointers used to walk across the image surface to accumulate taps
   CP_ITYPE *normCubeRowStartPtr;
   CP_ITYPE *srcCubeRowStartPtr;
   CP_ITYPE *texelVect;


   //accumulators are 64-bit floats in order to have the precision needed 
   // over a summation of a large number of pixels 
   float64 dstAccum[4];
   float64 weightAccum;

   CP_ITYPE tapDotProd;   //dot product between center tap and current tap

   int32 normCubePitch;
   int32 srcCubePitch;
   int32 normCubeRowWalk;
   int32 srcCubeRowWalk;

   int32 uStart, uEnd;
   int32 vStart, vEnd;

   int32 nSrcChannels; 

   nSrcChannels = a_SrcCubeMap[0].m_NumChannels;

   //norm cube map and srcCubeMap have same face width
   faceWidth = a_NormCubeMap[0].m_Width;

   //amount to add to pointer to move to next scanline in images
   normCubePitch = faceWidth * a_NormCubeMap[0].m_NumChannels;
   srcCubePitch = faceWidth * a_SrcCubeMap[0].m_NumChannels;

   //dest accum
   for(k=0; k<m_NumChannels; k++)
   {
      dstAccum[k] = 0.0f;
   }

   weightAccum = 0.0f;

   // SL BEGIN
   // Add a more efficient path (without test and switch) for cosine power,
   // Basically just a copy past.
   if (a_FilterType != CP_FILTER_TYPE_COSINE_POWER)
   {
   // SL END
   //iterate over cubefaces
   for(iFaceIdx=0; iFaceIdx<6; iFaceIdx++ )
   {
      //if bbox is non empty
      if(a_FilterExtents[iFaceIdx].Empty() == FALSE) 
      {
         uStart = a_FilterExtents[iFaceIdx].m_minCoord[0];
         vStart = a_FilterExtents[iFaceIdx].m_minCoord[1];
         uEnd = a_FilterExtents[iFaceIdx].m_maxCoord[0];
         vEnd = a_FilterExtents[iFaceIdx].m_maxCoord[1];

         normCubeRowStartPtr = a_NormCubeMap[iFaceIdx].m_ImgData + (a_NormCubeMap[iFaceIdx].m_NumChannels * 
            ((vStart * faceWidth) + uStart) );

         srcCubeRowStartPtr = a_SrcCubeMap[iFaceIdx].m_ImgData + (a_SrcCubeMap[iFaceIdx].m_NumChannels * 
            ((vStart * faceWidth) + uStart) );

         //note that <= is used to ensure filter extents always encompass at least one pixel if bbox is non empty
         for(v = vStart; v <= vEnd; v++)
         {
            normCubeRowWalk = 0;
            srcCubeRowWalk = 0;

            for(u = uStart; u <= uEnd; u++)
            {
               //pointer to direction in cube map associated with texel
               texelVect = (normCubeRowStartPtr + normCubeRowWalk);

               //check dot product to see if texel is within cone
               tapDotProd = VM_DOTPROD3(texelVect, a_CenterTapDir);

               if( tapDotProd >= a_DotProdThresh )
               {
                  CP_ITYPE weight;

                  //for now just weight all taps equally, but ideally
                  // weight should be proportional to the solid angle of the tap
                  if(a_bUseSolidAngleWeighting == TRUE)
                  {   //solid angle stored in 4th channel of normalizer/solid angle cube map
                     weight = *(texelVect+3); 
                  }
                  else
                  {   //all taps equally weighted
                     weight = 1.0f;          
                  }

                  switch(a_FilterType)
                  {
                  case CP_FILTER_TYPE_CONE:                                
                  case CP_FILTER_TYPE_ANGULAR_GAUSSIAN:
                     {
                        //weights are in same lookup table for both of these filter types
                        weight *= m_FilterLUT[(int32)(tapDotProd * (m_NumFilterLUTEntries - 1))];
                     }
                     break;
                  case CP_FILTER_TYPE_COSINE:
                     {
                        if(tapDotProd > 0.0f)
                        {
                           weight *= tapDotProd;
                        }
                        else
                        {
                           weight = 0.0f;
                        }
                     }
                     break;
                  case CP_FILTER_TYPE_DISC:
                  default:
                     break;
                  }

                  //iterate over channels
                  for(k=0; k<nSrcChannels; k++)   //(aSrcCubeMap[iFaceIdx].m_NumChannels) //up to 4 channels 
                  {
					 dstAccum[k] += weight * *(srcCubeRowStartPtr + srcCubeRowWalk);
                     srcCubeRowWalk++;                            
                  } 

                  weightAccum += weight; //accumulate weight
               }
               else
               {   
                  //step across source pixel
                  srcCubeRowWalk += nSrcChannels;                    
               }

               normCubeRowWalk += a_NormCubeMap[iFaceIdx].m_NumChannels;
            }

            normCubeRowStartPtr += normCubePitch;
            srcCubeRowStartPtr += srcCubePitch;
         }       
      }
   }
   // SL BEGIN
   }
   else // if (a_FilterType != CP_FILTER_TYPE_COSINE_POWER)
   {
   
   int32 IsPhongBRDF = (a_LightingModel == CP_LIGHTINGMODEL_PHONG_BRDF || a_LightingModel == CP_LIGHTINGMODEL_BLINN_BRDF) ? 1 : 0; // This value will be added to the specular power

   //iterate over cubefaces
   for(iFaceIdx=0; iFaceIdx<6; iFaceIdx++ )
   {
      //if bbox is non empty
      if(a_FilterExtents[iFaceIdx].Empty() == FALSE) 
      {
         uStart = a_FilterExtents[iFaceIdx].m_minCoord[0];
         vStart = a_FilterExtents[iFaceIdx].m_minCoord[1];
         uEnd = a_FilterExtents[iFaceIdx].m_maxCoord[0];
         vEnd = a_FilterExtents[iFaceIdx].m_maxCoord[1];

         normCubeRowStartPtr = a_NormCubeMap[iFaceIdx].m_ImgData + (a_NormCubeMap[iFaceIdx].m_NumChannels * 
            ((vStart * faceWidth) + uStart) );

         srcCubeRowStartPtr = a_SrcCubeMap[iFaceIdx].m_ImgData + (a_SrcCubeMap[iFaceIdx].m_NumChannels * 
            ((vStart * faceWidth) + uStart) );

         //note that <= is used to ensure filter extents always encompass at least one pixel if bbox is non empty
         for(v = vStart; v <= vEnd; v++)
         {
            normCubeRowWalk = 0;
            srcCubeRowWalk = 0;

            for(u = uStart; u <= uEnd; u++)
            {
               //pointer to direction in cube map associated with texel
               texelVect = (normCubeRowStartPtr + normCubeRowWalk);

               //check dot product to see if texel is within cone
               tapDotProd = VM_DOTPROD3(texelVect, a_CenterTapDir);

               if( tapDotProd >= a_DotProdThresh && tapDotProd > 0.0f)
               {
					CP_ITYPE weight;

					//solid angle stored in 4th channel of normalizer/solid angle cube map
					weight = *(texelVect+3); 

					// Here we decide if we use a Phong/Blinn or a Phong/Blinn BRDF.
					// Phong/Blinn BRDF is just the Phong/Blinn model multiply by the cosine of the lambert law
					// so just adding one to specularpower do the trick.					   
					weight *= pow(tapDotProd, (a_SpecularPower + (float32)IsPhongBRDF));

					//iterate over channels
					for(k=0; k<nSrcChannels; k++)   //(aSrcCubeMap[iFaceIdx].m_NumChannels) //up to 4 channels 
					{
						dstAccum[k] += weight * *(srcCubeRowStartPtr + srcCubeRowWalk);
						srcCubeRowWalk++;
					} 

					weightAccum += weight; //accumulate weight
               }
               else
               {   
                  //step across source pixel
                  srcCubeRowWalk += nSrcChannels;                    
               }

               normCubeRowWalk += a_NormCubeMap[iFaceIdx].m_NumChannels;
            }

            normCubeRowStartPtr += normCubePitch;
            srcCubeRowStartPtr += srcCubePitch;
         }       
      }
    }
   } // else // (a_FilterType != CP_FILTER_TYPE_COSINE_POWER)
   // SL END


   //divide through by weights if weight is non zero
   if(weightAccum != 0.0f)
   {
      for(k=0; k<m_NumChannels; k++)
      {
         a_DstVal[k] = (float32)(dstAccum[k] / weightAccum);
      }
   }
   else
   {   //otherwise sample nearest
      CP_ITYPE *texelPtr;

      texelPtr = GetCubeMapTexelPtr(a_CenterTapDir, a_SrcCubeMap);

      for(k=0; k<m_NumChannels; k++)
      {
         a_DstVal[k] = texelPtr[k];
      }
   }
}


//--------------------------------------------------------------------------------------
// Fixup cube edges
//
// average texels on cube map faces across the edges
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::FixupCubeEdges(CImageSurface *a_CubeMap, int32 a_FixupType, int32 a_FixupWidth)
{
   int32 i, j, k;
   int32 face;
   int32 edge;
   int32 neighborFace;
   int32 neighborEdge;

   int32 nChannels = a_CubeMap[0].m_NumChannels;
   int32 size = a_CubeMap[0].m_Width;

   CPCubeMapNeighbor neighborInfo;

   CP_ITYPE* edgeStartPtr;
   CP_ITYPE* neighborEdgeStartPtr;

   int32 edgeWalk;
   int32 neighborEdgeWalk;

   //pointer walk to walk one texel away from edge in perpendicular direction
   int32 edgePerpWalk;
   int32 neighborEdgePerpWalk;

   //number of texels inward towards cubeface center to apply fixup to
   int32 fixupDist;
   int32 iFixup;   

   // note that if functionality to filter across the three texels for each corner, then 
   CP_ITYPE *cornerPtr[8][3];      //indexed by corner and face idx
   CP_ITYPE *faceCornerPtrs[4];    //corner pointers for face
   int32 cornerNumPtrs[8];         //indexed by corner and face idx
   int32 iCorner;                  //corner iterator
   int32 iFace;                    //iterator for faces
   int32 corner;

   //if there is no fixup, or fixup width = 0, do nothing
   if((a_FixupType == CP_FIXUP_NONE) ||
      (a_FixupWidth == 0) 
	  // SL BEGIN
	  || (a_FixupType == CP_FIXUP_BENT && a_CubeMap[0].m_Width != 1) // In case of Bent Fixup and width of 1, we take the average of the texel color.
	  || (a_FixupType == CP_FIXUP_WARP && a_CubeMap[0].m_Width != 1)
	  // SL END
	  )
   {
      return;
   }

   //special case 1x1 cubemap, average face colors
   if( a_CubeMap[0].m_Width == 1 )
   {
      //iterate over channels
      for(k=0; k<nChannels; k++)
      {   
         CP_ITYPE accum = 0.0f;

         //iterate over faces to accumulate face colors
         for(iFace=0; iFace<6; iFace++)
         {
            accum += *(a_CubeMap[iFace].m_ImgData + k);
         }

         //compute average over 6 face colors
         accum /= 6.0f;

         //iterate over faces to distribute face colors
         for(iFace=0; iFace<6; iFace++)
         {
            *(a_CubeMap[iFace].m_ImgData + k) = accum;
         }
      }

      return;
   }


   //iterate over corners
   for(iCorner = 0; iCorner < 8; iCorner++ )
   {
      cornerNumPtrs[iCorner] = 0;
   }

   //iterate over faces to collect list of corner texel pointers
   for(iFace=0; iFace<6; iFace++ )
   {
      //the 4 corner pointers for this face
      faceCornerPtrs[0] = a_CubeMap[iFace].m_ImgData;
      faceCornerPtrs[1] = a_CubeMap[iFace].m_ImgData + ( (size - 1) * nChannels );
      faceCornerPtrs[2] = a_CubeMap[iFace].m_ImgData + ( (size) * (size - 1) * nChannels );
      faceCornerPtrs[3] = a_CubeMap[iFace].m_ImgData + ( (((size) * (size - 1)) + (size - 1)) * nChannels );

      //iterate over face corners to collect cube corner pointers
      for(i=0; i<4; i++ )
      {
         corner = sg_CubeCornerList[iFace][i];   
         cornerPtr[corner][ cornerNumPtrs[corner] ] = faceCornerPtrs[i];
         cornerNumPtrs[corner]++;
      }
   }


   //iterate over corners to average across corner tap values
   for(iCorner = 0; iCorner < 8; iCorner++ )
   {
      for(k=0; k<nChannels; k++)
      {             
         CP_ITYPE cornerTapAccum;

         cornerTapAccum = 0.0f;

         //iterate over corner texels and average results
         for(i=0; i<3; i++ )
         {
            cornerTapAccum += *(cornerPtr[iCorner][i] + k);
         }

         //divide by 3 to compute average of corner tap values
         cornerTapAccum *= (1.0f / 3.0f);

         //iterate over corner texels and average results
         for(i=0; i<3; i++ )
         {
            *(cornerPtr[iCorner][i] + k) = cornerTapAccum;
         }
      }
   }   


   //maximum width of fixup region is one half of the cube face size
   fixupDist = VM_MIN( a_FixupWidth, size / 2);

   //iterate over the twelve edges of the cube to average across edges
   for(i=0; i<12; i++)
   {
      face = sg_CubeEdgeList[i][0];
      edge = sg_CubeEdgeList[i][1];

      neighborInfo = sg_CubeNgh[face][edge];
      neighborFace = neighborInfo.m_Face;
      neighborEdge = neighborInfo.m_Edge;

      edgeStartPtr = a_CubeMap[face].m_ImgData;
      neighborEdgeStartPtr = a_CubeMap[neighborFace].m_ImgData;
      edgeWalk = 0;
      neighborEdgeWalk = 0;

      //amount to pointer to sample taps away from cube face
      edgePerpWalk = 0;
      neighborEdgePerpWalk = 0;

      //Determine walking pointers based on edge type
      // e.g. CP_EDGE_LEFT, CP_EDGE_RIGHT, CP_EDGE_TOP, CP_EDGE_BOTTOM
      switch(edge)
      {
         case CP_EDGE_LEFT:
            // no change to faceEdgeStartPtr  
            edgeWalk = nChannels * size;
            edgePerpWalk = nChannels;
         break;
         case CP_EDGE_RIGHT:
            edgeStartPtr += (size - 1) * nChannels;
            edgeWalk = nChannels * size;
            edgePerpWalk = -nChannels;
         break;
         case CP_EDGE_TOP:
            // no change to faceEdgeStartPtr  
            edgeWalk = nChannels;
            edgePerpWalk = nChannels * size;
         break;
         case CP_EDGE_BOTTOM:
            edgeStartPtr += (size) * (size - 1) * nChannels;
            edgeWalk = nChannels;
            edgePerpWalk = -(nChannels * size);
         break;
      }

      //For certain types of edge abutments, the neighbor edge walk needs to 
      //  be flipped: the cases are 
      // if a left   edge mates with a left or bottom  edge on the neighbor
      // if a top    edge mates with a top or right edge on the neighbor
      // if a right  edge mates with a right or top edge on the neighbor
      // if a bottom edge mates with a bottom or left  edge on the neighbor
      //Seeing as the edges are enumerated as follows 
      // left   =0 
      // right  =1 
      // top    =2 
      // bottom =3            
      // 
      //If the edge enums are the same, or the sum of the enums == 3, 
      //  the neighbor edge walk needs to be flipped
      if( (edge == neighborEdge) || ((edge + neighborEdge) == 3) )
      {   //swapped direction neighbor edge walk
         switch(neighborEdge)
         {
            case CP_EDGE_LEFT:  //start at lower left and walk up
               neighborEdgeStartPtr += (size - 1) * (size) *  nChannels;
               neighborEdgeWalk = -(nChannels * size);
               neighborEdgePerpWalk = nChannels;
            break;
            case CP_EDGE_RIGHT: //start at lower right and walk up
               neighborEdgeStartPtr += ((size - 1)*(size) + (size - 1)) * nChannels;
               neighborEdgeWalk = -(nChannels * size);
               neighborEdgePerpWalk = -nChannels;
            break;
            case CP_EDGE_TOP:   //start at upper right and walk left
               neighborEdgeStartPtr += (size - 1) * nChannels;
               neighborEdgeWalk = -nChannels;
               neighborEdgePerpWalk = (nChannels * size);
            break;
            case CP_EDGE_BOTTOM: //start at lower right and walk left
               neighborEdgeStartPtr += ((size - 1)*(size) + (size - 1)) * nChannels;
               neighborEdgeWalk = -nChannels;
               neighborEdgePerpWalk = -(nChannels * size);
            break;
         }            
      }
      else
      { //swapped direction neighbor edge walk
         switch(neighborEdge)
         {
            case CP_EDGE_LEFT: //start at upper left and walk down
               //no change to neighborEdgeStartPtr for this case since it points 
               // to the upper left corner already
               neighborEdgeWalk = nChannels * size;
               neighborEdgePerpWalk = nChannels;
            break;
            case CP_EDGE_RIGHT: //start at upper right and walk down
               neighborEdgeStartPtr += (size - 1) * nChannels;
               neighborEdgeWalk = nChannels * size;
               neighborEdgePerpWalk = -nChannels;
            break;
            case CP_EDGE_TOP:   //start at upper left and walk left
               //no change to neighborEdgeStartPtr for this case since it points 
               // to the upper left corner already
               neighborEdgeWalk = nChannels;
               neighborEdgePerpWalk = (nChannels * size);
            break;
            case CP_EDGE_BOTTOM: //start at lower left and walk left
               neighborEdgeStartPtr += (size) * (size - 1) * nChannels;
               neighborEdgeWalk = nChannels;
               neighborEdgePerpWalk = -(nChannels * size);
            break;
         }
      }


      //Perform edge walk, to average across the 12 edges and smoothly propagate change to 
      //nearby neighborhood

      //step ahead one texel on edge
      edgeStartPtr += edgeWalk;
      neighborEdgeStartPtr += neighborEdgeWalk;

      // note that this loop does not process the corner texels, since they have already been
      //  averaged across faces across earlier
      for(j=1; j<(size - 1); j++)       
      {             
         //for each set of taps along edge, average them
         // and rewrite the results into the edges
         for(k = 0; k<nChannels; k++)
         {             
            CP_ITYPE edgeTap, neighborEdgeTap, avgTap;  //edge tap, neighborEdgeTap and the average of the two
            CP_ITYPE edgeTapDev, neighborEdgeTapDev;

            edgeTap = *(edgeStartPtr + k);
            neighborEdgeTap = *(neighborEdgeStartPtr + k);

            //compute average of tap intensity values
            avgTap = 0.5f * (edgeTap + neighborEdgeTap);

            //propagate average of taps to edge taps
            (*(edgeStartPtr + k)) = avgTap;
            (*(neighborEdgeStartPtr + k)) = avgTap;

            edgeTapDev = edgeTap - avgTap;
            neighborEdgeTapDev = neighborEdgeTap - avgTap;

            //iterate over taps in direction perpendicular to edge, and 
            //  adjust intensity values gradualy to obscure change in intensity values of 
            //  edge averaging.
            for(iFixup = 1; iFixup < fixupDist; iFixup++)
            {
               //fractional amount to apply change in tap intensity along edge to taps 
               //  in a perpendicular direction to edge 
               CP_ITYPE fixupFrac = (CP_ITYPE)(fixupDist - iFixup) / (CP_ITYPE)(fixupDist); 
               CP_ITYPE fixupWeight;

               switch(a_FixupType )
               {
                  case CP_FIXUP_PULL_LINEAR:
                  {
                     fixupWeight = fixupFrac;
                  }
                  break;
                  case CP_FIXUP_PULL_HERMITE:
                  {
                     //hermite spline interpolation between 1 and 0 with both pts derivatives = 0 
                     // e.g. smooth step
                     // the full formula for hermite interpolation is:
                     //              
                     //                  [  2  -2   1   1 ][ p0 ] 
                     // [t^3  t^2  t  1 ][ -3   3  -2  -1 ][ p1 ]
                     //                  [  0   0   1   0 ][ d0 ]
                     //                  [  1   0   0   0 ][ d1 ]
                     // 
                     // Where p0 and p1 are the point locations and d0, and d1 are their respective derivatives
                     // t is the parameteric coordinate used to specify an interpoltion point on the spline
                     // and ranges from 0 to 1.
                     //  if p0 = 0 and p1 = 1, and d0 and d1 = 0, the interpolation reduces to
                     //
                     //  p(t) =  - 2t^3 + 3t^2
                     fixupWeight = ((-2.0 * fixupFrac + 3.0) * fixupFrac * fixupFrac);
                  }
                  break;
                  case CP_FIXUP_AVERAGE_LINEAR:
                  {
                     fixupWeight = fixupFrac;

                     //perform weighted average of edge tap value and current tap
                     // fade off weight linearly as a function of distance from edge
                     edgeTapDev = 
                        (*(edgeStartPtr + (iFixup * edgePerpWalk) + k)) - avgTap;
                     neighborEdgeTapDev = 
                        (*(neighborEdgeStartPtr + (iFixup * neighborEdgePerpWalk) + k)) - avgTap;
                  }
                  break;
                  case CP_FIXUP_AVERAGE_HERMITE:
                  {
                     fixupWeight = ((-2.0 * fixupFrac + 3.0) * fixupFrac * fixupFrac);

                     //perform weighted average of edge tap value and current tap
                     // fade off weight using hermite spline with distance from edge
                     //  as parametric coordinate
                     edgeTapDev = 
                        (*(edgeStartPtr + (iFixup * edgePerpWalk) + k)) - avgTap;
                     neighborEdgeTapDev = 
                        (*(neighborEdgeStartPtr + (iFixup * neighborEdgePerpWalk) + k)) - avgTap;
                  }
                  break;
               }

               // vary intensity of taps within fixup region toward edge values to hide changes made to edge taps
               *(edgeStartPtr + (iFixup * edgePerpWalk) + k) -= (fixupWeight * edgeTapDev);
               *(neighborEdgeStartPtr + (iFixup * neighborEdgePerpWalk) + k) -= (fixupWeight * neighborEdgeTapDev);
            }

         }

         edgeStartPtr += edgeWalk;
         neighborEdgeStartPtr += neighborEdgeWalk;
      }        
   }
}


//--------------------------------------------------------------------------------------
//Constructor
//--------------------------------------------------------------------------------------
CCubeMapProcessor::CCubeMapProcessor(void)
{
   // SL BEGIN
   // Get CPU info.
   SYSTEM_INFO SI;
   GetSystemInfo(&SI);
   // TODO : In case of command line, we want to use all hardware thread available
   m_NumFilterThreads  = SI.dwNumberOfProcessors - 1;	// - 1 cause the main core is used for the application.
														// If no extra hardware thread is available the filering will be done the main process.
   m_NumFilterThreads = max(min(m_NumFilterThreads, 6), 0); // We don't handle more than 6 hardware thread (6 face of cubemap) for now
   sg_ThreadFilterFace = new SThreadFilterFace[m_NumFilterThreads];

   //clear all threads
   memset(sg_ThreadFilterFace, 0, m_NumFilterThreads * sizeof(SThreadFilterFace));
   // SL END

   m_InputSize = 0;             
   m_OutputSize = 0;             
   m_NumMipLevels = 0;     
   m_NumChannels = 0; 

   m_NumFilterLUTEntries = 0;
   m_FilterLUT = NULL;

   //Constructors are automatically called for m_InputSurface and m_OutputSurface arrays
}


//--------------------------------------------------------------------------------------
//destructor
//--------------------------------------------------------------------------------------
CCubeMapProcessor::~CCubeMapProcessor()
{
    Clear();

	// SL BEGIN
	CP_SAFE_DELETE_ARRAY(sg_ThreadFilterFace);
	// SL END
}


//--------------------------------------------------------------------------------------
// Stop any currently running threads, and clear all allocated data from cube map 
//   processor.
//
// To use the cube map processor after calling Clear(....), you need to call Init(....) 
//   again
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::Clear(void)
{
   int32 i, j;

   TerminateActiveThreads();

   // SL BEGIN
   for(i=0; i<m_NumFilterThreads; i++ )
   {
      sg_ThreadFilterFace[i].m_bThreadInitialized = FALSE;
   }
   // SL END

   m_InputSize = 0;             
   m_OutputSize = 0;             
   m_NumMipLevels = 0;     
   m_NumChannels = 0; 

   //Iterate over faces for input images
   for(i=0; i<6; i++)
   {
      m_InputSurface[i].Clear();
   }

   //Iterate over mip chain, and allocate memory for mip-chain
   for(j=0; j<CP_MAX_MIPLEVELS; j++)
   {
      //Iterate over faces for output images
      for(i=0; i<6; i++)
      {
         m_OutputSurface[j][i].Clear();            
      }
   }

   m_NumFilterLUTEntries = 0;
   CP_SAFE_DELETE_ARRAY( m_FilterLUT );
}


//--------------------------------------------------------------------------------------
// Terminates execution of active threads
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::TerminateActiveThreads(void)
{
   int32 i;

   // SL BEGIN
   for(i=0; i<m_NumFilterThreads; i++)
   {
      if(sg_ThreadFilterFace[i].m_bThreadInitialized == TRUE)
      {
         if(sg_ThreadFilterFace[i].m_ThreadHandle != NULL)
         {
            TerminateThread(sg_ThreadFilterFace[i].m_ThreadHandle, CP_THREAD_TERMINATED);
            CloseHandle(sg_ThreadFilterFace[i].m_ThreadHandle);
            sg_ThreadFilterFace[i].m_ThreadHandle = NULL;
         }
      }
   }

   // Terminate main thread if needed
   TerminateThread(DumbThreadHandle, CP_THREAD_TERMINATED);
   
   m_Status = CP_STATUS_FILTER_TERMINATED;
   // SL END

}


//--------------------------------------------------------------------------------------
//Init cube map processor
//
// note that if a_MaxNumMipLevels is set to 0, the entire mip chain will be created
//  similar to the way the D3D tools allow you to specify 0 to denote an entire mip chain
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::Init(int32 a_InputSize, int32 a_OutputSize, int32 a_MaxNumMipLevels, int32 a_NumChannels)
{
    int32 i, j;
    int32 mipLevelSize;
    int32 maxNumMipLevels;

    m_Status = CP_STATUS_READY;

    //since input is being modified, terminate any active filtering threads
    TerminateActiveThreads();

    m_InputSize = a_InputSize;
    m_OutputSize = a_OutputSize;

    m_NumChannels = a_NumChannels;
        
    maxNumMipLevels = a_MaxNumMipLevels;

    //if nax num mip levels is set to 0, set it to generate the entire mip chain
    if(maxNumMipLevels == 0 )
    {
        maxNumMipLevels = CP_MAX_MIPLEVELS;
    }

    //Iterate over faces for input images
    for(i=0; i<6; i++)
    {
        m_InputSurface[i].Init(m_InputSize, m_InputSize, m_NumChannels );
    }

    //zero mip levels constructed so far
    m_NumMipLevels = 0;

    //first miplevel size 
    mipLevelSize = m_OutputSize;

    //Iterate over mip chain, and init CImageSurfaces for mip-chain
    for(j=0; j<a_MaxNumMipLevels; j++)
    {
        //Iterate over faces for output images
        for(i=0; i<6; i++)
        {
            m_OutputSurface[j][i].Init(mipLevelSize, mipLevelSize, a_NumChannels );            
        }

        //next mip level is half size
        mipLevelSize >>= 1;
        
        m_NumMipLevels++;

        //terminate if mip chain becomes too small
        if(mipLevelSize == 0)
        {            
            return;
        }
    }
}


//--------------------------------------------------------------------------------------
//Copy and convert cube map face data from an external image/surface into this object
//
// a_FaceIdx        = a value 0 to 5 speciying which face to copy into (one of the CP_FACE_? )
// a_Level          = mip level to copy into
// a_SrcType        = data type of image being copyed from (one of the CP_TYPE_? types)
// a_SrcNumChannels = number of channels of the image being copied from (usually 1 to 4)
// a_SrcPitch       = number of bytes per row of the source image being copied from
// a_SrcDataPtr     = pointer to the image data to copy from
// a_Degamma        = original gamma level of input image to undo by degamma
// a_Scale          = scale to apply to pixel values after degamma (in linear space)
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::SetInputFaceData(int32 a_FaceIdx, int32 a_SrcType, int32 a_SrcNumChannels, 
    int32 a_SrcPitch, void *a_SrcDataPtr, float32 a_MaxClamp, float32 a_Degamma, float32 a_Scale)
{
    //since input is being modified, terminate any active filtering threads
    TerminateActiveThreads();
  
    m_InputSurface[a_FaceIdx].SetImageDataClampDegammaScale( a_SrcType, a_SrcNumChannels, a_SrcPitch, 
       a_SrcDataPtr, a_MaxClamp, a_Degamma, a_Scale );
}


//--------------------------------------------------------------------------------------
//Copy and convert cube map face data from this object into an external image/surface
//
// a_FaceIdx        = a value 0 to 5 speciying which face to copy into (one of the CP_FACE_? )
// a_Level          = mip level to copy into
// a_DstType        = data type of image to copy to (one of the CP_TYPE_? types)
// a_DstNumChannels = number of channels of the image to copy to (usually 1 to 4)
// a_DstPitch       = number of bytes per row of the dest image to copy to
// a_DstDataPtr     = pointer to the image data to copy to
// a_Scale          = scale to apply to pixel values (in linear space) before gamma for output
// a_Gamma          = gamma level to apply to pixels after scaling
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::GetInputFaceData(int32 a_FaceIdx, int32 a_DstType, int32 a_DstNumChannels, 
    int32 a_DstPitch, void *a_DstDataPtr, float32 a_Scale, float32 a_Gamma)
{
    m_InputSurface[a_FaceIdx].GetImageDataScaleGamma( a_DstType, a_DstNumChannels, a_DstPitch, 
       a_DstDataPtr, a_Scale, a_Gamma );
}


//--------------------------------------------------------------------------------------
//ChannelSwapInputFaceData
//  swizzle data in first 4 channels for input faces
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::ChannelSwapInputFaceData(int32 a_Channel0Src, int32 a_Channel1Src, 
                                                 int32 a_Channel2Src, int32 a_Channel3Src )
{
   int32 iFace, u, v, k;
   int32 size;
   CP_ITYPE texelData[4];
   int32 channelSrcArray[4];

   //since input is being modified, terminate any active filtering threads
   TerminateActiveThreads();

   size = m_InputSize;

   channelSrcArray[0] = a_Channel0Src;
   channelSrcArray[1] = a_Channel1Src;
   channelSrcArray[2] = a_Channel2Src;
   channelSrcArray[3] = a_Channel3Src;

   //Iterate over faces for input images
   for(iFace=0; iFace<6; iFace++)
   {
      for(v=0; v<m_InputSize; v++ )
      {
         for(u=0; u<m_InputSize; u++ )
         {
            //get channel data
            for(k=0; k<m_NumChannels; k++)
            {
               texelData[k] = *(m_InputSurface[iFace].GetSurfaceTexelPtr(u, v) + k);
            }

            //repack channel data accoring to swizzle information
            for(k=0; k<m_NumChannels; k++)
            {
               *( m_InputSurface[iFace].GetSurfaceTexelPtr(u, v) + k) = 
                  texelData[ channelSrcArray[k] ];
            }
         }
      }
   }
}


//--------------------------------------------------------------------------------------
//ChannelSwapOutputFaceData
//  swizzle data in first 4 channels for input faces
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::ChannelSwapOutputFaceData(int32 a_Channel0Src, int32 a_Channel1Src, 
    int32 a_Channel2Src, int32 a_Channel3Src )
{
    int32 iFace, iMipLevel, u, v, k;
    int32 size;
    CP_ITYPE texelData[4];
    int32 channelSrcArray[4];

    //since output is being modified, terminate any active filtering threads
    TerminateActiveThreads();

    size = m_OutputSize;
    
    channelSrcArray[0] = a_Channel0Src;
    channelSrcArray[1] = a_Channel1Src;
    channelSrcArray[2] = a_Channel2Src;
    channelSrcArray[3] = a_Channel3Src;

    //Iterate over faces for input images
    for(iMipLevel=0; iMipLevel<m_NumMipLevels; iMipLevel++ )
    {
        for(iFace=0; iFace<6; iFace++)
        {
            for(v=0; v<m_OutputSurface[iMipLevel][iFace].m_Height; v++ )
            {
                for(u=0; u<m_OutputSurface[iMipLevel][iFace].m_Width; u++ )
                {
                    //get channel data
                    for(k=0; k<m_NumChannels; k++)
                    {
                        texelData[k] = *(m_OutputSurface[iMipLevel][iFace].GetSurfaceTexelPtr(u, v) + k);
                    }

                    //repack channel data accoring to swizzle information
                    for(k=0; k<m_NumChannels; k++)
                    {
                        *(m_OutputSurface[iMipLevel][iFace].GetSurfaceTexelPtr(u, v) + k) = texelData[ channelSrcArray[k] ];
                    }
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------
//Copy and convert cube map face data out of this class into an external image/surface
//
// a_FaceIdx        = a value 0 to 5 specifying which face to copy from (one of the CP_FACE_? )
// a_Level          = mip level to copy from
// a_DstType        = data type of image to copyed into (one of the CP_TYPE_? types)
// a_DstNumChannels = number of channels of the image to copyed into  (usually 1 to 4)
// a_DstPitch       = number of bytes per row of the source image to copyed into 
// a_DstDataPtr     = pointer to the image data to copyed into 
// a_Scale          = scale to apply to pixel values (in linear space) before gamma for output
// a_Gamma          = gamma level to apply to pixels after scaling
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::GetOutputFaceData(int32 a_FaceIdx, int32 a_Level, int32 a_DstType, 
   int32 a_DstNumChannels, int32 a_DstPitch, void *a_DstDataPtr, float32 a_Scale, float32 a_Gamma )   
{
   switch(a_DstType)
   {
      case CP_VAL_UNORM8:
      case CP_VAL_UNORM8_BGRA:
      case CP_VAL_UNORM16:
      case CP_VAL_FLOAT16:
      case CP_VAL_FLOAT32:
      {
         m_OutputSurface[a_Level][a_FaceIdx].GetImageDataScaleGamma( a_DstType, a_DstNumChannels, 
            a_DstPitch, a_DstDataPtr, a_Scale, a_Gamma ); 
      }
      break;
      default:
      break;
   }
}


//--------------------------------------------------------------------------------------
//Cube map filtering and mip chain generation.
// the cube map filtereing is specified using a number of parameters:
// Filtering per miplevel is specified using 2D cone angle (in degrees) that 
//  indicates the region of the hemisphere to filter over for each tap. 
//                
// Note that the top mip level is also a filtered version of the original input images 
//  as well in order to create mip chains for diffuse environment illumination.
// The cone angle for the top level is specified by a_BaseAngle.  This can be used to
//  generate mipchains used to store the resutls of preintegration across the hemisphere.
//
// Then the mip angle used to genreate the next level of the mip chain from the first level 
//  is a_InitialMipAngle
//
// The angle for the subsequent levels of the mip chain are specified by their parents 
//  filtering angle and a per-level scale and bias
//   newAngle = oldAngle * a_MipAnglePerLevelScale;
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::FilterCubeMapMipChain(float32 a_BaseFilterAngle, float32 a_InitialMipAngle, float32 a_MipAnglePerLevelScale, 
	// SL BEGIN
    int32 a_FilterType, /* int32 a_FixupType, */ int32 a_FixupWidth, bool8 a_bUseSolidAngle	
	, const ModifiedCubemapgenOption& a_MCO
	// SL END
	)
{
   int32 i;
   float32 coneAngle;
   SECURITY_ATTRIBUTES secAttr;

   //thread security attributes for thread1
   secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   secAttr.lpSecurityDescriptor = NULL;            //use same security descriptor as parent
   secAttr.bInheritHandle = TRUE;                  //handle is inherited by new processes spawning from this one


   //Build filter lookup tables based on the source miplevel size
   // SL BEGIN
   PrecomputeFilterLookupTables(a_FilterType, m_InputSurface[0].m_Width, a_BaseFilterAngle, a_MCO.FixupType);
   // SL END

   //initialize thread progress
   // SL BEGIN
   sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentMipLevel = 0;
   sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentRow = 0;
   sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentFace = 0;
   // SL END

   // SL BEGIN
   // If diffuse convolution is required, go through SH filtering for the base level
   if (a_MCO.bIrradianceCubemap)
   {
	   // Note that we redo the normalization as in PrecomputeFilterLookupTables
	   // Don't care for now because we should use Multithread approach
	   SHFilterCubeMap(a_bUseSolidAngle, a_MCO.FixupType);
   }
   else
   {
   // SL END

	   //if less than 2 filter threads, process all filtering from within callers thread
	   if(m_NumFilterThreads < 2)
	   {
		  //Filter the top mip level (initial filtering used for diffuse or blurred specular lighting )
		  FilterCubeSurfaces(m_InputSurface, m_OutputSurface[0], a_BaseFilterAngle, a_FilterType, a_bUseSolidAngle, 
			 0,  //start at face 0 
			 5,  //end at face 5
			 0
			// SL BEGIN
			, a_MCO.SpecularPower
			, a_MCO.LightingModel
			, a_MCO.FixupType
			// SL END
			 ); //thread 0 is processing
	   }   
	   else //otherwise, split work between callers thread, and newly created thread
	   {
		  // SL BEGIN
		  //consider thread 1 to be initialized
		  sg_ThreadFilterFace[1].m_bThreadInitialized = TRUE;
		  // SL END

		  //setup filtering options for thread1
		  sg_FilterOptionsCPU1.m_cmProc = this;
		  sg_FilterOptionsCPU1.m_SrcCubeMap = m_InputSurface; 
		  sg_FilterOptionsCPU1.m_DstCubeMap = m_OutputSurface[0];
		  sg_FilterOptionsCPU1.m_FilterConeAngle = a_BaseFilterAngle;
		  sg_FilterOptionsCPU1.m_FilterType = a_FilterType;
		  sg_FilterOptionsCPU1.m_bUseSolidAngle = a_bUseSolidAngle; 
		  sg_FilterOptionsCPU1.m_FaceIdxStart = 3; 
		  sg_FilterOptionsCPU1.m_FaceIdxEnd = 5; 
		  sg_FilterOptionsCPU1.m_ThreadIdx = 1; 
		  // SL BEGIN
		  sg_FilterOptionsCPU1.m_MCO = a_MCO;

   		  sg_ThreadFilterFace[1].m_ThreadProgress.m_CurrentMipLevel = 0;
		  sg_ThreadFilterFace[1].m_ThreadProgress.m_CurrentFace = 3;
		  sg_ThreadFilterFace[1].m_ThreadProgress.m_CurrentRow = 0;

		  //start thread 1
		  sg_ThreadFilterFace[1].m_ThreadHandle = CreateThread(
    		 &secAttr,                     //security attributes
			 0,                            //0 is sentinel for use default stack size
			 ThreadProcCPU1Filter,         //LPTHREAD_START_ROUTINE
			 (LPVOID)NULL,                 // pass nothing into function
			 NULL,                         // no creation flags (run thread immediately)
			 &sg_ThreadFilterFace[1].m_ThreadID );
		  // SL END


		  //Filter the top mip level (initial filtering used for diffuse or blurred specular lighting )
		  FilterCubeSurfaces(m_InputSurface, m_OutputSurface[0], a_BaseFilterAngle, a_FilterType, a_bUseSolidAngle, 
			 0,  //start at face 0 
			 2,  //end at face 2
			 0
			// SL BEGIN
			, a_MCO.SpecularPower
			, a_MCO.LightingModel
			, a_MCO.FixupType
			// SL END
			 ); //thread 0 is processing

		  //spin and sleep until thread1 is completed before continuing
		  while( IsFilterThreadActive(1) )
		  {
			 Sleep(100);  //free up this thread for other thread to complete
		  }
	   }

   // SL BEGIN
   } // if (a_bIrradianceCubemap)
   // SL END

   // SL BEGIN
   sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentMipLevel = 1;
   sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentRow = 0;
   sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentFace = 0;
   // SL END

   FixupCubeEdges(m_OutputSurface[0], a_MCO.FixupType, a_FixupWidth);

   //Cone angle start (for generating subsequent mip levels)
   coneAngle = a_InitialMipAngle;

   // SL BEGIN
   // In case of cosine power filter use cosine filter for generate mip level
   if (a_FilterType == CP_FILTER_TYPE_COSINE_POWER)
   {
		a_FilterType = CP_FILTER_TYPE_COSINE;
   }
	// SL END

   //generate subsequent mip levels
   for(i=0; i<(m_NumMipLevels-1); i++)
   {
	  // SL BEGIN
      sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentMipLevel = i+1;
      sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentRow = 0;
      sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentFace = 0;
	  // SL END

      //Build filter lookup tables based on the source miplevel size
	  // SL BEGIN
      PrecomputeFilterLookupTables(a_FilterType, m_OutputSurface[i][0].m_Width, coneAngle, a_MCO.FixupType);
	  // SL END

      //filter cube surfaces
      FilterCubeSurfaces(m_OutputSurface[i], m_OutputSurface[i+1], coneAngle, a_FilterType, a_bUseSolidAngle,
         0,  //start at face 0 
         5,  //end at face 5
         0
		// SL BEGIN
		, a_MCO.SpecularPower
		, a_MCO.LightingModel
		, a_MCO.FixupType
		// SL END
		 ); //thread 0 is processing

	  // SL BEGIN
      sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentMipLevel = i+2;
      sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentRow = 0;
      sg_ThreadFilterFace[0].m_ThreadProgress.m_CurrentFace = 0;
	  // SL END

      FixupCubeEdges(m_OutputSurface[i+1], a_MCO.FixupType, a_FixupWidth);        

      coneAngle = coneAngle * a_MipAnglePerLevelScale;
   }

   m_Status = CP_STATUS_FILTER_COMPLETED;
}


//--------------------------------------------------------------------------------------
//Builds the following lookup tables prior to filtering:
//  -normalizer cube map
//  -tap weight lookup table
// 
//--------------------------------------------------------------------------------------
// SL BEGIN
void CCubeMapProcessor::PrecomputeFilterLookupTables(uint32 a_FilterType, int32 a_SrcCubeMapWidth, float32 a_FilterConeAngle, int32 a_FixupType)
// SL END
{
    float32 srcTexelAngle;
    int32   iCubeFace;
        
    //angle about center tap that defines filter cone
    float32 filterAngle;

    //min angle a src texel can cover (in degrees)
    srcTexelAngle = (180.0f / (float32)CP_PI) * atan2f(1.0f, (float32)a_SrcCubeMapWidth);  

    //filter angle is 1/2 the cone angle
    filterAngle = a_FilterConeAngle / 2.0f;

    //ensure filter angle is larger than a texel
    if(filterAngle < srcTexelAngle)
    {
        filterAngle = srcTexelAngle;    
    }

    //ensure filter cone is always smaller than the hemisphere
    if(filterAngle > 90.0f)
    {
        filterAngle = 90.0f;
    }

    //build lookup table for tap weights based on angle between current tap and center tap
    BuildAngleWeightLUT(a_SrcCubeMapWidth * 2, a_FilterType, filterAngle);

    //clear pre-existing normalizer cube map
    for(iCubeFace=0; iCubeFace<6; iCubeFace++)
    {
        m_NormCubeMap[iCubeFace].Clear();            
    }

    //Normalized vectors per cubeface and per-texel solid angle 
	// SL BEGIN
    BuildNormalizerSolidAngleCubemap(a_SrcCubeMapWidth, m_NormCubeMap, a_FixupType);
	// SL END

}

//--------------------------------------------------------------------------------------
//The key to the speed of these filtering routines is to quickly define a per-face 
//  bounding box of pixels which enclose all the taps in the filter kernel efficiently.  
//  Later these pixels are selectively processed based on their dot products to see if 
//  they reside within the filtering cone.
//
//This is done by computing the smallest per-texel angle to get a conservative estimate 
// of the number of texels needed to be covered in width and height order to filter the
// region.  the bounding box for the center taps face is defined first, and if the 
// filtereing region bleeds onto the other faces, bounding boxes for the other faces are 
// defined next
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::FilterCubeSurfaces(CImageSurface *a_SrcCubeMap, CImageSurface *a_DstCubeMap, 
    float32 a_FilterConeAngle, int32 a_FilterType, bool8 a_bUseSolidAngle, int32 a_FaceIdxStart, 
    int32 a_FaceIdxEnd, int32 a_ThreadIdx
	// SL BEGIN	
	, float32 a_SpecularPower, int32 a_LightingModel, int32 a_FixupType
	// SL END
	)
{
    //CImageSurface normCubeMap[6];     //
    CBBoxInt32    filterExtents[6];   //bounding box per face to specify region to process
                                      // note that pixels within these regions may be rejected 
                                      // based on the     
    int32 iCubeFace, u, v;    
    int32 srcSize = a_SrcCubeMap[0].m_Width;
    int32 dstSize = a_DstCubeMap[0].m_Width;

    float32 srcTexelAngle;
    float32 dotProdThresh;
    int32   filterSize;
        
    //angle about center tap to define filter cone
    float32 filterAngle;

    //min angle a src texel can cover (in degrees)
    srcTexelAngle = (180.0f / (float32)CP_PI) * atan2f(1.0f, (float32)srcSize);  

    //filter angle is 1/2 the cone angle
    filterAngle = a_FilterConeAngle / 2.0f;

    //ensure filter angle is larger than a texel
    if(filterAngle < srcTexelAngle)
    {
        filterAngle = srcTexelAngle;    
    }

    //ensure filter cone is always smaller than the hemisphere
    if(filterAngle > 90.0f)
    {
        filterAngle = 90.0f;
    }

    //the maximum number of texels in 1D the filter cone angle will cover
    //  used to determine bounding box size for filter extents
    filterSize = (int32)ceil(filterAngle / srcTexelAngle);   

    //ensure conservative region always covers at least one texel
    if(filterSize < 1)
    {
        filterSize = 1;
    }

    //dotProdThresh threshold based on cone angle to determine whether or not taps 
    // reside within the cone angle
    dotProdThresh = cosf( ((float32)CP_PI / 180.0f) * filterAngle );

    //thread progress
	// SL BEGIN
    sg_ThreadFilterFace[a_ThreadIdx].m_ThreadProgress.m_StartFace = a_FaceIdxStart;
    sg_ThreadFilterFace[a_ThreadIdx].m_ThreadProgress.m_EndFace = a_FaceIdxEnd;
	// SL END

    //process required faces
    for(iCubeFace = a_FaceIdxStart; iCubeFace <= a_FaceIdxEnd; iCubeFace++)
    {
        CP_ITYPE *texelPtr = a_DstCubeMap[iCubeFace].m_ImgData;

		// SL BEGIN
        sg_ThreadFilterFace[a_ThreadIdx].m_ThreadProgress.m_CurrentFace = iCubeFace;
		// SL END

        //iterate over dst cube map face texel
        for(v = 0; v < dstSize; v++)
        {
		   // SL BEGIN
           sg_ThreadFilterFace[a_ThreadIdx].m_ThreadProgress.m_CurrentRow = v;
		   // SL END

            for(u=0; u<dstSize; u++)
            {
                float32 centerTapDir[3];  //direction of center tap

                //get center tap direction
                // SL BEGIN
				TexelCoordToVect(iCubeFace, (float32)u, (float32)v, dstSize, centerTapDir, a_FixupType);
				// SL END

                //clear old per-face filter extents
                ClearFilterExtents(filterExtents);

                //define per-face filter extents
                DetermineFilterExtents(centerTapDir, srcSize, filterSize, filterExtents );
                    
                //perform filtering of src faces using filter extents 
                ProcessFilterExtents(centerTapDir, dotProdThresh, filterExtents, m_NormCubeMap, a_SrcCubeMap, texelPtr, a_FilterType, a_bUseSolidAngle
					// SL BEGIN
					, a_SpecularPower
					, a_LightingModel
					// SL END
					);

                texelPtr += a_DstCubeMap[iCubeFace].m_NumChannels;
            }            
        }
    }

}


// SL BEGIN
// This function return the BaseFilterAngle require by cubemapgen to its FilterExtends
// It allow to optimize the texel to access base on the specular power.
static float32 GetBaseFilterAngle(float32 cosinePower)
{
	// We want to find the alpha such that:
	// cos(alpha)^cosinePower = epsilon
	// That's: acos(epsilon^(1/cosinePower))
	const float32 threshold = 0.000001f;  // Empirical threshold (Work perfectly, didn't check for a more big number, may get some performance and still god approximation)
	float32 Angle = 180.0f;
	if (Angle != 0.0f)
	{
		Angle = acosf(powf(threshold, 1.0f / cosinePower)); 
		Angle *= 180.0f / (float32)CP_PI; // Convert to degree
		Angle *= 2.0f; // * 2.0f because cubemapgen divide by 2 later
	}

	return Angle;
}
// SL END

//--------------------------------------------------------------------------------------
//starts a new thread to execute the filtering options
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::InitiateFiltering(float32 a_BaseFilterAngle, float32 a_InitialMipAngle, float32 a_MipAnglePerLevelScale,
		int32 a_FilterType, int32 a_FixupType, int32 a_FixupWidth, bool8 a_bUseSolidAngle
	  // SL BEGIN 
	  , bool8 a_bUseMultithread, float32 a_SpecularPower, float32 a_CosinePowerDropPerMip, int32 a_NumMipmap, int32 a_CosinePowerMipmapChainMode
	  ,	bool8 a_bExcludeBase, bool8 a_bIrradianceCubemap,	int32 a_LightingModel, float32 a_GlossScale, float32 a_GlossBias
	  // SL END
	  )
{
	// SL BEGIN
	ModifiedCubemapgenOption MCO;
	// Scale highlight shape to better match lighting model as we can only filter cubemap with Phong filtering.
	// http://seblagarde.wordpress.com/2012/03/29/relationship-between-phong-and-blinn-lighting-model/

	// Approximate curve
	float32 Factor = 1.0f;
	if (a_SpecularPower != 0.0f)
	{
		float32 SP_2	= a_SpecularPower * a_SpecularPower;
		float32 SP_3	= SP_2 * a_SpecularPower;
		Factor		= 4.00012f - (0.624042f / SP_3) + (0.728329f / SP_2) + (1.22792f / a_SpecularPower);
	}

	MCO.SpecularPower				= (a_LightingModel == CP_LIGHTINGMODEL_BLINN || a_LightingModel == CP_LIGHTINGMODEL_BLINN_BRDF) ? a_SpecularPower / Factor : a_SpecularPower; 
	MCO.CosinePowerDropPerMip		= a_CosinePowerDropPerMip;
	MCO.NumMipmap					= a_NumMipmap;
	MCO.CosinePowerMipmapChainMode	= a_CosinePowerMipmapChainMode;
	MCO.bExcludeBase				= a_bExcludeBase;
	MCO.bIrradianceCubemap			= a_bIrradianceCubemap;
	MCO.LightingModel				= a_LightingModel;
	MCO.FixupType					= a_FixupType;
	MCO.GlossScale					= a_GlossScale;
	MCO.GlossBias					= a_GlossBias;
	// SL END

   SECURITY_ATTRIBUTES secAttr;

   secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   secAttr.lpSecurityDescriptor = NULL;            //use same security descriptor as parent
   secAttr.bInheritHandle = TRUE;                  //handle is inherited by new processes spawning from this one

   //set filtering options in main class to determine 
   m_BaseFilterAngle = a_BaseFilterAngle;
   m_InitialMipAngle = a_InitialMipAngle;
   m_MipAnglePerLevelScale = a_MipAnglePerLevelScale;


   //terminate preexisting threads if needed
   TerminateActiveThreads();

   //start thread if at least one filter thread is available
   if(m_NumFilterThreads > 0)
   {
      //consider thread to be initialized
      // SL BEGIN
      sg_ThreadFilterFace[0].m_bThreadInitialized = TRUE;
	  // SL END

      sg_FilterOptionsCPU0.m_cmProc = this;

      sg_FilterOptionsCPU0.m_BaseFilterAngle = a_BaseFilterAngle;
      sg_FilterOptionsCPU0.m_InitialMipAngle = a_InitialMipAngle;
      sg_FilterOptionsCPU0.m_MipAnglePerLevelScale = a_MipAnglePerLevelScale; 
      sg_FilterOptionsCPU0.m_FilterType = a_FilterType;
	  // SL BEGIN
      // sg_FilterOptionsCPU0.m_FixupType = a_FixupType;
	  // SL END
      sg_FilterOptionsCPU0.m_FixupWidth = a_FixupWidth;
      sg_FilterOptionsCPU0.m_bUseSolidAngle = a_bUseSolidAngle;
	  // SL BEGIN
	  sg_FilterOptionsCPU0.m_MCO = MCO;
	  // SL END

      m_Status = CP_STATUS_PROCESSING;

	  // SL BEGIN
	  if (a_bUseMultithread)
	  {
		  // In case of multithread. In order to be able to get progress and preview updated, 
		  // we create a thread which only purpose is to launch other thread to process cubemap face.
		  // So this "dumb" thread is not handled as the other.
		  sg_ThreadFilterFace[0].m_bThreadInitialized = FALSE;
		  DWORD DumbThreadID;
		  DumbThreadHandle = CreateThread(
				 &secAttr,                     //security attributes
				 0,                            //0 is sentinel for use default stack size
				 ThreadProcCPU0FilterMultithread, //LPTHREAD_START_ROUTINE
				 (LPVOID)NULL,                 // pass nothing into function
				 NULL,                         // no creation flags (run thread immediately)
				 &DumbThreadID );
	  }
	  else
	  {
		  sg_ThreadFilterFace[0].m_ThreadHandle = CreateThread(
			 &secAttr,                     //security attributes
			 0,                            //0 is sentinel for use default stack size
			 ThreadProcCPU0Filter,         //LPTHREAD_START_ROUTINE
			 (LPVOID)NULL,                 // pass nothing into function
			 NULL,                         // no creation flags (run thread immediately)
			 &sg_ThreadFilterFace[0].m_ThreadID );
	  }
	  // SL END
   }
   else //otherwise call filtering function from the current process
   {
      FilterCubeMapMipChain(a_BaseFilterAngle, a_InitialMipAngle, a_MipAnglePerLevelScale, a_FilterType, 
		  // SL BEGIN
         /* a_FixupType,*/ a_FixupWidth, a_bUseSolidAngle		
		,MCO
		// SL END
		 );
   
   }


}


//--------------------------------------------------------------------------------------
//build filter lookup table
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::BuildAngleWeightLUT(int32 a_NumFilterLUTEntries, int32 a_FilterType, float32 a_FilterAngle)
{
    int32 iLUTEntry;

    CP_SAFE_DELETE_ARRAY( m_FilterLUT );

    m_NumFilterLUTEntries = 4096; //a_NumFilterLUTEntries;
    m_FilterLUT = new CP_ITYPE [m_NumFilterLUTEntries];

    // note that CP_FILTER_TYPE_DISC weights all taps equally and does not need a lookup table    
    if( a_FilterType == CP_FILTER_TYPE_CONE )
    {
        //CP_FILTER_TYPE_CONE is a cone centered around the center tap and falls off to zero 
        //  over the filtering radius
        CP_ITYPE filtAngleRad = a_FilterAngle * CP_PI / 180.0f;

        for(iLUTEntry=0; iLUTEntry<m_NumFilterLUTEntries; iLUTEntry++ )
        {
            CP_ITYPE angle = acos( (float32)iLUTEntry / (float32)(m_NumFilterLUTEntries - 1) );
            CP_ITYPE filterVal;
        
            filterVal = (filtAngleRad - angle) / filtAngleRad;

            if(filterVal < 0)
            {
                filterVal = 0;
            }

            //note that gaussian is not weighted by 1.0 / (sigma* sqrt(2 * PI)) seen as weights
            // weighted tap accumulation in filters is divided by sum of weights
            m_FilterLUT[iLUTEntry] = filterVal;
        }
    }
    else if( a_FilterType == CP_FILTER_TYPE_ANGULAR_GAUSSIAN )
    {
        //fit 3 standard deviations within angular extent of filter
        CP_ITYPE stdDev = (a_FilterAngle * CP_PI / 180.0) / 3.0;
        CP_ITYPE inv2Variance = 1.0 / (2.0 * stdDev * stdDev);
        
        for(iLUTEntry=0; iLUTEntry<m_NumFilterLUTEntries; iLUTEntry++ )
        {
            CP_ITYPE angle = acos( (float32)iLUTEntry / (float32)(m_NumFilterLUTEntries - 1) );
            CP_ITYPE filterVal;
        
            filterVal = exp( -(angle * angle) * inv2Variance );

            //note that gaussian is not weighted by 1.0 / (sigma* sqrt(2 * PI)) seen as weights
            // weighted tap accumulation in filters is divided by sum of weights
            m_FilterLUT[iLUTEntry] = filterVal;
        }
    }
}


//--------------------------------------------------------------------------------------
// WriteMipLevelIntoAlpha
//
//  Writes the current mip level into alpha in order for 2.0 shaders that need to 
//  know the current mip-level
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::WriteMipLevelIntoAlpha(void)
{
    int32 iFace, iMipLevel;

    //since output is being modified, terminate any active filtering threads
    TerminateActiveThreads();

    //generate subsequent mip levels
    for(iMipLevel = 0; iMipLevel < m_NumMipLevels; iMipLevel++)
    {
        //Iterate over faces for input images
        for(iFace = 0; iFace < 6; iFace++)
        {
            m_OutputSurface[iMipLevel][iFace].ClearChannelConst(3, (float32) (16.0f * (iMipLevel / 255.0f)) );
        }
    }
}


//--------------------------------------------------------------------------------------
// Horizonally flip input cube map faces
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::FlipInputCubemapFaces(void)
{
    int32 iFace;

    //since input is being modified, terminate any active filtering threads
    TerminateActiveThreads();

    //Iterate over faces for input images
    for(iFace = 0; iFace < 6; iFace++)
    {
        m_InputSurface[iFace].InPlaceHorizonalFlip();
    }
}


//--------------------------------------------------------------------------------------
//Horizonally flip output cube map faces
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::FlipOutputCubemapFaces(void)
{
   int32 iFace, iMipLevel;

   //since output is being modified, terminate any active filtering threads
   TerminateActiveThreads();

   //Iterate over faces for input images
   for(iMipLevel = 0; iMipLevel < m_NumMipLevels; iMipLevel++)
   {
      for(iFace = 0; iFace < 6; iFace++)
      {
         m_OutputSurface[iMipLevel][iFace].InPlaceHorizonalFlip();
      }
   }
}


//--------------------------------------------------------------------------------------
// test to see if filter thread is still active
//
//--------------------------------------------------------------------------------------
bool8 CCubeMapProcessor::IsFilterThreadActive(uint32 a_ThreadIdx)
{
	// SL BEGIN
   if( (sg_ThreadFilterFace[a_ThreadIdx].m_bThreadInitialized == FALSE) ||
       (sg_ThreadFilterFace[a_ThreadIdx].m_ThreadHandle == NULL) 
       )
   {
      return FALSE;
   }
   else 
   {  //thread is initialized
      DWORD exitCode;

      GetExitCodeThread(sg_ThreadFilterFace[a_ThreadIdx].m_ThreadHandle, &exitCode );

      if( exitCode == CP_THREAD_STILL_ACTIVE )
      {
         return TRUE;
      }
   }
   // SL END
   
   return FALSE;
}


//--------------------------------------------------------------------------------------
//estimate fraction completed of filter thread based on current conditions
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::EstimateFilterThreadProgress(SFilterProgress *a_FilterProgress)
{
   float32 totalMipComputation = 0.0f;     //time to compute all mip levels as a function of the time it takes 
											//to compute the top mip level

   float32 progressMipComputation = 0.0f;	//progress based on entirely computed mip levels
   float32 currentMipComputation = 0.0f;	//amount of computation it takes to process this entire mip level
   float32 progressFaceComputation = 0.0f;	//progress based on entirely computed faces for this mip level
   float32 currentFaceComputation = 0.0f;	//amount of computation it takes to process this entire face
   float32 progressRowComputation = 0.0f;	//progress based on entirely computed rows for this face
											//estimated fraction of total computation time the current face will take
   
   int32 i;

   float32 filterAngle = 1.0f;					//filter angle for given miplevel
   int32 dstSize = 1;						//destination cube map size of given mip level
   int32 currentMipSize = 1;				//size of mip level currently being processed

   //compuate total compuation time as a function of the time  
   // cubemap processing for each miplevel is roughly O(n^2 * m^2) 
   //  where n is the cube map size, and m is the filter size   
   // Each miplevel is half the size of the previous level,  
   //  and the filter size in texels is roughly proportional to the 
   // (filter angle size * size of source cubemap texels are fetched from) ^2 

   // computation to generate base mip level (generated from input cube map)
   if(m_BaseFilterAngle > 0.0f)
   {
      totalMipComputation = pow(m_InputSize * m_BaseFilterAngle , 2.0f) * (m_OutputSize * m_OutputSize);
   }
   else
   {
      totalMipComputation = pow(m_InputSize * 0.01f , 2.0f) * (m_OutputSize * m_OutputSize);
   }

   progressMipComputation = 0.0f;
   if(a_FilterProgress->m_CurrentMipLevel > 0)
   {
      progressMipComputation = totalMipComputation;
   }

   //filtering angle for this miplevel
   filterAngle = m_InitialMipAngle;
   dstSize = m_OutputSize;

   //computation for entire base mip level (if current level is base level)
   if(a_FilterProgress->m_CurrentMipLevel == 0)
   {
      currentMipComputation = totalMipComputation;
      currentMipSize = dstSize;
   }

   //compuatation to generate subsequent mip levels
   for(i=1; i<m_NumMipLevels; i++)
   {
      float32 computation;

      dstSize /= 2;
      filterAngle *= m_MipAnglePerLevelScale;

      if(filterAngle > 180)
      {
         filterAngle = 180;
      }

      //note src size is dstSize*2 since miplevels are generated from the subsequent level
      computation = pow(dstSize * 2 * filterAngle, 2.0f) * (dstSize * dstSize);

      totalMipComputation += computation;

      //accumulate computation for completed mip levels
      if(a_FilterProgress->m_CurrentMipLevel > i)
      {
         progressMipComputation = totalMipComputation;
      }

      //computation for entire current mip level
      if(a_FilterProgress->m_CurrentMipLevel == i)
      {
         currentMipComputation = computation;
         currentMipSize = dstSize;
      }
   }

   //fraction of compuation time processing the entire current mip level will take
   currentMipComputation  /= totalMipComputation; 
   progressMipComputation /= totalMipComputation;

   progressFaceComputation = currentMipComputation * 
      (float32)(a_FilterProgress->m_CurrentFace - a_FilterProgress->m_StartFace) /
      (float32)(1 + a_FilterProgress->m_EndFace - a_FilterProgress->m_StartFace);

   currentFaceComputation = currentMipComputation * 
      1.0f /
      (1 + a_FilterProgress->m_EndFace - a_FilterProgress->m_StartFace);

   progressRowComputation = currentFaceComputation * 
      ((float32)a_FilterProgress->m_CurrentRow / (float32)currentMipSize);  

   //progress completed
   a_FilterProgress->m_FractionCompleted = 
      progressMipComputation +
      progressFaceComputation +
      progressRowComputation;


   if( a_FilterProgress->m_CurrentFace < 0)
   {
      a_FilterProgress->m_CurrentFace = 0;
   }

   if( a_FilterProgress->m_CurrentMipLevel < 0)
   {
      a_FilterProgress->m_CurrentMipLevel = 0;
   }

   if( a_FilterProgress->m_CurrentRow < 0)
   {
      a_FilterProgress->m_CurrentRow = 0;
   }

}


//--------------------------------------------------------------------------------------
// Return string describing the current status of the cubemap processing threads
//
//--------------------------------------------------------------------------------------
WCHAR *CCubeMapProcessor::GetFilterProgressString(void)
{
   // SL BEGIN
   static WCHAR threadProgressString[CP_MAX_PROGRESS_STRING];

   m_ProgressString[0] = '\0';

   for(int32 i=0; i<m_NumFilterThreads; i++)
   {
      if(IsFilterThreadActive(i))
      {
         //set wait
         SetCursor(LoadCursor(NULL, IDC_WAIT ));

         EstimateFilterThreadProgress(&sg_ThreadFilterFace[i].m_ThreadProgress);

         _snwprintf_s(threadProgressString,
            CP_MAX_PROGRESS_STRING,
            CP_MAX_PROGRESS_STRING,
            L"Thread %d: %5.2f%% Complete (Level %3d, Face %3d, Row %3d)\n", 
			i,
            100.0f * sg_ThreadFilterFace[i].m_ThreadProgress.m_FractionCompleted,
            sg_ThreadFilterFace[i].m_ThreadProgress.m_CurrentMipLevel, 
            sg_ThreadFilterFace[i].m_ThreadProgress.m_CurrentFace,
            sg_ThreadFilterFace[i].m_ThreadProgress.m_CurrentRow        
            );
      }
      else
      {
         //set arrow
         SetCursor(LoadCursor(NULL, IDC_ARROW ));

		 _snwprintf_s(threadProgressString, 
            CP_MAX_PROGRESS_STRING,
            CP_MAX_PROGRESS_STRING,
            L"Thread %d: Ready\n", i);   
      }

	  wcscat_s(m_ProgressString, CP_MAX_PROGRESS_STRING, threadProgressString);
   }
   // SL END

   return m_ProgressString;
}


//--------------------------------------------------------------------------------------
//get status of cubemap processor
//
//--------------------------------------------------------------------------------------
int32 CCubeMapProcessor::GetStatus(void)
{
   return m_Status;
}


//--------------------------------------------------------------------------------------
//refresh status
// sets cubemap processor to ready state if not processing
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::RefreshStatus(void)
{
   if(m_Status != CP_STATUS_PROCESSING )
   {
      m_Status = CP_STATUS_READY;
   
   }
}




// SL BEGIN

SThreadFilterFace* CCubeMapProcessor::sg_ThreadFilterFace;

//Thread functions used to run filtering routines in the CPU0 process
// note that these functions can not be member functions since thread initiation
// must start from a global or static global function
DWORD WINAPI ThreadProcFilterFace(LPVOID a_ThreadIdx)
{
    CBBoxInt32    filterExtents[6];   //bounding box per face to specify region to process
                                      // note that pixels within these regions may be rejected 
                                      // based on the
	// Alias
	SThreadFilterFace* tff = &CCubeMapProcessor::sg_ThreadFilterFace[(int32)a_ThreadIdx];
		
	// Thread will process one face
	CP_ITYPE *texelPtr	= tff->m_DstCubeMap[tff->m_FaceIdx].m_ImgData;
	int32 srcSize		= tff->m_SrcCubeMap[tff->m_FaceIdx].m_Width;
	int32 dstSize		= tff->m_DstCubeMap[tff->m_FaceIdx].m_Width;

	tff->m_ThreadProgress.m_CurrentRow = 0;

	//iterate over dst cube map face texel
	for(int32 v = 0; v < dstSize; v++)
	{
		tff->m_ThreadProgress.m_CurrentRow = v;

		for(int32 u=0; u<dstSize; u++)
		{
			float32 centerTapDir[3];  //direction of center tap

			//get center tap direction
			// SL BEGIN
			TexelCoordToVect(tff->m_FaceIdx, (float32)u, (float32)v, dstSize, centerTapDir, tff->m_FixupType);
			// SL END

			//clear old per-face filter extents
			tff->m_cmProc->ClearFilterExtents(filterExtents);

			//define per-face filter extents
			tff->m_cmProc->DetermineFilterExtents(centerTapDir, srcSize, tff->m_filterSize, filterExtents );

			//perform filtering of src faces using filter extents 
			tff->m_cmProc->ProcessFilterExtents(centerTapDir, tff->m_dotProdThresh, filterExtents, tff->m_cmProc->m_NormCubeMap, tff->m_SrcCubeMap, texelPtr, tff->m_FilterType, tff->m_bUseSolidAngle, tff->m_SpecularPower, tff->m_LightingModel);

			texelPtr += tff->m_DstCubeMap[tff->m_FaceIdx].m_NumChannels;
		}            
	}

	return(CP_THREAD_COMPLETED);
}

void CCubeMapProcessor::FilterCubeSurfacesMultithread(CImageSurface *a_SrcCubeMap, CImageSurface *a_DstCubeMap, 
    float32 a_FilterConeAngle, int32 a_FilterType, bool8 a_bUseSolidAngle, float32 a_SpecularPower, uint32 a_MipIndex, int32 a_LightingModel, int32 a_FixupType)
{
    CBBoxInt32    filterExtents[6];   //bounding box per face to specify region to process
                                      // note that pixels within these regions may be rejected 
                                      // based on the     

    int32 srcSize = a_SrcCubeMap[0].m_Width;

    float32 srcTexelAngle;
    float32 dotProdThresh;
    int32   filterSize;
        
    //angle about center tap to define filter cone
    float32 filterAngle;

    //min angle a src texel can cover (in degrees)
    srcTexelAngle = (180.0f / (float32)CP_PI) * atan2f(1.0f, (float32)srcSize);  

    //filter angle is 1/2 the cone angle
    filterAngle = a_FilterConeAngle / 2.0f;

    //ensure filter angle is larger than a texel
    if(filterAngle < srcTexelAngle)
    {
        filterAngle = srcTexelAngle;    
    }

    //ensure filter cone is always smaller than the hemisphere
    if(filterAngle > 90.0f)
    {
        filterAngle = 90.0f;
    }

    //the maximum number of texels in 1D the filter cone angle will cover
    //  used to determine bounding box size for filter extents
    filterSize = (int32)ceil(filterAngle / srcTexelAngle);   

    //ensure conservative region always covers at least one texel
    if(filterSize < 1)
    {
        filterSize = 1;
    }

    //dotProdThresh threshold based on cone angle to determine whether or not taps 
    // reside within the cone angle
    dotProdThresh = cosf( ((float32)CP_PI / 180.0f) * filterAngle );

    //process required faces
	SECURITY_ATTRIBUTES secAttr;

	//thread security attributes for thread1
	secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	secAttr.lpSecurityDescriptor = NULL;            //use same security descriptor as parent
	secAttr.bInheritHandle = TRUE;                  //handle is inherited by new processes spawning from this one

	int32 FaceIdx = 0;
	INT NumAliveThread = 0;
	bool8* CurrentThreadReady = new bool8[m_NumFilterThreads];

	for (int32 i = 0; i < m_NumFilterThreads; ++i)
	{
		CurrentThreadReady[i] = true;
	}

	while (1)
	{
		// Scheduling is as follow:
		// For each available hardware thread minus one (m_NumFilterThreads) we create a thread to process a cubemap face.
		// When no more hardware thread are available, we wait that one finish its work (we sleep).
		// when a hardware thread become available, a new thread is created to process the next not yet filtered face.
		// TODO : the process can be improve by not createing/destroying thread each time. A better way will be to use GPU or GPGPU but it take too much time to implement it.
		// Remark: As all face take the same work time we can start all hardawre thread together and wait them all instead of waiting the first available. The algorithm here
		// can be good if we start to process mipmap face.
		for (int32 ThreadIdx = 0; ThreadIdx < m_NumFilterThreads; ++ThreadIdx)
		{
			if (CurrentThreadReady[ThreadIdx] && (FaceIdx < 6))
			{
				//initialize thread parameter
				sg_ThreadFilterFace[ThreadIdx].m_bThreadInitialized = TRUE;
				sg_ThreadFilterFace[ThreadIdx].m_cmProc				= this;

				sg_ThreadFilterFace[ThreadIdx].m_SrcCubeMap			= a_SrcCubeMap; 
				sg_ThreadFilterFace[ThreadIdx].m_DstCubeMap			= a_DstCubeMap;
				sg_ThreadFilterFace[ThreadIdx].m_FilterConeAngle	= a_FilterConeAngle;
				sg_ThreadFilterFace[ThreadIdx].m_FilterType			= a_FilterType;
				sg_ThreadFilterFace[ThreadIdx].m_bUseSolidAngle		= a_bUseSolidAngle; 
				sg_ThreadFilterFace[ThreadIdx].m_FaceIdx			= FaceIdx;
				sg_ThreadFilterFace[ThreadIdx].m_SpecularPower		= a_SpecularPower;
				sg_ThreadFilterFace[ThreadIdx].m_LightingModel		= a_LightingModel;
				sg_ThreadFilterFace[ThreadIdx].m_dotProdThresh		= dotProdThresh;
				sg_ThreadFilterFace[ThreadIdx].m_filterSize			= filterSize;
				sg_ThreadFilterFace[ThreadIdx].m_FixupType			= a_FixupType;				

				// thread progress
				sg_ThreadFilterFace[ThreadIdx].m_ThreadProgress.m_CurrentMipLevel	= a_MipIndex;
				sg_ThreadFilterFace[ThreadIdx].m_ThreadProgress.m_CurrentFace		= FaceIdx;
				sg_ThreadFilterFace[ThreadIdx].m_ThreadProgress.m_StartFace			= FaceIdx; // unused
				sg_ThreadFilterFace[ThreadIdx].m_ThreadProgress.m_EndFace			= FaceIdx; // unused

				// Create thread
				sg_ThreadFilterFace[ThreadIdx].m_ThreadHandle = CreateThread(	&secAttr,                     //security attributes
																				0,                            //0 is sentinel for use default stack size
																				ThreadProcFilterFace,         //LPTHREAD_START_ROUTINE
																				(LPVOID)ThreadIdx,            // pass the thread index
																				NULL,                         // no creation flags (run thread immediately)
																				&sg_ThreadFilterFace[ThreadIdx].m_ThreadID );

				CurrentThreadReady[ThreadIdx] = false;
				FaceIdx++;
				NumAliveThread++;
			}
			else if (!CurrentThreadReady[ThreadIdx] && !IsFilterThreadActive(ThreadIdx))
			{
				CurrentThreadReady[ThreadIdx] = true;
				NumAliveThread--;
			}
		}

		if (FaceIdx >= 6 && NumAliveThread == 0)
		{
			// Process is finished
			break;
		}
		else
		{
			//free up this thread for other thread to complete
			Sleep(100);
		}
	}

	CP_SAFE_DELETE_ARRAY(CurrentThreadReady);
}


// SH order use for approximation of irradiance cubemap is 5, mean 5*5 equals 25 coefficients
#define MAX_SH_ORDER 5
#define NUM_SH_COEFFICIENT (MAX_SH_ORDER * MAX_SH_ORDER)

// See Peter-Pike Sloan paper for these coefficients
static float64 SHBandFactor[NUM_SH_COEFFICIENT] = { 1.0,
												2.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0,
												1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0,
												0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // The 4 band will be zeroed
												- 1.0 / 24.0, - 1.0 / 24.0, - 1.0 / 24.0, - 1.0 / 24.0, - 1.0 / 24.0, - 1.0 / 24.0, - 1.0 / 24.0, - 1.0 / 24.0, - 1.0 / 24.0};

void EvalSHBasis(const float32* dir, float64* res )
{
	// Can be optimize by precomputing constant.
	static const float64 SqrtPi = sqrt(CP_PI);

	float64 xx = dir[0];
	float64 yy = dir[1];
	float64 zz = dir[2];

	// x[i] == pow(x, i), etc.
	float64 x[MAX_SH_ORDER+1], y[MAX_SH_ORDER+1], z[MAX_SH_ORDER+1];
	x[0] = y[0] = z[0] = 1.;
	for (int32 i = 1; i < MAX_SH_ORDER+1; ++i)
	{
		x[i] = xx * x[i-1];
		y[i] = yy * y[i-1];
		z[i] = zz * z[i-1];
	}

	res[0]  = (1/(2.*SqrtPi));

	res[1]  = -(sqrt(3/CP_PI)*yy)/2.;
	res[2]  = (sqrt(3/CP_PI)*zz)/2.;
	res[3]  = -(sqrt(3/CP_PI)*xx)/2.;

	res[4]  = (sqrt(15/CP_PI)*xx*yy)/2.;
	res[5]  = -(sqrt(15/CP_PI)*yy*zz)/2.;
	res[6]  = (sqrt(5/CP_PI)*(-1 + 3*z[2]))/4.;
	res[7]  = -(sqrt(15/CP_PI)*xx*zz)/2.;
	res[8]  = sqrt(15/CP_PI)*(x[2] - y[2])/4.;

	res[9]  = (sqrt(35/(2.*CP_PI))*(-3*x[2]*yy + y[3]))/4.;
	res[10] = (sqrt(105/CP_PI)*xx*yy*zz)/2.;
	res[11] = -(sqrt(21/(2.*CP_PI))*yy*(-1 + 5*z[2]))/4.;
	res[12] = (sqrt(7/CP_PI)*zz*(-3 + 5*z[2]))/4.;
	res[13] = -(sqrt(21/(2.*CP_PI))*xx*(-1 + 5*z[2]))/4.;
	res[14] = (sqrt(105/CP_PI)*(x[2] - y[2])*zz)/4.;
	res[15] = -(sqrt(35/(2.*CP_PI))*(x[3] - 3*xx*y[2]))/4.;

	res[16] = (3*sqrt(35/CP_PI)*xx*yy*(x[2] - y[2]))/4.;
	res[17] = (-3*sqrt(35/(2.*CP_PI))*(3*x[2]*yy - y[3])*zz)/4.;
	res[18] = (3*sqrt(5/CP_PI)*xx*yy*(-1 + 7*z[2]))/4.;
	res[19] = (-3*sqrt(5/(2.*CP_PI))*yy*zz*(-3 + 7*z[2]))/4.;
	res[20] = (3*(3 - 30*z[2] + 35*z[4]))/(16.*SqrtPi);
	res[21] = (-3*sqrt(5/(2.*CP_PI))*xx*zz*(-3 + 7*z[2]))/4.;
	res[22] = (3*sqrt(5/CP_PI)*(x[2] - y[2])*(-1 + 7*z[2]))/8.;
	res[23] = (-3*sqrt(35/(2.*CP_PI))*(x[3] - 3*xx*y[2])*zz)/4.;
	res[24] = (3*sqrt(35/CP_PI)*(x[4] - 6*x[2]*y[2] + y[4]))/16.;
}

void CCubeMapProcessor::SHFilterCubeMap(bool8 a_bUseSolidAngleWeighting, int32 a_FixupType)
{
	CImageSurface* SrcCubeImage = m_InputSurface;
	CImageSurface* DstCubeImage = m_OutputSurface[0];

	int32 SrcSize = SrcCubeImage->m_Width;
	int32 DstSize = DstCubeImage->m_Width;

	//pointers used to walk across the image surface
	CP_ITYPE *normCubeRowStartPtr;
	CP_ITYPE *srcCubeRowStartPtr;
	CP_ITYPE *dstCubeRowStartPtr;
	CP_ITYPE *texelVect;

	const int32 SrcCubeMapNumChannels	= SrcCubeImage[0].m_NumChannels;
	const int32 DstCubeMapNumChannels	= DstCubeImage[0].m_NumChannels;

	//First step - Generate SH coefficient for the diffuse convolution

	//Regenerate normalization cubemap
	//clear pre-existing normalizer cube map
	for(int32 iCubeFace=0; iCubeFace<6; iCubeFace++)
	{
		m_NormCubeMap[iCubeFace].Clear();            
	}

	//Normalized vectors per cubeface and per-texel solid angle 
	BuildNormalizerSolidAngleCubemap(m_InputSurface->m_Width, m_NormCubeMap, a_FixupType);

	const int32 NormCubeMapNumChannels = m_NormCubeMap[0].m_NumChannels; // This need to be init here after the generation of m_NormCubeMap

	//This is a custom implementation of D3DXSHProjectCubeMap to avoid to deal with LPDIRECT3DSURFACE9 pointer
	//Use Sh order 2 for a total of 9 coefficient as describe in http://www.cs.berkeley.edu/~ravir/papers/envmap/
    //accumulators are 64-bit floats in order to have the precision needed 
    //over a summation of a large number of pixels 
	float64 SHr[NUM_SH_COEFFICIENT];
	float64 SHg[NUM_SH_COEFFICIENT];
	float64 SHb[NUM_SH_COEFFICIENT];
	float64 SHdir[NUM_SH_COEFFICIENT];

	memset(SHr, 0, NUM_SH_COEFFICIENT * sizeof(float64));
	memset(SHg, 0, NUM_SH_COEFFICIENT * sizeof(float64));
	memset(SHb, 0, NUM_SH_COEFFICIENT * sizeof(float64));
	memset(SHdir, 0, NUM_SH_COEFFICIENT * sizeof(float64));

	float64 weightAccum = 0.0;
	float64 weight = 0.0;

	for (int32 iFaceIdx = 0; iFaceIdx < 6; iFaceIdx++)
	{
		for (int32 y = 0; y < SrcSize; y++)
		{
			normCubeRowStartPtr = &m_NormCubeMap[iFaceIdx].m_ImgData[NormCubeMapNumChannels * (y * SrcSize)];
			srcCubeRowStartPtr	= &SrcCubeImage[iFaceIdx].m_ImgData[SrcCubeMapNumChannels * (y * SrcSize)];

			for (int32 x = 0; x < SrcSize; x++)
			{
				//pointer to direction and solid angle in cube map associated with texel
				texelVect = &normCubeRowStartPtr[NormCubeMapNumChannels * x];

				if(a_bUseSolidAngleWeighting == TRUE)
				{   //solid angle stored in 4th channel of normalizer/solid angle cube map
					weight = *(texelVect+3);
				}
				else
				{   //all taps equally weighted
					weight = 1.0;   
				}

				EvalSHBasis(texelVect, SHdir);

				// Convert to float64
				float64 R = srcCubeRowStartPtr[(SrcCubeMapNumChannels * x) + 0];
				float64 G = srcCubeRowStartPtr[(SrcCubeMapNumChannels * x) + 1];
				float64 B = srcCubeRowStartPtr[(SrcCubeMapNumChannels * x) + 2];

				for (int32 i = 0; i < NUM_SH_COEFFICIENT; i++)
				{
					SHr[i] += R * SHdir[i] * weight;
					SHg[i] += G * SHdir[i] * weight;
					SHb[i] += B * SHdir[i] * weight;
				}
				
				weightAccum += weight;
			}
		}
	}

	//Normalization - The sum of solid angle should be equal to the solid angle of the sphere (4 PI), so
	// normalize in order our weightAccum exactly match 4 PI.
	for (int32 i = 0; i < NUM_SH_COEFFICIENT; ++i)
	{
		SHr[i] *= 4.0 * CP_PI / weightAccum;
		SHg[i] *= 4.0 * CP_PI / weightAccum;
		SHb[i] *= 4.0 * CP_PI / weightAccum;
	}

	//Second step - Generate cubemap from SH coefficient

	// regenerate normalization cubemap for the destination cubemap
	//clear pre-existing normalizer cube map
	for(int32 iCubeFace=0; iCubeFace<6; iCubeFace++)
	{
		m_NormCubeMap[iCubeFace].Clear();            
	}

	//Normalized vectors per cubeface and per-texel solid angle 
	BuildNormalizerSolidAngleCubemap(DstCubeImage->m_Width, m_NormCubeMap, a_FixupType);

	for (int32 iFaceIdx = 0; iFaceIdx < 6; iFaceIdx++)
	{
		for (int32 y = 0; y < DstSize; y++)
		{
			normCubeRowStartPtr = &m_NormCubeMap[iFaceIdx].m_ImgData[NormCubeMapNumChannels * (y * DstSize)];
			dstCubeRowStartPtr	= &DstCubeImage[iFaceIdx].m_ImgData[DstCubeMapNumChannels * (y * DstSize)];

			for (int32 x = 0; x < DstSize; x++)
			{
				//pointer to direction and solid angle in cube map associated with texel
				texelVect = &normCubeRowStartPtr[NormCubeMapNumChannels * x];

				EvalSHBasis(texelVect, SHdir);

				// get color value
				CP_ITYPE R = 0.0f, G = 0.0f, B = 0.0f;
				
				for (int32 i = 0; i < NUM_SH_COEFFICIENT; ++i)
				{
					R += (CP_ITYPE)(SHr[i] * SHdir[i] * SHBandFactor[i]);
					G += (CP_ITYPE)(SHg[i] * SHdir[i] * SHBandFactor[i]);
					B += (CP_ITYPE)(SHb[i] * SHdir[i] * SHBandFactor[i]);
				}

				dstCubeRowStartPtr[(DstCubeMapNumChannels * x) + 0] = R;
				dstCubeRowStartPtr[(DstCubeMapNumChannels * x) + 1] = G;
				dstCubeRowStartPtr[(DstCubeMapNumChannels * x) + 2] = B;
				if (DstCubeMapNumChannels > 3)
				{
					dstCubeRowStartPtr[(DstCubeMapNumChannels * x) + 3] = 1.0f;
				}
			}
		}
	}
}

void CCubeMapProcessor::FilterCubeMapMipChainMultithread(float32 a_BaseFilterAngle, float32 a_InitialMipAngle, float32 a_MipAnglePerLevelScale, 
    int32 a_FilterType, int32 a_FixupWidth, bool8 a_bUseSolidAngle, const ModifiedCubemapgenOption& a_MCO
	)
{
	//Cone angle start (for generating subsequent mip levels)
	float32 coneAngle = a_InitialMipAngle;
	float32 specularPower = a_MCO.SpecularPower;
	int32 Num = m_NumMipLevels; // Note that we need to filter the first level before generating mipmap
								// So LevelIndex == 0 is base filtering hen LevelIndex > 0 is mipmap generation

	for(int32 LevelIndex = 0; LevelIndex < Num; LevelIndex++)
	{
		// IF we require it calculate the specular power to used for the filtering
		if ( a_FilterType == CP_FILTER_TYPE_COSINE_POWER && a_MCO.CosinePowerMipmapChainMode == CP_COSINEPOWER_CHAIN_MIPMAP && a_MCO.NumMipmap > 1)
		{
			int32 NumMipmap			= min(a_MCO.NumMipmap, Num);
			float32 Glossiness		= (NumMipmap == 1) ? 1.0f : max(1.0f - (FLOAT)(LevelIndex) / (FLOAT)(NumMipmap - 1), 0.0f);
			// This function must match the decompression function of the engine.
			specularPower = powf(2.0f, a_MCO.GlossScale * Glossiness + a_MCO.GlossBias);
		}

		// TODO : Write a function to copy and scale the base mipmap in output
		// I am just lazy here and just put a high specular power value, and do some if.
		if (a_FilterType == CP_FILTER_TYPE_COSINE_POWER && a_MCO.bExcludeBase && (LevelIndex == 0))
		{
			// If we don't want to process the base mipmap, just put a very high specular power (this allow to handle scale of the texture).
			specularPower = 100000.0f;
		}

		// If diffuse convolution is required, go through SH filtering for base level
		// Other level have no meaning in this case.
		if (LevelIndex == 0 && a_MCO.bIrradianceCubemap)
		{
			SHFilterCubeMap(a_bUseSolidAngle, a_MCO.FixupType);

			FixupCubeEdges(m_OutputSurface[0], a_MCO.FixupType, a_FixupWidth);

			continue;
		}
		else if (a_MCO.bIrradianceCubemap)
		{
			// Following code will perturb artists which will not understand to check the select mip level 0 (the main view perform filtering...) so 
			// just apply a cosine filter with small angle for mipamp.
			/*
			// Fill texture with black pixel for mipmap.
			CImageSurface* DstCubeImage = m_OutputSurface[LevelIndex];
			int32 DstCubeMapNumChannels = DstCubeImage[0].m_NumChannels;
			int32 DstSize = DstCubeImage[0].m_Width;
			CP_ITYPE *dstCubeRowStartPtr;

			for (int32 iFaceIdx = 0; iFaceIdx < 6; iFaceIdx++)
			{
				for (int32 y = 0; y < DstSize; y++)
				{
					dstCubeRowStartPtr = &DstCubeImage[iFaceIdx].m_ImgData[DstCubeMapNumChannels * (y * DstSize)];

					for (int32 x = 0; x < DstSize; x++)
					{
						dstCubeRowStartPtr[(DstCubeMapNumChannels * x) + 0] = 0;
						dstCubeRowStartPtr[(DstCubeMapNumChannels * x) + 1] = 0;
						dstCubeRowStartPtr[(DstCubeMapNumChannels * x) + 2] = 0;
						if (DstCubeMapNumChannels > 3)
						{
							dstCubeRowStartPtr[(DstCubeMapNumChannels * x) + 3] = 1;
						}
					}
				}
			}

			continue;
			*/

			a_FilterType = CP_FILTER_TYPE_COSINE;
			a_BaseFilterAngle = 1.0f;
			a_MipAnglePerLevelScale = 1.0f;
		}

		int32 FilterType = a_FilterType;

		// Special case for cosine power mipmap chain. For quality requirement, we always process the current mipmap from the top mipmap
		CImageSurface* SrcCubeImage = m_InputSurface;
        if ((LevelIndex != 0) && (a_FilterType != CP_FILTER_TYPE_COSINE_POWER) )
		{
			SrcCubeImage = m_OutputSurface[LevelIndex-1];
		}

		CImageSurface* DstCubeImage = m_OutputSurface[LevelIndex];

		float32 Angle;

		if (LevelIndex == 0)
		{
			Angle = a_BaseFilterAngle;
		}
		else
		{
			Angle = coneAngle;
			coneAngle *= a_MipAnglePerLevelScale;
		}

		// If we use SpecularPower, automatically calculate the a_BaseFilterAngle required, this will speed the process
		if (a_FilterType == CP_FILTER_TYPE_COSINE_POWER)
		{
			Angle = GetBaseFilterAngle(specularPower);
			// Following code allow to fix thread '%' work display
			// It is only used for this purpose
			if (LevelIndex == 0)
			{
				m_BaseFilterAngle = Angle;
			}
			else
			{
				m_BaseFilterAngle = 0.0f;
			}
		}

		//Build filter lookup tables based on the source miplevel size
		// SL BEGIN
		PrecomputeFilterLookupTables(FilterType, SrcCubeImage->m_Width, Angle, a_MCO.FixupType);
		// SL END

		// Use multithread only for large mipmap
		if (SrcCubeImage->m_Width >= 64)
		{
			FilterCubeSurfacesMultithread(SrcCubeImage, DstCubeImage, Angle, FilterType, a_bUseSolidAngle, specularPower, LevelIndex, a_MCO.LightingModel, a_MCO.FixupType);
		}
		else
		{
			//filter cube surfaces
			FilterCubeSurfaces(SrcCubeImage, DstCubeImage, Angle, FilterType, a_bUseSolidAngle,
								0,  //start at face 0 
								5,  //end at face 5
								0, //Main thread is processing
								specularPower,
								a_MCO.LightingModel,
								a_MCO.FixupType
								); 

		}

		FixupCubeEdges(DstCubeImage, a_MCO.FixupType, a_FixupWidth);

		// Decrease the specular power to generate the mipmap chain
		if ( a_FilterType == CP_FILTER_TYPE_COSINE_POWER && a_MCO.CosinePowerMipmapChainMode == CP_COSINEPOWER_CHAIN_DROP)
		{
			// TODO : Use another method for Exclude (see first comment at start of the function
			if (a_MCO.bExcludeBase && (LevelIndex == 0))
			{
				specularPower = a_MCO.SpecularPower;
			}

			specularPower *= a_MCO.CosinePowerDropPerMip;
		}
	}

	m_Status = CP_STATUS_FILTER_COMPLETED;
}

// SL END