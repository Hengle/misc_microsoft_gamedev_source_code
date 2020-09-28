using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using SimEditor;
using EditorCore;


namespace PhoenixEditor.ScenarioEditor
{
   public partial class TriggerValueList : UserControl
   {
      public TriggerValueList()
      {
         InitializeComponent();

         gridControl1.Click += new EventHandler(gridControl1_Click);
         gridControl1.MouseDown += new MouseEventHandler(gridControl1_MouseDown);
      }

      void gridControl1_MouseDown(object sender, MouseEventArgs e)
      {
         Control c = gridControl1.GetChildAtPoint(new Point(e.X, e.Y), GetChildAtPointSkip.None);
         //object o = gridControl1.GetVisualGridElementAtPoint(new Point(e.X, e.Y));
         //throw new Exception("The method or operation is not implemented.");
      }

      void gridControl1_Click(object sender, EventArgs e)
      {
         
         //throw new Exception("The method or operation is not implemented.");
      }

      TriggerNamespace mParentTriggerNamespace = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public TriggerNamespace ParentTriggerNamespace
      {
         set
         {
            mParentTriggerNamespace = value;
            RefreshList();
         }
         get
         {
            return mParentTriggerNamespace;
         }
      }



      public void RefreshList()
      {
         if (ParentTriggerNamespace == null)
            return;

         //gridControl1.DataSource = 
         List<TriggerValueContainer> list = new List<TriggerValueContainer>();
         foreach (TriggerValue v in mParentTriggerNamespace.GetValues().Values)
         {
            list.Add(new TriggerValueContainer(v));
         }

         gridControl1.SingleClickEdit = false;// true;

         gridControl1.AddingDataRow += new Xceed.Grid.AddingDataRowEventHandler(gridControl1_AddingDataRow);
         //gridControl1.DataSource = mParentTriggerNamespace.GetValues().Values;
         gridControl1.DataSource = list;

         //gridControl1.DataMember = "TriggerValueContainer";

         //foreach (Xceed.Grid.DataCell cell in gridControl1.DataRowTemplate.Cells)
         //{

         //   cell.Click += new EventHandler(cell_Click);

         //}

         //gridControl1.DataRowTemplate.Cells["DefaultValue"].Click += new EventHandler(cell_Click);

         //AddIDEditor(gridControl1.Columns, "DefaultValue");
         //AddIDEditor(gridControl1.Columns, "Value");

         //gridControl1

      }

      void gridControl1_AddingDataRow(object sender, Xceed.Grid.AddingDataRowEventArgs e)
      {
         e.DataRow.Cells["DefaultValue"].Click += new EventHandler(cell_Click);
        // throw new Exception("The method or operation is not implemented.");
      }

      void cell_Click(object sender, EventArgs e)
      {
         Xceed.Grid.DataCell cell = sender as Xceed.Grid.DataCell;

         int id = (int)(cell.ParentRow.Cells["ID"].Value);

         if (mEditorforms.ContainsKey(id))
         {
            mEditorforms[id].BringToFront();

         }
         else
         {
            TriggerValue val = mParentTriggerNamespace.GetValues()[id];
            VarValueBinder binder = new VarValueBinder(val, val);
            HighLevelProperty HLProp = TriggerSystemMain.mTriggerDefinitions.GetHLProperty(binder, mParentTriggerNamespace);
            string bindName;
            Control c = HLProp.GetEditor(out bindName);
            //this.TemplateControl = c;
            //this.TemplateControl

            Form f = new Form();


            f.Tag = id;


            f.Controls.Add(c);
            c.Dock = DockStyle.Fill;
            f.Text = binder.GetName();
            //f.TopLevelControl = true;
            f.Size = new Size(200, 55);

            Point p1 = new Point(0, 0);
            Point p2 = cell.PointToScreen(p1);
            f.StartPosition = FormStartPosition.Manual;
            p2.Y -= 30;
            p2.X -= 60;
            f.Location = p2;
            //f.Init(this, mVariable, mValue, mHLProp);
            //f.FormClosed += new FormClosedEventHandler(f_FormClosed);
            //f.Show();
            f.FormClosed += new FormClosedEventHandler(f_FormClosed);

            f.TopMost = true;
            f.Show();

            mEditorforms[id] = f;

            
         }

      }

      Dictionary<int, Form> mEditorforms = new Dictionary<int, Form>();
      void f_FormClosed(object sender, FormClosedEventArgs e)
      {
         int id = (int)(((Form)sender).Tag);
         mEditorforms.Remove(id);
         //throw new Exception("The method or operation is not implemented.");
      }


