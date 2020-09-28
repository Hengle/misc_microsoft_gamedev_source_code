using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using EditorCore;
using Rendering;

namespace ParticleSystem
{
   public partial class TextureControl : UserControl
   {
      private TextureSet data;
      private bool bInitialized = false;
      private string mArtFolder = "art\\";
      public TextureControl()
      {
         InitializeComponent();

         listView1.LargeImageList = new ImageList();
         listView1.LargeImageList.ImageSize = new Size(120, 120);
         listView1.TileSize = new Size(800, 130);
      }

      public void setData(TextureSet set)
      {
         bInitialized = false;
         data = set;
         getModifiedData();
         bInitialized = true;
         refreshListBox(false);
      }

      private void selectItem(int index)
      {
         if (index < 0 || index >= listView1.Items.Count)
            return;

         listView1.Items[index].Selected = true;
         listView1.Select();
      }

      private void setModifiedData()
      {
         if (!bInitialized)
            return;

         if (listView1.SelectedIndices.Count == 0)
            return;

         int selectedIndex = listView1.SelectedIndices[0];
         if (selectedIndex < 0 || selectedIndex >= data.Textures.Count)
            return;

         data.Textures[selectedIndex].weight = (double)this.numericUpDown1.Value;
      }

      private void getModifiedData()
      {
         if (listView1.SelectedIndices.Count == 0)
            return;

         int selectedIndex = listView1.SelectedIndices[0];
         if (selectedIndex < 0 || selectedIndex >= data.Textures.Count)
            return;

         numericUpDown1.Value = (decimal)data.Textures[selectedIndex].weight;
      }

      /*
      private void refreshTextureInfo()
      {
         if (!bInitialized)
            return;

         if (listView1.SelectedIndices.Count == 0)
            return;

         int selectedIndex = listView1.SelectedIndices[0];
         if (selectedIndex < 0 || selectedIndex >= data.Textures.Count)
            return;

         Image img = listView1.LargeImageList.Images[selectedIndex];
         label4.Text = img.Width.ToString();
         label5.Text = img.Height.ToString();
      }
      */
      
      private void refreshListBox(bool reselectLastItem)
      {
         if (!bInitialized)
            return;

         listView1.BeginUpdate();

         int lastItemSelected = -1;
         if (listView1.SelectedItems.Count > 0)
            lastItemSelected = listView1.SelectedIndices[0];
         
         listView1.Clear();
         listView1.LargeImageList.Images.Clear();         
         for (int i = 0; i < data.Textures.Count; ++i)
         {
            String imagePath = CoreGlobals.getWorkPaths().mGameArtDirectory + "\\" + data.Textures[i].file;
            if (File.Exists(imagePath))
            {                                             
               Texture tex = TextureLoader.FromFile(BRenderDevice.getDevice(), imagePath);               
               Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
               listView1.LargeImageList.Images.Add(img);               
               listView1.Items.Add(data.Textures[i].ToString(), i);
            }
            else
            {
               listView1.Items.Add(data.Textures[i].ToString());
            }
         }

         //-- reselect the last item if we can
         if (reselectLastItem)            
            selectItem(lastItemSelected);


         listView1.EndUpdate();
      }

      //-- Add Stage
      private void button1_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         OpenFileDialog d = new OpenFileDialog();
         d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mParticleEffectDirectory;
         d.Filter = "TGA Texture (*.tga)|*.tga";
         if (d.ShowDialog() == DialogResult.OK)
         {
            EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectDirectory = Path.GetDirectoryName(d.FileName);
            
            TextureStage newTexture = new TextureStage();

            String filePath = d.FileName;
            ResourcePathInfo pathInfo = new ResourcePathInfo(d.FileName);
            
            newTexture.file = pathInfo.RelativePath;

            //-- strip out the stupid art directory -
            //-- ResourcePathInfo should cooler and do this for you
            if (mArtFolder.Length > 0)
               newTexture.file = newTexture.file.Substring(mArtFolder.Length);

            newTexture.weight = 1.0f;
            data.Textures.Add(newTexture);

            refreshListBox(false);
            getModifiedData();

            //-- select the new item
            int selectedItem = listView1.Items.Count - 1;
            if (selectedItem < 0)
               selectedItem = 0;
            selectItem(selectedItem);
         }
      }

      //-- Delete
      private void button2_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         //-- if nothing is selected bail out
         if (listView1.SelectedItems.Count == 0)
            return;

         //-- get the selected index
         int selectedIndex = listView1.SelectedIndices[0];
         if (selectedIndex < 0 || selectedIndex >= data.Textures.Count)
            return;

         //-- remove the texture entry
         data.Textures.RemoveAt(selectedIndex);

         refreshListBox(true);
      }

      //-- Apply
      private void button3_Click(object sender, EventArgs e)
      {
         setModifiedData();
         refreshListBox(true);
      }

      private void listView1_SelectedIndexChanged(object sender, EventArgs e)
      {
         getModifiedData();
         //refreshTextureInfo();
      }
   }
}
