//--------------------------------------------------------------------------------------
// File: CubeGenRender.fx
//
// The effect file for CubeMapGen   
// 
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
float    g_fTime;                   // App's time in seconds
float4x4 g_mWorld;                  // World matrix for object
float4x4 g_mViewProj;               // View * Projection matrix
float4x4 g_mWorldView;              // World * View
float4x4 g_mWorldViewProj;          // World * View * Projection matrix
float4   g_vWorldCamPos;            // camera position in world space

float4   g_vTextureSize;            // Texture size
float4   g_vHalfTexelOffset;        // Half texel offset (in u and v)
float4   g_vFullTexelOffset;        // Full texel offset (in u and v)

texture  g_tBaseMap;                // Base map
texture  g_tCubeMap;                // Cube map
float    g_fMipLevelClamp;          // highest mip level clamped to
float    g_fMipLODBias;             // LOD bias 

float  g_fScaleFactor;              // intensity scale factor
float  g_fNumMipLevels;             // number of mip levels in mip chain

bool   g_bShowAlpha;                // show alpha from cubemap
// SL BEGIN
bool	g_bFixSeams;
float	g_fCubeSize;
// SL END

float g_fRoughnessScaleFactor = 4.0; //number of miplevels to go down in cubemap for the roughest areas

//base sampler
sampler BaseSampler = sampler_state
{
   Texture = <g_tBaseMap>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
   AddressU  = Wrap;
   AddressV  = Wrap;
   MaxAnisotropy = 16;
};

//cube sampler
sampler CubeSampler = sampler_state
{
   Texture = <g_tCubeMap>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
   AddressU  = Clamp;
   AddressV  = Clamp;
   MaxAnisotropy = 16;
   MaxMipLevel = (g_fMipLevelClamp);
   MipMapLodBias = (g_fMipLODBias);
};

//cube sampler without bias (for shaders that use texCUBEBIAS)
sampler CubeSamplerNoBias = sampler_state
{
   Texture = <g_tCubeMap>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
   AddressU  = Clamp;
   AddressV  = Clamp;
   MaxAnisotropy = 16;
   MaxMipLevel = (g_fMipLevelClamp);
};




//--------------------------------------------------------------------------------------
// Utility Functions
//--------------------------------------------------------------------------------------
float4 BX2(float4 x)
{
   return 2.0f * x - 1.0f;
}


float4 SignPack(float4 x)
{
   return x * 0.5f + 0.5f;
}


float   WithinUnitSquare2D(float2 pos)
{
   float4 sepTests; 

   //test pos.x >= 0, pos.y >= 0, pos.x <   1,  pos.y   <=  1,  
   sepTests = (float4(1, 1, -1, -1) * pos.xyxy + float4(0, 0, 1, 1))    >=  0;

   return( dot(sepTests, float4(1, 1, 1, 1) ) >= 4  );
}


//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------
void vs_PosOnly(float4 Pos : POSITION,
                out float4 oPos : POSITION)
{
   oPos = mul( Pos, g_mWorldViewProj );
}


//--------------------------------------------------------------------------------------
//output WS normal and base tex coord
//--------------------------------------------------------------------------------------
void vs_PosBaseNorm(float4 Pos : POSITION,
                    float3 Normal : NORMAL,
                    float2 BaseTex : TEXCOORD0,                //texture coordinates for base map
                    out float4 oPos : POSITION,
                    out float2 oBaseTexCoord : TEXCOORD0,      //texture coords for base and bump map
                    out float3 oWorldSpaceNormal : TEXCOORD1,  //normal in world space
                    out float3 oWorldSpaceView : TEXCOORD2,    //world space view vector
                    out float3 oReflectVect : TEXCOORD3)       //reflection vector
{


   //Compute the projected coordinates
   oPos = mul( Pos, g_mWorldViewProj );

   //base texture coordinates
   oBaseTexCoord = BaseTex;

   //Normal in world space
   oWorldSpaceNormal = mul( Normal, (float3x3)g_mWorld);

   // later pass in position and compute WS view vector
   oWorldSpaceView = mul( Pos.xyz, (float3x3)g_mWorld) - g_vWorldCamPos;

   //reflection vector
   oReflectVect = reflect(oWorldSpaceView, normalize(oWorldSpaceNormal) );

}


//--------------------------------------------------------------------------------------
//render geometry directly to screen without using xform 
//--------------------------------------------------------------------------------------
void vs_ScreenQuad(// Inputs
                   float4 Pos : POSITION,
                   out float4 oPos : POSITION,
                   out float2 oTex0 : TEXCOORD0)             
{
   oTex0.x = (Pos.x * 0.5) + 0.5 + g_vHalfTexelOffset.x;
   oTex0.y = (Pos.y * -0.5) + 0.5 + g_vHalfTexelOffset.y;

   oPos.xyz = Pos.xyz;
   oPos.w = 1;
}


