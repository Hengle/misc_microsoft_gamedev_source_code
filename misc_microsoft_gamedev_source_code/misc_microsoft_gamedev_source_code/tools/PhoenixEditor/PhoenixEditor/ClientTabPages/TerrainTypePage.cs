using System;
using System.IO;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using SimEditor;
using EditorCore;
using Rendering;

namespace PhoenixEditor.ClientTabPages
{
   public partial class TerrainTypePage : EditorCore.BaseClientPage
   {
      public TerrainTypePage()
      {
         InitializeComponent();
      }
      override public void destroyPage()
      {
         base.destroyPage();
      }


      public override void init()
      {
         base.init();

         
      }

      int thumnailRes = 64;

      int getColumWithName(string name)
      {
         for (int i = 0; i < gridControl1.Columns.Count; i++)
            if (gridControl1.Columns[i].FieldName == name)
               return i;

         return 0;
      }
      public void AddComboEditor(Xceed.Grid.Collections.ColumnList columns, int index, string[] values)
      {
         ComboBox combo = new ComboBox();
         combo.Items.AddRange(values);
         combo.DropDownStyle = ComboBoxStyle.DropDownList;
         columns[index].CellEditorManager = new Xceed.Grid.Editors.CellEditorManager(combo, "Text", true, true);
      }
      private void initGridControl()
      {
         SimTerrainType.loadTerrainTileTypes();

         gridControl1.DataRows.Clear();
         //setup our control grid
         gridControl1.DataSource = mTypesList;
         gridControl1.SingleClickEdit = true;
       //  AddComboEditor(gridControl1.Columns, getColumWithName("TerrainGroup"), SimEditor.SimTerrainType.mValidGroupings.ToArray());

         int count = SimTerrainType.mTerrainTileTypes.mTypes.Count;
         string[] values = new string[count];
         for (int i = 0; i < count; i++)
         {
            values[i] = SimTerrainType.mTerrainTileTypes.mTypes[i].Name;
         }
         AddComboEditor(gridControl1.Columns, getColumWithName("TileType"), values);

         //load our thumbnail images
         gridControl1.Columns["Preview"].Width = thumnailRes;
         for(int i=0;i<mTypesList.Count;i++)
         {
            string pureName = SimTerrainType.getpureFileNameNoExt(mTypesList[i].TextureName);
            string texName = CoreGlobals.getWorkPaths().mTerrainTexturesPath + @"\" + mTypesList[i].Theme + @"\" + pureName + "_df.tga";
            Xceed.Grid.DataRow row = gridControl1.DataRows[i];
            row.Cells["Preview"].BackgroundImage = TextureManager.loadTextureToThumbnail(texName, thumnailRes);
            row.Height = thumnailRes;
         }
      }


      public override void Activate()
      {
         mTypesList.Clear();
         gridControl1.DataRows.Clear();
         themeSelectBox.Items.Clear();

         SimTerrainType.loadTerrainTypes();

         //populate our texture themes
         string[] texThemes = Directory.GetDirectories(CoreGlobals.getWorkPaths().mTerrainTexturesPath);
         for (int i = 0; i < texThemes.Length; i++)
            themeSelectBox.Items.Add(texThemes[i].Substring(texThemes[i].LastIndexOf("\\") + 1));

         fillSetComboBox();

         
      }

      //CLM [04.26.06] this function called when a tab becomes 'deactive'
      public override void Deactivate()
      {
         //writeTerrainTypesGrid();
      }
      //------------------------------------------------



      
      public void filterGrid(string theme, string group)
      {
         mTypesList.Clear();

         mTypesList = SimTerrainType.getFilteredDefs(theme);
      }
      public void fillTerrainTypesGrid(string texDir)
      {
         SimTerrainType.mergeFilteredDefs(mTypesList);

         mTypesList.Clear();
         
         mTypesList = SimTerrainType.getFilteredDefs(texDir);
         initGridControl();
      }
      public void writeTerrainTypesGrid()
      {
         if (mTypesList.Count == 0) return;
         SimTerrainType.mergeFilteredDefs(mTypesList);
         SimTerrainType.writeTerrainTypes();
      }
      
      

      
      

