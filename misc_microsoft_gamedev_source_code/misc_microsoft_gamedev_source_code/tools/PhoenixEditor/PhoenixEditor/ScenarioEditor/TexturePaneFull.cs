using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Xceed.DockingWindows;
using System.IO;
using SimEditor;
using Terrain;
using EditorCore;
using Rendering;
   
namespace PhoenixEditor.ScenarioEditor
{
   public partial class TexturePaneFull : Xceed.DockingWindows.ToolWindow
   {
      public int mThumbnailSize = 16;
      public int mNumThemes = 0;
      private TerrainTextureDef mSelectedDef = null;
      public TerrainTextureButton mSelectedButton = null;
      public bool mShowInfo = true;
      public Color mFontColor = Color.Black;
      public TerrainTextureDef SelectedDef
      {
         get
         {
            return mSelectedDef;
         }
         set
         {
            mSelectedDef = value;

            if (mSelectedDef != null)
            {
               SimTerrainType.addActiveSetDef(mSelectedDef);
               TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex = SimTerrainType.getActiveSetIndex(mSelectedDef);

               //since we're selecting from the list, negate all instances which may have been selected in 3d
               TerrainGlobals.getTexturing().unselectAllDecalInstances();

               if (TerrainGlobals.getEditor().getRenderMode() == BTerrainEditor.eEditorRenderMode.cRenderTextureSelectRender)
                  TerrainGlobals.getTexturing().reloadCachedVisuals();
            }
         }
      }
      public bool mSelectSwap = false;
      public bool mSelectReplace = false;

      public int firstSplatIndex()
      {
         return 1;
      }
      public int firstDecalIndex()
      {
         for (int i = firstSplatIndex(); i < flowLayoutPanel1.Controls.Count; i++)
            if (flowLayoutPanel1.Controls[i] is TerrainTitleButton)
               return i+1;
         return flowLayoutPanel1.Controls.Count - 1;
      }


      public void postTEDLoad(bool selectdIndexChanged)
      {
         try
         {

            if (selectdIndexChanged)
            {

               if (comboBox1.SelectedIndex != 0)
               {
                  comboBox1.SelectedIndex = 0;
                  redrawPreviewList(0);
               }


               if (TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeTexEdit)
               {
                  SelectButton((TerrainTextureButton)flowLayoutPanel1.Controls[firstSplatIndex() + TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex]);
               }
               else if (TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeDecalEdit)
               {
                  SelectButton((TerrainTextureButton)flowLayoutPanel1.Controls[firstDecalIndex() + TerrainGlobals.getTerrainFrontEnd().SelectedDecalIndex]);
               }
            }
            else
            {
               if (redrawList != null)
               {
                  this.Invoke(redrawList);
               }
            }
         }
         catch(System.Exception ex)
         {

            if(mbPostTEDLoadRecovery)
            {
               CoreGlobals.getErrorManager().LogErrorToNetwork(ex.ToString());

               try
               {
                  if (TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeTexEdit)
                  {
                     CoreGlobals.getErrorManager().LogErrorToNetwork(TerrainGlobals.getEditor().getMode().ToString()
                        + "  firstSplatIndex=" + firstSplatIndex().ToString()
                        + "  flowLayoutPanel1.Controls=" + flowLayoutPanel1.Controls.ToString()
                        + "  SelectedTextureIndex=" + TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex);
                  }
               }
               catch(System.Exception ex2)
               {
                  CoreGlobals.getErrorManager().OnException(ex2);
               }

               if(MessageBox.Show("Error building texture picker (error saved to network)","Error", MessageBoxButtons.RetryCancel) == DialogResult.Retry)
               {
                  postTEDLoad(selectdIndexChanged);
               }
               else
               {
                  mbPostTEDLoadRecovery = false;
               }

            }


         }

      }
      bool mbPostTEDLoadRecovery = true;

      public void postTEDLoadInner()
      {
         redrawPreviewList(0);
      }

