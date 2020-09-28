using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using System.IO;
using System.Windows.Forms;

namespace RemoteMemory
{
   public class GlobalSettings
   {
      //============================================================================
      // LastEXEFile
      //============================================================================
      static string mLastEXEFile = "";
      public static string LastEXEFile
      {
         get
         {
            return mLastEXEFile;
         }
         set
         {
            mLastEXEFile = value;
         }
      }

      //============================================================================
      // LastBINFile
      //============================================================================
      static string mLastBINFile = "";
      public static string LastBINFile
      {
         get
         {
            return mLastBINFile;
         }
         set
         {
            mLastBINFile = value;
         }
      }

      //============================================================================
      // LocalCodePath
      //============================================================================
      static string mLocalCodePath = "c:\\depot\\phoenix\\xbox\\code\\";
      public static string LocalCodePath
      {
         get
         {
            return mLocalCodePath;
         }
         set
         {
            mLocalCodePath = value;
         }
      }

      //============================================================================
      // MaxStackWalkDepth
      //============================================================================
      static uint mMaxStackWalkDepth = 1;
      public static uint MaxStackWalkDepth
      {
         get
         {
            return mMaxStackWalkDepth;
         }
         set
         {
            mMaxStackWalkDepth = value;
         }
      }

      //============================================================================
      // LocalCodePath
      //============================================================================
      public enum ePlaybackSpeeds
      {
         eSlow = 0,
         eMedium,
         eFast,
         eASAP
      };
      static ePlaybackSpeeds mPlaybackSpeed = ePlaybackSpeeds.eSlow;
      public static ePlaybackSpeeds PlaybackSpeed
      {
         get
         {
            return mPlaybackSpeed;
         }
         set
         {
            mPlaybackSpeed = value;
         }
      }



      static private string cSettingsFileLocation = "RemoteMemory.cfg";
      //============================================================================
      // LastBINFile
      //============================================================================
      static public void save()
      {
         try
         {
            string filePath = AppDomain.CurrentDomain.BaseDirectory + cSettingsFileLocation;
            if (File.Exists(filePath))
               File.Delete(filePath);

            XMLGlobalSettings gsXML = new XMLGlobalSettings();
            gsXML.LastBINFile = LastBINFile;
            gsXML.LastEXEFile = LastEXEFile;
            gsXML.LocalCodePath = LocalCodePath;
            gsXML.PlaybackSpeed = PlaybackSpeed;
            gsXML.MaxStackWalkDepth = MaxStackWalkDepth;

            XmlSerializer s = new XmlSerializer(typeof(XMLGlobalSettings), new Type[] { });
            Stream st = File.Open(filePath, FileMode.OpenOrCreate);
            s.Serialize(st, gsXML);
            st.Close();
         }
         catch (Exception e)
         {
            MessageBox.Show(e.InnerException.ToString());
         }
      }

      //============================================================================
      // LastBINFile
      //============================================================================
      static public void load()
      {
         string filePath = AppDomain.CurrentDomain.BaseDirectory + cSettingsFileLocation;

         if (!File.Exists(filePath))
            save();

         Stream st = null;
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(XMLGlobalSettings), new Type[] { });

            st = File.OpenRead(filePath);
            XMLGlobalSettings gsXML = (XMLGlobalSettings)s.Deserialize(st);
            st.Close();



            PlaybackSpeed = gsXML.PlaybackSpeed;
            LastBINFile = gsXML.LastBINFile;
            LastEXEFile = gsXML.LastEXEFile;
            LocalCodePath = gsXML.LocalCodePath;
            MaxStackWalkDepth = gsXML.MaxStackWalkDepth;
         }
         catch (Exception e)
         {
            MessageBox.Show("Error Loading " + cSettingsFileLocation + ":\n" + e.InnerException.ToString());

            if (st != null)
               st.Close();

            if (File.Exists(filePath))
               File.Delete(filePath);

         }
         
      }

      //============================================================================
      // XMLGlobalSettings
      //============================================================================
      public class XMLGlobalSettings
      {
         string mLastEXEFile = "";
         public string LastEXEFile
         {
            get
            {
               return mLastEXEFile;
            }
            set
            {
               mLastEXEFile = value;
            }
         }

         string mLastBINFile = "";
         public string LastBINFile
         {
            get
            {
               return mLastBINFile;
            }
            set
            {
               mLastBINFile = value;
            }
         }

         string mLocalCodePath = "c:\\depot\\phoenix\\xbox\\code\\";
         public string LocalCodePath
         {
            get
            {
               return mLocalCodePath;
            }
            set
            {
               mLocalCodePath = value;
            }
         }

         ePlaybackSpeeds mPlaybackSpeed = ePlaybackSpeeds.eSlow;
         public ePlaybackSpeeds PlaybackSpeed
         {
            get
            {
               return mPlaybackSpeed;
            }
            set
            {
               mPlaybackSpeed = value;
            }
         }

         uint mMaxStackWalkDepth = 1;
         public uint MaxStackWalkDepth
         {
            get
            {
               return mMaxStackWalkDepth;
            }
            set
            {
               mMaxStackWalkDepth = value;
            }
         }
      }

   }
}
