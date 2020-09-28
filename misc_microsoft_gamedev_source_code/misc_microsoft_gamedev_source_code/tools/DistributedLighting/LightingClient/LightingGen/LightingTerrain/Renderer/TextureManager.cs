using System;
using System.Drawing;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Runtime.InteropServices;
using System.Text;
using System.IO;
using System.Diagnostics;

namespace Rendering
{
   public delegate void TextureManagerCallbackType(string filename);


   public class TextureHandle
   {
      public TextureHandle()
      {
      }
      ~TextureHandle()
      {
         destroy();
      }
      public void destroy()
      {
         if (mTexture != null)
         {
            mTexture.Dispose();
            mTexture = null;
         }
         mCallbackFunction = null;
      }
      
      public void loadFromDisk(string filename,TextureManagerCallbackType callback )
      {
         mFilename = filename;
         mTexture = TextureLoader.FromFile(BRenderDevice.getDevice(), filename);
         mCallbackFunction = callback;
         //Debug.Print("TextureManager loaded: " + filename);
      }
      public void reload()
      {
         if(mTexture!=null)
         {
            mTexture.Dispose();
            mTexture = null;
         }
         mTexture = TextureLoader.FromFile(BRenderDevice.getDevice(), mFilename);
         if (mCallbackFunction!=null)
            mCallbackFunction(mFilename);
      }
      public Texture mTexture=null;

      public string mFilename = "";
      private TextureManagerCallbackType mCallbackFunction=null;
   }
    public class WatchedTextureHandle
    {
      public string mFilename = "";
      public TextureManagerCallbackType mCallbackFunction=null;
      public void reload()
      {
         if (mCallbackFunction!=null)
            mCallbackFunction(mFilename);
      }
    }

   
   public class TextureManager
   {
      
      //-----------------------------------
      public void init()
      {
         mTextureHandles.Clear();

          mFileWatcher = new FileSystemWatcher();
          mFileWatcher.Path = AppDomain.CurrentDomain.BaseDirectory;// EditorCore.CoreGlobals.getWorkPaths().mGameArtDirectory;
          mFileWatcher.Filter = "*.*";
          //mFileWatcher.NotifyFilter = NotifyFilters.LastWrite;
          mFileWatcher.NotifyFilter = NotifyFilters.LastAccess | NotifyFilters.LastWrite | NotifyFilters.CreationTime | NotifyFilters.Size;
       
          mFileWatcher.Changed += new FileSystemEventHandler(mFileWatcher_Changed);
          mFileWatcher.IncludeSubdirectories = true;
          mFileWatcher.EnableRaisingEvents = true;
      }

   
 

      public void destroy()
      {
          if (mFileWatcher != null)
            {
               mFileWatcher.Dispose();
               mFileWatcher = null;
            }
            clearAllTextureHandles();
            clearAllWatchedTextures();
      }

      //---------------------------------------------
      static public Image loadTextureToThumbnail(string filename, int desiredSize)
      {
         string fullFileName = (filename);

         Bitmap bitmap = null;
         if (File.Exists(fullFileName))
         {
            Texture tex = TextureLoader.FromFile(BRenderDevice.getDevice(), fullFileName);
            Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
            bitmap = new Bitmap(img);

            tex.Dispose();
            tex = null;
         }
         else
         {
            return null;
            /*Texture tex = TextureLoader.FromFile(BRenderDevice.getDevice(), EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName);
            Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
            bitmap = new Bitmap(img);

            tex.Dispose();
            tex = null;
             * */
         }

         Image myThumbnail = bitmap.GetThumbnailImage(desiredSize, desiredSize, null, IntPtr.Zero);

         return myThumbnail;
      }

      //---------------------------------------------
      public TextureHandle getTexture(string filename)
      {
         return getTexture(filename, null);
      }
      public TextureHandle getTexture(string filename, TextureManagerCallbackType callback)
      {
         int index = giveLoadedTextureIndex(filename);
         if (index != -1)
            return mTextureHandles[index];

         //texture not found...
         TextureHandle handle = new TextureHandle();
         if (!File.Exists(filename))
            return null;

         handle.loadFromDisk(filename, callback);

         mTextureHandles.Add(handle);
         return mTextureHandles[mTextureHandles.Count - 1];
      }
      //---------------------------------------------
      //CLM call this if you want a texture to be watched, but not loaded (useful for volume textures & texture arrays)
      public WatchedTextureHandle addWatchedTexture(string filename, TextureManagerCallbackType callback)
      {
         for (int i = 0; i < mWatchedTextures.Count; i++)
         {
            if (mWatchedTextures[i].mFilename == filename)
            {
               return mWatchedTextures[i];  //we've already got it in the list
            }
         }

         WatchedTextureHandle handle = new WatchedTextureHandle();
         handle.mFilename = filename;
         handle.mCallbackFunction = callback;
         mWatchedTextures.Add(handle);
         return mWatchedTextures[mWatchedTextures.Count - 1];
      }
      public void removeWatchedTexture(string filename)
      {
         for(int i=0;i<mWatchedTextures.Count;i++)
         {
            if (mWatchedTextures[i].mFilename == filename)
            {
               mWatchedTextures.RemoveAt(i);
               return;
            }
         }
      }
      public void clearAllWatchedTextures()
      {
         mWatchedTextures.Clear();
      }
      //---------------------------------------------
      public void freeTexture(string filename)
      {
         int index = giveLoadedTextureIndex(filename);
         if(index ==-1)
            return;

         mTextureHandles.RemoveAt(index);
      }
      public void reloadTexturesIfNeeded(bool force)
      {
         for(int i=0;i<mTextureIndexesToReload.Count;i++)
         {
            mTextureHandles[mTextureIndexesToReload[i]].reload();
         }
         mTextureIndexesToReload.Clear();
      }
      public void clearAllTextureHandles()
      {
         for (int i = 0; i < mTextureHandles.Count; i++)
            mTextureHandles[i].destroy();
         mTextureHandles.Clear();
      }

      
   
      private int giveLoadedTextureIndex(string filename)
      {
         for (int i = 0; i < mTextureHandles.Count; i++)
         {
            if (mTextureHandles[i].mFilename == filename)
               return i;
         }
         return -1;
      }



      //------------------------------------------------------------------
      void mFileWatcher_Changed(object sender, FileSystemEventArgs e)
      {
         string ext = Path.GetExtension(e.Name);
         for (int k = 0; k < mExtentionsToWatch.Length; k++)
         {
            if (mExtentionsToWatch[k] == ext) //should we ignore that this file changed?
            {
               int index = giveLoadedTextureIndex(e.FullPath);//e.Name);

               //was the file that changed a texture in our list?
               if (index == -1)
                  return;

               mTextureIndexesToReload.Add(index);

             //  CoreGlobals.getEditorMain().mIGUI.ReloadVisibleTextureThumbnails(false);
            }
         }
      }

      //------------------------------------------------------------------
      static private FileSystemWatcher mFileWatcher;

      private List<TextureHandle> mTextureHandles = new List<TextureHandle>();
      private List<WatchedTextureHandle> mWatchedTextures = new List<WatchedTextureHandle>();
      private List<int> mTextureIndexesToReload = new List<int>();

      static private string[] mExtentionsToWatch = new string[]
      {
         ".tga"
      };
   }
}