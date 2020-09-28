using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;

using graphapp;
using Terrain;
using EditorCore;


namespace PhoenixEditor
{
   public partial class MaskGenFrm : UserControl
   {
      
      MaskGenForm canvasForm = new MaskGenForm();

      public void onUpdateCalculate(ref DAGMask m)
      {
         //convert the mask to a picture box
         if (m == null)
            return;

         Bitmap bmp = new Bitmap(m.Width, m.Height, PixelFormat.Format24bppRgb);
         for (int x = 0; x < m.Width; x++)
         {
            for (int y = 0; y < m.Height; y++)
            {
               float clampK = m[x, y];
               if (clampK < 0) clampK = 0;
               if (clampK > 1) clampK = 1;
               byte bc = (byte)(clampK * byte.MaxValue);
               Color col;
               if (mColorsImage == null)
                  col = Color.FromArgb(255, bc, bc, bc);
               else
                  col = mColorsImage.GetPixel(bc, mColorLineIndex);
               bmp.SetPixel(x, y, col);
            }
         }

         pictureBox1.Image = bmp;

      }

      Bitmap mColorsImage = null;
      int mColorLineIndex = 0;
      void loadFillColorPalette()
      {
         
         bool loadFromResources = true;
         if(!loadFromResources)
         {
            string imageName = "colors.png";
            string textName = "colors.txt";

            mColorsImage = new Bitmap(imageName);
            mColorLineIndex = 0;

            StreamReader re = File.OpenText(textName);
            string input = null;
            while ((input = re.ReadLine()) != null)
            {
               comboBox1.Items.Add(input);
            }
         }
         else
         {
            string imageName = "colorPalette";
            string textName = "colorNames";

            System.Reflection.Assembly a = System.Reflection.Assembly.GetExecutingAssembly();

            // get a list of resource names from the manifest
            string[] resNames = a.GetManifestResourceNames() ;

            System.Resources.ResourceManager resources = new System.Resources.ResourceManager("PhoenixEditor.Properties.resources", a);

            mColorsImage = (Bitmap) resources.GetObject(imageName);

            string str = (String)resources.GetObject(textName);
            string[] stk = str.Split(new char[] { '\n', '\r'});
            
            for(int i=0;i<stk.Length;i++)
            {
               if(stk[i] != "")
                  comboBox1.Items.Add(stk[i]);
            }
            
         }


         comboBox1.SelectedIndex = 0;

         
      }

      public void loadCanvasFromMemoryStream(MemoryStream ms)
      {
         canvasForm.loadCanvasFromMemoryStream(ms);
      }
      public bool saveCanvasToMemoryStream(MemoryStream ms)
      {
         return canvasForm.saveCanvasToMemoryStream(ms);
      }

      public MaskGenFrm()
      {
         InitializeComponent();


         canvasForm.Location = new Point(panel1.Location.X, panel1.Location.Y);
         canvasForm.Size = new Size(panel1.Size.Width, panel1.Size.Height);
         canvasForm.init();
         canvasForm.setUpdateCallback(onUpdateCalculate);
         panel1.Visible = false;
         this.Controls.Add(canvasForm);

         loadFillColorPalette();
      }

      private void toolStripButton1_Click(object sender, EventArgs e)
      {
         
         canvasForm.newCanvas();
      }

      private void toolStripButton2_Click(object sender, EventArgs e)
      {
            canvasForm.loadCanvasFromDisk();
      }

      private void toolStripButton3_Click(object sender, EventArgs e)
      {
         canvasForm.saveCanvasToDisk();
      }

      private void MaskGenDlg_Load(object sender, EventArgs e)
      {

      }

      private void toolStripButton4_Click(object sender, EventArgs e)
      {
         canvasForm.saveCanvasAsToDisk();
      }

      private void toolStripButton5_Click(object sender, EventArgs e)
      {
         int tWidth = TerrainGlobals.getTerrain().getNumXVerts();
         int tHeight = TerrainGlobals.getTerrain().getNumZVerts();
         DAGMask resMask = canvasForm.execute(tWidth, tHeight);
         if (resMask == null)
         {
            MessageBox.Show("There was an error computing output");
            return;
         }

         GraphBasedMask gbm = new GraphBasedMask();

         bool ok = canvasForm.saveCanvasToMemoryStream(gbm.GraphMemStream);
         if(!ok)
         {
            MessageBox.Show("There was an error creating the mask memory stream");
            return;
         }

         CoreGlobals.getEditorMain().mIMaskPickerUI.AddMaskToList(gbm,"GraphMask" + CoreGlobals.getEditorMain().mIMaskPickerUI.GetNumMasks());
         
      }

      private void toolStripButton6_Click(object sender, EventArgs e)
      {
         int tWidth = TerrainGlobals.getTerrain().getNumXVerts();
         int tHeight = TerrainGlobals.getTerrain().getNumZVerts();
         DAGMask resMask = canvasForm.execute(tWidth, tHeight);
         if(resMask==null)
         {
            MessageBox.Show("There was an error computing output");
            return;
         }


         Masking.clearSelectionMask();

         for (int x = 0; x < tWidth; x++)
         {
            for (int y = 0; y < tHeight; y++)
            {
               float k = resMask[x, y];
               Masking.addSelectedVert(x, y, k);

            }
         }
         Masking.rebuildVisualsAfterSelection();
         resMask = null;
      }

      private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (comboBox1.SelectedIndex == -1)
         {
            mColorLineIndex = 0;
            return;
         }

         mColorLineIndex = comboBox1.SelectedIndex;
      }

      private void button1_Click(object sender, EventArgs e)
      {
         int tWidth = TerrainGlobals.getTerrain().getNumXVerts();
         int tHeight = TerrainGlobals.getTerrain().getNumZVerts();
         DAGMask resMask = canvasForm.execute(tWidth, tHeight);
         if (resMask == null)
         {
            MessageBox.Show("There was an error computing output");
            return;
         }

         PictureBox pixBoxHighRes = new PictureBox();
         pixBoxHighRes.Width = tWidth;
         pixBoxHighRes.Height = tHeight;
         Bitmap bmp = new Bitmap(tWidth, tHeight, PixelFormat.Format24bppRgb);
         for (int x = 0; x < tWidth; x++)
         {
            for (int y = 0; y < tHeight; y++)
            {
               float clampK = resMask[x, y];
               if (clampK < 0) clampK = 0;
               if (clampK > 1) clampK = 1;
               byte bc = (byte)(clampK * byte.MaxValue);
               Color col;
               if (mColorsImage == null)
                  col = Color.FromArgb(255, bc, bc, bc);
               else
                  col = mColorsImage.GetPixel(bc, mColorLineIndex);
               bmp.SetPixel(x, y, col);
            }
         }

         pixBoxHighRes.Image = bmp;


         PopupEditor pe = new PopupEditor();
         pe.ShowPopup(this, pixBoxHighRes, FormBorderStyle.FixedToolWindow,false,"Preview : " + tWidth + "x" + tHeight);
      }
   }
}
