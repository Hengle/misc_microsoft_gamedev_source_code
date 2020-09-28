using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

using SimEditor;
using Terrain;
using EditorCore;
using Rendering;

namespace PhoenixEditor
{
   public partial class TexturePaletteViewer : Form
   {
      public TexturePaletteViewer()
      {
         InitializeComponent();
      }

      private void TexturePaletteViewer_Load(object sender, EventArgs e)
      {
         paletteComboBox.Items.Clear();
         List<string> setNames = SimTerrainType.loadTerrainPalettes();
         for (int i = 0; i < setNames.Count; i++)
            paletteComboBox.Items.Add(setNames[i]);
      }

      private void paletteComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         flowLayoutPanel1.Controls.Clear();

         string setName = paletteComboBox.SelectedItem.ToString();
         string fname = CoreGlobals.getWorkPaths().mTerrainTexturesPath + @"\" + setName + SimTerrainType.mTextureSetExtention;
         List<TerrainSetTexture> texSet = SimTerrainType.loadTerrainPalette(fname);
         foreach (TerrainSetTexture obj in texSet)
         {
            TerrainTextureDef def = SimTerrainType.getFromTypeName(obj.mTypeName);
            if (def == null)
            {
               def = new TerrainTextureDef();
               def.ObstructLand = false;
               def.TextureName = EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName;
            }
            flowLayoutPanel1.Controls.Add(new TerrainTypeButton(obj.mTypeName, TextureManager.loadTextureToThumbnail(SimTerrainType.getWorkingTexName(def), 16), this));
         }
      }

      public void SelectButton(TerrainTypeButton but)
      {
         if (mSelectedButton != null)
            mSelectedButton.Invalidate();

         mSelectedButton = but;
         mSelectedButton.Invalidate();
         mSelectedDef = SimTerrainType.getFromTypeName(mSelectedButton.typename);
      }

      public TerrainTextureDef mSelectedDef = null;
      public TerrainTypeButton mSelectedButton = null;

     

      public class TerrainTypeButton : PictureBox
      {
         public TerrainTypeButton(string displayName, Image imageRep,TexturePaletteViewer parent)
         {
            this.Image = imageRep;
            typename = displayName;
            mParent = parent;
         }

         public string typename;

         public TexturePaletteViewer mParent;
         Brush mBr = new SolidBrush(Color.Yellow);

         protected override void OnPaint(PaintEventArgs pe)
         {
            base.OnPaint(pe);

            pe.Graphics.DrawString(Path.GetFileName(typename), Font, mBr, 0, 32);
            if ((mParent.mSelectedButton == this) && (Image != null))
            {
               pe.Graphics.DrawRectangle(new Pen(Color.Red, 2f), 2, 2, this.Width - 4, this.Height - 4);
            }

         }

         protected override void OnClick(EventArgs e)
         {

            mParent.SelectButton(this);

            //base.OnClick(e);

         }
         protected override void OnDoubleClick(EventArgs e)
         {
            mParent.mSelectedDef = SimTerrainType.getFromTypeName(typename);

            mParent.DialogResult = DialogResult.OK;
            mParent.Close();
         }
      }
   }
}