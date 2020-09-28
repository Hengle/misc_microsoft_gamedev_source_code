using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Xceed.DockingWindows;

namespace Terrain
{
   public partial class PropertiesWindow : Xceed.DockingWindows.ToolWindow //: Form
   {
      public PropertiesWindow()
      {
         InitializeComponent();
      }

      private void PropertiesWindow_Load(object sender, EventArgs e)
      {
         Stroke = TerrainGlobals.getTerrainFrontEnd().GetMainBrushStroke();

      }
      Bitmap mPreviewMap = null;
      private void timer1_Tick(object sender, EventArgs e)
      {
         if (mStroke != null && mbChanged) 
         {
            preview();
         }

      }
      private void preview()
      {
         mPreviewMap = mStroke.RenderPreview(pictureBox1.Width, pictureBox1.Height);
         pictureBox1.Image = mPreviewMap;
         pictureBox1.SizeMode = PictureBoxSizeMode.Normal;
         mbChanged = false;
      }
      protected override void OnPaint(PaintEventArgs e)
      {
         base.OnPaint(e);
         //if(mPreviewMap != null)
         //{
         //   e.Graphics.DrawImage(mPreviewMap,0,0);

         //}
      }


      BrushStroke mStroke = new BrushStroke();
      public BrushStroke Stroke
      {
         get
         {
            return mStroke;
         }
         set
         {
            mStroke = value;

            if (mStroke != null)
            {
               SpacingtrackBar.Value = (int)mStroke.mSpacing;
               MinSizetrackBar.Value = (int)mStroke.mMinSize;
               MaxSizetrackBar.Value = (int)mStroke.mMaxSize;
               AngleJittertrackBar.Value = (int)mStroke.mAngleJitter;
               PosJittertrackBar.Value = (int)mStroke.mPositionJitter;
               ScatterCounttrackBar.Value = (int)mStroke.mScatterCount;
               mbChanged = true;
            }
         }
      }
      bool mbChanged = true;

      private void SpacingtrackBar_Scroll(object sender, EventArgs e)
      {
         if(mStroke != null)
            mStroke.mSpacing = SpacingtrackBar.Value / 2f;

         mbChanged = true;
      }

      private void MinSizetrackBar_Scroll(object sender, EventArgs e)
      {
         if (mStroke != null)
         {
            
            mStroke.mMinSize = MinSizetrackBar.Value;

            if(mStroke.mMaxSize < mStroke.mMinSize)
            {
               mStroke.mMaxSize = mStroke.mMinSize;
               MaxSizetrackBar.Value = (int)mStroke.mMaxSize;
            }
         }
         mbChanged = true;

      }

      private void MaxSizetrackBar_Scroll(object sender, EventArgs e)
      {
         if (mStroke != null)
         {
            mStroke.mMaxSize = MaxSizetrackBar.Value;

            if (mStroke.mMaxSize < mStroke.mMinSize)
            {
               mStroke.mMinSize = mStroke.mMaxSize;
               MinSizetrackBar.Value = (int)mStroke.mMinSize;

            }

         }

         mbChanged = true;

      }

      private void AngleJittertrackBar_Scroll(object sender, EventArgs e)
      {
         if (mStroke != null)
            mStroke.mAngleJitter = AngleJittertrackBar.Value;

         mbChanged = true;

      }

      private void PosJittertrackBar_Scroll(object sender, EventArgs e)
      {
         if (mStroke != null)
            mStroke.mPositionJitter = PosJittertrackBar.Value;

         mbChanged = true;

      }

      private void ScatterCounttrackBar_Scroll(object sender, EventArgs e)
      {
         if (mStroke != null)
            mStroke.mScatterCount = ScatterCounttrackBar.Value;

         mbChanged = true;

      }

      private void Defaultbutton_Click(object sender, EventArgs e)
      {
         mStroke.SetDefaults();
         Stroke = mStroke;//update
      }

      private void SpacingtrackBar_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      private void MinSizetrackBar_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

   }
}