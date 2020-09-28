using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;

namespace GDIControls
{
   public partial class GDITreeView
   {

      //========================================
      // NodeTextColor
      //========================================
      public Color NodeTextColor
      {
         get
         {
            return mNodeTextColor;
         }
         set
         {
            mNodeTextColor = value;
            mFontBrush = new SolidBrush(this.NodeTextColor);
            
         }
      }

      //========================================
      // SelectedNodeTextColor
      //========================================
      public Color SelectedNodeTextColor
      {
         get
         {
            return mSelectedNodeTextColor;
         }
         set
         {
            mSelectedNodeTextColor = value;
            mSelectedFontBrush = new SolidBrush(this.SelectedNodeTextColor);
         }
      }


      //========================================
      // NodeHorizontalSpacing
      //========================================
      public int NodeHorizontalSpacing
      {
         get
         {
            return mNodeHorizontalSpacing;
         }
         set
         {
            mNodeHorizontalSpacing = value;
         }
      }

      //========================================
      // NodeVerticalSpacing
      //========================================
      public int NodeVerticalSpacing
      {
         get
         {
            return mNodeVerticalSpacing;
         }
         set
         {
            mNodeVerticalSpacing = value;
         }
      }

      //========================================
      // AutoUpdate
      //========================================
      public bool AutoUpdate
      {
         get
         {
            return mAutoUpdate;
         }
         set
         {
            mAutoUpdate = value;
         }
      }
  
   }

}
