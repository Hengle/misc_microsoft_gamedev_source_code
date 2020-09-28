#include "ugfout.h"

#include "common/math/quat.h"
#include "common/geom/indexed_tri.h"

#include <algorithm>

using namespace gr;

bool gExtendedLogInfo;

static const float cDefaultGamma = 2.0f;

// rg [9/15/06] - Just support lights for now.
#define SUPPORT_SPHERES 0
#define SUPPORT_CAMERA 1

bool UGFExporter::sampleControllerTimes(INode* pNode, TimeValueVec& timeVec)
{
   gLog.printf("Found keyframes object\n");

   Control *cont = pNode->GetTMController();
   Control *c = cont->GetPositionController();

   if (c)
   {
      const int numKeys = c->NumKeys();
      gLog.printf("Keys found: %i\n", numKeys);

      for (int i = 0; i < numKeys; i++)
      {
         const TimeValue time = c->GetKeyTime(i);
         timeVec.push_back(time);

         gLog.printf("[%i] Ticks: %i Frame: %f Secs: %f\n", 
            i, time, time / static_cast<float>(GetTicksPerFrame()), TicksToSec(time));
      }

      return true;
   }
   else
      gLog.printf("GetPositionController() returned NULL!\n");

   return false;
}

// true if "keyframes" object found
bool UGFExporter::enumNodeForKeyFrameObject(INode* pNode, TimeValueVec& timeVec, bool morph)
{
   const char* pName = morph ? "morphkeyframes" : "keyframes";

   if (strnicmp(pNode->GetName(), pName, strlen(pName)) == 0)
   {
      if (sampleControllerTimes(pNode, timeVec))
         return true;
   }

   for (int c = 0; c < pNode->NumberOfChildren(); c++) 
      if (enumNodeForKeyFrameObject(pNode->GetChildNode(c), timeVec, morph))
         return true;

   return false;
}

// true if "keyframes" pNode found
bool UGFExporter::findKeyFrameObject(
                                     TimeValueVec& timeVec, 
                                     Interface* ip,
                                     bool morph)
{
   const int numChildren = ip->GetRootNode()->NumberOfChildren();
   for (int i = 0; i < numChildren; i++)
      if (enumNodeForKeyFrameObject(ip->GetRootNode()->GetChildNode(i), timeVec, morph))
         return true;

   return false;
}

void UGFExporter::findSceneObjectsEnumNodes(INode* pNode, INodePtrVec& nodes, TimeValue time, bool onlySelected)
{
   if (((!onlySelected) || (pNode->Selected())) && (!pNode->IsNodeHidden()))
   {
      if (
         (IsNodeLight(pNode))  
#if SUPPORT_SPHERES         
         || (IsNodeSphere(pNode)) 
#endif
         || (IsNodeCamera(pNode))  
         )
      {
         gLog.printf("findSceneObjectsEnumNodes: Found node to sample: \"%s\"\n", pNode->GetName());
         
         nodes.push_back(pNode);
      }
               
      for (int c = 0; c < pNode->NumberOfChildren(); c++) 
         findSceneObjectsEnumNodes(pNode->GetChildNode(c), nodes, time, onlySelected);
   }
}

void UGFExporter::findSceneObjects(INodePtrVec& nodes, Interface* ip, TimeValue time, bool onlySelected)
{
   nodes.clear();

   const int numChildren = ip->GetRootNode()->NumberOfChildren();
   for (int i = 0; i < numChildren; i++)
      findSceneObjectsEnumNodes(ip->GetRootNode()->GetChildNode(i), nodes, time, onlySelected);
}

