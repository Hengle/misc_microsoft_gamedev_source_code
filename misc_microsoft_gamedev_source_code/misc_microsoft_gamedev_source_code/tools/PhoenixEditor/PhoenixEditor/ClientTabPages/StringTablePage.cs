using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.IO;

using Xceed.Grid;
using Xceed.Grid.Editors;
using Xceed.Editors;

using EditorCore;

namespace PhoenixEditor.ClientTabPages
{
   public partial class StringTablePage : EditorCore.BaseClientPage
   {

      private bool mEditMode = false;
      private bool m_filtered = false;
      private ArrayList m_foundRows = new ArrayList();

      private XmlDataDocument mDoc;

      PerforceSimple mP4;

      private string mStringTableFilename;

      int mCurrentSearchRow = 0;

      /// <summary>
      /// 
      /// </summary>
      public StringTablePage()
      {
         InitializeComponent();

         mStringTableFilename = "stringTable.xml";

         mP4 = new PerforceSimple(false);

         // Make sure we have the latest when we start
/*
         string strTablePath = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, mStringTableFilename);
         makeReadOnly(strTablePath);
         mP4.Sync(strTablePath);
*/
         mEditMode = false;
         m_filtered = false;

         LoadStringtable();

         MakeEditable(false);

         CheckEnableControls();
      }

      /// <summary>
      /// 
      /// </summary>
      private void ClearStringTable()
      {
         ClearSearch();

         gridControl1.DataRows.Clear();
      }

      /// <summary>
      /// Loads the stringtable into memory from the game work directory
      /// </summary>
      private void LoadStringtable()
      {
         gridControl1.DataRowTemplate.AutoHeightMode = AutoHeightMode.AllContent;
         gridControl1.WordWrap = true;

         mDoc = new XmlDataDocument();
         // mDoc.PreserveWhitespace = true;

         string strTablePath = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, mStringTableFilename);
         string strTableXsdPath = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "stringtable.xsd");

         // these need to be pulled from the data directory
         mDoc.DataSet.ReadXmlSchema(strTableXsdPath);
         mDoc.Load(strTablePath);

         gridControl1.DataSource = mDoc.DataSet;
         gridControl1.DataMember = "String";

         System.Data.DataTable table = mDoc.DataSet.Tables[2];

         DataColumn[] PrimaryKeyColumns = new DataColumn[1];
         PrimaryKeyColumns[0] = table.Columns["_locID"];
         table.PrimaryKey = PrimaryKeyColumns;

         Xceed.Grid.Column locColumn = gridControl1.Columns["_locID"];
         if (locColumn != null)
            locColumn.ReadOnly = true;

         Xceed.Grid.Column c = gridControl1.Columns["Language_Id"];
         if (c != null)
            c.Visible = false;

         c = gridControl1.Columns["category"];
         if (c != null)
         {
            // c.Visible = false;
            ComboBoxEditor combo = new ComboBoxEditor();

            combo.Items.Add( new ComboBoxItem( new object[1]{""} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"Code"} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"Techs"} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"Squads"} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"Powers"} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"Abilities"} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"Leaders"} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"Objects"} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"UI"} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"Campaign"} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"Cinematics"} ) );
            combo.Items.Add( new ComboBoxItem( new object[1]{"Skirmish"} ) );

            c.CellEditorManager = combo;
         }
         mDoc.DataSet.AcceptChanges();