      public void fillSetComboBox()
      {
         palleteListBox.Items.Clear();
         List<string> setNames = SimTerrainType.loadTerrainPalettes();
         for (int i = 0; i < setNames.Count; i++)
            palleteListBox.Items.Add(setNames[i]);

      }
      public void fillSetList(string setName)
      {
         setList.Items.Clear();

         string fname = CoreGlobals.getWorkPaths().mTerrainTexturesPath + @"\" + setName + SimTerrainType.mTextureSetExtention;

         List<TerrainSetTexture> texSet = SimTerrainType.loadTerrainPalette(fname);
         foreach (TerrainSetTexture obj in texSet)
         {
            setList.Items.Add(obj.mTypeName);
         }
      }
      public void writeSetList(string setName)
      {
         List<string> texSet = new List<string>();
         for (int i = 0; i < setList.Items.Count; i++)
            texSet.Add(setList.Items[i].ToString());
         

         SimTerrainType.writeTerrainPalette(setName,texSet);

         //reload our set listing
         fillSetComboBox();
         palleteListBox.SelectedIndex = -1;
            
      }


      List<TerrainTextureDef> mTypesList = new List<TerrainTextureDef>();
      int mNumMaxSetTextures = 16;


      #region GUI Stuff

      private void TerrainTypePage_Load(object sender, EventArgs e)
      {
         
      }

      private void themeSelectBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         fillTerrainTypesGrid(themeSelectBox.SelectedItem as string);
      }


      private void button2_Click(object sender, EventArgs e)
      {
         writeTerrainTypesGrid();
      }

      private void AddToSet_Click(object sender, EventArgs e)
      {
            for (int i = 0; i < gridControl1.DataRows.Count; i++)
            {
               if (gridControl1.DataRows[i].IsSelected)
               {
                  if (!setList.Items.Contains(mTypesList[i].TypeName) && setList.Items.Count < mNumMaxSetTextures)
                     setList.Items.Add(mTypesList[i].TypeName);
               }
            }
         
      }
      private void RemoveFromSet_Click(object sender, EventArgs e)
      {

         if (setList.SelectedItem != null)
         {
               setList.Items.Remove(setList.SelectedItem);
         }
      }

      private void saveSet_Click(object sender, EventArgs e)
      {
         SaveFileDialog d = new SaveFileDialog();
         d.Filter = "Terrain Texture Set (*" + SimTerrainType.mTextureSetExtention + ")|*" + SimTerrainType.mTextureSetExtention;
         d.InitialDirectory = CoreGlobals.getWorkPaths().mTerrainTexturesPath;
         if (d.ShowDialog() == DialogResult.OK)
         {
            string fname = d.FileName;
            if (!Path.HasExtension(fname))
               fname += SimTerrainType.mTextureSetExtention;
            writeSetList(fname);
            
         }

      }

      private void showSelectedPallete_Click(object sender, EventArgs e)
      {
         if (setList.Items.Count <= 0) return;

         mTypesList.Clear();
         for(int i=0;i<setList.Items.Count;i++)
         {
            string item = setList.Items[i].ToString();
            mTypesList.Add(SimTerrainType.getDefFromDat(item));
         }
         initGridControl();
      }

      private void palleteListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (palleteListBox.SelectedItem == null) return;

         fillSetList(palleteListBox.SelectedItem.ToString());
      }

      private void RemovePallete_Click(object sender, EventArgs e)
      {
         if(MessageBox.Show("Are you sure you'd like to delete this palette?","Delete",MessageBoxButtons.YesNo) == DialogResult.Yes)
         {
            string fname = CoreGlobals.getWorkPaths().mTerrainTexturesPath + @"\" + palleteListBox.SelectedItem.ToString() + SimTerrainType.mTextureSetExtention;

            SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(fname);
            if(status.InPerforce==true)
            {
               if (status.CheckedOut == false)
               {
                  MessageBox.Show("You must check this file out from perforce before you delete it");
                  return;
               }
               else if (status.CheckedOutOtherUser == true)
               {
                  MessageBox.Show("This file is checked out by " + status.ActionOwner + " you cannot delete it.");
                  return;
               }

            }
            
            
            if (File.Exists(fname))
            {
               File.Delete(fname);
               palleteListBox.Items.Remove(palleteListBox.SelectedItem);
            }
         }
      }

      private void clearSet_Click(object sender, EventArgs e)
      {
         setList.Items.Clear();
      }
     
      #endregion

      private void comboBox2_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (comboBox2.SelectedIndex == 0)thumnailRes = 16;
         else if (comboBox2.SelectedIndex == 1) thumnailRes = 32;
         else if (comboBox2.SelectedIndex == 2) thumnailRes = 64;
         else if (comboBox2.SelectedIndex == 3) thumnailRes = 128;
         initGridControl();
      }

      

   }

}
