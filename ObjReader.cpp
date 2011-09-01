//--------------------------------------------------------------------------------------
//lightwave .Obj file reader
//
//note: this code only handles polys with 30 or fewer sides
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------

#include "ObjReader.h"
#include "VectorMacros.h"

ObjReader::ObjReader(void)
{
   mNumIndex = 0;
   mNumVertex= 0;
   mIndex = NULL;   
   mPosition = NULL;
   mNormal = NULL;
   mTexCoord = NULL;

   mRawIndexTranslation = NULL;

   mNumRawPosition = 0;
   mNumRawNormal = 0;
   mNumRawTexCoord = 0;
   mNumRawIndex = 0;

   mRawPosition = NULL;
   mRawNormal = NULL;
   mRawTexCoord = NULL;
}


bool8 ObjReader::LoadObj(char8 *aFilename)
{
   FILE *ifp;
   char8 ch1, ch2, charBuffer[4096], valStr[20][4096];
   uint32 numRead;
   uint32 rawPositionIdx = 0;
   uint32 rawNormalIdx = 0;
   uint32 rawTexCoordIdx = 0;
   uint32 i;

   //first pass to count number of elements in each array
   errno_t result = fopen_s( &ifp, aFilename, "r" );

   if( result != 0 )
   {
       return false;
   }

   while (!feof(ifp))
   {
      fgets(charBuffer, 4096, ifp);
      numRead = sscanf_s(charBuffer, "%c%c%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n", &ch1, &ch2, 
         &valStr[0][0], &valStr[1][0], &valStr[2][0], &valStr[3][0], &valStr[4][0],
         &valStr[5][0], &valStr[6][0], &valStr[7][0], &valStr[8][0], &valStr[9][0],
         &valStr[10][0], &valStr[11][0], &valStr[12][0], &valStr[13][0], &valStr[14][0],
         &valStr[15][0], &valStr[16][0], &valStr[17][0], &valStr[18][0], &valStr[19][0]         
         );

      switch(ch1)
      {
         case 'g':
         break;
         case 'v':
            if(ch2 == ' ')
            {
               mNumRawPosition++;
            }
            else if(ch2 == 'n')
            {
               mNumRawNormal++;
            }
            else if(ch2 == 't')
            {
               mNumRawTexCoord++;
            }
         break;
         case 'f':
            mNumRawIndex += (numRead - 4) * 3;
         break;      
      }
   }

   //second pass to load in raw data
   fseek(ifp, 0, SEEK_SET);

   rawPositionIdx = 0;
   rawNormalIdx = 0;
   rawTexCoordIdx = 0;

   mRawPosition  = new float32 [mNumRawPosition * 3];
   mRawNormal  = new float32 [mNumRawNormal * 3];
   mRawTexCoord  = new float32 [mNumRawTexCoord * 2];
   mIndex  = new uint32 [mNumRawIndex];

   //for now allocate max size, and end of index condensing, reallocate arrays to optimal size
   mPosition = new float32 [mNumRawIndex * 3];
   mNormal = new float32 [mNumRawIndex * 3];
   mTexCoord = new float32 [mNumRawIndex * 2];
   
   mRawIndexTranslation = new uint32 [mNumRawIndex * 3];

   while (!feof(ifp))
   {
      fgets(charBuffer, 4096, ifp);
      //numRead = sscanf(charBuffer, "%c%c%s%s%s%s\n", &ch1, &ch2, &valStr[0][0], &valStr[1][0], &valStr[2][0], &valStr[3][0]);
      numRead = sscanf_s(charBuffer, "%c%c%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n", &ch1, &ch2, 
         &valStr[0][0], &valStr[1][0], &valStr[2][0], &valStr[3][0], &valStr[4][0],
         &valStr[5][0], &valStr[6][0], &valStr[7][0], &valStr[8][0], &valStr[9][0],
         &valStr[10][0], &valStr[11][0], &valStr[12][0], &valStr[13][0], &valStr[14][0],
         &valStr[15][0], &valStr[16][0], &valStr[17][0], &valStr[18][0], &valStr[19][0]         
         );

      
      switch(ch1)
      {
         case 'g':
         break;
         case 'v':
            if(ch2 == ' ')
            {
               mRawPosition [rawPositionIdx * 3 + 0] = (float32) atof(&valStr[0][0]);
               mRawPosition [rawPositionIdx * 3 + 1] = (float32) atof(&valStr[1][0]);
               mRawPosition [rawPositionIdx * 3 + 2] = (float32) atof(&valStr[2][0]);
               rawPositionIdx ++;
            }
            else if(ch2 == 'n')
            {
               mRawNormal [rawNormalIdx * 3 + 0] = (float32) atof(&valStr[0][0]);
               mRawNormal [rawNormalIdx * 3 + 1] = (float32) atof(&valStr[1][0]);
               mRawNormal [rawNormalIdx * 3 + 2] = (float32) atof(&valStr[2][0]);
               rawNormalIdx ++;
            }
            else if(ch2 == 't')
            {
               mRawTexCoord [rawTexCoordIdx * 2 + 0] = (float32) atof(&valStr[0][0]);
               mRawTexCoord [rawTexCoordIdx * 2 + 1] = (float32) atof(&valStr[1][0]);
               rawTexCoordIdx ++;
            }
         break;
         case 'f':
            //pack raw indices into array
            uint32 rawIndices[OR_MAX_FACE_VERTS][3];

            if(numRead < 5)
            {
            //error: face has less than 3 sides
            break;
            }

            for(i = 0; i < (numRead - 2); i++)
            {
               sscanf_s(&valStr[i][0], "%d/%d/%d", &rawIndices[i][0], &rawIndices[i][1], &rawIndices[i][2] ); 
               
               //convert indices to zero base instead of 1 base
               rawIndices[i][0]--;
               rawIndices[i][1]--;
               rawIndices[i][2]--;
            }

            for(i = 0; i < (numRead - 4); i++) //convert face polygon into a triangle fan e.g. (0,1,2) (0,2,3) (0,3,4) etc..
            {  //note that the first index looks funny (0, i+1, i+2) but is correct..
               mIndex [mNumIndex + 0] = LookupCreateIndex(rawIndices[0][0], rawIndices[0][1], rawIndices[0][2]);
               mIndex [mNumIndex + 1] = LookupCreateIndex(rawIndices[1+i][0], rawIndices[1+i][1], rawIndices[1+i][2]);
               mIndex [mNumIndex + 2] = LookupCreateIndex(rawIndices[2+i][0], rawIndices[2+i][1], rawIndices[2+i][2]);              
               mNumIndex += 3;
            }
         break;      
      }
   }

   //delete unnecessary raw data arrays
   delete [] mRawIndexTranslation;
   delete [] mRawPosition;
   delete [] mRawNormal;
   delete [] mRawTexCoord;

   CalculateTangentSpace();

   return TRUE;
}


