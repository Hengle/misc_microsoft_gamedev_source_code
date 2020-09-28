using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml.Serialization;
using System.IO;

namespace pxdb
{
   //==============================================================================
   // MainForm
   //==============================================================================
   public partial class MainForm : Form
   {
      private string mDataType = @"";
      private string mGroupType = @"";
      private string mDetailType = @"";

      //---------------------------------------------------------------------------
      // Constructor
      //---------------------------------------------------------------------------
      public MainForm()
      {
         InitializeComponent();
         Database.setupDataTypeCombo(dataTypeCombo);
         if (dataTypeCombo.Items.Count > 0)
            dataTypeCombo.SelectedIndex = 0;
      }

      //---------------------------------------------------------------------------
      // dataTypeCombo_SelectedIndexChanged
      //---------------------------------------------------------------------------
      private void dataTypeCombo_SelectedIndexChanged(object sender, EventArgs e)
      {
         if(dataTypeCombo.Text != mDataType)
         {
            mDataType = dataTypeCombo.Text;

            mGroupType = @"";
            groupList.Items.Clear();
            mainBindingSource.DataSource = null;
            mainGrid.DataSource = null;
            mainGrid.Columns.Clear();

            mDetailType = @"";
            detailList.Items.Clear();
            detailBindingSource.DataSource = null;
            detailBindingSource.DataMember = "";
            detailGrid.DataSource = null;
            detailGrid.Columns.Clear();

            Database.setupGroupList(groupList, mDataType);
            if (groupList.Items.Count > 0)
               groupList.SelectedIndex = 0;
         }
      }

      //---------------------------------------------------------------------------
      // groupList_SelectedIndexChanged
      //---------------------------------------------------------------------------
      private void grouplList_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (groupList.Text != mGroupType)
         {
            int currentRow = -1;
            int firstDisplayedRow = -1;
            if (mainGrid.CurrentRow != null)
            {
               currentRow = mainGrid.CurrentRow.Index;
               firstDisplayedRow = mainGrid.FirstDisplayedScrollingRowIndex;
            }

            mGroupType = groupList.Text;
            mainBindingSource.DataSource = null;
            mainGrid.DataSource = null;
            mainGrid.Columns.Clear();

            mDetailType = @"";
            detailBindingSource.DataSource = null;
            detailBindingSource.DataMember = "";
            detailGrid.DataSource = null;
            detailGrid.Columns.Clear();
            detailList.Items.Clear();

            if (mDataType != @"")
            {
               Database.setupGridView(mainGrid, mDataType, "", mGroupType);
               mainBindingSource.DataSource = Database.getDataSource(mDataType);
               mainGrid.DataSource = mainBindingSource;

               if (currentRow != -1 && mainGrid.Columns.Count > 0 && mainGrid.Rows.Count > currentRow)
               {
                  mainGrid.FirstDisplayedScrollingRowIndex = firstDisplayedRow;
                  mainGrid.Refresh();
                  if (mainGrid.Rows[currentRow].Cells[0].Visible)
                     mainGrid.CurrentCell = mainGrid.Rows[currentRow].Cells[0];
               }

               Database.setupDetailList(detailList, mDataType, mGroupType);
               if (detailList.Items.Count > 0)
                  detailList.SelectedIndex = 0;

               if (mainGrid.Columns.Count > 0)
                  mainGrid.Select();
            }
         }
      }

      //---------------------------------------------------------------------------
      // detailList_SelectedIndexChanged
      //---------------------------------------------------------------------------
      private void detailList_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (detailList.Text != mDetailType)
         {
            mDetailType = detailList.Text;

            detailBindingSource.DataSource = null;
            detailBindingSource.DataMember = "";
            detailGrid.DataSource = null;

            if (mDataType != @"" && mGroupType != @"")
            {
               Database.setupGridView(detailGrid, mDataType, mDetailType, mGroupType);
               detailBindingSource.DataSource = mainBindingSource;
               detailBindingSource.DataMember = mDetailType;
               detailGrid.DataSource = detailBindingSource;

               if (detailGrid.Columns.Count > 0)
                  detailGrid.Select();
            }
         }
      }

      //---------------------------------------------------------------------------
      // detailGrid_DataError
      //---------------------------------------------------------------------------
      private void detailGrid_DataError(object sender, DataGridViewDataErrorEventArgs e)
      {
         if (e.Context == (DataGridViewDataErrorContexts.Formatting | DataGridViewDataErrorContexts.PreferredSize))
            e.Cancel = true;
         else
         {
            e.Cancel = true;
         }
      }
   }
}