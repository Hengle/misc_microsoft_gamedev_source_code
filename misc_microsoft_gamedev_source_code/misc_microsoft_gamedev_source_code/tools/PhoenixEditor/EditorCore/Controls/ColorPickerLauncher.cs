using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore
{
   public partial class ColorPickerLauncher : UserControl
   {
      public ColorPickerLauncher()
      {
         InitializeComponent();

         this.PickColorButton.AllowDrop = true;
      }

      string mPropertyName = "Pick a Color";
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public string ColorPropertyName
      {
         set
         {
            mPropertyName = value;
         }
         get
         {
            return mPropertyName;
         }

      }



      Color mSelectedColor = Color.White;

      //[Browsable(false)]
      //[DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public Color SelectedColor
      {
         set
         {
            mSelectedColor = value;
            this.PickColorButton.BackColor = mSelectedColor;
            this.DescriptionLabel.Text = string.Format("{0},{1},{2}", mSelectedColor.R, mSelectedColor.G, mSelectedColor.B);
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

      public event EventHandler SelectedColorChanged;

      bool mbOpen = false;
      Form mPopupEditForm = null;
      Sano.PersonalProjects.ColorPicker.Controls.ColorPanel mColorPanel = null;
      bool mModlessPopup = false;

      
      private void PickColorButton_Click(object sender, EventArgs e)
      {
         if (mModlessPopup == true)
         {

            if (mbOpen == false)
            {
               mColorPanel = new Sano.PersonalProjects.ColorPicker.Controls.ColorPanel();

               //haha we have to do this in a funny order due to a bug in colorpicker settings.
               Sano.PersonalProjects.ColorPicker.Controls.ColorPanelSettings settings = mColorPanel.Settings;
               settings.TopColorBoxColor = mSelectedColor;
               settings.BottomColorBoxColor = mSelectedColor;
               mColorPanel.Settings = settings;

               mColorPanel.TopColorPickerChanged += new EventHandler(mColorPanel_TopColorPickerChanged);

               PopupEditor(mColorPanel);
            }
            else
            {
                
               //closes magically
            }
         }
         else 
         {
            if(mbOpen == false)
            {
               LaunchColorPickerForm();
            }


         }
      }

      public void LaunchColorPickerForm()
      {

         mbOpen = true;
         mColorPickerForm = new ColorPickerForm(this.SelectedColor);


         mColorPickerForm.StartPosition = FormStartPosition.Manual;
         Point p1 = new Point(25, -175);
         Point p2 = this.PointToScreen(p1);
         Rectangle r = System.Windows.Forms.Screen.PrimaryScreen.Bounds;
         if (r.Height < p2.Y + mColorPickerForm.Height)
         {
            p2.Y = r.Height - mColorPickerForm.Height;
         }
         if (r.Width < p2.X + mColorPickerForm.Width)
         {
            p2.X = r.Width - mColorPickerForm.Width;
         }
         mColorPickerForm.Location = p2;

         mColorPickerForm.FormClosed += new FormClosedEventHandler(mColorPickerForm_FormClosed);
         mColorPickerForm.SelectedColorChanged += new EventHandler(mColorPickerForm_SelectedColorChanged);

         mColorPickerForm.Text = ColorPropertyName;
         mColorPickerForm.Show(this);
      }

      void mColorPickerForm_SelectedColorChanged(object sender, EventArgs e)
      {
         this.SelectedColor = mColorPickerForm.SelectedColor;
         //throw new Exception("The method or operation is not implemented.");
      }

      void mColorPickerForm_FormClosed(object sender, FormClosedEventArgs e)
      {
         mbOpen = false;

         //if (((Form)sender).DialogResult == DialogResult.OK)
         //{
         //}
         //else
         //{
         //   mBackupVar.CopyTo(mVariable);
         //   mBackupValue.CopyTo(mValue);
         //}
         //UpdateControl();
      }

      ColorPickerForm mColorPickerForm = null;



      void mColorPanel_TopColorPickerChanged(object sender, EventArgs e)
      {
         SafeUpdateColor();
      }

      private void SafeUpdateColor()
      {
         if (SelectedColor != mColorPanel.Settings.TopColorBoxColor)
         {
            this.SelectedColor = mColorPanel.Settings.TopColorBoxColor;
         }
      }

      private void PopupEditor(Control c)
      {
         mbOpen = true;
         mPopupEditForm = new Form();
         mPopupEditForm.FormBorderStyle = FormBorderStyle.None;
         mPopupEditForm.Controls.Add(c);
         mPopupEditForm.Size = c.Size;
         
         //mPopupEditForm.StartPosition = FormStartPosition.
         mPopupEditForm.StartPosition = FormStartPosition.Manual;
         Point p1 = new Point(25, 25);
         Point p2 = this.PointToScreen(p1);
 
         Rectangle r = System.Windows.Forms.Screen.PrimaryScreen.Bounds;

         if (r.Height < p2.Y + 400)
         {
            p2.Y = r.Height - 400;
         }
         if (r.Width < p2.X + 800)
         {
            p2.X = r.Width - 800;
         }
         mPopupEditForm.Location = p2;
         mPopupEditForm.Deactivate += new EventHandler(mPopupEditForm_Deactivate);
         mPopupEditForm.Show(this);        
      }
      void mPopupEditForm_Deactivate(object sender, EventArgs e)
      {
         mbOpen = false;
         ((Form)sender).Close();
      }

      private void PickColorButton_MouseMove(object sender, MouseEventArgs e)
      {
         if(e.Button == MouseButtons.Left)
         {
            this.DoDragDrop(this.SelectedColor.ToArgb(), DragDropEffects.All);

         }
      }

      private void PickColorButton_DragEnter(object sender, DragEventArgs e)
      {

      }

      private void PickColorButton_DragDrop(object sender, DragEventArgs e)
      {
         
         object o = e.Data.GetData(typeof(int));
         if(o is int)
         {
            SelectedColor = Color.FromArgb((int)o);
         }
      }

      private void PickColorButton_DragOver(object sender, DragEventArgs e)
      {
         e.Effect = DragDropEffects.Copy;
      }
   }
}
