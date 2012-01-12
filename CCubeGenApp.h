//--------------------------------------------------------------------------------------
// File: CCubeGenApp.cpp
//
// Functions to setup/render/shutdowm for shadow mapping
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef CUBEGENAPP_H
#define CUBEGENAPP_H

#include "LocalDXUT\\dxstdafx.h"
#include "types.h"
#include "CGeom.h"
#include "CFrustum.h"
#include "CEffectList.h"
#include "CTexture.h"
#include "CCubeMapProcessor.h"
#include "ErrorMsg.h"

#include "resource.h"

//do not allow "dxstdafx.h" to depricate any core string functions
#define CG_MAX_FILENAME_LENGTH 4096

#define CG_EFFECT_RESOURCE_ID  IDR_CUBEGENRENDER_FX

#define CG_SCENE_FILENAME L"Models\\sphere.obj"
#define CG_EFFECT_FILENAME L"Effects\\CubeGenRender.fx"

#define CG_INITIAL_INPUT_CUBEMAP_FILENAME L"Textures\\RubyTestCube.dds"
#define CG_INITIAL_OUTPUT_CUBEMAP_FILENAME L"Textures\\OutputCube.dds"

#define CG_OBJECT_DIRECTORY     L".\\Models\\."
#define CG_BASEMAP_DIRECTORY    L".\\Textures\\BaseMaps\\."
#define CG_CUBEMAP_DIRECTORY    L".\\Textures\\CubeMaps\\."
#define CG_CUBECROSS_DIRECTORY  L".\\Textures\\CubeCrosses\\."
#define CG_SEPARATE_FACES_DIRECTORY L".\\Textures\\Separate Faces\\."
#define CG_TEXTURE_DIRECTORY    L".\\Textures\\."

//cube map to display
#define CG_CUBE_DISPLAY_INPUT  0
#define CG_CUBE_DISPLAY_OUTPUT 1

//how to display selected cubemap
#define CG_CUBE_SELECT_FACE_XPOS 0
#define CG_CUBE_SELECT_FACE_XNEG 1
#define CG_CUBE_SELECT_FACE_YPOS 2
#define CG_CUBE_SELECT_FACE_YNEG 3
#define CG_CUBE_SELECT_FACE_ZPOS 4
#define CG_CUBE_SELECT_FACE_ZNEG 5
#define CG_CUBE_SELECT_NONE      6

//export face layout
#define CG_EXPORT_FACE_LAYOUT_D3D    0
#define CG_EXPORT_FACE_LAYOUT_SUSHI  1
#define CG_EXPORT_FACE_LAYOUT_OPENGL 2

//rendering modes
#define CG_RENDER_NORMAL              0
#define CG_RENDER_REFLECT_PER_VERTEX  1
#define CG_RENDER_REFLECT_PER_PIXEL   2
#define CG_RENDER_MIP_ALPHA_LOD       3

//number of frames between each output cubemap refresh when processing cubemap
#define CG_PROCESSING_FRAME_REFRESH_INTERVAL 50

//set maximum clamp value to a huge number
#define CG_DEFAULT_MAX_CLAMP 10e30f

//intermediate surface type
//#define CG_INTERMEDIATE_SURFACE_FORMAT //A8R8G8B8
#define CG_INTERMEDIATE_SURFACE_FORMAT D3DFMT_A32B32G32R32F

class CCubeGenApp
{
public:
   IDirect3DDevice9 *m_pDevice;
   D3DCAPS9 m_DeviceCaps;

   bool8 m_bInitialized;
   bool8 m_bErrorOccurred;

   //camera frustum used for rendering
   CFrustum m_EyeFrustum;

   CGeom m_GeomCube;
   CGeom m_GeomQuad;
   CGeom m_GeomScene;

   CGeom m_GeomSkySphere;


   //current rendering options
   int32 m_RenderTechnique;
   int32 m_DisplayCubeSource;          //which cube map to display
   bool8 m_bMipLevelSelectEnable;
   int32 m_MipLevelDisplayed;
   bool8 m_bMipLevelClampEnable;       //mip level clamp enable
   bool8 m_bShowAlpha;                 //show alpha
   bool8 m_bDrawSkySphere;             //draw skysphere
   bool8 m_bCenterObject;              //center object

   bool8 m_bOutputPeriodicRefresh;     //whether or not to refresh output cubemap every so many frames
   // SL BEGIN
   bool8 m_bUseMultithread;
   bool8 m_bIrradianceCubemap;
   bool8 m_bPhongBRDF;
   // SL END
   int32 m_FramesSinceLastRefresh;     //number of frames since last cubemap refresh

   int32 m_SelectedCubeFace;           //selected cube map face
   int32 m_ExportFaceLayout;           //export face layout (e.g. how cube faces are saved out to individual files)
   bool8 m_bExportMipChain;            //whether or not to export the entire mipchain


