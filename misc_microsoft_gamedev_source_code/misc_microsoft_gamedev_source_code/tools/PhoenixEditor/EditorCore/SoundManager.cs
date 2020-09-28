using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Xml;

namespace EditorCore
{
   public class SoundManager
   {
      public SoundManager()
      {
         try
         {
          buildBankList();
          initSound(CoreGlobals.getWorkPaths().mGameSoundDirectory + "\\wwise_material\\generatedsoundbanks\\windows\\");
          mEngineInit = true;
         }
         catch
         {
          mEngineInit = false;
         }
      }

      public bool buildBankList()
      {    
         // Read sound bank
         string banksFilename = CoreGlobals.getWorkPaths().mEditorSettings + "\\soundbanks.xml";
        
         XmlTextReader reader = new XmlTextReader (banksFilename);

         while (reader.Read())
         {
            if (reader.NodeType == XmlNodeType.Element && reader.Name == "StaticBank" )
            {
                while (reader.Read())
                {
                    if (reader.NodeType == XmlNodeType.Text)
                    {
                        dllAddBankToLoad(reader.Value.ToString());
                        break;
                    }
                }
            }
         }
         return true;
      }

      public bool initSound(string str)
      {
      if(mEngineInit == false)
      {
         return SoundManager.dllInitSound(str);
      }
      else
         return false;
      }

      public void updateSound()
      {
         if(mEngineInit)
            SoundManager.dllUpdateSound();
      }

      public void playSound(string str)
      {
         if (mEngineInit)
            SoundManager.dllPlaySound(str);
      }

      public void stopAllSounds()
      {
         if (mEngineInit)
            SoundManager.dllPlaySound("stop_all");
      }

      //-- Exposed DLL methods
      [DllImport("WWiseLib.dll")]
      private extern static bool dllInitSound([MarshalAs(UnmanagedType.LPStr)] string m);

      [DllImport("WWiseLib.dll")]
      private extern static void dllUpdateSound();

      [DllImport("WWiseLib.dll")]
      private extern static bool dllPlaySound([MarshalAs(UnmanagedType.LPStr)] string m);

      [DllImport("WWiseLib.dll")]
      private extern static bool dllAddBankToLoad([MarshalAs(UnmanagedType.LPStr)] string m);

      private bool mEngineInit = false;
   }
}
