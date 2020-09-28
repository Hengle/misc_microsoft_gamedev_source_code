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
   public partial class InnerGroupControl : ClientNodeControl, IControlPointOwner, ISelectable, ITriggerUIUpdate
   {
      public InnerGroupControl(TriggerHostArea host)
      {
         InitializeComponent();

         mHost = host;

         this.AddDragSurface(this);
         this.AddDragSurface(this.TitleLabel);

         this.Move += new EventHandler(InnerGroupControl_Move);

      }


      List<IControlPoint> mControlPoints = new List<IControlPoint>();
      TriggerHostArea mHost = null;


      public enum eInnerGroupType
      {
         Input,
         Output
      }
      eInnerGroupType mInnerGroupType;
      public eInnerGroupType InnerGroupType
      {
         get
         {
            return mInnerGroupType;
         }
         set
         {
            mInnerGroupType = value;
            if (mInnerGroupType == eInnerGroupType.Input)
            {
               this.GroupInputHardPointsBar.Visible = true;
               this.GroupOutputHardPointsBar.Visible = false;
               this.TitleLabel.Text = "External Input";
            }
            else
            {
               this.GroupInputHardPointsBar.Visible = false;
               this.GroupOutputHardPointsBar.Visible = true;
               this.TitleLabel.Text = "External Output";
            }
         }

      }

      public override void SetGroupID(int id)
      {
         //mGroupUI.GroupID = id;
         //if (mHost != null)
         //{
         //   int parent = mHost.GetGroupParent(id);
         //   mOuterGroup = mHost.GetHostByGroupID(parent);
         //}
         base.SetGroupID(id);
      }


      NodeHostControl mOuterGroup;
      NodeHostControl mInnerGroup;
      TriggerNamespace mTriggerNamespace;

      //asdf List<IControlPoint> mControlPoints = new List<IControlPoint>();

      Dictionary<IControlPoint, IControlPoint> mInputControlPointMap = new Dictionary<IControlPoint, IControlPoint>();
      Dictionary<IControlPoint, IControlPoint> mOutputControlPointMap = new Dictionary<IControlPoint, IControlPoint>();


      //List<TriggerVarri

      public int mInnerGroupID;



      GroupUI mGroupUI = new GroupUI();
      public GroupUI GroupUI
      {
         get
         {

            if (mInnerGroupType == eInnerGroupType.Input)
            {
               mGroupUI.iX = this.Left;
               mGroupUI.iY = this.Top;
            }
            else
            {
               mGroupUI.oX = this.Left;
               mGroupUI.oY = this.Top;
            }
            //mGroupUI.Height = this.Height;
            //mGroupUI.Width = this.Width;
            //mGroupUI.GroupID = this.GetGroupID();
            //mGroupUI.InternalGroupID = mInnerGroupID;
            return mGroupUI;
         }
         set
         {
            mGroupUI = value;
            if (mInnerGroupType == eInnerGroupType.Input)
            {
               this.Left = mGroupUI.iX;
               this.Top = mGroupUI.iY;
            }
            else
            {
               this.Left = mGroupUI.oX;
               this.Top = mGroupUI.oY;
            }
            //this.SetGroupID( mGroupUI.GroupID );
            //mInnerGroupID = mGroupUI.InternalGroupID;
         }


      }
      void InnerGroupControl_Move(object sender, EventArgs e)
      {
         if (mInnerGroupType == eInnerGroupType.Input)
         {
            mGroupUI.iX = this.Left;
            mGroupUI.iY = this.Top;
         }
         else
         {
            mGroupUI.oX = this.Left;
            mGroupUI.oY = this.Top;
         }
      }

      public void RecalculateGroup(bool clear)
      {

         if (clear)
         {
            ClearData();
         }

         mbLoading = true;

         int parent = mHost.GetGroupParent(this.mInnerGroupID);
         mOuterGroup = mHost.GetHostByGroupID(parent);

         Setup(mOuterGroup, mInnerGroup, mTriggerNamespace);

         mbLoading = false;
      }

      public void ClearData()
      {
         mbLoading = true;         

         mControlPoints.Clear();
         GroupOutputHardPointsBar.RemoveAll();
         GroupInputHardPointsBar.RemoveAll();

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



      bool mbLoading = false;

      List<KeyValuePair<IControlPoint, IControlPoint>> mInLinks = new List<KeyValuePair<IControlPoint, IControlPoint>>();
      List<KeyValuePair<IControlPoint, IControlPoint>> mOutLinks = new List<KeyValuePair<IControlPoint, IControlPoint>>();

      int mOuterGroupID;

      public void Setup(NodeHostControl outerGroup, NodeHostControl innerGroup, TriggerNamespace triggerNamespace)
      {       

         mbLoading = true;

         base.mHost = innerGroup;

         mOuterGroup = outerGroup;
         mInnerGroup = innerGroup;
         mInnerGroupID = innerGroup.mGroupID;
         mTriggerNamespace = triggerNamespace;
         mOuterGroupID = outerGroup.mGroupID;

         List<KeyValuePair<IControlPoint, IControlPoint>> alllinks = mHost.GetLinks();
         List<IControlPoint> virtualCPs = mHost.GetVirtualControlPoints(mInnerGroupID);


         if (mInnerGroupType == eInnerGroupType.Input)
         {
            mInLinks.Clear();

            //Pointing "in"
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
               //&& (pair.Value.GetLevel() != mInnerGroupID))

               bool b1 = ki != mInnerGroupID;
               bool b2 = !mHost.IsChild(ki, mInnerGroupID);
               bool b3 = mHost.IsChild(vi, mInnerGroupID) || (vi == mInnerGroupID);

               if(b1 && b2 && b3)
               {
                  IControlPoint proxy = GetExternalInputControlPoint(pair.Key);
                  proxy.ParentConnections.Add(pair);
                  
                  IControlPoint foundPoint;
                  if (TryGetControlPointAtLevel(mInnerGroupID, virtualCPs, pair.Value, pair, out foundPoint))
                  {
                     mInLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(proxy, foundPoint));
                  }

               }


            }


            #region omfg
            //foreach (Control c in mOuterGroup.Controls)
            //{
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
            //               //|| (mHost.IsChild(mInnerGroup.mGroupID, targetControl.GetGroupID())))
            //               //|| (mHost.IsChild(mInnerGroup.mGroupID, targetControl.GetGroupID())))
            //               || (mHost.IsChild(targetControl.GetGroupID(), mInnerGroup.mGroupID)))
            //               {
            //                  IControlPoint proxy = GetExternalInputControlPoint(cpoint);
            //                  //proxy.ConnectControlPoint(target);
            //                  mInLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(proxy, target));

            //               }
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


         }
         else if (mInnerGroupType == eInnerGroupType.Output)
         {
            mOutLinks.Clear();

            //pointing "out"


            foreach (KeyValuePair<IControlPoint, IControlPoint> pair in alllinks)
            {
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

               bool b1 = vi != mInnerGroupID;
               bool b2 = !mHost.IsChild(vi, mInnerGroupID);
               bool b3 = mHost.IsChild(ki, mInnerGroupID) || (ki == mInnerGroupID);

               if(b1 && b2 && b3)
               {
                  IControlPoint proxy = GetExternalOutputControlPoint(pair.Key, pair.Value);
                  proxy.ParentConnections.Add(pair);

                  IControlPoint foundPoint;
                  if (TryGetControlPointAtLevel(mInnerGroupID, virtualCPs, pair.Key, pair, out foundPoint))
                  {
                     mOutLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(foundPoint, proxy));
                  }
               }
            }

            #region omfg
            //foreach (Control c in mInnerGroup.Controls)
            //{
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
            //               if (targetControl.GetGroupID() == mOuterGroup.mGroupID)
            //               {
            //                  IControlPoint proxy = GetExternalOutputControlPoint(cpoint, target);
            //                  mOutLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(cpoint, proxy));
            //               }
            //               //if (targetControl.GetGroupID() != mInnerGroup.mGroupID)
            //               //{
            //               //   IControlPoint proxy = GetExternalOutputControlPoint(cpoint, target);
            //               //   worklist.Add(new KeyValuePair<IControlPoint, IControlPoint>(cpoint, proxy));
            //               //}
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

         }

         //shared variables

         mbLoading = false;
      }

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


      // GroupInputHardPointsBar
      IControlPoint GetExternalInputControlPoint(IControlPoint originalSender)
      {
         if (mInputControlPointMap.ContainsKey(originalSender) == false)
         {
            BasicControlPoint proxy = new BasicControlPoint(mOuterGroup);
            //proxy.mbDontHide = true;
            proxy.Virtual = true;


            proxy.SetDirection(new Point(250, 0));

            proxy.TargetKey = originalSender.TargetKey;
            proxy.TypeKey = originalSender.TypeKey;
            //proxy.BackColor = originalSender.BackColor;
            proxy.TagObject = this;
            proxy.ControlPointDescription = proxy.ControlPointDescription;

            proxy.ProxyTarget = originalSender;

            mControlPoints.Add(proxy);
            GroupInputHardPointsBar.AddControl(proxy);
            mInputControlPointMap[originalSender] = proxy;


            proxy.ControlPointConnected += new BasicControlPoint.ControlPointEvent(inproxy_ControlPointConnected);
            proxy.ControlPointRemoved += new BasicControlPoint.ControlPointEvent(inproxy_ControlPointRemoved);

         }
         return mInputControlPointMap[originalSender];

      }

      IControlPoint GetExternalOutputControlPoint(IControlPoint originalSender, IControlPoint originalTarget)
      {
         if (mOutputControlPointMap.ContainsKey(originalTarget) == false)
         {
            originalTarget.HasProxy = true;
            

            BasicControlPoint proxy = new BasicControlPoint(mOuterGroup);
            proxy.Virtual = true;

            proxy.SetDirection(new Point(-250, 0));

            //proxy.ChainChild = target;
            proxy.TargetKey = originalTarget.TargetKey;
            proxy.TypeKey = originalTarget.TypeKey;
            //proxy.BackColor = originalSender.BackColor;
            proxy.ControlPointDescription = originalTarget.ControlPointDescription;

            proxy.Name = originalTarget.ToString();
            proxy.TagObject = this;
            proxy.ProxyTarget = originalTarget;


            mControlPoints.Add(proxy); //?
            GroupOutputHardPointsBar.AddControl(proxy);
            mOutputControlPointMap[originalTarget] = proxy;
         
            proxy.SenderControlPointConnected += new BasicControlPoint.ControlPointEvent(outproxy_ControlPointConnected);
            proxy.SenderControlPointRemoved += new BasicControlPoint.ControlPointEvent(outproxy_ControlPointRemoved);

         }
         return mOutputControlPointMap[originalTarget];

      }


      void inproxy_ControlPointConnected(BasicControlPoint cp, IControlPoint other)
      {
         if (!cp.MarkForDelete && !mbLoading)
         {
            if (cp.ProxyTarget.CanConnect(other))
            {
               cp.ProxyTarget.ConnectControlPoint(other);
            }
         }
         if (!cp.MarkForDelete && !mbLoading)
         {
            mHost.UIUpdate(mGroupUI, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);
         }
         mHost.SetGroupsDirty();

      }

      void inproxy_ControlPointRemoved(BasicControlPoint cp, IControlPoint other)
      {
         if (!cp.MarkForDelete && !mbLoading)
         {
            cp.ProxyTarget.DisconnectControlPoint(other);
         }
         if (!cp.MarkForDelete && !mbLoading)
         {            
            mHost.UIUpdate(mGroupUI, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);
         }
         mHost.SetGroupsDirty();

      }
      void outproxy_ControlPointConnected(BasicControlPoint cp, IControlPoint other)
      {
         if (!cp.MarkForDelete && !mbLoading)
         {
            if (other.CanConnect(cp.ProxyTarget))
            {
               other.ConnectControlPoint(cp.ProxyTarget);
            }
         }
         if (!cp.MarkForDelete && !mbLoading)
         {
            mHost.UIUpdate(mGroupUI, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);
         }
         mHost.SetGroupsDirty();

      }

      void outproxy_ControlPointRemoved(BasicControlPoint cp, IControlPoint other)
      {
         if (!cp.MarkForDelete && !mbLoading)
         {
            other.DisconnectControlPoint(cp.ProxyTarget);
         }
         if (!cp.MarkForDelete && !mbLoading)
         {
            mHost.UIUpdate(mGroupUI, new BasicArgument(BasicArgument.eArgumentType.Refresh), eUpdateVisibilty.AnyVisiblity);
         }
         mHost.SetGroupsDirty();
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

      #region ITriggerUIUpdate Members
      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity)
      {
         List<Control> notused = new List<Control>();
         UIUpdate(data, arguments, visiblity, ref notused);
      }
      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity, ref List<Control> owners)
      {
         //mbLoading = true;
         //mbUpdateState++;

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

         //mbLoading = false;

         //mbUpdateState--;
      }

      #endregion

      int mbUpdateState = 0;

   }
}
