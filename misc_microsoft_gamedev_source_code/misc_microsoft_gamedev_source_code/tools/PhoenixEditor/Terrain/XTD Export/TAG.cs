
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Xml;
using System.Xml.Serialization;
using System.Windows.Forms;

using EditorCore;

namespace Export360
{
   public class TAG
   {
      List<String> mFilesToBeTagged = new List<string>();

      ~TAG()
      {
         if (mFilesToBeTagged != null)
            mFilesToBeTagged.Clear();
         mFilesToBeTagged = null;
      }
      public void clearList()
      {
         mFilesToBeTagged.Clear();
      }
      public void addFile(string filename)
      {
         if(!mFilesToBeTagged.Contains(filename))
            mFilesToBeTagged.Add(filename);
      }
      public unsafe bool writeToFile(string filename)
      {
         try
         {
            if (File.Exists(CoreGlobals.getWorkPaths().mTAGToolPath) == false)
            {
               MessageBox.Show("Can't find: " + CoreGlobals.getWorkPaths().mXMBToolPath, "Error generating " + filename);
               return false;
            }

            string arguments = "";
            arguments = arguments + " -create";
            arguments = arguments + " -outFile \"" + filename + "\"";
            foreach (string output in mFilesToBeTagged)
            {
               arguments = arguments + " -file \"" + output + "\"";
            }

            System.Diagnostics.Process process;
            process = new System.Diagnostics.Process();
            process = System.Diagnostics.Process.Start(CoreGlobals.getWorkPaths().mTAGToolPath, arguments);
            process.WaitForExit();
            process.Close();

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
            return false;
         }
         return true;
      }
   }
}