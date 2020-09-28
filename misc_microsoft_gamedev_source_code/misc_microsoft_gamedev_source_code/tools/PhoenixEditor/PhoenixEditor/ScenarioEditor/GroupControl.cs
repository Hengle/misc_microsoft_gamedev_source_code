using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using SimEditor;
using EditorCore;
using PhoenixEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class GroupControl : ClientNodeControl, IControlPointOwner, ISelectable, IDeletable, ITriggerUIUpdate
   {
      public GroupControl(TriggerHostArea host)
      {
         InitializeComponent();

         mHost = host;

         this.AddDragSurface(this.panel1);
         this.AddDragSurface(this.panel2);
         this.AddDragSurface(this.GroupTitleTextBox);

         this.AddResizeSurface(this.ResizePanel);
         this.Move += new EventHandler(stateChanged);
         this.Resize += new EventHandler(stateChanged);
      }
      TriggerHostArea mHost = null;

      bool mbPaused = false;
      void stateChanged(object sender, EventArgs e)
      {

         if (mbPaused) return;
         GroupUI = GroupUI;//does update
      }

      NodeHostControl mOuterGroup;
      NodeHostControl mInnerGroup;
      TriggerNamespace mTriggerNamespace;

      List<IControlPoint> mControlPoints = new List<IControlPoint>();

      Dictionary<IControlPoint, IControlPoint> mInputControlPointMap = new Dictionary<IControlPoint, IControlPoint>();
      Dictionary<IControlPoint, IControlPoint> mOutputControlPointMap = new Dictionary<IControlPoint, IControlPoint>();


      //List<TriggerVarri

      public int mInnerGroupID;


      //public override void OnMoved()
      //{
      //   this.TriggerTemplateMapping.X = this.Location.X - mHost.AutoScrollPosition.X;
      //   this.TriggerTemplateMapping.Y = this.Location.Y - mHost.AutoScrollPosition.Y;
      //}

      public override void SetGroupID(int id)
      {
         //if (mHost != null)
         //{
         //   mOuterGroup = mHost.GetHostByGroupID(id);
         //}

         mGroupUI.GroupID = id;
         base.SetGroupID(id);
      }

      GroupUI mGroupUI = new GroupUI();
      public GroupUI GroupUI
      {
         get
         {
            mGroupUI.X = this.Location.X - mOuterGroup.AutoScrollPosition.X;
            mGroupUI.Y = this.Location.Y - mOuterGroup.AutoScrollPosition.Y;

            mGroupUI.Height = this.Height;
            mGroupUI.Width = this.Width;
            mGroupUI.GroupID = this.GetGroupID();
            mGroupUI.InternalGroupID = mInnerGroupID;
            return mGroupUI;
         }
         set
         {
            mGroupUI = value;
            //this.Left = mGroupUI.X;
            //this.Top = mGroupUI.Y;
            //this.SetGroupID( mGroupUI.GroupID );
            //mInnerGroupID = mGroupUI.InternalGroupID;
         }
      }

      private void UpdateUIFromData()
      {
         mbPaused = true;



         if (mbLockMovement == false && mGroupUI.Width != -1)
         {
            this.Left = mGroupUI.X;
            this.Top = mGroupUI.Y;
            this.Height = mGroupUI.Height;
            this.Width = mGroupUI.Width;
         }

         if (mGroupUI.Title == "")
         {
            mGroupUI.Title = "NewGroup" + this.mInnerGroupID;
         }

         this.GroupTitleTextBox.Text = mGroupUI.Title;

         //mbLoading = true;
         mbPaused = false;
      }

      public void RecalculateGroup(bool clear)
      {
         if (clear)
         {
            ClearData();
         }

         mbLoading = true;

         //mOuterGroup = mHost.GetHostByGroupID(this.mInnerGroupID);
         int parent = mHost.GetGroupParent(this.mInnerGroupID);
         mOuterGroup = mHost.GetHostByGroupID(parent);

         mbLockMovement = true;
         Setup(mOuterGroup, mInnerGroup, mTriggerNamespace);
         mbLockMovement = false;

         mbLoading = false;
      }
      bool mbLockMovement = false;

      public void ClearData()
      {
         mbLoading = true;

         mControlPoints.Clear();
         GroupOutpuHardPointsBar.RemoveAll();
         GoupInputHardPointsBar.RemoveAll();

         foreach (KeyValuePair<IControlPoint, IControlPoint> pair in mInLinks)
         {
            pair.Key.DisconnectControlPoint(pair.Value);
         }
         foreach (KeyValuePair<IControlPoint, IControlPoint> pair in mOutLinks)
         {
            pair.Key.DisconnectControlPoint(pair.Value);
         }

         mInLinks.Clear();
         mOutLinks.Clear();


         mInputControlPointMap.Clear();
         mOutputControlPointMap.Clear();

         mbLoading = false;

      }

      List<KeyValuePair<IControlPoint, IControlPoint>> mInLinks = new List<KeyValuePair<IControlPoint, IControlPoint>>();
      List<KeyValuePair<IControlPoint, IControlPoint>> mOutLinks = new List<KeyValuePair<IControlPoint, IControlPoint>>();



      static bool mbLoading = false;


      public bool TryGetControlPointAtLevel(int grouplevel, List<IControlPoint> virtualCPs, IControlPoint initialPoint, KeyValuePair<IControlPoint, IControlPoint> searchTarget, out IControlPoint foundPoint)
      {
         if (initialPoint.GetLevel() == this.mGroupID)
         {
            foundPoint = initialPoint;
            return true;
         }
         else
         {
            foreach (IControlPoint cp in virtualCPs)
            {
               if (cp.ParentConnections.Contains(searchTarget))
               {
                  foundPoint = cp;
                  return true;
               }
            }
         }
         

         foundPoint = null;
         return false;
      }

      int mOuterGroupID;

      public void Setup(NodeHostControl outerGroup, NodeHostControl innerGroup, TriggerNamespace triggerNamespace)
      {

         mbLoading = true;

         base.mHost = outerGroup;

         mOuterGroup = outerGroup;
         mInnerGroup = innerGroup;
         mInnerGroupID = innerGroup.mGroupID;
         mTriggerNamespace = triggerNamespace;
         mOuterGroupID = outerGroup.mGroupID;


         UpdateUIFromData();

         //Get proxy at level?
         // connect up later?


         List<KeyValuePair<IControlPoint, IControlPoint>> alllinks = mHost.GetLinks();
         List<IControlPoint> virtualCPs = mHost.GetVirtualControlPoints(outerGroup.mGroupID);

         mInLinks.Clear();
         //Pointing "in" 

         foreach (KeyValuePair<IControlPoint, IControlPoint> pair in alllinks)
         {
            //ClientNodeControl outer = pair.Key.TagObject as ClientNodeControl;

            int ki = pair.Key.GetLevel();
            int vi = pair.Value.GetLevel();

            if (ki == vi)
               continue;

            //bool b1 = (mHost.IsChild(pair.Value.GetLevel(), pair.Key.GetLevel()));
            //bool b1a = !(mHost.IsChild(pair.Key.GetLevel(), mInnerGroupID));
            //bool b2 = mHost.IsChild(pair.Value.GetLevel(), mInnerGroupID);
            //bool b3 = pair.Value.GetLevel() == mInnerGroupID;
            //if (!(mHost.IsChild(pair.Key.GetLevel(), mInnerGroupID))
            //&& (pair.Value.GetLevel() == mInnerGroupID || mHost.IsChild(pair.Value.GetLevel(), mInnerGroupID)))


            bool b1 = ki != mInnerGroupID;
            bool b2 = !mHost.IsChild(ki, mInnerGroupID);
            bool b3 = mHost.IsChild(vi, mInnerGroupID) || (vi == mInnerGroupID);

            if (b1 && b2 && b3)
            {
               IControlPoint proxy = GetInputControlPoint(pair.Key, pair.Value);

               IControlPoint sender = pair.Key;

               //find at this level.. that matches.?

               //while (sender.ProxyTarget != null && sender.GetLevel() != this.mGroupID)
               //{
               //   sender = sender.ProxyTarget;

               //}

               proxy.ParentConnections.Add(pair);

               //if (sender.GetLevel() != this.mGroupID)
               //{
               //   foreach (IControlPoint cp in virtualCPs)
               //   {
               //      if(cp.ParentConnections.Contains(pair))
               //      {
               //         sender = cp;
               //      }
               //   }
               //}
               //mInLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(sender, proxy));
               IControlPoint foundPoint;
               if (TryGetControlPointAtLevel(this.mGroupID, virtualCPs, pair.Key, pair, out foundPoint))
               {
                  mInLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(foundPoint, proxy));
               }
            }
         }
         #region omfg

         //foreach (Control c in mOuterGroup.Controls)  //????need to check others
         //{
         //   //if (c is GroupControl)
         //   //   continue;
         //   if (c is InnerGroupControl)
         //      continue;

         //   IControlPointOwner cpOwner = c as IControlPointOwner;
         //   if (cpOwner != null)
         //   {
         //      foreach (IControlPoint cpoint in cpOwner.GetControlPoints())
         //      {
         //         foreach (IControlPoint target in cpoint.GetTargets())
         //         {
         //            ClientNodeControl targetControl = target.TagObject as ClientNodeControl;
         //            if (targetControl != null)
         //            {
                        
         //               if ((targetControl.GetGroupID() == mInnerGroup.mGroupID)
         //               || (mHost.IsChild(targetControl.GetGroupID(), mInnerGroup.mGroupID)))
         //               {
         //                  IControlPoint proxy = GetInputControlPoint(cpoint,target);


         //                  mInLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(cpoint, proxy));
         //               }
         //               //else if (targetControl.GetGroupID() != mOuterGroup.mGroupID)
         //               //{
         //               //   IControlPoint proxy = GetInputControlPoint(cpoint, target);
         //               //   worklist.Add(new KeyValuePair<IControlPoint, IControlPoint>(cpoint, proxy));
         //               //}

         //            }
         //         }
         //      }
         //   }
         //}
#endregion
         
         //bind the connections in the worklist.
         foreach (KeyValuePair<IControlPoint, IControlPoint> pair in mInLinks)
         {
            pair.Key.ConnectControlPoint(pair.Value);
         }

         mOutLinks.Clear();

         foreach (KeyValuePair<IControlPoint, IControlPoint> pair in alllinks)
         {
            int ki = pair.Key.GetLevel();
            int vi = pair.Value.GetLevel();
            
            if (ki == vi)
               continue;

            //bool b1 = !mHost.IsChild(pair.Value.GetLevel(), mInnerGroupID);
            //bool b2 = pair.Key.GetLevel() == mInnerGroupID || mHost.IsChild(pair.Key.GetLevel(), mInnerGroupID);
            //bool b3 = pair.Value.GetLevel() != mInnerGroupID;
            //if ((!mHost.IsChild(pair.Value.GetLevel(), mInnerGroupID))
            //&& (pair.Key.GetLevel() == mInnerGroupID || mHost.IsChild(pair.Key.GetLevel(), mInnerGroupID))
            //&&  (pair.Value.GetLevel() != mInnerGroupID))

            bool b1 = vi != mInnerGroupID;
            bool b2 = !mHost.IsChild(vi, mInnerGroupID);
            bool b3 = mHost.IsChild(ki, mInnerGroupID) || (ki == mInnerGroupID);

            if (b1 && b2 && b3)
            {
               IControlPoint proxy = GetOutputControlPoint(pair.Key);
               proxy.ParentConnections.Add(pair);

                             
               //mOutLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(proxy, pair.Value));
               IControlPoint foundPoint;
               if (TryGetControlPointAtLevel(this.mGroupID, virtualCPs, pair.Value, pair, out foundPoint))
               {
                  mOutLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(proxy, foundPoint));
               }

            }


         }

         #region omfg2
         //pointing "out"
         //foreach (Control c in mInnerGroup.Controls)
         //{
         //   if (c is InnerGroupControl)
         //      continue;
         //   //if (c is GroupControl)
         //   //   continue;

         //   IControlPointOwner cpOwner = c as IControlPointOwner;
         //   if (cpOwner != null)
         //   {
         //      foreach (IControlPoint cpoint in cpOwner.GetControlPoints())
         //      {
         //         foreach (IControlPoint target in cpoint.GetTargets())
         //         {
         //            ClientNodeControl targetControl = target.TagObject as ClientNodeControl;
         //            if (targetControl != null)
         //            {
         //               //if (targetControl.GetGroupID() == mOuterGroup.mGroupID)
         //               //{
         //               //   IControlPoint proxy = GetOutputControlPoint(cpoint);
         //               //   //proxy.ConnectControlPoint(target);
         //               //   worklist.Add(new KeyValuePair<IControlPoint, IControlPoint>(proxy, target));
         //               //}

         //               if (targetControl is InnerGroupControl) //the innercontrol will handle this
         //               {
         //                  continue;
         //               }
         //               if (targetControl.GetGroupID() != mInnerGroup.mGroupID)
         //               {
         //                  IControlPoint proxy = GetOutputControlPoint(cpoint);
         //                  //proxy.ConnectControlPoint(target);
         //                  mOutLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(proxy, target));
         //               }
         //            }
         //         }
         //      }
         //   }
         //}
