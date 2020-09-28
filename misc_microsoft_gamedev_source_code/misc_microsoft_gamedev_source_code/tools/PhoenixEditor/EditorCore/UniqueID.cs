using System;
using System.Collections.Generic;
using System.Text;

namespace EditorCore
{
   public class UniqueID
   {
      private int mHighestID = -1;
      public int NextID()
      {
         return mHighestID + 1;
      }


      public int GetUniqueID()
      {
         mHighestID++;

         AddID(mHighestID);

         return mHighestID;
      }
      public void RegisterID(int ID)
      {
         if (ID > mHighestID)
            mHighestID = ID;

         AddID(ID);
      }

      public void AddID(int ID)
      {
         if (mUniqueIDs.Contains(ID))
         {
            //?
            //throw new System.Exception("duplicate ID");
         }
         mUniqueIDs.Add(ID);

      }
      public void ResetIDSystem()
      {
         mHighestID = -1;
         mUniqueIDs.Clear();
      }

      List<int> mUniqueIDs = new List<int>();
   }
}
