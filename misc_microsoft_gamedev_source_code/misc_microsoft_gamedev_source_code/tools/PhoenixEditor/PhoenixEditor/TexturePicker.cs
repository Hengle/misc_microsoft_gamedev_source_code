using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using Terrain;
using EditorCore;
using Rendering;
using SimEditor;

namespace PhoenixEditor
{
   class TexturePalette : ToolStrip
   {
      int mSelectedTextureID = 0;
      public string mTextureDirectory = @"textures\terrain";
      public string mBaseDirectory = CoreGlobals.getWorkPaths().mBaseDirectory;
      List<TextureButton> mTextureButtons = new List<TextureButton>();
      //public ToolStrip mToolStrip = new ToolStrip();

      public delegate void TextureSelected(int id);
      public delegate void TextureChanged(int id, TerrainTextureDef type);

      private bool mDontDoInvokeCallback = false;

      public event TextureSelected mTextureSelected = null;
      public event TextureChanged mTextureChanged = null;

      public TexturePaletteViewer tpv = new TexturePaletteViewer();

      public bool mIsMaskPalette = false;
      public bool mbRestrictTextureSize = true;
      public int mButtonSize = 48;
      public TexturePalette()
      {
         myCallback = new Image.GetThumbnailImageAbort(ThumbnailCallback);

         //string asdf = AppDomain.CurrentDomain.FriendlyName;
         //System.Reflection.Assembly a = System.Reflection.Assembly.GetCallingAssembly();

         mSelectedButton.Click += new EventHandler(mSelectedButton_Click);
      }

      void mSelectedButton_Click(object sender, EventArgs e)
      {
         SelectedTexture = SelectedTexture;
      }
      public int SelectedTexture
      {
         get
         {
            return mSelectedTextureID;
         }
         set
         {
            mSelectedTextureID = value;

            foreach (TextureButton b in mTextureButtons)
            {
               b.Invalidate();
            }
            if (mTextureSelected != null)
               mTextureSelected.Invoke(mSelectedTextureID);
            
            if(mTextureButtons.Count > mSelectedTextureID)
               mSelectedButton.Image = mTextureButtons[mSelectedTextureID].Image;
         }
      }
      protected void OnTextureChanged(int id,  TerrainTextureDef type)
      {
         if (mTextureChanged != null && !mDontDoInvokeCallback)
            mTextureChanged.Invoke(id, type);
      }
 

      ToolStripButton mSelectedButton = new ToolStripButton();
      ToolStripButton mSavePaletteButton = null;
      ToolStripButton mLoadPaletteButton = null;

      public void init(string []textures)
      {
         mTextureButtons.Clear();
         Items.Clear();

         Items.Add(mSelectedButton);

         for (int i = 0; i < textures.Length; i++)
         {
            TextureButton button = new TextureButton(i, this, "");
            button.TextureName = textures[i];
            Items.Add(button);
            mTextureButtons.Add(button);
         }
         mSelectedButton.Image = mTextureButtons[0].Image;
         mIsMaskPalette = true;
      }
      public void threadedInit(string[] textures)
      {
         mTextureButtons.Clear();
         Items.Clear();

         Items.Add(mSelectedButton);

         //for (int i = 0; i < textures.Length; i++)
         //{
         //   TextureButton button = new TextureButton(i, this, "");
         //   button.TextureName = textures[i];
         //   Items.Add(button);
         //   mTextureButtons.Add(button);
         //}
         texturesToLoad = textures;
         System.Threading.Thread t = new System.Threading.Thread(new System.Threading.ThreadStart(worker));
         //t.Priority = System.Threading.ThreadPriority.lo;
         t.Start();

         //mSelectedButton.Image = mTextureButtons[0].Image;
         mIsMaskPalette = true;

      }
      //List<TextureButton> mPostAdd = new List<TextureButton>();
      string[] texturesToLoad = null;
      public void worker()
      {
         for (int i = 0; i < texturesToLoad.Length; i++)
         {
            TextureButton button = new TextureButton(i, this, "");
            button.TextureName = texturesToLoad[i];
            //Items.Add(button);
            mTextureButtons.Add(button);
         }

         this.Invoke((MethodInvoker)(delegate()
         {
            foreach (TextureButton b in mTextureButtons)
            {
               Items.Add(b);
            }
            mSelectedButton.Image = mTextureButtons[0].Image;
         }));
      }

      public void join()
      {
         foreach (TextureButton b in mTextureButtons)
         {
            Items.Add(b);
         }
      }