SceneData::Base& UGFExporter::createSceneDataBase(SceneData::Base& base, INode* pNode, TimeValue time)
{
#if 0
   base.mName = pNode->GetName();

   Matrix3 mat3NodeTM = pNode->GetNodeTM(time); 
   mat3NodeTM.NoScale();
   mat3NodeTM.Orthogonalize();
      
   Matrix44 orient(Matrix44::I);
   orient.setRow(0, Vec4(mat3NodeTM.GetRow(0).x, mat3NodeTM.GetRow(0).y, mat3NodeTM.GetRow(0).z, 0.0f));
   orient.setRow(1, Vec4(mat3NodeTM.GetRow(1).x, mat3NodeTM.GetRow(1).y, mat3NodeTM.GetRow(1).z, 0.0f));
   orient.setRow(2, Vec4(mat3NodeTM.GetRow(2).x, mat3NodeTM.GetRow(2).y, mat3NodeTM.GetRow(2).z, 0.0f));

   gr::Quat quat(orient);
   if (quat == gr::Quat::Z)
   {
      gLog.printf("createSceneDataBase: Can't convert orientation to quaternion!\n");
      quat = gr::Quat::I;
   }
   
   base.mOrient = Vec<4>(quat[0], quat[1], quat[2], quat[3]);
   base.mPos = Vec<3>(mat3NodeTM.GetRow(3).x, mat3NodeTM.GetRow(3).y, mat3NodeTM.GetRow(3).z);
#endif

   // rg [9/15/06] - This always exports the node relative to a parent bone that has "bone" in the name, or the root.
   // Yes this is a hack.
   INode* pParent = pNode->GetParentNode();
   
   for ( ; ; )
   {
      if (pParent->IsRootNode())
         break;
         
      BigString name(pParent->GetName());
      name.tolower();
      
      if (strstr(name.c_str(), "bone") != NULL)
         break;
      
      pParent = pParent->GetParentNode();
   }      

   // bone->model
   Matrix3 mat3NodeTM = pNode->GetNodeTM(time); 

   // parent_bone->model
   Matrix3 mat3ParentTM = pParent->GetNodeTM(time); 

   Matrix3 rawLocalTM = mat3NodeTM * Inverse(mat3ParentTM);
   
   rawLocalTM.NoScale();
   
   Vec<3> r0(rawLocalTM.GetRow(0).x, rawLocalTM.GetRow(0).y, rawLocalTM.GetRow(0).z);
   Vec<3> r1(rawLocalTM.GetRow(1).x, rawLocalTM.GetRow(1).y, rawLocalTM.GetRow(1).z);
   Vec<3> r2(rawLocalTM.GetRow(2).x, rawLocalTM.GetRow(2).y, rawLocalTM.GetRow(2).z);
   Vec<3> r3(rawLocalTM.GetRow(3).x, rawLocalTM.GetRow(3).y, rawLocalTM.GetRow(3).z);

   // bone->parent_bone = bone->model * model->parent_bone
   Point3 p, s;
   ::Quat q;

   DecomposeMatrix(rawLocalTM, p, q, s);
   
   base.mOrient = Vec<4>(q.x, q.y, q.z, q.w).normalize();
   base.mPos = Vec3(p.x, p.y, p.z);
      
   return base;
}

SceneData::Light UGFExporter::createSceneDataLight(INode* pNode, TimeValue time)
{
   ObjectState os = pNode->EvalWorldState(time);
   Verify(os.obj && os.obj->SuperClassID() == LIGHT_CLASS_ID);

   GenLight* pGenLight = (GenLight*)os.obj;
   
   SceneData::Light light;
   
   createSceneDataBase(light, pNode, time);
                 
   Texmap* tex = pGenLight->GetProjMap();
   if (tex)
   {
      if (tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00))
      {
         TSTR mapName = ((BitmapTex *)tex)->GetMapName();
         BigString filename = mapName;
         filename.removePath().removeExtension().tolower();
         light.mMask = filename;
      }
   }

   gLog.printf("Light \"%s\", Mask Texture \"%s\":\n",
      light.mName.c_str(),
      light.mMask.c_str());
   
   gLog.printf("Pos: %f %f %f, Rotation: %f %f %f %f\n",
      light.mPos[0], light.mPos[1], light.mPos[2],
      light.mOrient[0], light.mOrient[1], light.mOrient[2], light.mOrient[3]);

   gLog.printf("Type: ");
   switch (pGenLight->Type())
   {
      case OMNI_LIGHT:
      {
         light.mType = SceneData::eOmniLightType;
         gLog.printf("Omni\n");
         break;
      }
      case TSPOT_LIGHT:
      case FSPOT_LIGHT:
      {
         light.mType = SceneData::eSpotLightType;
         gLog.printf("Spot\n");
         break;
      }
      case DIR_LIGHT:
      case TDIR_LIGHT:
      {
         light.mType = SceneData::eDirLightType;
         gLog.printf("Dir\n");
         break;
      }
   }

