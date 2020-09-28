using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Reflection;

using SimEditor;
using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class TriggerHostArea : UserControl, INodeHostControlOwner, ITriggerUIUpdate //NodeHostControl
   {
      public TriggerHostArea()
      {
         InitializeComponent();

         LoadContextMenu();
         LoadSelectedContextMenu();

         this.tabControl1.SelectedIndexChanged += new EventHandler(tabControl1_SelectedIndexChanged);

      }
      bool once = false;
       int lastClickedGroup = -1;
       bool reloadLastGroup = true;

      void tabControl1_SelectedIndexChanged(object sender, EventArgs e)
      {
         if(false)
         {
            WatchHandles(this);
            once = false;

         }
         
         //GC.Collect();

         if (tabControl1.SelectedTab == null)
            return;
         foreach (Control c in tabControl1.SelectedTab.Controls)
         {
            if (c is NodeHostControl)
            {
               ActiveNodeHostControl = (NodeHostControl)c;
            }
         }
         reloadLastGroup = true;
         DoClean();
      }
      public void DoClean()
      {
         //MethodInfo m = typeof(Control).GetMethod("DestroyHandle", BindingFlags.NonPublic | BindingFlags.IgnoreCase);//, BindingFlags.NonPublic);
         //CleanHandles(this, m);

         //IntPtr handle = (IntPtr)obj2;
         //if (handle != IntPtr.Zero)
         //{
         //   SafeNativeMethods.DeleteObject(new HandleRef(this, handle));
         //}
         //MethodInfo[] m2 = typeof(Control).GetMethods(BindingFlags.NonPublic);
         //this.SetTopLevel(bool)
      }
       
       public void ReloadUI()
      {
         this.Visible = false;
         foreach (Control c in tabControl1.TabPages)
         {
            DestroyControls(c);
         }

         lastClickedGroup = this.ActiveNodeHostControl.mGroupID;
         CurrentTriggerNamespace = CurrentTriggerNamespace;
         this.Visible = true;
      }

      public void AutoUpdateTriggers()
      {
         int upgradeCount = 0;
         try
         {
            List<Trigger> triggers = mCurrentTriggerNamespace.TriggerEditorData.TriggerSystem.Trigger;//.WalkTriggers();
            foreach (Trigger t in triggers)
            {
               foreach (TriggerCondition c in mCurrentTriggerNamespace.WalkConditions(t))
               {
                  TriggerSystemDefinitions.UpgradeStatus status = TriggerSystemMain.mTriggerDefinitions.CalculateUpgrade(c);
                  if (status != TriggerSystemDefinitions.UpgradeStatus.UpToDate)
                  {
                     Dictionary<int, TriggerValue> values;
                     TriggerComponent cmp = TriggerSystemMain.mTriggerDefinitions.TryUpgrade(mCurrentTriggerNamespace, t, c, status, out values);
                     if (cmp != null)
                     {
                        upgradeCount++;
                     }
                  }
               }

               foreach (TriggerEffect c in mCurrentTriggerNamespace.WalkEffects(t))
               {
                  TriggerSystemDefinitions.UpgradeStatus status = TriggerSystemMain.mTriggerDefinitions.CalculateUpgrade(c);
                  if (status != TriggerSystemDefinitions.UpgradeStatus.UpToDate)
                  {
                     Dictionary<int, TriggerValue> values;
                     TriggerComponent cmp = TriggerSystemMain.mTriggerDefinitions.TryUpgrade(mCurrentTriggerNamespace, t, c, status, out values);
                     if (cmp != null)
                     {
                        upgradeCount++;
                     }
                  }
               }
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

         if (upgradeCount > 0)
         {
            mTriggerControls.Clear();
            ReloadUI();
         }
         CoreGlobals.ShowMessage(upgradeCount.ToString() + " Effects and Conditions upgraded to latest version");
      }
      
      
      public void DestroyControls(Control c)
      {

         foreach (Control cc in c.Controls)
         {
            DestroyControls(cc);
            cc.Dispose();
         }
      }

      public void CleanHandles(Control c, MethodInfo m)
      {

         foreach (Control cc in c.Controls)
         {
            if (cc.Visible == false)
            {
               //cc.HandleCreated += new EventHandler(c_HandleCreated);
               //cc.HandleDestroyed += new EventHandler(c_HandleDestroyed);
               m.Invoke(cc,null);

            }
            WatchHandles(cc);
         }
      }

      public void WatchHandles(Control c)
      {
         
         foreach(Control cc in c.Controls)
         {
            if(cc.Visible == false)
            {
               cc.HandleCreated += new EventHandler(c_HandleCreated);
               cc.HandleDestroyed += new EventHandler(c_HandleDestroyed);
            }
            WatchHandles(cc);
         }
      }
      int des = 0;
      int create = 0;
      void c_HandleDestroyed(object sender, EventArgs e)
      {
         des++;
         //throw new Exception("The method or operation is not implemented.");
      }

      void c_HandleCreated(object sender, EventArgs e)
      {
         create++;
         //throw new Exception("The method or operation is not implemented.");
      }

      public event EventHandler ViewChanged;

      NodeHostControl mActiveNodeHostControl = null;
      public NodeHostControl ActiveNodeHostControl
      {
         set
         {
            mActiveNodeHostControl = value;
            if (ViewChanged != null)
            {
               ViewChanged.Invoke(this, null);
            }
         }
         get
         {
            return mActiveNodeHostControl;
         }
      }

      public void ShowGroupTab(int groupID)
      {
         foreach (TabPage t in tabControl1.TabPages)
         {
            NodeHostControl c = t.Tag as NodeHostControl;
            if (c == mHostByGroupID[groupID])
            {
               tabControl1.SelectedTab = t;
               continue;
            }            
         }
      }
      public void DeleteGroupTab(int id)
      {
         if (mTabsByGroupID.ContainsKey(id))
         {
            tabControl1.TabPages.Remove(mTabsByGroupID[id]);

         }
      }

      public void InitNodeHosts()
      {
         mRootLevel = new NodeHostControl();
         mHostByGroupID = new Dictionary<int, NodeHostControl>();
         mHostByGroupID[-1] = mRootLevel;
         tabControl1.TabPages.Clear();

         GC.Collect();

         CreateTab("Top", mRootLevel, -1);
         ActiveNodeHostControl = mRootLevel;

         //Load Group Tabs
         foreach(GroupUI groupui in mCurrentTriggerNamespace.TriggerEditorData.UIData.mGroups)
         {
            NodeHostControl group = LoadGroup(groupui);

         }
         if (lastClickedGroup != -1 && reloadLastGroup == true)
         {
             ShowGroupTab(lastClickedGroup);
             lastClickedGroup = -1;
             reloadLastGroup = false;
         }
      }

      public void SetTabText(int groupID, string text)
      {
         if (groupID != -1)
         {
            mTabsByGroupID[groupID].Text = text;
         }
      }


      protected void CreateTab(string name, NodeHostControl host, int groupid)
      {
         BetterTabPage p = new BetterTabPage();
         p.owhShap = this;
         p.BackColor = Color.White;
         p.Controls.Add(host);
         //host.Dock = DockStyle.Fill;
         p.Text = name;
         p.Tag = host;
         tabControl1.TabPages.Add(p);
         host.Owner = this;
         host.AutoScrollMinSize = new Size(6000, 6000);

         host.Size = p.Size;
         host.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
         host.AutoScroll = false;

         mTabsByGroupID[groupid] = p;

      }
      protected NodeHostControl CreateNewGroup()
      {
         NodeHostControl newGroup =  new NodeHostControl();

         int max = int.MinValue;
         foreach (int i in mHostByGroupID.Keys)
         {
            if (i > max)
            {
               max = i;
            }
         }
         max++;

         mHostByGroupID[max] = newGroup;
         newGroup.mGroupID = max;
         CreateTab(max.ToString(), newGroup, max);

         return newGroup;
      }
      Dictionary<int, TabPage> mTabsByGroupID = new Dictionary<int, TabPage>();

      protected NodeHostControl LoadGroup(GroupUI groupUI)
      {
         NodeHostControl group = new NodeHostControl();
         mHostByGroupID[groupUI.InternalGroupID] = group;
         group.mGroupID = groupUI.InternalGroupID;
         CreateTab(groupUI.InternalGroupID.ToString(), group, groupUI.InternalGroupID);

         SetTabText(groupUI.InternalGroupID, groupUI.Title);

         return group;
      }

      protected NodeHostControl GetOrCreateHostByGroupID(int id)
      {
         if (mHostByGroupID.ContainsKey(id) == false)
         {
            mHostByGroupID[id] = new NodeHostControl();
            mHostByGroupID[id].mGroupID = id;
            CreateTab(id.ToString(), mRootLevel, id);
         }
         return mHostByGroupID[id];
      }


      public NodeHostControl mRootLevel = null;
      Dictionary<int, NodeHostControl> mHostByGroupID = null;
      public NodeHostControl GetHostByGroupID(int id)
      {
         return mHostByGroupID[id];
      }


      TriggerNamespace mCurrentTriggerNamespace = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public TriggerNamespace CurrentTriggerNamespace
      {
         set
         {
            mCurrentTriggerNamespace = value;

            InitNodeHosts();

            LoadNamespace();

            this.Refresh();

            this.LoadContextMenu();
            this.LoadSelectedContextMenu();
         }
         get
         {
            //mCurrentTriggerNamespace.TriggerEditorData.UIData.mNoteNodes.Clear();

            

            //foreach (NoteNodeXml note in mCurrentTriggerNamespace.TriggerEditorData.UIData.mNoteNodes)
            //{
            //   NotesNode n = new NotesNode();
            //   n.LoadFromData(note);

            //   nodehost.InsertControl(n, n.Trigger.X, n.Trigger.Y);


            //}

            return mCurrentTriggerNamespace;
         }
      }


      private void LoadNamespace()
      {



         foreach (Trigger trig in mCurrentTriggerNamespace.TriggerEditorData.TriggerSystem.Trigger)
         {
            NodeHostControl nodehost = GetOrCreateHostByGroupID(trig.GroupID);
            TriggerControl t = new TriggerControl(nodehost,this);
            t.ParentTriggerNamespace = this.CurrentTriggerNamespace;
            t.Trigger = trig;
            nodehost.InsertControl(t, t.Trigger.X, t.Trigger.Y);

         }

         foreach (Trigger trig in mCurrentTriggerNamespace.TriggerEditorData.TriggerSystem.Trigger)
         {
            TriggerControl tc = GetTriggerControl(trig.ID);
            if (tc != null)
            {
               tc.LoadExistingConnections();
            }
         }

         foreach (TriggerTemplateMapping mapping in mCurrentTriggerNamespace.TriggerEditorData.TriggerMappings)
         {
            NodeHostControl nodehost = GetOrCreateHostByGroupID(mapping.GroupID);
            TemplateControl t = new TemplateControl(nodehost, this);
            t.ParentTriggerNamespace = this.CurrentTriggerNamespace;
            t.TriggerTemplateMapping = mapping;
            nodehost.InsertControl(t, mapping.X, mapping.Y);
         }

         foreach (TriggerTemplateMapping mapping in mCurrentTriggerNamespace.TriggerEditorData.TriggerMappings)
         {
            TemplateControl tc = GetTemplatetControl(mapping.ID);
            if (tc != null)
            {

               tc.LoadExistingConnections();
            }
         }

         foreach (NoteNodeXml note in mCurrentTriggerNamespace.TriggerEditorData.UIData.mNoteNodes)
         {
            NotesNode n = new NotesNode();
            NodeHostControl nodehost = GetOrCreateHostByGroupID(note.GroupID);
            n.mHost = nodehost;
            n.ParentTriggerNamespace = this.CurrentTriggerNamespace;
            n.LoadFromData(note);
            nodehost.InsertControl(n, n.Left, n.Top);           
         }


         foreach (GroupUI groupui in mCurrentTriggerNamespace.TriggerEditorData.UIData.mGroups)
         {
            NodeHostControl nodehost = GetOrCreateHostByGroupID(groupui.GroupID);
            NodeHostControl innernodehost = GetOrCreateHostByGroupID(groupui.InternalGroupID);

            GroupControl groupControl = new GroupControl(this);
            groupControl.GroupUI = groupui;
            groupControl.SetGroupID(groupui.GroupID);
            //groupControl.Setup(nodehost, innernodehost, this.mCurrentTriggerNamespace);
            //nodehost.InsertControl(groupControl, groupui.X, groupui.Y);

            mGroupControls[groupui.InternalGroupID] = groupControl;

            //int ox = groupui.oX;
            //int oy = groupui.oY;
            //int ix = groupui.iX;
            //int iy = groupui.iY;

            //CreateInternalGroupControls(groupui, innernodehost, nodehost, ox, oy, ix, iy);

         }
         foreach (GroupUI groupui in mCurrentTriggerNamespace.TriggerEditorData.UIData.mGroups)
         {
            NodeHostControl nodehost = GetOrCreateHostByGroupID(groupui.GroupID);
            NodeHostControl innernodehost = GetOrCreateHostByGroupID(groupui.InternalGroupID);

            GroupControl groupControl = mGroupControls[groupui.InternalGroupID];
            //GroupControl groupControl = new GroupControl(this);
            //groupControl.GroupUI = groupui;
            //groupControl.SetGroupID(groupui.GroupID);
            groupControl.Setup(nodehost, innernodehost, this.mCurrentTriggerNamespace);
            nodehost.InsertControl(groupControl, groupui.X, groupui.Y);

            //mGroupControls[groupui.InternalGroupID] = groupControl;

            int ox = groupui.oX;
            int oy = groupui.oY;
            int ix = groupui.iX;
            int iy = groupui.iY;

            CreateInternalGroupControls(groupui, innernodehost, nodehost, ox, oy, ix, iy);

         }
      }

      Dictionary<int, InnerGroupControl> mGroupInnerInputs = new Dictionary<int, InnerGroupControl>();
      Dictionary<int, InnerGroupControl> mGroupInnerOutputs = new Dictionary<int, InnerGroupControl>();


      Dictionary<int, TemplateControl> mTemplateControls = new Dictionary<int, TemplateControl>();
      public TemplateControl GetTemplatetControl(int id)
      {
         if (mTemplateControls.ContainsKey(id))
         {
            return mTemplateControls[id];
         }
         return null;
      }


      Dictionary<int, TriggerControl> mTriggerControls = new Dictionary<int, TriggerControl>();
      public TriggerControl GetTriggerControl(int id)
      {
         if (mTriggerControls.ContainsKey(id))
            return mTriggerControls[id];
         return null;
      }

      //public IControlPoint GetProxyControlPoint(NodeHostControl host, IControlPoint cp)
      //{
      //   //ClientNodeControl
      //   ClientNodeControl node = cp.TagObject as ClientNodeControl;

      //   if (node.GetGroupID() != host.mGroupID)
      //   {
      //      if (mGroupControls.ContainsKey(node.GetGroupID()) == true)
      //      {
      //         return mGroupControls[node.GetGroupID()].GetAlternateControlPoint(cp);
      //      }
      //   }

      //   return cp;
      //}



      public void ControlAdded(Control c)
      {
         TriggerControl tc = c as TriggerControl;
         if (tc != null)
         {
            mTriggerControls[tc.Trigger.ID] = tc;
         }
         TemplateControl templ = c as TemplateControl;
         if (templ != null)
         {
            mTemplateControls[templ.TriggerTemplateMapping.ID] = templ;
         }
      }

      NodeHostControl mMenuNodeHost = null;
      public void ShowContextMenu(Control sender, int x, int y)
      {
         mMenuNodeHost = sender as NodeHostControl;

         if(ActiveNodeHostControl.GetSelected().Count > 0)
         {
            SelectedItemsContextMenuStrip.Show(sender, x, y);
         }
         else
         {        
            HostContextMenuStrip.Show(sender, x, y);
         }
      }

      protected NodeHostControl GetMenuOwner()
      {
         return mMenuNodeHost;
      }

      private void newTriggerToolStripMenuItem_Click(object sender, EventArgs e)
      {
         if (mCurrentTriggerNamespace == null)
            return;

         NodeHostControl hostConrol = GetMenuOwner();

         if (hostConrol == null)
            return;

         TriggerControl t = new TriggerControl(hostConrol, this);

         t.ParentTriggerNamespace = this.CurrentTriggerNamespace;

         Trigger trig = new Trigger();
         int newID;
         mCurrentTriggerNamespace.InsertTrigger(trig, out newID);
         trig.Name = "newTrigger" + newID.ToString();
         t.Trigger = trig;
         
         hostConrol.InsertControl(t, hostConrol.mLastPosition.X, hostConrol.mLastPosition.Y);
         t.OnMoved();
      }

      private void LoadContextMenu()
      {
         if (TriggerSystemMain.mTriggerDefinitions == null)
            return;

         HostContextMenuStrip.Items.Clear();
         ToolStripMenuItem newTriggerMenu = new ToolStripMenuItem();
         newTriggerMenu.Text = "New Trigger";
         newTriggerMenu.Click += new System.EventHandler(newTriggerToolStripMenuItem_Click);
         HostContextMenuStrip.Items.Add(newTriggerMenu);


         ToolStripMenuItem newNoteMenu = new ToolStripMenuItem();
         newNoteMenu.Text = "New Note";
         newNoteMenu.Click += new EventHandler(newNoteMenu_Click);
         HostContextMenuStrip.Items.Add(newNoteMenu);


         ToolStripMenuItem templateMenu = new ToolStripMenuItem();
         templateMenu.Text = "Template";
         HostContextMenuStrip.Items.Add(templateMenu);

         ICollection<string> templates = TriggerSystemMain.mTriggerDefinitions.GetTemplateNames();
         foreach (string s in templates)
         {
            ToolStripMenuItem newTemplateItem = new ToolStripMenuItem();
            newTemplateItem.Text = s;
            newTemplateItem.Tag = s;
            newTemplateItem.Click += new EventHandler(newTemplateItem_Click);
            templateMenu.DropDownItems.Add(newTemplateItem);
         }

         if (CoreGlobals.IsDev == true)
         {
            ToolStripMenuItem userClassesMenu = new ToolStripMenuItem();
            userClassesMenu.Text = "User Classes";
            userClassesMenu.Click += new EventHandler(userClassesMenu_Click);
            HostContextMenuStrip.Items.Add(userClassesMenu);

            ToolStripMenuItem triggerDataEditMenu = new ToolStripMenuItem();
            triggerDataEditMenu.Text = "Data Files";
            triggerDataEditMenu.Click += new EventHandler(triggerDataEditMenu_Click);
            HostContextMenuStrip.Items.Add(triggerDataEditMenu);
         }

         HostContextMenuStrip.Items.Add("-");

         ToolStripMenuItem autoUpgrade = new ToolStripMenuItem();
         autoUpgrade.Text = "Auto Update Conditions/Effects";
         autoUpgrade.Click += new EventHandler(autoUpgrade_Click);
         HostContextMenuStrip.Items.Add(autoUpgrade);


         HostContextMenuStrip.Items.Add("-");


         ToolStripMenuItem toolOpen = new ToolStripMenuItem();
         toolOpen.Text = "Group Browser";
         toolOpen.Click += new EventHandler(toolOpen_Click);
         HostContextMenuStrip.Items.Add(toolOpen);

         ToolStripMenuItem createGroupWithSelected = new ToolStripMenuItem();
         createGroupWithSelected.Text = "Create Group";
         createGroupWithSelected.Click += new EventHandler(createGroupWithSelected_Click);
         HostContextMenuStrip.Items.Add(createGroupWithSelected);

         HostContextMenuStrip.Items.Add("-");

         ToolStripMenuItem defaultValues = new ToolStripMenuItem();
         defaultValues.Text = "DefalutValues";
         defaultValues.Click += new EventHandler(defaultValues_Click);
         HostContextMenuStrip.Items.Add(defaultValues);

         ToolStripMenuItem scanTriggerScript = new ToolStripMenuItem();
         scanTriggerScript.Text = "Scan";
         scanTriggerScript.Click += new EventHandler(scanTriggerScript_Click);
         HostContextMenuStrip.Items.Add(scanTriggerScript);

         ToolStripMenuItem searchTriggerScript = new ToolStripMenuItem();
         searchTriggerScript.Text = "Search";
         searchTriggerScript.Click += new EventHandler(searchTriggerScript_Click);
         HostContextMenuStrip.Items.Add(searchTriggerScript);

         HostContextMenuStrip.Items.Add("-");  

         ToolStripMenuItem commentAllItem = new ToolStripMenuItem();
         commentAllItem.Text = "Comment All";
         commentAllItem.Click += new EventHandler(commentAllItem_Click);
         HostContextMenuStrip.Items.Add(commentAllItem);
         ToolStripMenuItem unCommentAllItem = new ToolStripMenuItem();
         unCommentAllItem.Text = "Un Comment All";
         unCommentAllItem.Click += new EventHandler(unCommentAllItem_Click);
         HostContextMenuStrip.Items.Add(unCommentAllItem);

         HostContextMenuStrip.Items.Add("-");         
         
         ToolStripMenuItem pasteFromClipboard = new ToolStripMenuItem();
         pasteFromClipboard.Text = "Paste";
         pasteFromClipboard.Click += new EventHandler(pasteFromClipboard_Click);
         HostContextMenuStrip.Items.Add(pasteFromClipboard);

      }

      void triggerDataEditMenu_Click(object sender, EventArgs e)
      {
         PopupEditor pe = new PopupEditor();
         TriggerDataFileEditor de = new TriggerDataFileEditor();
         de.SetTriggerNamespace(mCurrentTriggerNamespace);
         pe.ShowPopup(this, de);
      }

      void userClassesMenu_Click(object sender, EventArgs e)
      {
         PopupEditor pe = new PopupEditor();
         TriggerUserClass userClassMenu = new TriggerUserClass();

         //userClassMenu.SetUserClasses(mCurrentTriggerNamespace.TriggerEditorData.UIData.mUserClasses);
         pe.ShowPopup(this, userClassMenu);
      }

      void autoUpgrade_Click(object sender, EventArgs e)
      {
         this.AutoUpdateTriggers();
      }

      void searchTriggerScript_Click(object sender, EventArgs e)
      {
         TriggerSearch search = new TriggerSearch();

         search.Init(this);

         PopupEditor pe = new PopupEditor();
         pe.ShowPopup(this, search, FormBorderStyle.SizableToolWindow);
      
      }

      void toolOpen_Click(object sender, EventArgs e)
      {
         TriggerScriptTools t = new TriggerScriptTools();
         t.HostArea = this;

         PopupEditor pe = new PopupEditor();
         pe.ShowPopup(this, t, FormBorderStyle.SizableToolWindow);
      }

      void unCommentAllItem_Click(object sender, EventArgs e)
      {
         foreach (Control c in ActiveNodeHostControl.Controls)
         {
            ICommentOutable co = c as ICommentOutable;
            if (co != null)
            {
               co.CommentOut = false;
               c.Update();
            }
         }
      }

      void commentAllItem_Click(object sender, EventArgs e)
      {
         foreach (Control c in ActiveNodeHostControl.Controls)
         {
            ICommentOutable co = c as ICommentOutable;
            if (co != null)
            {
               co.CommentOut = true;
               c.Update();

            }
         }
      }

      void newNoteMenu_Click(object sender, EventArgs e)
      {
         if (mCurrentTriggerNamespace == null)
            return;

         NodeHostControl hostConrol = GetMenuOwner();

         if (hostConrol == null)
            return;

         NotesNode n = new NotesNode();
         n.mHost = hostConrol;

         n.ParentTriggerNamespace = this.CurrentTriggerNamespace;
         n.ParentTriggerNamespace.TriggerEditorData.UIData.mNoteNodes.Add(n.GetData());

         hostConrol.InsertControl(n, hostConrol.mLastPosition.X, hostConrol.mLastPosition.Y);
         n.OnMoved();
      }

      private void LoadSelectedContextMenu()
      {
         if (TriggerSystemMain.mTriggerDefinitions == null)
            return;
         SelectedItemsContextMenuStrip = new ContextMenuStrip();



         ToolStripMenuItem toolOpen = new ToolStripMenuItem();
         toolOpen.Text = "Group Browser";
         toolOpen.Click += new EventHandler(toolOpen_Click);
         SelectedItemsContextMenuStrip.Items.Add(toolOpen);

         ToolStripMenuItem createGroupWithSelected = new ToolStripMenuItem();
         createGroupWithSelected.Text = "Create Group";
         createGroupWithSelected.Click += new EventHandler(createGroupWithSelected_Click);
         SelectedItemsContextMenuStrip.Items.Add(createGroupWithSelected);

         ToolStripMenuItem removeFromGroup = new ToolStripMenuItem();
         removeFromGroup.Text = "Remove From Group";
         removeFromGroup.Click += new EventHandler(removeFromGroup_Click);
         SelectedItemsContextMenuStrip.Items.Add(removeFromGroup);

         ToolStripMenuItem addToGroup = new ToolStripMenuItem();
         addToGroup.Text = "Add to Group";
         addToGroup.Click += new EventHandler(addToGroup_Click);
         SelectedItemsContextMenuStrip.Items.Add(addToGroup);


         SelectedItemsContextMenuStrip.Items.Add("-"); 

         ToolStripMenuItem commentItem = new ToolStripMenuItem();
         commentItem.Text = "Comment";
         commentItem.Click += new EventHandler(commentItem_Click);
         SelectedItemsContextMenuStrip.Items.Add(commentItem);
         ToolStripMenuItem unCommentItem = new ToolStripMenuItem();
         unCommentItem.Text = "Un Comment";
         unCommentItem.Click += new EventHandler(unCommentItem_Click);
         SelectedItemsContextMenuStrip.Items.Add(unCommentItem);


         SelectedItemsContextMenuStrip.Items.Add("-"); 

         ToolStripMenuItem deleteSelected = new ToolStripMenuItem();
          deleteSelected.Text = "Delete Selected";
         deleteSelected.Click += new EventHandler(deleteSelected_Click);
         SelectedItemsContextMenuStrip.Items.Add(deleteSelected);

         SelectedItemsContextMenuStrip.Items.Add("-");

         ToolStripMenuItem copyToClipboard = new ToolStripMenuItem();
         copyToClipboard.Text = "Copy";
         copyToClipboard.Click += new EventHandler(copyToClipboard_Click);
         SelectedItemsContextMenuStrip.Items.Add(copyToClipboard);

         ToolStripMenuItem pasteFromClipboard = new ToolStripMenuItem();
         pasteFromClipboard.Text = "Paste";
         pasteFromClipboard.Click += new EventHandler(pasteFromClipboard_Click);
         SelectedItemsContextMenuStrip.Items.Add(pasteFromClipboard);

         //createGroupWithSelected.Enabled = false;
         //copyToClipboard.Enabled = false;

      }


      public void SetDirty()
      {
         RecalculateGroups();
      }


      void unCommentItem_Click(object sender, EventArgs e)
      {
         List<Control> selected = ActiveNodeHostControl.GetSelected();
         foreach (Control c in selected)
         {
            ICommentOutable co = c as ICommentOutable;
            if (co != null)
            {
               co.CommentOut = false;
               c.Update();

            }
         }
      }

      void commentItem_Click(object sender, EventArgs e)
      {
         RecalculateGroups();

         List<Control> selected = ActiveNodeHostControl.GetSelected();
         foreach (Control c in selected)
         {
            ICommentOutable co = c as ICommentOutable;
            if (co != null)
            {
               co.CommentOut = true;
               c.Update();

            }
         }
      }


      //special copy logic with groups..???
      void copyToClipboard_Click(object sender, EventArgs e)
      {
         
         sTriggerClipboard = new TriggerClipboard(); 

         List<Control> selected = ActiveNodeHostControl.GetSelected();
         if (selected.Count > 0)
         {


            //foreach(TriggerValue v in CurrentTriggerNamespace.GetValueList())
            //{
            //   TriggerValue valCopy = v.GetCopy();
            //   sTriggerClipboard.mValues[valCopy.ID] = (valCopy);
            //}
            foreach (TriggerValue v in CurrentTriggerNamespace.GetValueList())
            {
               TriggerValue valCopy = v.GetCopy();
               sTriggerClipboard.mValues[valCopy.ID] = (valCopy);
            }
            //sTriggerClipboard.mValues

            foreach (Control c in selected)
            {
               if (c is TemplateControl)
                  continue;

               ICopyPaste copyControl = c as ICopyPaste;
               if (copyControl != null)
               {
                  sTriggerClipboard.mData.Add(copyControl.MakeCopy());
                  sTriggerClipboard.mTypes.Add(copyControl.GetType());
                  sTriggerClipboard.AddPoint(new Point(c.Left, c.Top));
               }
            }
         }

      }

      public static TriggerClipboard sTriggerClipboard = null;

      void pasteFromClipboard_Click(object sender, EventArgs e)
      {
         Point p = this.mActiveNodeHostControl.PointToClient(Cursor.Position);
         int x = p.X;
         int y = p.Y;

         Dictionary<int, int> triggerMapping = new Dictionary<int, int>();
         List<Control> newControls = new List<Control>();
         if(sTriggerClipboard != null)
         {
            //foreach (TriggerValue v in sTriggerClipboard.mValues.Values)
            //{
            //   TriggerValue valCopy = v.GetCopy();
            //   sTriggerClipboard.mValues[valCopy.ID] = (valCopy);
            //}

            for(int i=0; i < sTriggerClipboard.mData.Count; i++)
            {
               Type t = sTriggerClipboard.mTypes[i];
               ConstructorInfo con = t.GetConstructor(new Type[] { });
               if(con != null)
               {
                  Control newControl = con.Invoke(new object[] { }) as Control;
                  ((ICopyPaste)newControl).BindToHost(mActiveNodeHostControl, this);


                  ((ICopyPaste)newControl).PasteContents(sTriggerClipboard, sTriggerClipboard.mData[i], triggerMapping, false);
                  
                  mActiveNodeHostControl.InsertControl(newControl, 400, 400);

                  newControl.Left = x + (sTriggerClipboard.mPositions[i].X - sTriggerClipboard.mTopLeft.X);
                  newControl.Top = y + (sTriggerClipboard.mPositions[i].Y - sTriggerClipboard.mTopLeft.Y);

                  newControls.Add(newControl);
               }              
            }

            //void post load fixup
            foreach(Control newControl in newControls)
            {
               ((ICopyPaste)newControl).PostCopy(triggerMapping);

            }

            sTriggerClipboard.ClearTempData();
         }     
      }

      void addToGroup_Click(object sender, EventArgs e)
      {
         ContextMenu m = new ContextMenu();

         foreach (GroupControl groupui in mGroupControls.Values)
         {
            MenuItem addToGroupItem = new MenuItem();
            addToGroupItem.Text = groupui.GroupUI.Title;
            addToGroupItem.Tag = groupui.GroupUI.InternalGroupID;
            addToGroupItem.Click +=new EventHandler(addToGroupItem_Click);
            m.MenuItems.Add(addToGroupItem);
         }

         m.Show(this,this.PointToClient( Cursor.Position));
      }

      void addToGroupItem_Click(object sender, EventArgs e)
      {
         NodeHostControl outerHostConrol = GetMenuOwner();

         if (outerHostConrol == null)
            return;

         int innerID = (int)(((MenuItem)sender).Tag);
         int outerID = mGroupControls[innerID].GetGroupID();

         NodeHostControl innerHostConrol = mHostByGroupID[innerID];
         List<Control> selected = outerHostConrol.GetSelected();
         ChangeGroup(selected, outerHostConrol, innerHostConrol);      
      }

      void removeFromGroup_Click(object sender, EventArgs e)
      {
         NodeHostControl innerHostConrol = GetMenuOwner();

         if (innerHostConrol == null || innerHostConrol.mGroupID == -1)
            return;

         int innerID = innerHostConrol.mGroupID;
         int outerID = mGroupControls[innerID].GetGroupID();

         NodeHostControl outerHostConrol = mHostByGroupID[outerID];
         List<Control> selected = innerHostConrol.GetSelected();
         ChangeGroup(selected, innerHostConrol, outerHostConrol);
      }

      void createGroupWithSelected_Click(object sender, EventArgs e)
      {
         NodeHostControl oldhostConrol = GetMenuOwner();
         if (oldhostConrol == null )
            return;
         List<Control> selected = oldhostConrol.GetSelected();

         if (selected.Count == 0)
         {
            MessageBox.Show("Please select one or more triggers first.");
            return;
         }

         NodeHostControl newGroup = CreateNewGroup();

         int minX = int.MaxValue;
         int minY = int.MaxValue;
         foreach (Control c in selected)
         {
            minX = Math.Min(minX, c.Left);
            minY = Math.Min(minY, c.Top);

            oldhostConrol.DetachControl(c);
         }
         foreach (Control c in selected)
         {
            newGroup.InsertControl(c, 300 + c.Left - minX, 300 + c.Top - minY);
            //newGroup.InsertControl(c, c.Left ,  c.Top );
            ClientNodeControl cl = c as ClientNodeControl;
            if (cl != null)
            {
               cl.mHost = newGroup;
               cl.SetGroupID(newGroup.mGroupID);
            }
         }


         GroupControl groupControl = new GroupControl(this);
         groupControl.SetGroupID(oldhostConrol.mGroupID);
         mGroupControls[newGroup.mGroupID] = groupControl;

         groupControl.Setup(oldhostConrol, newGroup, this.mCurrentTriggerNamespace);
         oldhostConrol.InsertControl(groupControl, minX, minY);

         //groupControl.GroupUI = groupControl.GroupUI;
         this.mCurrentTriggerNamespace.TriggerEditorData.UIData.mGroups.Add(groupControl.GroupUI);



         GroupUI groupui = groupControl.GroupUI;
         NodeHostControl innernodehost = newGroup;
         NodeHostControl outernodehost = oldhostConrol;
         int ox = 800;
         int oy = 200;
         int ix = 0;
         int iy = 200;

         CreateInternalGroupControls(groupui, innernodehost, outernodehost, ox, oy, ix, iy);

         SetTabText(groupui.InternalGroupID, groupui.Title);

      }

      public bool IsChild(int child, int parent)
      {
         int id = child;
         int lastid = id;
         if ((parent == -1) && (child != -1))
            return true;

         while (id != -1)
         {
            if (mGroupControls.ContainsKey(id) == false)
            {
               return false;
            }
            id = mGroupControls[id].GetGroupID();
            if (id == parent)
               return true;
            if (lastid == id)
               return false;
            lastid = id;
         }
         return false;
      }

      public int GetGroupParent(int child)
      {
         return mGroupControls[child].GetGroupID();     
      }

      public List<KeyValuePair<IControlPoint, IControlPoint>> GetLinks()
      {
         List<KeyValuePair<IControlPoint, IControlPoint>> allLinks = new List<KeyValuePair<IControlPoint, IControlPoint>>();

         foreach (NodeHostControl c in mHostByGroupID.Values)
         {

            foreach (Control o in c.Controls)
            {
               IControlPointOwner own = o as IControlPointOwner;
               if (own == null)
                  continue;
               foreach (IControlPoint p in own.GetControlPoints())
               {
                  if(p.Virtual)
                     continue;
                  foreach (IControlPoint t in p.GetTargets())
                  {
                     if (t.Virtual)
                        continue;
                     allLinks.Add(new KeyValuePair<IControlPoint, IControlPoint>(p, t));
                  }
               }
            }
         }
         return allLinks;
      }

      public List<IControlPoint> GetVirtualControlPoints(int groupLevel)
      {
         List<IControlPoint> virtualCP = new List<IControlPoint>();

         NodeHostControl c = mHostByGroupID[groupLevel];
                    
         foreach (Control ctrl in c.Controls)
         {
            IControlPointOwner own = ctrl as IControlPointOwner;
            if (own == null)
               continue;
            foreach (IControlPoint p in own.GetControlPoints())
            {
               if (p.Virtual)
                  virtualCP.Add(p);
            }
         }

         return virtualCP;
      }

      Dictionary<int, GroupControl> mGroupControls = new Dictionary<int, GroupControl>();
      bool mbGroupsDirty = false;

      public void SetGroupsDirty()
      {
         mbGroupsDirty = true;
      }

      public void RecalculateGroups()
      {
         foreach (GroupControl g in mGroupControls.Values)
         {
            g.ClearData();
         }
         foreach (InnerGroupControl g in mGroupInnerOutputs.Values)
         {
            g.ClearData();
         }
         foreach (InnerGroupControl g in mGroupInnerInputs.Values)
         {
            g.ClearData();
         }


         foreach (GroupControl g in mGroupControls.Values)
         {
            g.RecalculateGroup(false);
         }
         foreach (InnerGroupControl g in mGroupInnerOutputs.Values)
         {
            g.RecalculateGroup(false);
         }
         foreach (InnerGroupControl g in mGroupInnerInputs.Values)
         {
            g.RecalculateGroup(false);
         }
         ActiveNodeHostControl.Refresh();

         mbGroupsDirty = false;
      }

      public void ChangeGroup(List<Control> controls, NodeHostControl innerHostConrol, NodeHostControl outerHostConrol)
      {

         int minX = int.MaxValue;
         int minY = int.MaxValue;
         foreach (Control c in controls)
         {
            minX = Math.Min(minX, c.Left);
            minY = Math.Min(minY, c.Top);

            innerHostConrol.DetachControl(c);
         }
         foreach (Control c in controls)
         {
            outerHostConrol.InsertControl(c, outerHostConrol.Controls.Count*5 + 300 + c.Left - minX, 300 + c.Top - minY);
            ClientNodeControl cl = c as ClientNodeControl;
            if (cl != null)
            {
               cl.mHost = outerHostConrol;
               cl.SetGroupID(outerHostConrol.mGroupID);
            }
         }
         GroupControl innerGroupControl = null;
         GroupControl outerGroupControl = null;

         mGroupControls.TryGetValue(outerHostConrol.mGroupID, out outerGroupControl);
         mGroupControls.TryGetValue(innerHostConrol.mGroupID, out innerGroupControl);

         //if (innerGroupControl != null)
         //   innerGroupControl.RecalculateGroup(true);
         //if(outerGroupControl != null)
         //   outerGroupControl.RecalculateGroup(true);


         RecalculateGroups();
      }

      private void CreateInternalGroupControls(GroupUI groupui, NodeHostControl innernodehost, NodeHostControl outernodehost, int ox, int oy, int ix, int iy)
      {
         InnerGroupControl innerOutputs = new InnerGroupControl(this);
         innerOutputs.InnerGroupType = InnerGroupControl.eInnerGroupType.Output;
         innerOutputs.SetGroupID(innernodehost.mGroupID);
         innerOutputs.GroupUI = groupui;
         innerOutputs.Setup(outernodehost, innernodehost, this.mCurrentTriggerNamespace);
         innernodehost.InsertControl(innerOutputs, ox, oy);
         mGroupInnerOutputs[innernodehost.mGroupID] = innerOutputs;

         InnerGroupControl innerInputs = new InnerGroupControl(this);
         innerInputs.InnerGroupType = InnerGroupControl.eInnerGroupType.Input;
         innerInputs.SetGroupID(innernodehost.mGroupID);
         innerInputs.GroupUI = groupui;
         innerInputs.Setup(outernodehost, innernodehost, this.mCurrentTriggerNamespace);
         innernodehost.InsertControl(innerInputs, ix, iy);
         mGroupInnerInputs[innernodehost.mGroupID] = innerInputs;
      }




      void deleteSelected_Click(object sender, EventArgs e)
      {
         NodeHostControl hostConrol = GetMenuOwner();
         if (hostConrol == null)
            return;

         hostConrol.DeleteSelected(false);
      }

      void defaultValues_Click(object sender, EventArgs e)
      {
         Form f = new Form();
         TriggerValueList valueList = new TriggerValueList();
         int width = valueList.Width;
         f.Controls.Add(valueList);
         valueList.Dock = DockStyle.Fill;
         f.Width = width + 10;
         f.Height = 600;
         f.TopMost = true;
         f.Show();
         valueList.ParentTriggerNamespace = CurrentTriggerNamespace;
      }

      void newTemplateItem_Click(object sender, EventArgs e)
      {
         NodeHostControl hostConrol = GetMenuOwner();
         if (hostConrol == null)
            return;

         ToolStripMenuItem item = sender as ToolStripMenuItem;
         string name = item.Tag as string;
         if (name != null)
         {
            TriggerTemplateMapping m = TriggerSystemMain.mTriggerDefinitions.GetNewMapping(name);

            TemplateControl t = new TemplateControl(hostConrol, this);

            int newID;
            this.mCurrentTriggerNamespace.InsertTemplateMapping(m, out newID);
            t.ParentTriggerNamespace = this.CurrentTriggerNamespace;
            t.TriggerTemplateMapping = m;

            hostConrol.InsertControl(t, hostConrol.mLastPosition.X, hostConrol.mLastPosition.Y);
            t.OnMoved();
         }
      }

      public void Scan(bool onlyShowErrors)
      {
         TriggerNamespace ns = new TriggerNamespace();
         //ns.TriggerData 
         TriggerRoot root = this.CurrentTriggerNamespace.TriggerData;//def.TriggerSystemRoot;
         //ns.TriggerEditorData.TriggerSystem = this.mCurrentTriggerNamespace.TriggerData

         TreeNode thisFile = new TreeNode();
         
         ns.mDebugInfo.Clear();
         //ns.mDebugInfo.AddRange(this.CurrentTriggerNamespace.mDebugInfo);
         //root.TriggerEditorData = root
         ns.PreProcessObjectGraph(root, true);
         //ns.PreProcessObjectGraph(ns.TriggerData);
         ErrorCount = 0;
         WarningCount = 0;

         mLastDebugInfo = ns.mDebugInfo;
         if (ns.mDebugInfo.Count > 0)
         {
            foreach (TriggerSystemDebugInfo d in ns.mDebugInfo)
            {

               TreeNode info = new TreeNode(d.ToString());
               info.Tag = d;
               if(d.mLevel == TriggerSystemDebugLevel.Error)
               {
                  info.ImageIndex = 0;
                  info.SelectedImageIndex = 0;
                  ErrorCount++;
               }
               else if (d.mLevel == TriggerSystemDebugLevel.Warning)
               {
                  info.ImageIndex = 1;
                  info.SelectedImageIndex = 1;
                  WarningCount++;
               }
               thisFile.Nodes.Add(info);
            }
            
         }



         if (ErrorCount == 0 && onlyShowErrors == true)
            return;

         ImageList l = SharedResources.GetImageList(new string[] { "Error.bmp", "Warning.bmp" });

         TreeView v = new TreeView();
         v.ImageList = l;
         thisFile.Text = "Results:";
         thisFile.ImageIndex = 1;
         thisFile.SelectedImageIndex = 1;
         //v.TopNode = thisFile;
         v.Nodes.Add(thisFile);
         v.Size = new Size(500, 800);
         v.NodeMouseDoubleClick += new TreeNodeMouseClickEventHandler(v_NodeMouseDoubleClick);
         thisFile.Expand();
         PopupEditor p = new PopupEditor();
         p.ShowPopup(this, v);


         if (ScanComplete != null)
            ScanComplete.Invoke(this, null);
      }

      //int lastindex = 0;
      void v_NodeMouseDoubleClick(object sender, TreeNodeMouseClickEventArgs e)
      {
         TriggerSystemDebugInfo info = e.Node.Tag as TriggerSystemDebugInfo;
         if(info != null)
         {
            object subject = info.mSubject;

            SnapViewToItem(subject);

            if (info.mType == TriggerSystemDebugType.ValueError)
            {
               UIUpdate(subject, new BasicArgument(BasicArgument.eArgumentType.HighlightError), eUpdateVisibilty.AnyVisiblity);
            }
         }
      }

      public void SnapViewToItem(object subject)
      {
         List<Control> controls = new List<Control>();
         UIUpdate(subject, new BasicArgument(BasicArgument.eArgumentType.Search), eUpdateVisibilty.AnyVisiblity, ref controls);
         if (controls.Count > 0)
         {
            Control node = controls[0];
            while (!(node is ClientNodeControl) && node != null)
            {
               node = node.Parent;
            }
            ClientNodeControl clientNode = node as ClientNodeControl;
            if (clientNode != null)
            {

               int groupid = clientNode.GetGroupID();
               ActiveNodeHostControl = GetHostByGroupID(groupid);
               ShowGroupTab(groupid);

               foreach (NodeHostControl n in this.mHostByGroupID.Values)
               {
                  if (n.Contains(clientNode))
                  {
                     Point p = new Point(-500, -300);
                     p.Offset(clientNode.Location);
                     p.Offset((int)Math.Abs(n.AutoScrollPosition.X), (int)Math.Abs(n.AutoScrollPosition.Y));
                     n.AutoScrollPosition = p;
                  }
               }
            }
         }
      }

 




      public event EventHandler ScanComplete = null;
      List<TriggerSystemDebugInfo> mLastDebugInfo = null;
      public int ErrorCount = 0;
      public int WarningCount = 0;



      void scanTriggerScript_Click(object sender, EventArgs e)
      {
         Scan(false);
      }




      #region ITriggerUIUpdate Members
      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity)
      {
         List<Control> notused = new List<Control>();
         UIUpdate(data, arguments, visiblity, ref notused);
      }
      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity, ref List<Control> owners)
      {
         //Trigger trigger = data as Trigger;
         //if(trigger 

         if (visiblity == eUpdateVisibilty.AnyVisiblity)
         {
            foreach(NodeHostControl n in this.mHostByGroupID.Values)
            {
               foreach (Control c in n.Controls)
               {
                  ITriggerUIUpdate ui = c as ITriggerUIUpdate;
                  if (ui != null)
                  {
                     ui.UIUpdate(data, arguments, visiblity, ref owners);
                  }
               }
            }
         }
         else
         {
            foreach (Control c in mActiveNodeHostControl.Controls)
            {
               ITriggerUIUpdate ui = c as ITriggerUIUpdate;
               if (ui != null)
               {
                  ui.UIUpdate(data, arguments, visiblity, ref owners);
               }
            }
         }

      
      }

      #endregion

      //Need walk functions

      //Walk popups?

      ////global query for datamodel
      //public bool IsDataSelected(object obj)
      //{

      //}


      //UI statics
   }
   public enum eUpdateVisibilty
   {
      VisibleOnly,
      NotVisible,
      AnyVisiblity,
      Selected
   }


