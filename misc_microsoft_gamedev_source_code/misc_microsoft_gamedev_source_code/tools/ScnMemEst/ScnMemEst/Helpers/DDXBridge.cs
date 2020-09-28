using System;
using System.Drawing;
using System.Collections.Generic;

using System.Runtime.InteropServices;
using System.Text;
using System.IO;
using System.Windows.Forms;


using DDXDLL_CLI;

namespace ScnMemEst
{

   public class DDXBridge
   {
      static bool mInitalized = false;
      static public void init()
      {
         DDXDLL_Interface.init();
         mInitalized = true;
      }
      static public void destroy()
      {
         DDXDLL_Interface.release();
         mInitalized = false;
      }
      static public int give360TextureMemFootprint(string name)
      {
         if (!File.Exists(name))
            return 0;

         if (Path.GetExtension(name) != ".ddx")
            return 0;

         int width = 0;
         int height = 0;
         int format = 0;
         int numMips = 0;
         int fullTextureSize = 0;

         unsafe
         {
            try
            {
               System.Text.ASCIIEncoding encoding = new System.Text.ASCIIEncoding();
               byte[] dat = encoding.GetBytes(name);

               if (!mInitalized)
                  init();

               fixed (byte* v = dat)
                  DDXDLL_Interface.loadDDXTextureInfo((sbyte*)v, &width, &height, &format, &numMips, &fullTextureSize);
            }
            catch (System.IO.FileLoadException e)
            {
               return -1;
            }
         }

         return fullTextureSize;

      }
   };
}