      public delegate void _redrawList();
      public _redrawList redrawList = null;

      
      public void redrawPreviewList(int paletteIndex)
      {
         //try
         //{

            flowLayoutPanel1.Controls.Clear();
            flowLayoutPanel1.Controls.Add(new TerrainTitleButton("SplatTextures", this));
            string setName = "";
            if (paletteIndex == 0)
            {
               List<TerrainTextureDef> activeTexDefs = SimTerrainType.getDefsOfActiveSet();

               if (activeTexDefs.Count == 0)
               {
                  MessageBox.Show("An error has occured with the active texture defnitions. Please send this scenario to the editor people.");
                  return;
               }

               for (int i = 0; i < activeTexDefs.Count; i++)
               {
                  flowLayoutPanel1.Controls.Add(new TerrainTextureSplatButton(activeTexDefs[i].TypeName,
                                 SimTerrainType.getWorkingTexName(activeTexDefs[i]),
                                 TextureManager.loadTextureToThumbnail(SimTerrainType.getWorkingTexName(activeTexDefs[i]), mThumbnailSize),
                                 this));
               }
               //visually identify our base texture
               flowLayoutPanel1.Controls[firstSplatIndex()].BackColor = Color.FromArgb(96, 0, 0, 0);

            }
            else if (paletteIndex <= mNumThemes)
            {
               setName = comboBox1.Items[paletteIndex].ToString();
               setName = setName.Remove(setName.LastIndexOf("_FULL"));

               List<TerrainTextureDef> tList = SimTerrainType.getFilteredDefs(setName);
               setName = comboBox1.Items[paletteIndex].ToString();
               for (int i = 0; i < tList.Count; i++)
               {
                  flowLayoutPanel1.Controls.Add(new TerrainTextureSplatButton(tList[i].TypeName,
                                 SimTerrainType.getWorkingTexName(tList[i]),
                                 TextureManager.loadTextureToThumbnail(SimTerrainType.getWorkingTexName(tList[i]), mThumbnailSize),
                                 this));
               }
            }
            else
            {
               setName = comboBox1.Items[paletteIndex].ToString();

               string fname = CoreGlobals.getWorkPaths().mTerrainTexturesPath + @"\" + setName + SimTerrainType.mTextureSetExtention;
               List<TerrainSetTexture> texSet = SimTerrainType.loadTerrainPalette(fname);

               for (int i = 0; i < texSet.Count; i++)
               {
                  TerrainSetTexture obj = texSet[i];
                  {
                     TerrainTextureDef def = SimTerrainType.getFromTypeName(obj.mTypeName);
                     if (def == null)
                     {
                        def = new TerrainTextureDef();
                        def.ObstructLand = false;
                        def.TextureName = EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName;
                     }
                     flowLayoutPanel1.Controls.Add(new TerrainTextureSplatButton(obj.mTypeName,
                        SimTerrainType.getWorkingTexName(def),
                        TextureManager.loadTextureToThumbnail(SimTerrainType.getWorkingTexName(def), mThumbnailSize),
                        this));
                  }
               }
            }

            flowLayoutPanel1.Controls.Add(new TerrainTitleButton("DecalTextures", this));
            int decalIndex = firstDecalIndex();
            //now add our decals
            if (paletteIndex == 0)
            {
               for (int i = 0; i < TerrainGlobals.getTexturing().getActiveDecalCount(); i++)
               {
                  string displName = Path.GetFileNameWithoutExtension(TerrainGlobals.getTexturing().getActiveDecal(i).mFilename);
                  displName = displName.Remove(displName.LastIndexOf("_"));

                  flowLayoutPanel1.Controls.Add(new TerrainTextureDecalButton(displName,
                                                                              TerrainGlobals.getTexturing().getActiveDecal(i).mFilename,
                                                                              TextureManager.loadTextureToThumbnail(TerrainGlobals.getTexturing().getActiveDecal(i).mFilename, mThumbnailSize),
                                                                              this));
               }
            }
            else if (paletteIndex <= mNumThemes)
            {
               setName = comboBox1.Items[paletteIndex].ToString();
               setName = setName.Remove(setName.LastIndexOf("_FULL"));

               string[] textureNames = Directory.GetFiles(CoreGlobals.getWorkPaths().mTerrainTexturesPath + "\\" + setName, "*_dcl_*.ddx", SearchOption.AllDirectories);

               for (int i = 0; i < textureNames.Length; i++)
               {
                  if (!textureNames[i].Contains("_df"))
                     continue;

                  String ext = Path.GetExtension(textureNames[i]);
                  if (!File.Exists(textureNames[i].Substring(0, textureNames[i].LastIndexOf("_df")) + "_op" + ext))
                     continue;


                  string displName = Path.GetFileNameWithoutExtension(textureNames[i]);
                  displName = displName.Remove(displName.LastIndexOf("_"));
                  flowLayoutPanel1.Controls.Add(new TerrainTextureDecalButton(displName,
                                                Path.ChangeExtension(textureNames[i], ".tga"),
                                                TextureManager.loadTextureToThumbnail(Path.ChangeExtension(textureNames[i], ".tga"), mThumbnailSize),
                                                this));
               }
            }
            else
            {
               //not supported yet...
            }

         //}
         //catch (System.Exception ex)
         //{
         //   if (mbPostTEDLoadRecovery)
         //   {
         //      CoreGlobals.getErrorManager().OnException(ex);
         //   }

         //}
      }
      public bool isActiveSetSelected()
      {
         return comboBox1.SelectedIndex == 0 ;
      }
      public void selectActiveSet()
      {
         comboBox1.SelectedIndex = 0;
      }
      public void addSelectTexture(TerrainTextureButton but)
      {
         if (mSelectedButton != null)
            mSelectedButton.Invalidate();

         mSelectedButton = but;
         mSelectedButton.Invalidate();


         if(but is TerrainTextureSplatButton)
         {
            SelectedDef = SimTerrainType.getFromTypeName(but.typename);

            if (!isActiveSetSelected())
            {
               TerrainGlobals.getTexturing().addActiveTexture(SimTerrainType.getWorkingTexName(mSelectedDef));
            }
         }
         else if (but is TerrainTextureDecalButton)
         {
            if (!isActiveSetSelected())
            {
               TerrainGlobals.getTexturing().addActiveDecal(((TerrainTextureDecalButton)but).mFullFilename);
            }
            TerrainGlobals.getTerrainFrontEnd().SelectedDecalIndex = TerrainGlobals.getTexturing().getActiveDecalIndex(((TerrainTextureDecalButton)but).mFullFilename);
            if (TerrainGlobals.getEditor().getRenderMode() == BTerrainEditor.eEditorRenderMode.cRenderTextureSelectRender)
               TerrainGlobals.getTexturing().reloadCachedVisuals();
         }
         

         
      }
      public void swapSelectTexture(TerrainTextureButton but)
      {
         if (mSelectedButton != but)
         {
            TerrainTextureDef newDef = null;
            TerrainTextureDef origDef = mSelectedDef;
            bool activeSel = isActiveSetSelected();
            if (activeSel)
            {
               newDef = SimTerrainType.getFromTypeName(but.typename);
            }
            else
            {
                addSelectTexture(but);
               newDef = mSelectedDef;
            }

            //replace across the board
            if (mSelectSwap)
            {
               TerrainGlobals.getTexturing().swapActiveTextures(SimTerrainType.getActiveSetIndex(origDef), SimTerrainType.getActiveSetIndex(newDef));
               SimTerrainType.swapActiveSetDef(SimTerrainType.getActiveSetIndex(origDef), SimTerrainType.getActiveSetIndex(newDef));
            }
            else if (mSelectReplace)
            {
               if (SimTerrainType.getActiveSetIndex(newDef) == 0)
               {
                  MessageBox.Show("You can not replace with the base texture");
               }
               else
               {
                     TerrainGlobals.getTexturing().replaceActiveTexture(SimTerrainType.getActiveSetIndex(origDef), SimTerrainType.getActiveSetIndex(newDef));
                     SimTerrainType.removeActiveSetDef(origDef);
               }
            }
            
            if (!activeSel)
            {
               //now select our new texture
               addSelectTexture(but);
            }
         }
         mSelectSwap = false;
         mSelectReplace = false;
         alertLabel.Text = "";

         TerrainGlobals.getTexturing().freeAllCaches();
         selectActiveSet();
         redrawPreviewList(0);
         SelectButton(but);
         
         //TerrainGlobals.getTexturing().reloadActiveTextures(true);
      }
      