/*
   if (pGenLight->Type() == OMNI_LIGHT)
   {
      light.mRadius = Math::Max<float>(1.0f, pGenLight->GetAtten(time, ATTEN_END));
   }
   else
   {
      light.mRadius = Math::Max<float>(1.0f, pGenLight->GetTDist(time));

      if ((pGenLight->Type() == TSPOT_LIGHT) || (pGenLight->Type() == TDIR_LIGHT))
      {
         INode* pTarget = pNode->GetTarget();
         if (pTarget) 
         {
            Point3 src = pNode->GetNodeTM(time).GetTrans();
            Point3 dst = pTarget->GetNodeTM(time).GetTrans();

            light.mRadius = Math::Max<float>(1.0f, (dst - src).Length());
         }
      }
   }
*/
   
   const float attenStart = Math::Min(pGenLight->GetAtten(time, ATTEN_START), pGenLight->GetAtten(time, ATTEN_END));
   const float attenEnd = Math::Max(pGenLight->GetAtten(time, ATTEN_START), pGenLight->GetAtten(time, ATTEN_END));
            
   light.mRadius = Math::Max(1.0f, attenEnd); 
   light.mFarAttenStart = Math::Clamp<float>(attenStart / light.mRadius, 0.0f, 1.0f);
   
   float decayDist = pGenLight->GetDecayRadius(time);
   int decayType = pGenLight->GetDecayType();

   light.mDecayDist = Math::Max(0.0f, decayDist);
   light.mDecayType = static_cast<SceneData::EDecayType>(decayType);
   
   light.mFog = (0 != pGenLight->GetAtmosShadows(time));
   light.mFogShadows = (pGenLight->GetAtmosColAmt(time) < .75f) && light.mFog;
   light.mFogDensity = pGenLight->GetAtmosOpacity(time);
   if (pGenLight->GetLightAffectsShadow())
      light.mLightBuffered = false;
         
   gLog.printf("Radius: %f, Far Atten Start: %f, Decay Dist: %f, Decay Type: %i Fog: %i FogShadows: %i FogDensity: %f LightBuffered: %i\n", 
      light.mRadius, 
      light.mFarAttenStart,
      light.mDecayDist, 
      light.mDecayType,
      light.mFog,
      light.mFogShadows,
      light.mFogDensity,
      light.mLightBuffered);

   float spotInner = Math::fDegToRad(Math::Clamp<float>(pGenLight->GetHotspot(time), 1.0f, 160.0f));
   float spotOuter = Math::fDegToRad(Math::Clamp<float>(pGenLight->GetFallsize(time), 1.0f, 160.0f));

   light.mSpotInner = Math::Min(spotInner, spotOuter);
   light.mSpotOuter = Math::Max(spotInner, spotOuter);
      
   gLog.printf("Spot Falloff Inner: %f, Outer: %f\n", light.mSpotInner, light.mSpotOuter);

   light.mShadows = pGenLight->GetShadow() != 0;
   light.mDiffuse = pGenLight->GetAffectDiffuse() != 0;
   light.mSpecular = pGenLight->GetAffectSpecular() != 0;
   
   gLog.printf("Shadows: %i, Diffuse: %i, Specular: %i\n", light.mShadows, light.mDiffuse, light.mSpecular);
         
   light.mIntensity = pGenLight->GetIntensity(time);
   
   const Point3 color = pGenLight->GetRGBColor(time);
   light.mColor[0] = pow(color.x, cDefaultGamma);
   light.mColor[1] = pow(color.y, cDefaultGamma);
   light.mColor[2] = pow(color.z, cDefaultGamma);
      
   gLog.printf("Linear intensity: %f, Linear color: %f %f %f\n", light.mIntensity, light.mColor[0], light.mColor[1], light.mColor[2]);
   
   return light;
}

