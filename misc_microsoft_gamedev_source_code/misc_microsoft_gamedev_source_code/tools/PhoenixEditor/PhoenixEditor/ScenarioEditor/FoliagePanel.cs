using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Xceed.DockingWindows;
using System.IO;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;


using SimEditor;
using Terrain;
using EditorCore;
using Rendering;
   
namespace PhoenixEditor.ScenarioEditor
{
   public partial class FoliagePanel : Xceed.DockingWindows.ToolWindow
   {
      //------------------------------------------
      public FoliagePanel()
      {
         InitializeComponent();
      }

      //------------------------------------------
      


      //------------------------------------------
      //Display set list boxes
      void populateListBox()
      {
         string[] foliageSets = Directory.GetFiles(CoreGlobals.getWorkPaths().mFoliagePath, "*.xml.xmb");
         for (int i = 0; i < foliageSets.Length; i++)
         {
            int x = foliageSets[i].IndexOf(".xml.xmb");
            foliageListBox.Items.Add(Path.GetFileNameWithoutExtension(foliageSets[i].Substring(0,x)));
         }
      }
      static public Bitmap Copy(Bitmap srcBitmap, Rectangle section)
      {
         // Create the new bitmap and associated graphics object
         Bitmap bmp = new Bitmap(section.Width, section.Height);
         Graphics g = Graphics.FromImage(bmp);

         // Draw the specified section of the source bitmap to the new one
         g.DrawImage(srcBitmap, 0, 0, section, GraphicsUnit.Pixel);

         // Clean up
         g.Dispose();

         // Return the bitmap
         return bmp;
      }
      List<Image> splitImg(Bitmap mainImage, int numSegments, FoliageSet fs)
      {
         List<Image> images = new List<Image>();
         for (int i = 0; i < fs.mNumBlades; i++)
         {
            Vector2 min = new Vector2(float.MaxValue,float.MaxValue);
            Vector2 max = new Vector2(float.MinValue, float.MinValue);

            //get the UV bounds for this blade
            for(int k=0;k<fs.mNumVertsPerBlade;k++)
            {
               if (fs.mFoliageBlades[i].uvs[k].X < min.X) min.X = fs.mFoliageBlades[i].uvs[k].X;
               if (fs.mFoliageBlades[i].uvs[k].Y < min.Y) min.Y = fs.mFoliageBlades[i].uvs[k].Y;

               if (fs.mFoliageBlades[i].uvs[k].X > max.X) max.X = fs.mFoliageBlades[i].uvs[k].X;
               if (fs.mFoliageBlades[i].uvs[k].Y > max.Y) max.Y = fs.mFoliageBlades[i].uvs[k].Y;
            }

            //convert it to integers
            Point mnI = new Point((int)(min.X * mainImage.Width), (int)(min.Y * mainImage.Height));
            Point mxI = new Point((int)(max.X * mainImage.Width), (int)(max.Y * mainImage.Height));
            int w = mxI.X - mnI.X;
            int h = mxI.Y - mnI.Y;
            Rectangle rect = new Rectangle(mnI.X, mnI.Y, w, h);
            images.Add(Copy(mainImage, rect));

         }
         return images;
      }
      List<Image> splitImgHorizontally(Bitmap mainImage,int numSegments)
      {
         List<Image> images = new List<Image>();
         int numPixelsPerSegment = mainImage.Width / numSegments;
         for(int i=0;i<numSegments;i++)
         {
            Rectangle rect = new Rectangle(i * numPixelsPerSegment, 0, numPixelsPerSegment, mainImage.Height);
            images.Add(Copy(mainImage, rect));
         }
         return images;
      }
      void displayFoliageSet(string foliageSetName)
      {
         string fullFileName = CoreGlobals.getWorkPaths().mFoliagePath + "\\" + foliageSetName;

         FoliageSet fs = null;
         int setIndex =FoliageManager.newSet(fullFileName);
         fs = FoliageManager.giveSet(setIndex);

         //load our image for display
         if (!File.Exists(fullFileName + "_df.ddx"))
         {
            MessageBox.Show(fullFileName + "_df.ddx Not found for this foliage set. Please ensure it exists.");
            return;
         }

         Texture tex = TextureLoader.FromFile(BRenderDevice.getDevice(), fullFileName + "_df.tga");
         Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
         tex.Dispose();
         tex = null;

         Bitmap mainImage = new Bitmap(img); 

         //now, split us into X sub images to display.
         int numsections = fs.mNumBlades;
         List<Image> images = splitImg(mainImage, numsections, fs);// splitImgHorizontally(mainImage, numsections);
         for (int i = 0; i < numsections; i++)
         {
            flowLayoutPanel1.Controls.Add(new FoliageBladeButton(images[i].Width, images[i].Height, images[i], fullFileName, i, this));
         }
      }