/*******************************************************************************************
Looks up the final index using the three array raw data indices, if the index does not 
exist, create a new vertex.
********************************************************************************************/
uint32 ObjReader::LookupCreateIndex(uint32 rawPosIdx, uint32 rawTexCoordIdx, uint32 rawNormIdx)
{
   uint32 i;

   for(i = 0; i < mNumVertex; i++)
   {
      if((rawPosIdx == mRawIndexTranslation[i * 3 + 0] ) &&
         (rawNormIdx == mRawIndexTranslation[i * 3 + 1] ) &&
         (rawTexCoordIdx == mRawIndexTranslation[i * 3 + 2] )
         ) 
      {
         return i;
      }
   }

   //add new vertex and index
   mPosition [mNumVertex * 3 + 0] = mRawPosition [rawPosIdx * 3 + 0];
   mPosition [mNumVertex * 3 + 1] = mRawPosition [rawPosIdx * 3 + 1];
   mPosition [mNumVertex * 3 + 2] = mRawPosition [rawPosIdx * 3 + 2]; 

   mTexCoord [mNumVertex * 2 + 0] = mRawTexCoord [rawTexCoordIdx * 2 + 0];
   mTexCoord [mNumVertex * 2 + 1] = mRawTexCoord [rawTexCoordIdx * 2 + 1];

   mNormal [mNumVertex * 3 + 0] = mRawNormal [rawNormIdx * 3 + 0];
   mNormal [mNumVertex * 3 + 1] = mRawNormal [rawNormIdx * 3 + 1];
   mNormal [mNumVertex * 3 + 2] = mRawNormal [rawNormIdx * 3 + 2]; 

   mRawIndexTranslation[mNumVertex * 3 + 0] = rawPosIdx;
   mRawIndexTranslation[mNumVertex * 3 + 1] = rawNormIdx;
   mRawIndexTranslation[mNumVertex * 3 + 2] = rawTexCoordIdx;

   mNumVertex++;
   return (mNumVertex - 1);
}


