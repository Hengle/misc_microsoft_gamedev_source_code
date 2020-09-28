using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class BasicControlPoint : UserControl, IControlPoint//, IActiveDrag
   {

      NodeHostControl mHost;
      public BasicControlPoint(NodeHostControl Host)
      {
         InitializeComponent();

         this.DragEnter += new DragEventHandler(BasicControlPoint_DragEnter);
         this.DragOver += new DragEventHandler(BasicControlPoint_DragOver);
         this.DragDrop += new DragEventHandler(BasicControlPoint_DragDrop);
         
         
         this.GiveFeedback += new GiveFeedbackEventHandler(BasicControlPoint_GiveFeedback);
         this.MouseMove += new MouseEventHandler(BasicControlPoint_MouseMove);
         this.MouseUp += new MouseEventHandler(BasicControlPoint_MouseUp);
         this.MouseEnter += new EventHandler(BasicControlPoint_MouseEnter);
         this.MouseLeave += new EventHandler(BasicControlPoint_MouseLeave);
         this.MouseClick += new MouseEventHandler(BasicControlPoint_MouseClick);

         this.AllowDrop = true;

         this.BackColor = Color.Blue;

         mHost = Host;

         //this.b

         this.Size = new Size(16, 16);

         
      }

 


      public override string ToString()
      {
         //return base.ToString();
         return this.Name;
      }


      public delegate void ControlPointEvent(BasicControlPoint cp, IControlPoint other);

      public event ControlPointEvent ControlPointConnected = null;
      public event ControlPointEvent ControlPointRemoved = null;

      public event ControlPointEvent SenderControlPointConnected = null;
      public event ControlPointEvent SenderControlPointRemoved = null;


      public string RenderText = "";

      Brush textbrush = null;//new SolidBrush(Color.Black);
      protected override void OnPaintBackground(PaintEventArgs e)
      {
         if (textbrush == null)
         {

            Color b = this.BackColor;
            //textbrush = new SolidBrush(Color.FromArgb(255-b.R,255-b.G,255-b.B));
            textbrush = new SolidBrush(Color.White);
         }

         base.OnPaintBackground(e);

         //int offset = 0;
         if (RenderText == "" || RenderText == null)
         {
         }
         else if (RenderText.Length == 1)
         {
            e.Graphics.DrawString(RenderText, this.Font, textbrush,0,0,null);
         }
         else if (RenderText.Length == 2)
         {
            e.Graphics.DrawString(RenderText, this.Font, textbrush, -1, 0, null);
         }
         else
         {
            e.Graphics.DrawString(RenderText, this.Font, textbrush, -2, 0, null);
         }

      }

      void BasicControlPoint_MouseLeave(object sender, EventArgs e)
      {

         HighLight = false;
         mHost.SetDirty();

         this.Cursor = Cursors.Arrow;
      }

      void BasicControlPoint_MouseEnter(object sender, EventArgs e)
      {
         HighLight = true;
         mHost.SetDirty();

         if (NewLinkInitiator != null)
         {   
            if(ValidateConnection(NewLinkInitiator))
               this.Cursor = Cursors.Cross;
         }

      }

      void BasicControlPoint_MouseClick(object sender, MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Left && NewLinkInitiator != null)
         {
            BindControlPoints(NewLinkInitiator);
            mHost.Owner.SetDirty();  //Recalculate Groups
            NewLinkInitiator = null;
         }
      }

      public void SetToolTipText(string toolTip)
      {
         toolTip1.SetToolTip(this, toolTip);
         this.toolTip1.InitialDelay = 1000;
         this.toolTip1.AutomaticDelay = 0;
         toolTip1.UseAnimation = false;
         toolTip1.UseFading = false;
      }

 
      SmartLineSegment mActiveDrag = null;
      int mlastDDEvent = -1;
      void BasicControlPoint_GiveFeedback(object sender, GiveFeedbackEventArgs e)
      {
         if (mActiveDrag == null)
            mActiveDrag = new SmartLineSegment(mHost);

         if (mlastDDEvent != mDDCount)
         {
            mlastDDEvent = mDDCount;
            mActiveDrag.EraseLast();
         }

         mActiveDrag.A = Parent.PointToScreen(this.Location);
         mActiveDrag.B =   new Point(MousePosition.X, MousePosition.Y);
         mHost.QueueDirty();

      }

      void BasicControlPoint_DragOver(object sender, DragEventArgs e)
      {
         //throw new Exception("The method or operation is not implemented.");
         //e.Effect = DragDropEffects.Link;
      }


      protected override void OnPaint(PaintEventArgs e)
      {
         //e.Graphics.
         //base.OnPaint(e);
      }
      

      void BasicControlPoint_MouseUp(object sender, MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Right)
         {
            ContextMenu contextMenu = new ContextMenu();
            SetContextMenuItems(contextMenu);
            contextMenu.Show(this, new Point(e.X, e.Y));
         } 
      }

      public bool mbDontHide = false;

      virtual protected void SetContextMenuItems(ContextMenu menu)
      {
         MenuItem newConnection = new MenuItem("+New Link");
         //newConnection.Tag = cp;
         newConnection.Click += new EventHandler(newConnection_Click);
         menu.MenuItems.Add(newConnection);

         foreach(IControlPoint cp in GetTargets())
         {
            //if (!mbDontHide && cp.HasProxy == true)
            //   continue;
            if (GetLevel() != cp.GetLevel())
            {
               continue;
            }

            MenuItem deleteConnection = new MenuItem("Delete link to: " + cp.ControlPointDescription);
            deleteConnection.Tag = cp;
            deleteConnection.Click += new EventHandler(deleteConnection_Click);
            menu.MenuItems.Add(deleteConnection);
         }
      }


      public int GetLevel()
      {
         ClientNodeControl node = this.TagObject as ClientNodeControl;
         if (node == null)
            return -1;

         return node.GetGroupID();
      }


      static BasicControlPoint NewLinkInitiator = null;

      void newConnection_Click(object sender, EventArgs e)
      {
         NewLinkInitiator = this;
         //throw new Exception("The method or operation is not implemented.");
         Cursor.Current = Cursors.Cross;
      }

      void deleteConnection_Click(object sender, EventArgs e)
      {
         MenuItem m = sender as MenuItem;
         IControlPoint cp = m.Tag as IControlPoint;
         this.DisconnectControlPoint(cp);
         mHost.SetDirty();
      }

      int mDDCount = 0;
      void BasicControlPoint_MouseMove(object sender, MouseEventArgs e)
      {         
         if (e.Button == MouseButtons.Left)
         {
            DoDragDrop((IControlPoint)this, DragDropEffects.Link);//DragDropEffects.All | 
            mDDCount++;
            if (isConnected() == false)
            {
            
            }

            mBaseColor = this.BackColor;
         }
         
      }

      Color mBaseColor;
      private void Flicker()
      {
         if (Color.White == this.BackColor)
            //if (mBaseColor == this.BackColor)
         {
            this.BackColor = Color.Gold;

         }
         else
         {
            this.BackColor = Color.White;
         }
      }

      void BasicControlPoint_DragDrop(object sender, DragEventArgs e)
      {

         string[] formats = e.Data.GetFormats();
         if (formats.Length == 0)
            return;
         IControlPoint otherControlPoint = e.Data.GetData(e.Data.GetFormats()[0]) as IControlPoint;
         if (e.Effect == DragDropEffects.Link)
         {
            BindControlPoints(otherControlPoint);
         }
      }

      void BindControlPoints(IControlPoint otherControlPoint)
      {
         if (otherControlPoint != null)
         {
            if (otherControlPoint.CanConnect(this))
            {
               otherControlPoint.ConnectControlPoint(this);
            }
            else if (this.CanConnect(otherControlPoint))
            {
               ConnectControlPoint(otherControlPoint);
            }
            mHost.SetDirty();
         }

      }


      void BasicControlPoint_DragEnter(object sender, DragEventArgs e)
      {
         string[] formats = e.Data.GetFormats();
         if (formats.Length == 0)
            return;
         IControlPoint otherControlPoint = e.Data.GetData(e.Data.GetFormats()[0]) as IControlPoint;

         e.Effect = DragDropEffects.None;

         if (otherControlPoint == this)
            return;

         if(ValidateConnection(otherControlPoint))
         {
            e.Effect = DragDropEffects.Link;                   
         }      
      }
      bool ValidateConnection(IControlPoint otherControlPoint)
      {
         if (otherControlPoint != null)
         {
            if (otherControlPoint.CanConnect(this) || this.CanConnect(otherControlPoint))
            {
               return true;
            }
         }
         return false;
      }

      Point mDirection = new Point(0, 50);
      public void SetDirection(Point v)
      {
         mDirection = v;
      }

      List<IControlPoint> mTargets = new List<IControlPoint>();

      #region IControlPoint Members
      bool mMarkForDelete = false;
      public bool MarkForDelete
      {
         get
         {
            return mMarkForDelete;
         }
         set
         {
            mMarkForDelete = value;
         }
      }

      public string GetName()
      {
         return this.Name;
      }

      public bool isConnected()
      {
         if (mTargets.Count > 0)
            return true;
         return false;
      }

      public ICollection<IControlPoint> GetTargets()
      {
         return mTargets;
      }

      public Point GetPosition()
      {         
         //Parent.Location
         Point loc = Location;
         loc.Y += 5;
         if (Parent == null)
         {
            return loc;
         }

         return Parent.PointToScreen(loc);//this.Location;
      }

      public Point GetVector()
      {
         Point p = GetPosition();
         //return new Point(mDirection.X + Parent.Location.X, mDirection.Y + Parent.Location.Y);
         p.Offset(mDirection);
         return p;
      }


      public void ConnectControlPoint(IControlPoint cpTarget)
      {
         if (CanConnect(cpTarget))
         {
            GetTargets().Add(cpTarget);

            if (ControlPointConnected != null)
            {
               ControlPointConnected.Invoke(this, cpTarget);
            }
            BasicControlPoint target = cpTarget as BasicControlPoint;
            if (target.SenderControlPointConnected != null && target != null)
            {
               target.SenderControlPointConnected.Invoke(target, this);
            }

         }
      }
      public void DisconnectControlPoint(IControlPoint cpTarget)
      {
         if(GetTargets().Contains(cpTarget))
         {
            //Recursive removal of chained control points
            //if (cpTarget.ChainChild != null)
            //{
            //   foreach(IControlPoint subtarget in cpTarget.ChainChild.GetTargets())
            //   {
            //      //this.DisconnectControlPoint(subtarget);
            //      //cpTarget.ChainChild.DisconnectControlPoint(subtarget);
            //   }
            //}

            //IControlPoint chainStart = cpTarget.ChainChild


            //Remove the actual target
            GetTargets().Remove(cpTarget);
            if(ControlPointRemoved != null)
            {
               ControlPointRemoved.Invoke(this, cpTarget);
            }

            BasicControlPoint target = cpTarget as BasicControlPoint;
            if (target.SenderControlPointRemoved != null && target != null)
            {
               target.SenderControlPointRemoved.Invoke(target, this);
            }

         }

      }
      public bool CanConnect(IControlPoint cpother)
      {
         if(GetTargets().Contains(cpother) == true)
            return false;
         if (cpother.TypeKey == "none" || this.TargetKey == "none")
            return false;
         if (cpother.TypeKey == this.TargetKey)
         {
            return true;
         }
         return false;

      }

      object mTagObject = null;
      public object TagObject
      {
         get
         {
            return mTagObject;
         }
         set
         {
            mTagObject = value;
         }
      }

      IControlPoint mProxyTarget = null;
      public IControlPoint ProxyTarget
      {
         get
         {
            return mProxyTarget;
         }
         set
         {
            mProxyTarget = value;
         }
      }

      List<KeyValuePair<IControlPoint, IControlPoint>> mParentConnections = new List<KeyValuePair<IControlPoint, IControlPoint>>();

      public List<KeyValuePair<IControlPoint, IControlPoint>> ParentConnections
      {
         get
         {
            return mParentConnections;
         }
         set
         {
            mParentConnections = value;
         }
      }


      bool mbHighlight = false;
      public bool HighLight
      {
         get
         {
            return mbHighlight;
         }
         set
         {
            mbHighlight = value;
         }

      }
      bool mbHasProxy = false;
      public bool HasProxy
      {
         get
         {
            return mbHasProxy;
         }
         set
         {
            mbHasProxy = value;
         }
      }

      bool mbVirtual = false;
      public bool Virtual
      {
         get
         {
            return mbVirtual;
         }
         set
         {
            mbVirtual = value;
         }
      }

      string mControlPointDescription = "asdf";
      public string ControlPointDescription
      {
         get
         {
            return mControlPointDescription;
         }
         set
         {
            mControlPointDescription = value;
         }
      }
      string mTypeKey = "none";
      string mTargetKey = "none";

      public string TypeKey
      {
         set
         {
            mTypeKey = value;

         }
         get
         {
            return mTypeKey;
         }
      }
      public string TargetKey
      {
         set
         {
            mTargetKey = value;
         }
         get
         {
            return mTargetKey;
         }
      }

      IControlPoint mChainParent;
      public IControlPoint ChainParent
      {
         get
         {
            return mChainParent;
         }
         set
         {
            mChainParent = value;
         }
      }
      IControlPoint mChainChild;
      public IControlPoint ChainChild
      {
         get
         {
            return mChainParent;
         }
         set
         {
            mChainParent = value;
         }
      }

      #endregion


   }
}
