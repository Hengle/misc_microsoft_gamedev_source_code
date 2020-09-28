using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;
using System.ComponentModel;
using System.Diagnostics;
using System.Xml.Serialization;
using System.Threading;


namespace ScnMemEst
{
   






   class ScnMemoryEstimate
   {

      public enum eMainCatagory
      {
         eCat_Terrain = 0,
         eCat_Models,
         eCat_Models_Detailed
      }
      public class memoryElement 
      {
         public memoryElement(string name, int memSize, eMainCatagory cat, bool addToTotals)
         {
            mName = name;
            mMemoryInBytes = memSize;
            mCatagory = cat;
            mAddToTotals = addToTotals;
         }
         public memoryElement Clone()
         {
            return new memoryElement(mName,mMemoryInBytes,mCatagory,mAddToTotals);
         }

         public int mMemoryInBytes;
         public string mName;
         public eMainCatagory mCatagory;
         public bool mAddToTotals;
      };

      int mMaxTerrainMemory = 52428800; //50mb
      int mMaxModelMemory = 138412032; // 100mb for textures, 32mb for UGX/UAX

      List<memoryElement> mMemoryUsageItems = new List<memoryElement>();
      int findMemoryElement(string name)
      {
         for (int i = 0; i < mMemoryUsageItems.Count; i++)
            if (mMemoryUsageItems[i].mName.Equals(name))
               return i;
         return -1;
      }

      public void setOrAddMemoryElement(string name, int memSize, eMainCatagory cat)
      {
         setOrAddMemoryElement(name, memSize, cat,true);
      }
      public void setOrAddMemoryElement(string name, int memSize, eMainCatagory cat,bool addToTotals)
      {
         int ex = findMemoryElement(name);
         if (ex == -1)
         {
            ex = mMemoryUsageItems.Count;
            mMemoryUsageItems.Add(new memoryElement(name, memSize, cat, addToTotals));
         }
         else
            mMemoryUsageItems[ex].mMemoryInBytes += memSize;
      }
      public int giveTotalMaxMemory()
      {
         return mMaxTerrainMemory + mMaxModelMemory;
      }
      public int giveTotalMaxMemoryCatagory(eMainCatagory cat)
      {
         if (cat == eMainCatagory.eCat_Terrain)
            return mMaxTerrainMemory;
         else if (cat == eMainCatagory.eCat_Models)
            return mMaxModelMemory;

         return 0;
      }
      public int giveTotalAvailableMemory()
      {
         int sum = giveTotalMaxMemory() - giveTotalMemoryUsage();
     //    if (sum < 0) sum = 0;
         return sum;
      }
      public int giveTotalAvailableMemoryCatagory(eMainCatagory cat)
      {
         int sum = giveTotalMaxMemoryCatagory(cat) - giveTotalMemoryUsageCatagory(cat);
         if (sum < 0) sum = 0;
         return sum;
      }

      public int giveTotalMemoryUsage()
      {
         int sum = 0;
         for (int i = 0; i < mMemoryUsageItems.Count; i++)
         {
            if(mMemoryUsageItems[i].mAddToTotals)
               sum += mMemoryUsageItems[i].mMemoryInBytes;
         }
         return sum;
      }
      public int giveTotalMemoryUsageCatagory(eMainCatagory cat)
      {
         int sum = 0;
         for (int i = 0; i < mMemoryUsageItems.Count; i++)
         {
            if (mMemoryUsageItems[i].mCatagory == cat)
               if (mMemoryUsageItems[i].mAddToTotals)
                  sum += mMemoryUsageItems[i].mMemoryInBytes;
         }
         return sum;
      }

      public float giveMemoryUsagePercent()
      {
         int sum = 0;
         for (int i = 0; i < mMemoryUsageItems.Count; i++)
         {
            if (mMemoryUsageItems[i].mAddToTotals)
               sum += mMemoryUsageItems[i].mMemoryInBytes;
         }

         float perc = 0;
         perc = sum / (float)giveTotalMaxMemory();
         return perc;
      }
      public float giveMemoryUsageCatagoryPercent(eMainCatagory cat)
      {
         int sum = 0;
         for (int i = 0; i < mMemoryUsageItems.Count; i++)
         {
            if (mMemoryUsageItems[i].mCatagory == cat)
               if (mMemoryUsageItems[i].mAddToTotals)
                  sum += mMemoryUsageItems[i].mMemoryInBytes;
         }

         float perc = 0;
         if (cat == eMainCatagory.eCat_Terrain)
            perc = sum / (float)mMaxTerrainMemory;
         else if (cat == eMainCatagory.eCat_Models)
            perc = sum / (float)mMaxModelMemory;

         return perc;
      }

      //---------------------------------------
      public int getNumMemoryElements()
      {
         return mMemoryUsageItems.Count;
      }
      public memoryElement getMemoryElement(int index)
      {
         if (index < 0 || index >= mMemoryUsageItems.Count)
            return null;

         return mMemoryUsageItems[index];
      }

      //---------------------------------------

