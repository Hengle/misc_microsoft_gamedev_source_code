using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Drawing.Design;
using System.ComponentModel;
using System.Drawing;

namespace AkFilePackager
{
    /// <summary>
    /// List view with reordering capabilities.
    /// </summary>
    [Designer("System.Windows.Forms.Design.ListViewDesigner")]
    public class ReorderingListView : ListView
    {
        /// <summary>
        /// Reorder event custom argument.
        /// </summary>
        public class ReorderEventArgs : DragEventArgs
        {
            public ReorderEventArgs(DragEventArgs in_args, int in_iDropIndex, SelectedIndexCollection in_selIndices)
                : base(in_args.Data, in_args.KeyState, in_args.X, in_args.Y, in_args.AllowedEffect, in_args.Effect)
            {
                m_iDropIndex = in_iDropIndex;
                m_selIndices = in_selIndices;
            }
            public int DropIndex
            {
                get { return m_iDropIndex; }
                set { m_iDropIndex = value; }
            }
            public SelectedIndexCollection SelIndices
            {
                get { return m_selIndices; }
                set { m_selIndices = value; }
            }

            private int m_iDropIndex;
            private SelectedIndexCollection m_selIndices;
        }

        /// <summary>
        /// Reorder event. Dispatched when the user reorders items in the list.
        /// </summary>
        public event EventHandler<ReorderEventArgs> Reorder;

        public ReorderingListView()
        {
            base.FullRowSelect = true;
        }

        protected virtual void OnReorder(ReorderEventArgs e)
        {
            if (Reorder != null)
                Reorder(this, e);
        }

        protected override void OnItemDrag(ItemDragEventArgs e)
        {
            base.OnItemDrag(e);
            if (!AllowRowReorder)
                return;
            base.DoDragDrop(SelectedItems, DragDropEffects.Move);
        }

        protected override void OnDragEnter(DragEventArgs e)
        {
            base.OnDragEnter(e);
            if (!this.AllowRowReorder)
            {
                e.Effect = DragDropEffects.None;
                return;
            }
            if (!e.Data.GetDataPresent(typeof(SelectedListViewItemCollection)))
            {
                e.Effect = DragDropEffects.None;
                return;
            }
            
            e.Effect = DragDropEffects.Move;
        }

        protected override void OnDragOver(DragEventArgs e)
        {
            base.OnDragOver(e);

            if (!this.AllowRowReorder)
            {
                e.Effect = DragDropEffects.None;
                return;
            }

            if (!e.Data.GetDataPresent(typeof(SelectedListViewItemCollection)))
            {
                e.Effect = DragDropEffects.None;
                return;
            }

            e.Effect = DragDropEffects.Move;
            
            Point cp = base.PointToClient(new Point(e.X, e.Y));
            
            // Get new hoveritem.
            ListViewItem newHoverItem = base.GetItemAt(cp.X, cp.Y);

            // Clear previous bounding box.
            if (m_hoverItem != null
                && newHoverItem != m_hoverItem)
            {
                using (Graphics graphics = base.CreateGraphics())
                {
                    graphics.DrawRectangle(Pens.White, m_hoverItem.Bounds);
                }
            }

            m_hoverItem = newHoverItem;

            if (m_hoverItem != null)
            {
                m_hoverItem.EnsureVisible();

                using (Graphics graphics = base.CreateGraphics())
                {
                    graphics.DrawRectangle(Pens.Green, m_hoverItem.Bounds);
                }
            }
        }

        protected override void OnDragDrop(DragEventArgs e)
        {
            base.OnDragDrop(e);

            // Clear bounding box.
            if (m_hoverItem != null)
            {
                using (Graphics graphics = base.CreateGraphics())
                {
                    graphics.DrawRectangle(Pens.White, m_hoverItem.Bounds);
                }
                m_hoverItem = null;
            }

            if (!AllowRowReorder 
                || ( !base.VirtualMode 
                    && base.SelectedItems.Count == 0))
            {
                return;
            }
            Point cp = base.PointToClient(new Point(e.X, e.Y));
            ListViewItem dragToItem = base.GetItemAt(cp.X, cp.Y);
            if (dragToItem != null)
            {
                OnReorder(new ReorderEventArgs(e, dragToItem.Index, base.SelectedIndices));
            }
            else if (this.Bounds.Contains(cp))
            {
                // Dragging passed the end of the list: drop to Items.Count.
                OnReorder(new ReorderEventArgs(e, this.Items.Count, base.SelectedIndices));
            }
        }
        
        public bool AllowRowReorder
        {
            get { return m_bAllowRowReorder; }
            set 
            {
                m_bAllowRowReorder = value;
                base.AllowDrop = value;
            }
        }
        private bool m_bAllowRowReorder = true;
        private ListViewItem m_hoverItem = null;
    }
}
