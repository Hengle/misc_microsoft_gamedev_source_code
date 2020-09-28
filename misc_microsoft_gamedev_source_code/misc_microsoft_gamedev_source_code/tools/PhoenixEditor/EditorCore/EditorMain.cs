using System;
using System.Collections.Generic;
using System.Text;

using Microsoft.Ink;
using Microsoft.StylusInput;
using Microsoft.StylusInput.PluginData;
using Microsoft.DirectX;

using System.Windows.Forms;
using System.Drawing;

namespace EditorCore
{


   public interface IEditor
   {
      void input();
      void render();
      void update();
   }

   public interface ITerrain
   {
      bool intersectPointOnTerrainSimRep(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt);
      bool intersectPointOnTerrain(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt);
      bool intersectPointOnTerrain(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal, ref int X, ref int Z);
      float getSimRepHeightInterpolated(ref Vector3 origin);

      Vector3 getTerrainPos(int x, int z);
      float getTerrainHeight(int x, int z);

      void setCameraPos(Vector3 pos);
      void setCameraTarget(Vector3 target);
      Vector3 getCameraPos();
      Vector3 getCameraTarget();



      BFrustum getFustrum();

      void simEditorLightMoved(int simEditorObjectIndex);
      void simEditorLightAdd(int simEditorObjectIndex);
      void simEditorLightDelete(int simEditorObjectIndex);

      float getWorldSizeX();
      float getWorldSizeZ();

      Vector3 getBBMin();
      Vector3 getBBMax();

      void getMaskTileBounds(ref int minX, ref int maxX, ref int minZ, ref int maxZ);
      bool isVertexMasked(int x, int z);
      IMask getMask();
      IMask getBaseMask();
      int getNumXVerts();

   }

   public interface IMainWindow
   {
      void ShowDialog(string name);
      //void ShowDialog(string name, BaseClientPage owner, bool bShared);

      void AddToErrorList(string text);
      void deviceResize(int width, int height, bool force);
      BaseClientPage getActiveClientPage();
      Point getWindowLocation();
      void SetClientFocus();
      void ShowScenarioOptions();

      void ReloadVisibleTextureThumbnails(bool selectdIndexChanged);
      void roadSelectionCallback();

      void setTerrainMemoryPercent(float val);
      void setMemoryEstimateString(string msg);
      void setCursorLocationLabel(float x, float y, float z);
      void clearCursorLocationLabel();
      void doQuickView();
      void doQuickSave(string name);
      void afterQuickView();
   }

   public interface IPhoenixScenarioEditor
   {
      void ReloadVisibleObjects();
      void HandleCommand(string commandName);
      void UpdateSliders();
      bool CheckTopicPermission(string topicName);
   }


   public class BEditorMain 
   {
      eMainMode mMainMode;
      public enum eMainMode
      {
         cSim,
         cTerrain

      }
      public eMainMode MainMode
      {
         set
         {
            mMainMode = value;
         }
         get
         {
            return mMainMode;
         }


      }

      public ITerrain mITerrainShared = null;
      public IMainWindow mIGUI = null;
      public IMaskPickerUI mIMaskPickerUI = null;
      public IPhoenixScenarioEditor mPhoenixScenarioEditor = null;

      public bool mOneFrame = false; //bump forward one frame.

      public void Register(object sharedFunctionality)
      {
         if(sharedFunctionality is ITerrain)
         {
            mITerrainShared = (ITerrain)sharedFunctionality;
         }
         if (sharedFunctionality is IMainWindow)
         {
            mIGUI = (IMainWindow)sharedFunctionality;
         }
         if(sharedFunctionality is IMaskPickerUI)
         {
            mIMaskPickerUI = (IMaskPickerUI)sharedFunctionality;
         }
         if(sharedFunctionality is IPhoenixScenarioEditor)
         {
            mPhoenixScenarioEditor = (IPhoenixScenarioEditor)sharedFunctionality;
         }
      }

      //used for communication between worker threads and the UI owning thread.
      public ThreadSafeMessageList mMessageList = new ThreadSafeMessageList();
   }

   public class ThreadSafeMessageList
   {
      public delegate void MessagePacketCallback(string messageID, object objData);

      public class MessagePacket
      {
         public MessagePacket()
         {
            mProducerThreadID = System.Threading.Thread.CurrentThread.ManagedThreadId;
         }
         private int mProducerThreadID = -1;  //useful for debugging

         public string mMessageID="";
         public MessagePacketCallback mCallback = null;
         public object mDataObject = null;
      };

      ~ThreadSafeMessageList()
      {
         mMessageQueue = null;
      }

      protected System.Collections.Queue mMessageQueue = System.Collections.Queue.Synchronized(new System.Collections.Queue());

      public void enqueueMessage(MessagePacket packet)
      {
         mMessageQueue.Enqueue(packet);
      }

      public MessagePacket dequeueMessage()
      {
         return (MessagePacket)mMessageQueue.Dequeue();
      }

      public void dequeueProcessMessage()
      {
         MessagePacket mp = dequeueMessage();
         if(mp!=null && mp.mCallback!=null)
         {
            mp.mCallback(mp.mMessageID,mp.mDataObject);
         }
      }
      public int Count()
      {
         return mMessageQueue.Count;
      }
   };




}
