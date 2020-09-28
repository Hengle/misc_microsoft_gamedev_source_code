using System;
using System.Collections.Generic;
using System.Text;

using System.Collections;
using System.IO;

namespace PhoenixBuildServer
{
   public class ConfigData
   {
      private class Data : IComparable
      {
         public string mData;
         public string mDisplayName;

         public int CompareTo(object obj)
         {
            if (obj is Data)
            {
               Data temp = (Data)obj;
               return mDisplayName.CompareTo(temp.mDisplayName);
            }
            else if (obj is String)
            {
               String temp = (String)obj;
               return mDisplayName.CompareTo(temp);
            }

            throw new ArgumentException("object is not a Data or String");
         }

      }

      #region Private Members
      /// <summary>
      /// private member variables.
      /// </summary>
      private ArrayList mDataProperties;
      #endregion

      public ConfigData()
      {
         //-- new and clear everything out.
         mDataProperties = new ArrayList();
      }

      public int getIndex(string paramName)
      {
         // Check the limits.
         if (paramName == null || paramName.Length <= 0)
            return (-1);

         int index = mDataProperties.BinarySearch(paramName);

         return (index);
      }

      public string getString(string paramName)
      {
         int index = getIndex(paramName);
         if (index < 0)
            return (null);

         return (((Data)mDataProperties[index]).mData);
      }

      public string getString(int index)
      {
         if (index < 0 || index >= mDataProperties.Count)
            return (null);

         return (((Data)mDataProperties[index]).mData);
      }

      public int getInt(string paramName)
      {
         int index = getIndex(paramName);
         if (index < 0)
            return (-1);

         return (System.Convert.ToInt32(((Data)mDataProperties[index]).mData));
      }

      public int getInt(int index)
      {
         if (index < 0 || index >= mDataProperties.Count)
            return (-1);

         return (System.Convert.ToInt32(((Data)mDataProperties[index]).mData));
      }

      public void addData(string paramName, string data)
      {
         int index = getIndex(paramName);
         if (index < 0)
         {
            Data d = new Data();
            d.mData = data;
            d.mDisplayName = paramName;
            mDataProperties.Add(d);
            Sort();
         }
         else
         {
            setData(index, data);
         }
      }

      public void setData(int index, string data)
      {
         if (index < 0)
            return;

         ((Data)mDataProperties[index]).mData = data;
      }

      public string getParamName(int index)
      {
         if (index < 0 || index >= mDataProperties.Count)
            return null;
         return ((Data)mDataProperties[index]).mDisplayName;
      }

      public int getTotalItems()
      {
         return (mDataProperties.Count);
      }

      protected bool initFromFile(string path)
      {
         if (File.Exists(path) == false)
         {
            return (false);
         }

         // Open the stream and read it back.
         using (StreamReader sr = File.OpenText(path))
         {
            string s = "";
            string param = "";
            string data = "";
            while ((s = sr.ReadLine()) != null)
            {
               if (s.StartsWith("//") == true)
                  continue;

               s = s.Trim();
               int equalIndex = s.LastIndexOf("=");
               if (equalIndex <= 0)
                  continue;
               param = s.Substring(0, equalIndex).Trim();
               data = s.Substring(equalIndex + 1).Trim();

               addData(param, data);
            }
         }
         Sort();
         return (true);
      }

      protected void Sort()
      {
         mDataProperties.Sort();
      }
   }
}