      public void exportToFile(string filename)
      {
         if (File.Exists(filename))
         {
            try
            {
               File.Delete(filename);
            }
            catch (Exception e)
            {
               MessageBox.Show("Could not save over " + filename + ".\n Please close any other programs using this file, and make sure it is not write protected, then try again.");
               return;
            }
         }

         string tabs = "     ";


         List<memoryElement> mTemMemoryItems = new List<memoryElement>();

         using (StreamWriter sw = new StreamWriter(filename, true))
         {
            sw.WriteLine("File : " + Path.GetFileNameWithoutExtension(filename));
            sw.WriteLine("Generated : " + DateTime.Now);

            sw.WriteLine("Executive Memory Summary,");
            sw.WriteLine(tabs + "Budget," + bytesToMB(giveTotalMaxMemory()).ToString("N3") + "MB,");
            sw.WriteLine(tabs + "Used," + bytesToMB(giveTotalMemoryUsage()).ToString("N3") + "MB,");
            sw.WriteLine(tabs + "Free," + bytesToMB(giveTotalAvailableMemory()).ToString("N3") + "MB,");


            sw.WriteLine();
            sw.WriteLine();
            sw.WriteLine("Terrain Summary," + bytesToMB(giveTotalMemoryUsageCatagory(eMainCatagory.eCat_Terrain)).ToString("N3") + "MB,");

            int numMemoryElements = getNumMemoryElements();
            for (int i = 0; i < numMemoryElements; i++)
            {
               memoryElement me = getMemoryElement(i);
               if (me.mCatagory == eMainCatagory.eCat_Terrain)
                  mTemMemoryItems.Add(me.Clone());
            }

            mTemMemoryItems.Sort(delegate(memoryElement p1, memoryElement p2) { return p2.mMemoryInBytes.CompareTo(p1.mMemoryInBytes); });

            for (int i = 0; i < mTemMemoryItems.Count; i++)
            {
               sw.WriteLine(tabs + mTemMemoryItems[i].mName + "," + bytesToMB(mTemMemoryItems[i].mMemoryInBytes).ToString("N3") + "MB");
            }
            mTemMemoryItems.Clear();




            sw.WriteLine();
            sw.WriteLine();
            sw.WriteLine("Models Summary," + bytesToMB(giveTotalMemoryUsageCatagory(eMainCatagory.eCat_Models)).ToString("N3") + "MB,");


            for (int i = 0; i < numMemoryElements; i++)
            {
               memoryElement me = getMemoryElement(i);
               if (me.mCatagory == eMainCatagory.eCat_Models)
               {
                  sw.WriteLine(tabs + me.mName + "," + bytesToMB(me.mMemoryInBytes).ToString("N3") + "MB");
               }
            }

            sw.WriteLine();
            sw.WriteLine();
            sw.WriteLine("Models Detailed");

            sw.WriteLine(tabs + "Textures : DDX");
            for (int i = 0; i < numMemoryElements; i++)
            {
               memoryElement me = getMemoryElement(i);
               if (me.mCatagory == eMainCatagory.eCat_Models_Detailed)
               {
                  if (Path.GetExtension(me.mName) == ".ddx")
                  {
                     mTemMemoryItems.Add(me.Clone());
                  }
               }
            }

            mTemMemoryItems.Sort(delegate(memoryElement p1, memoryElement p2) { return p2.mMemoryInBytes.CompareTo(p1.mMemoryInBytes); });
            for (int i = 0; i < mTemMemoryItems.Count; i++)
            {
               sw.WriteLine((tabs + tabs + mTemMemoryItems[i].mName + "," + bytesToMB(mTemMemoryItems[i].mMemoryInBytes).ToString("N3") + "MB"));
            }
            mTemMemoryItems.Clear();








            sw.WriteLine(tabs + "Models : UGX");
            for (int i = 0; i < numMemoryElements; i++)
            {
               memoryElement me = getMemoryElement(i);
               if (me.mCatagory == eMainCatagory.eCat_Models_Detailed)
               {
                  if (Path.GetExtension(me.mName) == ".ugx")
                     mTemMemoryItems.Add(me.Clone());
               }
            }
            mTemMemoryItems.Sort(delegate(memoryElement p1, memoryElement p2) { return p2.mMemoryInBytes.CompareTo(p1.mMemoryInBytes); });
            for (int i = 0; i < mTemMemoryItems.Count; i++)
            {
               sw.WriteLine((tabs + tabs + mTemMemoryItems[i].mName + "," + bytesToMB(mTemMemoryItems[i].mMemoryInBytes).ToString("N3") + "MB"));
            }
            mTemMemoryItems.Clear();






            sw.WriteLine(tabs + "Anims : UAX");
            for (int i = 0; i < numMemoryElements; i++)
            {
               memoryElement me = getMemoryElement(i);
               if (me.mCatagory == eMainCatagory.eCat_Models_Detailed)
               {
                  if (Path.GetExtension(me.mName) == ".uax")
                     mTemMemoryItems.Add(me.Clone());
               }
            }
            mTemMemoryItems.Sort(delegate(memoryElement p1, memoryElement p2) { return p2.mMemoryInBytes.CompareTo(p1.mMemoryInBytes); });
            for (int i = 0; i < mTemMemoryItems.Count; i++)
            {
               sw.WriteLine((tabs + tabs + mTemMemoryItems[i].mName + "," + bytesToMB(mTemMemoryItems[i].mMemoryInBytes).ToString("N3") + "MB"));
            }
            mTemMemoryItems.Clear();

         }

      }


      float bytesToMB(int bytes)
      {
         return bytes / 1048576.0f;
      }
 
 
   };
};