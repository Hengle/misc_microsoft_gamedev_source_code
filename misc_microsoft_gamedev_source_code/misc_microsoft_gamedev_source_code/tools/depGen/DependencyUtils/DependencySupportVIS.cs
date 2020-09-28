using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using ConsoleUtils;



namespace DependencyUtils
{
   class DependencySupportVIS : IDependencyInterface
   {
      private string mExtensions = ".vis";
      private static XmlSerializer mXmlSerializerVIS = new XmlSerializer(typeof(visual), new Type[] { });

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
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a VIS file.  The extension must be \"{1}\".\n", filename, mExtensions);
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


         VisContainer container = new VisContainer();


         // Load it
         Stream st = File.OpenRead(fileNameAbsolute);
         visual visualFile = null;
         try
         {
            visualFile = (visual)mXmlSerializerVIS.Deserialize(st);
         }
         catch (System.Exception ex)
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a valid xml Visual (.vis) file.\n", filename);
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "{0}\n", ex.ToString());
            st.Close();
            return (false);
         }
         st.Close();

         // Validate the Vis file - This 
         bool isValid = visualFile.isBranchValid();

         if (!isValid)
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Warn, "File \"{0}\" has failed the validation pass.\n", filename);
         }




         foreach (visualModel model in visualFile.model)
         {
            // Process component
            //
            if (model.component != null)
            {
               // Look at component asset (if it exists)
               if (model.component.asset != null)
               {
                  processComponentAsset(model.component.asset, container, filename);
               }

               // Loot at logic (if it exists)
               if (model.component.logic != null)
               {
                  processLogicNode(model.component.logic, container, filename);
               }

               // Look at attachments
               foreach (visualModelComponentOrAnimAttach attach in model.component.attach)
               {
                  processComponentOrAnimAttach(attach, container, filename);
               }
            }


            // Process anims
            //
            int i = 0;
            foreach (visualModelAnim anim in model.anim)
            {
               i++;
               // Look at animation files
               foreach (visualModelAnimAsset animAsset in anim.asset)
               {
                  container.addAnimation(animAsset.file, filename);

                  // Add tags
                  foreach (visualModelAnimAssetTag tag in animAsset.tag)
                  {
                     processAnimAssetTag(tag, container, filename);
                  }
               }

               // Look at attachments
               foreach (visualModelComponentOrAnimAttach attach in anim.attach)
               {
                  processComponentOrAnimAttach(attach, container, filename);
               }
            }
         }


         // Copy into dependency list
         //
         foreach (string file in container.ugxs)
         {
            bool fileExists = checkFileExistance(file);
            if (!fileExists)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Visual File \"{0}\" is referring to Model \"{1}\" which does not exist.\n", filename, file);
            }

            dependencies.Add(new FileInfo(file, fileExists));
         }

         foreach (string file in container.uaxs)
         {
            bool fileExists = checkFileExistance(file);
            if (!fileExists)
            {
               // This isn't really important
               //ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Visual File \"{0}\" is referring to Animation \"{1}\" which does not exist\n", filename, file);
            }

            dependencies.Add(new FileInfo(file, fileExists));
         }

         foreach (string file in container.pfxs)
         {
            bool fileExists = checkFileExistance(file);
            if (!fileExists)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Visual File \"{0}\" is referring to ParticleSystem \"{1}\" which does not exist.\n", filename, file);
            }

            dependencies.Add(new FileInfo(file, fileExists));
         }

         foreach (string file in container.tfxs)
         {
            bool fileExists = checkFileExistance(file);
            if (!fileExists)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Visual File \"{0}\" is referring to TerrainEffect \"{1}\" which does not exist.\n", filename, file);
            }

            dependencies.Add(new FileInfo(file, fileExists));
         }

         foreach (string file in container.lgts)
         {
            bool fileExists = checkFileExistance(file);
            if (!fileExists)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Visual File \"{0}\" is referring to Light \"{1}\" which does not exist.\n", filename, file);
            }

            dependencies.Add(new FileInfo(file, fileExists));
         }

         foreach (string file in container.dmgs)
         {
            bool fileExists = checkFileExistance(file);
            if (!fileExists)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Visual File \"{0}\" is referring to Damage \"{1}\" which does not exist.\n", filename, file);
            }

            dependencies.Add(new FileInfo(file, fileExists));
         }

         foreach (string file in container.ddxs)
         {
            bool fileExists = checkFileExistance(file);
            if (!fileExists)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Visual File \"{0}\" is referring to Decal texture \"{1}\" which does not exist.\n", filename, file);
            }

            dependencies.Add(new FileInfo(file, fileExists));
         }

         return true;

      }

      private void processComponentAsset(visualModelComponentAsset asset, VisContainer container, string filename)
      {
         switch (asset.type)
         {
            case visualModelComponentAsset.ComponentAssetType.Light:
               container.addLight(asset.file, filename);
               break;
            case visualModelComponentAsset.ComponentAssetType.Model:
               container.addModel(asset.file, filename);

               if (!String.IsNullOrEmpty(asset.damagefile))
               {
                  container.addDamage(asset.damagefile, filename);
               }
               break;
            case visualModelComponentAsset.ComponentAssetType.Particle:
               container.addParticle(asset.file, filename);
               break;
         }
      }

      private void processComponentOrAnimAttach(visualModelComponentOrAnimAttach attach, VisContainer container, string filename)
      {
         switch (attach.type)
         {
            case visualModelComponentOrAnimAttach.AttachType.LightFile:
               container.addLight(attach.name, filename);
               break;
            case visualModelComponentOrAnimAttach.AttachType.ModelFile:
               container.addModel(attach.name, filename);
               break;
            case visualModelComponentOrAnimAttach.AttachType.ModelRef:
               break;
            case visualModelComponentOrAnimAttach.AttachType.ParticleFile:
               container.addParticle(attach.name, filename);
               break;
            case visualModelComponentOrAnimAttach.AttachType.TerrainEffect:
               container.addTerrainEffect(attach.name, filename);
               break;
         }
      }

      private void processLogicNode(visualLogic logic, VisContainer container, string filename)
      {
         foreach (visualLogicData data in logic.logicdata)
         {
            if (data.asset != null)
            {
               processComponentAsset(data.asset, container, filename);
            }

            if (data.logic != null)
            {
               processLogicNode(data.logic, container, filename);
            }
         }
      }

      private void processAnimAssetTag(visualModelAnimAssetTag tag, VisContainer container, string filename)
      {
         switch (tag.type)
         {
            case visualModelAnimAssetTag.TagType.Light:
               container.addLight(tag.name, filename);
               break;
            case visualModelAnimAssetTag.TagType.Particle:
               container.addParticle(tag.name, filename);
               break;
            case visualModelAnimAssetTag.TagType.TerrainEffect:
               container.addTerrainEffect(tag.name, filename);
               break;
            case visualModelAnimAssetTag.TagType.BuildingDecal:
               container.addDecalTexture(tag.name, filename);
               break;
         }
      }

      private bool checkFileExistance(string filename)
      {
         // check if file exists
         string filenameAbsolute = CoreGlobals.getWorkPaths().mGameDirectory + "\\" + filename;

         // check if file exists
         if (!File.Exists(filenameAbsolute))
            return (false);

         return (true);
      }
   }



   public class VisContainer
   {
      public List<string> ugxs = new List<string>();
      public List<string> uaxs = new List<string>();
      public List<string> pfxs = new List<string>();
      public List<string> tfxs = new List<string>();
      public List<string> lgts = new List<string>();
      public List<string> dmgs = new List<string>();
      public List<string> ddxs = new List<string>();

      public bool addModel(string filename, string visName)
      {
         filename = String.Concat("art\\", filename, ".ugx");

         if (!ugxs.Contains(filename))
            ugxs.Add(filename);

         return true;
      }

      public bool addDamage(string filename, string visName)
      {
         filename = String.Concat("art\\", filename, ".dmg");

         if (!dmgs.Contains(filename))
            dmgs.Add(filename);

         return true;
      }

      public bool addAnimation(string filename, string visName)
      {
         filename = String.Concat("art\\", filename, ".uax");

         if (!uaxs.Contains(filename))
            uaxs.Add(filename);

         return true;
      }

      public bool addParticle(string filename, string visName)
      {
         filename = String.Concat("art\\", filename, ".pfx");

         if (!pfxs.Contains(filename))
            pfxs.Add(filename);

         return true;
      }

      public bool addTerrainEffect(string filename, string visName)
      {
         filename = String.Concat("art\\", filename, ".tfx");

         if (!tfxs.Contains(filename))
            tfxs.Add(filename);

         return true;
      }

      public bool addLight(string filename, string visName)
      {
         filename = String.Concat("art\\", filename, ".lgt");

         if (!lgts.Contains(filename))
            lgts.Add(filename);

         return true;
      }

      public bool addDecalTexture(string filename, string visName)
      {
         filename = String.Concat("art\\", filename, ".ddx");

         if (!ddxs.Contains(filename))
            ddxs.Add(filename);

         return true;
      }
   }
}
