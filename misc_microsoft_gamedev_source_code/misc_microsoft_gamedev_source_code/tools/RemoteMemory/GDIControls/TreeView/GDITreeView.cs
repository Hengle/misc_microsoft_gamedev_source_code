using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;

using System.Text;
using System.Windows.Forms;

namespace GDIControls
{
   public partial class GDITreeView : UserControl
   {

      bool mAutoUpdate = true;
      int mNodeVerticalSpacing = 0;
      int mNodeHorizontalSpacing = 3;

      Color mSelectedNodeTextColor = Color.Gray;
      Color mNodeTextColor = Color.Black;

      Brush mFontBrush = null;
      Brush mSelectedFontBrush = null;

      float mYTranslateAmount = 0;
      
      GDITreeViewNode mSelectedNode = null;
      //=========================================
      // HeightPerNode
      //=========================================
      private float HeightPerNode
      {
         get { return this.Font.Size + NodeVerticalSpacing;  } 
      }
      //=========================================
      // GDITreeView
      //=========================================
      public GDITreeView()
      {
         InitializeComponent();

         this.SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);

         mFontBrush = new SolidBrush(this.NodeTextColor);
         mSelectedFontBrush = new SolidBrush(this.SelectedNodeTextColor);

         

      }




      #region SCROLL MANAGEMENT
      //=========================================
      // updateScroll
      //=========================================
      protected void updateScroll()
      {
         int numAcc = getNumAccessableNodes();
         float totalHeight = numAcc * HeightPerNode;

         float visibleHeight = totalHeight;


         if (visibleHeight > this.Height)
         {
            vScrollBar1.Visible = true;
            vScrollBar1.Maximum = (int)((visibleHeight - this.Height + HeightPerNode) / HeightPerNode);
            vScrollBar1.Minimum = 0;
         }
         else
         {
            vScrollBar1.Visible = false;
            mYTranslateAmount = 0;
            vScrollBar1.Value = 0;
         }
      }

      private void vScrollBar1_ValueChanged(object sender, EventArgs e)
      {
         mYTranslateAmount = -(vScrollBar1.Value*HeightPerNode);

         updatePositions();
         this.Refresh();
      }
      #endregion

      #region ON EVENTS
      //=========================================
      // OnPaint
      //=========================================
      protected override void OnPaint(PaintEventArgs e)
      {
         Graphics g = e.Graphics;

         //paint our selectedNode Background
         if (mSelectedNode != null)
         {
            PointF selStart = mSelectedNode.Location;
            SizeF selSize = mSelectedNode.Size;

            g.FillRectangle(mSelectedFontBrush, selStart.X, selStart.Y, this.Width, selSize.Height);
         }

         for (int i = 0; i < mNodes.Count; i++)
            mNodes[i].OnPaint(e, this.Font, mFontBrush);

      }

      //=========================================
      // OnMouseClick
      //=========================================
      protected override void OnMouseClick(MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Right)
         {
            mainContextMenu.Show(this.PointToScreen(e.Location));
            return;
         }

         mSelectedNode = getNodeAt(e.X, e.Y);
         if (mSelectedNode != null)
         {
            updatePositions();

            mSelectedNode.OnMouseClick(e);

            
            updateScroll();
         }

