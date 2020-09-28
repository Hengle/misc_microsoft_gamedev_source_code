using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Rendering;

namespace PhoenixEditor.ClientTabPages
{
   public partial class OptionsPage : EditorCore.BaseClientPage
   {
      public OptionsPage()
      {
         InitializeComponent();
      }

      override public void destroyPage()
      {
         base.destroyPage();
      }

      private void optionsOK_Click(object sender, EventArgs e)
      {
         if (mDoD3DReload)
            doD3DReload();
      }


      
      bool mDoD3DReload = false;
      int mD3DResolution = 0;    //0 = 640x480, 1 = 800x600, 2 = 1024x728, 3 = snap to window
      private void doD3DReload()
      {
         int width=800;
         int height=600;

         MainWindow.mMainWindow.setD3DSnapToClient(false);
         if (d3dResBox.SelectedIndex == 0)
         {
            width = 640;
            height = 480;
         }
         else if (d3dResBox.SelectedIndex == 1)
         {
            width = 800;
            height = 600;
         }
         else if (d3dResBox.SelectedIndex == 2)
         {
            width = 1024;
            height = 768;
         }
         else if (d3dResBox.SelectedIndex ==3)
         {
            MainWindow.mMainWindow.setD3DSnapToClient(true);
            mDoD3DReload = false;
            return;
         }

         MainWindow.mMainWindow.deviceResize(width, height,true);
         

         mDoD3DReload = false;
      }

      private void d3dResBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         mDoD3DReload = true;
      }

      private void optionsCancel_Click(object sender, EventArgs e)
      {
            destroyPage();
      }

      private void OptionsPage_Load(object sender, EventArgs e)
      {
         StaticKeysLabel.Text = "Rendering Modes : ";
         StaticKeysLabel.Text += "\n   1 - Full Textured";
         StaticKeysLabel.Text += "\n   2 - Wireframe Textured";
         StaticKeysLabel.Text += "\n   3 - Lit Only";
         StaticKeysLabel.Text += "\n   4 - Lit Wireframe";
         StaticKeysLabel.Text += "\n   5 - Wireframe Overlay";
         StaticKeysLabel.Text += "\n   6 - Ambient Occlusion";
         StaticKeysLabel.Text += "\n   7 - TextureEval";
         StaticKeysLabel.Text += "\n   8 - NormalMap view";
         StaticKeysLabel.Text += "\n";

         StaticKeysLabel.Text += "\nTerrain Masking:";
         StaticKeysLabel.Text += "\n   CTRL + LeftMouse - Select Area";
         StaticKeysLabel.Text += "\n   CTRL + RightMouse - Deselect Area";
         StaticKeysLabel.Text += "\n   ~ - Invert selected terrain area";
         StaticKeysLabel.Text += "\n   Esc - Clear mask";
         StaticKeysLabel.Text += "\n";

         StaticKeysLabel.Text += "\nBrushes :";
         StaticKeysLabel.Text += "\n   F1 - Move selected terrain + vertically (Height / setHeight only)";
         StaticKeysLabel.Text += "\n   F2 - Move selected terrain - vertically (Height / setHeight only)";
         StaticKeysLabel.Text += "\n   [ ] - Decrement/Increment brush size fast";
         StaticKeysLabel.Text += "\n   - = - Decrement/Increment brush size slow";
         StaticKeysLabel.Text += "\n";

         StaticKeysLabel.Text += "\nGeneral :";
         StaticKeysLabel.Text += "\n   Space - Reset camera position";
         StaticKeysLabel.Text += "\n   F4 - Compute Ambient Occlusion (Temporary)";

         keyOptionsBox.SelectedIndex = 0;
      }

      private void keyOptionsBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if(keyOptionsBox.SelectedIndex==0)//ARTIST MODE
         {
            transCamLabel.Text = "WASD or ARROWKEYS or NUMPAD ARROWS";
            rotCamLabel.Text = "SHIFT + MouseMove";
            zoomCamLabel.Text = "SHIFT + CTRL + MouseMove;";
         }
         else if(keyOptionsBox.SelectedIndex==1)
         {
            transCamLabel.Text = "Center Mouse + MouseMove or WASD or \nARROWKEYS or NUMPAD ARROWS";
            rotCamLabel.Text = "ALT + Center Mouse + MouseMove";
            zoomCamLabel.Text = "ALT + Mousewheel";
         }
      }

   }

   /*
   //blank overrides
      public override void initDeviceData() { }
      public override void deinitDeviceData() { }

      //these functions will get the active update
      public override void input() { }
      public override void update() { }
      public override void render() { }*/
}
