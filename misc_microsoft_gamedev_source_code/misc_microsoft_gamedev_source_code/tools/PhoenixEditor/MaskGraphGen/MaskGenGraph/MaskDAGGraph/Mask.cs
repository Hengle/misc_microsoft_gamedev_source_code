
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;

using EditorCore;

namespace graphapp
{

    public class DAGMask : Sparse2DBlockArray<float>
    {
       public DAGMask(int width, int height)
          :
            base(width,height,0.0f,false)
        {
            
        }

       public DAGMask Clone()
        {
           DAGMask m = new DAGMask(mWidth, mHeight);
            CopyTo(m);
            return m;
        }

       public DAGMask mConstraintMask = null;

       public override float this[int xIndex, int yIndex]
       {
          get
          { 
             return base[xIndex,yIndex];
          }
          set
          {
             if (xIndex < 0 || xIndex >= mWidth || yIndex < 0 || yIndex >= mWidth)
                return;

             float v= value;
             if (mConstraintMask != null)
                v *= mConstraintMask[xIndex, yIndex];

             base[xIndex, yIndex] = v;

             
          }
       }
       


    }
}