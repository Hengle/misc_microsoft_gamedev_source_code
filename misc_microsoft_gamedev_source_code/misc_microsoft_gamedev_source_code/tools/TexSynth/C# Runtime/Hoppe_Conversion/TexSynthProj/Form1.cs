using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Rendering;
using EditorCore;
using TextureSynthesis;

namespace TexSynthProj
{
   public partial class Form1 : Form
   {

      TextureSynthesisMain mSynthesisMain = new TextureSynthesisMain();

      bool doreinit = false;
      bool m_bPaint = false;


      public Form1()
      {
         InitializeComponent();
         init();
      }

      protected override void OnPaint(System.Windows.Forms.PaintEventArgs e)
      {
     //    render();
        // d3dPanel.Invalidate();
         //this.Invalidate();
       //  this.Refresh();
      }


      void init()
      {
         UIManager.InitUIManager(this, d3dPanel);
         UIManager.InitKeyBoardCapture();


         BRenderDevice.createDevice(d3dPanel, d3dPanel.Width, d3dPanel.Height, false);

         mSynthesisMain.init(d3dPanel.Width, d3dPanel.Height, 1);

         brushSizeTrackBar.Value = (int) (mSynthesisMain.m_fInnerRadius * 100);

         timer1.Enabled = true;
      }

      void destroy()
      {
         BRenderDevice.destroyDevice();
      }

      void input()
      {
      }
      

      void update()
      {
      }
      void render()
      {
         if (mSynthesisMain.mExemplar == null)
            return;

        

         if(doreinit)
         {
            if (comboBox3.SelectedIndex == 0)
               mSynthesisMain.reinit(d3dPanel.Width, d3dPanel.Height, 1);
            else if (comboBox3.SelectedIndex == 1 || comboBox3.SelectedIndex == 2)
               mSynthesisMain.reinit(d3dPanel.Width >> comboBox3.SelectedIndex, d3dPanel.Height >> comboBox3.SelectedIndex, (int)Math.Pow(2, comboBox3.SelectedIndex));
            doreinit = false;
        }
         mSynthesisMain.refreshBuffer();
         BRenderDevice.clear(true, true, unchecked((int)0x00007700), 1.0f, 0);
         BRenderDevice.beginScene();
         mSynthesisMain.renderBuffer();
         BRenderDevice.endScene();
         BRenderDevice.present();


      }


      private void d3dPanel_Paint(object sender, PaintEventArgs e)
      {
    
      }

      private void exitToolStripMenuItem_Click(object sender, EventArgs e)
      {
         destroy();
         this.Close();
      }

      private void loadExemplarToolStripMenuItem_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();
         d.Filter = "ExemplarImg (*.png)|*.png";
         if (d.ShowDialog() == DialogResult.OK)
         {
        //    resetParams();
            mSynthesisMain.LoadExemplar(d.FileName);
         }
      }
      int worldScrollSize = 512 * 4;
      private void Form1_Load(object sender, EventArgs e)
      {

         hScrollBar1.Maximum = worldScrollSize;// d3dPanel.Width * 2;
         vScrollBar1.Maximum = worldScrollSize;// d3dPanel.Height * 2;
         comboBox4.SelectedIndex = 0;
         //comboBox1.SelectedIndex = 0;
      }


      private void resetParams()
      {
       
         comboBox2.SelectedIndex = 0;
         
         trackBar1.Value = 50;
         trackBar4.Value = 0;
         trackBar5.Value = 0;
         trackBar6.Value = 0;
         trackBar7.Value = 0;
         trackBar8.Value = 0;
         trackBar9.Value = 0;
         trackBar10.Value = 0;
         trackBar11.Value = 0;
         comboBox4.SelectedIndex = 0;
         
         trackBar2.Value = (trackBar2.Maximum - trackBar2.Minimum) >> 1;
      }

      private void comboBox2_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (comboBox2.SelectedIndex == 0)
         {
            mSynthesisMain.mRenderTechnique = TextureSynthesisMain.eRenderTechnique.cColor;
            mSynthesisMain.m_bHighRes = false;
         }
         else if (comboBox2.SelectedIndex == 1) mSynthesisMain.mRenderTechnique = TextureSynthesisMain.eRenderTechnique.cIndicies;
         else if (comboBox2.SelectedIndex == 2) mSynthesisMain.mRenderTechnique = TextureSynthesisMain.eRenderTechnique.cPatches;
         else if (comboBox2.SelectedIndex == 3)
         {
            mSynthesisMain.mRenderTechnique = TextureSynthesisMain.eRenderTechnique.cMagLinear;
            mSynthesisMain.m_bHighRes = mSynthesisMain.mExemplar.getHighResExemplar() != null ? true : false;
         }
         else if (comboBox2.SelectedIndex == 4) mSynthesisMain.mRenderTechnique = TextureSynthesisMain.eRenderTechnique.cPaintLayer;
         else if (comboBox2.SelectedIndex == 5) mSynthesisMain.mRenderTechnique = TextureSynthesisMain.eRenderTechnique.cJMAP;
         else if (comboBox2.SelectedIndex == 6) mSynthesisMain.mRenderTechnique = TextureSynthesisMain.eRenderTechnique.cWorkBuffer;

