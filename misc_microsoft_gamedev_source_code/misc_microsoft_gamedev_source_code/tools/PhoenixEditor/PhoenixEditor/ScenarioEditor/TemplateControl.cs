using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

using SimEditor;
using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class TemplateControl : PhoenixEditor.ScenarioEditor.ClientNodeControl, IControlPointOwner, ISelectable, IDeletable, ICopyPaste, ITriggerUIUpdate, ICommentOutable
   {
      public TemplateControl()
      {
         InitUI();

         SetStyle(ControlStyles.OptimizedDoubleBuffer, true);

      }

      public TemplateControl(NodeHostControl nodeHost, TriggerHostArea Host)
      {
         BindToHost(nodeHost, Host);

         InitUI();

      }
      private void InitUI()
      {
         InitializeComponent();

         AddDragSurface(this.TriggerNameText);
         //AddDragSurface(this.flowLayoutPanel1);
         AddDragSurface(this.panel1);

         this.panel1.MouseUp += new MouseEventHandler(TemplateControl_MouseUp);

         panel1.Paint += new PaintEventHandler(panel1_Paint);
         //this.DoubleBuffered = true;

         //panel1.BackgroundImage = SharedResources.GetImage(@"Icons\TriggerEditor\loop.bmp");

         mLastBackColor = Color.White;// this.BackColor;

         //UpdateVisuals();
      }

      #region ITriggerComponent Members

      public void BindToHost(NodeHostControl nodeHost, TriggerHostArea Host)
      {
         mHost = nodeHost;
         mTriggerHost = Host;
      }

      #endregion

      TriggerHostArea mTriggerHost;

      void TemplateControl_MouseUp(object sender, MouseEventArgs e)
      {
         if(e.Button == MouseButtons.Right)
         {
            ContextMenu contextMenu = new ContextMenu();
            SetContextMenuItems(contextMenu);
            contextMenu.Show(this, new Point(e.X, e.Y));

         }
      }

     
         
      virtual protected void SetContextMenuItems(ContextMenu menu)
      {         
         MenuItem deleteTemplate = new MenuItem("Delete");
         deleteTemplate.Click += new EventHandler(deleteTemplate_Click);
         menu.MenuItems.Add(deleteTemplate);

         MenuItem commentOutItem = new MenuItem("Comment Out");
         commentOutItem.Checked = TriggerTemplateMapping.CommentOut;
         commentOutItem.Click += new EventHandler(commentOutItem_Click);
         menu.MenuItems.Add(commentOutItem);
      }

      void commentOutItem_Click(object sender, EventArgs e)
      {
         TriggerTemplateMapping.CommentOut = !TriggerTemplateMapping.CommentOut;
         ((MenuItem)(sender)).Checked = TriggerTemplateMapping.CommentOut;
         UpdateVisuals();
      }
      Color mLastBackColor = Color.Empty;
      public void UpdateVisuals()
      {
         if (TriggerTemplateMapping.CommentOut == true)
         {
            this.BackColor = Color.DarkGray;
         }
         else
         {
            this.BackColor = mLastBackColor;
         }
      }

      void deleteTemplate_Click(object sender, EventArgs e)
      {

         Delete();

      }

      public void Delete()
      {
         ParentTriggerNamespace.DeleteTemplateMapping(this.TriggerTemplateMapping);
         this.Parent.Controls.Remove(this);

         List<IControlPoint> cps = this.GetControlPoints();
         foreach (IControlPoint cp in cps)
         {
            cp.MarkForDelete = true;
         }
         this.mTriggerHost.Update();

      }

      public override string ToString()
      {
         if (this.TriggerTemplateMapping != null)
         {
            return this.TriggerTemplateMapping.ID.ToString();
         }

         return "-1";
         //return base.ToString();
      }

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

      TriggerTemplateMapping mTriggerTemplateMapping = new TriggerTemplateMapping();

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public TriggerTemplateMapping TriggerTemplateMapping
      {
         get
         {
            return mTriggerTemplateMapping;
         }
         set
         {
       
            mTriggerTemplateMapping = value;

            if (mTriggerTemplateMapping != null)
            {

               LoadMappingUI(mTriggerTemplateMapping);

               if (mTriggerTemplateMapping.SizeX != -1)
                  this.Width = mTriggerTemplateMapping.SizeX;
               if (mTriggerTemplateMapping.SizeY != -1)
                  this.Height = mTriggerTemplateMapping.SizeY;

            }

            UpdateVisuals();

         }

      }

      public override int GetGroupID() { return mTriggerTemplateMapping.GroupID; }
      public override void SetGroupID(int id) { mTriggerTemplateMapping.GroupID = id; }

      protected override void OnPaint(PaintEventArgs e)
      {

         //panel1.Visible = false;

         //base.OnPaint(e);
      }
      static Brush sTextbrush = new SolidBrush(Color.Black);
      private void panel1_Paint(object sender, PaintEventArgs e)
      {
         if (this.TriggerTemplateMapping.Image != "") 
         {
            Image i = SharedResources.GetImage(TriggerTemplateMapping.Image);//@"Icons\TriggerEditor\loop.bmp");

            //e.Graphics.DrawImage(i, 0, 0, panel1.Width, panel1.Height);
            e.Graphics.DrawImage(i, 32, 32, 32, 32);
         }
         if (mTriggerTemplateMapping != null)
         {
            e.Graphics.DrawString(mTriggerTemplateMapping.ID.ToString(), this.Font, sTextbrush, 7, 17);
         }
      }

      //<InputMapping Name="Count" Type="Count" BindID="25"></InputMapping >
      //<OutputMapping Name="I" Type="Count" BindID="21"></OutputMapping >
      //<TriggerInput Name="Start" BindID = "5" ></TriggerInput>
      //<TriggerOutput Name="Do" BindID = "30"></TriggerOutput>
      //private void LoadMappingUI(TriggerTemplateMapping mapping)
      //{
      //   LoadMappingUI(mapping, false);

      //}

      public bool mbDemoMode = false;


      //Loads ui.  Creates values for the mappings if they do not exist
      private void LoadMappingUI(TriggerTemplateMapping mapping)
      {
         TriggerNameText.Text = Path.GetFileNameWithoutExtension(mapping.Name);

         foreach(TriggersTemplateInputVariable v in mapping.InputMappings)
         {               
            TriggerParameterControl c = new TriggerParameterControl();
            TriggerValue val = new TriggerValue();

            if (mbDemoMode || (ParentTriggerNamespace.GetValues().ContainsKey(v.ID) == false) || (v.ID == -1))
            {
               val.Name = v.Name;
               val.Type = v.Type;
               int newID = -1;
               ParentTriggerNamespace.InsertValue(val, out newID);
               v.ID = newID;

               //this line set optional parmas to default off for newly placed templates
               if (v.Optional == true)
               {
                  val.IsNull = true;
               }                           
            }
            else
            {
               val = ParentTriggerNamespace.GetValues()[v.ID];
            }
         
            
            c.Init(null, v, val, ParentTriggerNamespace);

            VarInHardPointsBar.AddControl(c);
         }
         foreach(TriggersTemplateOutputVariable v in mapping.OutputMappings)
         {
            TriggerParameterControl c = new TriggerParameterControl();
            TriggerValue val = new TriggerValue();
            if (ParentTriggerNamespace.GetValues().ContainsKey(v.ID) == false)
            {
               val.Name = v.Name;
               val.Type = v.Type;
               int newID = -1;
               ParentTriggerNamespace.InsertValue(val, out newID);
               v.ID = newID;
            }
            else
            {
               val = ParentTriggerNamespace.GetValues()[v.ID];
            }
            c.Init(null, v, val, ParentTriggerNamespace);

            VarOutHardPointsBar.AddControl(c);
         }
         foreach (TriggersTemplateInputActionBinder b in mapping.TriggerInputs)
         {
            BasicControlPoint cp = new BasicControlPoint(mHost);
  
            INHardPointsBar.AddControl(cp);
            cp.TargetKey = "none";//"Trigger.Link.Back";
            cp.TypeKey = "Trigger.Link.Forward";
            cp.BackColor = b.GetColor();
            cp.SetToolTipText(b.Name);
            cp.Name = b.Name;
            cp.SetDirection(new Point(-250, 0));
            cp.Tag = this;
            cp.TagObject = this;
            cp.RenderText = cp.Name;
            cp.ControlPointDescription = this.mTriggerTemplateMapping.Name + "." + b.Name;

            mControlPoints.Add(cp);

            mInputActionCPs[cp] = b;
         }
         //flowLayoutPanel1.Controls.Add(MakeLabel("OutputTriggers:"));
         foreach (TriggersTemplateOutputActionBinder b in mapping.TriggerOutputs)
         {
            BasicControlPoint cp = new BasicControlPoint(mHost);
            OutHardPointsBar.AddControl(cp);
            cp.TargetKey = "Trigger.Link.Forward";
            cp.TypeKey = "none";//"Trigger.Link.Back";
            cp.BackColor = b.GetColor();
            cp.SetToolTipText(b.Name);
            cp.Name = b.Name;
            cp.SetDirection(new Point(250, 0));
            cp.Tag = this;
            cp.TagObject = this;
            cp.RenderText = b.Name;
            cp.ControlPointConnected += new BasicControlPoint.ControlPointEvent(output_ControlPointConnected);
            cp.ControlPointRemoved += new BasicControlPoint.ControlPointEvent(output_ControlPointRemoved);
            cp.ControlPointDescription = this.mTriggerTemplateMapping.Name + "." + b.Name;

            mControlPoints.Add(cp);

            mOutputActionCPs[cp] = b;
         } 
      }


      public Dictionary<BasicControlPoint, TriggersTemplateInputActionBinder> mInputActionCPs = new Dictionary<BasicControlPoint, TriggersTemplateInputActionBinder>();
      public Dictionary<BasicControlPoint, TriggersTemplateOutputActionBinder> mOutputActionCPs = new Dictionary<BasicControlPoint, TriggersTemplateOutputActionBinder>();

      //Remove the binding target
      void output_ControlPointRemoved(BasicControlPoint cp, IControlPoint other)
      {


         if (other.Virtual == true)
            return;
         TriggersTemplateOutputActionBinder outputBinder = mOutputActionCPs[cp];
         TriggerBindInfo toremove = null;
         foreach (TriggerBindInfo b in outputBinder.TargetIDs)
         {
            if (b != null && other != null)
            {
               if (b.LinkName == other.ToString())
               {
                  toremove = b;
               }
            }
            else
            {
               CoreGlobals.getErrorManager().LogErrorToNetwork("(b.LinkName == other.ToString())");
            }
         }
         if (toremove != null)
         {
            outputBinder.TargetIDs.Remove(toremove);
         }
      }

      /// <summary>
      /// Add a new binding target
      /// </summary>
      /// <param name="cp">This(starting) control point</param>
      /// <param name="other">Target control point</param>
      void output_ControlPointConnected(BasicControlPoint cp, IControlPoint other)
      {
         if (other.Virtual == true)
            return;
         //set target value:
         TriggerControl t = other.TagObject as TriggerControl;

         TriggersTemplateOutputActionBinder outputbinder = mOutputActionCPs[cp];

         if (t != null)
         {
            string name = cp.ToString();
            if(mOutputActionCPs.ContainsKey(cp))
            {
               
               
               TriggerBindInfo bindInfo = new TriggerBindInfo();
               bindInfo.SetTarget(t.Trigger.ID, other.ToString());
               outputbinder.TargetIDs.Add(bindInfo);
            
               return;
            }
         }
        
         TemplateControl templateControl = other.TagObject as TemplateControl;
         if (templateControl != null)
         {
            string name = other.ToString();
            foreach (TriggersTemplateInputActionBinder inputBinder in templateControl.TriggerTemplateMapping.TriggerInputs)
            {
               if (inputBinder.Name == name)
               {
                  foreach (TriggerBindInfo b in inputBinder.TargetIDs)
                  {
                     if (b.ID == templateControl.TriggerTemplateMapping.ID && b.LinkName == other.ToString())
                     {
                        CoreGlobals.getErrorManager().OnSimpleWarning("can't bind. template is already bound to trigger.  this is a bug");
                        return;
                     }
                  }
                  
                  TriggerBindInfo bindInfo = new TriggerBindInfo();
                  bindInfo.SetTarget(templateControl.TriggerTemplateMapping.ID, other.ToString());
                  outputbinder.TargetIDs.Add(bindInfo);

                  return;
               }
            }
         }
      }


      /// <summary>
      /// This connects the ui elements based off of the data that was loaded.
      /// Templates hold the connection data when a connection to a template is made in the ActionBinder classes
      /// Templates always load the connections to the right(output), and load connections to the left if trigger control is left of them
      /// </summary>
      public void LoadExistingConnections()
      {
         Dictionary<BasicControlPoint, TriggersTemplateInputActionBinder>.Enumerator it = mInputActionCPs.GetEnumerator();
         while (it.MoveNext())
         {
            TriggersTemplateInputActionBinder binder = it.Current.Value;
            BasicControlPoint thisCp = it.Current.Key;
            foreach (TriggerBindInfo targetinfo in binder.TargetIDs)
            {
               if (targetinfo.HasTarget())
               {
                  if (targetinfo.LinkName == "Effect.False" || targetinfo.LinkName == "Effect.True")
                  {
                     TriggerControl targetC = mTriggerHost.GetTriggerControl(targetinfo.ID);
                     if (targetC != null)
                     {
                        if (targetinfo.LinkName == "Effect.True")
                           targetC.GetEffectTruePoint().GetTargets().Add(thisCp);
                        else
                           targetC.GetEffectFalsePoint().GetTargets().Add(thisCp);
                     }
                  }
               }
            }
         }

         Dictionary<BasicControlPoint, TriggersTemplateOutputActionBinder>.Enumerator it2 = mOutputActionCPs.GetEnumerator();
         while (it2.MoveNext())
         {
            TriggersTemplateOutputActionBinder binder = it2.Current.Value;
            BasicControlPoint thisCp = it2.Current.Key;

            List<TriggerBindInfo> toRemove = new List<TriggerBindInfo>();

            foreach (TriggerBindInfo targetinfo in binder.TargetIDs)
            {

               if (targetinfo.HasTarget())
               {
                  //bound to trigger
                  if (targetinfo.LinkName == "Activate" || targetinfo.LinkName == "Deactivate")
                  {
                     TriggerControl triggerTarget = mTriggerHost.GetTriggerControl(targetinfo.ID);
                     if (triggerTarget != null)
                     {
                        if (targetinfo.LinkName == "Activate")
                        {
                           thisCp.GetTargets().Add(triggerTarget.GetActivationPoint());
                        }
                        else if (targetinfo.LinkName == "Deactivate")
                        {
                           thisCp.GetTargets().Add(triggerTarget.GetDeactivationPoint());
                        }
                     }
                  }
                  else //it is a template
                  {
                     TemplateControl c = mTriggerHost.GetTemplatetControl(targetinfo.ID);
                     if (c != null)
                     {
                        BasicControlPoint cpTarget = c.GetInputControlPoint(targetinfo.LinkName);
                        thisCp.GetTargets().Add(cpTarget);
                     }
                     else
                     {
                        string groupName = this.ParentTriggerNamespace.GetGroupName(TriggerTemplateMapping.GroupID);
                        string errormsg = "Error connecting template " + this.TriggerTemplateMapping.Name + "(" + this.TriggerTemplateMapping.ID + ")"
                           + "." + thisCp.Name
                           + "  to missing template input: " + targetinfo.LinkName + ", " + targetinfo.ID + "     in group: " + groupName
                           + "   Bad Link removed (most likely harmless residue from an old deleted template that this was connected to)";
                        toRemove.Add(targetinfo);
                        this.BackColor = Color.Red;
                        this.TriggerNameText.Text = Path.GetFileNameWithoutExtension(TriggerTemplateMapping.Name) + "(" + this.TriggerTemplateMapping.ID + ")";

                        CoreGlobals.getErrorManager().OnSimpleWarning(errormsg);
                        CoreGlobals.ShowMessage(errormsg);
                        CoreGlobals.getErrorManager().LogErrorToNetwork(errormsg);
                     }                                   
                  }                  
               }
            }

            foreach (TriggerBindInfo badBind in toRemove)
            {
               binder.TargetIDs.Remove(badBind);
            }
         }
      }

      bool mbIgnoreRenameOnce = false;
      public override void OnMoved()
      {
         mbIgnoreRenameOnce = true;

         this.TriggerTemplateMapping.X = this.Location.X - mHost.AutoScrollPosition.X;
         this.TriggerTemplateMapping.Y = this.Location.Y - mHost.AutoScrollPosition.Y;
      }

      public BasicControlPoint GetInputControlPoint(string name)
      {
         Dictionary<BasicControlPoint, TriggersTemplateInputActionBinder>.Enumerator it = mInputActionCPs.GetEnumerator();
         while (it.MoveNext())
         {
            BasicControlPoint thisCp = it.Current.Key;
            if (thisCp.Name == name)
               return thisCp;
         }
         return null;
      }


      private Label MakeLabel(string name)
      {
         Label l = new Label();
         l.Text = name;
         //l.ForeColor = Color.Red;
         l.AutoSize = true;
         l.Margin = new Padding(3,0,3,0);
         return l;
      }

      List<IControlPoint> mControlPoints = new List<IControlPoint>();
      public List<IControlPoint> GetControlPoints()
      {



         return mControlPoints;
      }



      #region ISelectable Members

      public Rectangle GetBounds()
      {
         return Bounds;
      }

      public void SelectControl()
      {
         this.BorderStyle = BorderStyle.FixedSingle;
         mbSelected = true;
      }

      public void DeSelectControl()
      {
         this.BorderStyle = BorderStyle.None;
         mbSelected = false;
      }
      bool mbSelected = false;
      public bool IsSelected()
      {
         return mbSelected;
      }
      #endregion

      private void OutHardPointsBar_Load(object sender, EventArgs e)
      {

      }



      #region ICopyPaste Members

      public object MakeCopy()
      {
         return new object();
      }

      public void PasteContents(TriggerClipboard clipboard, object input, Dictionary<int, int> triggerMapping, bool bShallow)
      {
         if(bShallow == true)
         {


         }
         else         
         {



         }

      }

      public void PostCopy(Dictionary<int, int> triggerMapping)
      {

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
         TriggerValue trigerValue = data as TriggerValue;
         if (trigerValue != null)
         {
            foreach (Control c in VarInHardPointsBar.GetLogicalControls())
            {
               ITriggerUIUpdate ui = c as ITriggerUIUpdate;
               if (ui != null)
                  ui.UIUpdate(data, arguments, visiblity, ref owners);
            }
            foreach (Control c in VarOutHardPointsBar.GetLogicalControls())
            {
               ITriggerUIUpdate ui = c as ITriggerUIUpdate;
               if (ui != null)
                  ui.UIUpdate(data, arguments, visiblity, ref owners);
            }

         }


         BasicArgument ba = arguments as BasicArgument;
         if (data == null)
         {
         }
         else if ((data == this.TriggerTemplateMapping) && (ba != null))
         {
            if (ba.mArgument == BasicArgument.eArgumentType.Search)
            {
               owners.Add(this);
            }
         }

         //throw new Exception("The method or operation is not implemented.");
      }

      #endregion

      #region ICommentOutable Members

      public bool CommentOut
      {
         get
         {
            return this.TriggerTemplateMapping.CommentOut;
         }
         set
         {
            this.TriggerTemplateMapping.CommentOut = value;
            this.UpdateVisuals();
         }
      }

      #endregion

   }
}