      public void removeSelectedTexture()
      {
         

         if (mSelectedButton is TerrainTextureSplatButton)
         {
            if (TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex == 0)
               return;

            if (SimTerrainType.mActiveWorkingSet.Count - 1 <= 0)
               return;

            TerrainGlobals.getTexturing().removeActiveTexture(SimTerrainType.getActiveSetIndex(mSelectedDef));

            SimTerrainType.removeActiveSetDef(mSelectedDef);

            flowLayoutPanel1.Controls.Remove(mSelectedButton);
            SelectedDef = SimTerrainType.getDefsOfActiveSet()[0];
         }
         else
         {
         //   if (TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex == 0)
         //      return;

            int index = TerrainGlobals.getTexturing().getActiveDecalIndex(((TerrainTextureDecalButton)mSelectedButton).mFullFilename);
            TerrainGlobals.getTexturing().removeActiveDecal(index);


            flowLayoutPanel1.Controls.Remove(mSelectedButton);
            SelectedDef = SimTerrainType.getDefsOfActiveSet()[0];
         }
         redrawPreviewList(0);
         mSelectedButton = (TerrainTextureButton)flowLayoutPanel1.Controls[firstSplatIndex()];
      }

      public void SelectButton(TerrainTextureButton but)
      {
         if(but is TerrainTextureSplatButton)
         {
            if (mSelectSwap || mSelectReplace)
            {
               if(mSelectedButton is TerrainTextureSplatButton)
                  swapSelectTexture(but);
            }
            else
            {
               addSelectTexture(but);
               //show our properties window
               if (isActiveSetSelected())
               {
                  texturePropertiesPanel.Enabled = true;
                  setTextureProperties();
                  panel1.Refresh();
               }
               else
               {
                  texturePropertiesPanel.Enabled = false;
                  panel1.Refresh();

               }
               TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeTexEdit);
            }
         }
         else if (but is TerrainTextureDecalButton)
         {
            if (mSelectSwap || mSelectReplace)
            {
               return;
            }
            else
            {
               addSelectTexture(but);
               //TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeDecalEdit);
            }
         }
         
      }
      public void setTextureProperties()
      {
         BTerrainActiveTextureContainer act = TerrainGlobals.getTexturing().getActiveTexture(TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex);
         if (act == null)
         {
            CoreGlobals.ShowMessage("Error getting properties from active texture. index: = " + TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex.ToString());
            TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex = 0;
            redrawPreviewList(0);
            return;
         }
         uScalebar.Value = act.mUScale;
         vScalebar.Value = act.mVScale;
         specExponentVal.Value = (Decimal)act.mSpecExponent;
         fresnelBiasVal.Value = (Decimal)act.mFresnelBias;
         fresnelPowerVal.Value = (Decimal)act.mFresnelPower;
         fresnelScaleVal.Value = (Decimal)act.mFresnelScale;
         fresnelRefractVal.Value = (Decimal)act.mFresnelRefractPercent;
      }
      public void popertiesToActiveTextures()
      {
         BTerrainActiveTextureContainer act = TerrainGlobals.getTexturing().getActiveTexture(TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex);
         if (act == null)
         {
            CoreGlobals.ShowMessage("Error setting properties to active texture. index: = " + TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex.ToString());
            TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex = 0;
            redrawPreviewList(0);
            return;
         }
         act.mUScale = uScalebar.Value;
         act.mVScale = vScalebar.Value;
         act.mSpecExponent = (float)specExponentVal.Value;
         act.mFresnelBias = (float)fresnelBiasVal.Value;
         act.mFresnelPower = (float)fresnelPowerVal.Value;
         act.mFresnelScale = (float)fresnelScaleVal.Value;
         act.mFresnelRefractPercent = (float)fresnelRefractVal.Value;


     //    TerrainGlobals.getTexturing().writeTexturesToXML();
      }

