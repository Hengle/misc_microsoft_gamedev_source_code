using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using EditorCore.Controls;
using EditorCore;

namespace PhoenixEditor
{
   public partial class LocStringDlg : Form
   {
      public LocStringDlg()
      {
         InitializeComponent();
      }

      public string LocStringID
      {
         get 
         {
            LocString locString = mLocStringPicker.LocString;
            if (locString != null)
            {
               return locString.mLocIDAsString;
            }

            return null;
         }
      }
   }
}