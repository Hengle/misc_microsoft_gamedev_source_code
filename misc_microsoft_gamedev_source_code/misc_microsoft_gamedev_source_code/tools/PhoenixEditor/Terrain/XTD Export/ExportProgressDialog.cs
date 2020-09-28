using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Export360
{
   public partial class ExportProgressDialog : Form
   {
      public ExportProgressDialog()
      {
         InitializeComponent();
         try
         {
            mThis = this;
            this.StartPosition = FormStartPosition.CenterScreen;
            
         }
         catch (System.Exception ex)
         {
            ex.ToString();
         }
      }

      public void setName(String name)
      {
         //nameLabel.Text = name;
      }
      static ExportProgressDialog mThis = null;
      public void Init(int max)
      {
         progressBar1.Minimum = 0;// min;
         progressBar1.Maximum = max;
         if(updateProgress==null)
            updateProgress = new _ProgressInc(ProgressInc);
      }

      static public void Increase()
      {
         if (updateProgress!=null)
            mThis.progressBar1.Invoke(updateProgress);
      }

      static public void ProgressInc()
      {
         if (mThis.progressBar1.Value + 1 <= mThis.progressBar1.Maximum)
            mThis.progressBar1.Value++;
         if (mThis.progressBar1.Value >= mThis.progressBar1.Maximum)
         {
            mThis.Close();
            updateProgress = null;
         }
      }
      public delegate void _ProgressInc();
      static public _ProgressInc updateProgress = new _ProgressInc(ProgressInc);
   }
}