//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------
float4 ps_White( void ) : COLOR0
{
   // Return white
   return float4 ( 1.0f, 1.0f, 1.0f, 1.0f );
}


//----------------------------------------------------------------------------------------------
//write base map and modulate with color
//----------------------------------------------------------------------------------------------
float4 ps_BaseMap(float2 oBaseTex : TEXCOORD0) : COLOR0
{

   return( tex2D(BaseSampler, oBaseTex ) );
}


//----------------------------------------------------------------------------------------------
//lookup cube map using vertex normal
//----------------------------------------------------------------------------------------------
float4 ps_CubeNorm( float2 oBaseTexCoord : TEXCOORD0,
                   float3 oWorldSpaceNormal : TEXCOORD1) : COLOR0
{
   float4 cubeMap;

   //cubeMap = texCUBEbias(CubeSampler, float4(oWorldSpaceNormal, g_fMipLODBias) );
   cubeMap = texCUBE(CubeSampler, oWorldSpaceNormal);

   if( g_bShowAlpha == true )
   {
      cubeMap = cubeMap.a;
   }

   return( g_fScaleFactor * cubeMap );
}


//----------------------------------------------------------------------------------------------
//lookup cube map using vertex normal and modulate with basemap
//----------------------------------------------------------------------------------------------
float4 ps_BaseCubeNorm( float2 oBaseTexCoord : TEXCOORD0,
                   float3 oWorldSpaceNormal : TEXCOORD1) : COLOR0
{
   float4 baseMap, cubeMap;
   
   // SL BEGIN
   if (g_bFixSeams)
   {
	   // Note : LOD level to display is fixed in .fx see
	   // MaxMipLevel = (g_fMipLevelClamp); and MipMapLodBias = (g_fMipLODBias);
	   // so texCUBE show the right lod. Standard code should use texCUBElod
		float scale = 1 - exp2(g_fMipLevelClamp) / g_fCubeSize;
		float M = max(max(abs(oWorldSpaceNormal.x), abs(oWorldSpaceNormal.y)), abs(oWorldSpaceNormal.z));
		if (abs(oWorldSpaceNormal.x) != M) oWorldSpaceNormal.x *= scale;
		if (abs(oWorldSpaceNormal.y) != M) oWorldSpaceNormal.y *= scale;
		if (abs(oWorldSpaceNormal.z) != M) oWorldSpaceNormal.z *= scale;
   }
  // SL END

   baseMap = tex2D(BaseSampler, oBaseTexCoord);
   //cubeMap = texCUBEbias(CubeSampler, float4(oWorldSpaceNormal, g_fMipLODBias) );
   cubeMap = texCUBE(CubeSampler, oWorldSpaceNormal);

   if( g_bShowAlpha == true )
   {
      cubeMap = cubeMap.a;
   }

   return ( g_fScaleFactor * baseMap * cubeMap );
}


//----------------------------------------------------------------------------------------------
//lookup cube map using reflection vector and modulate with basemap
//----------------------------------------------------------------------------------------------
float4 ps_BaseCubeReflectPerVertex(float2 oBaseTexCoord : TEXCOORD0,
                      float3 oWorldSpaceNormal : TEXCOORD1,
                      float3 oWorldSpaceView : TEXCOORD2,
                      float3 oReflectVect : TEXCOORD3) : COLOR0
{
   float4 baseMap, cubeMap;
   float3 reflectVect;

   baseMap = tex2D(BaseSampler, oBaseTexCoord);
   cubeMap = texCUBE(CubeSampler, oReflectVect);

   if( g_bShowAlpha == true )
   {
      cubeMap = cubeMap.a;
   }

   return( g_fScaleFactor * baseMap * cubeMap );
}


//----------------------------------------------------------------------------------------------
//lookup cube map using reflection vector and modulate with basemap
//----------------------------------------------------------------------------------------------
float4 ps_BaseCubeReflectPerPixel(float2 oBaseTexCoord : TEXCOORD0,
                      float3 oWorldSpaceNormal : TEXCOORD1,
                      float3 oWorldSpaceView : TEXCOORD2,
                      float3 oReflectVect : TEXCOORD3) : COLOR0
{
   float4 baseMap, cubeMap;
   float3 reflectVect;
   reflectVect = reflect(oWorldSpaceView, normalize(oWorldSpaceNormal) );

   baseMap = tex2D(BaseSampler, oBaseTexCoord);
   //cubeMap = texCUBEbias(CubeSampler, float4(reflectVect, g_fMipLODBias) );
   //cubeMap = texCUBE(CubeSampler, oReflectVect);
   cubeMap = texCUBE(CubeSampler, reflectVect);

   if( g_bShowAlpha == true )
   {
      cubeMap = cubeMap.a;
   }

   return( g_fScaleFactor * baseMap * cubeMap );
}


