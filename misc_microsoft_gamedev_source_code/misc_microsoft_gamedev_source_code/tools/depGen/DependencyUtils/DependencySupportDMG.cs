using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using ConsoleUtils;



namespace DependencyUtils
{
   class DependencySupportDMG : IDependencyInterface
   {
      private string mExtensions = ".dmg";

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
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a DMG file.  The extension must be \"{1}\".\n", filename, mExtensions);
            return false;
         }

         string fileNameAbsolute = CoreGlobals.getWorkPaths().mGameDirectory + "\\" + filename;

         // check if file exists
         if (!File.Exists(fileNameAbsolute))
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" not found.\n", filename);
            return false;
         }

         ConsoleOut.Write(ConsoleOut.MsgType.Info, "Processing File: \"{0}\".\n", filename);





         XmlDocument dmgDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(dmgDoc, fileNameAbsolute);


         // Read dependent files
         //
         XmlNodeList actionNodes = dmgDoc.SelectNodes("//action");
         foreach (XmlNode actionNode in actionNodes)
         {
            // read effect names
            for (int i = 0; i < 3; i++)
            {
               XmlNode effectNameNode = null;

               switch (i)
               {
                  case 0:
                     effectNameNode = actionNode.Attributes.GetNamedItem("effect");
                     break;
                  case 1:
                     effectNameNode = actionNode.Attributes.GetNamedItem("streamereffect");
                     break;
                  case 2:
                     effectNameNode = actionNode.Attributes.GetNamedItem("releaseeffect");
                     break;
               }

               if ((effectNameNode == null) || (effectNameNode.FirstChild == null))
                  continue;

               string dependentFileName = effectNameNode.FirstChild.Value;
               dependentFileName = String.Concat("art\\", dependentFileName, ".pfx");

               if (!dependenciesFile.Contains(dependentFileName))
               {
                  // Check file existance
                  bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentFileName);

                  if (!fileExists)
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Damage File \"{0}\" is referring to pfx file \"{1}\" which does not exist.\n", filename, dependentFileName);
                  }

                  dependenciesFile.Add(dependentFileName);
                  dependencies.Add(new FileInfo(dependentFileName, fileExists));
               }
            }


            // Read terrain effect
            XmlNode terrainEffectNameNode = null;
            terrainEffectNameNode = actionNode.Attributes.GetNamedItem("terraineffect");

            if ((terrainEffectNameNode != null) && (terrainEffectNameNode.FirstChild != null))
            {
               string dependentFileName = terrainEffectNameNode.FirstChild.Value;
               dependentFileName = String.Concat("art\\", dependentFileName, ".tfx");

               if (!dependenciesFile.Contains(dependentFileName))
               {
                  // Check file existance
                  bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentFileName);

                  if (!fileExists)
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Damage File \"{0}\" is referring to tfx file \"{1}\" which does not exist.\n", filename, dependentFileName);
                  }

                  dependenciesFile.Add(dependentFileName);
                  dependencies.Add(new FileInfo(dependentFileName, fileExists));
               }
            }
         }

         return true;
      }
   }
}
