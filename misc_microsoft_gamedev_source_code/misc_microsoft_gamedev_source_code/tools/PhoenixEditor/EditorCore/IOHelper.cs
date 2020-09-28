
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using Xceed.Zip;
using Xceed.FileSystem;


namespace EditorCore
{
   public interface IDataStream
   {
      string GetSreamName();
      bool Load(Stream s);
      bool Save(Stream s);
   }

   public class ZipStreamArchive
   {
      Dictionary<string, IDataStream> mStreams = new Dictionary<string, IDataStream>();
      public void RegisterStream(IDataStream stream)
      {
         mStreams[stream.GetSreamName()] = stream;
         //if (!mStreams.ContainsKey(stream.GetSreamName()))
         //{
         //   mStreams[stream.GetSreamName()] = stream;
         //}
      }
      public bool LoadData(string fileName)
      {
         DiskFile zipFile = new DiskFile(fileName);
         if (!zipFile.Exists)
         {
            return false;
         }
         ZipArchive zip = new ZipArchive(zipFile);
         AbstractFile[] files = zip.GetFiles(false, null);
         foreach (AbstractFile file in files)
         {
            if (mStreams.ContainsKey(file.Name))
            {
               Stream s = file.OpenRead();
               mStreams[file.Name].Load(s);
               s.Close();
            }
         }
         return true;
      }
      public bool SaveData(string fileName, bool overwrite, bool checkReadOnly)
      {
         try
         {
            using (PerfSection p = new PerfSection("SaveData() " + Path.GetFileName(fileName)))
            {
               if (File.Exists(fileName))
               {
                  if (!overwrite)
                  {
                     return false;
                  }
                  if (checkReadOnly && File.GetAttributes(fileName) == FileAttributes.ReadOnly)
                  {
                     return false;
                  }
                  File.SetAttributes(fileName, FileAttributes.Normal);
                  File.Delete(fileName);
               }

               DiskFile zipFile = new DiskFile(fileName);
               zipFile.Create();

               if (!zipFile.Exists)
               {
                  return false;
               }
               ZipArchive zip = new ZipArchive(zipFile);

               Dictionary<string, IDataStream>.Enumerator it = mStreams.GetEnumerator();
               while (it.MoveNext() != false)
               {
                  AbstractFile md = zip.CreateFile(it.Current.Key, true);
                  Stream s = md.OpenWrite(true);
                  BufferedStream bs = null;
                  if (CoreGlobals.OutOfMemory == false)
                  {
                     bs = new BufferedStream(s, 10000000);  //~10mb buffer
                     it.Current.Value.Save(bs);
                  }
                  else
                  {
                     it.Current.Value.Save(s);
                  }


                  if (bs != null)
                  {
                     bs.Flush();
                     bs.Close();
                  }
                  else
                  {
                     s.Close();
                  }

               }

            }
            return true;
         }
         catch(System.Exception ex)
         {
            CoreGlobals.FatalEdDataSaveError = true;
            throw ex;
         }
         return false;
      }
   }

}