   //cubemap processor and filtering options
   CCubeMapProcessor   m_CubeMapProcessor;
   int32               m_FilterTech;
   float32             m_BaseFilterAngle;
   float32             m_MipInitialFilterAngle;
   float32             m_MipFilterAngleScale; 
   bool8               m_bUseSolidAngleWeighting;
   // SL BEGIN
   // Specular power will be used when cosinus power filter is set
   uint32			   m_SpecularPower;
   float32			   m_SpecularPowerDropPerMip;
   // SL END

   int32               m_EdgeFixupTech;
   bool8               m_bCubeEdgeFixup;
   int32               m_EdgeFixupWidth;
   bool8               m_bWriteMipLevelIntoAlpha;

   bool8               m_bValidOutputCubemap;  //is the output cube map valid?


   //information about cube maps
   int32 m_InputCubeMapSize;           // input cube map size
   D3DFORMAT m_InputCubeMapFormat;     // pixel format of input cube map
   float32 m_InputScaleFactor;         //  input scale factor
   float32 m_InputDegamma;             //  input degamma

   float32 m_InputMaxClamp;            //  input max clamp val

   int32 m_OutputCubeMapSize;          // output cube map size
   D3DFORMAT m_OutputCubeMapFormat;    // pixel format of cube map
   float32 m_OutputScaleFactor;        // output scale factor
   float32 m_OutputGamma;              // output gamma

   CTexture m_pInputCubeMap;
   CTexture m_pOutputCubeMap;
   CTexture m_pBaseMap;                // base map 

   bool8 m_bFlipFaceSurfacesOnImport;  // flip face surfaces horizonally on import
   bool8 m_bFlipFaceSurfacesOnExport;  // flip face surfaces horizonally on export

   WCHAR m_BaseMapFilename[CG_MAX_FILENAME_LENGTH];
   WCHAR m_SceneFilename[CG_MAX_FILENAME_LENGTH];
   WCHAR m_InputCubeMapFilename[CG_MAX_FILENAME_LENGTH];
   WCHAR m_OutputCubeMapFilename[CG_MAX_FILENAME_LENGTH];


   //used for rendering scene
   D3DXMATRIX m_ObjectWorldMatrix;
   D3DXMATRIX m_EyeSceneWVP;
   D3DXMATRIX m_EyeSceneVP;

   //effects used for rendering
   CEffect m_pEffect;

   CCubeGenApp(void);
   ~CCubeGenApp();

   HRESULT Init(IDirect3DDevice9 *a_pDevice);
   HRESULT InitShaders(void);

   HRESULT LoadBaseMap(void);

   HRESULT LoadInputCubeMap(void);
   HRESULT LoadSelectedCubeMapFace(WCHAR *aCubeFaceFilename);
   HRESULT LoadInputCubeCross(WCHAR *aFilename);

   //an input of zero means that a full mip chain is created
   HRESULT CreateOutputCubeMap(int32 a_NumMipLevels);

   HRESULT SaveOutputCubeMap(void);
   HRESULT SaveOutputCubeMapToFiles(WCHAR *aFilenamePrefix, D3DXIMAGE_FILEFORMAT aDestFormat);
   HRESULT SaveOutputCubeMapToCrosses(WCHAR *aFilenamePrefix, D3DXIMAGE_FILEFORMAT aDestFormat);

   void LoadObjects(void);

   void SetOutputCubeMapSize(int32 a_Size);
   void SetOutputCubeMapTexelFormat(D3DFORMAT a_Format);
   void RefreshOutputCubeMap(void);

   void SetRenderTechnique(int32 a_RenderTechnique);

   void SetEyeFrustumProj(D3DXMATRIX *aProjMX);
   void SetEyeFrustumView(D3DXMATRIX *aViewMX);
   void SetObjectWorldMatrix(D3DXMATRIX *aWorldMX);
   void UpdateEyeEffectParams(void);

   void Draw(void);
   void DrawScene(int32 m_RenderTechnique);

   void SetCubeMapProcessorInputCubeMap(CCubeMapProcessor *aCMProc);
   //void GetCubeMapProcessorInputCubeMap(CCubeMapProcessor *aCMProc);
   void GetCubeMapProcessorOutputCubeMap(CCubeMapProcessor *aCMProc);

   void FlipSelectedFaceDiagonal(void); 
   void FlipSelectedFaceVertical(void); 
   void FlipSelectedFaceHorizontal(void); 
   void InitCSurfaceImageUsingD3DSurface(CImageSurface *a_pSurfImg, int32 a_NumChannels, LPDIRECT3DSURFACE9 a_pD3DSurface, RECT *a_Rect );
   void CopyCSurfaceImageToD3DSurface(CImageSurface *a_pSurfImg, LPDIRECT3DSURFACE9 a_pD3DSurface );

   void FilterCubeMap(void);
   bool8 HandleInvalidOutputCubemapBeforeSaving(void);


   void OnResetDevice(IDirect3DDevice9 *a_pDevice);
   void OnLostDevice(void);
   void OnDestroyDevice(void);

};

#endif //CUBEGENAPP_H