//   ////Update

////Target(complex)   .....// Visiblity(vis/notis/all)   //Arguments

// ...data leaf target?...      

   //ui singletons?

   public class BasicArgument
   {
      public enum eArgumentType
      {
         Select,
         Deselect,
         Refresh,
         Rebind,
         HighlightError,
         Search
      }
      public eArgumentType mArgument;
      public BasicArgument(eArgumentType arg)
      {
         mArgument = arg;
      }
   }
   interface ITriggerUIUpdate
   {
      void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity, ref List<Control> owners);
      void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity);

   }

   interface ITriggerUIUpdateRoot
   {
      void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity, ref List<Control> owners);
      void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity);

   }
   public interface ICopyPaste
   {
      object MakeCopy();
      void PasteContents(TriggerClipboard clipboard, object input, Dictionary<int, int> triggerMapping, bool bShallow);
      void BindToHost(NodeHostControl nodeHost, TriggerHostArea Host);
      void PostCopy(Dictionary<int, int>  triggerMapping);


   }
   //interface ITriggerComponent
   //{
   //   void BindToHost(NodeHostControl nodeHost, TriggerHostArea Host);
   //}

   class BetterTabPage : TabPage
   {
      public TriggerHostArea owhShap = null;
      protected override void SetVisibleCore(bool value)
      {
         //if (value == false)
         //   return; 
         try
         {

            base.SetVisibleCore(value);
         }
         catch
         {
            owhShap.ReloadUI();

         }
      }
   }
   //class BetterTabControl : TabControl
   //{
   //   protected override void OnSelectedIndexChanged(EventArgs e)
   //   {
   //      int selectedIndex = this.SelectedIndex;
   //      this.cachedDisplayRect = Rectangle.Empty;
   //      this.UpdateTabSelection2(this.tabControlState[0x20]);
   //      this.tabControlState[0x20] = false;
   //      if (this.onSelectedIndexChanged != null)
   //      {
   //         this.onSelectedIndexChanged(this, e);
   //      }


   //   }

   //   protected void UpdateTabSelection2(bool updateFocus)
   //   {
   //      if (base.IsHandleCreated)
   //      {
   //         int index = this.SelectedIndex;
   //         TabPage[] tabPages = this.GetTabPages();
   //         for (int i = 0; i < tabPages.Length; i++)
   //         {
   //            if (i != this.SelectedIndex)
   //            {
   //               tabPages[i].Visible = false;
   //               //tabPages[i].
   //            }
   //         }
   //         if (index != -1)
   //         {
   //            if (this.currentlyScaling)
   //            {
   //               tabPages[index].SuspendLayout();
   //            }
   //            tabPages[index].Bounds = this.DisplayRectangle;
   //            if (this.currentlyScaling)
   //            {
   //               tabPages[index].ResumeLayout(false);
   //            }
   //            tabPages[index].Visible = true;
   //            if (updateFocus && (!this.Focused || this.tabControlState[0x40]))
   //            {
   //               this.tabControlState[0x20] = false;
   //               bool flag = false;
   //               IntSecurity.ModifyFocus.Assert();
   //               try
   //               {
   //                  flag = tabPages[index].SelectNextControl(null, true, true, false, false);
   //               }
   //               finally
   //               {
   //                  CodeAccessPermission.RevertAssert();
   //               }
   //               if (flag)
   //               {
   //                  if (!base.ContainsFocus)
   //                  {
   //                     IContainerControl containerControlInternal = base.GetContainerControlInternal();
   //                     if (containerControlInternal != null)
   //                     {
   //                        while (containerControlInternal.ActiveControl is ContainerControl)
   //                        {
   //                           containerControlInternal = (IContainerControl)containerControlInternal.ActiveControl;
   //                        }
   //                        if (containerControlInternal.ActiveControl != null)
   //                        {
   //                           containerControlInternal.ActiveControl.FocusInternal();
   //                        }
   //                     }
   //                  }
   //               }
   //               else
   //               {
   //                  IContainerControl control2 = base.GetContainerControlInternal();
   //                  if ((control2 != null) && !base.DesignMode)
   //                  {
   //                     if (control2 is ContainerControl)
   //                     {
   //                        ((ContainerControl)control2).SetActiveControlInternal(this);
   //                     }
   //                     else
   //                     {
   //                        IntSecurity.ModifyFocus.Assert();
   //                        try
   //                        {
   //                           control2.ActiveControl = this;
   //                        }
   //                        finally
   //                        {
   //                           CodeAccessPermission.RevertAssert();
   //                        }
   //                     }
   //                  }
   //               }
   //            }
   //         }

   //      }
   //   }

   //}
}
