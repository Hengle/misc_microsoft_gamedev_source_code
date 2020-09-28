using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Serialization;
using System.IO;
using System.Windows.Forms;

namespace pxdb
{
   //==============================================================================
   // Database
   //==============================================================================
   static public class Database
   {
      //---------------------------------------------------------------------------
      // Members
      //---------------------------------------------------------------------------
      static public Dictionary<string, int> mProtoObjectTable = new Dictionary<string, int>();
      static public GameData mGameData = null;
      static public ProtoObjects mProtoObjectData = null;
      static public GridDef mGridDefData = null;

      //---------------------------------------------------------------------------
      // setup
      //---------------------------------------------------------------------------
      static public bool setup()
      {
         XmlSerializer s = null;
         Stream st = null;

         //-- GameData
         s = new XmlSerializer(typeof(GameData));
         st = File.OpenRead(@"data\gamedata.xml");
         mGameData = (GameData)s.Deserialize(st);

         //-- ProtoObjects
         s = new XmlSerializer(typeof(ProtoObjects));
         st = File.OpenRead(@"data\objects.xml");
         mProtoObjectData = (ProtoObjects)s.Deserialize(st);
         mProtoObjectData.mList.calcNextID();
                                                                                                            
         //-- GridDef
         s = new XmlSerializer(typeof(GridDef));
         st = File.OpenRead(@"tools\pxdb\griddef.xml");
         mGridDefData = (GridDef)s.Deserialize(st);

         /*
         st = File.OpenWrite(@"c:\x\work\data\temp2.xml");
         s.Serialize(st, mProtoObjectData);
         */

         return true;
      }

      //---------------------------------------------------------------------------
      // getDataSource
      //---------------------------------------------------------------------------
      static public Object getDataSource(string dataType)
      {
         if (dataType == @"Object")
            return mProtoObjectData.mList;
         else if (dataType == @"Game")
            return mGameData;
         else if (dataType == @"Resource")
            return mGameData.Resources;
         else if (dataType == @"Pop")
            return mGameData.Pops;
         else if (dataType == @"Link")
            return mGameData.Links;
         return null;
      }

      //---------------------------------------------------------------------------
      // setupDataTypeCombo
      //---------------------------------------------------------------------------
      static public void setupDataTypeCombo(ComboBox comboBox)
      {
         comboBox.Items.Clear();
         foreach (GridDefData dataDef in mGridDefData.mDatas)
         {
            comboBox.Items.Add(dataDef.mType);
         }
      }

      //---------------------------------------------------------------------------
      // setupGroupList
      //---------------------------------------------------------------------------
      static public void setupGroupList(ListBox listBox, string dataType)
      {
         listBox.Items.Clear();
         foreach (GridDefData dataDef in mGridDefData.mDatas)
         {
            if (dataDef.mType != dataType)
               continue;
            foreach (GridDefGroup groupDef in dataDef.mGroups)
            {
               listBox.Items.Add(groupDef.mGroupName);
            }
            break;
         }
      }

      //---------------------------------------------------------------------------
      // setupDetailList
      //---------------------------------------------------------------------------
      static public void setupDetailList(ListBox listBox, string dataType, string groupType)
      {
         listBox.Items.Clear();
         foreach (GridDefData dataDef in mGridDefData.mDatas)
         {
            if (dataDef.mType != dataType)
               continue;
            foreach (GridDefGroup groupDef in dataDef.mGroups)
            {
               if (groupDef.mGroupName != groupType)
                  continue;
               foreach (GridDefDetail detail in groupDef.mDetails)
               {
                  listBox.Items.Add(detail.mDataName);
               }
               break;
            }
            break;
         }
      }

      //---------------------------------------------------------------------------
      // setupGridView
      //---------------------------------------------------------------------------
      static public void setupGridView(DataGridView gridView, string dataType, string detailType, string groupType)
      {
         gridView.AutoGenerateColumns = false;
         gridView.Columns.Clear();

         foreach (GridDefData dataDef in mGridDefData.mDatas)
         {
            if (dataDef.mType != dataType)
               continue;
            if (dataDef.mDisableAdd)
               gridView.AllowUserToAddRows = false;
            else
               gridView.AllowUserToAddRows = true;
            foreach (GridDefGroup groupDef in dataDef.mGroups)
            {
               if (groupDef.mGroupName != groupType)
                  continue;
               List<GridDefColumn> columns = null;
               if (detailType != "")
               {
                  foreach (GridDefDetail detail in groupDef.mDetails)
                  {
                     if (detail.mDataName == detailType)
                     {
                        columns = detail.mColumns;
                        break;
                     }
                  }
                  if (columns == null)
                     break;
               }
               else
                  columns = groupDef.mColumns;
               foreach (GridDefColumn colDef in columns)
               {
                  DataGridViewColumn col = null;
                  if (colDef.mColumnType == "Text")
                     col = new DataGridViewTextBoxColumn();
                  else if (colDef.mColumnType == "Check")
                     col = new DataGridViewCheckBoxColumn();
                  else if (colDef.mColumnType == "Combo")
                  {
                     DataGridViewComboBoxColumn comboCol = new DataGridViewComboBoxColumn();
                     Object dataSource = getDataSource(colDef.mDataSource);
                     if (dataSource != null)
                     {
                        comboCol.DataSource = dataSource;
                        comboCol.ValueMember = colDef.mValueMember;
                        comboCol.DisplayMember = colDef.mDisplayMember;
                     }
                     else
                     {
                        foreach (string itemDef in colDef.mItems)
                           comboCol.Items.Add(itemDef);
                     }
                     col = comboCol;
                  }
                  if (col != null)
                  {
                     col.DataPropertyName = colDef.mDataName;
                     if (colDef.mValueType == "int")
                        col.ValueType = typeof(int);
                     else if (colDef.mValueType == "string")
                        col.ValueType = typeof(string);
                     else if (colDef.mValueType == "bool")
                        col.ValueType = typeof(bool);
                     else if (colDef.mValueType == "float")
                        col.ValueType = typeof(float);
                     if (colDef.mHeaderText.Length > 0)
                        col.HeaderText = colDef.mHeaderText;
                     col.AutoSizeMode = DataGridViewAutoSizeColumnMode.AllCells;
                     col.ReadOnly = colDef.mReadOnly;
                     col.Frozen = colDef.mFrozen;
                     gridView.Columns.Add(col);
                  }
               }
               break;
            }
            break;
         }
      }
   }

   //==============================================================================
   // DataString
   //==============================================================================
   public class DataString
   {
      private string mName = @"";

      [XmlText]
      public string Name
      {
         get { return mName; }
         set { mName = value; }
      }
   }

   //==============================================================================
   // DataStringList
   //==============================================================================
   public class DataStringList : List<DataString>
   {
   };

   //==============================================================================
   // DataID
   //==============================================================================
   public class DataID
   {
      private int mID = 0;

      [XmlText]
      public int ID
      {
         get { return mID; }
         set { mID = value; }
      }
   }

   //==============================================================================
   // DataIDList
   //==============================================================================
   public class DataIDList : List<DataID>
   {
   };
}
