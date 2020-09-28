using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace EditorCore
{
   public partial class ColorPickerForm : Form
   {
      public ColorPickerForm()
      {
         InitializeComponent();
         this.TopMost = true;

      }
      public ColorPickerForm(Color defaultColor)
      {
         InitializeComponent();
         this.TopMost = true;

         mDefaultColor = defaultColor;


         SelectedColor = mDefaultColor;
         this.FormClosing += new FormClosingEventHandler(ColorPickerForm_FormClosing);
      }

      void ColorPickerForm_FormClosing(object sender, FormClosingEventArgs e)
      {
         if(DialogResult != DialogResult.OK)
         {
            SelectedColor = mDefaultColor;
         }
      }
      Color mDefaultColor = Color.White;

      private void OKColorButton_Click(object sender, EventArgs e)
      {

         DialogResult = DialogResult.OK;
         this.Close();
      }

      private void CancelColorButton_Click(object sender, EventArgs e)
      {
         //undo
         SelectedColor = mDefaultColor;

         DialogResult = DialogResult.Cancel;
         this.Close();
      }
      private void colorPanel1_TopColorPickerChanged(object sender, EventArgs e)
      {
         if (SelectedColor != colorPanel1.Settings.TopColorBoxColor)
         {
            this.SelectedColor = colorPanel1.Settings.TopColorBoxColor;
         }
      }

      public event EventHandler SelectedColorChanged;
      Color mSelectedColor = Color.White;
      public Color SelectedColor
      {
         set
         {
            if (value == mSelectedColor)
               return;

            mSelectedColor = value;

            Sano.PersonalProjects.ColorPicker.Controls.ColorPanelSettings settings = colorPanel1.Settings;
            settings.TopColorBoxColor = mSelectedColor;
            settings.BottomColorBoxColor = mSelectedColor;
            colorPanel1.Settings = settings;

            if (SelectedColorChanged != null)
            {
               SelectedColorChanged(this, null);
            }
         }
         get
         {
            return mSelectedColor;
         }
      }

   }
}