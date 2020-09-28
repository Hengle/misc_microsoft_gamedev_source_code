using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls
{
   public partial class ContainerListDragButton : UserControl
   {
      ContainerList mParent = null;
      public ContainerListDragButton(ContainerList parent)
      {
         InitializeComponent();

         mParent = parent;

         this.Margin = new Padding(0);
         this.BackColor = parent.BackColor;

         this.AllowDrop = true;
      }

      ContextMenu mContextMenu = null;
      private void InitMenu()
      {
         mContextMenu = new ContextMenu();

         MenuItem deleteItem = new MenuItem("Delete");
         deleteItem.Click += new EventHandler(deleteItem_Click);
         mContextMenu.MenuItems.Add(deleteItem);
      }

      public ContainerList GetParentList()
      {
         return mParent;
      }


      void deleteItem_Click(object sender, EventArgs e)
      {
         mParent.OnDelete(this);
      }

      Color mLastBackColor = Color.Black;
      private void SuperListDragButton_MouseEnter(object sender, EventArgs e)
      {
         mLastBackColor = BackColor;
         BackColor = Color.Orange;
      }

      private void SuperListDragButton_MouseLeave(object sender, EventArgs e)
      {
         BackColor = mLastBackColor;
      }

      private void SuperListDragButton_MouseUp(object sender, MouseEventArgs e)
      {
         if(e.Button == MouseButtons.Right)
         {
            if (mContextMenu == null)
               InitMenu();

            mContextMenu.Show(this, new Point(e.X, e.Y));

         }
      }

      private void SuperListDragButton_MouseMove(object sender, MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Left)
         {
            DoDragDrop(this, DragDropEffects.All);

        
         }
      }

      //protected 

      private void SuperListDragButton_DragEnter(object sender, DragEventArgs e)
      {
         e.Effect = DragDropEffects.None;
         if(e.Data.GetDataPresent(typeof(ContainerListDragButton)))
         {
            object data = e.Data.GetData(typeof(ContainerListDragButton));
            ContainerListDragButton otherButton = data as ContainerListDragButton;
            if(otherButton != null)
            {              
               e.Effect = mParent.ValidateDragTarget(otherButton, this, e);
            }
            //e.Effect = DragDropEffects.Move;
         }

         ////DataFormat
         //if (e.Data.GetDataPresent(DataFormats.Bitmap))
         //{
         //   //e.Effect = DragDropEffects.Copy;           
         //}
         //else
         //{
         //   Type o = e.Data.GetType();
         //   //e.Effect = DragDropEffects.None;  
         //}
         //if (mbDebugColors)
         //   this.BackColor = Color.GreenYellow;

         //this.BackColor = mDragOverColor;
      }

      //private void SuperListDragButton_DragDrop(object sender, DragEventArgs e)
      //{
      //   object data = e.Data.GetData(typeof(SuperListDragButton));
      //   SuperListDragButton otherButton = data as SuperListDragButton;
      //   if(otherButton != null)
      //   {
      //      FunctorEditor ctrlMove = otherButton.Tag as FunctorEditor;
      //      FunctorEditor ctrlTarget = this.Tag as FunctorEditor;

      //      if(e.Effect == DragDropEffects.Move)
      //      {    
      //         if (ctrlMove.LogicalHost == ctrlTarget.LogicalHost)
      //         {
      //            mParent.OnReOrderRequest(otherButton, this);                  
      //         }
      //         else if (ctrlMove.LogicalHost.GetType() == ctrlTarget.LogicalHost.GetType())
      //         {
      //            SuperList senderParent = otherButton.GetParentList();
      //            senderParent.OnTransferRequest(mParent, otherButton);
      //            //return DragDropEffects.Move;
      //         }
      
      //      }


      //   }
         
      //}


      private void SuperListDragButton_DragDrop(object sender, DragEventArgs e)
      {
         object data = e.Data.GetData(typeof(ContainerListDragButton));
         ContainerListDragButton otherButton = data as ContainerListDragButton;
         if (otherButton != null)
         {
            mParent.HandleDrop(e, otherButton, this);


         }

      }



   }
}
