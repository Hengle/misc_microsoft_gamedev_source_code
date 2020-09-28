using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;

using ConsoleUtils;



namespace DependencyUtils
{
   class DependencySupportDEP: IDependencyInterface
   {
      private string mExtensions = ".dep";

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
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a DEP file.  The extensions must be \"{1}\".\n", filename, mExtensions);
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



         XmlDocument depDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(depDoc, fileNameAbsolute);


         // Read dependent files
         //
         XmlNodeList dependentFileNodes = depDoc.SelectNodes("//FileDependencies/File");
         foreach (XmlNode dependentFileNode in dependentFileNodes)
         {
            if (dependentFileNode.FirstChild == null)
               continue;

            string dependentFileName = dependentFileNode.FirstChild.Value;

            // Retrieve isOptional attribute
            bool bIsOptional = false;
            XmlNode isOptionalNode = dependentFileNode.Attributes.GetNamedItem("isOptional");
            if ((isOptionalNode != null) && (isOptionalNode.FirstChild != null))
            {
               bIsOptional = Convert.ToBoolean(isOptionalNode.FirstChild.Value);
            }

            if (!dependenciesFile.Contains(dependentFileName))
            {
               // Check file existance
               bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentFileName);

               if ((!fileExists) && (!bIsOptional))
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Dependency File \"{0}\" is referring to file \"{1}\" which does not exist.\n", filename, dependentFileName);
               }

               if ((fileExists) || (!bIsOptional))
               {
                  dependenciesFile.Add(dependentFileName);
                  dependencies.Add(new FileInfo(dependentFileName, fileExists));
               }
            }
         }

         return true;
      }
   }
}