      public void AddIDEditor(Xceed.Grid.Collections.ColumnList columns, string columnName  )
      {

         //Button idedit = new Button();
         //idedit.Text = "asdf";
         //idedit.Click += new EventHandler(idedit_Click);
         //columns[columnName].CellEditorManager = new Xceed.Grid.Editors.CellEditorManager(idedit, "Text", true, true);

         //columns[columnName].CellEditorManager = new Xceed.Grid.Editors.CellEditorManager(
         //columns[columnName].CellEditorManager.ActivatingControl += new Xceed.Grid.Editors.CellEditorEventHandler(CellEditorManager_ActivatingControl);
         //columns[columnName].CellViewerManager = new Xceed.Grid.Viewers.CellViewerManager(idedit, "Tag");

         columns[columnName].CellEditorManager = new FunManger(ParentTriggerNamespace);
      }

      void CellEditorManager_ActivatingControl(object sender, Xceed.Grid.Editors.CellEditorEventArgs e)
      {
         //throw new Exception("The method or operation is not implemented.");
         //gridControl1.get

         int id = (int)(e.Cell.ParentRow.Cells["ID"].Value);
         //e.Control.Tag = o;

         TriggerValue val = mParentTriggerNamespace.GetValues()[id];
         VarValueBinder binder = new VarValueBinder(val, val);
         HighLevelProperty HLProp = TriggerSystemMain.mTriggerDefinitions.GetHLProperty(binder, mParentTriggerNamespace);
         string bindName;
         Control c = HLProp.GetEditor(out bindName);

         Form f = new Form();
         f.Controls.Add(c);
         c.Dock = DockStyle.Fill;
         f.Location = (new Point(0,0));
         f.Text = binder.GetName();
         f.Show();

         //e.Cell. = c;
         //e.Control = c;

      }
      class FunManger : Xceed.Grid.Editors.CellEditorManager
      {
         TriggerNamespace mParentTriggerNamespace = null;
         public FunManger(TriggerNamespace parentNamespace) : base (new Control(),false,false)
         {
               mParentTriggerNamespace = parentNamespace;
               this.ActivatingControl += new Xceed.Grid.Editors.CellEditorEventHandler(FunManger_ActivatingControl);
         }

         void FunManger_ActivatingControl(object sender, Xceed.Grid.Editors.CellEditorEventArgs e)
         {
            //throw new Exception("The method or operation is not implemented.");
         }
         protected override Control CreateControl()
         {
            //return base.CreateControl();

            return new Control();
         }

         Dictionary<int, Form> mEditorforms = new Dictionary<int, Form>();

         


         protected override void ActivateControlCore(Control control, Xceed.Grid.Cell cell)
         {
            int id = (int)(cell.ParentRow.Cells["ID"].Value);

            if (mEditorforms.ContainsKey(id))
            {
               mEditorforms[id].BringToFront();

            }
            else
            {
               TriggerValue val = mParentTriggerNamespace.GetValues()[id];
               VarValueBinder binder = new VarValueBinder(val, val);
               HighLevelProperty HLProp = TriggerSystemMain.mTriggerDefinitions.GetHLProperty(binder, mParentTriggerNamespace);
               string bindName;
               Control c = HLProp.GetEditor(out bindName);
               //this.TemplateControl = c;
               //this.TemplateControl

               Form f = new Form();


               f.Tag = id;


               f.Controls.Add(c);
               c.Dock = DockStyle.Fill;
               f.Text = binder.GetName();
               //f.TopLevelControl = true;
               f.Size = new Size(200, 55);

               Point p1 = new Point(0, 0);
               Point p2 = cell.PointToScreen(p1);
               f.StartPosition = FormStartPosition.Manual;
               p2.Y -= 30;
               p2.X -= 60;
               f.Location = p2;
               //f.Init(this, mVariable, mValue, mHLProp);
               //f.FormClosed += new FormClosedEventHandler(f_FormClosed);
               //f.Show();
               f.FormClosed += new FormClosedEventHandler(f_FormClosed);

               f.TopMost = true;
               f.Show();

               mEditorforms[id] = f;

               base.ActivateControlCore(c, cell);
            }
         }

         void f_FormClosed(object sender, FormClosedEventArgs e)
         {
            int id = (int)(((Form)sender).Tag);
            mEditorforms.Remove(id);
            //throw new Exception("The method or operation is not implemented.");
         }
         

      }

      void idedit_Click(object sender, EventArgs e)
      {
         int i = (int)((sender as Control).Tag);
      }


      private void button1_Click(object sender, EventArgs e)
      {
         RefreshList();
      }

   }

}
