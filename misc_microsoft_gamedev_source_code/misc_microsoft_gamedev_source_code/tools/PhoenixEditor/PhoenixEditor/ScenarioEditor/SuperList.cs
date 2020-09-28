using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class SuperList : UserControl
   {
      protected List<Control> mControls = new List<Control>();
      public SuperList()
      {
         InitializeComponent();

         //tableLayoutPanel1.VerticalScroll.Enabled = true;
         //tableLayoutPanel1.AutoScroll = true;

         tableLayoutPanel1.DragEnter += new DragEventHandler(SuperList_DragEnter);
         tableLayoutPanel1.DragDrop += new DragEventHandler(SuperList_DragDrop);
         tableLayoutPanel1.AllowDrop = true;


         //tableLayoutPanel1.Controls.Add(new Button(), 1,0);
         this.AddItemButton.Paint += new PaintEventHandler(AddItemButton_Paint);
      }


   

      protected void BaseGuiInitializeComponent()
      {
         InitializeComponent();
      }


      public bool mbAutoScroll = false;
      public bool AutoScroll
      {
         set
         {
            mbAutoScroll = value;

            tableLayoutPanel1.AutoScroll = mbAutoScroll;
            tableLayoutPanel1.VerticalScroll.Enabled = mbAutoScroll;
            tableLayoutPanel1.AutoSize = !mbAutoScroll;

         }
         get
         {
            return mbAutoScroll;
         }


      }

      public void BatchSuspend()
      {
         tableLayoutPanel1.SuspendLayout();
         tableLayoutPanel1.Visible = false;
      }
      public void BatchResume()
      {
         tableLayoutPanel1.ResumeLayout();
         tableLayoutPanel1.Visible = true;
      }

      public string RenderText = "";
      Brush textbrush = null;//new SolidBrush(Color.Black);
      void AddItemButton_Paint(object sender, PaintEventArgs e)
      {

         if (textbrush == null)
         {

            Color b = this.BackColor;
            //textbrush = new SolidBrush(Color.FromArgb(255-b.R,255-b.G,255-b.B));
            textbrush = new SolidBrush(Color.White);
         }

         //base.OnPaintBackground(e);

         //int offset = 0;
         if (RenderText == "")
         {
         }
         else if (RenderText.Length == 1)
         {
            e.Graphics.DrawString(RenderText, this.Font, textbrush, 0, 0, null);
         }
         else if (RenderText.Length == 2)
         {
            e.Graphics.DrawString(RenderText, this.Font, textbrush, -1, 0, null);
         }
         else
         {
            e.Graphics.DrawString(RenderText, this.Font, textbrush, -2, 0, null);
         }
         //throw new Exception("The method or operation is not implemented.");
      }

      void SuperList_DragDrop(object sender, DragEventArgs e)
      {

         if (mControls.Count == 0)
         {
          
            if (e.Data.GetDataPresent(typeof(SuperListDragButton)))
            {
               object data = e.Data.GetData(typeof(SuperListDragButton));
               SuperListDragButton otherButton = data as SuperListDragButton;
               if (otherButton != null)
               {

                  SuperList senderParent = otherButton.GetParentList();
                  if (e.Effect == DragDropEffects.Move)
                  {
                     senderParent.OnTransferRequest(this, otherButton);
                  }
                  else if (e.Effect == DragDropEffects.Copy)
                  {
                     senderParent.OnCopyRequest(this, otherButton);

                  }
               }
            }
         }
      }



      void SuperList_DragEnter(object sender, DragEventArgs e)
      {
         if (mControls.Count > 0)
            return;
         e.Effect = DragDropEffects.None;
         if (e.Data.GetDataPresent(typeof(SuperListDragButton)))
         {
            object data = e.Data.GetData(typeof(SuperListDragButton));
            SuperListDragButton otherButton = data as SuperListDragButton;
            if (otherButton != null)
            {
               e.Effect = ValidateDragTarget(otherButton, this, e);
            }
            //e.Effect = DragDropEffects.Move;
         }
      }

      public Color GridColor
      {
         set
         {
            this.AddItemButton.BackColor = value;
         }
         get
         {
            return this.AddItemButton.BackColor;
         }
      }

      private void AddItemButton_Click(object sender, EventArgs e)
      {
         //TestAddRow();
         //if (mContextMenu == null)
         //{
         //   mContextMenu = GetAddMenu();
         //}
         if (mContextMenu != null)
         {
            mContextMenu.Show(AddItemButton, new Point(0, 0));
         }
         else
         {
            HandleAddItemButton(AddItemButton, new Point(0, 0));
         }
      }


      public event EventHandler NeedsResize;

      virtual public void AddRow(Control c)
      {
         tableLayoutPanel1.AutoSize = !mbAutoScroll;
         int oldHeight = tableLayoutPanel1.Height;
         //tableLayoutPanel1.AutoSize = true;
         //tableLayoutPanel1.RowStyles.Insert(mControls.Count, new RowStyle(SizeType.AutoSize));
         tableLayoutPanel1.RowStyles.Insert(mControls.Count, new RowStyle(SizeType.Absolute,c.Height));
         tableLayoutPanel1.Controls.Add(c, 1, mControls.Count);

         SuperListDragButton sb = new SuperListDragButton(this);
         sb.Tag = c;
         sb.Dock = DockStyle.Fill;
         tableLayoutPanel1.Controls.Add(sb, 0, mControls.Count);

         //tableLayoutPanel1.AutoScroll = true;
         //tableLayoutPanel1.AutoScrollMargin = new Size(5, 0);
         //tableLayoutPanel1.AutoScrollMargin.Width = 3;

         int change = tableLayoutPanel1.Height - oldHeight;

         //todo.. notify parent of height change in a proper way
         if (this.Parent != null && this.Parent.Parent != null)
            this.Parent.Parent.Height += change;

         mControls.Add(c);

         if (NeedsResize != null)
            NeedsResize(this, null);
      }


      public void OnReOrderRequest( SuperListDragButton toMove, SuperListDragButton target)
      {

         Control ctrlMove = toMove.Tag as Control;
         Control ctrlTarget = target.Tag as Control;

         ReOrder(tableLayoutPanel1, tableLayoutPanel1.GetRow(toMove), tableLayoutPanel1.GetRow(target));
         //if (DoReorder(ctrlMove, ctrlTarget))
         //{
         //}
         OnReordered();

         if (NeedsResize != null)
            NeedsResize(this, null);
      }

      public void OnTransferRequest(SuperList otherlist, SuperListDragButton toMove)//, SuperListDragButton target)
      {
         Control c = toMove.Tag as Control;
         //otherlist.re
         OnDelete(toMove);
         otherlist.AddRow(c);

         OnReordered();
         otherlist.OnReordered();

         if (NeedsResize != null)
            NeedsResize(this, null);
      }

      public virtual void OnCopyRequest(SuperList otherlist, SuperListDragButton toMove)//, SuperListDragButton target)
      {
         //Control c = toMove.Tag as Control;
         ////otherlist.re
         //OnDelete(toMove);
         //otherlist.AddRow(c);

         //OnReordered();
         //otherlist.OnReordered();

         //if (NeedsResize != null)
         //   NeedsResize(this, null);



         


      }

      //virtual protected bool DoReorder(Control toMove, Control target)
      //{
      //   return true;
      //}
      virtual protected void OnReordered()
      {      
      }




      private void ReOrder(TableLayoutPanel table, int toMoveID, int targetID)
      {
         int rowCount = mControls.Count; // table.RowStyles.Count;
         List<int> rowMapping = new List<int>();
            
         for(int i = 0; i< rowCount; i++)
         {
            rowMapping.Add(i);
         }
         if(toMoveID < targetID)
         {
            rowMapping.Insert(targetID+1, toMoveID);
         }
         else
         {
            rowMapping.Insert(targetID, toMoveID);
         }

         for (int i = 0; i < rowCount; i++)
         {
            if(toMoveID == rowMapping[i] && targetID != i)
            {
               rowMapping.RemoveAt(i);
               break;
            }
         }
         Dictionary<int, int> reorderHash = new Dictionary<int, int>();
         Dictionary<int, Control> controlHash = new Dictionary<int, Control>();
         List<Control> tempList = new List<Control>();
         for (int i = 0; i < rowCount; i++)
         {
            reorderHash[rowMapping[i]] = i;
         }
         for (int i = 0; i < rowCount; i++)
         {
            int spot = reorderHash[i];
            tempList.Add(mControls[spot]);
         }
         //mControls.Clear();
         //for (int i = 0; i < rowCount; i++)
         //{
         //   mControls.Add(tempList[i]);
         //}

         Control[] tempControls = new Control[mControls.Count];

         table.SuspendLayout();
         foreach (Control c in table.Controls)
         {
            int oldrow = table.GetRow(c);
            int newRow = reorderHash[oldrow];
            table.SetRow(c, newRow);

            if (!(c is SuperListDragButton))
               tempControls[newRow] = c;
         }

         mControls.Clear();
         mControls.AddRange(tempControls);

         table.ResumeLayout();
        
      }

      protected TableLayoutPanel GetTableLayout()
      {
         return tableLayoutPanel1;
      }

      virtual public int RecaluculateSize()
      {
         int totalsize = 0;
         int row = 0;
         foreach (Control f in mControls)
         {
            int fingSize = f.Size.Height;// RecalculateSize();
            tableLayoutPanel1.RowStyles[row].Height = fingSize;// f.Height;
            totalsize += fingSize;// f.Height;

            row++;
         }

         this.Height = totalsize + 20;
         return totalsize + 10;
         
      }

      virtual public DragDropEffects ValidateDragTarget(Control toMove, Control target, DragEventArgs e)
      {
         //FunctorEditor ctrlMove = toMove.Tag as FunctorEditor;
         //FunctorEditor ctrlTarget = target.Tag as FunctorEditor;

         //SuperList targetList = target as SuperList;

         ////to empty list
         //if(targetList != null)
         //{
         //   SuperListDragButton button = toMove as SuperListDragButton;
         //   if (targetList.GetType() == button.GetParentList().GetType())
         //   {
         //      return DragDropEffects.Move;
         //   }
         //}
         //if (ctrlMove == null || ctrlTarget == null)
         //{

         //   return DragDropEffects.None;
         //}

         //if ((e.KeyState & 8) == 8 && (e.AllowedEffect & DragDropEffects.Copy) == DragDropEffects.Copy)
         //{

         //   return DragDropEffects.Copy;
         //}
         //else if ((e.AllowedEffect & DragDropEffects.Move) == DragDropEffects.Move)
         //{
         //   if (ctrlMove.LogicalHost == ctrlTarget.LogicalHost)
         //   {
         //      return DragDropEffects.Move;
         //   }
         //   else if (ctrlMove.LogicalHost.GetType() == ctrlTarget.LogicalHost.GetType())
         //   {
         //      return DragDropEffects.Move;
         //   }
         //}

         //return DragDropEffects.None;
         return DragDropEffects.None;

      }


      virtual public void HandleDrop(DragEventArgs e, SuperListDragButton otherButton , SuperListDragButton thisButton)
      {
         //FunctorEditor ctrlMove = otherButton.Tag as FunctorEditor;
         //FunctorEditor ctrlTarget = thisButton.Tag as FunctorEditor;

         //if (e.Effect == DragDropEffects.Move)
         //{
         //   if (ctrlMove.LogicalHost == ctrlTarget.LogicalHost)
         //   {
         //      this.OnReOrderRequest(otherButton, thisButton);
         //   }
         //   else if (ctrlMove.LogicalHost.GetType() == ctrlTarget.LogicalHost.GetType())
         //   {
         //      SuperList senderParent = otherButton.GetParentList();
         //      senderParent.OnTransferRequest(this, otherButton);
         //      //return DragDropEffects.Move;
         //   }

         //}
      }

      protected void ClearControls()
      {
         tableLayoutPanel1.Controls.Clear();
         tableLayoutPanel1.RowStyles.Clear();
         mControls.Clear();
      }

      virtual public void ClearItems()
      {
         List<SuperListDragButton> hitlist = new List<SuperListDragButton>();
         foreach(Control c in tableLayoutPanel1.Controls)
         {
            if(c is SuperListDragButton)
            {
               hitlist.Add((SuperListDragButton)c);
            }
         }
         foreach(SuperListDragButton b in hitlist)
         {
            OnDelete(b);
         }

      }

      //protected virtual OnReorder(SuperListDragButton )
      //??

      //on delete row...
      virtual public void OnDelete(SuperListDragButton button)
      {
         int rowid = tableLayoutPanel1.GetRow(button);

         tableLayoutPanel1.SuspendLayout();

         Control innerControl = button.Tag as Control;
         if(innerControl != null)
         {
            tableLayoutPanel1.Controls.Remove(innerControl);
            mControls.Remove(innerControl);
         }
         tableLayoutPanel1.Controls.Remove(button);

         tableLayoutPanel1.RowStyles.RemoveAt(rowid);
         MoveControlsUpN(tableLayoutPanel1, rowid, 1);

         tableLayoutPanel1.ResumeLayout();

         if (NeedsResize != null)
            NeedsResize(this, null);

      }

      public void InvokeNeedsResize()
      {
         if (NeedsResize != null)
         {
            NeedsResize(this, null);
         }
      }

      public void MoveControlsUpN(TableLayoutPanel table,  int startID, int N)
      {
         int rowCount = table.RowStyles.Count;
         foreach(Control c in table.Controls)
         {
            if(table.GetRow(c) > startID)
            {
             
               table.SetRow(c, table.GetRow(c) - N);
            }
         }
      }



      //on reorder rows...


      ContextMenu mContextMenu = null;
      #region test code
      virtual protected ContextMenu GetAddMenu()
      {
         ContextMenu contextMenu = new ContextMenu();
         MenuItem testAddStuff = new MenuItem("Test add");
         testAddStuff.Click += new EventHandler(testAddStuff_Click);
         //testAddStuff.Tag = s;
         contextMenu.MenuItems.Add(testAddStuff);
         return contextMenu;
      }

      void testAddStuff_Click(object sender, EventArgs e)
      {
         TestAddRow();
      }
      public void TestAddRow()
      {
         TextBox t = new TextBox();
         t.Text = "aslfkdsflj";
         this.AddRow(t);
      }

      #endregion
      
      
      protected void SetSimpleMenu(List<string> options)
      {
         mContextMenu = new ContextMenu();

         foreach (string s in options)
         {
            MenuItem m = new MenuItem(s);
            m.Click += new EventHandler(simpleOption_Click);
            m.Tag = s;
            mContextMenu.MenuItems.Add(m);
         }

      }

      void simpleOption_Click(object sender, EventArgs e)
      {
         MenuItem m = (MenuItem)sender;
         HandleMenuOption(m.Text, m.Tag);
      }




      virtual protected void HandleMenuOption(string text, object tag)
      {

      }

      virtual protected void HandleAddItemButton(Control AddItemButton, Point p)
      {

      }


   }
}
