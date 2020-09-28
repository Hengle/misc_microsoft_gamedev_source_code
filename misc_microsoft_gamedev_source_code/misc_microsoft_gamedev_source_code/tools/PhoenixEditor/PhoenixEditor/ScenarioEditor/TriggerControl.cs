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
   public partial class TriggerControl : PhoenixEditor.ScenarioEditor.ClientNodeControl, ISelectable, IControlPointOwner, IDeletable, ICopyPaste, ITriggerUIUpdate, ICommentOutable
   {
      public TriggerControl()
      {
         InitUI();

         //SetStyle(ControlStyles.UserPaint | ControlStyles.DoubleBuffer | ControlStyles.AllPaintingInWmPaint, true);
         SetStyle(ControlStyles.OptimizedDoubleBuffer , true);
      }
      public TriggerControl(NodeHostControl nodeHost, TriggerHostArea Host)// NodeHostControl
      {
         BindToHost(nodeHost, Host);

         InitUI();
 
      }

      protected void InitUI()
      {
         InitializeComponent();
         effectsList1.NeedsResize += new EventHandler(event_NeedsResize);
         effectsList2.NeedsResize += new EventHandler(event_NeedsResize);
         conditionsList1.NeedsResize += new EventHandler(event_NeedsResize);
         AddDragSurface(this.panel1);
         AddDragSurface(this.TriggerIDLabel);
         AddDragSurface(this.TriggerTitleLabel);
         conditionsList1.RenderText = "?";
         effectsList1.RenderText = "T";
         effectsList2.RenderText = "F";
         this.Width = this.Width - 80;// 100;     


         mLastBackColor = this.tableLayoutPanel1.BackColor;

      }

      #region ITriggerComponent Members

      public void BindToHost(NodeHostControl nodeHost, TriggerHostArea Host)
      {  
         mHost = nodeHost;
         mTriggerHost = Host;
      }

      #endregion


      TriggerHostArea mTriggerHost;

      void event_NeedsResize(object sender, EventArgs e)
      {
         RecaluculateSize();
      }

      public bool SetConditional(bool conditional)
      {

         if(conditional == false)
         {
            if(Trigger.TriggerEffectsFalse.Effects.Count > 0)
            {
               //if(Trigger.TriggerEffectsFalse.Effects[0].Type == 
               {               
                  if(MessageBox.Show("Non conditional triggers can't have on false effects","Remove them now?", MessageBoxButtons.OKCancel) != DialogResult.OK)
                  {
                     return true; //true, because we failed to set it to false...
                  }
               }
               effectsList2.ClearItems();
               RecaluculateSize();
            }
            mEffectFalseControlPoint.Visible = false;


            effectsList2.Visible = false;
            tableLayoutPanel1.SetRowSpan(effectsList1, 2);
    
         }
         else         
         {
            mEffectFalseControlPoint.Visible = true;

            effectsList2.Visible = true;
            tableLayoutPanel1.SetRowSpan(effectsList1, 1);


         }

         return conditional;
      }



      TriggerNamespace mParentTriggerNamespace = null;
      public TriggerNamespace ParentTriggerNamespace
      {
         set
         {
            mParentTriggerNamespace = value;
            this.conditionsList1.ParentTriggerNamespace = mParentTriggerNamespace;
            this.effectsList1.ParentTriggerNamespace = mParentTriggerNamespace;
            this.effectsList2.ParentTriggerNamespace = mParentTriggerNamespace;
            this.effectsList2.EffectsOnFalse = true;

         }
         get
         {
            return mParentTriggerNamespace;
         }
      }
      BasicControlPoint mActivateControlPoint;
      BasicControlPoint mDeActivateControlPoint;
      BasicControlPoint mEffectTrueControlPoint ;
      BasicControlPoint mEffectFalseControlPoint;


      Trigger mThisTrigger = null;
      public Trigger Trigger
      {
         get
         {
            return mThisTrigger;
         }
         set
         {
            mThisTrigger = value;
            this.conditionsList1.Trigger = mThisTrigger;
            this.effectsList1.Trigger = mThisTrigger;
            this.effectsList2.Trigger = mThisTrigger;
            this.effectsList2.EffectsOnFalse = true;

            this.effectsList1.Name = mThisTrigger.ID.ToString();
            this.effectsList2.Name = mThisTrigger.ID.ToString();
            //this.VarOutputHardPointsBar.InitialSetup();
            //this.VarInputHardPointsBar.InitialSetup();
            //this.TriggerInHardPointsBar.InitialSetup();
            //this.TriggerOutHardPointsBar.InitialSetup();


            mActivateControlPoint = new BasicControlPoint(mHost);
            mActivateControlPoint.TargetKey = "none";// "Trigger.Link.Back";
            mActivateControlPoint.TypeKey = "Trigger.Link.Forward";
            mActivateControlPoint.BackColor = Color.Green;//b.GetColor();
            mActivateControlPoint.SetToolTipText("Activate");
            mActivateControlPoint.SetDirection(new Point(-250, 0));
            mActivateControlPoint.Tag = this;
            mActivateControlPoint.TagObject = this;
            mActivateControlPoint.Name = "Activate";
            mActivateControlPoint.RenderText = "on";
            //mActivateControlPoint.BackgroundImage = SharedResources.GetImage(@"Icons\TriggerEditor\Activate.bmp");
            mActivateControlPoint.ControlPointDescription = this.Trigger.Name + "." + "Activate";
            TriggerInHardPointsBar.AddControl(mActivateControlPoint);


            Panel p1 = new Panel();
            p1.BackColor = Color.White;
            p1.Width = 25;
            p1.Height = 50;
            TriggerInHardPointsBar.AddControl(p1);

            mDeActivateControlPoint = new BasicControlPoint(mHost);
            mDeActivateControlPoint.TargetKey = "none";// "Trigger.Link.Back";
            mDeActivateControlPoint.TypeKey = "Trigger.Link.Forward";
            mDeActivateControlPoint.BackColor = Color.Red;//b.GetColor();
            mDeActivateControlPoint.SetToolTipText("Deactivate");
            mDeActivateControlPoint.SetDirection(new Point(-250, 0));
            mDeActivateControlPoint.Tag = this;
            mDeActivateControlPoint.TagObject = this;
            mDeActivateControlPoint.Name = "Deactivate";
            mDeActivateControlPoint.RenderText = "off";
            //mDeActivateControlPoint.BackgroundImage = SharedResources.GetImage(@"Icons\TriggerEditor\Deactivate.bmp");
            mDeActivateControlPoint.ControlPointDescription = this.Trigger.Name + "." + "Deactivate";
            TriggerInHardPointsBar.AddControl(mDeActivateControlPoint);

            mEffectTrueControlPoint = new BasicControlPoint(mHost);
            mEffectTrueControlPoint.TargetKey = "Trigger.Link.Forward";
            mEffectTrueControlPoint.TypeKey = "none";//"Trigger.Link.Back";
            mEffectTrueControlPoint.BackColor = Color.LightSkyBlue;
            mEffectTrueControlPoint.SetToolTipText("Effect.True");
            mEffectTrueControlPoint.SetDirection(new Point(250, 0));
            mEffectTrueControlPoint.Tag = this;
            mEffectTrueControlPoint.TagObject = this;
            mEffectTrueControlPoint.Name = "Effect.True";
            mEffectTrueControlPoint.RenderText = "T";
            //mEffectTrueControlPoint.BackgroundImage = SharedResources.GetImage(@"Icons\TriggerEditor\True.bmp");
            mEffectTrueControlPoint.ControlPointDescription = this.Trigger.Name + "." + "Effect.True";
            mEffectTrueControlPoint.ControlPointConnected += new BasicControlPoint.ControlPointEvent(mTriggerOutControlPoint_ControlPointConnected);
            mEffectTrueControlPoint.ControlPointRemoved += new BasicControlPoint.ControlPointEvent(mTriggerOutControlPoint_ControlPointRemoved);
            TriggerOutHardPointsBar.AddControl(mEffectTrueControlPoint);


            Panel p2 = new Panel();
            p2.BackColor = Color.White;
            p2.Width = 25;
            p2.Height = 50;
            TriggerOutHardPointsBar.AddControl(p2);

            mEffectFalseControlPoint = new BasicControlPoint(mHost);
            mEffectFalseControlPoint.TargetKey = "Trigger.Link.Forward";
            mEffectFalseControlPoint.TypeKey = "none";//"Trigger.Link.Back";
            mEffectFalseControlPoint.BackColor = Color.LightSalmon;
            mEffectFalseControlPoint.SetToolTipText("Effect.False");
            mEffectFalseControlPoint.SetDirection(new Point(250, 0));
            mEffectFalseControlPoint.Tag = this;
            mEffectFalseControlPoint.TagObject = this;
            mEffectFalseControlPoint.Name = "Effect.False";
            mEffectFalseControlPoint.RenderText = "F";
            //mEffectFalseControlPoint.BackgroundImage = SharedResources.GetImage(@"Icons\TriggerEditor\False.bmp");
            mEffectFalseControlPoint.ControlPointDescription = this.Trigger.Name + "." + "Effect.False";
            mEffectFalseControlPoint.ControlPointConnected += new BasicControlPoint.ControlPointEvent(mTriggerOutControlPoint_ControlPointConnected);
            mEffectFalseControlPoint.ControlPointRemoved += new BasicControlPoint.ControlPointEvent(mTriggerOutControlPoint_ControlPointRemoved);
            TriggerOutHardPointsBar.AddControl(mEffectFalseControlPoint);

            mControlPointList.Add(mActivateControlPoint);
            mControlPointList.Add(mDeActivateControlPoint);
            mControlPointList.Add(mEffectTrueControlPoint);
            mControlPointList.Add(mEffectFalseControlPoint);

            SetConditional(mThisTrigger.ConditionalTrigger);

            //set ui...\
            this.TriggerTitleLabel.Text = mThisTrigger.Name;
            UpdateVisuals();

            //load up the rest of the trigger...

            List<TriggerCondition> conditionList = mParentTriggerNamespace.WalkConditions(mThisTrigger.TriggerConditions.Child);
            foreach (TriggerCondition c in conditionList)
            {
               this.conditionsList1.AddExistingConditionToUI(c, mParentTriggerNamespace.GetValues());
            }

            List<TriggerEffect> effectList = mThisTrigger.TriggerEffects.Effects;
            foreach (TriggerEffect e in effectList)
            {
               if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName())
               {
                  continue;
               }
               if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
               {
                  continue;
               }      
               this.effectsList1.AddExistingEffectToUI(e, mParentTriggerNamespace.GetValues());
            }

            List<TriggerEffect> effectsOnFalse = mThisTrigger.TriggerEffectsFalse.Effects;
            foreach (TriggerEffect e in effectsOnFalse)
            {
               if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName())
               {
                  continue;
               }
               if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
               {
                  continue;
               } 
               this.effectsList2.AddExistingEffectToUI(e, mParentTriggerNamespace.GetValues());
            }    
                      
         }
      }

      public override int GetGroupID() { return mThisTrigger.GroupID; }
      public override void SetGroupID(int id) { mThisTrigger.GroupID = id; }


      public override string ToString()
      {
         if(this.Trigger != null)
         {
            return this.Trigger.ID.ToString();
         }

         return "-1";
         //return base.ToString();
      }

      public void LoadExistingConnections()
      {
         List<TriggerEffect> effectList = mThisTrigger.TriggerEffects.Effects;

         List<TriggerEffect> hitlist = new List<TriggerEffect>();
         foreach (TriggerEffect e in effectList)
         {
            if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName())
            {
               //Hide it... and load it as a line
               //mTriggerOutControlPoint.
               int targetTrigger = System.Convert.ToInt32(ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value);
               TriggerControl targetC = mTriggerHost.GetTriggerControl(targetTrigger);
               //mEffectTrueControlPoint.GetTargets().Add(targetC.GetActivationPoint());
               if (targetC == null)
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning("Error Remapping link to trigger " + targetTrigger.ToString());
                  hitlist.Add(e);
               }
               else
               {
                  mEffectTrueControlPoint.GetTargets().Add(targetC.GetActivationPoint());

               }

               continue;
            }
            if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
            {
               int targetTrigger = System.Convert.ToInt32(ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value);
               TriggerControl targetC = mTriggerHost.GetTriggerControl(targetTrigger);
               //mEffectTrueControlPoint.GetTargets().Add(targetC.GetDeactivationPoint());
               if (targetC == null)
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning("Error Remapping link to trigger " + targetTrigger.ToString());
                  hitlist.Add(e);

               }
               else
               {
                  mEffectTrueControlPoint.GetTargets().Add(targetC.GetDeactivationPoint());

               }

               continue;
            }


         }

         foreach (TriggerEffect e in hitlist)
         {
            effectList.Remove(e);
         }
         hitlist.Clear();

         List<TriggerEffect> effectsOnFalse = mThisTrigger.TriggerEffectsFalse.Effects;
         foreach (TriggerEffect e in effectsOnFalse)
         {
            if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName())
            {
               //Hide it... and load it as a line
               //mTriggerOutControlPoint.
               int targetTrigger = System.Convert.ToInt32(ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value);
               TriggerControl targetC = mTriggerHost.GetTriggerControl(targetTrigger);
               if (targetC == null)
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning("Error Remapping link to trigger " + targetTrigger.ToString());
                  hitlist.Add(e);

               }
               else
               {
                  mEffectFalseControlPoint.GetTargets().Add(targetC.GetActivationPoint());               
               }
               continue;
            }
            if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
            {
               int targetTrigger = System.Convert.ToInt32(ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value);
               TriggerControl targetC = mTriggerHost.GetTriggerControl(targetTrigger);
               //mEffectFalseControlPoint.GetTargets().Add(targetC.GetDeactivationPoint());
               if (targetC == null)
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning("Error Remapping link to trigger " + targetTrigger.ToString());
                  hitlist.Add(e);

               }
               else
               {
                  mEffectFalseControlPoint.GetTargets().Add(targetC.GetDeactivationPoint());
                 
               }

               continue;
            }               
         }
         foreach (TriggerEffect e in hitlist)
         {
            effectsOnFalse.Remove(e);
         }
      }
      public BasicControlPoint GetActivationPoint()
      {
         return mActivateControlPoint;
      }
      public BasicControlPoint GetDeactivationPoint()
      {
         return mDeActivateControlPoint;
      }
      public BasicControlPoint GetEffectFalsePoint()
      {
         return mEffectFalseControlPoint;
      }
      public BasicControlPoint GetEffectTruePoint()
      {
         return mEffectTrueControlPoint;
      } 

      void mTriggerOutControlPoint_ControlPointConnected(BasicControlPoint cp, IControlPoint other)
      {
         //set target value:
         TriggerControl t = other.TagObject as TriggerControl;

         if (t != null)
         {
            string effectName = "";

            if (other.GetName() == "Activate")
            {

               effectName = TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName();
            }
            if(other.GetName() == "Deactivate")
            {
               effectName = TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName();
            }

            TriggerEffect effect;
            Dictionary<int, TriggerValue> values;
            TriggerSystemMain.mTriggerDefinitions.GetTriggerEffect(effectName, out effect, out values);
            Dictionary<int, TriggerValue>.Enumerator it = values.GetEnumerator();
            while (it.MoveNext())
            {
               it.Current.Value.Value = t.Trigger.ID.ToString();
               break;
            }
            int newID;

            bool onfalse = (cp.GetName() == "Effect.False")?true:false;

            ParentTriggerNamespace.InsertEffect(Trigger.ID, effect, values, onfalse, out newID);
            return;//Finished
         }

         //else.. template object         
         TemplateControl templateControl = other.TagObject as TemplateControl;
         if (templateControl != null)
         {
            string name = other.ToString();
            foreach (TriggersTemplateInputActionBinder binder in templateControl.TriggerTemplateMapping.TriggerInputs)
            {
               if (binder.Name == name)
               {
                  foreach(TriggerBindInfo b in binder.TargetIDs)
                  {
                     if (b.ID == templateControl.TriggerTemplateMapping.ID && b.LinkName == other.ToString())
                     {
                        CoreGlobals.getErrorManager().OnSimpleWarning("can't bind. template is already bound to trigger.  this is a bug");
                        return;
                     }
                  }
                  TriggerBindInfo bindInfo = new TriggerBindInfo();
                  bindInfo.SetTarget(this.Trigger.ID, cp.ToString());
                  binder.TargetIDs.Add(bindInfo);
                  return;
               }
            }
         }


      }

      void mTriggerOutControlPoint_ControlPointRemoved(BasicControlPoint cp, IControlPoint other)
      {

         TriggerControl otherctrl = other.TagObject as TriggerControl;
         if (otherctrl != null)
         {
            bool onfalse = (cp.GetName() == "Effect.False") ? true : false;
            TriggerEffect toremove = null;

            if (onfalse == false)
            {
               foreach (TriggerEffect effect in Trigger.TriggerEffects.Effects)
               {
                  if ((effect.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
                  || (effect.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName()))
                  {
                     TriggerValue val = ParentTriggerNamespace.GetValues()[effect.Parameter[0].ID];
                     if (val.Value == otherctrl.Trigger.ID.ToString())
                     {
                        toremove = effect;
                     }
                  }
               }
            }
            if (onfalse == true)
            {
               foreach (TriggerEffect effect in Trigger.TriggerEffectsFalse.Effects)
               {
                  if ((effect.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
                  || (effect.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName()))
                  {
                     TriggerValue val = ParentTriggerNamespace.GetValues()[effect.Parameter[0].ID];
                     if (val.Value == otherctrl.Trigger.ID.ToString())
                     {
                        toremove = effect;
                     }
                  }
               }
            }
            if (toremove != null)
            {
               ParentTriggerNamespace.DeleteEffect(Trigger, toremove, onfalse);
            }
         }

         TemplateControl templateControl = other.TagObject as TemplateControl;
         if (templateControl != null)
         {

            string name = other.ToString();
            BasicControlPoint othercp = other as BasicControlPoint;
            TriggersTemplateInputActionBinder targetBinder = templateControl.mInputActionCPs[othercp];
            TriggerBindInfo toremove = null;
            string linkName = cp.Name;
            int linkID = this.Trigger.ID;

            foreach (TriggerBindInfo b in targetBinder.TargetIDs)
            {
               if ((b.LinkName == linkName) && (b.ID == linkID ))
               {
                  toremove = b;
               }
            }
            if(toremove != null)
            {            
               targetBinder.TargetIDs.Remove(toremove);
            }
            else
            {
               CoreGlobals.getErrorManager().LogErrorToNetwork("mTriggerOutControlPoint_ControlPointRemoved: error deleting template link");
            }
         }

      }  

      private void CleanUpConnectionPoints()
      {



      }


      public void OldRecaluculateSize()
      {
         int conditionHeight = conditionsList1.RecaluculateSize();
         //int conditionHeight = conditionsList1.Height;

         int effectsHeight = effectsList1.RecaluculateSize();
         //int effectsHeight = effectsList1.Height;
         
         //if(Trigger.ConditionalTrigger == true)
         {
            int effectsOnFalseHeight = effectsList2.RecaluculateSize();
            //effectsList2.Height = effectsOnFalseHeight;

            //tableLayoutPanel1.RowStyles[3].Height = effectsOnFalseHeight;
            //tableLayoutPanel1.RowStyles[3].Height = effectsOnFalseHeight;

            effectsHeight += effectsOnFalseHeight;
         }

         tableLayoutPanel1.RowStyles[4].Height = 17;

         int newHeight = Math.Max(conditionHeight, effectsHeight);
         if (newHeight <= 40)
            newHeight += 115;
         else
            newHeight += 65;

         this.Height = newHeight;
                
      }


      public void RecaluculateSize()
      {
         int conditionHeight = conditionsList1.RecaluculateSize();
 
         int effectsHeight = effectsList1.RecaluculateSize();

         if(Trigger.ConditionalTrigger == true)
         {
            int effectsOnFalseHeight = effectsList2.RecaluculateSize();
            effectsHeight += effectsOnFalseHeight;
            if (effectsOnFalseHeight <= 10)
            {
               effectsHeight += 5;
            }
            else
            {
               effectsHeight += 10;
            }
         }

         tableLayoutPanel1.RowStyles[4].Height = 2;

         int newHeight = Math.Max(conditionHeight, effectsHeight);

         newHeight = Math.Max(conditionsList1.Height, newHeight) + 30;
         //if (newHeight <= 15)
         //   newHeight += 115-20;
         //else if (newHeight <= 40)
         //   newHeight += 115 - 35;
         //else if (newHeight <= 55)
         //   newHeight += 90 - 25;
         //else
         //   newHeight += 65-25;

         this.Height = newHeight;

      }

      public void UpdateData()
      {


      }

      Color mLastBackColor = Color.Empty;
      public void UpdateVisuals()
      {
         this.TriggerIDLabel.Text = mThisTrigger.ID.ToString() + ((Trigger.Active) ? "+A" : "") /*+ ((Trigger.StayActiveOnFire) ? "+SA" : "")*/ + ((OrCondition) ? "+OR" : "") + ((Trigger.EvalLimit != 0) ? "+ONCE" : "");
         TriggerTitleLabel.Text = Trigger.Name;

         TriggerTitleLabel.Left = TriggerIDLabel.Right + 5;

         if(Trigger.Active == true)
         {
            this.panel1.BackColor = System.Drawing.Color.LightGreen;

         }
         else
         {
            this.panel1.BackColor = System.Drawing.SystemColors.ControlLight;
         }


         if (Trigger.CommentOut == true)
         {
            this.tableLayoutPanel1.BackColor = Color.DarkGray;
         }
         else
         {
            this.tableLayoutPanel1.BackColor = mLastBackColor;
         }
      }


      ContextMenu mOptionsMenu = null;
      //    <Trigger ID="1" Name="GetPlayer" Active="true" StayActiveOnFire="false" EvaluateFrequency="0" ConditionalTrigger="false">
      public void InitMenu()
      {
         mOptionsMenu = new ContextMenu();


         MenuItem triggerActiveItem = new MenuItem("Active");
         triggerActiveItem.Checked = Trigger.Active;
         triggerActiveItem.Click += new EventHandler(triggerActiveItem_Click);
         mOptionsMenu.MenuItems.Add(triggerActiveItem);

         MenuItem triggerOnceItem = new MenuItem("Only Once per Frame");
         triggerOnceItem.Checked = (Trigger.EvalLimit == 0)?false:true;
         triggerOnceItem.Click += new EventHandler(triggerOnceItem_Click);
         mOptionsMenu.MenuItems.Add(triggerOnceItem);

         //MenuItem triggerStayActiveItem = new MenuItem("Stay Active on Fire");
         //triggerStayActiveItem.Checked = Trigger.StayActiveOnFire;
         //triggerStayActiveItem.Click += new EventHandler(triggerStayActiveItem_Click);
         //mOptionsMenu.MenuItems.Add(triggerStayActiveItem);

         MenuItem triggerConditionalItem = new MenuItem("Conditional Trigger");
         triggerConditionalItem.Checked = Trigger.ConditionalTrigger;
         triggerConditionalItem.Click += new EventHandler(triggerConditionalItem_Click);
         mOptionsMenu.MenuItems.Add(triggerConditionalItem);

         MenuItem orConditionsItem = new MenuItem("OR Conditions");
         orConditionsItem.Checked = OrCondition;// Trigger.TriggerConditions.Child is TriggersTriggerOR;
         orConditionsItem.Click += new EventHandler(orConditionsItem_Click);
         mOptionsMenu.MenuItems.Add(orConditionsItem);

         mOptionsMenu.MenuItems.Add("-");

         MenuItem commentOutItem = new MenuItem("Comment Out");
         commentOutItem.Checked = Trigger.CommentOut;
         commentOutItem.Click += new EventHandler(commentOutItem_Click);
         mOptionsMenu.MenuItems.Add(commentOutItem);

         mOptionsMenu.MenuItems.Add("-");
        
         MenuItem deleteTriggerItem = new MenuItem("Delete Trigger");
         deleteTriggerItem.Click += new EventHandler(deleteTriggerItem_Click);
         mOptionsMenu.MenuItems.Add(deleteTriggerItem);

      }

      


      void commentOutItem_Click(object sender, EventArgs e)
      {
         Trigger.CommentOut = !Trigger.CommentOut;
         ((MenuItem)(sender)).Checked = Trigger.CommentOut;
         UpdateVisuals();
      }

      void triggerOnceItem_Click(object sender, EventArgs e)
      {
         if(Trigger.EvalLimit == 0)
         {
            Trigger.EvalLimit = 1;
            ((MenuItem)(sender)).Checked = true;
         }
         else
         {
            Trigger.EvalLimit = 0;
            ((MenuItem)(sender)).Checked = false;
         }
         UpdateVisuals();
      }

      bool OrCondition
      {
         get
         {
            return Trigger.TriggerConditions.Child is TriggersTriggerOR;
         }

      }


      void orConditionsItem_Click(object sender, EventArgs e)
      {
         bool orConditionState = !((MenuItem)(sender)).Checked;
         ((MenuItem)(sender)).Checked = orConditionState;

         if (orConditionState)
         {
            TriggersTriggerAnd mainAnd = Trigger.TriggerConditions.Child as TriggersTriggerAnd;
            TriggersTriggerOR newOr = new TriggersTriggerOR();
            if(mainAnd != null)
               newOr.Children.AddRange(mainAnd.Children);
            Trigger.TriggerConditions.Child = newOr;

         }
         else
         {
            TriggersTriggerOR mainOr = Trigger.TriggerConditions.Child as TriggersTriggerOR;
            TriggersTriggerAnd newAnd = new TriggersTriggerAnd();
            if(mainOr != null)
               newAnd.Children.AddRange(mainOr.Children);
            Trigger.TriggerConditions.Child = newAnd;
         }
         UpdateVisuals();
   
      }

      void deleteTriggerItem_Click(object sender, EventArgs e)
      {
         if(MessageBox.Show(this.Trigger.Name, "Delete Trigger?",  MessageBoxButtons.OKCancel) == DialogResult.OK)
         {
            Delete();
         }
      }

      public void Delete()
      {
         ParentTriggerNamespace.DeleteTrigger(this.Trigger);
         this.Parent.Controls.Remove(this);
         List<IControlPoint> cps = this.GetControlPoints();
         foreach (IControlPoint cp in cps)
         {
            cp.MarkForDelete = true;
         }
         this.mTriggerHost.Update();
      }

      void triggerConditionalItem_Click(object sender, EventArgs e)
      {
         Trigger.ConditionalTrigger = !Trigger.ConditionalTrigger;
         Trigger.ConditionalTrigger = SetConditional(Trigger.ConditionalTrigger);
         ((MenuItem)(sender)).Checked = Trigger.ConditionalTrigger;
         UpdateVisuals();         
         RecaluculateSize();

      }

      //void triggerStayActiveItem_Click(object sender, EventArgs e)
      //{
      //   Trigger.StayActiveOnFire = !Trigger.StayActiveOnFire;
      //   ((MenuItem)(sender)).Checked = Trigger.StayActiveOnFire;
      //   UpdateVisuals();
      //}

      void triggerActiveItem_Click(object sender, EventArgs e)
      {
         Trigger.Active = !Trigger.Active;
         ((MenuItem)(sender)).Checked = Trigger.Active;
         UpdateVisuals();
      }




      bool mbMinimized = false;
      int mLastHeight;
      float mLastRowHeight;


      private void ResizeButton_Click(object sender, EventArgs e)
      {
         mbMinimized = !mbMinimized;
         if (mbMinimized == true)
         {
            mLastRowHeight = this.tableLayoutPanel1.RowStyles[3].Height;
            this.tableLayoutPanel1.RowStyles[3].Height = 0;
            this.tableLayoutPanel1.RowStyles[2].Height = 0;
            mLastHeight = this.Height;
            this.Height = this.panel1.Height + 20;
            this.ResizeButton.Text = "^";
         }
         else
         {
            this.Height = mLastHeight;
            //this.tableLayoutPanel1.RowStyles[3].Height = mLastRowHeight;// 50;
            //this.tableLayoutPanel1.RowStyles[4].Height = 17;
            this.ResizeButton.Text = "V";
            RecaluculateSize();
         }
         this.tableLayoutPanel1.Refresh();
      }

      private void panel1_MouseUp(object sender, MouseEventArgs e)
      {
         
         if(e.Button == MouseButtons.Right)
         {
            if(mOptionsMenu == null)
            {
               InitMenu();
            }
            mOptionsMenu.Show(panel1, new Point(e.X, e.Y));
         }
      }

      Font mUnderlineFont = null; 
      Font mNormalFont = null; 

      private void TriggerTitleLabel_MouseEnter(object sender, EventArgs e)
      {
         if(mUnderlineFont == null)
         {
            mUnderlineFont = new Font(TriggerTitleLabel.Font, FontStyle.Underline | FontStyle.Bold);
            mNormalFont = new Font(TriggerTitleLabel.Font, FontStyle.Bold); 
         }

         TriggerTitleLabel.Font = mUnderlineFont;
      }

      private void TriggerTitleLabel_MouseLeave(object sender, EventArgs e)
      {
         if (mUnderlineFont == null)
         {
            mUnderlineFont = new Font(TriggerTitleLabel.Font, FontStyle.Underline | FontStyle.Bold);
            mNormalFont = new Font(TriggerTitleLabel.Font, FontStyle.Bold);
         }
         TriggerTitleLabel.Font = mNormalFont;

      }

      bool mbIgnoreRenameOnce = false;
      public override void OnMoved()
      {
         mbIgnoreRenameOnce = true;

         this.Trigger.X = this.Location.X - mHost.AutoScrollPosition.X;
         this.Trigger.Y = this.Location.Y - mHost.AutoScrollPosition.Y;
      }


      private void TriggerTitleLabel_Click(object sender, EventArgs e)
      {
         if (mbIgnoreRenameOnce)
         {
            mbIgnoreRenameOnce = false;
            return;
         }

         ReNameTextBox.Visible = true;
         ReNameCancelButton.Visible = true;
         ReNameOKButton.Visible = true;
         ReNameTextBox.Text = Trigger.Name;

         ReNameTextBox.Focus();
      }

      private void ReNameCancelButton_Click(object sender, EventArgs e)
      {
         ReNameTextBox.Visible = false;
         ReNameCancelButton.Visible = false;
         ReNameOKButton.Visible = false;
      }

      private void ReNameOKButton_Click(object sender, EventArgs e)
      {
         RenameOK();
      }
      private void RenameOK()
      {
         ReNameTextBox.Visible = false;
         ReNameCancelButton.Visible = false;
         ReNameOKButton.Visible = false;

         if (ReNameTextBox.Text == "")
         {
            ReNameTextBox.Text = "noname";
         }

         Trigger.Name = ReNameTextBox.Text;

         UpdateVisuals();
      }

      private void ReNameTextBox_KeyPress(object sender, KeyPressEventArgs e)
      {
         if (e.KeyChar == (char)13)
         {
            RenameOK();
            e.Handled = true;
         }
         else
         {
            base.OnKeyPress(e);
         }     
      }




      #region ISelectable Members
      void ISelectable.SelectControl()
      {
         this.BorderStyle = BorderStyle.FixedSingle;
         mbSelected = true;
      }

      void ISelectable.DeSelectControl()
      {
         this.BorderStyle = BorderStyle.None;
         mbSelected = false;
      }
      Rectangle ISelectable.GetBounds()
      {
         return Bounds;

      }

      bool mbSelected = false;
      public bool IsSelected()
      {
         return mbSelected;
      }

      #endregion

      #region IControlPointOwner Members
      List<IControlPoint> mControlPointList = new List<IControlPoint>();

      public List<IControlPoint> GetControlPoints()
      {
         return mControlPointList;
         //throw new Exception("The method or operation is not implemented.");
      }

      #endregion

      #region ICopyPaste Members

      public object MakeCopy()
      {
         Trigger copy = new Trigger();
         this.Trigger.DeepCopyTo(copy);

         //copy lines?



         return copy;
      }

      public void PasteContents(TriggerClipboard clipboard, object input, Dictionary<int, int> triggerMapping, bool bShallow)
      {
         int newID;
         Trigger copy = new Trigger();

         this.ParentTriggerNamespace = mTriggerHost.CurrentTriggerNamespace; 
         Trigger data = input as Trigger;
         int oldID = data.ID;

         if (bShallow == true) //not working yet?
         {
            data.DeepCopyTo(copy);
            mParentTriggerNamespace.InsertTrigger(copy, out newID);
            this.Trigger = copy;
         }
         else
         {

            //data.DeepCopyTo(copy);
            //mParentTriggerNamespace.InsertTrigger(copy, out newID);

            //clipboard.
            
            copy = clipboard.GetTrigger(data, this.ParentTriggerNamespace, out newID);

            this.Trigger = copy;
         }

         triggerMapping[oldID] = newID;
      }

      public void PostCopy(Dictionary<int, int> triggerMapping)
      {
         //fixup trigger ids
         List<TriggerEffect> effectList = mThisTrigger.TriggerEffects.Effects;
         List<TriggerEffect> hitlist = new List<TriggerEffect>();

         foreach (TriggerEffect e in effectList)
         {
            if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName())
            {
               int targetTrigger = System.Convert.ToInt32(ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value);
               if (triggerMapping.ContainsKey(targetTrigger))
               {
                  ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value = triggerMapping[targetTrigger].ToString();
               }
               else
               {
                  hitlist.Add(e);
               }
               continue;
            }
            if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
            {
               int targetTrigger = System.Convert.ToInt32(ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value);
               if (triggerMapping.ContainsKey(targetTrigger))
               {
                  ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value = triggerMapping[targetTrigger].ToString();
               }
               else
               {
                  hitlist.Add(e);
               }
               continue;
            }
         }
         foreach (TriggerEffect e in hitlist)
         {
            effectList.Remove(e);
         }
         hitlist.Clear();

         List<TriggerEffect> effectsOnFalse = mThisTrigger.TriggerEffectsFalse.Effects;
         foreach (TriggerEffect e in effectsOnFalse)
         {
            if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName())
            {
               int targetTrigger = System.Convert.ToInt32(ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value);
               if (triggerMapping.ContainsKey(targetTrigger))
               {
                  ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value = triggerMapping[targetTrigger].ToString();
               }
               else
               {
                  hitlist.Add(e);
               }
               continue;
            }
            if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
            {
               int targetTrigger = System.Convert.ToInt32(ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value);
               if (triggerMapping.ContainsKey(targetTrigger))
               {
                  ParentTriggerNamespace.GetValues()[e.Parameter[0].ID].Value = triggerMapping[targetTrigger].ToString();
               }
               else
               {
                  hitlist.Add(e);
               }
               continue;
            }
         }
         foreach (TriggerEffect e in hitlist)
         {
            effectList.Remove(e);
         }
         hitlist.Clear();


         this.LoadExistingConnections();

      }


      #endregion



      #region ITriggerUIUpdate Members
      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity)
      {
         List<Control> notused = new List<Control>();
         UIUpdate(data, arguments, visiblity, ref notused);
      }
      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity, ref List<Control> owners)
      {
         effectsList1.UIUpdate(data, arguments, visiblity, ref owners);
         effectsList2.UIUpdate(data, arguments, visiblity, ref owners);
         conditionsList1.UIUpdate(data, arguments, visiblity, ref owners);
      
         BasicArgument ba = arguments as BasicArgument;
         if (data == null)
         {
         }
         else if (data is Trigger && (((Trigger)data).ID == mThisTrigger.ID) && (ba != null))
         {
            if (ba.mArgument == BasicArgument.eArgumentType.Search)
            {
               owners.Add(this);
            }
         }

      }

      #endregion

      #region ICommentOutable Members

      public bool CommentOut
      {
         get
         {
            return this.Trigger.CommentOut;
         }
         set
         {
            this.Trigger.CommentOut = value;
            this.UpdateVisuals();
         }
      }

      #endregion
   }
}

