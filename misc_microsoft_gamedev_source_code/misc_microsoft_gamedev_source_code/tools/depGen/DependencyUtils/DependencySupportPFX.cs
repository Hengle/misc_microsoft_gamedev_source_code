using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using ConsoleUtils;



namespace DependencyUtils
{
   class DependencySupportPFX : IDependencyInterface
   {
      private string mExtensions = ".pfx";

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
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a PFX file.  The extension must be \"{1}\".\n", filename, mExtensions);
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




         XmlDocument particleEffectDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(particleEffectDoc, fileNameAbsolute);

         // Read emitters
         //
         XmlNodeList emitterNodes = particleEffectDoc.SelectNodes("/ParticleEffect/ParticleEmitter");
         foreach (XmlNode emitterNode in emitterNodes)
         {

            // Look at emitter data
            //
            XmlNode emitterDataNode = emitterNode.SelectSingleNode("./EmitterData");

            if (emitterDataNode != null)
            {
               XmlNode childPFXFileNode = emitterDataNode.SelectSingleNode("./PFXFilePath");

               if (childPFXFileNode != null)
               {
                  if (childPFXFileNode.FirstChild == null)
                     continue;

                  string pfxFileName = "art\\" + childPFXFileNode.FirstChild.Value;

                  // Replace .tga extension with .ddx
                  pfxFileName = Path.ChangeExtension(pfxFileName, ".pfx");

                  if (!dependenciesFile.Contains(pfxFileName))
                  {
                     // Check file existance
                     bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + pfxFileName);

                     if (!fileExists)
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Particle File \"{0}\" is referring to pfx file \"{1}\" which does not exist.\n", filename, pfxFileName);
                     }

                     dependenciesFile.Add(pfxFileName);
                     dependencies.Add(new FileInfo(pfxFileName, fileExists));
                  }
               }
            }

            // Look at texture data
            //
            XmlNode textureDataNode = emitterNode.SelectSingleNode("./TextureData");

            if (textureDataNode != null)
            {
               for (int i = 0; i < 5; i++)
               {
                  XmlNode node = null;
                  switch (i)
                  {
                     case 0: node = textureDataNode.SelectSingleNode("./Diffuse"); break;
                     case 1: node = textureDataNode.SelectSingleNode("./Diffuse2"); break;
                     case 2: node = textureDataNode.SelectSingleNode("./Diffuse3"); break;
                     case 3: node = textureDataNode.SelectSingleNode("./Masks"); break;
                     case 4: node = textureDataNode.SelectSingleNode("./Intensity"); break;
                  }

                  if (node != null)
                  {
                     XmlNodeList textureFileNodes = node.SelectNodes("./Textures/Stage/file");
                     foreach (XmlNode textureFileNode in textureFileNodes)
                     {
                        if (textureFileNode.FirstChild == null)
                           continue;

                        string textureName = "art\\" + textureFileNode.FirstChild.Value;

                        // Replace .tga extension with .ddx
                        textureName = Path.ChangeExtension(textureName, ".ddx");

                        if (!dependenciesFile.Contains(textureName))
                        {
                           // Check file existance
                           bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + textureName);

                           if (!fileExists)
                           {
                              ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Particle File \"{0}\" is referring to texture \"{1}\" which does not exist.\n", filename, textureName);
                           }


                           dependenciesFile.Add(textureName);
                           dependencies.Add(new FileInfo(textureName, fileExists));
                        }
                     }
                  }
               }
            }

         }

         return true;

      }

   }

}
