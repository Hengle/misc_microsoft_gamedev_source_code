using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using Microsoft.DirectX;

using Terrain;
using EditorCore;

//Stupid ass xceed junky namespaces WTF!
using Xceed.SmartUI;
using Xceed.SmartUI.Controls;
using Xceed.SmartUI.Controls.OptionList;
using Xceed.SmartUI.Controls.TreeView;
using Xceed.SmartUI.Controls.ToolBar;
using Xceed.SmartUI.Controls.CheckedListBox;


namespace PhoenixEditor
{
   public partial class ClipArtPicker : Xceed.DockingWindows.ToolWindow //: Form
   {
      public ClipArtPicker()
      {
         InitializeComponent();

         LoadClipArt();

         this.subfolderPicker1.RootFolder = CoreGlobals.getWorkPaths().mClipArt;
         subfolderPicker1.FolderSelected += new EventHandler(subfolderPicker1_FolderSelected);

         InitTreeHandlers(smartTreeView1);

         RotationSliderControl.Setup(-90, 480, true);
         RotationSliderControl.ValueChanged += new EventHandler(RotationSliderControl_ValueChanged);

         this.ScaleXSliderControl.Logrithmic = true;
         ScaleXSliderControl.Setup(0, 5, true);
         ScaleXSliderControl.ValueChanged += new EventHandler(ScaleSliderControl_ValueChanged);

         this.ScaleYSliderControl.Logrithmic = true;
         ScaleYSliderControl.Setup(0, 5, true);
         ScaleYSliderControl.ValueChanged += new EventHandler(ScaleYSliderControl_ValueChanged);

         this.ScaleZSliderControl.Logrithmic = true;
         ScaleZSliderControl.Setup(0, 5, true);
         ScaleZSliderControl.ValueChanged += new EventHandler(ScaleZSliderControl_ValueChanged);

         HeightSliderControl.Setup(-100, 100, true);
         HeightSliderControl.ValueChanged += new EventHandler(HeightSliderControl_ValueChanged);

         SetDefaults();

         mUpdateTimer.Interval = 300;
         mUpdateTimer.Tick += new EventHandler(mUpdateTimer_Tick);
         mUpdateTimer.Start();

      }

      public void SetDefaults()
      {
         this.RotationSliderControl.NumericValue = 0;
         this.ScaleXSliderControl.NumericValue = 1;
         this.ScaleYSliderControl.NumericValue = 1;
         this.ScaleZSliderControl.NumericValue = 1;
         this.HeightSliderControl.NumericValue = 0;
         this.radioReplaceButton.Checked = true;
         this.SmartHeightcheckBox.Checked = false;
         this.ShowBBOnlyCheckBox.Checked = false;
      }


      Timer mUpdateTimer = new Timer();

      bool mbSuspendUIUpdate = false;
      void mUpdateTimer_Tick(object sender, EventArgs e)
      {
         mbSuspendUIUpdate = true;
         int x = TerrainGlobals.getEditor().mLastTerrainPasteLocationX;
         int z = TerrainGlobals.getEditor().mLastTerrainPasteLocationZ;

         XPosUpDown.Value = x;
         ZPosUpDown.Value = z;



         if (mbDirty == true )
         {
            if(((System.TimeSpan)(System.DateTime.Now - mLastDirtyTime)).TotalMilliseconds > 400)
            {
               if (Control.MouseButtons == MouseButtons.Left)
               {
                  return;
               }
               if (ShowBBOnlyCheckBox.Checked == true)
               {
                  return;
               }
         
               mbDirty = false;
               TerrainGlobals.getEditor().TransformClipart(GetXScale(), GetYScale(), GetZScale(), GetRotation());
               TerrainGlobals.getEditor().applyPastedData(x, z, GetYOffset());
               CoreGlobals.getEditorMain().mOneFrame = true;

               ChangeLabel.Text = "Done";
               ChangeLabel.ForeColor = System.Drawing.Color.Green;
            }
         }
         mbSuspendUIUpdate = false;
      }
      private float GetYOffset()
      {
         float v = (float)this.HeightSliderControl.NumericValue;

         if (SmartHeightcheckBox.Checked == true)
         {
            Vector3 min;
            Vector3 max;
            
            CopiedVertexData vertexData = TerrainGlobals.getEditor().GetClipartVertexData();
            if(vertexData != null)
            {
               vertexData.GetOriginalSize(out min, out max);
               v = v - min.Y;
            }
         }
         return v;
      }

