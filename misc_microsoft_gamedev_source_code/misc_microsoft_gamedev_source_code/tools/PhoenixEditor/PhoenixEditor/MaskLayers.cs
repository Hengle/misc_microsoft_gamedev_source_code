using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;

using Terrain;
using EditorCore;
using graphapp;


namespace PhoenixEditor
{

   public partial class MaskLayers : Xceed.DockingWindows.ToolWindow /*Form*/, IDataStream, IMaskPickerUI
   {
      public MaskLayers()
      {
         InitializeComponent();

         //MaskItem mask = new MaskItem();
         //mask.Text = "--LastUsedMask--";

         //myCallback = new Image.GetThumbnailImageAbort(ThumbnailCallback);


         //this.panel1.si.


         SlopeSelectSliderControl.Setup(0.01, 0.2, true);
         SlopeSelectSliderControl.NumericValue = 0.05;
         SlopeSelectSliderControl.ValueChanged += new EventHandler(SlopeSelectSliderControl_ValueChanged);

         ScaleAmountSliderControl.Setup(0, 1, true);
         ScaleAmountSliderControl.NumericValue = 0.1;


         CurrentMaskNameTextBox.Text = "NewMaskName";

         button9.Enabled = CoreGlobals.IsDev;

      }



      public void SetLastMask(IMask mask)
      {
         MaskItem item  = null;

         string lastItemName = "--Last Selected Mask--";

         foreach(MaskItem m in MaskCheckList.Items)
         {
            if(m.Name == lastItemName)
            {
               item = m;
            }
         }
         if (item == null)
         {         
            item = new MaskItem();
            item.Name = lastItemName;

            MaskCheckList.Items.Add(item, false);

         }

         item.mMask = mask.Clone();

         CurrentMaskNameTextBox.Text = "NewMaskName";

      }

      public void ClearMaskData()
      {
         MaskCheckList.Items.Clear();
      }

      public void AddMaskToList(IMask mask, string maskName)
      {
         string mName = maskName;

         foreach (MaskItem m in MaskCheckList.Items)
         {
            if (m.Name == mName)
            {
               //is there a difference in the two?
               if (mask.GetType() == m.mMask.GetType())
               {
                  m.mMask = mask.Clone();
                  return;
               }
               else
               {
                  maskName += "1";
               }
            }
         }

         //new
         MaskItem item = new MaskItem();
         item.Name = maskName;
         item.mMask = mask;
         MaskCheckList.Items.Add(item, false);
      }

      public int GetNumMasks()
      {
         return MaskCheckList.Items.Count;
      }
      public List<string> GetMaskNames()
      {
         int numMasks = GetNumMasks();
         List<string> names = new List<string>();

         for(int i=0;i<numMasks;i++)
         {
            if (((MaskItem)MaskCheckList.Items[i]).Name[0]!='-')
               names.Add(((MaskItem)MaskCheckList.Items[i]).Name);
         }

         return names;
      }
      public IMask GetMask(string maskName)
      {
         int numMasks = GetNumMasks();

         for (int i = 0; i < numMasks; i++)
         {
            if (((MaskItem)MaskCheckList.Items[i]).Name == maskName)
               return ((MaskItem)MaskCheckList.Items[i]).mMask;
         }

         return null;
      }
      private void MaskListView_Click(object sender, EventArgs e)
      {
         //if(MaskListView.SelectedItems.Count > 0)
         //{
         //   MaskItem mask = MaskListView.SelectedItems[0] as MaskItem;

         //   if(mask != null)
         //   {
         //      TerrainGlobals.getEditor().setCurrSelectionMaskWeights(CopyMask(mask.mMask));
         //   }
         //}
         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();
      }
      private void MaskListView_SelectedIndexChanged(object sender, EventArgs e)
      {
         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();
      }

      private void MaskListView_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();
      }