//----------------------------------------------------------------------------------------------
// Shader with Per-Pixel Roughness using mip encoded in alpha to offset texCUBEBias amount to 
//   emulate effect of texCUBElod on 2.0 shader parts.
//
//----------------------------------------------------------------------------------------------
float4 ps_BaseCubeNormMipAlphaLOD(float2 oBaseTexCoord : TEXCOORD0,
                              float3 oWorldSpaceNormal : TEXCOORD1, 
                              float3 oWorldSpaceView : TEXCOORD2 ) : COLOR0
{
   float4 baseMap;
   float mipLevel;
   float3 reflectVect, normNormal, envLookupVector;
   float roughnessFactor;  

   baseMap = tex2D(BaseSampler, oBaseTexCoord * float2(1, 1 ));

   roughnessFactor = baseMap.a;

   normNormal = normalize(oWorldSpaceNormal);
   reflectVect = normalize(reflect(oWorldSpaceView, normalize(oWorldSpaceNormal) ));

   //to determine mip-LOD levels from 0 to g_fNumMipLevels
   float mipLevelMinification = (255.0/16.0) * texCUBE(CubeSamplerNoBias, oWorldSpaceNormal ).a;

   //to determine mip-LOD levels from -g_fNumMipLevels to 0
   float mipLevelMagnification = (255.0/16.0) * texCUBEbias(CubeSamplerNoBias, float4(oWorldSpaceNormal, g_fNumMipLevels-1) ).a;

   //choose between magnification and minification range
   if(mipLevelMinification == 0)
   {
      mipLevel = mipLevelMagnification - (g_fNumMipLevels-1);
   }
   else
   {
      mipLevel = mipLevelMinification;
   }

   //lerp between surface normal and relfection vector based on glossiness
   //envLookupVector = normalize( lerp( normNormal, reflectVect, roughnessFactor) );

   //or just use reflection vector as is
   envLookupVector = reflectVect;

   //glossiness controls number of miplevels to go down (in this case up to 6 mip-levels)
   float mipBias = max( (g_fRoughnessScaleFactor * (1.0 - roughnessFactor)) - mipLevel, 0) ;

   return( g_fScaleFactor 
      * baseMap 
      * texCUBEbias(CubeSamplerNoBias, float4(envLookupVector, mipBias) ) );
}



//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

//just render in white
technique White
{
   pass P0
   {
      VertexShader = compile vs_2_0 vs_PosOnly();
      PixelShader = compile ps_2_0 ps_White();

      CullMode = None;

      ZEnable = False;
      ZWriteEnable = False;
      ZFunc = Always;
   }    
}


//index into cubemap using surface normal
technique CubeMapNorm
{
   pass P0
   {
      VertexShader = compile vs_2_0 vs_PosBaseNorm();
      PixelShader = compile ps_2_0 ps_CubeNorm();

      CullMode = None;

      ZEnable = True;
      ZWriteEnable = True;
      ZFunc = LessEqual;
   }    
}


//index into cubemap using surface normal
technique BaseCubeMapNorm
{
   pass P0
   {
      VertexShader = compile vs_2_0 vs_PosBaseNorm();
      PixelShader = compile ps_2_0 ps_BaseCubeNorm();

      CullMode = None;

      ZEnable = True;
      ZWriteEnable = True;
      ZFunc = LessEqual;
   }    
}


//index into cubemap using reflection vector
technique BaseCubeMapReflectPerVertex
{
   pass P0
   {
      VertexShader = compile vs_2_0 vs_PosBaseNorm();
      PixelShader = compile ps_2_0 ps_BaseCubeReflectPerVertex();

      CullMode = None;

      ZEnable = True;
      ZWriteEnable = True;
      ZFunc = LessEqual;
   }    
}


//index into cubemap using reflection vector
technique BaseCubeMapReflectPerPixel
{
   pass P0
   {
      VertexShader = compile vs_2_0 vs_PosBaseNorm();
      PixelShader = compile ps_2_0 ps_BaseCubeReflectPerPixel();

      CullMode = None;

      ZEnable = True;
      ZWriteEnable = True;
      ZFunc = LessEqual;
   }    
}


//index into cubemap using surface normal
technique BaseCubeMapMipAlphaLOD
{
   pass P0
   {
      VertexShader = compile vs_2_0 vs_PosBaseNorm();
      PixelShader = compile ps_2_0 ps_BaseCubeNormMipAlphaLOD();

      CullMode = None;

      ZEnable = True;
      ZWriteEnable = True;
      ZFunc = LessEqual;
   }    
}





