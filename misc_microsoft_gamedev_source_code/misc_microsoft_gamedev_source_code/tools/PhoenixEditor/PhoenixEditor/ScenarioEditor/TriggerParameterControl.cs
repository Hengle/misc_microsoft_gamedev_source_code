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
   public partial class TriggerParameterControl : UserControl, ITriggerUIUpdate
   {
      public TriggerParameterControl()
      {
         InitializeComponent();
         this.Size = TextLabel.Size;
         TextLabel.AutoSize = false;
         this.AllowDrop = true;

         mBackGroundColor = this.BackColor;
         
      }

      TriggerComponent mComponent;
      TriggerVariable mVariable;
      TriggerValue mValue;
      public void Init(TriggerComponent comp, TriggerVariable var, TriggerValue val, TriggerNamespace trigNS)//, Trigger trigger)
      {
         mComponent = comp;
         mVariable = var;
         mValue = val;

         ParentTriggerNamespace = trigNS;
         //Trigger = trigger;

         InitHLProp();

         UpdateControl();
      }

      public event EventHandler LabelChanged;
      public event EventHandler HotSelect;
      public event EventHandler HotSelectEnd;


      TriggerNamespace mParentTriggerNamespace = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public TriggerNamespace ParentTriggerNamespace
      {
         set
         {
            mParentTriggerNamespace = value;
         }
         get
         {
            return mParentTriggerNamespace;
         }
      }
      //Trigger mParentTrigger = null;
      //[Browsable(false)]
      //[DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      //public Trigger Trigger
      //{
      //   get
      //   {
      //      return mParentTrigger;
      //   }
      //   set
      //   {
      //      mParentTrigger = value;
      //   }
      //}
      public TriggerVariable GetVariable()
      {
         return mVariable;
      }
      public TriggerValue GetValue()
      {
         return mValue;
      }
      public TriggerComponent GetComponent()
      {
         return mComponent;
      }

      public bool OptionalParameter
      {
         get
         {
            return mVariable.Optional;
         }

      }


      private void UpdateControl()
      {
         if(mValue.IsNull == true)
         {
            TextValue = "-op-" + mVariable.Name;
            ParameterMode = eParameterMode.NullValue;

         }
         else if (mValue.Name != "")
         {
            if(TextValue != mValue.Name)
            {
               mTextValue = mValue.Name;
               if ((mHost == null) && (mUIUpdateRoot == null))
                  UpdateParent();
               if (mUIUpdateRoot != null)
                  mUIUpdateRoot.UIUpdate(mValue, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);
               if (mHost != null)
                  mHost.UIUpdate(mValue, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);
            }

            //TextValue = mValue.Type + ":" + mVariable.Name + mVariable.ID.ToString();
            TextValue = mValue.Name;
            if (mParentTriggerNamespace.IsSharedValueVar(mValue.ID))
            {
               ParameterMode = eParameterMode.GlobalVariable;
            }
            else
            {
               ParameterMode = eParameterMode.LocalVariable;
            }



         }
         else if (mValue.Value == null || mValue.Value == "")
         {
            TextValue = mVariable.Name + TriggerNamespace.GetDefaultVarSuffix() + mVariable.ID.ToString();

            mValue.Name = TextValue;


            if (mParentTriggerNamespace.IsSharedValueVar(mValue.ID))
            {
               ParameterMode = eParameterMode.GlobalVariable;
            }
            else
            {
               ParameterMode = eParameterMode.LocalVariable;
            }
         }
         else
         {
            mbConstBindError = false;
            //    const mode
            if (mHLProp.PresentationValue != null)
            {
               StringDecorator dec;
               if (StringDecorator.TryParse(mHLProp.PresentationValue.ToString(), out dec))
               {
                  object o;
                  int id;
                  string propname;
                  if (TriggerSystemMain.mTriggerDefinitions.IsListType(mValue.Type))
                  {
                     TextValue = dec.mValue;
                  }                                                                                                                                                   
                  else if ("Object" == (mValue.Type) || "Entity" == (mValue.Type) || "Unit" == (mValue.Type) || "Squad" == (mValue.Type) || "Percent" == (mValue.Type) || "Cost" == (mValue.Type) || "DesignLine" == mValue.Type)
                  {
                     TextValue = dec.mValue;
                  }
                  else if (SimGlobals.getSimMain().TryParseObjectPropertyRef(dec.mDecorator, out o, out id, out propname) == true)
                  {
                     TextValue = dec.mValue + "." + propname;
                  }
                  else if ("Float" == (mValue.Type))
                  {
                     TextValue = dec.mDecorator;
                  }
                  else
                  {
                     //TextValue = dec.mValue;
                     TextValue = "Bind Error: " + dec.mValue + "." + propname;
                     mbConstBindError = true;
                  }

                  //TextValue = dec.mValue;
               }
               else
               {
                  TextValue = mHLProp.PresentationValue.ToString();// val.Value;
               }
            }
            else
            {
               TextValue = mValue.Value;
            }
            ParameterMode = eParameterMode.Const;

         }
      }

      public enum eParameterMode
      {
         Const,
         LocalVariable,
         GlobalVariable,
         NullValue
      }

      bool mbConstBindError = false;

      eParameterMode mParameterMode;
      public eParameterMode ParameterMode
      {
         set
         {
            if (mParameterMode != value)
            {
               
            }

            mParameterMode = value;
            if (mParameterMode == eParameterMode.Const)
            {
               if (mbConstBindError == true)
               {
                  this.ForeColor = System.Drawing.Color.Red;
               }
               else
               {
                  TextLabel.ForeColor = Color.Black;
                  //TextLabel.Visible = false;
               }
            }
            else if(mParameterMode == eParameterMode.GlobalVariable)
            {
               TextLabel.ForeColor = Color.Green;
               //TextLabel.Visible = true;
            }
            else if (mParameterMode == eParameterMode.LocalVariable)
            {
               TextLabel.ForeColor = Color.DarkGoldenrod;
               //TextLabel.Visible = true;
            }
            else if(mParameterMode == eParameterMode.NullValue)
            {
               TextLabel.ForeColor = Color.DarkGray;// .Orange;
               //TextLabel.Text = ""
            }
            if (mParameterMode == eParameterMode.NullValue)
            {
               mValue.IsNull = true;
            }
            else
            {
               mValue.IsNull = false;
            }
            TextLabel.Update();
         }
         get
         {
            return mParameterMode;
         }

      }


      string mTextValue = "no value";
      public string TextValue
      {
         set
         {
            TextLabel.Text = value;
            mTextValue = value;
            //this.Size = TextLabel.Size;
            if (this.LabelChanged != null)
               LabelChanged.Invoke(this, null);


         }
         get
         {
            return mTextValue;
         }
      }

      Color mBackGroundColor;
      private void ActiveDragLablel_MouseEnter(object sender, EventArgs e)
      {
         ShowSelected(true);

         //this.BorderStyle = BorderStyle.FixedSingle;
         if (HotSelect != null)
            HotSelect.Invoke(this, null);

         if ((mHost == null) && (mUIUpdateRoot == null))
            UpdateParent();       
         if (mUIUpdateRoot != null)
            mUIUpdateRoot.UIUpdate(mValue, new BasicArgument(BasicArgument.eArgumentType.Select), eUpdateVisibilty.AnyVisiblity);
         if (mHost != null)
            mHost.UIUpdate(mValue, new BasicArgument(BasicArgument.eArgumentType.Select), eUpdateVisibilty.AnyVisiblity);

      }

      private void ActiveDragLablel_MouseLeave(object sender, EventArgs e)
      {
         ShowSelected(false);
         ShowSelectedError(false);

         if (HotSelectEnd != null)
            HotSelectEnd.Invoke(this, null);

         if ((mHost == null) && (mUIUpdateRoot == null))
            UpdateParent();
         if (mUIUpdateRoot != null)
            mUIUpdateRoot.UIUpdate(mValue, new BasicArgument(BasicArgument.eArgumentType.Deselect), eUpdateVisibilty.AnyVisiblity);
         if (mHost != null)
            mHost.UIUpdate(mValue, new BasicArgument(BasicArgument.eArgumentType.Deselect), eUpdateVisibilty.AnyVisiblity);
           
      }

      public void ShowSelected(bool selected)
      {
         if (selected)
         {
            this.BackColor = System.Drawing.Color.LightSteelBlue;
         }
         else
         {
            this.BackColor = System.Drawing.Color.Empty;
         }
      }
      public void ShowSelectedError(bool selected)
      {
         if (selected)
         {
            this.BackColor = System.Drawing.Color.Red;
         }
         else
         {
            this.BackColor = System.Drawing.Color.Empty;
         }
      }

      int lastX = -1;

      private void TriggerParameterControl_MouseMove(object sender, MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Left && (mValue.IsNull == false ))
         {

            if (lastX == -1) // add some filtering to drag drop ops
            {
               lastX = e.X;
            }
            if (Math.Abs(lastX - e.X) > 3)
            {
               DoDragDrop(this, DragDropEffects.All | DragDropEffects.Link);
               lastX = -1;
            }
         }
      }

      bool mbOpen = false;
      private void TextLabel_Click(object sender, EventArgs e)
      {
         PopupEditor();
      }

      TriggerValue mBackupValue = null;
      TriggerVariable mBackupVar = null;

      private void PopupEditor()
      {
         if (mbOpen == true)
            return; //haha

         mBackupValue = this.mValue.GetCopy();
         mBackupVar = this.mVariable.GetCopy();

         mbOpen = true;

         TriggerPopupInputForm f = new TriggerPopupInputForm();
         Point p1 = new Point(0, 0);
         Point p2 = this.PointToScreen(p1);
         f.StartPosition = FormStartPosition.Manual;
         p2.Y -= 30;
         p2.X -= 60;
         f.Location = p2;
         
         f.Init(this, mVariable, mValue, mHLProp);
         f.FormClosed += new FormClosedEventHandler(f_FormClosed);
         f.Show();

         
      }

      void f_FormClosed(object sender, FormClosedEventArgs e)
      {
         mbOpen = false;

         if (((Form)sender).DialogResult == DialogResult.OK)
         {
         }
         else
         {
            mBackupVar.CopyTo(mVariable);
            mBackupValue.CopyTo(mValue);
         }
         UpdateControl();
      }

      HighLevelProperty mHLProp = null;
      VarValueBinder mBinder = null;
      public void InitHLProp()
      {
         mBinder = new VarValueBinder(mVariable, mValue);
         mHLProp = TriggerSystemMain.mTriggerDefinitions.GetHLProperty(mBinder, mParentTriggerNamespace);
         //string temp;
         //Control editor = prop.GetEditor(out temp);
         //editor.Margin = new Padding(0);
         //this.Controls.Add(editor);
      }
      public TriggerValue SetValueMode()
      {
         ParentTriggerNamespace.SwitchVarToConstant(mVariable, ref mValue);
         mValue.IsNull = false;

         mBinder.UpdateValue(mValue);
         //UpdateControl();
         return mValue;
      }
      public TriggerValue SetVarMode(int newValueID)
      {
         //ParentTriggerNamespace.SwitchConstantToVar(mVariable, ref mValue);         
         //newValueID
         //mVariable.ID = newValueID;
         
         //mValue = ParentTriggerNamespace.GetValues()[newValueID];

         //ParentTriggerNamespace.AssignValueToVar(mVariable, mValue); 

         mValue = ParentTriggerNamespace.AssignValueToVar(mVariable, newValueID);
         mValue.IsNull = false;

         mBinder.UpdateValue(mValue);
         //UpdateControl();
         return mValue;
      }
      public TriggerValue SetVarMode()
      {

         ParentTriggerNamespace.SwitchConstantToVar(mVariable, ref mValue);
         mValue.IsNull = false;
         mBinder.UpdateValue(mValue);
         //UpdateControl();
         return mValue;
      }
      public TriggerValue SetNullMode()
      {
         ParentTriggerNamespace.SwitchVarToConstant(mVariable, ref mValue);
         mValue.IsNull = true;
         mBinder.UpdateValue(mValue);
         return mValue;
      }

      private void TextLabel_DragEnter(object sender, DragEventArgs e)
      {
         
         e.Effect = DragDropEffects.None;
         if (e.Data.GetDataPresent(typeof(TriggerParameterControl)))
         {
            object data = e.Data.GetData(typeof(TriggerParameterControl));
            TriggerParameterControl otherButton = data as TriggerParameterControl;
            if (otherButton != null)
            {
               TriggerValue ctrlMove = otherButton.mValue as TriggerValue;
               TriggerValue ctrlTarget = this.mValue as TriggerValue;

               if (ctrlMove.Type == ctrlTarget.Type)
                  e.Effect = DragDropEffects.Move;

               if ((otherButton.ParameterMode == eParameterMode.Const) 
               && (TriggerSystemMain.mTriggerDefinitions.CanConvertConst(ctrlMove.Type, ctrlTarget.Type)))
               {
                  e.Effect = DragDropEffects.Link;
               }
            }

         }
      }

      



      private void TextLabel_DragDrop(object sender, DragEventArgs e)
      {
         object data = e.Data.GetData(typeof(TriggerParameterControl));
         TriggerParameterControl otherButton = data as TriggerParameterControl;
         if (otherButton != null)
         {
            TriggerVariable ctrlMove = otherButton.mVariable as TriggerVariable;
            TriggerVariable ctrlTarget = this.mVariable as TriggerVariable;

            if (e.Effect == DragDropEffects.Move)
            {
               //if (TriggerSystemMain.mTriggerDefinitions.IsListType(otherButton.mValue.Type) && otherButton.ParameterMode == eParameterMode.Const)
               //{
               //   SetVarMode(ctrlMove.ID);

               //}                             
               /*else*/   if (otherButton.ParameterMode == eParameterMode.Const)
               {
                  SetValueMode();
                  mValue.Value = otherButton.mValue.Value;

               }
               else
               {
                  SetVarMode(ctrlMove.ID);
                  //ctrlTarget.ID = ctrlMove.ID;
                  //mValue = otherButton.mValue;

                  //mBinder.UpdateValue(mValue);
                  //ParentTriggerNamespace.ProcessVarMapping();
                  if ((mHost == null) && (mUIUpdateRoot == null))
                     UpdateParent();
                  if (mUIUpdateRoot != null)
                     mUIUpdateRoot.UIUpdate(mValue, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);
                  if (mHost != null)
                     mHost.UIUpdate(mValue, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);
      
               }
               UpdateControl();        
            }
            if (e.Effect == DragDropEffects.Link)
            {
               if (otherButton.ParameterMode == eParameterMode.Const)
               {

                  SetValueMode();
                  mValue.Value = TriggerSystemMain.mTriggerDefinitions.ConvertConst(otherButton.mValue.Value, otherButton.mValue.Type, this.mValue.Type).ToString();
                  UpdateControl();
               }

            }
         }
      }





      //Form mPopupEditForm = null;
      //bool mbOpen = false;
      //private void PopupEditor(Control c)
      //{

      //   mPopupEditForm = new Form();
      //   mPopupEditForm.FormBorderStyle = FormBorderStyle.None;
      //   mPopupEditForm.Controls.Add(c);
      //   mPopupEditForm.Size = c.Size;
         

      //   //mPopupEditForm.StartPosition = FormStartPosition.
      //   mPopupEditForm.StartPosition = FormStartPosition.Manual;
      //   //Point p1 = new Point(25, 25);
      //   Point p1 = new Point(0, 0);
      //   Point p2 = this.PointToScreen(p1);

      //   Rectangle r = System.Windows.Forms.Screen.PrimaryScreen.Bounds;

      //   //if (r.Height < p2.Y + 400)
      //   //{
      //   //   p2.Y = r.Height - 400;
      //   //}
      //   //if (r.Width < p2.X + 800)
      //   //{
      //   //   p2.X = r.Width - 800;
      //   //}
      //   mPopupEditForm.Location = p2;
      //   mPopupEditForm.Deactivate += new EventHandler(mPopupEditForm_Deactivate);
      //   mPopupEditForm.FormClosed += new FormClosedEventHandler(mPopupEditForm_FormClosed);
      //   mPopupEditForm.Show(this);
      //}

      //void mPopupEditForm_FormClosed(object sender, FormClosedEventArgs e)
      //{
      //   //throw new Exception("The method or operation is not implemented.");
      //   mbOpen = false;
      //}
      //void mPopupEditForm_Deactivate(object sender, EventArgs e)
      //{
      //   mbOpen = false;
      //   ((Form)sender).Close();
      //}



      #region ITriggerUIUpdate Members


      //refactor this to be more strait forward.
      public TriggerHostArea mHost = null;
      ITriggerUIUpdateRoot mUIUpdateRoot = null;
      protected void UpdateParent()
      {
         //bad...
         Control chain = Parent;
         NodeHostControl host = null;
         while(chain != null && (host == null) && (mUIUpdateRoot == null))
         {
            if(chain is NodeHostControl)
            {
               NodeHostControl nh = (NodeHostControl)chain;
               mHost = (TriggerHostArea)nh.Owner;
            }
            else if (chain is ITriggerUIUpdateRoot)
            {
               mUIUpdateRoot = (ITriggerUIUpdateRoot)chain;
            }
            chain = chain.Parent;
         }
      }

      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity)
      {
         List<Control> notused = new List<Control>();
         UIUpdate(data, arguments, visiblity, ref notused);
      }
      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity, ref List<Control> owners)
      {        
         BasicArgument ba = arguments as BasicArgument;
         if (data == null)
         {

         }
         else if (data is TriggerValue && (((TriggerValue)data).ID == mValue.ID) && (ba != null))
         {
            if(ba.mArgument == BasicArgument.eArgumentType.Refresh)
            {
               UpdateControl();
            }
            else if(ba.mArgument == BasicArgument.eArgumentType.Select)
            {
               ShowSelected(true);
            }
            else if(ba.mArgument == BasicArgument.eArgumentType.Deselect)
            {
               ShowSelected(false);

            }
            else if (ba.mArgument == BasicArgument.eArgumentType.Search)
            {
               owners.Add(this);
            }
            else if (ba.mArgument == BasicArgument.eArgumentType.HighlightError)
            {
               ShowSelectedError(true);
            }

         }
         else if (data is TriggerVariable && (((TriggerVariable)data) == mVariable) && (ba != null))
         {
            if (ba.mArgument == BasicArgument.eArgumentType.Search)
            {
               owners.Add(this);
            }
         }
         else if (data is TriggerVariable && (ba != null))
         {
            TriggerVariable searchvar = data as TriggerVariable;
            //This ain't quite right..  it just mostly matches but lacks its own id
            if (searchvar.ID == mVariable.ID && searchvar.Name == mVariable.Name && searchvar.SigID == mVariable.SigID)
            {
               if (ba.mArgument == BasicArgument.eArgumentType.Search)
               {
                  owners.Add(this);
               } 
            }
         }

      }

      #endregion
   }
}
