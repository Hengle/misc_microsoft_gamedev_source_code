//============================================================================
//
//  vertexDeclManager.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "vertexDeclManager.h"
#include "vertexDeclUtils.h"

#define LOG_VERTEX_DECLS 0

BVertexDeclManager gVertexDeclManager;

BVertexDecl::BVertexDecl() :
   mpDecl(NULL)
{
}

BVertexDecl::BVertexDecl(const BUnivertPackerArray& packers) :
   mpDecl(NULL),
   mUnivertPackers(packers)
{
   create();
}

BVertexDecl::BVertexDecl(const BVertexDecl& rhs) :
   mpDecl(NULL)
{
   *this = rhs;
}

BVertexDecl::~BVertexDecl()
{
   clear();
}

BVertexDecl& BVertexDecl::operator= (const BVertexDecl& rhs)
{
   if (this == &rhs)
      return *this;

   clear();

   mUnivertPackers = rhs.mUnivertPackers;

   create();

   return *this;
}

bool BVertexDecl::operator== (const BVertexDecl& rhs) const
{
   return *this == rhs.mUnivertPackers;
}

bool BVertexDecl::operator== (const BUnivertPackerArray& packers) const
{
   if (mUnivertPackers.size() != packers.size())
      return false;
   for (uint i = 0; i < mUnivertPackers.size(); i++)
      if (mUnivertPackers[i] != packers[i])
         return false;
   return true;
}

IDirect3DVertexDeclaration9* BVertexDecl::getDecl(void) const 
{
   return mpDecl; 
}

void BVertexDecl::setToDevice(IDirect3DDevice9* pDev) const
{  
   pDev->SetVertexDeclaration(getDecl());   
}

// Creates a canonicalized vertex declaration.
// Note that not every possible vertex format configuration is supported!
void BVertexDecl::create(void)
{
   BD3D::safeRelease(mpDecl);        
   
   if (mUnivertPackers.empty())
      return;

   const int numStreams = static_cast<int>(mUnivertPackers.size());
   BASSERT((numStreams >= 1) && (numStreams <= MaxStreams));

   const int MaxElements = 64;                  
   D3DVERTEXELEMENT9 elements[MaxElements];
   Utils::ClearObj(elements);
   
   D3DVERTEXELEMENT9* pNext = elements;
                                       
   for (int streamIndex = 0; streamIndex < numStreams; streamIndex++)
   {
      const BUnpackedUnivertPackerType& packer = mUnivertPackers[streamIndex];
                        
      const char* pDeclOrder = packer.declOrder();
      while (*pDeclOrder)
      {
         const char c = (char)toupper(*pDeclOrder++);
         
         const int origIndex = Utils::ConvertHexChar(*pDeclOrder++);
         const int usageIndex = Utils::ConvertHexChar(*pDeclOrder++);
         
         BASSERT(origIndex >= 0);
         BASSERT(usageIndex >= 0);
                  
         pNext->Stream = (WORD)streamIndex;
         pNext->Method = D3DDECLMETHOD_DEFAULT;
         pNext->UsageIndex = (BYTE)usageIndex;
                        
         switch (c)
         {
            // POS and NORM may appear once per stream.
            case ePOS_SPEC:
            {
               pNext->Type = convertType(packer.pos());
               pNext->Usage = D3DDECLUSAGE_POSITION;
               break;
            }
            case eNORM_SPEC:
            {
               pNext->Type = convertType(packer.norm());
               pNext->Usage = D3DDECLUSAGE_NORMAL;
               break;
            }
            // BASIS may appear multiple times per stream, in more than one stream.
            case eBASIS_SPEC:
            {
               pNext->Type = convertType(packer.basis());
               pNext->Usage = D3DDECLUSAGE_TANGENT;
               pNext++;
               
               pNext->Stream = (WORD)streamIndex;
               pNext->Method = D3DDECLMETHOD_DEFAULT;
               pNext->Type = convertType(packer.basis());
               pNext->Usage = D3DDECLUSAGE_BINORMAL;
               pNext->UsageIndex = (BYTE)usageIndex;
               
               break;
            }
            // BASIS_SCALE may appear multiple times in one stream.
            // Each BASIS_SCALE is assigned to texcoords, starting at BasisScalesFirstTexcoordIndex.
            case eBASIS_SCALE_SPEC:
            {
               pNext->Type = convertType(packer.basisScales());
               pNext->Usage = D3DDECLUSAGE_TEXCOORD;
               break;
            }
            // TANGENT may appear multiple times per stream, in more than one stream.
            case eTANGENT_SPEC:
            {
               pNext->Type = convertType(packer.tangent());
               pNext->Usage = D3DDECLUSAGE_TANGENT;
               break;
            }
            // TEXCOORDS may appear multiple times in one stream.
            case eTEXCOORDS_SPEC:
            {
               pNext->Type = convertType(packer.UV(origIndex));
               pNext->Usage = D3DDECLUSAGE_TEXCOORD;
               break;
            }
            // SKIN may appear one time in a stream.
            case eSKIN_SPEC:
            {
               pNext->Type = convertType(packer.indices());
               pNext->Usage = D3DDECLUSAGE_BLENDINDICES;
               pNext++;
               
               pNext->Stream = (WORD)streamIndex;
               pNext->Method = D3DDECLMETHOD_DEFAULT;
               pNext->Type = convertType(packer.weights());
               pNext->Usage = D3DDECLUSAGE_BLENDWEIGHT;
               pNext->UsageIndex = (BYTE)usageIndex;
               break;
            }
            // DIFFUSE may appear one time in a stream.
            case eDIFFUSE_SPEC:
            {
               pNext->Type = convertType(packer.diffuse());
               pNext->Usage = D3DDECLUSAGE_COLOR;
               break;
            }
            // INDEX may appear one time in a stream.
            case eINDEX_SPEC:
            {
               pNext->Type = convertType(packer.index());
               pNext->Usage = D3DDECLUSAGE_BLENDINDICES;
               break;
            }
            default:
               BASSERT(false);
         }
         
         pNext++;
      }
   }
   
   const D3DVERTEXELEMENT9 endDecl = {0xFF, 0, (DWORD)D3DDECLTYPE_UNUSED, 0, 0, 0};
   *pNext++ = endDecl;
   
   BASSERT((pNext - elements)  <= MaxElements);
   
   BVertexDeclUtils::setVertexDeclarationOffsets(elements);

#if LOG_VERTEX_DECLS      
   BVertexDeclUtils::dumpVertexDeclaration(BTraceTextDispatcher(), elements);
#endif      
   
   BD3D::checkHResult(BD3D::mpDev->CreateVertexDeclaration(elements, &mpDecl), "CreateVertexDeclaration failed");
}