      private IMask AddMasks(List<IMask> inputs)
      {

         IMask mask = MaskFactory.GetNewMask();

         foreach (IMask input in inputs)
         {
            if (input is GraphBasedMask)
               ((GraphBasedMask)input).loadAndExecute();

            float destVal;
            
            long index;
            float value;
            input.ResetIterator();
            while(input.MoveNext(out index, out value))
            {
               float baseValue = 1.0f;
               if (Masking.checkBaseWritePermission(index, out baseValue) == false)
                  continue;

               if (value == 0) continue;
               destVal = mask.GetMaskWeight(index);
               //if (destVal == 0) continue;
               destVal = value + destVal;
               if(destVal > 1) 
                  destVal = 1;

               destVal = Masking.combineValueWithBaseMask(baseValue, destVal);
               mask.SetMaskWeight(index, destVal);
               
            }
         }
         return mask;
      }

      class MaskItem 
      {
         public IMask mMask = null;
         string mName = "";
         public MaskItem()
         {
            Name = "unnamed mask";
         }

         public override string ToString()
         {
            return mName;
            
         }
         public string Name
         {
            get
            {
               return mName;
            }
            set
            {
               mName = value;
            }
         }
         public Color BackColor
         {
            get
            {
               return Color.Black;
            }
            
         }
      }


      private void MaskListView_MouseMove(object sender, MouseEventArgs e)
      {
         
      }

      private void SaveMasksButton_Click(object sender, EventArgs e)
      {
         //string filename = Path.Combine(CoreGlobals.getWorkPaths().mGameScenarioDirectory , "masks.zip");
         //SaveMaskData(filename, false, false);
      }

      private void LoadMasksButton_Click(object sender, EventArgs e)
      {
         //string filename = Path.Combine(CoreGlobals.getWorkPaths().mGameScenarioDirectory , "masks.zip");
         //LoadMaskData(filename);
      }

      static int cCurrMaskVersion = 2;
      int cNewVersionDelimeter = int.MinValue + cCurrMaskVersion;
      public bool Save(Stream s)
      {
         BinaryWriter b = new BinaryWriter(s);

         b.Write(cNewVersionDelimeter);
         b.Write(MaskCheckList.Items.Count);
         b.Write(TerrainGlobals.getTerrain().getNumXVerts());
         foreach (MaskItem item in MaskCheckList.Items)
         {
            b.Write(item.Name);
            b.Write(item.mMask.GetType().ToString());
            if (item.mMask is ArrayBasedMask)
            {
               JaggedContainer<float> container = item.mMask as JaggedContainer<float>;
               container.SaveByStripe(b,
                  (JaggedContainer<float>.SaveStripeDelegate)(delegate(BinaryWriter w, float[] values)
                  {
                     for (int i = 0; i < values.Length; i++)
                     {
                        w.Write(values[i]);
                     }
                  }));
            }
            else if (item.mMask is GraphBasedMask)
            {
               GraphBasedMask gbm = item.mMask as GraphBasedMask;
               b.Write(gbm.GraphMemStream.Length);
               gbm.GraphMemStream.Seek(0, SeekOrigin.Begin);
               gbm.GraphMemStream.WriteTo(b.BaseStream);
            }

         }



         return true;
      }

