using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using SimEditor;
using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class TriggerPopupInputForm : Form
   {
      public TriggerPopupInputForm()
      {
         InitializeComponent();

         this.TopMost = true;

         this.Width += 90;
         
         
      }

      TriggerVariable mVariable;
      TriggerValue mValue;
      TriggerParameterControl mParent;
      HighLevelProperty mHLProp;
      public void Init (TriggerParameterControl parent, TriggerVariable var, TriggerValue val, HighLevelProperty HLProp)
      {
         this.Text = var.Name + " : " + val.Type;

         mVariable = var;
         mValue = val;
         mParent = parent;
         mHLProp = HLProp;

         UpdateControl();

         //save teh mouseclicks.
         if (mValue.Name.Contains( (mVariable.Name + TriggerNamespace.GetDefaultVarSuffix()) ))
         {
            if (TriggerSystemMain.mTriggerDefinitions.IsListType(mValue.Type))
            {
               mValue.IsNull = false;
               ////mParent.ParameterMode = TriggerParameterControl.eParameterMode.
               UpdateControl();
               InitVarMode();
            }
            else if(parent.ParentTriggerNamespace.IsSharedValueVar(  mValue.ID))
            {
               //SetValueMode();
            }
            else
            {
               //InitValueMode();
               SetValueMode();
            }
         }
      }

      private void UpdateControl()
      {

         if (mParent.ParameterMode == TriggerParameterControl.eParameterMode.Const)
         {
            InitValueMode();
         }
         else if( mParent.ParameterMode == TriggerParameterControl.eParameterMode.NullValue)
         {
            InitNullMode();
         }
         else
         {
            InitVarMode();
         }
      }

      
      private void SetCenterControl(Control c)
      {
         int height = c.Height + 30;
         int width = c.Width + 30;

         if (width < 230)
            width = 230;

         this.Height = height;
         this.Width = width;
         panel1.Controls.Clear();
         c.Margin = new Padding(0);
         c.Dock = DockStyle.Fill;
         this.panel1.Controls.Add(c);

      }

      enum localEditMode 
      {
         Value,
         VarView,
         VarRename,
         VarNull

      };
      localEditMode mMode;
      private void InitValueMode()
      {

         string temp;
         Control editor = mHLProp.GetEditor(out temp);

         SetCenterControl(editor);
         mMode = localEditMode.Value;

         this.Text = "CONSTANT " + mVariable.Name + " : " + mValue.Type;

      }
      private void InitVarMode()
      {
         panel1.Controls.Clear();

         //need var display label...
         Label l = new Label();
         l.Text = mValue.Name;


         //if (TriggerSystemMain.mTriggerDefinitions.IsListType(mValue.Type))
         {
            Button setDefault = new Button();

            setDefault.Text = "Default";
            setDefault.Click += new EventHandler(setDefault_Click);
            SetCenterControl( DynamicUIHelpers.MakePair(l, setDefault) );


         }
         //else
         //{

         //   SetCenterControl(l);
         //}


         //Button b = new Button();
         //b.Text = "Default value..";
         //if(mValue.Value != null && mValue.Value != "")
         //{
         //   b.ForeColor = System.Drawing.Color.Green;
         //}

         //Control c = DynamicUIHelpers.MakePair(l, b);


         mMode = localEditMode.VarView;

         this.Text = "VARIABLE " + mVariable.Name + " : " + mValue.Type;

      }
      private void InitNullMode()
      {
         panel1.Controls.Clear();
         Label l = new Label();
         l.Text = "No Optional Value Set";
         SetCenterControl(l);

         mMode = localEditMode.VarNull;

         this.Text = "IGNORED " + mVariable.Name + " : " + mValue.Type;

      }
      void setDefault_Click(object sender, EventArgs e)
      {
         //string temp;
         //Control editor = mHLProp.GetEditor(out temp);
         ////editor.Size
         //PopupEditor pe = new PopupEditor();
         //pe.ShowPopup(this, editor);//, FormBorderStyle.Sizable);

         Label l = new Label();
         l.Text = mValue.Name;
         string temp;
         Control editor = mHLProp.GetEditor(out temp);
         int height = editor.Height;
         int width = editor.Width;
         Control pair = DynamicUIHelpers.MakePair(l, editor, l.Width / 3);
         pair.Height = height;
         SetCenterControl(pair);

         this.Width += width;
      
      }






      TextBox mRenameVarTextBox = null;
      private void RenameVarMode()
      {
         //mRenameVarTextBox
         panel1.Controls.Clear();

         mRenameVarTextBox = new TextBox();

         mRenameVarTextBox.Text = this.mValue.Name;

         SetCenterControl(mRenameVarTextBox);
         mMode = localEditMode.VarRename;
      }

      private void TriggerPopupInputForm_Deactivate(object sender, EventArgs e)
      {
         this.TopMost = true;
      }

      private void OKButton_Click(object sender, EventArgs e)
      {
         if(mMode == localEditMode.VarRename)
         {
            this.mValue.Name = mRenameVarTextBox.Text;
         }


         DialogResult = DialogResult.OK;
         this.Close();
      }

      private void OptionsButton_Click(object sender, EventArgs e)
      {
         if (mContextMenu == null)
         {
            mContextMenu = BuildMenu();
            
         }
         mContextMenu.Show(OptionsButton, new Point(0, 0));
      }



      ContextMenu mContextMenu = null;

      private ContextMenu BuildMenu()
      {
         ContextMenu menu = new ContextMenu();



         if (mParent.ParameterMode == TriggerParameterControl.eParameterMode.Const)
         {



         }
         //else if(mParent.ParameterMode == TriggerParameterControl.eParameterMode.NullValue)
         //{
         //   MenuItem editValueItem = new MenuItem("Edit value...");
         //   editValueItem.Click += new EventHandler(editValueItem_Click);
         //   menu.MenuItems.Add(editValueItem);
         //}
         else
         {
            MenuItem editValueItem = new MenuItem("Edit value...");
            editValueItem.Click += new EventHandler(editValueItem_Click);
            menu.MenuItems.Add(editValueItem);

            MenuItem renameVarItem = new MenuItem("Rename variable...");
            renameVarItem.Click += new EventHandler(renameVarItem_Click);
            menu.MenuItems.Add(renameVarItem);
         }

         MenuItem newVariableItem = new MenuItem("New Variable");
         newVariableItem.Click += new EventHandler(newVariableItem_Click);
         menu.MenuItems.Add(newVariableItem);

         if (mParent.OptionalParameter)
         {         
            MenuItem setNullItem = new MenuItem("Ignore Optional Value");
            setNullItem.Checked = (mParent.ParameterMode == TriggerParameterControl.eParameterMode.NullValue);
            setNullItem.Click += new EventHandler(setNullItem_Click);
            menu.MenuItems.Add(setNullItem);
         }

         //List<int> queryres = mParent.ParentTriggerNamespace.QueryVars(mValue.Type, -1);
         //Dictionary<int, TriggerValue> values = mParent.ParentTriggerNamespace.GetValues();

         //if (queryres.Count > 0)
         //{
         //   MenuItem varMenu = new MenuItem("Pick Variable");
         //   foreach (int i in queryres)
         //   {
         //      bool isglobal = mParent.ParentTriggerNamespace.IsGlobalVar(i);

         //      MenuItem varSelection = new MenuItem(values[i].Name);

         //      varSelection.Tag = i;
         //      //if(mParent.ParentTriggerNamespace.IsGlobalVar(i))
         //      //{
         //      //   varSelection.Color = "wtf"?????????!
         //      //}

         //      //editValueItem.Click += new EventHandler(editValueItem_Click);
         //      varSelection.Click += new EventHandler(varMenu_Click);
         //      varMenu.MenuItems.Add(varSelection);
         //   }
            
         //   menu.MenuItems.Add(varMenu);
         //}
         MenuItem varPick = new MenuItem("Pick Variable...");
         varPick.Click += new EventHandler(varPick_Click);
         menu.MenuItems.Add(varPick);

         return menu;
         //mContextMenu = new ContextMenu();

         //foreach (string s in options)
         //{
         //   MenuItem m = new MenuItem(s);
         //   m.Click += new EventHandler(simpleOption_Click);
         //   m.Tag = s;
         //   mContextMenu.MenuItems.Add(m);
         //}


      }
      
      //PopupEditor mPopupListChooser;
      void varPick_Click(object sender, EventArgs e)
      {
         if (mPopupVarPicker != null && mPopupVarPicker.IsDisposed == false)
         {
            mPopupVarPicker.Close();
         }

         List<int> queryres = mParent.ParentTriggerNamespace.QueryVars(mValue.Type, -1);
         Dictionary<int, TriggerValue> values = mParent.ParentTriggerNamespace.GetValues();
         ListView listView = new ListView();
         listView.MouseUp += new MouseEventHandler(listView_MouseDown);
         if (queryres.Count > 0)
         {
            foreach (int i in queryres)
            {
               ListViewItem item = new ListViewItem();

               bool isglobal = mParent.ParentTriggerNamespace.IsGlobalVar(i);

               item.Text = values[i].Name;
               item.Tag = i;
               if(mParent.ParentTriggerNamespace.IsGlobalVar(i))
               {
                  item.ForeColor = Color.Blue;

               }
               listView.Items.Add(item);
            }
            listView.View = View.List;
            if (queryres.Count < 25)
            {
               listView.Size = new Size(400, 400);
            }
            else
            {
               listView.Size = new Size(400, 900);
            }
            listView.Sorting = SortOrder.Ascending;
            listView.HotTracking = true;
            listView.SelectedIndexChanged += new EventHandler(listView_SelectedIndexChanged);
         }

         PopupEditor pe = new PopupEditor();
         
         mPopupVarPicker = pe.ShowPopup(this, listView, FormBorderStyle.Sizable, false,  "Blue = Used in multiple groups");
      
      }

      void listView_SelectedIndexChanged(object sender, EventArgs e)
      {
         ListView lv = sender as ListView;
         lv.SelectedItems.Clear();
      }
      Form mPopupVarPicker = null;
      void listView_MouseDown(object sender, MouseEventArgs e)
      {
         ListView lv = sender as ListView;
         if (e.Button == MouseButtons.Left)
         {
            ListViewItem item = lv.GetItemAt(e.X, e.Y);
            if (item == null)
               return;
          
            SetVariableMode((int)(item.Tag));

            mPopupVarPicker.Close();            
         }
      }

      TriggerParameterControl.eParameterMode mLastMode; 
      void setNullItem_Click(object sender, EventArgs e)
      {
         MenuItem menuitem = sender as MenuItem;

         menuitem.Checked = !menuitem.Checked;
         if (menuitem.Checked == true)
         {
            //mLastMode = mParent.ParameterMode;
            //mParent.ParameterMode = TriggerParameterControl.eParameterMode.NullValue;
            SetNullMode();
         }
         else
         {
            SetVariableMode();
         }
         //else
         //{
         //   mParent.ParameterMode = mLastMode;
         //}


         //throw new Exception("The method or operation is not implemented.");
      }

      void renameVarItem_Click(object sender, EventArgs e)
      {
         RenameVarMode();
      }

      void varMenu_Click(object sender, EventArgs e)
      {
         SetVariableMode((int)((MenuItem)sender).Tag);
      }

      void newVariableItem_Click(object sender, EventArgs e)
      {
         SetVariableMode();

         RenameVarMode();
      }

      void editValueItem_Click(object sender, EventArgs e)
      {
         SetValueMode();


      }
      void SetValueMode()
      {
         //mParent.ParentTriggerNamespace.SwitchVarToConstant(mVariable, ref mValue);         
         //mParent.ParameterMode = TriggerParameterControl.eParameterMode.Const;
         mValue = mParent.SetValueMode();
         InitValueMode();
         //UpdateControl();     
      }
      void SetVariableMode()
      {
         mValue = mParent.SetVarMode();
         //UpdateControl(); 
         InitVarMode();

         //mParent.ParameterMode = TriggerParameterControl.eParameterMode.LocalVariable;
         //UpdateControl();     


      }

      void SetVariableMode(int newValueID)
      {
         mValue = mParent.SetVarMode(newValueID);
         //UpdateControl(); 
         InitVarMode();

         //mParent.ParameterMode = TriggerParameterControl.eParameterMode.LocalVariable;
         //UpdateControl();     


      }
      void SetNullMode()
      {
         mValue = mParent.SetNullMode();
         InitNullMode();

      }

      //protected void SetSimpleMenu(List<string> options)
      //{
      //   mContextMenu = new ContextMenu();

      //   foreach (string s in options)
      //   {
      //      MenuItem m = new MenuItem(s);
      //      m.Click += new EventHandler(simpleOption_Click);
      //      m.Tag = s;
      //      mContextMenu.MenuItems.Add(m);
      //   }

      //}

      //void simpleOption_Click(object sender, EventArgs e)
      //{
      //   MenuItem m = (MenuItem)sender;
      //   HandleMenuOption(m.Text, m.Tag);
      //}

   }
}