         mSynthesisMain.m_bNeedRefresh = true;
      }

      

    

      private void trackBar1_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.m_fZoomFactor = 1+(trackBar1.Value / 50.0f);
         mSynthesisMain.m_bNeedRefresh = true;
         
      }

    

      private void trackBar4_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelJitterRandomness[0] = trackBar4.Value;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void trackBar5_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelJitterRandomness[1] = trackBar5.Value;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void trackBar6_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelJitterRandomness[2] = trackBar6.Value;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void trackBar7_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelJitterRandomness[3] = trackBar7.Value;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void trackBar8_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelJitterRandomness[4] = trackBar8.Value;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void trackBar9_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelJitterRandomness[5] = trackBar9.Value;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void trackBar10_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelJitterRandomness[6] = trackBar10.Value;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void trackBar11_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelJitterRandomness[7] = trackBar11.Value;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void comboBox4_SelectedIndexChanged(object sender, EventArgs e)
      {
         mSynthesisMain.setJMapPattern((TextureSynthesisMain.eJMapPattern)(comboBox4.SelectedIndex-1));
         
         brushSizeTrackBar.Enabled = comboBox4.SelectedIndex==1;
         button1.Enabled = comboBox4.SelectedIndex==1;
         checkBox1.Enabled = comboBox4.SelectedIndex == 1;
         checkBox2.Enabled = comboBox4.SelectedIndex == 1;

      }



      private void button1_Click(object sender, EventArgs e)
      {
         mSynthesisMain.clearPaintLayer();
         mSynthesisMain.m_bNeedUpdate = true;
      }



      private void trackBar2_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.TextureScale = trackBar2.Value / 100.0f;
         mSynthesisMain.m_bNeedUpdateJacobianMap = true;
      }

      private void caputureScreensToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Globals.TAKE_SCREEN_CAPTURES = !Globals.TAKE_SCREEN_CAPTURES;
         caputureScreensToolStripMenuItem.Checked = Globals.TAKE_SCREEN_CAPTURES;
      }

      private void hScrollBar1_Scroll(object sender, ScrollEventArgs e)
      {
         float top = hScrollBar1.Value / ((float)d3dPanel.Width);
         float left = vScrollBar1.Value / ((float)d3dPanel.Height);
         BRenderDevice.getDevice().Transform.Projection = Matrix.OrthoOffCenterRH(top, top + 1.0f, left + 1.0f, left, 0.0f, 1.0f);
       //  mSynthesisMain.mSynthControlPacket.WindowMinX = hScrollBar1.Value;
       //  mSynthesisMain.m_bNeedUpdate = true;
      }

      private void vScrollBar1_Scroll(object sender, ScrollEventArgs e)
      {
         float top = hScrollBar1.Value / ((float)d3dPanel.Width);
         float left = vScrollBar1.Value / ((float)d3dPanel.Height);
         BRenderDevice.getDevice().Transform.Projection = Matrix.OrthoOffCenterRH(top, top + 1.0f, left + 1.0f, left, 0.0f, 1.0f);
      //   mSynthesisMain.mSynthControlPacket.WindowMinY = vScrollBar1.Value;
       //  mSynthesisMain.m_bNeedUpdate = true;
      }

      private void button2_Click(object sender, EventArgs e)
      {
         //BRenderDevice.writeTextureToFile(mSynthesisMain.mSynthResultPacket.mResultTex, AppDomain.CurrentDomain.BaseDirectory + "screens\\preview.bmp");
      }

      private void comboBox3_SelectedIndexChanged(object sender, EventArgs e)
      {
         doreinit = true;
      }


      private void d3dPanel_MouseDown(object sender, MouseEventArgs e)
      {
         if (mSynthesisMain.getJMAPPattern() != TextureSynthesisMain.eJMapPattern.cPaint)
            return;

         switch (e.Button)
         {
            case MouseButtons.Right:
               {
                  int mx = e.X;
                  int my = e.Y;

                  mSynthesisMain.m_iAnchorX = mx;
                  mSynthesisMain.m_iAnchorY = my;

                  mSynthesisMain.paintAt(mx, my);

                  m_bPaint = true;

                  // update synthesis
                  mSynthesisMain.m_bNeedUpdateJacobianMap = true;
               }
               break;
         }
      }

      private void d3dPanel_MouseUp(object sender, MouseEventArgs e)
      {
         if (mSynthesisMain.getJMAPPattern() != TextureSynthesisMain.eJMapPattern.cPaint)
            return;

         switch (e.Button)
         {
            case MouseButtons.Right:
               m_bPaint = false;
               break;
         }
      }

      private void d3dPanel_MouseMove(object sender, MouseEventArgs e)
      {
         if (mSynthesisMain.getJMAPPattern() != TextureSynthesisMain.eJMapPattern.cPaint)
            return;

         int mx=e.X;
         int my=e.Y;

         if (m_bPaint)
         {
            // paint
            mSynthesisMain.paintAt(mx, my);
            mSynthesisMain.m_iAnchorX = mx;
            mSynthesisMain.m_iAnchorY = my;
            /*
            if (m_bPaintVectorField) 
            {
               if (  m_VectorFieldPyramid == NULL
                  || (m_VectorFieldPyramid != NULL && m_VectorFieldPyramid->pyramidLevel(0) != m_d3dPaintLayer)) 
               {
                  SAFE_DELETE(m_VectorFieldPyramid);
                  const char *fx_dirs[] ={".","..\\anisotexsyn",""};
                  m_VectorFieldPyramid=new D3DGenMipMapPyramid(m_pd3dDevice,"vfield.pyramid.fx",m_d3dPaintLayer,fx_dirs);
               }
               m_VectorFieldPyramid->bind(m_d3dPaintLayer);
            }
            */

            // update synthesismSynthesisMain.mRenderTechnique = TextureSynthesisMain.eRenderTechnique.cPaintLayer;
            if (mSynthesisMain.mRenderTechnique != TextureSynthesisMain.eRenderTechnique.cPaintLayer)
               mSynthesisMain.m_bNeedUpdateJacobianMap = true;

         }
      }

      private void d3dPanel_MouseLeave(object sender, EventArgs e)
      {
         m_bPaint = false;
      }

      private void brushSizeTrackBar_Scroll(object sender, EventArgs e)
      {
         mSynthesisMain.m_fInnerRadius = (brushSizeTrackBar.Value / 100.0f);
      }


      private void timer1_Tick_1(object sender, EventArgs e)
      {
         render();
         System.Threading.Thread.Sleep(2);
      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mPaintOrientation = checkBox1.Checked;
      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mPaintScale = checkBox2.Checked;
      }

      private void tableLayoutPanel1_Paint(object sender, PaintEventArgs e)
      {

      }

   

      private void checkBox5_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelDoCorrection[1] = checkBox5.Checked;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void checkBox7_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelDoCorrection[2] = checkBox7.Checked;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void checkBox9_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelDoCorrection[3] = checkBox9.Checked;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void checkBox11_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelDoCorrection[4] = checkBox11.Checked;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void checkBox13_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelDoCorrection[5] = checkBox13.Checked;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void checkBox15_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelDoCorrection[6] = checkBox15.Checked;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void checkBox17_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mPerLevelDoCorrection[7] = checkBox17.Checked;
         mSynthesisMain.m_bNeedUpdate = true;
      }


      private void comboBox1_SelectedIndexChanged_1(object sender, EventArgs e)
      {
         if (comboBox1.SelectedIndex == 0) mSynthesisMain.numPaketsToGen = 1;
         if (comboBox1.SelectedIndex == 1) mSynthesisMain.numPaketsToGen = 4;
         if (comboBox1.SelectedIndex == 2) mSynthesisMain.numPaketsToGen = 9;

         mSynthesisMain.createControlPackets();
         mSynthesisMain.postLoadCreateControlPackets();
         mSynthesisMain.m_bNeedUpdateJacobianMap = true;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void radioButton7_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mStartAtLevel = 7;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void radioButton6_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mStartAtLevel = 6;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void radioButton5_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mStartAtLevel = 5;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void radioButton4_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mStartAtLevel = 4;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void radioButton3_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mStartAtLevel = 3;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void radioButton2_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mStartAtLevel = 2;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void radioButton1_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mExemplar.mSynthParams.mStartAtLevel = 1;
         mSynthesisMain.m_bNeedUpdate = true;
      }

      private void radioButton8_CheckedChanged(object sender, EventArgs e)
      {
         mSynthesisMain.mSynthesiser.doForceMode = radioButton8.Checked;
         mSynthesisMain.m_bNeedUpdate = true;
      }
   }
}