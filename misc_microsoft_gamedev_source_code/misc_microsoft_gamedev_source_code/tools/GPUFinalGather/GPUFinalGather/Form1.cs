using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using Rendering;
using EditorCore;
using FinalGather;

namespace GPUFinalGather
{
   public partial class Form1 : Form
   {

      BCameraManager mCameraManager = new BCameraManager();
      FGScene scene = new FGScene();
      public Form1()
      {
         InitializeComponent();
         this.StartPosition = FormStartPosition.CenterScreen;
         init();
      }

      void init()
      {
         UIManager.InitUIManager(this, d3dPanel);
         UIManager.InitKeyBoardCapture();


         BRenderDevice.createDevice(d3dPanel, d3dPanel.Width, d3dPanel.Height, false);

         scene.init(d3dPanel.Width, d3dPanel.Height);
         mCameraManager.mEye.X = 0;
         mCameraManager.mEye.Y = 25;
         mCameraManager.mEye.Z = 0;

         mCameraManager.mLookAt.X = 22;
         mCameraManager.mLookAt.Y = 0;
         mCameraManager.mLookAt.Z = 22;

         timer1.Enabled = true;
      }

      void destroy()
      {
         BRenderDevice.destroyDevice();
      }

      void input()
      {
         mCameraManager.CameraMovement();
      }

      void update()
      {
         mCameraManager.camUpdate();
      }

      void render()
      {

         BRenderDevice.clear(true, true, unchecked((int)0x00777777), 1.0f, 0);
         BRenderDevice.beginScene();
         scene.render();
         BRenderDevice.endScene();
         BRenderDevice.present();

      }

      private void timer1_Tick(object sender, EventArgs e)
      {
         input();
         update();
         render();
         System.Threading.Thread.Sleep(2);
      }
   }
}