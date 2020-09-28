using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Text;
using System.IO;
using System.Diagnostics;

using EditorCore;

using Compression;   //Runtime DXT compressor

namespace Rendering
{
   public delegate void TextureManagerCallbackType(string filename);

   public class TextureStats
   {
      static public long mSmallTextureSize = 0;
      static public long mLargeTextureSizes = 0;
      static public long mL16FileSize = 0;

      static public long mCCreate = 0;
      static public long mRaw = 0;
      static public long mCLoad = 0;

      static float cGB = 1073741824;
      const float cMB = 1048576;

      static public string Log()
      {
         return String.Format("Raw Texture Files read: small {0:F0} large {1:F0} l16 {2:F0} ccreate {3:F0} raw {4:F0} cload {5:F0}", mSmallTextureSize / cMB, mLargeTextureSizes / cMB, mL16FileSize / cMB, mCCreate / cMB, mRaw / cMB, mCLoad / cMB);
      }
   }

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
         freeD3DHandle();
         mCallbackFunction = null;
      }



      unsafe void loadAndGrabMip(string filename, int mipToGrab)
      {
         try
         {
            bool bCacheBitmaps = false;
            bool bCacheDXT = true;

            System.IO.FileInfo fileInfo = new System.IO.FileInfo(filename);

            ImageInformation imageInfo = TextureLoader.ImageInformationFromFile(filename);

            Format desiredFormat = Format.X8R8G8B8;
            if (mFilename.EndsWith("_op.tga") || mFilename.EndsWith("_xf.tga"))
               desiredFormat = Format.L8;
            
            int desiredWidth = imageInfo.Width  >> mipToGrab;
            int desiredHeight =imageInfo.Height  >> mipToGrab;
            if (desiredFormat == Format.X8R8G8B8)
            {
               if(desiredWidth < 16 && desiredHeight < 16)
               {
                  mTexture = TextureLoader.FromFile(BRenderDevice.getDevice(), filename, desiredWidth, desiredHeight, 1, 0, desiredFormat, Pool.Managed, Filter.Box, Filter.Box, 0);
                  TextureStats.mSmallTextureSize += fileInfo.Length;
               }
               else
               {
                  Image img;
                  Bitmap bmr;
                  
                  string cachedDXTName = filename.Replace("\\work\\art\\", "\\work\\artcache\\");
                  cachedDXTName = Path.ChangeExtension(cachedDXTName, ".dxt");

                  long dxtFileLen = 0;
                  bool bLoadFromCache = false;
                  if (bCacheDXT && File.Exists(cachedDXTName))
                  {
                     System.IO.FileInfo dxtFileInfo = new System.IO.FileInfo(cachedDXTName);
                     dxtFileLen = dxtFileInfo.Length;
                     if (dxtFileInfo.LastWriteTimeUtc >= fileInfo.LastWriteTimeUtc)
                     {
                        bLoadFromCache = true; //DXT exists and is current
                     }
                  }

                  if (bLoadFromCache == false)
                  {
                     string cacheDir = Path.GetDirectoryName(cachedDXTName);
                     if ((bCacheDXT || bCacheBitmaps) && Directory.Exists(cacheDir) == false)
                     {
                        Directory.CreateDirectory(cacheDir);
                     }

                     //if (!bCacheBitmaps || mipToGrab == 0)
                     {
                        lock (DevIL.sSyncRoot)
                        {
                           img = DevIL.LoadImageFromFile(filename);
                           bmr = new Bitmap(img, desiredWidth, desiredHeight);
                        }
                        TextureStats.mLargeTextureSizes += fileInfo.Length;
                        TextureStats.mRaw += fileInfo.Length;
                     }
                     //else
                     //{
                     //   string cachedBMPName = filename.Replace("\\work\\art\\", "\\work\\artcache\\");
                     //   cachedBMPName = Path.ChangeExtension(cachedBMPName, ".bmp");
                     //   if (File.Exists(cachedBMPName) == false)
                     //   {
                     //      TextureStats.mLargeTextureSizes += fileInfo.Length;
                     //      lock (DevIL.sSyncRoot)
                     //      {
                     //         img = DevIL.LoadImageFromFile(filename);
                     //         bmr = new Bitmap(img, desiredWidth, desiredHeight);
                     //      }
                     //      bmr.Save(cachedBMPName, ImageFormat.Bmp);

                     //      System.IO.FileInfo tfileInfo = new System.IO.FileInfo(cachedBMPName);
                     //      TextureStats.mCCreate += tfileInfo.Length;
                     //   }
                     //   else
                     //   {
                     //      System.IO.FileInfo tfileInfo = new System.IO.FileInfo(cachedBMPName);
                     //      TextureStats.mLargeTextureSizes += tfileInfo.Length;

                     //      lock (DevIL.sSyncRoot)
                     //      {
                     //         img = DevIL.LoadImageFromFile(cachedBMPName);
                     //         bmr = new Bitmap(img, desiredWidth, desiredHeight);
                     //      }
                     //   }
                     //}


                     Rectangle r = new Rectangle(0, 0, desiredWidth, desiredHeight);
                     BitmapData sourceData = bmr.LockBits(r, ImageLockMode.ReadOnly, bmr.PixelFormat);

                     byte* pInData = (byte*)sourceData.Scan0;
                     int size = 0;
                     //  byte[] pko = new byte[desiredWidth * desiredHeight * 4];
                     // fixed (byte* pPko = pko)
                     //DXT_CLI.compressDXT(pInData, desiredWidth, desiredHeight, 1, pPko, &size);

                     mTexture = new Texture(BRenderDevice.getDevice(), desiredWidth, desiredHeight, 1, Usage.None, Format.Dxt1, Pool.Managed);
                     GraphicsStream gsOut = mTexture.LockRectangle(0, LockFlags.None);
                     byte* pOutData = (byte*)gsOut.InternalDataPointer;

                     DXT_CLI.compressDXT(pInData, desiredWidth, desiredHeight, 1, pOutData, &size);
                     //  for (int i = 0; i < size; i++)
                     //    pOutData[i] = pko[i];

                     if (bCacheDXT)
                     {
                        byte[] byteArrayName = new byte[size];
                        Marshal.Copy(new IntPtr(pOutData), byteArrayName, 0, size);
                        BinaryWriter wr = new BinaryWriter(new FileStream(cachedDXTName, FileMode.Create));
                        wr.Write(size);
                        wr.Write(byteArrayName, 0, size);
                        wr.Close();

                        TextureStats.mCCreate += size;
                     }

                     
                     mTexture.UnlockRectangle(0);
                     bmr.UnlockBits(sourceData);

                     gsOut = null;
                     pOutData = null;
                     bmr = null;
                     pInData = null;
                     img = null;
                     //pko = null;

                  }
                  else  //Load DXT from cached file
                  {
                     BinaryReader r = new BinaryReader(new FileStream(cachedDXTName, FileMode.Open));
                     int size;
                     size = r.ReadInt32();

                     if (size != (desiredWidth * desiredHeight * 0.5))
                     {
                        r.Close();
                        File.Delete(cachedDXTName);
                        loadAndGrabMip(filename, mipToGrab);
                        return;
                     }

                     byte[] byteArrayName = new byte[size];
                     r.Read(byteArrayName, 0, size);
                     //long bytesread = r.BaseStream.Position;
                     r.Close();

                     //byte* pByteArrayName = stackalloc byte[size];
                     //IntPtr pByteArrayName = Marshal.AllocHGlobal(size);
                     //Marshal.Copy(byteArrayName, 0, pByteArrayName, size);


                     TextureStats.mLargeTextureSizes += size;
                     TextureStats.mCLoad += size;

                     mTexture = new Texture(BRenderDevice.getDevice(), desiredWidth, desiredHeight, 1, Usage.None, Format.Dxt1, Pool.Managed);
                     GraphicsStream gsOut = mTexture.LockRectangle(0, LockFlags.None);
                     byte* pOutData = (byte*)gsOut.InternalDataPointer;

                     Marshal.Copy(byteArrayName, 0, (IntPtr)pOutData, size);

                     //byte* pBytes = (byte*)(pByteArrayName.ToPointer());
                     //for (int i = 0; i < size; i++)
                     //{
                     //   pOutData[i] = pBytes[i];
                     //}

                     mTexture.UnlockRectangle(0);
                  }
               }

            }
            else if(desiredFormat == Format.L8)
            {
               mTexture = TextureLoader.FromFile(BRenderDevice.getDevice(), filename, desiredWidth, desiredHeight, 1, 0, desiredFormat, Pool.Managed, Filter.Box, Filter.Box, 0);
               TextureStats.mL16FileSize += fileInfo.Length;
            }
            else 
            {
               Debug.Assert(false);
            }
         }
         catch (OutOfVideoMemoryException ve)
         {
            MessageBox.Show("Out of video memory.. Talk to the editor guys..");
         }
         catch (OutOfMemoryException me)
         {
            MessageBox.Show("Out of System memory.. Talk to the editor guys..");
         }

      }

      public void loadFromDisk(string filename, TextureManagerCallbackType callback)
      {
         loadFromDisk(filename, callback,0);
      }
      public void loadFromDisk(string filename, TextureManagerCallbackType callback, int mipLevelToUse)
      {
         mMipLevelUsedFromFile = mipLevelToUse;
         mFilename = filename;
         loadAndGrabMip(filename, mipLevelToUse);// TextureLoader.FromFile(BRenderDevice.getDevice(), filename, 0, 0, 1, 0, 0, 0, Filter.Linear, Filter.Linear, 0);// .FromFile(BRenderDevice.getDevice(), filename);
         mCallbackFunction = callback;
      }
      public void reload()
      {
         freeD3DHandle();
         loadAndGrabMip(mFilename, mMipLevelUsedFromFile);// TextureLoader.FromFile(BRenderDevice.getDevice(), mFilename, 0, 0, 1, 0, 0, 0, Filter.Linear, Filter.Linear, 0);//TextureLoader.FromFile(BRenderDevice.getDevice(), mFilename);
         if (mCallbackFunction!=null)
            mCallbackFunction(mFilename);
      }
      public void freeD3DHandle()
      {
         if (mTexture != null)
         {
            mTexture.Dispose();
            mTexture = null;
         }
      }
      public Texture mTexture=null;

      public string mFilename = "";
      public int mMipLevelUsedFromFile = 0;
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
      public enum eDefaultTextures
      {
         cDefTex_White =0,
         cDefTex_Black,
         cDefTex_Red,
         cDefTex_Green,
         cDefTex_Blue,
         cDefTex_Yellow,
         cDefTex_Count
      }
      Texture[] mDefaultTextures = new Texture[(int)eDefaultTextures.cDefTex_Count];
      unsafe private void initDefaultTextures()
      {
         int texSize = 2;
         mDefaultTextures[(int)eDefaultTextures.cDefTex_White] = new Texture(BRenderDevice.getDevice(), texSize, texSize, 1, Usage.None, Format.A8R8G8B8, Pool.Managed);
         GraphicsStream streamLht = mDefaultTextures[(int)eDefaultTextures.cDefTex_White].LockRectangle(0, 0);
         uint* lht = (uint*)streamLht.InternalDataPointer;
         lht[0] = 0xFFFFFFFF;
         mDefaultTextures[(int)eDefaultTextures.cDefTex_White].UnlockRectangle(0);


         mDefaultTextures[(int)eDefaultTextures.cDefTex_Black] = new Texture(BRenderDevice.getDevice(), texSize, texSize, 1, Usage.None, Format.A8R8G8B8, Pool.Managed);
         streamLht = mDefaultTextures[(int)eDefaultTextures.cDefTex_Black].LockRectangle(0, 0);
         lht = (uint*)streamLht.InternalDataPointer;
         lht[0] = 0xFF000000;
         mDefaultTextures[(int)eDefaultTextures.cDefTex_Black].UnlockRectangle(0);

         mDefaultTextures[(int)eDefaultTextures.cDefTex_Red] = new Texture(BRenderDevice.getDevice(), texSize, texSize, 1, Usage.None, Format.A8R8G8B8, Pool.Managed);
         streamLht = mDefaultTextures[(int)eDefaultTextures.cDefTex_Red].LockRectangle(0, 0);
         lht = (uint*)streamLht.InternalDataPointer;
         lht[0] = 0xFFFF0000;
         mDefaultTextures[(int)eDefaultTextures.cDefTex_Red].UnlockRectangle(0);

         mDefaultTextures[(int)eDefaultTextures.cDefTex_Green] = new Texture(BRenderDevice.getDevice(), texSize, texSize, 1, Usage.None, Format.A8R8G8B8, Pool.Managed);
         streamLht = mDefaultTextures[(int)eDefaultTextures.cDefTex_Green].LockRectangle(0, 0);
         lht = (uint*)streamLht.InternalDataPointer;
         lht[0] = 0xFF00FF00;
         mDefaultTextures[(int)eDefaultTextures.cDefTex_Green].UnlockRectangle(0);

         mDefaultTextures[(int)eDefaultTextures.cDefTex_Blue] = new Texture(BRenderDevice.getDevice(), texSize, texSize, 1, Usage.None, Format.A8R8G8B8, Pool.Managed);
         streamLht = mDefaultTextures[(int)eDefaultTextures.cDefTex_Blue].LockRectangle(0, 0);
         lht = (uint*)streamLht.InternalDataPointer;
         lht[0] = 0xFF0000FF;
         mDefaultTextures[(int)eDefaultTextures.cDefTex_Blue].UnlockRectangle(0);

         mDefaultTextures[(int)eDefaultTextures.cDefTex_Yellow] = new Texture(BRenderDevice.getDevice(), texSize, texSize, 1, Usage.None, Format.A8R8G8B8, Pool.Managed);
         streamLht = mDefaultTextures[(int)eDefaultTextures.cDefTex_Yellow].LockRectangle(0, 0);
         lht = (uint*)streamLht.InternalDataPointer;
         lht[0] = 0xFFFFFF00;
         mDefaultTextures[(int)eDefaultTextures.cDefTex_Yellow].UnlockRectangle(0);

      }
      public Texture getDefaultTexture(eDefaultTextures id)
      {
         return mDefaultTextures[(int)id];
      }
      //-----------------------------------
      public void init()
      {
         mTextureHandles.Clear();

          mFileWatcher = new FileSystemWatcher();
          mFileWatcher.Path = EditorCore.CoreGlobals.getWorkPaths().mGameArtDirectory;
          mFileWatcher.Filter = "*.*";
          //mFileWatcher.NotifyFilter = NotifyFilters.LastWrite;
          mFileWatcher.NotifyFilter = /*NotifyFilters.LastAccess |*/ NotifyFilters.LastWrite | NotifyFilters.CreationTime | NotifyFilters.Size;
       
          mFileWatcher.Changed += new FileSystemEventHandler(mFileWatcher_Changed);
          mFileWatcher.IncludeSubdirectories = true;
          mFileWatcher.EnableRaisingEvents = true;

          initDefaultTextures();
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

         for(int i=0;i<(int)eDefaultTextures.cDefTex_Count;i++)
         {
            if(mDefaultTextures[i]!=null)
            {
               mDefaultTextures[i].Dispose();
               mDefaultTextures[i] = null;
            }
         }

      }

      //---------------------------------------------
      static public Image loadTextureToThumbnail(string filename, int desiredSize)
      {
         string fullFileName = (filename);

         try
         {

            Bitmap bitmap = null;
            if (File.Exists(fullFileName))
            {

               try
               {
                  Texture tex = TextureLoader.FromFile(BRenderDevice.getDevice(), fullFileName);
                  Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
                  bitmap = new Bitmap(img);

                  tex.Dispose();
                  tex = null;
               }
               catch (System.Exception ex)
               {
                  Texture tex = BRenderDevice.getTextureManager().getDefaultTexture(eDefaultTextures.cDefTex_Red);
                  Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
                  bitmap = new Bitmap(img);
                  tex.Dispose();
                  tex = null;

                  ex.ToString();
               }
            }
            else
            {

               Texture tex = BRenderDevice.getTextureManager().getDefaultTexture(eDefaultTextures.cDefTex_Red);
               try
               {
                  Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
                  bitmap = new Bitmap(img);
               }
               catch (System.Exception ex)
               {
                  bitmap = null;

                  ex.ToString();
               }


               tex.Dispose();
               tex = null;
            }
            if (bitmap != null)
               return bitmap.GetThumbnailImage(desiredSize, desiredSize, null, IntPtr.Zero);

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }


         return null;



      }

      //---------------------------------------------
      public TextureHandle getTexture(string filename)
      {
         return getTexture(filename, null,0);
      }
      public TextureHandle getTexture(string filename, TextureManagerCallbackType callback)
      {
         return getTexture(filename, callback, 0);
      }
      public TextureHandle getTexture(string filename, TextureManagerCallbackType callback, int mipLevelToUse)
      {
         //filename = EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName;


         int index = giveLoadedTextureIndex(filename);
         if (index != -1)
            return mTextureHandles[index];

         //texture not found...
         TextureHandle handle = new TextureHandle();
         if (!File.Exists(filename))
            filename = EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName;

         handle.loadFromDisk(filename, callback, mipLevelToUse);

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
         if(force)
         {
            for (int i = 0; i < mTextureHandles.Count; i++)
            {
               mTextureHandles[i].reload();
            }
         }
         else
         {
            if(mTextureHandles.Count != 0)
               for (int i = 0; i < mTextureIndexesToReload.Count; i++)
               {
                  if (mTextureIndexesToReload[i] > mTextureHandles.Count)
                     continue;

                  mTextureHandles[mTextureIndexesToReload[i]].reload();
               }
            mTextureIndexesToReload.Clear();
         }
         
      }
      public void clearAllTextureHandles()
      {
         for (int i = 0; i < mTextureHandles.Count; i++)
            mTextureHandles[i].destroy();
         mTextureHandles.Clear();
      }
      public void freeAllD3DTextureHandles()//CLM DON'T CALL THIS UNLESS YOU KNOW WHAT YOU'RE DOING!!!
      {
         for (int i = 0; i < mTextureHandles.Count; i++)
            mTextureHandles[i].freeD3DHandle();
      }

      
   
      private int giveLoadedTextureIndex(string filename)
      {
         for (int i = 0; i < mTextureHandles.Count; i++)
         {
            if (mTextureHandles[i].mFilename.ToLower() == filename.ToLower())
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

               CoreGlobals.getEditorMain().mIGUI.ReloadVisibleTextureThumbnails(false);
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