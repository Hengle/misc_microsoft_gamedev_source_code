using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;

using ConsoleUtils;



namespace DependencyUtils
{
   class DependencySupportPHYSICS: IDependencyInterface
   {
      private string mExtensions = ".physics";

      string IDependencyInterface.getExtensions()
      {
         return (mExtensions);
      }

      bool IDependencyInterface.getDependencyList(string filename, List<FileInfo> dependencies, List<string> dependentUnits)
      {
         List<string> dependenciesFile = new List<string>();

         // check extension
         String ext = Path.GetExtension(filename).ToLower();
         if (!mExtensions.Contains(ext))
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a Physics file.  The extensions must be \"{1}\".\n", filename, mExtensions);
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



         XmlDocument physicsDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(physicsDoc, fileNameAbsolute);


         // Read dependent files
         //

         // Terrain effects
         //
         XmlNodeList dependentTerrainEffectNodes = physicsDoc.SelectNodes("//TerrainEffects");
         foreach (XmlNode dependentTerrainEffectNode in dependentTerrainEffectNodes)
         {
            if (dependentTerrainEffectNode.FirstChild == null)
               continue;

            string dependentTerrainEffectName = dependentTerrainEffectNode.FirstChild.Value;
            dependentTerrainEffectName = String.Concat("art\\", dependentTerrainEffectName, ".tfx");

            if (!dependenciesFile.Contains(dependentTerrainEffectName))
            {
               // Check file existance
               bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentTerrainEffectName);

               if (!fileExists)
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Physics File \"{0}\" is referring to tfx file \"{1}\" which does not exist.\n", filename, dependentTerrainEffectName);
               }

               dependenciesFile.Add(dependentTerrainEffectName);
               dependencies.Add(new FileInfo(dependentTerrainEffectName, fileExists));
            }
         }


         return true;
      }
   }
}