#endregion

         //bind the connections in the worklist.
         foreach (KeyValuePair<IControlPoint, IControlPoint> pair in mOutLinks)
         {
            pair.Key.ConnectControlPoint(pair.Value);
         }

         mbLoading = false;

         //shared variables
      }

      IControlPoint GetOutputControlPoint(IControlPoint originalSender)
      {
         if (mOutputControlPointMap.ContainsKey(originalSender) == false)
         {
            BasicControlPoint proxy = new BasicControlPoint(mOuterGroup);
            proxy.Virtual = true;


            //proxy.mbDontHide = true;            

            proxy.SetDirection(new Point(250, 0));

            proxy.TargetKey = originalSender.TargetKey;
            proxy.TypeKey = originalSender.TypeKey;
            //proxy.BackColor = originalSender.BackColor;
            proxy.TagObject = this;
            proxy.ControlPointDescription = proxy.ControlPointDescription;

            proxy.SetToolTipText(proxy.ControlPointDescription);


            proxy.ProxyTarget = originalSender;


            mControlPoints.Add(proxy);
            GroupOutpuHardPointsBar.AddControl(proxy);
            mOutputControlPointMap[originalSender] = proxy;


            proxy.ControlPointConnected += new BasicControlPoint.ControlPointEvent(outproxy_ControlPointConnected);
            proxy.ControlPointRemoved += new BasicControlPoint.ControlPointEvent(outproxy_ControlPointRemoved);

         }


         return mOutputControlPointMap[originalSender];

      }

      IControlPoint GetInputControlPoint(IControlPoint originalSender, IControlPoint originalTarget)
      {
         if (mInputControlPointMap.ContainsKey(originalTarget) == false)
         {
            originalTarget.HasProxy = true;

            BasicControlPoint proxy = new BasicControlPoint(mOuterGroup);
            proxy.Virtual = true;

            proxy.SetDirection(new Point(-250, 0));

            //proxy.ChainChild = target;
            proxy.TargetKey = originalTarget.TargetKey;
            proxy.TypeKey = originalTarget.TypeKey;
            //proxy.BackColor = originalSender.BackColor;
            //proxy.ControlPointDescription = "Proxy";
            proxy.ControlPointDescription = this.GroupUI.Title + "." + originalTarget.ControlPointDescription;

            proxy.SetToolTipText(proxy.ControlPointDescription);

            proxy.Name = originalTarget.ToString();
            proxy.TagObject = this;
            proxy.ProxyTarget = originalTarget;

            mControlPoints.Add(proxy); //?
            GoupInputHardPointsBar.AddControl(proxy);
            mInputControlPointMap[originalTarget] = proxy;

            proxy.SenderControlPointConnected += new BasicControlPoint.ControlPointEvent(inproxy_ControlPointConnected);
            proxy.SenderControlPointRemoved += new BasicControlPoint.ControlPointEvent(inproxy_ControlPointRemoved);
         }


         return mInputControlPointMap[originalTarget];

      }

      void inproxy_ControlPointConnected(BasicControlPoint cp, IControlPoint other)
      {

         if (!cp.MarkForDelete && !mbLoading)
         {
            if (other.CanConnect(cp.ProxyTarget))
            {
               other.ConnectControlPoint(cp.ProxyTarget);

            }
         }
         if (!mbLoading)
            mHost.UIUpdate(mGroupUI, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);

         mHost.SetGroupsDirty();

      }

      void inproxy_ControlPointRemoved(BasicControlPoint cp, IControlPoint other)
      {

         if (!cp.MarkForDelete && !mbLoading)
         {
            other.DisconnectControlPoint(cp.ProxyTarget);
         }
         if (!cp.MarkForDelete && !mbLoading)
            mHost.UIUpdate(mGroupUI, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);

         mHost.SetGroupsDirty();


      }
      void outproxy_ControlPointConnected(BasicControlPoint cp, IControlPoint other)
      {

         if (!cp.MarkForDelete && !mbLoading)
         {
            if (cp.ProxyTarget.CanConnect(other))
            {
               cp.ProxyTarget.ConnectControlPoint(other);
            }
         }
         if (!cp.MarkForDelete && !mbLoading)
            mHost.UIUpdate(mGroupUI, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);

         mHost.SetGroupsDirty();

      }

      void outproxy_ControlPointRemoved(BasicControlPoint cp, IControlPoint other)
      {

         if (!cp.MarkForDelete && !mbLoading)
         {
            cp.ProxyTarget.DisconnectControlPoint(other);
         }
         if (!cp.MarkForDelete && !mbLoading)
            mHost.UIUpdate(mGroupUI, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);

         mHost.SetGroupsDirty();

      }

      private void RefreshLines()
      {


      }
      private void GroupTitleTextBox_TextChanged(object sender, EventArgs e)
      {
         mGroupUI.Title = this.GroupTitleTextBox.Text;

         if(mHost != null)
         {
            mHost.SetTabText(mGroupUI.InternalGroupID, mGroupUI.Title);
         }
      }

      #region IControlPointOwner Members

      public List<IControlPoint> GetControlPoints()
      {
         //return mControlPoints;
         List<IControlPoint> controlpoints = new List<IControlPoint>();
         controlpoints.AddRange(mInputControlPointMap.Values);
         controlpoints.AddRange(mOutputControlPointMap.Values);
         return controlpoints;
      }

      #endregion

      #region ISelectable Members

      public Rectangle GetBounds()
      {
         return this.Bounds;
      }

      public void SelectControl()
      {
         this.BorderStyle = BorderStyle.FixedSingle;
         mbSelected = true;
      }
      bool mbSelected = false;

      public void DeSelectControl()
      {
         this.BorderStyle = BorderStyle.None;
         mbSelected = false;
      }

      public bool IsSelected()
      {
         return mbSelected;
      }

      #endregion

      #region IDeletable Members

      public void Delete()
      {
         if(mInnerGroup.Controls.Count > 1)
         {
            if(MessageBox.Show("Delete " + this.GroupUI.Title, "Delete Group and contents?" , MessageBoxButtons.YesNo) != DialogResult.Yes)
            {
               return;
            }
         }

         mOuterGroup.Controls.Remove(this);
         mTriggerNamespace.TriggerEditorData.UIData.mGroups.Remove(this.GroupUI);
         List<Control> markfordelete = new List<Control>();
         foreach(Control c in mInnerGroup.Controls)
         {
            markfordelete.Add(c);
         }

         foreach (Control c in markfordelete)
         {
            IDeletable d = c as IDeletable;
            if (d != null)
            {
               d.Delete();
            }

         }
         mInnerGroup.Controls.Clear();




         foreach (IControlPoint p in mControlPoints)
         {
            p.MarkForDelete = true;
         }
         mHost.DeleteGroupTab(this.GroupUI.InternalGroupID);
         mHost.Refresh();
         
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

         //BasicArgument ba = arguments as BasicArgument;
         //if (data == null)
         //{

         //}
         //else if ((data == mGroupUI) && (ba != null))
         //{
         //   if (ba.mArgument == BasicArgument.eArgumentType.Refresh)
         //   {
         //      RecalculateGroup();
         //   }
         //}
      
      
      }

      #endregion
   }
}