         this.Refresh();

         
      }

      //=========================================
      // OnMouseDoubleClick
      //=========================================
      protected override void OnMouseDoubleClick(MouseEventArgs e)
      {
         mSelectedNode = getNodeAt(e.X, e.Y);
         if (mSelectedNode != null)
         {
            mSelectedNode.Expanded = !mSelectedNode.Expanded;

            updatePositions();

            mSelectedNode.OnMouseDoubleClick(e);

            updateScroll();
         }

         this.Refresh();
      }

      

      //=========================================
      // OnMouseEnter
      //=========================================
      protected override void OnMouseEnter(EventArgs e)
      {

      }

      //=========================================
      // OnMouseHover
      //=========================================
      protected override void OnMouseHover(EventArgs e)
      {
         
      }

      //=========================================
      // OnMouseLeave
      //=========================================
      protected override void OnMouseLeave(EventArgs e)
      {
      }

      //=========================================
      // OnMouseMove
      //=========================================
      protected override void OnMouseMove(MouseEventArgs e)
      {
      }

    

      //=========================================
      // OnMouseWheel
      //=========================================
      protected override void OnMouseWheel(MouseEventArgs e)
      {
      }

      //=========================================
      // OnKeyDown
      //=========================================
      protected override void OnKeyDown(KeyEventArgs e)
      {
      }

      //=========================================
      // OnKeyPress
      //=========================================
      protected override void OnKeyPress(KeyPressEventArgs e)
      {
      }

      //=========================================
      // OnKeyUp
      //=========================================
      protected override void OnKeyUp(KeyEventArgs e)
      {
      }
      #endregion

      #region NODE MANAGEMENT
      //=========================================
      // Nodes
      //=========================================
      List<GDITreeViewNode> mNodes = new List<GDITreeViewNode>();
      public List<GDITreeViewNode> Nodes
      {
         get
         {
            return mNodes;
         }
      }
      //=========================================
      // addNode
      //=========================================
      public GDITreeViewNode addNode(string nodeText)
      {
         GDITreeViewNode newNode = new GDITreeViewNode();
         newNode.Text = nodeText;
         mNodes.Add(newNode);

         if (AutoUpdate)
         {
            updatePositions();
            this.Refresh();
         }

         return newNode;
      }
      //=========================================
      // addNode
      //=========================================
      public void addNode(GDITreeViewNode newNode)
      {
         mNodes.Add(newNode);


         if (AutoUpdate)
         {
            updatePositions();
            this.Refresh();
         }
      }

      //=========================================
      // addNode
      //=========================================
      public void deleteNode(GDITreeViewNode targetNode)
      {
         if (mNodes.Contains(targetNode))
         {
            mNodes.Remove(targetNode);
         }
         else
         {
            for (int i = 0; i < mNodes.Count; i++)
               mNodes[i].deleteNode(targetNode);
         }

         if (AutoUpdate)
         {
            updatePositions();
            this.Refresh();
         }
      }

      //=========================================
      // clearNodes
      //=========================================
      public void clearNodes()
      {
         for (int i = 0; i < mNodes.Count; i++)
            mNodes[i].clearNodes();

         mSelectedNode = null;

         mNodes.Clear();
         if (AutoUpdate)
         {
            updatePositions();
            this.Refresh();
         }
      }

      //=========================================
      // getNodeAt
      //=========================================
      GDITreeViewNode getNodeAt(float screenSpaceX, float screenSpaceY)
      {

         GDITreeViewNode tvn = null;
         for (int i = 0; i < mNodes.Count; i++)
         {
            tvn = mNodes[i].getNodeAt(screenSpaceX, screenSpaceY);
            if (tvn != null)
               return tvn;
         }

         return null;
      }

      //=========================================
      // getNumAccessableNodes
      //=========================================
      public int getNumAccessableNodes()
      {
         int numAcc = mNodes.Count;

         
            for (int i = 0; i < mNodes.Count; i++)
               numAcc += mNodes[i].getNumAccessableNodes();

         return numAcc;
      }

      //=========================================
      // updatePositions
      //=========================================
      public void updatePositions()
      {
         float xOffset = 0;
         float yOffset = mYTranslateAmount;
         for (int i = 0; i < mNodes.Count; i++)
         {
            mNodes[i].onUpdatePosition(xOffset, ref yOffset, NodeHorizontalSpacing, HeightPerNode);
         }
      }

      //=========================================
      // expandAllNodes
      //=========================================
      public void expandAllNodes()
      {
         for (int i = 0; i < mNodes.Count; i++)
            mNodes[i].expandAllNodes();
      }

      //=========================================
      // collapseAllNodes
      //=========================================
      public void collapseAllNodes()
      {
         
         for (int i = 0; i < mNodes.Count; i++)
            mNodes[i].collapseAllNodes();
      }

      //=========================================
      // getSelectedNode
      //=========================================
      public GDITreeViewNode getSelectedNode()
      {
         return mSelectedNode;
      }
      #endregion

      #region UI EVENTS
      private void expandAllToolStripMenuItem_Click(object sender, EventArgs e)
      {
         expandAllNodes();
         updatePositions();
         updateScroll();
         this.Refresh();
      }

      private void collapseAllToolStripMenuItem_Click(object sender, EventArgs e)
      {
         collapseAllNodes();
         updatePositions();
         updateScroll();
         this.Refresh();
      }
      #endregion



   }
}