/**************************************************************
Calculates tangent space vectors for the .obj
**************************************************************/
void ObjReader::CalculateTangentSpace(void)
{
   uint32 triIndex[3];
   float32 vec3PosEdge0[3];
   float32 vec3PosEdge1[3];
   float32 vec2TexEdge0[2];
   float32 vec2TexEdge1[2];
   float32 f32TanWeight[2];
   float32 f32Det;
   float32 vec3Tmp0[3];
   float32 vec3Tmp1[3];
   uint32 i, k;

 //  perTriTangentU = new float32 [mNumIndex / 3 * 3];
 //  perTriTangentV = new float32 [mNumIndex / 3 * 3];

   mTangentU = new float32 [mNumVertex * 3];
   mTangentV = new float32 [mNumVertex * 3];

   //zero out tangent space vectors
   for(i = 0; i < mNumVertex * 3; i++)
   {
      mTangentU[i] = 0.0f;
      mTangentV[i] = 0.0f;
   }

   //loop over triangles to build up a tangent and binormal per vertex
   // for the 3 verts associated with the triangle, the tangents are accumulated
   // per vertex 

   for(i = 0; i < (mNumIndex / 3); i++)
   {
      for(k = 0; k < 3; k++)
      {
         triIndex[k] = mIndex[i * 3 + k];
      }

      //calc edge positon and tex coord vectors
      VM_SUB3(vec3PosEdge0, &mPosition[triIndex[1] * 3], &mPosition[triIndex[0] * 3]);
      VM_SUB3(vec3PosEdge1, &mPosition[triIndex[2] * 3], &mPosition[triIndex[0] * 3]);

      VM_SUB2(vec2TexEdge0, &mTexCoord[triIndex[1] * 2], &mTexCoord[triIndex[0] * 2]);
      VM_SUB2(vec2TexEdge1, &mTexCoord[triIndex[2] * 2], &mTexCoord[triIndex[0] * 2]);

      /*
      Solving linear equation for tangent space u:

        |du0  du1||w0| = |1| 
        |dv0  dv1||w1|   |0|
      */
      f32Det=(vec2TexEdge0[0] * vec2TexEdge1[1]) - (vec2TexEdge1[0] * vec2TexEdge0[1]);

      f32TanWeight[0] = vec2TexEdge1[1] / f32Det; 
      f32TanWeight[1] = -vec2TexEdge0[1] / f32Det; 

      VM_SCALE3(vec3Tmp0, vec3PosEdge0, f32TanWeight[0]);
      VM_SCALE3(vec3Tmp1, vec3PosEdge1, f32TanWeight[1]);
      VM_ADD3(vec3Tmp0, vec3Tmp0, vec3Tmp1);

      //accumulate tangent space U vectors
      VM_ADD3(&mTangentU[triIndex[0] * 3], &mTangentU[triIndex[0] * 3], vec3Tmp0);
      VM_ADD3(&mTangentU[triIndex[1] * 3], &mTangentU[triIndex[1] * 3], vec3Tmp0);
      VM_ADD3(&mTangentU[triIndex[2] * 3], &mTangentU[triIndex[2] * 3], vec3Tmp0);

      /*********************************************************
      Solving linear equation for tangent space V: 

        |du0  du1||w0| = |0| 
        |dv0  dv1||w1|   |1|

        re-use previous determinant...
      *********************************************************/

      f32TanWeight[0] = -vec2TexEdge1[0] / f32Det; 
      f32TanWeight[1] = vec2TexEdge0[0] / f32Det; 

      VM_SCALE3(vec3Tmp0, vec3PosEdge0, f32TanWeight[0]);
      VM_SCALE3(vec3Tmp1, vec3PosEdge1, f32TanWeight[1]);
      VM_ADD3(vec3Tmp0, vec3Tmp0, vec3Tmp1);

      //accumulate tangent space V vectors
      VM_ADD3(&mTangentV[triIndex[0] * 3], &mTangentV[triIndex[0] * 3], vec3Tmp0);
      VM_ADD3(&mTangentV[triIndex[1] * 3], &mTangentV[triIndex[1] * 3], vec3Tmp0);
      VM_ADD3(&mTangentV[triIndex[2] * 3], &mTangentV[triIndex[2] * 3], vec3Tmp0);
   }

   //normalize tangent space vectors
   for(i = 0; i < mNumVertex; i++)
   {
      VM_NORM3(&mTangentU[i * 3], &mTangentU[i * 3]);
      VM_NORM3(&mTangentV[i * 3], &mTangentV[i * 3]);
   }

}

