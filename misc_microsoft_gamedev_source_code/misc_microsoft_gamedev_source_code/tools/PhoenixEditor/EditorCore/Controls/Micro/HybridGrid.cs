using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls.Micro
{
   public partial class HybridGrid : UserControl
   {
      public HybridGrid()
      {
         InitializeComponent();

         gridControl1.MouseClick += new MouseEventHandler(gridControl1_MouseClick);

         //Random r = new Random();
         //int[][] data = new int[32][];
         //for (int i = 0; i < 32; i++)
         //{
         //   data[i] = new int[5];
         //   for (int j = 0; j < 5; j++)
         //   {
         //      data[i][j] = r.Next();
         //   }
         //}
         //gridControl1.DataSource = data;         

         gridControl1.ReadOnly = true;

         //gridControl1.SelectedRows
      }
      public void SetData(object data)
      {
         gridControl1.DataSource = data;                   
      }
      public void SetSelectedRowIndex(int index)
      {
         gridControl1.SelectedRows.Clear();
         if (index == -1)
            return;
         gridControl1.SelectedRows.Add(gridControl1.DataRows[index]);
      }
      public int GetSelectedRowIndex()
      {
         if (gridControl1.SelectedRows.Count > 0)
         {
            Xceed.Grid.DataRow dr = gridControl1.SelectedRows[0] as Xceed.Grid.DataRow;
            if (dr != null)
            {
               return dr.Index;
            }
         }

         return -1;
      }

      public ObjectEditorControl mObjectEditor = null;

      public delegate void CellInfo(object sourceObject, string field, int row, int column);
      public event CellInfo mCellClicked;

      void gridControl1_MouseClick(object sender, MouseEventArgs e)
      {
         if (e.Button != MouseButtons.Left)
            return;

         Point p = new Point(e.X, e.Y);
         object o2 = gridControl1.GetVisualGridElementAtPoint(p);
         Xceed.Grid.DataCell cell = o2 as Xceed.Grid.DataCell;
         if (cell != null)
         {
            //cell.Value;
            int col = cell.ParentColumn.Index;
            Xceed.Grid.DataBoundColumn dataCol = cell.ParentColumn as Xceed.Grid.DataBoundColumn;
            Xceed.Grid.DataRow dataRow = cell.ParentRow as Xceed.Grid.DataRow;
            if (dataCol != null && dataRow != null)
            {
               object sourceObject = dataRow.SourceObject;
               int rowNum = dataRow.Index;
               string field = dataCol.FieldName;

               if(mCellClicked != null)
               {
                  mCellClicked.Invoke(sourceObject, field, rowNum, col);
               }

               if (mObjectEditor != null)
               {
                  Control c = mObjectEditor.GetSingleControl(sourceObject, field);
                  PopupEditor pe = new PopupEditor();
                  Form f = pe.ShowPopup(this, c, FormBorderStyle.FixedSingle, true);
                  Point p2 = this.PointToScreen(p);
                  f.Deactivate += new EventHandler(f_Deactivate);
                  f.Tag = cell;
                  f.Top = p2.Y;
                  f.Left = p2.X;
                  f.Show();

                  //if (ContentMessageBox.ShowMessage(this, c, "") == DialogResult.OK)
                  //{
                  //   //this.LocStringID = p.LocStringID;
                  //}

               }
            }
         }
      }

      void f_Deactivate(object sender, EventArgs e)
      {
         Form f = sender as Form;
         Xceed.Grid.DataCell cell = f.Tag as Xceed.Grid.DataCell;
         if (cell != null)
         {
            cell.BackColor = Color.LightCoral;
         }
         //throw new Exception("The method or operation is not implemented.");
      }
   }
}
