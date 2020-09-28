using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class HardPointsBar : UserControl
   {
      public HardPointsBar()
      {
         InitializeComponent();

         InitialSetup();
         //TestLoad();

         //tableLayoutPanel1.CellBorderStyle = TableLayoutPanelCellBorderStyle.Single;
      }

      bool mHorizontalLayout = true;

      public bool HorizontalLayout
      {
         set
         {
            mHorizontalLayout = value;

            ClearSetup();
            InitialSetup();

         }
         get
         {
            return mHorizontalLayout;
         }

      }
      public int MarginSize
      {
         set
         {
            mMargin = value;
         }
         get
         {
            return mMargin;
         }
      }

      public int mMargin = 25;

      public void InitialSetup()
      {
         ClearSetup();
         LinearInsertStyle(0, SizeType.Absolute, mMargin);
         //LinearInsertStyle(1, SizeType.AutoSize, 0);
         //LinearInsertStyle(2, SizeType.AutoSize, 0);
         LinearInsertStyle(1, SizeType.Absolute, mMargin);
      }

      private void ClearSetup()
      {
         tableLayoutPanel1.RowCount = 1;
         tableLayoutPanel1.ColumnCount = 1;
         tableLayoutPanel1.RowStyles.Clear();
         tableLayoutPanel1.ColumnStyles.Clear();
         mControls.Clear();
      }

      public void RemoveAll()
      {
         tableLayoutPanel1.Controls.Clear();
         ClearSetup();

      }

      public void LinearInsertStyle(int slotID, SizeType type, float amount)
      {
         if(mHorizontalLayout == true)
         {
            tableLayoutPanel1.ColumnCount++;
            tableLayoutPanel1.ColumnStyles.Insert(slotID, new ColumnStyle(type, amount));
         }
         else         
         {
            tableLayoutPanel1.RowCount++;
            tableLayoutPanel1.RowStyles.Insert(slotID, new RowStyle(type, amount));
         }
      }

      List<Control> mControls = new List<Control>();

      public ICollection<Control> GetLogicalControls() { return mControls; }

      public void AddControl(Control c)
      {
         if (mHorizontalLayout)
         {
            c.Margin = new Padding(3,0,3,0);
         }
         else
         {
            c.Margin = new Padding(0,3,0,3);
         }

         int offset = mControls.Count * 1;
         int controlPos = /*2 +*/ offset;
         //LinearInsertStyle(controlPos, SizeType.Percent, 20);
         LinearInsertStyle(controlPos, SizeType.AutoSize, 0);
         //LinearInsertStyle(controlPos + 1, SizeType.AutoSize, 0);
         if(mHorizontalLayout)
         {
            tableLayoutPanel1.Controls.Add(c, controlPos, 0);
         }
         else
         {
            tableLayoutPanel1.Controls.Add(c, 0, controlPos);

         }

         mControls.Add(c);
      }

      public void TestLoad()
      {


         tableLayoutPanel1.DoubleClick += new EventHandler(tableLayoutPanel1_DoubleClick);
      }

      void tableLayoutPanel1_DoubleClick(object sender, EventArgs e)
      {
         Label l = new Label();
         l.Text = "*new";
         l.BorderStyle = BorderStyle.FixedSingle;
         l.AutoSize = true;

         AddControl(l);         //throw new Exception("The method or operation is not implemented.");
      }
   }
}