//         ResizeColumns();
      }

      /// <summary>
      /// Resize the columns to better fit the grid control
      /// </summary>
      private void ResizeColumns()
      {

         Xceed.Grid.Column locColumn = gridControl1.Columns["_locID"];
         Xceed.Grid.Column categoryColumn = gridControl1.Columns["category"];
         Xceed.Grid.Column scenarioColumn = gridControl1.Columns["scenario"];
         Xceed.Grid.Column subtitleColumn = gridControl1.Columns["subtitle"];

         Xceed.Grid.Column strColumn = gridControl1.Columns["String_text"];
         if ( (locColumn != null) && (categoryColumn != null) && (scenarioColumn != null) && (subtitleColumn != null) && (strColumn != null))
         {
            int w = gridControl1.Width - locColumn.Width - categoryColumn.Width - scenarioColumn.Width - subtitleColumn.Width - 40;
            if (w > 0)
               strColumn.Width = w;
         }
      }

      /// <summary>
      /// If the grid control changes, we need to resize our columns again.
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="e"></param>
      private void gridControl1_SizeChanged(object sender, EventArgs e)
      {
         ResizeColumns();
      }

      private int GetNextLocID()
      {

         int locID = 0;

         string controlFile = null;
         string errorMessage = "";
         try
         {
            string filename = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "stringtable.control");
            mP4.Sync(filename);

            // Check out the control file
            if (!mP4.Checkout(filename))
            {
               errorMessage = "Could not check out dbid.control: \n\n" + mP4.LastError;
               throw new Exception("Error");
            }

            // now the exception handler knows to revert the file if we get an error.
            controlFile = filename;

            if (!File.Exists(controlFile))
            {
               errorMessage = "The DBID control file cannot be found: " + controlFile;
               throw new Exception("Error");
            }

            // lock the control file
            if (!mP4.Lock(controlFile))
            {
               errorMessage = "Could not lock the control file: \n\n" + mP4.LastError;
               throw new Exception("Error");
            }

            // read the control file.
            XmlDocument controlDoc = new XmlDocument();
            try
            {
               controlDoc.Load(controlFile);
            }
            catch (Exception)
            {
               errorMessage = "The control file could not be loaded: \n\n" + controlFile;
               throw new Exception("Error");
            }

            XmlNode root = controlDoc.SelectSingleNode("/StringtableControlNumber");
            if (root == null)
            {
               errorMessage = "The root node could not be found on the control file: /StringtableControlNumber";
               throw new Exception("Error");
            }

            XmlAttribute rootAttr = root.Attributes["nextLocID"];
            if (rootAttr == null)
            {
               errorMessage = "The control file does not contain the attribute: nextLocID";
               throw new Exception("Error");
            }

            try
            {
               int nextLocID = int.Parse(rootAttr.Value);
               locID = nextLocID;
               nextLocID++;

               // we have modified the DBID 
               // update the root next DBID
               rootAttr.Value = nextLocID.ToString();

               // save our control file
               controlDoc.Save(controlFile);

               // submit our control file.
               mP4.Checkin(controlFile, "Next String ID is " + nextLocID);

            }
            catch (Exception e)
            {
               errorMessage = e.Message;
               throw new Exception("Error");

            }

         }
         catch (System.Exception e)
         {
            if (controlFile != null)
               mP4.Revert(controlFile);

            // do we have an error message to display?
            if (errorMessage.Length > 0)
               MessageBox.Show(this, errorMessage, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);

            return 0;
         }

         return locID;
      }

      /// <summary>
      /// Add a new string into the string table.
      /// </summary>
      private void AddString()
      {
         int locID = GetNextLocID();

         if (locID == 0)
         {
            MessageBox.Show(this, "Oops. Could not get the next string ID.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
         }

         Xceed.Grid.DataRow row = gridControl1.DataRows.AddNew();
         row.BeginEdit();

         Xceed.Grid.Cell cell = row.Cells["_locID"];
         cell.Value = locID;

         cell = row.Cells["String_text"];
         cell.Value = "Insert Text Here";

         cell = row.Cells["Language_Id"];
         cell.Value = "0";

         cell = row.Cells["subtitle"];
         cell.Value = false;

         row.EndEdit();

         // If we have the filter enabled, we are going to add the string to group so that it shows up
         // in the view to avoid the "Hey, where's my string I just added?" issues that surely will arise
         if (m_filtered)
         {
            // Make sure that each parent group of the found data row are expanded.
            if ((row.ParentGroup != null) && row.ParentGroup.Collapsed)
            {
               Group parentGroup = row.ParentGroup as Group;

               while (parentGroup != null)
               {
                  parentGroup.Expand();
                  parentGroup = parentGroup.ParentGroup as Group;
               }
            }

            // Highlight the found data row by changing its back color
//         row.BackColor = Color.AliceBlue;
            // Highlight the found data row's parent groups by changing their
            // GroupManagerRow's back color.
            this.TagGroups(row);
            m_foundRows.Add(row);

         }

         gridControl1.UpdateGrouping();

         gridControl1.Scroll(Xceed.Grid.ScrollDirection.BottomPage);

         gridControl1.SelectedRows.Clear();
         gridControl1.SelectedRows.Add(row);
         gridControl1.CurrentRow = row;


      }

      /// <summary>
      /// The user wants a new string.
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="e"></param>
      private void mBtnNewString_Click(object sender, EventArgs e)
      {
         AddString();
      }

      private void Search()
      {
         mCurrentSearchRow = 0;

         // Reset the back color of each row that was found in the previous search, if any.
         foreach (Xceed.Grid.DataRow dataRow in m_foundRows)
         {
            dataRow.ResetBackColor();

            // We must not forget the parent groups of the data row for which their 
            // GroupManagerRow's BackColor has been modified.
            Group parentGroup = dataRow.ParentGroup as Group;

            while (parentGroup != null)
            {
               parentGroup.Collapse();
               parentGroup.HeaderRows[0].ResetBackColor();
               parentGroup = parentGroup.ParentGroup as Group;
            }
         }

         m_foundRows.Clear();
         bool caseSensitive = mChkCaseSensitive.Checked;

         if (this.mtxtbSearch.Text.Length > 0)
         {
            string searchString = null;
            if (caseSensitive)
               searchString = this.mtxtbSearch.Text;
            else
               searchString = this.mtxtbSearch.Text.ToUpper();

            bool currentRow = false;
            // Iterates each data rows of the grid and check if the searched string applies to it.
            foreach (Xceed.Grid.DataRow dataRow in gridControl1.DataRows)
            {
               // We compare the specified string with the GetDisplayText() of the Cell to 
               // make sure that the search will apply on what's displayed in the grid 
               // (can make a difference with date values for instance).

               string cellData = null;
               if (caseSensitive)
                  cellData = dataRow.Cells["String_text"].GetDisplayText();
               else
                  cellData = dataRow.Cells["String_text"].GetDisplayText().ToUpper();

               if (cellData.Contains(searchString))
               {
                  // Make sure that each parent group of the found data row are expanded.
                  if ((dataRow.ParentGroup != null) && dataRow.ParentGroup.Collapsed)
                  {
                     Group parentGroup = dataRow.ParentGroup as Group;

                     while (parentGroup != null)
                     {
                        parentGroup.Expand();
                        parentGroup = parentGroup.ParentGroup as Group;
                     }
                  }

                  if (!currentRow)
                  {
                     gridControl1.FirstVisibleRow = dataRow;
                     currentRow = true;
                  }


                  // Highlight the found data row by changing its back color
//                  dataRow.BackColor = Color.AliceBlue;
                  dataRow.BackColor = Color.LightSteelBlue;
                  // Highlight the found data row's parent groups by changing their
                  // GroupManagerRow's back color.
                  this.TagGroups(dataRow);
                  m_foundRows.Add(dataRow);
               }
            }

            // Add a text row in the grid fixed footers displaying the number of rows found.
            TextRow searchResultsRow = new TextRow(m_foundRows.Count.ToString() + " item" + ((m_foundRows.Count == 1) ? "" : "s") + " found.");
            searchResultsRow.Font = new Font("Verdana", 9, FontStyle.Bold | FontStyle.Italic);
            searchResultsRow.Height = searchResultsRow.Height + 10;
            searchResultsRow.ForeColor = Color.AliceBlue;
            searchResultsRow.BackColor = Color.SteelBlue;
            searchResultsRow.CanBeSelected = false;
            searchResultsRow.CanBeCurrent = false;
            searchResultsRow.VerticalAlignment = VerticalAlignment.Center;

            gridControl1.FixedFooterRows.Clear();
            gridControl1.FixedFooterRows.Add(searchResultsRow);

            if (m_foundRows.Count > 0)
               this.btnFilter.Enabled = true;
         }
         else
         {
            gridControl1.FixedFooterRows.Clear();
            this.btnFilter.Enabled = false;
         }

         gridControl1.Focus();
      }

      /// <summary>
      /// Search the strings for matching criteria
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="e"></param>
      private void mBtnSearch_Click(object sender, EventArgs e)
      {
         Search();
      }

      /// <summary>
      /// TagGroups
      /// </summary>
      /// <param name="foundDataRow"></param>
      private void TagGroups(Xceed.Grid.DataRow foundDataRow)
      {
         // Make sure that each foundDataRow's parent groups have their GroupManagerRow's
         // BackColor set to the chosen highlight color.
         Group parentGroup = foundDataRow.ParentGroup as Group;
         Color color = Color.LightSteelBlue;

         while (parentGroup != null)
         {
            if (parentGroup.HeaderRows.Count > 0)
            {
               if (parentGroup.HeaderRows[0].BackColor != color)
               {
                  parentGroup.HeaderRows[0].BackColor = color;
               }
               else
               {
                  // The group is already tagged. That means that all its parent groups are
                  // tagged too. We exit immediately.
                  break;
               }
            }

            parentGroup = parentGroup.ParentGroup as Group;
         }
      }

      private void SetNextSearchRow()
      {
         if (m_foundRows.Count<=0)
            return;

         mCurrentSearchRow++;

         if (mCurrentSearchRow >= m_foundRows.Count)
            mCurrentSearchRow = 0;


         try
         {
            Xceed.Grid.DataRow row = m_foundRows[mCurrentSearchRow] as Xceed.Grid.DataRow;

            gridControl1.CurrentRow = row;
            row.BringIntoView();
         }
         catch (System.Exception e)
         {
            // m_foundRows.Clear();
         }
      }

      private void ChangeFilterMode()
      {
         if (m_filtered)
         {
            // The following line removes the filtering group which was placed at position 0 in the 
            // grid.GroupTemplates list.
            gridControl1.GroupTemplates.RemoveAt(0);

            this.btnFilter.Text = "Filter Search Results";
         }
         else
         {
            // Create a filtering group that has no header/footer rows and that has its 
            // margin not visible. In effect, the filter groups have no visual representation.
            // They are only use to show (expand) or hide (collapse) their inner data rows.
            Group filter = new Group();
            filter.SideMargin.Visible = false;

            // The following line inserts the filtering group at position 0 in the 
            // grid.GroupTemplates list.
            gridControl1.GroupTemplates.Insert(0, filter);

            this.btnFilter.Text = "Show All Rows";
         }

         m_filtered = !m_filtered;

         // This call will iterate each data row and assign a group to each one. For more 
         // details, see the grid_QueryGroupKeys and 
         // grid_GroupingUpdated event handler.
         gridControl1.UpdateGrouping();

         // Button btnSearch is disabled when the elements contained in the grid are filtered.
         this.mBtnSearch.Enabled = !m_filtered;
         this.mtxtbSearch.Enabled = !m_filtered;
         this.mChkCaseSensitive.Enabled = !m_filtered;
      }

      /// <summary>
      /// Only show the rows that were found in the search (or toggle - this button does double duty)
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="e"></param>
      private void btnFilter_Click(object sender, EventArgs e)
      {
         ChangeFilterMode();
         gridControl1.Focus();
      }

      /// <summary>
      /// Event handler for the grid
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="e"></param>
      private void gridControl1_GroupAdded(object sender, GroupAddedEventArgs e)
      {
         // The GroupAdded event is used for filtering. This event handler is in charge of
         // collapsing the instance of the filtering group that contains rejected rows and
         // expanding the instance of the filtering group that contains found rows.
         // Since the filtering groups do not have a visual representation (no margin, 
         // no header/footer rows), the collapsed group and all its content will not appear 
         // in the grid.
         if ((this.m_filtered) && (e.Group.Level == 0))
            e.Group.Collapsed = !((bool)e.Group.Key);
      }

      /// <summary>
      /// Event handler for the group
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="e"></param>
      private void gridControl1_GroupingUpdated(object sender, EventArgs e)
      {
         // The GroupingUpdated event is called whenever the grid's groupings are updated. 
         // This event handler "tags" the groups (their GroupManagerRow, to be more precise) 
         // that contain datarows that answer the search criteria.
         foreach (Xceed.Grid.DataRow dataRow in m_foundRows)
            this.TagGroups(dataRow);
      }
     
      /// <summary>
      /// Event handler for the grid
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="e"></param>
      private void gridControl1_QueryGroupKeys(object sender, QueryGroupKeysEventArgs e)
      {
         // The QueryGroupKeys event is used for filtering. This event handler groups the 
         // datarows manually by setting the first group key according to the datarow's 
         // presence in the foundRows list. We know that the first group key ( e.GroupKey[ 0 ] ) 
         // is the filtering group because we previously inserted the filtering group template 
         // at position 0 in the grid's group templates collection.
         if (m_filtered)
            e.GroupKeys[0] = m_foundRows.Contains(e.DataRow);
      }


      /// <summary>
      /// Delete some rows.
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="e"></param>
      private void mBtnDelete_Click(object sender, EventArgs e)
      {
         if (gridControl1.SelectedRows.Count <= 0)
            return;

         if (DialogResult.No == MessageBox.Show(this, "Are you sure you want to delete the selected strings?", "Phoenix Editor", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button2))
            return;

         for( int i = gridControl1.SelectedRows.Count - 1; i >= 0; i-- )
         {
            if( gridControl1.SelectedRows[ i ] is Xceed.Grid.DataRow )
               gridControl1.SelectedRows[ i ].Remove();
         }
      }

      /// <summary>
      /// check out the stringtable
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="e"></param>
      private void mBtnCheckout_Click(object sender, EventArgs e)
      {
         // sync these without checking the error
         string strTablePath = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, mStringTableFilename);
         string strTableXsdPath = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "stringtable.xsd");

         // If this file is checked out, then we cannot edit it in the editor. It must be fixed up manually first.
         SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(strTablePath);
         if (status.CheckedOutThisUser == true)
         {
            MessageBox.Show(this, "The stringtable is already checked out and must be resolved in perforce before editing in the editor.", "Error", MessageBoxButtons.OK);
            return;
         }

         // sync without checking for an error.
         mP4.Sync(strTablePath);
         mP4.Sync(strTableXsdPath);

         ReloadStringtable(true);
      }

      private void ClearSearch()
      {
         gridControl1.FixedFooterRows.Clear();
         m_foundRows.Clear();

         if (m_filtered)
            ChangeFilterMode();
      }



      private void MakeEditable(bool makeEditable)
      {
         Xceed.Grid.Column c = gridControl1.Columns["String_text"];
         if (c != null)
            c.ReadOnly = !makeEditable;

         c = gridControl1.Columns["category"];
         if (c != null)
            c.ReadOnly = !makeEditable;

         c = gridControl1.Columns["scenario"];
         if (c != null)
            c.ReadOnly = !makeEditable;

         c = gridControl1.Columns["subtitle"];
         if (c != null)
            c.ReadOnly = !makeEditable;
      }

      private bool makeWriteable(string filename)
      {
         if (!File.Exists(filename))
            return true;

         try
         {
            FileAttributes attrs = File.GetAttributes(filename);
            attrs &= (~FileAttributes.ReadOnly);                  // clear the RO flag
            File.SetAttributes(filename, attrs);
         }
         catch (Exception e)
         {
            MessageBox.Show(e.StackTrace);
            return false;
         }

         return true;
      }

      private bool makeReadOnly(string filename)
      {
         if (!File.Exists(filename))
            return true;

         try
         {
            FileAttributes attrs = File.GetAttributes(filename);
            attrs |= FileAttributes.ReadOnly;                     // set the RO flag
            File.SetAttributes(filename, attrs);
         }
         catch (Exception e)
         {
            MessageBox.Show(e.StackTrace);
            return false;
         }

         return true;
      }

      /// <summary>
      /// Save the changes back to the XML and check them in.
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="ev"></param>
      private void mBtnSave_Click(object sender, EventArgs ev)
      {
         // check to see if we have any changes to save out
         DataSet changes = mDoc.DataSet.GetChanges();
         if (changes == null)
         {
            MakeEditable(false);
            mEditMode = false;
            CheckEnableControls();

            return;
         }

         // build our paths
         string strTablePath = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, mStringTableFilename);
         string strTableXsdPath = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "stringtable.xsd");
         string strTableXmbPath = strTablePath + ".xmb";

         try
         {
            string backup = strTablePath + ".backup";
            makeWriteable(backup);
            if (!SaveStringTableToFile(backup, mDoc))
            {
               MessageBox.Show(this, "Error creating a backup copy of the stringtable.", "Error", MessageBoxButtons.OK);
               throw new Exception("error");
            }

            // make the current stringtable file RO
            if (!makeReadOnly(strTablePath))
               return;

            // get the latest file
            mP4.Sync(strTablePath);
            mP4.Sync(strTableXsdPath);

            // check out the existing file
            mP4.Checkout(strTablePath);
            SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(strTablePath);
            if (status.CheckedOutThisUser != true)
            {
               MessageBox.Show(this, "Could not check out the string table: \n\n" + mP4.LastError, "Error", MessageBoxButtons.OK);
               throw new Exception("error");
            }

            // lock the string table
            if (!mP4.Lock(strTablePath))
            {
               mP4.Revert(strTablePath);
               string errorMessage = "Could not lock the stringtable: \n\n" + mP4.LastError;
               throw new Exception(errorMessage);
            }

            // load the file into a data set
            XmlDataDocument origStringTable = new XmlDataDocument();
            origStringTable.DataSet.ReadXmlSchema(strTableXsdPath);
            origStringTable.Load(strTablePath);

            System.Data.DataTable table = origStringTable.DataSet.Tables[2];

            DataColumn[] PrimaryKeyColumns = new DataColumn[1];
            PrimaryKeyColumns[0] = table.Columns["_locID"];
            table.PrimaryKey = PrimaryKeyColumns;

            // merge the changes into the file
            origStringTable.DataSet.Merge(changes, false, MissingSchemaAction.Error);

            // Save out the stringtable changes
            if (!SaveStringTableToFileWithBackup(strTablePath, origStringTable))
               throw new Exception("error");

            // check the file back into perforce
            if (!mP4.Checkin(strTablePath, "String changes from the editor"))
            {
               MessageBox.Show(this, "An error occurred trying to checkin the stringtable: \n\n" + mP4.LastError, "", MessageBoxButtons.OK, MessageBoxIcon.Error);
               throw new Exception("error");
            }

            // XMB the file
            mP4.Sync(strTableXmbPath);
            EditorCore.XMBProcessor.CreateXMB(strTablePath, true);

            if (!mP4.Checkin(strTableXmbPath, "String changes from the editor"))
            {
               MessageBox.Show(this, "An error occurred trying to checkin the stringtable XMB file: stringtable.xml.xmb : \n\n" + mP4.LastError + "\n\n Please make sure this file get's checked in.", "", MessageBoxButtons.OK, MessageBoxIcon.Error);
               throw new Exception("error");
            }

            // force a reload of the stringtable file for other areas in the editor that use it.
            EditorCore.CoreGlobals.getGameResources().mStringTable.Load();

            ReloadStringtable(false);

            MessageBox.Show(this, "The stringtable was saved and checked in.", "", MessageBoxButtons.OK);

         }
         catch (Exception ex)
         {
            // do some clean up here
            SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(strTablePath);
            if (status.CheckedOutThisUser != true)
               mP4.Revert(strTablePath);

            // rethrow the exception
            throw ex;
         }
      }


      /// <summary>
      /// Discard Changes
      /// </summary>
      /// <param name="sender"></param>
      /// <param name="e"></param>
      private void mBtnDiscard_Click(object sender, EventArgs e)
      {
         if (DialogResult.No == MessageBox.Show(this, "Are you sure you want to discard your changes?", "Phoenix Editor", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button2))
            return;

         // go get the string from perforce again.
         string strTablePath = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, mStringTableFilename);
         makeReadOnly(strTablePath);
         mP4.Sync(strTablePath);

         ReloadStringtable(false);
      }

      private void ReloadStringtable(bool makeEditable)
      {
         ClearStringTable();
         LoadStringtable();
         MakeEditable(makeEditable);
         mEditMode = makeEditable;
         CheckEnableControls();
      }

      private void CheckEnableControls()
      {
         // we can delete, insert, save, discard
         this.mBtnDelete.Enabled = mEditMode;
         this.mBtnNewString.Enabled = mEditMode;
         this.mBtnSave.Enabled = mEditMode;
         this.mBtnSaveOnly.Enabled = mEditMode;
         this.mBtnDiscard.Enabled = mEditMode;

         // we cannot checkout
         this.mBtnCheckout.Enabled = !mEditMode;
      }

      private void mBtnSaveOnly_Click(object sender, EventArgs e)
      {
         string strTablePath = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, mStringTableFilename);
         string strTableXsdPath = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "stringtable.xsd");

         if (!makeWriteable(strTablePath))
         {
            MessageBox.Show("The stringtable file could not be made writeable");
            return;
         }

         // Save out the data set 
