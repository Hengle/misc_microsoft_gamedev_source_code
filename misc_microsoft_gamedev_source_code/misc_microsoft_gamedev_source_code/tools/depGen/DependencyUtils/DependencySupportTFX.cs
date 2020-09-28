using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;

using ConsoleUtils;



namespace DependencyUtils
{
   class DependencySupportTFX: IDependencyInterface
   {
      private string mExtensions = ".tfx";

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
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a TerrainEffect file.  The extensions must be \"{1}\".\n", filename, mExtensions);
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



         XmlDocument tfxDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(tfxDoc, fileNameAbsolute);


         // Read dependent files
         //

         // Particle effects
         //
         XmlNodeList dependentParticleNodes = tfxDoc.SelectNodes("//particlefile");
         foreach (XmlNode dependentParticleNode in dependentParticleNodes)
         {
            if (dependentParticleNode.FirstChild == null)
               continue;

            string dependentParticleName = dependentParticleNode.FirstChild.Value;
            dependentParticleName = String.Concat("art\\", dependentParticleName, ".pfx");

            if (!dependenciesFile.Contains(dependentParticleName))
            {
               // Check file existance
               bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentParticleName);

               if (!fileExists)
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "TerrainEffect File \"{0}\" is referring to pfx file \"{1}\" which does not exist.\n", filename, dependentParticleName);
               }


               dependenciesFile.Add(dependentParticleName);
               dependencies.Add(new FileInfo(dependentParticleName, fileExists));
            }
         }


         // Vis files
         //
         XmlNodeList dependentVisNodes = tfxDoc.SelectNodes("//vis");
         foreach (XmlNode dependentVisNode in dependentVisNodes)
         {
            if (dependentVisNode.FirstChild == null)
               continue;

            string dependentVisName = dependentVisNode.FirstChild.Value;
            dependentVisName = String.Concat("art\\", dependentVisName, ".vis");

            if (!dependenciesFile.Contains(dependentVisName))
            {
               // Check file existance
               bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentVisName);

               if (!fileExists)
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "TerrainEffect File \"{0}\" is referring to vis file \"{1}\" which does not exist.\n", filename, dependentVisName);
               }


               dependenciesFile.Add(dependentVisName);
               dependencies.Add(new FileInfo(dependentVisName, fileExists));
            }
         }

         // Lights
         //
         XmlNodeList dependentLightNodes = tfxDoc.SelectNodes("//light");
         foreach (XmlNode dependentLightNode in dependentLightNodes)
         {
            if (dependentLightNode.FirstChild == null)
               continue;

            string dependentLightName = dependentLightNode.FirstChild.Value;
            dependentLightName = String.Concat("art\\", dependentLightName, ".lgt");

            if (!dependenciesFile.Contains(dependentLightName))
            {
               // Check file existance
               bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentLightName);

               if (!fileExists)
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "TerrainEffect File \"{0}\" is referring to light file \"{1}\" which does not exist.\n", filename, dependentLightName);
               }


               dependenciesFile.Add(dependentLightName);
               dependencies.Add(new FileInfo(dependentLightName, fileExists));
            }
         }


         // Impact decal effects
         //
         XmlNodeList dependentImpactDecalNodes = tfxDoc.SelectNodes("//impactdecal");
         foreach (XmlNode dependentImpactDecalNode in dependentImpactDecalNodes)
         {
            if (dependentImpactDecalNode.FirstChild == null)
               continue;

            string dependentImpactDecalRootName = dependentImpactDecalNode.FirstChild.Value;
            dependentImpactDecalRootName = String.Concat("art\\", dependentImpactDecalRootName);


            string[] dependentImpactDecalName = new string[3];

            dependentImpactDecalName[0] = String.Concat(dependentImpactDecalRootName, "_df.ddx");
            dependentImpactDecalName[1] = String.Concat(dependentImpactDecalRootName, "_nm.ddx");
            dependentImpactDecalName[2] = String.Concat(dependentImpactDecalRootName, "_op.ddx");

            long textureChannelFoundCount = 0;

            for (long channel = 0; channel < 3; channel++)
            {
               if (!dependenciesFile.Contains(dependentImpactDecalName[channel]))
               {
                  // Check file existance
                  bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentImpactDecalName[channel]);

                  if (fileExists)
                  {
                     dependenciesFile.Add(dependentImpactDecalName[channel]);
                     dependencies.Add(new FileInfo(dependentImpactDecalName[channel], fileExists));
                     textureChannelFoundCount += 1;
                  }
               }
               else
               {
                  textureChannelFoundCount += 1;
               }
            }

            if (textureChannelFoundCount == 0)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "TerrainEffect File \"{0}\" is referring to impactdecal \"{1}\" which does not exist.\n", filename, dependentImpactDecalRootName);
            }
         }


         // Trail effects
         //
         XmlNodeList dependentTrailDecalNodes = tfxDoc.SelectNodes("//trail");
         foreach (XmlNode dependentTrailDecalNode in dependentTrailDecalNodes)
         {
            if (dependentTrailDecalNode.FirstChild == null)
               continue;

            string dependentTrailDecalRootName = dependentTrailDecalNode.FirstChild.Value;
            dependentTrailDecalRootName = String.Concat("art\\", dependentTrailDecalRootName);


            string[] dependentImpactDecalName = new string[3];

            dependentImpactDecalName[0] = String.Concat(dependentTrailDecalRootName, "_df.ddx");
            dependentImpactDecalName[1] = String.Concat(dependentTrailDecalRootName, "_nm.ddx");
            dependentImpactDecalName[2] = String.Concat(dependentTrailDecalRootName, "_op.ddx");

            long textureChannelFoundCount = 0;

            for (long channel = 0; channel < 3; channel++)
            {
               if (!dependenciesFile.Contains(dependentImpactDecalName[channel]))
               {
                  // Check file existance
                  bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentImpactDecalName[channel]);

                  if (fileExists)
                  {
                     dependenciesFile.Add(dependentImpactDecalName[channel]);
                     dependencies.Add(new FileInfo(dependentImpactDecalName[channel], fileExists));
                     textureChannelFoundCount += 1;
                  }
               }
               else
               {
                  textureChannelFoundCount += 1;
               }
            }

            if (textureChannelFoundCount == 0)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "TerrainEffect File \"{0}\" is referring to trail decal \"{1}\" which does not exist.\n", filename, dependentTrailDecalRootName);
            }
         }

         return true;
      }
   }
}