      public void init()
      {
         mTextureButtons.Clear();
         Items.Clear();

         Items.Add(mSelectedButton);

         for (int i = 0; i < SimTerrainType.mActiveWorkingSet.Count; i++)
         {
            TerrainTextureDef d = SimTerrainType.getFromNumber(SimTerrainType.mActiveWorkingSet[i]);
            TextureButton button = new TextureButton(i, this,d.TypeName);
            button.TextureName = CoreGlobals.getWorkPaths().mTerrainTexturesPath + @"\" + d.Theme + @"\" + d.TextureName + ".tga";
            Items.Add(button);
            mTextureButtons.Add(button);
         }
         mSelectedButton.Image = mTextureButtons[0].Image;

         if (mLoadPaletteButton == null)
         {
            mLoadPaletteButton = new ToolStripButton();
            mLoadPaletteButton.Text = "Load Palette";
            mLoadPaletteButton.Click += new EventHandler(mLoadPaletteButton_Click);
            
         }
         

         if(mSavePaletteButton==null)
         {
            mSavePaletteButton = new ToolStripButton();
            mSavePaletteButton.Text = "Save Palette";
            mSavePaletteButton.Click += new EventHandler(mSavePaletteButton_Click);
            
         }

         Items.Add(mLoadPaletteButton);
         Items.Add(mSavePaletteButton);
         
      }

      void mLoadPaletteButton_Click(object sender, EventArgs e)
      {
         mDontDoInvokeCallback = true;

         OpenFileDialog d = new OpenFileDialog();
         d.Filter = "Terrain Texture Palette (*" + SimTerrainType.mTextureSetExtention + ")|*" + SimTerrainType.mTextureSetExtention;
         d.InitialDirectory = CoreGlobals.getWorkPaths().mTerrainTexturesPath;
         if (d.ShowDialog() == DialogResult.OK)
         {
            SimTerrainType.loadActiveSetFromPalette(d.FileName);
            init();
           // TerrainGlobals.getTexturing().reloadActiveTextures(true);
         }

         mDontDoInvokeCallback = false;  
      }

      void mSavePaletteButton_Click(object sender, EventArgs e)
      {
         SaveFileDialog d = new SaveFileDialog();
         d.Filter = "Terrain Texture Set (*" + SimTerrainType.mTextureSetExtention + ")|*" + SimTerrainType.mTextureSetExtention;
         d.InitialDirectory = CoreGlobals.getWorkPaths().mTerrainTexturesPath;
         if (d.ShowDialog() == DialogResult.OK)
         {
            string fname = d.FileName;
            if (!Path.HasExtension(fname))
               fname += SimTerrainType.mTextureSetExtention;

            List<string> texSet = SimTerrainType.getTypeNamesOfActiveSet();
            SimTerrainType.writeTerrainPalette(fname, texSet);

         }
      }

      //Does nothing...
      public Image.GetThumbnailImageAbort myCallback;
      public bool ThumbnailCallback()
      {
         return false;
      }

      class TextureButton : ToolStripSplitButton
      {
         int mID;
         TexturePalette mPalette;
         string mTexture = "none";
         string mTypeName = "type";
         

