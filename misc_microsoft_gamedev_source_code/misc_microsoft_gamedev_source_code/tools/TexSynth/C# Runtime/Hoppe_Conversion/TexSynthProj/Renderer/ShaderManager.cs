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

namespace Rendering
{
   class ShaderManager
   {
      static public Microsoft.DirectX.Direct3D.Effect loadShader(string filename, Macro [] defines)
      {
         Microsoft.DirectX.Direct3D.Effect shader = null;
         //Load Our Shader
         string errors = "";
        // if (mTerrainGPUShader == null || mTerrainGPUShader.Disposed == true)
         {
            try
            {
              // if (File.Exists(filename))
               {
                  shader = Microsoft.DirectX.Direct3D.Effect.FromFile(BRenderDevice.getDevice(), filename, defines, null, ShaderFlags.None, null, out errors);
                  if (shader == null)
                  {  
                     MessageBox.Show("Shader did not load:\n " + errors);
                     return null;
                  }
                  
               }
            }
            catch (System.Exception ex)
            {
               MessageBox.Show("Shader did not load");
               throw (ex);
            }
         }
         return shader;
      }
   }
}