      //-------------------------------------------------
      //-------------------------------------------------
      public class TerrainTitleButton : PictureBox
      {
         public TerrainTitleButton(string titleName, TexturePaneFull parent)
         {
            mTitle = titleName;
            this.Width = parent.flowLayoutPanel1.Width - 10;// mThumbnailSize;
            this.Height = 16;
            mWidth = parent.flowLayoutPanel1.Width - 10;
         }
         public string mTitle;
         private int mWidth;

         public TexturePaneFull mParent;
         Brush mBr = new SolidBrush(Color.Black);

         protected override void OnPaint(PaintEventArgs pe)
         {
            base.OnPaint(pe);

            mBr = new SolidBrush(Color.Black);
            pe.Graphics.DrawString(mTitle, Font, mBr, 0, 0);
            pe.Graphics.DrawRectangle(new Pen(Color.Black, 1f), 0, 12, mWidth, 1);
         }

      }
      //-------------------------------------------------
      public class TerrainTextureButton : PictureBox
      {
         public TerrainTextureButton()
         {
         }
         public TerrainTextureButton(string displayName, string fullfilename, Image imageRep, TexturePaneFull parent)
         {
            this.Width = parent.flowLayoutPanel1.Width - 10;// mThumbnailSize;
            this.Height = parent.mThumbnailSize;
            this.Image = imageRep;
            typename = displayName;
            mParent = parent;
            mFullFilename = fullfilename;
            this.MouseUp += new MouseEventHandler(TerrainTextureButton_MouseUp);
         }