DWORD BVertexDecl::convertType(VertexElement::EType type)
{
   switch (type)
   {
      case VertexElement::eFLOAT1:        return D3DDECLTYPE_FLOAT1;
      case VertexElement::eFLOAT2:        return D3DDECLTYPE_FLOAT2;
      case VertexElement::eFLOAT3:        return D3DDECLTYPE_FLOAT3;
      case VertexElement::eFLOAT4:        return D3DDECLTYPE_FLOAT4;
      case VertexElement::eD3DCOLOR:      return D3DDECLTYPE_D3DCOLOR;
      case VertexElement::eUBYTE4:        return D3DDECLTYPE_UBYTE4;
      case VertexElement::eSHORT2:        return D3DDECLTYPE_SHORT2;
      case VertexElement::eSHORT4:        return D3DDECLTYPE_SHORT4;
      case VertexElement::eUBYTE4N:       return D3DDECLTYPE_UBYTE4N;
      case VertexElement::eSHORT2N:       return D3DDECLTYPE_SHORT2N;
      case VertexElement::eSHORT4N:       return D3DDECLTYPE_SHORT4N;
      case VertexElement::eUSHORT2N:      return D3DDECLTYPE_USHORT2N;
      case VertexElement::eUSHORT4N:      return D3DDECLTYPE_USHORT4N;
      case VertexElement::eUDEC3N:        return D3DDECLTYPE_UDEC3;
      case VertexElement::eUDEC3:         return D3DDECLTYPE_UDEC3; 
      case VertexElement::eDEC3N:         return D3DDECLTYPE_DEC3N;
      case VertexElement::eHALFFLOAT2:    return D3DDECLTYPE_FLOAT16_2;
      case VertexElement::eHALFFLOAT4:    return D3DDECLTYPE_FLOAT16_4;
      case VertexElement::eDHEN3N:        return D3DDECLTYPE_DHEN3N;
   }
   
   BVERIFY(false);
   return 0;
}

void BVertexDecl::clear(void)
{
   if (mpDecl)
   {
      mpDecl->Release();
      mpDecl = NULL;
   }
   mUnivertPackers.clear();
}
      
void BVertexDeclManager::clear(void)
{
   mVertexDeclCache.resize(0);
}

BVertexDeclManager::BVertexDeclManager() 
{
   clear();
   
   mVertexDeclCache.reserve(128);
}

BVertexDeclManager::~BVertexDeclManager()
{
}

const BVertexDeclManager::Handle BVertexDeclManager::create(const BUnivertPackerArray& packers)
{
   for (uint i = 0; i < mVertexDeclCache.size(); i++)
      if (mVertexDeclCache[i] == packers)
         return i;

#if LOG_VERTEX_DECLS 
   trace("BVertexDeclManager::create: Cache miss, %i entries\n", mVertexDeclCache.size());
#endif
   
   mVertexDeclCache.push_back(BVertexDecl(packers));
               
   return mVertexDeclCache.size() - 1;
}

void BVertexDeclManager::setToDevice(BVertexDeclManager::Handle handle, IDirect3DDevice9* pDev) const
{
   mVertexDeclCache[debugRangeCheck(handle, mVertexDeclCache.size())].setToDevice(pDev);
}