      private float GetZScale()
      {
         return (float)ScaleZSliderControl.NumericValue;
      }
      private float GetXScale()
      {
         return (float)ScaleXSliderControl.NumericValue;
      }
      private float GetYScale()
      {
         return (float)ScaleYSliderControl.NumericValue;
      }
      private float GetRotation()
      {
         return (float)((RotationSliderControl.NumericValue / 180) * (Math.PI));
      }
      void DrawClipArtBB()
      {
         TerrainGlobals.getEditor().setPastedDataPreview((int)XPosUpDown.Value, (int)ZPosUpDown.Value, GetYOffset(), GetRotation(), GetXScale(), GetYScale(), GetZScale());
      }
      void RotationSliderControl_ValueChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate == true) return;


         ClipArtSetDirty();
      }
      void ScaleSliderControl_ValueChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate == true) return;
         if (LockScaleCheckBox.Checked == true)
         {
            mbSuspendUIUpdate = true;
            ScaleZSliderControl.NumericValue = ScaleXSliderControl.NumericValue;
            ScaleYSliderControl.NumericValue = ScaleXSliderControl.NumericValue;
            mbSuspendUIUpdate = false;
         }

         ClipArtSetDirty();
      }

      void ScaleZSliderControl_ValueChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate == true) return;

         if(LockScaleCheckBox.Checked == true)
         {
            mbSuspendUIUpdate = true;
            ScaleXSliderControl.NumericValue = ScaleZSliderControl.NumericValue;
            ScaleYSliderControl.NumericValue = ScaleZSliderControl.NumericValue;
            mbSuspendUIUpdate = false;
         }

         ClipArtSetDirty();
      }
      void ScaleYSliderControl_ValueChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate == true) return;
         if (LockScaleCheckBox.Checked == true)
         {
            mbSuspendUIUpdate = true;
            ScaleXSliderControl.NumericValue = ScaleYSliderControl.NumericValue;
            ScaleZSliderControl.NumericValue = ScaleYSliderControl.NumericValue;
            mbSuspendUIUpdate = false;
         }
         ClipArtSetDirty();
      }
      void HeightSliderControl_ValueChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate == true) return;
         ClipArtSetDirty();
      }

      bool mbDirty = false;
      DateTime mLastDirtyTime;
      void ClipArtSetDirty()
      {
         mLastDirtyTime = System.DateTime.Now;
         if(!mbDirty)
         {
            ChangeLabel.Text = "Settings Changed...";
            ChangeLabel.ForeColor = System.Drawing.Color.Red;
         }
         mbDirty = true;

         DrawClipArtBB();
         CoreGlobals.getEditorMain().mOneFrame = true;
      }


      public int mThumbnailSize = 128;
      public bool mbRecursive = true;

      int mMinVertValue;
      int mMaxVertValue;
      int mRangeLevels = 20;
      int mSizeRange;
      int mValueScale = 100;
      public void LoadClipArt()
      {
         mMinVertValue = int.MaxValue;
         mMaxVertValue = int.MinValue;

         mClipData.Clear();
         LoadDirectory(CoreGlobals.getWorkPaths().mClipArt, mbRecursive);
         LoadControls();

         if (mMinVertValue < 0 || mMinVertValue == int.MaxValue) mMinVertValue = 0;
         if (mMaxVertValue < 0) mMaxVertValue = 1;
         if(mMaxVertValue == mMinVertValue) mMaxVertValue++;

         mSizeRange = (int)((mMaxVertValue - mMinVertValue) / mRangeLevels);
         sizeFiltertrackBar.Minimum = mMinVertValue / mValueScale;
         sizeFiltertrackBar.Maximum = mMaxVertValue / mValueScale;
      }

      Dictionary<string, ClipArtData> mClipData = new Dictionary<string,ClipArtData>();

      public void LoadControls()
      {
         flowLayoutPanel1.Controls.Clear();
         string containsSearchString = "";//this.ContainstextBox.Text;
         string filnameContains = "";//this.fileNameContainstextBox.Text;


         foreach(ClipArtData d in mClipData.Values)
         {
            if (SearchDescriptionCheckBox.Checked)
            {
               int matchcount = 0;
               string dontcare;
               List<string> tags = GetMetaDataFromTree(smartTreeView1, out dontcare);  
               foreach (string s in tags)
               {
                  if (d.mMetadata.mTags.Contains(s))
                  {
                     matchcount++;
                  }

               }
               if(matchcount == 0)
                  continue;
            }
            if (SearchFolderCheckBox.Checked)
            {
               //if (!d.mFileName.Contains(filnameContains))
               //   continue;
               if (!d.mFileName.Contains(subfolderPicker1.RelativePath))
                  continue;
            }            
            if(SearchSizeFilterCheckBox.Checked == true)
            {
               int max = (sizeFiltertrackBar.Value * mValueScale) + mSizeRange;
               int min = (sizeFiltertrackBar.Value * mValueScale) - mSizeRange;

               if (d.mMetadata.mVertCount > max || d.mMetadata.mVertCount < min)
                  continue;
            }
           
            ClipArtButton b = new ClipArtButton(this, d);
            flowLayoutPanel1.Controls.Add(b);
            
         }

      }

      public void LoadDirectory(string dir, bool bRecursive)
      {
         string[] files = Directory.GetFiles(dir, "*.esd");

         foreach (string filename in files)
         {
            ClipArtData d = new ClipArtData(filename);
            if (d.Preview())
            {
               mClipData[d.mFileName] = d;

               int numVerts = d.mMetadata.mVertCount;
               if (numVerts != 0 && numVerts < mMinVertValue)
                  mMinVertValue = numVerts;
               if (numVerts != 0 && numVerts > mMaxVertValue)
                  mMaxVertValue = numVerts;
           
            }
         }

         if(bRecursive)
         {
            string[] directories = Directory.GetDirectories(dir);
            foreach (string subdir in directories)
            {
               LoadDirectory(subdir, bRecursive);
            }
         }
      }
      ClipArtButton mSelectedButton = null;

      void SelectButton(ClipArtButton b)
      {
         if (mSelectedButton != null)
            mSelectedButton.Invalidate();

         mSelectedButton = b;
         mSelectedButton.Invalidate();

         //selectedgroupBox.Text = String.Format("Selected: {0}", Path.GetFileName(mSelectedButton.mData.mFileName));
         //this.selectedMetaDesctextBox.Text = mSelectedButton.mData.mMetadata.mDescription;

         //this.selectedDesctextBox.Text = String.Format("VertCount:{0}\r\nDataChannels:\r\n{1}", mSelectedButton.mData.mMetadata.mVertCount, mSelectedButton.mData.getComponentList());

         TerrainGlobals.getEditor().SetClipArt(mSelectedButton.mData);
         TerrainGlobals.getTerrainFrontEnd().PasteMode();
      }

      class ClipArtButton : PictureBox
      {
         public ClipArtData mData = null;
         public ClipArtPicker mParent = null;
         
         public ClipArtButton(ClipArtPicker parent, ClipArtData data)
         {
            mData = data;
            mParent = parent;
            Height = parent.mThumbnailSize;
            Width = parent.mThumbnailSize;
            Image = mData.mThumbnail;
            // = mData.mFileName;
            SizeMode = PictureBoxSizeMode.StretchImage;

            //this.DoubleBuffered = true;

         }
         Brush mBr = new SolidBrush(Color.Yellow);
         protected override void OnPaint(PaintEventArgs pe)
         {
            base.OnPaint(pe);

            pe.Graphics.DrawString(Path.GetFileName(mData.mFileName), Font,mBr, 0,0);
            if ((mParent.mSelectedButton == this) && (Image != null))
            {
               pe.Graphics.DrawRectangle(new Pen(Color.Red, 2f), 2, 2, this.Width - 4, this.Height - 4);
            }

         }

         protected override void OnClick(EventArgs e)
         {

            mParent.SelectButton(this);

            //base.OnClick(e);
            CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

         }



      }

      private void RefreshButton_Click(object sender, EventArgs e)
      {
         LoadClipArt();
         this.subfolderPicker1.RootFolder = CoreGlobals.getWorkPaths().mClipArt;

         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

      }

      
      private void DescContainscheckBox_CheckedChanged(object sender, EventArgs e)
      {
         //DescContainscheckBox.Checked;
         LoadControls();

         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

      }

      private void FileNameContainscheckBox_CheckedChanged(object sender, EventArgs e)
      {
         LoadControls();

         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

      }

      //private void ContainstextBox_TextChanged(object sender, EventArgs e)
      //{
      //   if (DescContainscheckBox.Checked)
      //      LoadControls();
      //}

      //private void fileNameContainstextBox_TextChanged(object sender, EventArgs e)
      //{
      //   if (FileNameContainscheckBox.Checked)
      //      LoadControls();
      //}

      private void SizeFilterCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         LoadControls();

         if (SearchSizeFilterCheckBox.Checked == true)
            this.sizeFiltertrackBar.Enabled = true;
         else
            this.sizeFiltertrackBar.Enabled = false;

         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

      }

      private void sizeFiltertrackBar_Scroll(object sender, EventArgs e)
      {
         if (SearchSizeFilterCheckBox.Checked)
            LoadControls();

         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();


      }

      private void UpdateMetaDatabutton_Click(object sender, EventArgs e)
      {
         bool overwriteReadOnly = this.UpdateReadOnlycheckBox.Checked;
         foreach(ClipArtButton d in flowLayoutPanel1.Controls)
         {
            d.mData.Update(overwriteReadOnly);            
         }
         LoadClipArt();

         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

      }

      private void SearchFolderCheckBox_CheckedChanged(object sender, EventArgs e)
      {

         if(SearchFolderCheckBox.Checked == true)
         {
            this.subfolderPicker1.Enabled = true;
            //this.SearchSubfoldersCheckBox.Enabled = true;
         }
         else
         {
            this.subfolderPicker1.Enabled = false;
            this.SearchSubfoldersCheckBox.Enabled = false;
         }
         LoadControls();


         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

      }

      private void SearchDescriptionCheckBox_CheckedChanged(object sender, EventArgs e)
      {


         if (SearchDescriptionCheckBox.Checked == true)
         {
            smartTreeView1.Enabled = true;
         }
         else
         {
            smartTreeView1.Enabled = false;


            this.radioButtonNode1.Checked = false;
            this.radioButtonNode2.Checked = false;
            this.radioButtonNode3.Checked = false;
         }
         LoadControls();

         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

      }

      private void SearchSubfoldersCheckBox_CheckedChanged(object sender, EventArgs e)
      {


         if (SearchSubfoldersCheckBox.Checked == true)
         {

         }
         else
         {

         }
         LoadControls();

         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

      }
      void subfolderPicker1_FolderSelected(object sender, EventArgs e)
      {
         LoadControls();


         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

      }



      List<SmartItem> FlattenTree(SmartItem root)
      {
         List<SmartItem> itemList = new List<SmartItem>();
         itemList.Add(root);
         SmartItem item = root.Items.GetFirstItem(SmartItemState.Any);
         while (item != null)
         {
            itemList.AddRange(FlattenTree(item).ToArray());
            item = root.Items.GetNextItem(item, SmartItemState.Any);
         }
         return itemList;
      }

      public List<string> GetMetaDataFromTree(SmartTreeView treeview, out string quickName)
      {
         List<string> metadata = new List<string>();
         StringWriter b = new StringWriter();
         List<SmartItem> itemList = FlattenTree(treeview.Items.GetFirstItem(SmartItemState.Any));

         string tagName = "";
         string mainName = "";
         quickName = "";
         foreach (SmartItem item in itemList)
         {
            if (item is CheckedListBoxItem)
            {
               if (((CheckedListBoxItem)item).Checked == true)
               {
                  metadata.Add(item.Tag.ToString());
               }
            }
            if (item is RadioButtonNode)
            {

               if (((RadioButtonNode)item).Checked == true)
               {
                  metadata.Add(item.Tag.ToString());
                  mainName = item.Tag.ToString();
               }
            }
            if (item is TextBoxTool)
            {
               if (((TextBoxTool)item).Text != "")
               {
                  TextBoxTool tb = (TextBoxTool)item;

                  if (tb.VirtualTextBox.Text != "")
                  {
                     metadata.Add(tb.VirtualTextBox.Text.ToString());

                     if (tb.Text == "Tag")
                     {
                        tagName = tb.VirtualTextBox.Text;

                     }
                  }

               }

            }
         }

         //build a quickname
         int names = 0;
         int maxNames = 4;
         if (tagName != "")
         {
            quickName += tagName;
            names++;
         }
         if (mainName != "")
         {
            quickName += mainName;
            names++;
         }
         foreach (string s in metadata)
         {
            if (names <= maxNames && s != tagName && s != mainName)
            {
               quickName += s;
               names++;
            }
         }
         return metadata;
      }

      private void InitTreeHandlers(SmartTreeView treeview)
      {
         List<SmartItem> itemList = FlattenTree(treeview.Items.GetFirstItem(SmartItemState.Any));
         foreach (SmartItem item in itemList)
         {
            item.Click += new SmartItemClickEventHandler(item_Click);
            if (item is TextBoxTool)
            {

               TextBoxTool tb = (TextBoxTool)item;
               tb.VirtualTextBox.TextChanged += new EventHandler(VirtualTextBox_TextChanged);
            }
         }
      }

      void VirtualTextBox_TextChanged(object sender, EventArgs e)
      {
         LoadControls();
         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();
      }

      void item_Click(object sender, SmartItemClickEventArgs e)
      {
         LoadControls();
         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();
      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().mUseClipartVertexData = checkBox1.Checked;
      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().mUseClipartTextureData = checkBox2.Checked;
      }

      private void checkBox3_CheckedChanged(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().mUseClipartSimData = checkBox3.Checked;
      }

      private void XPosUpDown_ValueChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate) return;

         TerrainGlobals.getEditor().mLastTerrainPasteLocationX = (int)XPosUpDown.Value;

         ClipArtSetDirty();

      }

      private void ZPosUpDown_ValueChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate) return;

         TerrainGlobals.getEditor().mLastTerrainPasteLocationZ = (int)ZPosUpDown.Value;

         ClipArtSetDirty();
      }

      private void DefaultButton_Click(object sender, EventArgs e)
      {
         SetDefaults();
      }

      private void radioAddButton_CheckedChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate) return;
         if (radioAddButton.Checked == false) return;
         TerrainGlobals.getTerrainFrontEnd().PasteOperation = BTerrainFrontend.ePasteOperation.Add;
         ClipArtSetDirty();

      }

      private void radioReplaceButton_CheckedChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate) return;
         if (radioReplaceButton.Checked == false) return;

         TerrainGlobals.getTerrainFrontEnd().PasteOperation = BTerrainFrontend.ePasteOperation.Replace;
         ClipArtSetDirty();

      }
      private void radioTallestButton_CheckedChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate) return;
         if (radioTallestButton.Checked == false) return;

         TerrainGlobals.getTerrainFrontEnd().PasteOperation = BTerrainFrontend.ePasteOperation.Merge;
         ClipArtSetDirty();

      }

      private void AddHeightRadioButton_CheckedChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate) return;
         if (AddHeightRadioButton.Checked == false) return;

         TerrainGlobals.getTerrainFrontEnd().PasteOperation = BTerrainFrontend.ePasteOperation.AddYOnly;
         ClipArtSetDirty();
      }

      private void RightClickOKCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().mbRightClickApply = RightClickOKCheckBox.Checked;
      }

      private void ApplyButton_Click(object sender, EventArgs e)
      {
         if(mbDirty == true)
         {
            Cursor.Current = Cursors.WaitCursor;
            int x = TerrainGlobals.getEditor().mLastTerrainPasteLocationX;
            int z = TerrainGlobals.getEditor().mLastTerrainPasteLocationZ;

            TerrainGlobals.getEditor().TransformClipart(GetXScale(), GetYScale(), GetZScale(), GetRotation());
            TerrainGlobals.getEditor().applyPastedData(x, z, GetYOffset());
            CoreGlobals.getEditorMain().mOneFrame = true;
            Cursor.Current = Cursors.Default;
         }

         TerrainGlobals.getEditor().OKPaste();
      }

      private void SmartHeightcheckBox_CheckedChanged(object sender, EventArgs e)
      {
         if (mbSuspendUIUpdate) return;

         ClipArtSetDirty();

      }

      private void ShowBBOnlyCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().mbClipartBBOnly = ShowBBOnlyCheckBox.Checked;
         if (mbSuspendUIUpdate) return;
         ClipArtSetDirty();


         if (TerrainGlobals.getEditor().mbClipartBBOnly == false)
         {
            Cursor.Current = Cursors.WaitCursor;
            int x = TerrainGlobals.getEditor().mLastTerrainPasteLocationX;
            int z = TerrainGlobals.getEditor().mLastTerrainPasteLocationZ;

            TerrainGlobals.getEditor().TransformClipart(GetXScale(), GetYScale(), GetZScale(), GetRotation());
            TerrainGlobals.getEditor().applyPastedData(x, z, GetYOffset());
            CoreGlobals.getEditorMain().mOneFrame = true;
            Cursor.Current = Cursors.Default;
         }
         else
         {
            TerrainGlobals.getEditor().clearBrushDeformations();
         }
      }


   }






}