         void TerrainTextureButton_MouseUp(object sender, MouseEventArgs e)
         {
            if (e.Button == MouseButtons.Right)
            {
               if (mParent.isActiveSetSelected())
               {
                  mParent.contextMenuStrip1.Show(this, new Point(0, 0));
               }
            }
         }

         public string mFullFilename=null;
         public string typename;
         public TexturePaneFull mParent;
         protected Brush mBr = new SolidBrush(Color.Black);

         protected void paintInfo(PaintEventArgs pe)
         {
            if (mParent.mShowInfo)
            {
               int infoXStart = mParent.mThumbnailSize + 2;
               mBr = new SolidBrush(mParent.mFontColor);
               pe.Graphics.DrawString(Path.GetFileName(typename), Font, mBr, infoXStart, 0);

               //draw our material info
               int materialHeight = 12;
               String ext = ".ddx";// Path.GetExtension(mFullFilename);
               bool mMatHasEmissive = File.Exists(mFullFilename.Substring(0, mFullFilename.LastIndexOf("_df")) + "_em" + ext);
               bool mMatHasEnvMask = File.Exists(mFullFilename.Substring(0, mFullFilename.LastIndexOf("_df")) + "_rm" + ext);
               bool mMatHasSpecular = File.Exists(mFullFilename.Substring(0, mFullFilename.LastIndexOf("_df")) + "_sp" + ext);


               int w = infoXStart;
               int wInc = 17;
               if (mMatHasSpecular)
               {
                  pe.Graphics.DrawString("Sp", Font, mBr, w, materialHeight);
                  w += wInc;
               }
               if (mMatHasEmissive)
               {
                  pe.Graphics.DrawString("Em", Font, mBr, w, materialHeight);
                  w += wInc;
               }
               if (mMatHasEnvMask)
               {
                  pe.Graphics.DrawString("Rm", Font, mBr, w, materialHeight);
                  w += wInc;
               }

            }
            
         }
         protected override void OnPaint(PaintEventArgs pe)
         {
            base.OnPaint(pe);

            paintInfo(pe);


            if ((mParent.mSelectedButton == this) && (Image != null))
            {
               pe.Graphics.DrawRectangle(new Pen(Color.Red, 2f), 0, 0, mParent.mShowInfo ? this.Width : mParent.mThumbnailSize, mParent.mThumbnailSize);
            }

         }

