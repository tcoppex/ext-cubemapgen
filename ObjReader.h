//--------------------------------------------------------------------------------------
//lightwave .Obj file reader
//
//note: this code only handles polys with 30 or fewer sides
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------

#include "Types.h"
#include <stdio.h>
#include <stdlib.h>

#define OR_MAX_FACE_VERTS 128

class ObjReader
{
   public:

   //object data converted from file
   uint32 mNumIndex;
   uint32 mNumVertex;

   uint32 *mIndex;   
   float32 *mPosition;
   float32 *mNormal;
   float32 *mTexCoord;
   float32 *mTangentU;
   float32 *mTangentV;

   // translation from triple index in obj file for each newly created condensed index
   uint32 *mRawIndexTranslation;

   //data as stored in obj file
   uint32 mNumRawPosition;
   float32 *mRawPosition;
   uint32 mNumRawNormal;
   float32 *mRawNormal;
   uint32 mNumRawTexCoord;
   float32 *mRawTexCoord;
   uint32 mNumRawIndex;

   ObjReader(void);
   bool8 LoadObj(char8 *aFileName);
   uint32 LookupCreateIndex(uint32 rawPosIdx, uint32 rawNormIdx, uint32 rawTexCoordIdx);
   void CalculateTangentSpace(void);

};