/*
         SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(strTablePath);
         if (status.CheckedOutThisUser != true)
         {
            MessageBox.Show(this, "You do not have the stringtable.xml file checked out", "Error", MessageBoxButtons.OK);
            return;
         }
*/
         if (!SaveStringTableToFileWithBackup(strTablePath, mDoc))
            return;

         MessageBox.Show(this, "The stringtable was saved to: \n\n" +strTablePath, "", MessageBoxButtons.OK);
      }

      private bool SaveStringTableToFileWithBackup(string strTablePath, XmlDataDocument xmlDataDoc)
      {
         // Save to the original file.
         if (!SaveStringTableToFile(strTablePath, xmlDataDoc))
            return false;

         // save to the backup file.
         string backupFile = strTablePath + ".backup";
         makeWriteable(backupFile);
         return SaveStringTableToFile(backupFile, xmlDataDoc);
      }

      private bool SaveStringTableToFile(string strTablePath, XmlDataDocument xmlDataDoc)
      {
         // save out the changes to the stringtable
         try
         {
            // XmlTextWriter writer = new XmlTextWriter(strTablePath, Encoding.Unicode);
            XmlTextWriter writer = new XmlTextWriter(strTablePath, Encoding.UTF8);
            writer.Indentation = 3;
            writer.IndentChar = ' ';
            writer.QuoteChar = '\'';
            writer.Formatting = Formatting.Indented;
            // mDoc.Save(strTablePath);
            xmlDataDoc.Save(writer);
            writer.Flush();
            writer.Close();
         }
         catch (Exception e)
         {
            MessageBox.Show(this, "An error occurred trying to save the stringtable: \n\n" + e.Message, "", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return false;
         }

         return true;
      }

      private void mtxtbSearch_KeyPress(object sender, KeyPressEventArgs e)
      {
         switch (e.KeyChar)
         {
            case '\r':
               Search();
               break;
         }
      }

      private void StringTablePage_KeyPress(object sender, KeyPressEventArgs e)
      {
         switch (e.KeyChar)
         {
/*
            Keys.F3:
               SetNextSearchRow();
               break;
*/
         }
      }

      private void StringTablePage_KeyUp(object sender, KeyEventArgs e)
      {
      }

      private void gridControl1_KeyUp(object sender, KeyEventArgs e)
      {
         switch (e.KeyCode)
         {
            case Keys.F3:
               SetNextSearchRow();
               break;
         }
      }

   }
}