SceneData::Object UGFExporter::createSceneDataObject(INode* pNode, TimeValue time, bool firstFrame)
{
   Verify(IsNodeSphere(pNode));
   
   SceneData::Object object;

   createSceneDataBase(object, pNode, time);

   gLog.printf("Object \"%s\":\n", object.mName.c_str());
   
   gLog.printf("Pos: %f %f %f, Rotation: %f %f %f %f\n",
      object.mPos[0], object.mPos[1], object.mPos[2],
      object.mOrient[0], object.mOrient[1], object.mOrient[2], object.mOrient[3]);
      
   object.mSphereRadius = GetSphereRadius(pNode, time);
   
   gLog.printf("Sphere radius: %f\n", object.mSphereRadius);
   
   if (firstFrame)
      GetNodeUDP(pNode, object.mUDP);
      
   Control* visCont = pNode->GetVisController();
   if (visCont)
   {
      float value;
      visCont->GetValue(time, &value, FOREVER);
      
      object.mVisibility = value;//Math::Clamp(value, 0.0f, 1.0f);
            
      gLog.printf("Visibility: %f\n", object.mVisibility);
   }
      
   return object;
}

SceneData::Camera UGFExporter::createSceneDataCamera(INode* pNode, TimeValue time, bool firstFrame)
{
   Verify(IsNodeCamera(pNode));
   
   SceneData::Camera camera;
   
   createSceneDataBase(camera, pNode, time);
   
   CameraState cs;
  
   ObjectState os = pNode->EvalWorldState(time);
   CameraObject* cam = (CameraObject *)os.obj;
   
   cam->EvalCameraState(time, FOREVER, &cs);    
   
   camera.mFov = cs.fov;
   camera.mNearRange = cs.nearRange;
   camera.mFarRange = cs.farRange;
   camera.mFocalDepth = cs.tdist;

   if(cs.manualClip != 0)
   {
		camera.mNearClip = cs.hither;
		camera.mFarClip = cs.yon;
   }
   else
   {
		camera.mNearClip = 0.0f;
		camera.mFarClip = 0.0f;
   }
   
   gLog.printf("Camera \"%s\" FOV: %f FocalDepth: %f EnvNearRange: %f EnvFarRange: %f NearClip: %f FarClip: %f\n", pNode->GetName(), camera.mFov, camera.mFocalDepth, camera.mNearRange, camera.mFarRange, camera.mNearClip, camera.mFarClip);
   
   return camera;
}

void UGFExporter::sampleSceneAtTime(SceneData::Scene& scene, TimeValue time, INodePtrVec& nodes, bool firstFrame)
{
   gLog.printf("\nsampleSceneAtTime: Sampling scene at TimeValue: %i Frame: %f Secs: %f\n", 
      time, time / static_cast<float>(GetTicksPerFrame()), TicksToSec(time));

   SceneData::Frame frame(TicksToSec(time));

   for (int i = 0; i < nodes.size(); i++)
   {
      INode* pNode = nodes[i];
      if (IsNodeLight(pNode))
         frame.insertLight(createSceneDataLight(pNode, time));
      else if (IsNodeSphere(pNode))
         frame.insertObject(createSceneDataObject(pNode, time, firstFrame));
      else if (IsNodeCamera(pNode))
         frame.insertCamera(createSceneDataCamera(pNode, time, firstFrame));
   }

   scene.insertFrame(frame);
}

void UGFExporter::sampleScene(SceneData::Scene& scene, TimeValueVec timeVec, INodePtrVec& nodes)
{
   gLog.printf("sampleScene: Found %i nodes to sample\n", nodes.size());

   if (!nodes.empty())
   {
      for (int i = 0; i < timeVec.size(); i++)
         sampleSceneAtTime(scene, timeVec[i], nodes, (i == 0));
   }
}

void UGFExporter::createUniformKeyFrameTimes(
   TimeValueVec& timeVec, 
   int startFrame, int endFrame, int frameInterval)
{
   int curFrame = startFrame;
   for ( ; ; )
   {
      timeVec.push_back(curFrame * GetTicksPerFrame());
      if (curFrame >= endFrame)
         break;
      curFrame = Math::Min(curFrame + frameInterval, endFrame);
   }
}

bool UGFExporter::exportLights(
   Stream& stream, 
   Interface* ip, 
   int startFrame, 
   int endFrame, 
   int frameInterval, 
   bool onlySelected)
{
   SceneData::Scene scene;

   TimeValueVec timeVec;
   if (!findKeyFrameObject(timeVec, ip, false))
      createUniformKeyFrameTimes(timeVec, startFrame, endFrame, frameInterval);

   INodePtrVec nodes;
   findSceneObjects(nodes, ip, startFrame * GetTicksPerFrame(), onlySelected);
            
   sampleScene(scene, timeVec, nodes);

   return stream << scene;
}


