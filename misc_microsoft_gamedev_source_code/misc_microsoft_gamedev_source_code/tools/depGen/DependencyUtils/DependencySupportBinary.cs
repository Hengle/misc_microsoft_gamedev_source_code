using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;

using ConsoleUtils;



namespace DependencyUtils
{
   class DependencySupportBinary: IDependencyInterface
   {
      private string mExtensions = ".xsd,.ugx,.swf";

      string IDependencyInterface.getExtensions()
      {
         return (mExtensions);
      }


      bool IDependencyInterface.getDependencyList(string filename, List<FileInfo> dependencies, List<string> dependentUnits)
      {
         // check extension
         String ext = Path.GetExtension(filename).ToLower();
         if (!mExtensions.Contains(ext))
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a supported binary file.  The extensions must be \"{1}\".\n", filename, mExtensions);
            return false;
         }


         string fileNameAbsolute = CoreGlobals.getWorkPaths().mGameDirectory + "\\" + filename;

         // check if file exists
         if (!File.Exists(fileNameAbsolute))
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" not found.\n", filename);
            return false;
         }

         ConsoleOut.Write(ConsoleOut.MsgType.Info, "Processing File: \"{0}\"\n", filename);



         // This file depends on a DEP file by the same name
         string dependentFileName = Path.ChangeExtension(filename, ".dep");


         // Check file existance
         bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentFileName);

         if (!fileExists)
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Binary file \"{0}\" depends on file \"{1}\" which does not exist.\n", filename, dependentFileName);
         }


         dependencies.Add(new FileInfo(dependentFileName, fileExists));

         return true;
      }
   }
}
