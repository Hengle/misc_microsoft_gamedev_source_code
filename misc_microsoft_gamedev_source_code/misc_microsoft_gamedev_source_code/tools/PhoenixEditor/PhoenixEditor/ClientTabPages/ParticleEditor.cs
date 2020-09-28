using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

using EditorCore;
using Rendering;

namespace PhoenixEditor
{
   public partial class ParticleEditor : EditorCore.BaseClientPage
   {

      public ParticleEditor()
      {
         InitializeComponent();
      }

      //CLM [04.26.06] this function called when a tab becomes 'active'
      public override void Activate()
      {
         base.Activate();
         //this call passes in the panel width and height
         MainWindow.mMainWindow.deviceResize(d3dRenderPanel.Width, d3dRenderPanel.Height, false);

         //this call passes in the panel handle in which you want to render to.
         //this is handled during the 'present' call
         BRenderDevice.setWindowTarget(d3dRenderPanel);

      }
      //CLM [04.26.06] this function called when a tab becomes 'deactive'
      public override void Deactivate()
      {

      }

      //CLM [04.26.06] called when this page is closed from the main tab
      public override void destroyPage()
      {
         base.destroyPage();
         deinitDeviceData();
         deinit();
      }

      //these two functions are called when the tab is created, and deinitalized respectivly
      public override void init()
      {
         base.init();
      }

      public override void deinit()
      {
         base.deinit();
         
      }

      //CLM [04.26.06] these functions called for all data that's not in the MANAGED pool for d3d.
      //on a device resize, or reset, these functions are called for you.
      public override void initDeviceData()
      {
         base.initDeviceData();
         
      }
      override public void deinitDeviceData()
      {
         base.deinitDeviceData();
         
      }

     
      //override these functions to ensure your app gets the proper processing.
      override public void input()
      {
         base.input();
        
      }
      override public void update()
      {
         base.update();
        
      }
      override public void render()
      {
         base.render();

         BRenderDevice.clear(true, true, unchecked((int)0x0000FF00), 1.0f, 0);

         BRenderDevice.beginScene();
         BRenderDevice.endScene();
         BRenderDevice.present();
      }




      public void SetTabName(string filename)
      {
         Parent.Text = Path.GetFileNameWithoutExtension(filename);
      }
      private void SaveAsButton_Click(object sender, EventArgs e)
      {
         SaveParticleSystem();
      }
      public bool SaveParticleSystem()
      {
         SaveFileDialog d = new SaveFileDialog();
         if (d.ShowDialog() == DialogResult.OK)
         {
            if (SaveParticleSystem(d.FileName))
            {
               //SetTabName( d.FileName);
               return true;
            }
         }
         return false;
      }
      public bool LoadParticleSystem()
      {
         OpenFileDialog d = new OpenFileDialog();
         if (d.ShowDialog() == DialogResult.OK)
         {
            if (LoadParticleSystem(d.FileName))
            {
               //SetTabName( d.FileName);
               return true;
            }
         }
         return false;
      }

      public bool NewParticleSystem()
      {
         SetTabName("NewParticleSystem");



         return true;
      }
      public bool SaveParticleSystem(string filename)
      {
         SetTabName(filename);



         return true;
      }



      public bool LoadParticleSystem(string filename)
      {
         SetTabName(filename);

         return true;
      }

   }
}
