using System.Collections.Generic;

namespace Terrain
{
   public delegate void UndoManagerCallbackType(UndoInstanceData undoDataPacket);

   public class UndoInstanceData
   {
      public int mMemorySize = 0;
   }

   public class UndoPacket
   {
      public UndoInstanceData mCurrData = null;
      public UndoInstanceData mPrevData = null;
      public UndoManagerCallbackType mCallbackFunction = null;
     
   };


   class UndoManager
   {

      //remove all undos from the list
      public static void clearAllUndos()
      {
         mUndoPackets.Clear();
         mCurrMemInBytes = 0;
      }

      //this will add a new undo to the list
      public static void pushUndo(UndoPacket pkt)
      {
         if (mCurrUndoStep < mUndoPackets.Count-1)
         {
            clearAllUndos();// mUndoPackets.Clear();
            /*
            //we're in the middle of an undo-redo chain
            //clear everything above us, for new input
            if (mCurrUndoStep ==0)
               mUndoPackets.Clear();
            else
            {
               int strt = mCurrUndoStep -1;
               int range = mUndoPackets.Count - mCurrUndoStep;// +1;
               mUndoPackets.RemoveRange(strt,range);
            }
             * */
         }

         //CHECK IF WE NEED TO CLEAN MEMORY TO ADD THIS!
         
         int currMem = pkt.mCurrData.mMemorySize + pkt.mPrevData.mMemorySize + mCurrMemInBytes;
         //if our input is larger than the rest, in general
         if (pkt.mCurrData.mMemorySize + pkt.mPrevData.mMemorySize >= mMaxMemoryInBytes)
         {
            clearAllUndos();
         }
         else if (currMem >= mMaxMemoryInBytes)
         {
            
            //drop the last undos until we get enough room
            for(int i=mUndoPackets.Count-1;i>=0;i--)
            {
               removeUndo(i);
               if (currMem < mMaxMemoryInBytes || mUndoPackets.Count==0)
                  break;
               i++;
            }
            //if the memory we're adding is bigger than our system in general...
            //just add it..   
         }
         mUndoPackets.Add(pkt);
         mCurrUndoStep = mUndoPackets.Count - 1;
      }

      //this will execute the top UNDO, and remove it from the list
      public static void popUndo()
      {
         executeUndo(mUndoPackets.Count-1);
         removeUndo(mUndoPackets.Count - 1);
      }

      //this will execute the current UNDO pointed to by mCurrUndoStep, and increment it
      public static void redo()
      {
         executeRedo(mCurrUndoStep);

         if (mCurrUndoStep + 1 < mUndoPackets.Count)
            mCurrUndoStep++;
      }

      //this will execute the current UNDO pointed to by mCurrUndoStep, and decrement it
      public static void undo()
      {
         executeUndo(mCurrUndoStep);
         if(mCurrUndoStep-1 >= 0)
            mCurrUndoStep--;
      }

      //internals
      static void executeUndo(int index)
      {
         if (index < 0 || index >= mUndoPackets.Count)
            return;

         if (mUndoPackets[index].mCallbackFunction != null)
            mUndoPackets[index].mCallbackFunction(mUndoPackets[index].mPrevData);
      }

      static void executeRedo(int index)
      {
         if (index < 0 || index >= mUndoPackets.Count)
            return;

         if (mUndoPackets[index].mCallbackFunction != null)
            mUndoPackets[index].mCallbackFunction(mUndoPackets[index].mCurrData);
      }

      static void removeUndo(int index)
      {
         if (index < 0 || index >= mUndoPackets.Count)
            return;

         mCurrMemInBytes -= mUndoPackets[index].mPrevData.mMemorySize + mUndoPackets[index].mCurrData.mMemorySize;
         mUndoPackets.RemoveAt(index);
      }


      //members
      static List<UndoPacket> mUndoPackets = new List<UndoPacket>();
      static int mMaxMemoryInBytes = 2000000;   //50mb....
      
      static int mCurrMemInBytes = 0;
      static int mCurrUndoStep = 0;

   }
}