      public bool Load(Stream s)
      {
         MaskCheckList.Items.Clear();

         BinaryReader b = new BinaryReader(s);
         int delimiter = b.ReadInt32();


         if (delimiter == cNewVersionDelimeter )
         {
            return LoadV4(b);
         }
         else if (delimiter == cNewVersionDelimeter-1)
         {
            return LoadV3(b);
         }
         else if (delimiter == cNewVersionDelimeter - 2)
         {
            return LoadV2(b);
         }
         else if (delimiter != cNewVersionDelimeter)
         {
            return LoadV1(delimiter, s);
         }

         return true;
      }
      static void resampleJaggedArrayFloat(ref JaggedContainer<float> dat, int origX, int origY, int newX, int newY, float emptyVal)
      {
         float[] oldArry = new float[origX * origY];
         for (int i = 0; i < origX * origY; i++)
            oldArry[i] = dat.GetValue(i);

         float[] imgScaledX = new float[newX * newY];
         ImageManipulation.resizeF32Img(oldArry, imgScaledX, origX, origY, newX, newY, ImageManipulation.eFilterType.cFilter_Nearest);


         dat.Clear();
         dat.SetEmptyValue(emptyVal);
         for (int i = 0; i < newX * newY; i++)
         {
            if (imgScaledX[i] != emptyVal)
               dat.SetValue(i, imgScaledX[i]);
         }
         imgScaledX = null;
         oldArry = null;
      }
      public bool LoadV1(int count, Stream s)
      {
         MaskCheckList.Items.Clear();

         BinaryReader b = new BinaryReader(s);
         //int count = b.ReadInt32();
         for (int i = 0; i < count; i++)
         {
            MaskItem item = new MaskItem();
            item.mMask = MaskFactory.GetNewMask();
            item.Name = b.ReadString();
            int hashSize = b.ReadInt32();
            for (int j = 0; j < hashSize; j++)
            {
               long key = b.ReadInt64();
               float val = b.ReadSingle();
               item.mMask.SetMaskWeight(key, val);
            }
            //MaskListView.Items.Add(item);
            //CLM [07.27.07]
            //these older versions need to be scaled by 0.5 because of a 
            //massive terrain scaling change.
            int oldSize = TerrainGlobals.getTerrain().getNumXVerts() * 2;
            JaggedContainer<float> container = item.mMask as JaggedContainer<float>;
            resampleJaggedArrayFloat(ref container, oldSize, oldSize, TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumXVerts(), 0);

            MaskCheckList.Items.Add(item, false);
         }

         return true;
      }
      public bool LoadV2(BinaryReader b)
      {
         int count = b.ReadInt32();

         for (int i = 0; i < count; i++)
         {
            MaskItem item = new MaskItem();
            item.mMask = MaskFactory.GetNewMask();
            item.Name = b.ReadString();

            JaggedContainer<float> container = item.mMask as JaggedContainer<float>;
            container.LoadByStripe(b,
               (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
               {
                  for (int j = 0; j < values.Length; j++)
                  {
                     values[j] = r.ReadSingle();
                  }
               }));

            //CLM [07.27.07]
            //these older versions need to be scaled by 0.5 because of a 
            //massive terrain scaling change.
            int oldSize = TerrainGlobals.getTerrain().getNumXVerts() *2;
            resampleJaggedArrayFloat(ref container, oldSize, oldSize, TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumXVerts(), 0);

            MaskCheckList.Items.Add(item, false);
         }

         return true;
      }
      public bool LoadV3(BinaryReader b)
      {
         int count = b.ReadInt32();
         int origMapWidth = b.ReadInt32();

         for (int i = 0; i < count; i++)
         {
            MaskItem item = new MaskItem();
            item.mMask = MaskFactory.GetNewMask();
            item.Name = b.ReadString();

            JaggedContainer<float> container = item.mMask as JaggedContainer<float>;
            container.LoadByStripe(b,
               (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
               {
                  for (int j = 0; j < values.Length; j++)
                  {
                     values[j] = r.ReadSingle();
                  }
               }));

            MaskCheckList.Items.Add(item, false);
         }

         return true;
      }
      public bool LoadV4(BinaryReader b)
      {
         int count = b.ReadInt32();
         int origMapWidth = b.ReadInt32();

         for (int i = 0; i < count; i++)
         {
            MaskItem item = new MaskItem();
            item.Name = b.ReadString();
            string type = b.ReadString();
            if(type == typeof(ArrayBasedMask).ToString())
            {
               item.mMask = MaskFactory.GetNewMask();
               JaggedContainer<float> container = item.mMask as JaggedContainer<float>;
               container.LoadByStripe(b,
                  (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
                  {
                     for (int j = 0; j < values.Length; j++)
                     {
                        values[j] = r.ReadSingle();
                     }
                  }));
            }
            else if (type == typeof(GraphBasedMask).ToString())
            {
               long len = b.ReadInt64();
               item.mMask = new GraphBasedMask(b,len);
            }

            MaskCheckList.Items.Add(item, false);
         }

         return true;
      }

      public string GetSreamName() 
      {
         return "MaskData";
      }