         public TextureButton(int id, TexturePalette palette,string typeName)
         {
            mID = id;
            mPalette = palette;
            mTypeName = typeName;
            DropDown.Items.Add("Load", null, new EventHandler(OnLoad));
            DropDown.Items.Add("Clear", null, new EventHandler(OnClear));
            Click += new EventHandler(TextureButton_Click);
         }
         public string TextureName
         {
            set
            {
              
                  if (Load(value))
                  {
                     mTexture = value;
                     mPalette.OnTextureChanged(mID, SimTerrainType.getFromTextureName(mTexture));
                  }
                  else                  
                  {
                     if (Load(EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName))
                     {
                        mTexture = EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName;
                        mPalette.OnTextureChanged(mID, SimTerrainType.getFromTextureName(mTexture));
                     }
                  }
               
            }
            get
            {
               return mTexture;
            }
         }
         bool Load(string texFilename)
         {
            string ext = Path.GetExtension(texFilename);
            string fullFileName = texFilename;
            Bitmap bitmap = null;

            if (ext != @".bmp")//for masks
            {
               fullFileName = CoreGlobals.getWorkPaths().getWorkingTexName(texFilename);

               if (File.Exists(fullFileName))
               {
                  Texture tex = TextureLoader.FromFile(BRenderDevice.getDevice(), fullFileName);
                  Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
                  bitmap = new Bitmap(img);

                  if (mPalette.mbRestrictTextureSize && (bitmap.Height != 512 || bitmap.Width != 512 ||
                     (bitmap.PixelFormat != System.Drawing.Imaging.PixelFormat.Format32bppArgb)))
                  {
                     MessageBox.Show(String.Format("Texture Format must be Format32bppRgb or Format24bppRgb RGB\n {0} is {1} by {2} by {3}", fullFileName, bitmap.Height, bitmap.Width, bitmap.PixelFormat.ToString()));
                     return false;
                  }
                  else if (bitmap.Height != 512 || bitmap.Width != 512)
                  {
                     this.DisplayStyle = ToolStripItemDisplayStyle.Image;
                     DropDown.Items.Clear();
                     return false;
                  }

               }
               else
               {
                  Texture tex = TextureLoader.FromFile(BRenderDevice.getDevice(), EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName);
                  Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
                  bitmap = new Bitmap(img);
               }

            }
            else //OUR BMP MASKS
            {
               if (File.Exists(fullFileName))
               {
                  bitmap = new Bitmap(fullFileName);
              }
               else
               {
                  Texture tex = TextureLoader.FromFile(BRenderDevice.getDevice(), EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName);
                  Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
                  bitmap = new Bitmap(img);
               }
            }

            Image myThumbnail = bitmap.GetThumbnailImage(mPalette.mButtonSize, mPalette.mButtonSize, mPalette.myCallback, IntPtr.Zero);
            ImageScaling = ToolStripItemImageScaling.None;
            Image = myThumbnail;
            ToolTipText = texFilename;

           
            return true;
         }
         void OnLoad(object sender, EventArgs e)
         {
            UIManager.Pause();

            if(mPalette.mIsMaskPalette)
            {
               OpenFileDialog d = new OpenFileDialog();
               d.Filter = "Terrain Texture Masks (*.bmp)|*.bmp";
               d.InitialDirectory = CoreGlobals.getWorkPaths().mBrushMasks;
               if (d.ShowDialog() == DialogResult.OK)
               {
                  TextureName = (d.FileName);
               }

            }
            else
            {
               if (mPalette.tpv.ShowDialog() == DialogResult.OK)
               {
                  TextureName = SimTerrainType.getWorkingTexName(mPalette.tpv.mSelectedDef);
               }
            }
            

            UIManager.UnPause();
         }
         void OnClear(object sender, EventArgs e)
         {
            TextureName = EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName;
         }

         void TextureButton_Click(object sender, EventArgs e)
         {
            if (TextureName.CompareTo(EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName) == 0)
            {
               OnLoad(sender, e);
            }
            else
            {
               mPalette.SelectedTexture = mID;
            }
         }
         protected override void OnPaint(PaintEventArgs e)
         {
            base.OnPaint(e);
            if ((mPalette.SelectedTexture == mID) && (Image != null))
            {
               e.Graphics.DrawRectangle(new Pen(Color.Red, 2f), 2, 2, Image.Width - 1, Image.Height - 1);
            }
         }

      }
   }


   public class ScrollStripItem : ToolStripItem
   {
      public ScrollStripItem(string name, float min, float max)
      {
         mValueName = name;
         mMin = min;
         mMax = max;
       

         ImageScaling = ToolStripItemImageScaling.None;
         Image = new Bitmap(180, 20); //this seems to be the best way to set the size of this control
         Anchor = AnchorStyles.None;
         this.MouseMove += new MouseEventHandler(ScrollStripItem_MouseMove);
         this.MouseUp += new MouseEventHandler(ScrollStripItem_MouseUp);
         this.MouseLeave += new EventHandler(ScrollStripItem_MouseLeave);
         this.MouseDown += new MouseEventHandler(ScrollStripItem_MouseDown);         
      }

      float mValue = 0.5f;
      float mMin = 0;
      float mMax = 10;
      public float Value
      {
         get
         {
            return mValue;
         }
         set
         {
            if (value >= mMin && value <= mMax)
            {
               mValue = value;
               Invalidate();
            }
         }
      }

      string mValueName;
      public string ValueName
      {
         get
         {
            return mValueName;
         }
      }
      public delegate void ValueChanged(float val);
      public event ValueChanged mValueChanged = null;
      int X = 0;
      int Y = 0;
      public bool mbLiveUpdate = true;
      public bool mbShowValue = false;


      void ScrollStripItem_MouseUp(object sender, MouseEventArgs e)
      {
         if (mbLiveUpdate == false && mValueChanged != null)
            mValueChanged.Invoke(Value);
      }
      void ScrollStripItem_MouseLeave(object sender, EventArgs e)
      {
         if (mbLiveUpdate == false && mValueChanged != null)
            mValueChanged.Invoke(Value);
      }
      void ScrollStripItem_MouseDown(object sender, MouseEventArgs e)
      {
         mouseChange(e);
      }
      void ScrollStripItem_MouseMove(object sender, MouseEventArgs e)
      {
         mouseChange(e);
      }
      void mouseChange(MouseEventArgs e)
      {
         X = e.X;
         Y = e.Y;
         if (e.Button == MouseButtons.Left)
         {
            float v = 1 - ((DataWidth - X) / (float)(DataWidth));
            Value = (v * (mMax - mMin)) + mMin;
            Invalidate();
            if ((mbLiveUpdate) && mValueChanged != null)
            {
               mValueChanged.Invoke(Value);
            }
         }
      }

