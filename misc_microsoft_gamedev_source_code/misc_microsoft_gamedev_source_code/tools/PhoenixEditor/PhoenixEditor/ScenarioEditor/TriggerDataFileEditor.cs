using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;

using SimEditor;
using EditorCore;
using EditorCore.Controls;
using EditorCore.Controls.Micro;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class TriggerDataFileEditor : UserControl
   {
      public TriggerDataFileEditor()
      {
         InitializeComponent();
         //comboBox1.Click += new EventHandler(comboBox1_Click);

         hybridGrid1.mCellClicked += new EditorCore.Controls.Micro.HybridGrid.CellInfo(hybridGrid1_mCellClicked);
         
         mFileName = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "userclasses.xml");

         Load();
      }
      string mFileName = "";

      public UserClassDefinitions mUserClassDefinitions;
      private void Load()
      {
         if (File.Exists(mFileName))
         {
            mUserClassDefinitions = BaseLoader<UserClassDefinitions>.Load(mFileName);
         }
      }

      void comboBox1_Click(object sender, EventArgs e)
      {
         //if (comboBox1.SelectedItem == null)
         //   return;
         //string name = comboBox1.SelectedItem.ToString();
         //foreach (UserClassDefinition d in mUserClassDefinitions.mUserClasses)
         //{
         //   if (d.Name == name)
         //   {
         //      mCurrentSchema = d;
         //      mTable = new TableData(mCurrentSchema.Fields.Count);
         //      //hybridGrid1.SetData(mTable.mData.ToArray());
         //   }
         //}
      }
      UserClassDefinition mCurrentSchema;
      TableData mCurrentTable = null;
      TableCollection mTables = new TableCollection();
      TriggerNamespace mCurrentTriggerNamespace;
      
      public void SetTriggerNamespace(TriggerNamespace n)
      {
         mCurrentTriggerNamespace = n;
         
         //foreach (UserClassDefinition d in mUserClassDefinitions.mUserClasses)
         //{
         //   //comboBox1.Items.Add(d.Name);
         //}
      }

      private void addrow_click(object sender, EventArgs e)
      {
         if (mCurrentTable == null)
         {
            MessageBox.Show("add or select a table first");
            return;
         }
         mCurrentTable.AddRow();
         UpdateUI();
      }

      private void UpdateUI()
      {

         if (mCurrentTable != null)
         {
            this.TableNameLabel.Text = mCurrentTable.mName;
            this.TableTypeLabel.Text = mCurrentTable.mClass.Name;
            if (mCurrentTable.mData.Count > 0)
            {
               hybridGrid1.SetData(mCurrentTable.mData.ToArray());


            }
            else
            {
               hybridGrid1.SetData(new string[0]);
               //clear grid
            }
         }
         mbSuppressListEvents = true;
         this.TableListBox.Items.Clear();
         this.TableListBox.Items.AddRange(mTables.mTables.ToArray());
         this.TableListBox.SelectedItem = mCurrentTable;
         mbSuppressListEvents = false;
         
      }
      bool mbSuppressListEvents = false;
      private void TableListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (mbSuppressListEvents)
            return;

         mCurrentTable = TableListBox.SelectedItem as TableData;
         if(mCurrentTable != null)
            mCurrentSchema = mCurrentTable.mClass;
         UpdateUI();
      }

      void hybridGrid1_mCellClicked(object sourceObject, string field, int row, int column)
      {
         if (mCurrentSchema == null)
            return;   
         UserClassFieldDefinition d = mCurrentSchema.Fields[column];
         ArrayCellBinder bind = new ArrayCellBinder(mCurrentTable.mData[row], column, d.Name, d.Type);
         //hybridGrid1.mObjectEditor.ApplyMetaDataToType(
         HighLevelProperty pr = TriggerSystemMain.mTriggerDefinitions.GetHLProperty(bind, mCurrentTriggerNamespace);

         string bindName;
         Control c = pr.GetEditor(out bindName);//mObjectEditor.GetSingleControl(sourceObject, field);
         PopupEditor pe = new PopupEditor();
         Form f = pe.ShowPopup(this, c, FormBorderStyle.Sizable, true);
         Point p2 = Cursor.Position;//this.PointToScreen(p);
         //f.Deactivate += new EventHandler(f_Deactivate);
         //f.Tag = cell;
         f.Width = 300;
         f.Top = p2.Y -25;
         f.Left = p2.X;
         f.TopMost = true;
         f.Show();

      }

      private void LoadButton_Click(object sender, EventArgs e)
      {
         OpenFileDialog ofd = new OpenFileDialog();
         ofd.InitialDirectory = CoreGlobals.getWorkPaths().mGameDataAIDataDirectory;
         ofd.Filter = "AIData (*" + CoreGlobals.getWorkPaths().mAIDataExtension + ")|*" + CoreGlobals.getWorkPaths().mAIDataExtension;
         if (ofd.ShowDialog() == DialogResult.OK)
         {
            try
            {
               Stream s = new FileStream(ofd.FileName, FileMode.Open, FileAccess.Read);
               mTables.LoadFromTableXML(s);
               mLastFileName = ofd.FileName;
            
               //mCurrentTable = mTables;

               UpdateUI();
            }
            catch (System.Exception ex)
            {
               MessageBox.Show(ex.ToString());
            }
         }
      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         if (mLastFileName != "")
         {
            try
            {               
               Stream s = new FileStream(mLastFileName, FileMode.Create);
               mTables.SaveToTableXML(s);

               XMBProcessor.CreateXMB(mLastFileName, false);

            }
            catch (System.Exception ex)
            {
               MessageBox.Show(ex.ToString());
            }

         }
      }
      string mLastFileName = "";
      private void SaveAsButton_Click(object sender, EventArgs e)
      {
         SaveFileDialog sfd = new SaveFileDialog();
         sfd.InitialDirectory = CoreGlobals.getWorkPaths().mGameDataAIDataDirectory;
         sfd.Filter = "AIData (*" + CoreGlobals.getWorkPaths().mAIDataExtension + ")|*" + CoreGlobals.getWorkPaths().mAIDataExtension;
         if (sfd.ShowDialog() == DialogResult.OK)
         {
            try
            {
               Stream s = new FileStream(sfd.FileName, FileMode.Create);
               mTables.SaveToTableXML(s);
               mLastFileName = sfd.FileName;

               XMBProcessor.CreateXMB(mLastFileName, false);

            }
            catch (System.Exception ex)
            {
               MessageBox.Show(ex.ToString());
            }
         }
      }

      class NewTableOptions
      {
         private string mName = "newTable";
         public string Name
         {
            set { mName = value; }
            get { return mName; }
         }
         private string mUserClass;
         public string UserClass
         {
            set { mUserClass = value; }
            get { return mUserClass; }
         }
      }

      private void AddNewTableButton_Click(object sender, EventArgs e)
      {
         List<string> classes = new List<string>();
         foreach (UserClassDefinition d in mUserClassDefinitions.mUserClasses)
         {
            classes.Add(d.Name);
         }
         PropertyList list = new PropertyList();
         NewTableOptions options = new NewTableOptions();
         list.AddMetaDataForProp("NewTableOptions", "UserClass", "SimpleEnumeration", classes.ToArray());
         list.SetTypeEditor("NewTableOptions", "UserClass", typeof(EnumeratedProperty));        
         list.SelectedObject = options;
         if (ContentMessageBox.ShowMessage(this, list, "Create a new table") == DialogResult.OK)
         {

            foreach (UserClassDefinition d in mUserClassDefinitions.mUserClasses)
            {
               if(options.UserClass == d.Name)
               {
                  TableData table = new TableData(options.Name, d);
                  mTables.mTables.Add(table);
                  mCurrentTable = table;
                  mCurrentSchema = d;
                  UpdateUI();
                  break;
               }
            }

         }
         //TableData table = new TableData(
      }

      private void RowUpButton_Click(object sender, EventArgs e)
      {
         if (mCurrentTable == null)
         {
            MessageBox.Show("add or select a table first");
            return;
         }

         int selectedRow = hybridGrid1.GetSelectedRowIndex();
         if (selectedRow != -1 && selectedRow > 0)
         {
            int nextIndx = selectedRow - 1;
            string[] temp = mCurrentTable.mData[nextIndx];
            mCurrentTable.mData[nextIndx] = mCurrentTable.mData[selectedRow];
            mCurrentTable.mData[selectedRow] = temp;
            UpdateUI();
            hybridGrid1.SetSelectedRowIndex(nextIndx);
         }
      }

      private void RowDownButton_Click(object sender, EventArgs e)
      {
         if (mCurrentTable == null)
         {
            MessageBox.Show("add or select a table first");
            return;
         }

         int selectedRow = hybridGrid1.GetSelectedRowIndex();
         if (selectedRow != -1 && selectedRow < (mCurrentTable.mData.Count-1))
         {
            int nextIndx = selectedRow + 1;
            string[] temp = mCurrentTable.mData[nextIndx];
            mCurrentTable.mData[nextIndx] = mCurrentTable.mData[selectedRow];
            mCurrentTable.mData[selectedRow] = temp;
            UpdateUI();
            hybridGrid1.SetSelectedRowIndex(nextIndx);
         }
      }

      private void DeleteRowButton_Click(object sender, EventArgs e)
      {
         if (mCurrentTable == null)
         {
            MessageBox.Show("add or select a table first");
            return;
         }

         int selectedRow = hybridGrid1.GetSelectedRowIndex();
         if (selectedRow != -1)
         {
            mCurrentTable.mData.RemoveAt(selectedRow);
            UpdateUI();
         }
         if (mCurrentTable.mData.Count > selectedRow)
         {
            hybridGrid1.SetSelectedRowIndex(selectedRow);
         }


      }
      class ImportColumn
      {
         private string mType = "";
         public string Type
         {
            set { mType = value; }
            get { return mType; }
         }
         private List<string> mData;
         public List<string> Data
         {
            set { mData = value; }
            get { return mData; }
         }
      }
      private void ImportColumnButton_Click(object sender, EventArgs e)
      {
         List<string> classes = new List<string>();
         foreach (UserClassFieldDefinition d in mCurrentSchema.Fields)
         {
            classes.Add(d.Name);
         }
         PropertyList list = new PropertyList();
         ImportColumn import = new ImportColumn();
         list.AddMetaDataForProp("ImportColumn", "Type", "SimpleEnumeration", classes.ToArray());
         list.SetTypeEditor("ImportColumn", "Type", typeof(EnumeratedProperty));
         list.SetTypeEditor("ImportColumn", "Data", typeof(StringListProperty));
         list.SelectedObject = import;
         
         if (ContentMessageBox.ShowMessage(this, list, "Import a column of data") == DialogResult.OK)
         {
            if (import.Data == null)
               return;
            for (int i = 0; i < mCurrentSchema.Fields.Count; i++)
            {
               if (mCurrentSchema.Fields[i].Name == import.Type)
               {
                  for (int j = 0; j < import.Data.Count; j++)
                  {
                     
                     if (mCurrentTable.mData.Count <= j)
                     {
                        mCurrentTable.AddRow();
                     }
                     string[] row = mCurrentTable.mData[j];
                     row[i] = import.Data[j];
                  }
                  UpdateUI();
                  return;
               }
            }
         }

      }


   }

   public class ArrayCellBinder : INamedTypedProperty
   {
      public string[] mParentRow;
      public int mIndex;
      public string mName;
      public string mTypeName;

      public ArrayCellBinder(string[] parentRow, int index, string name, string typeName)
      {
         mParentRow = parentRow;
         mIndex = index; 
         mName = name;
         mTypeName = typeName; 
      }

      #region INamedTypedProperty Members

      public string GetName()
      {
         return mName;
      }

      public string GetTypeName()
      {
         return mTypeName;
      }

      Dictionary<string, object> mMetaData = new Dictionary<string, object>();
      public Dictionary<string, object> MetaData
      {
         get
         {
            return mMetaData;
         }
         set
         {
            mMetaData = value;
         }
      }

      public object GetValue()
      {
         return mParentRow[mIndex];
      }

      public void SetValue(object val)
      {
         string v = val.ToString();// as string;
         mParentRow[mIndex] = v;
      }

      #endregion
   }

   //public class TableDataRow
   //{
   //   public TableDataRow(int numColumns)
   //   {
   //      mCells = new string[numColumns];
   //      for (int i = 0; i < numColumns; i++)
   //      {
   //         mCells[i] = "";
   //      }
   //   }
   //   public string[] mCells = null;
   //}

   public class TableCollection
   {
      public List<TableData> mTables = new List<TableData>();

      public void SaveToTableXML(Stream s)
      {
         XmlDocument doc = new XmlDocument();
         XmlElement rootNode = doc.CreateElement("Tables");

         //SaveToTableXML(doc, rootNode);
         foreach (TableData table in mTables)
         {
            table.SaveToTableXML(doc, rootNode);
         }

         doc.AppendChild(rootNode);
         doc.Save(s);
         s.Close();

      }

      public void LoadFromTableXML(Stream s)
      {
         XmlDocument doc = new XmlDocument();
         doc.Load(s);
         XmlElement rootNode = doc.FirstChild as XmlElement;

         //LoadFromTableXML(root);

         //foreach (TableData table in mTables)
         //{
         //   table.LoadFromTableXML(rootNode);
         //}
         mTables.Clear();

         foreach (XmlElement table in rootNode.ChildNodes)
         {
            if (table.Name == "Table")
            {
               TableData data = new TableData();
               data.LoadFromTableXML(table);
               mTables.Add(data);
            }
         }

         s.Close();
      }
   }

   public class TableData
   {

      public string mName = "";
      public UserClassDefinition mClass;
      
      public TableData()
      {
      }
      public TableData(string name, UserClassDefinition userClass)
      {
         mNumColumns = userClass.Fields.Count;
         mName = name;
         mClass = userClass;
      }
      public int mNumColumns;

      public override string ToString()
      {
         return mName + " (" + mClass.Name + ")";
      }

      //public List<TableDataRow> mData = new List<TableDataRow>();
      //public TableDataRow AddRow()
      //{
      //   TableDataRow r = new TableDataRow(mNumColumns);
      //   mData.Add(r);
      //   return r;
      //}

      public List<string[]> mData = new List<string[]>();
      public string[] AddRow()
      {
         string[] r = new string[mNumColumns];
         for (int i = 0; i < mNumColumns; i++)
         {
            r[i] = "";
         }
         mData.Add(r);
         return r;
      }

      //Format demo
      //<Table name="asdf" class="fasd">
      //  <Row c0="cov_banshee_upgrade3" c1="2" />
      //  <Row c0="cov_bruteChopper_upgrade1" c1="3" />
      //  <Row c0="cov_cleansing_upgrade1" c1="3" />
      //</Table>

      public void SaveToTableXML(Stream s)
      {
         XmlDocument doc = new XmlDocument();
         XmlElement rootNode  = doc.CreateElement("Table");

         SaveToTableXML(doc, rootNode);

         doc.AppendChild(rootNode);
         doc.Save(s);
         s.Close();
      }

      public void SaveToTableXML(XmlDocument doc, XmlElement rootNode)
      {
         
         XmlElement tableRoot = doc.CreateElement("Table");

         XmlAttribute nameAttr = doc.CreateAttribute("Name");
         nameAttr.Value = mName;
         tableRoot.Attributes.Append(nameAttr);

         XmlAttribute typeAttr = doc.CreateAttribute("Type");
         typeAttr.Value = mClass.Name;
         tableRoot.Attributes.Append(typeAttr);

         foreach (string[] row in mData)
         {
            XmlElement rowNode = doc.CreateElement("Row");
            int i = 0;
            foreach (string cell in row)
            {
               //Attribute Version!!
               //XmlAttribute col = doc.CreateAttribute("c" + i.ToString());
               //col.Value = cell;
               //rowNode.Attributes.Append(col);
               //i++;

               //Subnode Version!!
               XmlElement e = doc.CreateElement("c");
               XmlText t = doc.CreateTextNode(cell);
               e.AppendChild(t);
               rowNode.AppendChild(e);
               i++;
            }
            tableRoot.AppendChild(rowNode);
            rootNode.AppendChild(tableRoot);
         }
      }

      public void LoadFromTableXML(Stream s)
      {
         XmlDocument doc = new XmlDocument();
         doc.Load(s);
         XmlElement root = doc.FirstChild as XmlElement;


         LoadFromTableXML(root);


         s.Close();
      }
      public void LoadFromTableXML(XmlElement rootNode)
      {
         //XmlAttribute name = rootNode.Attributes["Name"];
         mName = rootNode.GetAttribute("Name");
         string className = rootNode.GetAttribute("Type");
         mClass = TriggerSystemMain.mTriggerDefinitions.GetUserClassDefinition(className);
         mNumColumns = mClass.Fields.Count;
         foreach (XmlElement row in rootNode.ChildNodes)
         {
            int numColumns = row.ChildNodes.Count;//row.Attributes.Count;
            string[] columns = new string[numColumns];
            for (int i = 0; i < numColumns; i++)
            {
               //Attribute Version!!
               //XmlAttribute col = row.Attributes[i];
               //columns[i] = col.Value;

               //Subnode Version!!
               XmlElement e = row.ChildNodes[i] as XmlElement;
               if (e != null)
               {
                  columns[i] = e.InnerText;
               }
            }
            mData.Add(columns);
         }
      }

      public void SaveToTable(Stream s)
      {
         StreamWriter w = new StreamWriter(s);
         w.WriteLine("1");  //version 1
         w.WriteLine();
         w.WriteLine();
         w.WriteLine();
         foreach (string[] row in mData)
         {
            foreach (string cell in row)
            {
               w.Write(cell + "\t");
            }
            w.WriteLine();
         }
         w.Close();
      }
      public void LoadFromTable(Stream s)
      {
         mData.Clear();
         StreamReader r = new StreamReader(s);
         r.ReadLine(); 
         r.ReadLine();
         r.ReadLine();
         r.ReadLine();
         string line = r.ReadLine();
         while (line != null)
         {
            string[] data = line.Split('\t');
            Array.Resize(ref data, data.Length - 1);
            mData.Add(data);
            line = r.ReadLine();
         }
         r.Close();
      }


   }
}