      private void MaskCheckList_ItemCheck(object sender, ItemCheckEventArgs e)
      {
         List<IMask> masks = new List<IMask>();
         MaskItem mClickedItem = null;
         
         mClickedItem = ((MaskItem)MaskCheckList.Items[e.Index]);
         
         if(e.NewValue == CheckState.Checked)
         {
            masks.Add(mClickedItem.mMask);

            //betterPropertyGrid1.SelectedObject = mClickedItem;
            //DeleteLinkLabel.Enabled = true;
            //SaveLinkLabel.Enabled = true;

            CurrentMaskNameTextBox.Text = mClickedItem.Name;
         }
         else
         {
            //betterPropertyGrid1.SelectedObject = null;
            //DeleteLinkLabel.Enabled = false;
            //SaveLinkLabel.Enabled = false;
         }

         foreach(MaskItem m in MaskCheckList.CheckedItems)
         {
            if(mClickedItem != m)
               masks.Add(m.mMask);
         }
         if (masks.Count > 0)
         {
            Masking.setCurrSelectionMaskWeights(AddMasks(masks));
         } 
         else
         {
            Masking.setCurrSelectionMaskWeights(MaskFactory.GetNewMask());
         }         
      }




      void picker_ImageSelected(object sender, EventArgs e)
      {
         //ImageSourcePicker picker = sender as ImageSourcePicker;
         //TerrainGlobals.getEditor().mCurrentAbstractImage = picker.AbstractImage;


         ////Bitmap bits = new Bitmap(image.mMaxA-image.mMinA,image.mMaxB-image.mMinB);
         ////for(int x=image.mMinA; x<image.mMaxA; x++)
         ////{
         ////   for(int z=image.mMinB; z< image.mMaxB; z++)
         ////   {
         ////      bits.SetPixel()
         ////   }

         ////}

         ////Image myThumbnail = bitmap.GetThumbnailImage(xScale, yScale, myCallback, IntPtr.Zero);


         ////throw new Exception("The method or operation is not implemented.");
         //item.Name = "importedMask";////Path.GetFileName(d.FileName);
         //item.mMask = Masking.CreateMask(loadedImage);
         //MaskCheckList.Items.Add(item, false);
         //Masking.setCurrSelectionMaskWeights(item.mMask); 

      }




      private void DetectEdges_Click(object sender, EventArgs e)
      {
         Masking.detectEdges();
      }

      private void SmoothFilter_Click(object sender, EventArgs e)
      {
         Masking.smoothFilter();
      }

      private void SharpenFilter_Click(object sender, EventArgs e)
      {
         Masking.sharpenFilter();

      }

      private void ExpandSelection_Click(object sender, EventArgs e)
      {
         Masking.expandSelection(trackBar1.Value);
      }

      private void ContractSelection_Click(object sender, EventArgs e)
      {
         Masking.contractSelection(trackBar1.Value);
      }

      private void BlurSelection_Click(object sender, EventArgs e)
      {
         Masking.blurSelection(30/*blurWidthTrackbar.Value*/, (float)(blurSigmaTrackbar.Value / 10.0f));
      }

      private void trackBar1_Scroll(object sender, EventArgs e)
      {
         vertLabel.Text = "Verts : " + trackBar1.Value;
      }

      private void blurSigmaTrackbar_Scroll(object sender, EventArgs e)
      {
         blurSigmaLabel.Text = "Amount : " + blurSigmaTrackbar.Value;
      }

