using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using Terrain;

namespace PhoenixEditor
{
   public partial class ImageSourcePicker : UserControl
   {
      public ImageSourcePicker()
      {
         InitializeComponent();
      }

      AbstractImage mSelectedImage = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public AbstractImage AbstractImage
      {
         set
         {
            mSelectedImage = value;
            //this.Parent.c
            if (ImageSelected != null)
               ImageSelected.Invoke(this, null);
         }
         get
         {
            return mSelectedImage;
         }         
      }

      public event EventHandler ImageSelected;

      private void FractalButton_Click(object sender, EventArgs e)
      {
         this.AbstractImage = new FractalImage();
         
      }

      private void tabPage1_Click(object sender, EventArgs e)
      {

      }

      private void PickImageButton_Click(object sender, EventArgs e)
      {
         MasklImage maskimage = new MasklImage();
         maskimage.Load();
         AbstractImage = maskimage;
      }

      private void SolidButton_Click(object sender, EventArgs e)
      {
         AbstractImage = new AbstractImage();

      }

      private void GradientButton_Click(object sender, EventArgs e)
      {
         AbstractImage = new LinearGradientImage();

      }

      //



   }
}
