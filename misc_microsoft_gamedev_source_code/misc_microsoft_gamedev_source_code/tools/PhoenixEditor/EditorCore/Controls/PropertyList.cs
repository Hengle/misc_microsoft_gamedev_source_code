using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls
{
   public partial class PropertyList : ObjectEditorControl
   {
      public PropertyList()
      {
         InitializeComponent();

         this.AutoScroll = true;
      }


      override protected void ClearUI()
      {
         //mTableControlCount = 0;
         //this.SuspendLayout();
         //MainTableLayoutPanel.Visible = false;
         //MainTableLayoutPanel.SuspendLayout();
         //MainTableLayoutPanel.Controls.Clear();
         //MainTableLayoutPanel.RowStyles.Clear();
         //MainTableLayoutPanel.ResumeLayout();
         //MainTableLayoutPanel.Visible = true;
         //this.ResumeLayout();

         this.HorizontalScroll.Value = 0;
         this.VerticalScroll.Value = 0;

         mContentControls.Clear();
         this.Controls.Clear();
      }
      override protected void AddControls(List<Control> controls)
      {
         //mTableControlCount = 0;
         //this.SuspendLayout();
         //MainTableLayoutPanel.Visible = false;
         //MainTableLayoutPanel.SuspendLayout();
         //MainTableLayoutPanel.Controls.Clear();
         //MainTableLayoutPanel.RowStyles.Clear();
         //foreach (Control c in controls)
         //{
         //   AddRow(c.Name, c);
         //}
         //if (mbLastRowHack)
         //{
         //   AddBlankRow();
         //}
         //MainTableLayoutPanel.ResumeLayout();
         //MainTableLayoutPanel.Visible = true;
         //this.ResumeLayout();
         this.SuspendLayout();
         ClearUI();
         foreach (Control c in controls)
         {
            c.SuspendLayout();
            c.Visible = false;
            AddRow(c.Name, c);
         }
         foreach (Control c in controls)
         {
            
            c.ResumeLayout();
            c.Visible = true;
         }
         this.ResumeLayout();

      }

      //int mTableControlCount = 0;
      List<Control> mContentControls = new List<Control>();
      private void AddRow(string name, Control c)
      {
         //MainTableLayoutPanel.RowStyles.Add(new RowStyle(SizeType.AutoSize));
         //Label l = new Label();
         //l.Text = name;
         //l.AutoSize = true;
         //l.Name = name + "Label";
         //l.Margin = new Padding(0);
         //c.Margin = new Padding(0);
         //c.Dock = DockStyle.Fill;
         //mTableControlCount++;
         //MainTableLayoutPanel.Controls.Add(l, 0, mTableControlCount);
         //MainTableLayoutPanel.Controls.Add(c, 1, mTableControlCount);

         int leftmargin = 100;
         int rightemptyspace = 20;
         

         c.Margin = new Padding(0);

         if(mContentControls.Count == 0)
         {    
            //this.scrol   //set scrolling...
            this.Controls.Add(c);
            this.mContentControls.Add(c);

            c.Top = 0;
            c.Left = leftmargin;
            c.Width = this.Width + -leftmargin + -rightemptyspace;
         }
         else
         {
            Control lastControl = this.mContentControls[ this.mContentControls.Count - 1];

            this.Controls.Add(c);
            this.mContentControls.Add(c);

            c.Top = lastControl.Bottom + 5;
            c.Left = leftmargin;
            c.Width = this.Width + -leftmargin + -rightemptyspace;

         }



         //LinkLabel l = new LinkLabel();
         Label l = new Label();
         l.Text = name;
         l.AutoSize = true;
         l.Name = name + "Label";
         l.Margin = new Padding(0);

         this.Controls.Add(l);
         l.Top = c.Top;
         l.Left = 0;


      }

   }


   
}
