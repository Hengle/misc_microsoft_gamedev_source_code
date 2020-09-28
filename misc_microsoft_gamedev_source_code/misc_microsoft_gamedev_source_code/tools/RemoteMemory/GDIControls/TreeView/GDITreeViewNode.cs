using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
namespace GDIControls
{
   public class GDITreeViewNode
   {

      //=========================================
      // Expanded
      //=========================================
      bool mExpanded = false;
      public bool Expanded
      {
         get { return mExpanded; }
         set { mExpanded = value; }
      }

      //=========================================
      // Text
      //=========================================
      string mText = "";
      public string Text
      {
         get { return mText; }
         set { mText = value; }
      }

      //=========================================
      // Location
      //=========================================
      PointF mLocation = new PointF(0,0);
      public PointF Location
      {
         get { return mLocation; }
         set { mLocation = value; }
      }

      //=========================================
      // Size
      //=========================================
      SizeF mSize = new SizeF(0, 0);
      public SizeF Size
      {
         get { return mSize; }
         set { mSize = value; }
      }

      //=========================================
      // SortChildren
      //=========================================
      bool mSortChildren = false;
      public bool SortChildren
      {
         get { return mSortChildren; }
         set { mSortChildren = value; }
      }
     

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

         return newNode;
      }

      //=========================================
      // addNode
      //=========================================
      public void addNode(GDITreeViewNode newNode)
      {
         mNodes.Add(newNode);
      }

      //=========================================
      // deleteNode
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
      }

      //=========================================
      // clearNodes
      //=========================================
      public void clearNodes()
      {
         for (int i = 0; i < mNodes.Count; i++)
            mNodes[i].clearNodes();

         mNodes.Clear();
      }

      //=========================================
      // getNumVisibleNodes
      //=========================================
      public int getNumAccessableNodes()
      {
         if (!Expanded)
            return 0;

         int numAcc = mNodes.Count;

         
            for (int i = 0; i < mNodes.Count; i++)
               numAcc += mNodes[i].getNumAccessableNodes();

         return numAcc;
      }

      //=========================================
      // getNodeAt
      //=========================================
      public GDITreeViewNode getNodeAt(float screenSpaceX, float screenSpaceY)
      {
         bool isOver = screenSpaceX > mLocation.X && screenSpaceX < mLocation.X + Size.Width &&
                        screenSpaceY > mLocation.Y && screenSpaceY < mLocation.Y + Size.Height;

         if (isOver)
            return this;

         if (Expanded)
         {
            GDITreeViewNode tvn = null;
            for (int i = 0; i < mNodes.Count; i++)
            {
               tvn = mNodes[i].getNodeAt(screenSpaceX, screenSpaceY);
               if (tvn != null)
                  return tvn;
            }

         }

         return null;
      }

      //=========================================
      // expandAllNodes
      //=========================================
      public void expandAllNodes()
      {
         this.Expanded = true;

         for (int i = 0; i < mNodes.Count; i++)
            mNodes[i].expandAllNodes();
      }

      //=========================================
      // collapseAllNodes
      //=========================================
      public void collapseAllNodes()
      {
         this.Expanded = false;

         for (int i = 0; i < mNodes.Count; i++)
            mNodes[i].collapseAllNodes();
      }
      #endregion

      #region ON EVENTS
      //=========================================
      // OnPaint
      //=========================================
      public void onUpdatePosition(float xOffset, ref float yOffset, float xOffsetIncrement, float yOffsetIncrement)
      { 
        
         mLocation.X = xOffset;
         mLocation.Y = yOffset;

         mSize.Width = 240; //CLM HOW DO WE GET THIS NUMBER?!?!?
         mSize.Height = yOffsetIncrement;
        

         yOffset += yOffsetIncrement;

         if (SortChildren)
            mNodes.Sort(delegate(GDITreeViewNode p1, GDITreeViewNode p2) { return p1.compareTo(p2); });
         

         ///////////////
         //now draw our children.
         if (Expanded)
         {
            
            for (int i = 0; i < mNodes.Count; i++)
            {
               mNodes[i].onUpdatePosition(xOffset + xOffsetIncrement, ref yOffset, xOffsetIncrement, yOffsetIncrement);
            }
         }
      }
      //=========================================
      // OnPaint
      //=========================================
      public virtual void OnPaint(PaintEventArgs e, Font parentFont, Brush fontBrush)
      {

         Graphics g = e.Graphics;
         ///////////////
         //draw our 'plus/minus' icon first
         Image ico = Expanded ? global::GDIControls.Properties.Resources.MinusButtonIcon :
                                                         global::GDIControls.Properties.Resources.PlusButtonIcon;

         if (mNodes.Count != 0)
         {
            g.DrawImage(ico, this.Location.X, this.Location.Y);
         }

         //add spacing for the text
         int cIconWidth = global::GDIControls.Properties.Resources.PlusButtonIcon.Width;
         int cSpacingToText = cIconWidth - 3;


         ///////////////
         //draw our data string
         g.DrawString(mText, parentFont, fontBrush, this.Location.X + cSpacingToText, this.Location.Y);


         ///////////////
         //now draw our children.
         if (Expanded)
         {
            for (int i = 0; i < mNodes.Count; i++)
            {
               mNodes[i].OnPaint(e, parentFont, fontBrush);
            }
         }
      }

      //=========================================
      // OnMouseClick
      //=========================================
      public virtual void OnMouseClick(MouseEventArgs e)
      {
      }

      //=========================================
      // OnMouseDoubleClick
      //=========================================
      public virtual void OnMouseDoubleClick(MouseEventArgs e)
      {
      }

   

     

      //=========================================
      // OnMouseMove
      //=========================================
      public virtual void OnMouseMove(MouseEventArgs e)
      {
      }

     

      //=========================================
      // OnMouseWheel
      //=========================================
      public virtual void OnMouseWheel(MouseEventArgs e)
      {
      }

      //=========================================
      // OnKeyDown
      //=========================================
      public virtual void OnKeyDown(KeyEventArgs e)
      {
      }

      //=========================================
      // OnKeyPress
      //=========================================
      public virtual void OnKeyPress(KeyPressEventArgs e)
      {
      }

      //=========================================
      // OnKeyUp
      //=========================================
      public virtual void OnKeyUp(KeyEventArgs e)
      {
      }
      #endregion


      #region COMPARISIONS
      //=========================================
      // isLessThan
      //=========================================
      public virtual int compareTo(GDITreeViewNode rhs)
      {
         return Text.CompareTo(rhs.Text);
      }

      //=========================================
      // isLessThan
      //=========================================
      public virtual bool isLessThan(GDITreeViewNode rhs)
      {
         return Text.CompareTo(rhs.Text) < 0;
      }

      //=========================================
      // isGreaterThan
      //=========================================
      public virtual bool isGreaterThan(GDITreeViewNode rhs)
      {
         return Text.CompareTo(rhs.Text) > 0;
      }

      //=========================================
      // isEqualTo
      //=========================================
      public virtual bool isEqualTo(GDITreeViewNode rhs)
      {
         return Text.CompareTo(rhs.Text) == 0;
      }
      #endregion
   }
}