         protected override void OnClick(EventArgs e)
         {
            mParent.SelectButton(this);

         }
         protected override void OnDoubleClick(EventArgs e)
         {


         }
      }
      public class TerrainTextureSplatButton : TerrainTextureButton
      {
         public TerrainTextureSplatButton(string displayName, string fullfilename, Image imageRep, TexturePaneFull parent)
         {
            
            this.Width = parent.flowLayoutPanel1.Width-10;// mThumbnailSize;
            this.Height = parent.mThumbnailSize;
            this.Image = imageRep;
            typename = displayName;
            mParent = parent;
            mFullFilename = fullfilename;
            this.MouseUp += new MouseEventHandler(TerrainTextureSplatButton_MouseUp);
         }

         void TerrainTextureSplatButton_MouseUp(object sender, MouseEventArgs e)
         {
            if (e.Button == MouseButtons.Right)
            {
               if (mParent.isActiveSetSelected())
               {
                  mParent.contextMenuStrip1.Items[0].Enabled = true;
                  mParent.contextMenuStrip1.Items[1].Enabled = true;
                  mParent.contextMenuStrip1.Show(this, new Point(0, 0));
               }
            }
         }

        
         protected override void OnPaint(PaintEventArgs pe)
         {
            base.OnPaint(pe);

            paintInfo(pe);
            

            if ((mParent.mSelectedButton == this) && (Image != null))
            {
               pe.Graphics.DrawRectangle(new Pen(Color.Red, 2f), 0, 0, mParent.mShowInfo ? this.Width : mParent.mThumbnailSize, mParent.mThumbnailSize);
            }

         }

         protected override void OnClick(EventArgs e)
         {
            mParent.SelectButton(this);

         }
         protected override void OnDoubleClick(EventArgs e)
         {
            
           
         }
      }

      public class TerrainTextureDecalButton : TerrainTextureButton
      {
         public TerrainTextureDecalButton(string displayName, string fullfilename,Image imageRep, TexturePaneFull parent)
         {
            this.Width = parent.flowLayoutPanel1.Width - 10;// mThumbnailSize;
            this.Height = parent.mThumbnailSize;
            this.Image = imageRep;
            typename = displayName;
            mParent = parent;
            this.MouseUp += new MouseEventHandler(TerrainTextureDecalButton_MouseUp);
            mFullFilename = fullfilename;
         }
         void TerrainTextureDecalButton_MouseUp(object sender, MouseEventArgs e)
         {
            
            if (e.Button == MouseButtons.Right)
            {
               if (mParent.isActiveSetSelected())
               {
                  mParent.contextMenuStrip1.Items[0].Enabled = false;
                  mParent.contextMenuStrip1.Items[1].Enabled = false;
                  mParent.contextMenuStrip1.Show(this, new Point(0, 0));
               }
            }
         }
         protected override void OnPaint(PaintEventArgs pe)
         {
            base.OnPaint(pe);

            paintInfo(pe);


            if ((mParent.mSelectedButton == this) && (Image != null))
            {
               pe.Graphics.DrawRectangle(new Pen(Color.Red, 2f), 0, 0, mParent.mShowInfo ? this.Width : mParent.mThumbnailSize, mParent.mThumbnailSize);
            }

         }
         protected override void OnClick(EventArgs e)
         {
            mParent.SelectButton(this);

         }
         protected override void OnDoubleClick(EventArgs e)
         {


         }
      }

