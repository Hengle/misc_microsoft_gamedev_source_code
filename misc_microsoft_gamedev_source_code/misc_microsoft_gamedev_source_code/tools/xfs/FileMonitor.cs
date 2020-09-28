using System;
using System.IO;
using System.Windows.Forms;
using System.Diagnostics;
using System.Collections.Generic;
using XDevkit;

//CLM [04.21.06] This file is responsible for detecting changes in files that the 360 is currently using.
//    and issuing the appropriate commands back to the 360

namespace xfs
{
   class openedFile
   {
      public openedFile(string name)
      {
         mFileName = name;
      }
      public string mFileName;
   };
   
   static class FileMonitor
   {
      static private Object mMutex = new Object();
      //static private List<openedFile> mActiveFiles = new List<openedFile>();
      static private FileSystemWatcher mFileWatcher;
      static private string[] mIgnoreExtentions = new string[]
      {
         ".rec",
         ".txt",
         ".exe",
         ".dll",
         ".xex",
         ".ted",
         ".tga",
         ".ddt",   
         ".psd",
         ".max",
         ".bat",
         ".dll",
         ".bmp",
         ".tmp"
      };
      static private string mDataPath;

      static private bool mEnabled = true;

      public static void SetEnabled(bool vaue)
      {
         lock (mMutex)
         {
            mEnabled = vaue;
         }
      }

      public static bool GetEnabled()
      {
         lock (mMutex)
         {
            return mEnabled;
         }
      }

      public static void init(string path)
      {
         lock (mMutex)
         {
            mDataPath = path;

            mFileWatcher = new FileSystemWatcher();
            mFileWatcher.Path = path;
            mFileWatcher.Filter = "*.*";
            mFileWatcher.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.Size | NotifyFilters.CreationTime;
            mFileWatcher.Changed += new FileSystemEventHandler(mFileWatcher_Changed);
            mFileWatcher.Created += new FileSystemEventHandler(mFileWatcher_Changed);
            mFileWatcher.Deleted += new FileSystemEventHandler(mFileWatcher_Changed);
            mFileWatcher.IncludeSubdirectories = true;
            mFileWatcher.EnableRaisingEvents = true;
         }
      }

      public static void destroy()
      {
         lock (mMutex)
         {
            if (mFileWatcher != null)
            {
               mFileWatcher.Dispose();
               mFileWatcher = null;
            }
            FileMonitor.clearList();
         }
      }
      
      static void mFileWatcher_Changed(object sender, FileSystemEventArgs e)
      {
         lock (mMutex)
         {
            // rg [5/2/07] - I'm changing this to always just send the event when a file is changes, this should 
            // be harmless.
            //if (mEnabled && fileUsed(e.Name))
            if (mEnabled && !shouldIgnore(e.Name))
            {
               try
               {
                  DirectoryInfo dirInfo = new DirectoryInfo(e.FullPath);
                  // Don't return events for paths, just filenames.
                  if (dirInfo.Exists)
                     return;
               }
               catch (Exception)
               {
               }                     
            
               // Incoming string should already be relative to the path.
               PhxGameInterface.reloadFile(e.Name);
            }
         }
      }
      
      public static void clearList()
      {
/*
         lock (mMutex)
         {
            mActiveFiles.Clear();
         }
*/
      }

      private static bool shouldIgnore(string filename)
      {
         string ext = Path.GetExtension(filename).ToLower();
         for (int k = 0; k < mIgnoreExtentions.Length; k++)
            if (mIgnoreExtentions[k] == ext)
               return true;
         
         return false;
      }

/*
      public static bool fileUsed(string filename)
      {
         lock (mMutex)
         {
            if (shouldIgnore(filename))
               return false;

            string fullPath = mDataPath + @"\" + filename;
            fullPath = fullPath.ToLower();

            // Check if it exists.
            for (int i = 0; i < mActiveFiles.Count; i++)
            {
               if (mActiveFiles[i].mFileName.ToLower() == fullPath)
                  return true;
            }
         }
         return false;
      }
*/

      public static void addFile(string filename)
      {
/*
         lock (mMutex)
         {
            if (shouldIgnore(filename))
               return;

            // Make sure it doesn't already exist.
            for (int i = 0; i < mActiveFiles.Count; i++)
            {
               if (mActiveFiles[i].mFileName == filename)
                  return;
            }

            mActiveFiles.Add(new openedFile(filename));
         }
*/
      }
            
   }
}