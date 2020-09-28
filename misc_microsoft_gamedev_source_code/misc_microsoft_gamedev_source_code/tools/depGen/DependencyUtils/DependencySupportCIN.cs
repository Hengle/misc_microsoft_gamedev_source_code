using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using ConsoleUtils;
using DatabaseUtils;


namespace DependencyUtils
{
   class DependencySupportCIN : IDependencyInterface
   {
      public enum ModelType
      {
         gr2 = 0,
         proto
      }

      private string mExtensions = ".cin";
      private static XmlSerializer mXmlSerializerCIN = new XmlSerializer(typeof(cinematic), new Type[] { });

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
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a cinematic file.  The extension must be \"{1}\".\n", filename, mExtensions);
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


         // Only camera files are needed here, since the cinematics are prerendered.  The camera is still needed though to
         // be able to access shot times.  The shot times are required for playing the chats.
         //

         // Load it
         Stream st = File.OpenRead(fileNameAbsolute);
         cinematic cinFile = null;
         try
         {
            cinFile = (cinematic)mXmlSerializerCIN.Deserialize(st);
         }
         catch (System.Exception ex)
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a valid xml cinematic (.cin) file.\n", filename);
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "{0}\n", ex.ToString());
            st.Close();
            return (false);
         }
         st.Close();


         List<string> dependencyFile = new List<string>();
         TechTree tree = new TechTree(Database.m_protoTechs);

/*
         if (cinFile.head != null)
         {
            foreach (cinematicModel model in cinFile.head)
            {
               ModelType type = ModelType.gr2;

               if (!String.IsNullOrEmpty(model.type))
               {
                  if (String.Compare(model.type, "gr2", true) == 0)
                     type = ModelType.gr2;
                  else if (String.Compare(model.type, "proto", true) == 0)
                     type = ModelType.proto;
               }


               switch (type)
               {
                  case ModelType.gr2:
                     // Add model
                     if (!String.IsNullOrEmpty(model.modelfile))
                     {
                        string modelFileName = "art\\" + model.modelfile;
                        modelFileName = String.Concat(modelFileName, ".ugx");

                        if (!dependencyFile.Contains(modelFileName))
                           dependencyFile.Add(modelFileName);
                     } 
                     break;

                  case ModelType.proto:
                     {
                        long unitID;
                        if (Database.m_objectTypes.TryGetValue(model.modelfile.ToLower(), out unitID))
                        {
                           tree.buildUnit(unitID);
                           //tree.applyProtoObjectEffect(unitID);
                        }
                        else
                        {
                           ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Cinematic \"{0}\" is referring to proto unit \"{1}\" which does not exist in objects.xml.\n", filename, model.modelfile);
                        }
                     }
                     break;
               }
            }
         }
*/

         if (cinFile.body != null)
         {
            foreach (cinematicShot shot in cinFile.body)
            {
               // Add camera
               if (!String.IsNullOrEmpty(shot.camera))
               {
                  string cameraFileName = "art\\" + shot.camera;
                  cameraFileName = String.Concat(cameraFileName, ".lgt");

                  if(!dependencyFile.Contains(cameraFileName))
                     dependencyFile.Add(cameraFileName);
               }

               if (shot.tag != null)
               {
                  foreach (cinematicShotTag tag in shot.tag)
                  {
                     // Add talking head
                     if (!String.IsNullOrEmpty(tag.talkinghead))
                     {
                        string talkingHeadFileName = "video\\talkingheads\\" + tag.talkinghead;
                        talkingHeadFileName = String.Concat(talkingHeadFileName, ".bik");

                        if (!dependencyFile.Contains(talkingHeadFileName))
                        {
                           string absoluteTalkingHeadFileName = CoreGlobals.getWorkPaths().mGameDirectory + "\\" + talkingHeadFileName;

                           // check if file exists
                           if (File.Exists(absoluteTalkingHeadFileName))
                           {
                              dependencyFile.Add(talkingHeadFileName);
                           }
                           else
                           {
                              //ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Cinematic \"{0}\" refers to has Talking Head video \"{1}\" which does not exist.\n", filename, talkingHeadFileName);
                           }
                        }
                     }
                  }
               }
/*
               if (shot.animatedmodel != null)
               {
                  foreach (cinematicShotAnimatedmodel animModel in shot.animatedmodel)
                  {
                     // Add animation
                     if (!String.IsNullOrEmpty(animModel.animationfile))
                     {
                        string animationFileName = "art\\" + animModel.animationfile;
                        animationFileName = String.Concat(animationFileName, ".uax");

                        if(!dependencyFile.Contains(animationFileName))
                           dependencyFile.Add(animationFileName);
                     }
                  }
               }
*/
            }
         }


         // Process
         /*
         tree.process();
         */

         tree.getDependencyList(dependencyFile, false);


         foreach (string file in dependencyFile)
         {
            bool fileExists = checkFileExistance(file);
            if (!fileExists)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Cinematic File \"{0}\" is referring to file \"{1}\" which does not exist.\n", filename, file);
            }

            dependencies.Add(new FileInfo(file, fileExists));
         }

         return true;

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
}