      void SlopeSelectSliderControl_ValueChanged(object sender, EventArgs e)
      {
         UpdateComponentMaskingSettings();
      }
      private void SampleSlopeButton_Click(object sender, EventArgs e)
      {
         this.UseSlopeCheckBox.Checked = true;
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSampleHeightSlope);
         TerrainGlobals.getEditor().mComponentMaskSettings.mbUseSlope = true;
         UpdateComponentMaskingSettings();
      }

      private void SampleMinHeightButton_Click(object sender, EventArgs e)
      {
         this.UseMinHeightCheckBox.Checked = true;
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSampleMinHeight);
         TerrainGlobals.getEditor().mComponentMaskSettings.mbUseMinHeight = true;
         UpdateComponentMaskingSettings();
      }

      private void SampleMaxHeightButton_Click(object sender, EventArgs e)
      {
         this.UseMaxHeightCheckBox.Checked = true;
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSampleMaxHeight);
         TerrainGlobals.getEditor().mComponentMaskSettings.mbUseMaxHeight = true;
         UpdateComponentMaskingSettings();
      }

      private void UseSlopeCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         bool mbValue = ((CheckBox)sender).Checked;
         TerrainGlobals.getEditor().mComponentMaskSettings.mbUseSlope = mbValue;
         if (mbValue == false)
            UpdateComponentMasking(); 
      }

      private void UseMinHeightCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         bool mbValue = ((CheckBox)sender).Checked;
         TerrainGlobals.getEditor().mComponentMaskSettings.mbUseMinHeight = mbValue;
         if (mbValue == false) 
            UpdateComponentMasking(); 

      }

      private void UseMaxHeightCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         bool mbValue = ((CheckBox)sender).Checked;
         TerrainGlobals.getEditor().mComponentMaskSettings.mbUseMaxHeight = mbValue;
         if (mbValue == false) 
            UpdateComponentMasking(); 
      }

      private void PostSmoothCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         bool mbValue = ((CheckBox)sender).Checked;
         TerrainGlobals.getEditor().mComponentMaskSettings.mbPostSmooth = mbValue;
         UpdateComponentMasking();
      }

      private void UpdateComponentMaskingSettings()
      {
         TerrainGlobals.getEditor().mComponentMaskSettings.mSlopeRange = (float)SlopeSelectSliderControl.NumericValue;

      }
      private void UpdateComponentMasking()
      {
         UpdateComponentMaskingSettings();
         Masking.createSelectionMaskFromTerrain(TerrainGlobals.getEditor().mComponentMaskSettings);
      }

      private void MinHeightCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         bool mbValue = ((CheckBox)sender).Checked;
         if (mbValue == true)
         {
            GradientFunction func =  new GradientFunction();
            func.mStartValue = 1;
            func.mEndValue = 0;
            TerrainGlobals.getEditor().mComponentMaskSettings.mMinGradient = func;
         }
         else
         {
            TerrainGlobals.getEditor().mComponentMaskSettings.mMinGradient = null;
         }
         UpdateComponentMasking();
      }

      private void MaxHeightCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         bool mbValue = ((CheckBox)sender).Checked;
         if (mbValue == true)
         {
            GradientFunction func = new GradientFunction();
            func.mStartValue = 0;
            func.mEndValue = 1;
            TerrainGlobals.getEditor().mComponentMaskSettings.mMaxGradient = func;
         }
         else
         {
            TerrainGlobals.getEditor().mComponentMaskSettings.mMaxGradient = null;
         }

         UpdateComponentMasking();
      }

      private void MinNoiseCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         bool mbValue = ((CheckBox)sender).Checked;
         if (mbValue == true)
         {
            NoiseFunction func = new NoiseFunction();
            TerrainGlobals.getEditor().mComponentMaskSettings.mMinNoiseFunction = func;
         }
         else
         {
            TerrainGlobals.getEditor().mComponentMaskSettings.mMinNoiseFunction = null;
         }

         UpdateComponentMasking();
      }

      private void MaxNoiseCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         bool mbValue = ((CheckBox)sender).Checked;
         if (mbValue == true)
         {
            NoiseFunction func = new NoiseFunction();
            //NoiseGeneration.RigedMultiFractal.mFrequency = 0.05f;
            TerrainGlobals.getEditor().mComponentMaskSettings.mMaxNoiseFunction = func;
         }
         else
         {
            TerrainGlobals.getEditor().mComponentMaskSettings.mMaxNoiseFunction = null;
         }

         UpdateComponentMasking();
      }

  
      ////////////////////////////////
      private void SaveCurrentButton_Click(object sender, EventArgs e)
      {
         if (CoreGlobals.getEditorMain().mPhoenixScenarioEditor.CheckTopicPermission("Masks") == false)
         {
            return;
         }

         string saveName = CurrentMaskNameTextBox.Text;
         AddMaskToList(Masking.getCurrSelectionMaskWeights().Clone(), saveName);
      }

      private void ClearCurrentButton_Click(object sender, EventArgs e)
      {
         CurrentMaskNameTextBox.Text = "NewMaskName";
         Masking.clearSelectionMask();
      }

      private void LoadLastButton_Click(object sender, EventArgs e)
      {
         
      }

      private void SetAsBaseButton_Click(object sender, EventArgs e)
      {
         Masking.setCurrentMaskToBase();
      }

      private void ClearBaseButton_Click(object sender, EventArgs e)
      {
         Masking.clearBaseMaskingMask();
      }

      private void SetAsCurrentButton_Click(object sender, EventArgs e)
      {
         Masking.setBaseMaskToCurrent();
      }

      private void DeleteMaskButton_Click(object sender, EventArgs e)
      {
         string saveName = CurrentMaskNameTextBox.Text;
         foreach (MaskItem m in MaskCheckList.Items)
         {
            string mName = m.Name;
           
            if (mName == saveName)
            {
               MaskCheckList.Items.Remove(m);
               return;
            }
         }
      }


      private void panel1_Paint(object sender, PaintEventArgs e)
      {

      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         bool mbValue = ((CheckBox)sender).Checked;
         TerrainGlobals.getEditor().mComponentMaskSettings.mUseGradiant = mbValue;
         //if (mbValue == false)
            UpdateComponentMasking(); 
      }

      private void button7_Click(object sender, EventArgs e)
      {
         float value = (float)this.ScaleAmountSliderControl.NumericValue;
         bool addorpercent = this.ScaleAddCheckBox.Checked;
         if (addorpercent == false)
         {
            value += 1.0f;
         }
         Masking.scaleSelection(value, addorpercent);
      }

      private void button8_Click(object sender, EventArgs e)
      {
         float value = (float)this.ScaleAmountSliderControl.NumericValue;
         bool addorpercent = this.ScaleAddCheckBox.Checked;
         if (addorpercent == false)
         {
            value = 1.0f - value;
         }
         else
         {
            value = -value;
         }
         Masking.scaleSelection(value, addorpercent);
      }

      private void MaskTextureButton_Click_1(object sender, EventArgs e)
      {

         Masking.MaskCurrentTexture(this.DisplacementFadeCheckBox.Checked);
      }

      private void FromClipboardButton_Click(object sender, EventArgs e)
      {
         try
         {
            IDataObject id = Clipboard.GetDataObject();
            string[] formats;
            if (id != null)
            {
               formats = id.GetFormats();
            }
            System.Drawing.Image clipboardImage = null;
            if (Clipboard.ContainsImage())
            {
               clipboardImage = Clipboard.GetImage();
               MaskItem item = new MaskItem();
               item.mMask = Masking.CreateMask(clipboardImage);
               MaskCheckList.Items.Add(item, false);
               Masking.setCurrSelectionMaskWeights(item.mMask.Clone() );
            }
         }
         catch
         {
            MessageBox.Show("Error importing image from clipboard. Image must be 24bpp RGB");
         }
      }
      private void ImportMaskButton_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();
         d.Filter = "Image (*.bmp)|*.bmp|Raw32 (*.r32)|*.r32";
         if (d.ShowDialog() == DialogResult.OK)
         {
            string tName = Path.GetFileNameWithoutExtension(d.FileName);
            bool okToLoad = true;
            for (int i = 0; i < MaskCheckList.Items.Count; i++)
            {
               MaskItem mi = MaskCheckList.Items[i] as MaskItem;
               if (Path.GetFileNameWithoutExtension(mi.Name) == tName)
               {
                  if (MessageBox.Show("There exists a similar mask name already loaded. Would you still like to import this mask?", "Warning!", MessageBoxButtons.YesNo) != DialogResult.Yes)
                     okToLoad=false;
                  break;

               }
            }
            if (!okToLoad)
               return;
            if (Path.GetExtension(d.FileName.ToLower()) == ".bmp")
            {

               Image loadedImage = Masking.GetScaledImageFromFile(d.FileName, TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumZVerts());
               MaskItem item = new MaskItem();
               item.Name = Path.GetFileName(d.FileName);
               item.mMask = Masking.CreateMask(loadedImage);
               MaskCheckList.Items.Add(item, false);
               Masking.setCurrSelectionMaskWeights(item.mMask.Clone());
               
            }
            else if(Path.GetExtension(d.FileName.ToLower()) == ".r32")
            {
               MaskItem item = new MaskItem();
               item.Name = Path.GetFileName(d.FileName);
               item.mMask = MaskFactory.GetNewMask();
               BinaryReader r = new BinaryReader(File.OpenRead(d.FileName));
               
               long index = 0;
               try
               {
                  while (true)
                  {
                     item.mMask.SetMaskWeight(index, r.ReadSingle());
                     index++;            
                  }
               }
               catch (System.IO.EndOfStreamException ex)
               {
                  ex.ToString();
               }

               if (scaleToFitCheckbox.Checked)
               {
                  //rescale this mask to fit current terrain
                  int numX = (int)Math.Sqrt(index);
                  float[] img = new float[numX * numX];
                  long id;
                  float value;
                  item.mMask.ResetIterator();
                  while (item.mMask.MoveNext(out id, out value))
                  {
                     if (value == 0)
                        continue;
                     img[id] = value;
                  }

                  int newWidth = TerrainGlobals.getTerrain().getNumXVerts();
                  float[] outImg = ImageManipulation.resizeF32Img(img, numX, numX, newWidth, newWidth, ImageManipulation.eFilterType.cFilter_Linear);
                  item.mMask.Clear();
                  for (index = 0; index < newWidth * newWidth; index++)
                     item.mMask.SetMaskWeight(index, outImg[index]);

               }
               MaskCheckList.Items.Add(item, false);
               Masking.setCurrSelectionMaskWeights(item.mMask.Clone());
               r.Close();
            }
         }
         //ImageSourcePicker picker = new ImageSourcePicker();
         //picker.ImageSelected += new EventHandler(picker_ImageSelected);
         //PopupEditor editor = new PopupEditor();
         //editor.ShowPopup(this, picker);
      }
      static public Image.GetThumbnailImageAbort myCallback = null;

      private void ToClipboardButton_Click(object sender, EventArgs e)
      {
         Image tosave = Masking.ExportMask(Masking.getCurrSelectionMaskWeights());
         if (tosave != null)
         {
            Clipboard.Clear();
            Clipboard.SetImage(tosave);
         }
      }


      private void ToFileButton_Click(object sender, EventArgs e)
      {
         SaveFileDialog d = new SaveFileDialog();
         d.Filter = "Raw32 (*.r32)|*.r32";
         if (d.ShowDialog() == DialogResult.OK)
         {
            BinaryWriter w = new BinaryWriter(File.OpenWrite(d.FileName));             
            IMask mask = Masking.getCurrSelectionMaskWeights();
            long max = TerrainGlobals.getTerrain().getNumXVerts() * TerrainGlobals.getTerrain().getNumXVerts();
            for(long i = 0 ; i < max; i++)
            {
               w.Write(mask.GetMaskWeight(i));               
            }
            w.Close(); 
         }
      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {

      }

      

      private void button9_Click(object sender, EventArgs e)
      {
         MaskGenFrm mgf = new MaskGenFrm();
         PopupEditor pe = new PopupEditor();
         pe.ShowPopup(this, mgf);
      }

      private void previewToolStripMenuItem_Click(object sender, EventArgs e)
      {
         
         MaskItem mClickedItem = ((MaskItem)MaskCheckList.Items[MaskCheckList.SelectedIndex]);
         if(mClickedItem.mMask is GraphBasedMask)
         {
            GraphBasedMask gbp = (GraphBasedMask)mClickedItem.mMask;
            
            MaskGenFrm mgf = new MaskGenFrm();
            mgf.loadCanvasFromMemoryStream(gbp.GraphMemStream);

            PopupEditor pe = new PopupEditor();
            pe.ShowPopup(this, mgf);

         }
         else
         {

         }
         
      }


   }


   
}