using System;
using System.Drawing;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Text;
using System.IO;
using System.Diagnostics;

using EditorCore;

namespace Rendering
{

   public delegate void ShaderManagerCallbackType(string filename);

   public class ShaderHandle
   {
      public ShaderHandle()
      {
      }
      ~ShaderHandle()
      {
         try
         {
            destroy();
         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }
      public void destroy()
      {
         if (mShader != null)
         {
            mShader.Dispose();
            mShader = null;
         }
         mCallbackFunction = null;
      }

      void loadInternal(string filename)
      {
         string errors = "";
         {
            try
            {
               mShader = Microsoft.DirectX.Direct3D.Effect.FromFile(BRenderDevice.getDevice(), filename, null/*defines*/, null, null, ShaderFlags.None, null, out errors);
               if (mShader == null)
               {
                  MessageBox.Show("Shader did not load:\n " + errors);
                  return;
               }
            }
            catch (System.Exception ex)
            {
               MessageBox.Show("Shader did not load");
               throw (ex);
            }
         }
      }
      public void loadFromDisk(string filename, ShaderManagerCallbackType callback)
      {
         mFilename = filename;
         mCallbackFunction = callback;

         loadInternal(filename);
      }
      public void reload()
      {
         if (mShader != null)
         {
            mShader.Dispose();
            mShader = null;
         }

         loadInternal(mFilename);

         if (mCallbackFunction != null)
            mCallbackFunction(mFilename);
      }

      public EffectHandle getEffectParam(string paramName)
      {
         EffectHandle eh = mShader.GetParameter(null, paramName);
         if (eh == null)
            MessageBox.Show("EffectParam " + paramName + " not found in file " + mFilename);

         return eh;
      }

      public Microsoft.DirectX.Direct3D.Effect mShader = null;
      public string mFilename = "";
      private ShaderManagerCallbackType mCallbackFunction = null;
   }
   public class WatchedShaderHandle
   {
      public string mFilename = "";
      public ShaderManagerCallbackType mCallbackFunction = null;
      public void reload()
      {
         if (mCallbackFunction != null)
            mCallbackFunction(mFilename);
      }
   }

   public class ShaderManager
   {
      
      public void init()
      {
         mShaderHandles.Clear();

         mFileWatcher = new FileSystemWatcher();
         mFileWatcher.Path = EditorCore.CoreGlobals.getWorkPaths().mEditorShaderDirectory;
         mFileWatcher.Filter = "*.*";
         //mFileWatcher.NotifyFilter = NotifyFilters.LastWrite;
         mFileWatcher.NotifyFilter = NotifyFilters.LastAccess | NotifyFilters.LastWrite | NotifyFilters.CreationTime | NotifyFilters.Size;

         mFileWatcher.Changed += new FileSystemEventHandler(mFileWatcher_Changed);
         mFileWatcher.IncludeSubdirectories = true;
         mFileWatcher.EnableRaisingEvents = true;

         mFFS.init();
      }
      public void destroy()
      {
         if (mFileWatcher != null)
         {
            mFileWatcher.Dispose();
            mFileWatcher = null;
         }
         clearAllShaderHandles();
         clearAllWatchedShaders();

         mFFS.deinit();
      }

      //---------------------------------------------
      public ShaderHandle getShader(string filename)
      {
         return getShader(filename, null);
      }
      public ShaderHandle getShader(string filename, ShaderManagerCallbackType callback)
      {
         int index = giveLoadedShaderIndex(filename);
         if (index != -1)
            return mShaderHandles[index];

         //Shader not found...
         ShaderHandle handle = new ShaderHandle();
         if (!File.Exists(filename))
         {
            MessageBox.Show(filename + " Not found");
            return null;
         }

         handle.loadFromDisk(filename, callback);

         mShaderHandles.Add(handle);
         return mShaderHandles[mShaderHandles.Count - 1];
      }
      //---------------------------------------------
      //---------------------------------------------
      //CLM call this if you want a Shader to be watched, but not loaded (useful for volume Shaders & Shader arrays)
      public WatchedShaderHandle addWatchedShader(string filename, ShaderManagerCallbackType callback)
      {
         for (int i = 0; i < mWatchedShaders.Count; i++)
         {
            if (mWatchedShaders[i].mFilename == filename)
            {
               return mWatchedShaders[i];  //we've already got it in the list
            }
         }

         WatchedShaderHandle handle = new WatchedShaderHandle();
         handle.mFilename = filename;
         handle.mCallbackFunction = callback;
         mWatchedShaders.Add(handle);
         return mWatchedShaders[mWatchedShaders.Count - 1];
      }
      public void removeWatchedShader(string filename)
      {
         for (int i = 0; i < mWatchedShaders.Count; i++)
         {
            if (mWatchedShaders[i].mFilename == filename)
            {
               mWatchedShaders.RemoveAt(i);
               return;
            }
         }
      }
      public void clearAllWatchedShaders()
      {
         mWatchedShaders.Clear();
      }
      //---------------------------------------------
      public void freeShader(string filename)
      {
         int index = giveLoadedShaderIndex(filename);
         if (index == -1)
            return;

         mShaderHandles.RemoveAt(index);
      }
      public void reloadShadersIfNeeded(bool force)
      {
         if (force == false)
         {
            for (int i = 0; i < mShaderIndexesToReload.Count; i++)
            {
               mShaderHandles[mShaderIndexesToReload[i]].reload();
            }
         }
         else if (force)
         {
            foreach (ShaderHandle h in mShaderHandles)
            {
               h.reload();
            }
            mFFS.deinit();
            mFFS.init();
         }

         mShaderIndexesToReload.Clear();
      }
      public void clearAllShaderHandles()
      {
         for (int i = 0; i < mShaderHandles.Count; i++)
            mShaderHandles[i].destroy();
         mShaderHandles.Clear();
      }



      private int giveLoadedShaderIndex(string filename)
      {
         for (int i = 0; i < mShaderHandles.Count; i++)
         {
            if (mShaderHandles[i].mFilename == filename)
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
               int index = giveLoadedShaderIndex(e.FullPath);//e.Name);

               //was the file that changed a Shader in our list?
               if (index == -1)
                  return;

               mShaderIndexesToReload.Add(index);
            }
         }
      }

      //------------------------------------------------------------------
      static private FileSystemWatcher mFileWatcher;

      private List<ShaderHandle> mShaderHandles = new List<ShaderHandle>();
      private List<WatchedShaderHandle> mWatchedShaders = new List<WatchedShaderHandle>();
      private List<int> mShaderIndexesToReload = new List<int>();

      static private string[] mExtentionsToWatch = new string[]
      {
         ".fx"
      };



      FixedFuncShaders mFFS = new FixedFuncShaders();
      
      public VertexShader getFFVS(FixedFuncShaders.eFixedFuncShaderIndex id)
      {
         return mFFS.getVertexShader(id);
      }
      public PixelShader getFFPS(FixedFuncShaders.eFixedFuncShaderIndex id)
      {
         return mFFS.getPixelShader(id);
      }

   }
}