      //-------------------------------------------------
      //-------------------------------------------------
      //-------------------------------------------------
      #region GUI STUFF
      public TexturePaneFull()
      {
         InitializeComponent();

         redrawList = new _redrawList(postTEDLoadInner);

         alertLabel.Text = "";
         PlaceHolderButton.FlatStyle = FlatStyle.Flat;
         texturePropertiesPanel.Enabled = false;
         
      }
      private void TexturingPanelFull_Load(object sender, EventArgs e)
      {
         comboBox1.Items.Clear();
         comboBox1.Items.Add("Active Set");

         //populate our texture themes
         string[] texThemes = Directory.GetDirectories(CoreGlobals.getWorkPaths().mTerrainTexturesPath);
         for (int i = 0; i < texThemes.Length; i++)
            comboBox1.Items.Add(texThemes[i].Substring(texThemes[i].LastIndexOf("\\") + 1) + "_FULL");

         mNumThemes = texThemes.Length;

         List<string> setNames = SimTerrainType.loadTerrainPalettes();
         for (int i = 0; i < setNames.Count; i++)
            comboBox1.Items.Add(setNames[i]);

         comboBox1.SelectedIndex = 0;

         comboBox2.SelectedIndex = 1;


         InitToolBars();
      }
      private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         redrawPreviewList(comboBox1.SelectedIndex);
      }
      private void button6_Click(object sender, EventArgs e)
      {
         //mBGColor.GetEditor().Show();
      }
      private void button7_Click(object sender, EventArgs e)
      {

      }
      private void tabPage1_Click(object sender, EventArgs e)
      {

      }
      private void colorPickerLauncher1_SelectedColorChanged(object sender, EventArgs e)
      {
         flowLayoutPanel1.BackColor = colorPickerLauncher1.SelectedColor;
      }
      private void colorPickerLauncher2_SelectedColorChanged(object sender, EventArgs e)
      {
         mFontColor = colorPickerLauncher2.SelectedColor;
         redrawPreviewList(comboBox1.SelectedIndex);
      }
      private void deleteFromActiveSetToolStripMenuItem_Click(object sender, EventArgs e)
      {
         removeSelectedTexture();
      }
      private void swapWithTextureToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mSelectSwap = true;
         mSelectReplace = false;
         alertLabel.Text = "Pick a texture to swap with\n If the texture is in the active set, it will be replaced.";
      }
      private void replaceWithTextureToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mSelectReplace = true;
         mSelectSwap = false;
         alertLabel.Text = "Pick a texture to replace with\n If the texture is in the active set, it will be replaced.";
      }
      
      private void button6_Click_2(object sender, EventArgs e)
      {
         swapModes();
      }
      private void button6_Click_3(object sender, EventArgs e)
      {
         MainWindow.mMainWindow.ShowDialog("BrushSettings");
      }
      private void InitToolBars()
      {

         mMaskPalette.mButtonSize = 32;
         mMaskPalette.mbRestrictTextureSize = false;
         if (CoreGlobals.getSettingsFile().ThreadedStart == true)
         {
            mMaskPalette.threadedInit(TerrainGlobals.getTerrainFrontEnd().getMaskTextures());
         }
         else
         {
            mMaskPalette.init(TerrainGlobals.getTerrainFrontEnd().getMaskTextures());
         }
         mMaskPalette.mTextureSelected += new TexturePalette.TextureSelected(mMaskPalette_mTextureSelected);
         mMaskPalette.OverflowButton.Size = new Size(100, 100);
         mMaskPalette.OverflowButton.AutoSize = false;
      }
      void mMaskPalette_mTextureSelected(int id)
      {
         try
         {
            TerrainGlobals.getTerrainFrontEnd().SelectedMaskIndex = id;

            //Todo cleaner way to access this window???
            if (MainWindow.mMainWindow.mBrushSettingsWindow != null)
            {
               MainWindow.mMainWindow.mBrushSettingsWindow.Stroke = TerrainGlobals.getEditor().getCurrentStroke();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }
      private void button7_Click_1(object sender, EventArgs e)
      {
      //   return;

         //CLM
         //TerrainGlobals.getTexturing().flattenLayers();
         TerrainGlobals.getTexturing().removeUnusedActiveTextures();
         TerrainGlobals.getTexturing().removeUnusedActiveDecals();
         TerrainGlobals.getTexturing().ensureCleanLayers();
         TerrainGlobals.getTexturing().reloadCachedVisuals(); 
         
         redrawPreviewList(0);
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeNone);
      }
      private void comboBox2_SelectedIndexChanged(object sender, EventArgs e)
      {
         mThumbnailSize = 16 << comboBox2.SelectedIndex;
         redrawPreviewList(comboBox1.SelectedIndex);
      }
      private void flowLayoutPanel1_Paint(object sender, PaintEventArgs e)
      {

      }
      private void button1_Click_1(object sender, EventArgs e)
      {
         TerrainGlobals.getTexturing().reloadCachedVisuals();
         popertiesToActiveTextures();
      }
      private void button5_Click(object sender, EventArgs e)
      {
         mShowInfo = !mShowInfo;
         redrawPreviewList(comboBox1.SelectedIndex);
      }
      #endregion


      bool mFullMode = true;
      public void toggleGUIModes(bool visible)
      {
         //hide gui elements
         label1.Visible = visible;
         label2.Visible = visible;
         label3.Visible = visible;
         comboBox1.Visible = visible;
         alertLabel.Visible = visible;
         colorPickerLauncher1.Visible = visible;
         colorPickerLauncher2.Visible = visible;
         
         flowLayoutPanel1.Left= Visible?18:10;
         flowLayoutPanel1.Top = Visible?91:10;
         flowLayoutPanel1.FlowDirection = Visible ? FlowDirection.TopDown : FlowDirection.LeftToRight;

         mThumbnailSize = 64;
         mShowInfo = Visible;
      }
      public void SwapToFullMode()
      {
         mFullMode = true;
         //phoenixeditor.dockToGrouping(this);
         toggleGUIModes(true);
      }
      public void SwapToMinMode()
      {
         mFullMode = false;
         this.DockTo(DockTargetHost.DockHost, DockPosition.Bottom);
         mShowInfo = false;
         toggleGUIModes(false);
      }
      public void swapModes()
      {
         if (mFullMode)
            SwapToMinMode();
         else
            SwapToFullMode();
      }

      private void comboBox1_KeyDown(object sender, KeyEventArgs e)
      {
         e.Handled = true;
      }

      private void comboBox1_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      private void colorPickerLauncher3_Load(object sender, EventArgs e)
      {

      }

      private void colorPickerLauncher3_SelectedColorChanged(object sender, EventArgs e)
      {
         Color col = colorPickerLauncher3.SelectedColor;
         TerrainGlobals.getRender().mCursorColorTint.X = col.R / 255.0f;
         TerrainGlobals.getRender().mCursorColorTint.Y = col.G / 255.0f;
         TerrainGlobals.getRender().mCursorColorTint.Z = col.B / 255.0f;
         TerrainGlobals.getRender().mCursorColorTint.W = 1;
      }

      private void button2_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeDecalModify);
      }

      private void panel1_Paint(object sender, PaintEventArgs e)
      {

      }

      private void texturePropertiesPanel_Enter(object sender, EventArgs e)
      {

      }

      private void numericUpDown5_ValueChanged(object sender, EventArgs e)
      {

      }

      private void createMaskFromToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().selectedTextureToMask();
      }

      private void applyToMaskToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().applyTextureToMask();
      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().mEraseTextureInstead = checkBox1.Checked;
      }

   }
}