      //------------------------------------------
      //button fun..
      FoliageBladeButton mSelectedButton = null;
      void SelectButton(FoliageBladeButton but)
      {
         if (mSelectedButton != null)
            mSelectedButton.Invalidate();

         mSelectedButton = but;

         mSelectedButton.Invalidate();

         if (paintSet.Checked)
         {
            TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeFoliageSet);
         }
         else if(paintErase.Checked)
         {
            TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeFoliageErase);
         }

         int selectedSetIndex = FoliageManager.giveIndexOfSet(mSelectedButton.mFullFileName);
         int selectedBladeIndex = mSelectedButton.mIndexInConatiningSet;

         FoliageManager.SelectedSetIndex = selectedSetIndex;
         FoliageManager.SelectedBladeIndex = selectedBladeIndex;
         
         TerrainGlobals.getEditor().newFoliageBrush();

         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Foliage Paint");    

      }


      //------------------------------------------
      //foliage box
      class FoliageBladeButton : PictureBox
      {
         public string mFullFileName = "";
         public int mIndexInConatiningSet = 0;
         FoliagePanel mParent = null;
         

         public FoliageBladeButton(int width, int height, Image img, string fullFileName, int indexInSet, FoliagePanel parent)
         {
            this.Width = width +12;
            this.Height = height;
            this.Image = img;
            mFullFileName = fullFileName;
            mIndexInConatiningSet = indexInSet;
            mParent = parent;

            this.MouseUp += new MouseEventHandler(FoliageBladeButton_MouseUp);
         }
         ~FoliageBladeButton()
         {

         }

         void FoliageBladeButton_MouseUp(object sender, MouseEventArgs e)
         {
            if (e.Button == MouseButtons.Right)
            {
               mParent.bladeMenuStrip.Show(this, new Point(0, 0));  
            }
         }
         protected override void OnClick(EventArgs e)
         {
            mParent.SelectButton(this);

         }
         protected override void OnDoubleClick(EventArgs e)
         {


         }

         Brush mBr = new SolidBrush(Color.Black);
         protected override void OnPaint(PaintEventArgs pe)
         {
            base.OnPaint(pe);

            mBr = new SolidBrush(Color.Black);

            if ((mParent.mSelectedButton == this) && (Image != null))
            {
               pe.Graphics.DrawRectangle(new Pen(Color.Red, 2f), 0, 0, this.Width, this.Height);
            }

         }
      };


      //------------------------------------------
      //UI functions
      private void foliageListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (foliageListBox.SelectedItem == null)
            return;
         flowLayoutPanel1.Controls.Clear();

        displayFoliageSet(foliageListBox.SelectedItem.ToString());
      }

      private void FoliagePanel_Load(object sender, EventArgs e)
      {
         populateListBox();
      }

      private void removeFromMaToolStripMenuItem_Click(object sender, EventArgs e)
      {
         int setindex = FoliageManager.giveIndexOfSet(CoreGlobals.getWorkPaths().mFoliagePath + "\\" + foliageListBox.SelectedItem.ToString());

         if (setindex == -1 || mSelectedButton ==null)
            return;

         FoliageManager.eraseBladeFromMap(setindex, mSelectedButton.mIndexInConatiningSet);
      }

      private void removeFromMapToolStripMenuItem_Click(object sender, EventArgs e)
      {
         int index = FoliageManager.giveIndexOfSet( CoreGlobals.getWorkPaths().mFoliagePath + "\\" + foliageListBox.SelectedItem.ToString() );

         if (index != -1)
            FoliageManager.eraseSetFromMap(index);
      }

      private void eraseAllButton_Click(object sender, EventArgs e)
      {
         if(MessageBox.Show("This will erase ALL foliage on the map. Are you sure?","Are you sure?", MessageBoxButtons.OKCancel)== DialogResult.OK)
            FoliageManager.eraseAllFoliage();
      }

      private void paintSet_CheckedChanged(object sender, EventArgs e)
      {
         {
            TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeFoliageSet);

            TerrainGlobals.getEditor().newFoliageBrush();

            CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

            CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Foliage Paint");
         }
         
      }

      private void paintErase_CheckedChanged(object sender, EventArgs e)
      {
         
            TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeFoliageErase);

            TerrainGlobals.getEditor().newFoliageBrush();

            CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

            CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Foliage Paint");
         
      }
   }
}
