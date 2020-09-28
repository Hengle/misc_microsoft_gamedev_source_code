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
   class XboxModelEstimate
   {
      string gameDirectory = null;

     public void estimateMemory(string scenarioName,string gameWorkDirectory, ScnMemoryEstimate memEst)
      {
         gameDirectory = gameWorkDirectory;


         scenarioName = Path.ChangeExtension(scenarioName, null);
         generateFileList(scenarioName);


         //file will be in form of "<filename>_fileList.txt" : _memestsave_FileList.txt
         processFileList(Path.GetFileNameWithoutExtension(scenarioName) + "_FileList.txt", memEst);
         quickDeleteFile(scenarioName);

        
      }
      void generateFileList(string fName)
      {
         if (!Directory.Exists("cache"))
            Directory.CreateDirectory("cache");

         string scenerioPath = gameDirectory + "\\scenario";
         string scenarioName = scenerioPath + @"\" + fName + ".scn";

         string arguments = " -quiet";
         arguments = arguments + @" -scriptfile " + @"MemEst_createScenarioFileList.xml";
         arguments = arguments + @" /define scenariofilename=" + scenarioName;

         string argGenPath = gameDirectory + @"\tools\arcGen\arcGen.exe";
         System.Diagnostics.Process arcgenUtility = new System.Diagnostics.Process();
         arcgenUtility.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
         //arcgenUtility.StartInfo.CreateNoWindow = true;
         arcgenUtility.StartInfo.Arguments = arguments;
         arcgenUtility.StartInfo.FileName = argGenPath;
         arcgenUtility.Start();

         arcgenUtility.WaitForExit();
         if (arcgenUtility.ExitCode != 0)
         {
            MessageBox.Show("Error calculating memory estimates for : " + scenarioName + "\n Please send a screenshot of this message to the editor programmers.");
         }

         arcgenUtility.Close();
      }
      string removeWorkPrepath(string filename)
      {
         int idx = filename.IndexOf("work");
         if(idx == -1)
            return filename;

         string str = filename.Substring(idx);
         return str;
      }
      void processFileList(string listfilename,ScnMemoryEstimate memEst)
      {

         

         if (!File.Exists(listfilename))
            return;

         Stream st = null;
         // if we're in async mode, we may not have access to this yet.
         while (st == null)
         {
            try
            {
               st = File.OpenRead(listfilename);
            }
            catch (IOException e)
            {

            }
         }

         StreamReader tr = new StreamReader(st);
         int otherFiles = 0;

         List<string> ddxFiles = new List<string>();

         try
         {
            string filename = tr.ReadLine();
            do
            {
               if (!File.Exists(filename))
                  continue;

               if (filename.ToLower().Contains(".xmb"))
                  continue;
               if (filename.ToLower().Contains(".lgt"))
                  continue;
               if (filename.ToLower().Contains(".tfx"))
                  continue;
               if (filename.ToLower().Contains(".xpr"))
                  continue;
               if (filename.ToLower().Contains(".scn"))
                  continue;
               if (filename.ToLower().Contains(".txt"))
                  continue;
               if (filename.ToLower().Contains(".gls"))
                  continue;
               if (filename.ToLower().Contains(".pfx"))
                  continue;

               if (filename.ToLower().Contains(".ddx"))
               {
                  int DDXMem = DDXBridge.give360TextureMemFootprint(filename);

                  memEst.setOrAddMemoryElement("Model Texture Memory", DDXMem, ScnMemoryEstimate.eMainCatagory.eCat_Models);
                  memEst.setOrAddMemoryElement(removeWorkPrepath(filename), DDXMem, ScnMemoryEstimate.eMainCatagory.eCat_Models_Detailed, false);
               }
               else if (filename.ToLower().Contains(".ugx"))
               {
                  System.IO.FileInfo fi = new System.IO.FileInfo(filename);
                  int UGXMem = (int)fi.Length;
                  fi = null;

                  memEst.setOrAddMemoryElement("Model UGX Memory", UGXMem, ScnMemoryEstimate.eMainCatagory.eCat_Models);
                  memEst.setOrAddMemoryElement(removeWorkPrepath(filename), UGXMem, ScnMemoryEstimate.eMainCatagory.eCat_Models_Detailed, false);
                  
               }
               else if (filename.ToLower().Contains(".uax"))
               {
                  System.IO.FileInfo fi = new System.IO.FileInfo(filename);
                  int UAXMem = (int)fi.Length;
                  fi = null;

                  memEst.setOrAddMemoryElement("Model UAX Memory", UAXMem, ScnMemoryEstimate.eMainCatagory.eCat_Models);
                  memEst.setOrAddMemoryElement(removeWorkPrepath(filename), UAXMem, ScnMemoryEstimate.eMainCatagory.eCat_Models_Detailed, false);
               }
               else
               {
                  otherFiles++;
               }

            } while ((filename = tr.ReadLine()) != null);

         }
         catch (IOException e)
         {

         }
         finally
         {
            tr.Close();
            st.Close();
         }

      }
      void quickDeleteFile(string fName)
      {
        if (Directory.Exists("cache"))
            Directory.Delete("cache", true);

         if (File.Exists(Path.GetFileNameWithoutExtension(fName) + "_FileList.txt"))
            File.Delete(Path.GetFileNameWithoutExtension(fName) + "_FileList.txt");

      }

   };
}