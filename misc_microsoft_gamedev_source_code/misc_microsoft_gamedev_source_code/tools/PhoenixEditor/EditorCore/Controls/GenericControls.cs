using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Drawing;

namespace EditorCore
{

   public class PopupEditor
   {
      public bool mbOpen = false;
      Form mPopupEditForm = null;
      Control mHost = null;

      public Form ShowPopup(Control host, Control content)
      {
         return ShowPopup(host, content, FormBorderStyle.FixedDialog, false);
      }
      public Form ShowPopup(Control host, Control content, FormBorderStyle style)
      {
         return ShowPopup(host, content, style, false);

      }
      public Form ShowPopup(Control host, Control content, FormBorderStyle style, bool dontShowYet)
      {
         return ShowPopup(host, content, style, dontShowYet, "");
      }

      public Form ShowPopup(Control host, Control content, FormBorderStyle style, bool dontShowYet, string title)
      {
         
         mbOpen = true;
         mHost = host;
         mPopupEditForm = new Form();
         mPopupEditForm.FormBorderStyle = style;
         mPopupEditForm.Controls.Add(content);
         mPopupEditForm.Size = new Size(content.Size.Width, content.Size.Height + 35);
         content.Dock = DockStyle.Fill;
         mPopupEditForm.StartPosition = FormStartPosition.Manual;
         Point p1 = new Point(25, 25);
         Point p2 = mHost.PointToScreen(p1);

         Rectangle r = System.Windows.Forms.Screen.PrimaryScreen.Bounds;

         int ymax = content.Height +100;
         int xmax = content.Width + 100;
         if (r.Height < p2.Y + ymax)
         {
            p2.Y = r.Height - ymax;
         }
         if (r.Width < p2.X + xmax)
         {
            p2.X = r.Width - xmax;
         }
         if (p2.Y < 0)
            p2.Y = 0;
         if (p2.X < 0)
            p2.X = 0;

         mPopupEditForm.Location = p2;
         mPopupEditForm.Deactivate += new EventHandler(mPopupEditForm_Deactivate);
         mPopupEditForm.FormClosing += new FormClosingEventHandler(mPopupEditForm_FormClosing);

         mPopupEditForm.Text = title;

         if (!dontShowYet)
         {
            mPopupEditForm.Show(mHost);
         }
         return mPopupEditForm;
      }

      void mPopupEditForm_FormClosing(object sender, FormClosingEventArgs e)
      {
         mbOpen = false;
      }
      void mPopupEditForm_Deactivate(object sender, EventArgs e)
      {
         if (FormBorderStyle.None == mPopupEditForm.FormBorderStyle)
         {
            mbOpen = false;
            ((Form)sender).Close();
         }
      }
   }



   public class EnumViewer<T> : Control where T : IConvertible
   {
      public ImageList mImageList = null;
      public ImageList ImageList
      {
         get
         {
            return mImageList;
         }
         set
         {
            mImageList = value;
         }
      }

      T mValue;
      public T Value
      {
         get
         {
            return mValue;
         }
         set
         {
            mValue = value;

            if (ImageList != null)
            {
               int enumValue = System.Convert.ToInt32(mValue);
               BackgroundImage = ImageList.Images[enumValue];
               this.BackgroundImageLayout = ImageLayout.Center;
            }
         }
      }
   }

}