      public int DataWidth = 160;

      Brush br = new SolidBrush(Color.Black);
      Brush gbr = new SolidBrush(Color.Green);
      Pen pn = new Pen(Color.Black, 2f);
      protected override void OnPaint(PaintEventArgs e)
      {
         float normalValue = (Value - mMin) / (mMax - mMin);
         e.Graphics.DrawRectangle(pn, 0, 0, DataWidth, Height);
         e.Graphics.FillRectangle(br, 0, Height / 2, normalValue * DataWidth, Height / 2);
         e.Graphics.DrawString(mValueName, Font, br, 0, 0);
         if (mbShowValue)
            e.Graphics.DrawString(mValue.ToString(), Font, gbr, 0, Height / 2);
      }
   }

   public class ScrollButton : Button
   {
      public ScrollButton()
      {

         Init(VarName, Min, Max);
      }

      public ScrollButton(string name, float min, float max)
      {
         Init(name, min, max);
      }

      public void Init(string name, float min, float max)
      {
         mValueName = name;
         mMin = min;
         mMax = max;

         //ImageScaling = ToolStripItemImageScaling.None;
         Image = new Bitmap(60, 20); //this seems to be the best way to set the size of this control
         Anchor = AnchorStyles.None;
         this.MouseMove += new MouseEventHandler(ScrollStripItem_MouseMove);
         this.MouseUp += new MouseEventHandler(ScrollStripItem_MouseUp);
         this.MouseLeave += new EventHandler(ScrollStripItem_MouseLeave);
         this.MouseDown += new MouseEventHandler(ScrollStripItem_MouseDown);
      }
      public string VarName
      {
         set
         {
            mValueName = value;
         }
         get
         {
            return mValueName;
         }

      }

      public float Min
      {
         set
         {
            mMin = value;
         
         }
         get
         {
            return mMin;
         }

      }
      public float Max
      {
         set
         {
            mMax = value;
         }
         get
         {
            return Max;
         }


      }


      float mValue = 0.5f;
      float mMin = 0;
      float mMax = 10;
      public float Value
      {
         get
         {
            return mValue;
         }
         set
         {
            if (value >= mMin && value <= mMax)
            {
               mValue = value;
               Invalidate();
            }
         }
      }
      public delegate void ValueChanged(float val);
      public event ValueChanged mValueChanged = null;
      int X = 0;
      int Y = 0;
      public bool mbLiveUpdate = true;
      public bool mbShowValue = false;

      string mValueName = "Name";
      void ScrollStripItem_MouseUp(object sender, MouseEventArgs e)
      {
         if (mbLiveUpdate == false && mValueChanged != null)
            mValueChanged.Invoke(Value);
      }
      void ScrollStripItem_MouseLeave(object sender, EventArgs e)
      {
         if (mbLiveUpdate == false && mValueChanged != null)
            mValueChanged.Invoke(Value);
      }
      void ScrollStripItem_MouseDown(object sender, MouseEventArgs e)
      {
         mouseChange(e);
      }
      void ScrollStripItem_MouseMove(object sender, MouseEventArgs e)
      {
         mouseChange(e);
      }
      void mouseChange(MouseEventArgs e)
      {
         X = e.X;
         Y = e.Y;
         if (e.Button == MouseButtons.Left)
         {
            float v = 1 - ((Width - X) / (float)(Width));
            Value = (v * (mMax - mMin)) + mMin;
            Invalidate();
            if ((mbLiveUpdate) && mValueChanged != null)
            {
               mValueChanged.Invoke(Value);
            }
         }
      }

      Brush br = new SolidBrush(Color.Black);
      Brush gbr = new SolidBrush(Color.Green);
      Pen pn = new Pen(Color.Black, 2f);
      protected override void OnPaint(PaintEventArgs e)
      {
         float normalValue = (Value - mMin) / (mMax - mMin);
         e.Graphics.DrawRectangle(pn, 0, 0, Width, Height);
         e.Graphics.FillRectangle(br, 0, Height / 2, normalValue * Width, Height / 2);
         e.Graphics.DrawString(mValueName, Font, br, 0, 0);
         if (mbShowValue)
            e.Graphics.DrawString(mValue.ToString(), Font, gbr, 0, Height / 2);
      }
   }
}