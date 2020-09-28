using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

using ConsoleUtils;

namespace DependencyUtils
{
   class FileInfo
   {
      public string   mFilename = null;
      public bool     bExists = false;
      public long     lFilesize = 0;

      public FileInfo(string filename, bool exists)
      {
         mFilename = filename;
         bExists = exists;
         if (bExists)
         {
             lFilesize = new System.IO.FileInfo(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + filename).Length;
         }
      }
   }


   interface IDependencyInterface
   {
      bool     getDependencyList(string fileinfo, List<FileInfo> dependencies, List<string> dependentUnits);
      string   getExtensions();
   }



   static class DependencyManager
   {
      private static Dictionary<string, IDependencyInterface> supportedExtensions = new Dictionary<string, IDependencyInterface>();

      static DependencyManager()
      {
         addDependencyType(new DependencySupportSCN());
         addDependencyType(new DependencySupportDEP());
         addDependencyType(new DependencySupportVIS());
         addDependencyType(new DependencySupportPFX());
         addDependencyType(new DependencySupportTFX());
         addDependencyType(new DependencySupportCIN());
         addDependencyType(new DependencySupportDMG());
         addDependencyType(new DependencySupportPHYSICS());
         addDependencyType(new DependencySupportBinary());
      }

      public static void addDependencyType(IDependencyInterface depInterface)
      {
         string extensionList = depInterface.getExtensions();

         while (!string.IsNullOrEmpty(extensionList))
         {
            string extension;

            int posOfComma = extensionList.IndexOf(',');
            if (posOfComma == -1)
            {
               extension = extensionList.TrimEnd(null);
               extensionList = null;
            }
            else
            {
               extension = extensionList.Remove(posOfComma).TrimEnd(null);
               extensionList = extensionList.Remove(0, posOfComma);
               extensionList = extensionList.TrimStart(',');
            }

            supportedExtensions.Add(extension, depInterface);
         }
      }


      /*
      public static void getDependencyList(string filename, List<string> dependencies)
      {
         string fileExtension = Path.GetExtension(filename).ToLower();

         IDependencyInterface depInterface;
         if (supportedExtensions.TryGetValue(fileExtension, out depInterface))
         {
            depInterface.getDependencyList(filename, dependencies);
         }
      }
      */

      public static void getDependencyList(string filename, List<FileInfo> dependencies, List<string> dependentUnits)
      {
         string fileExtension = Path.GetExtension(filename).ToLower();

         IDependencyInterface depInterface;
         if (supportedExtensions.TryGetValue(fileExtension, out depInterface))
         {
            depInterface.getDependencyList(filename, dependencies, dependentUnits);
         }
      }
   }
}
