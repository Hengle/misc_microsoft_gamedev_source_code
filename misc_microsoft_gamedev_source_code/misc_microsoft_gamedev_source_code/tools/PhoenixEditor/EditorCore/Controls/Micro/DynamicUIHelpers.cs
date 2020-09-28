using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace EditorCore
{
   public class DynamicUIHelpers
   {
      static public SplitContainer MakePair(Control A, Control B)
      {
         int height = A.Height;
         SplitContainer c = new SplitContainer();
         c.SplitterDistance = A.Width;
         c.Panel1.Controls.Add(A);
         A.Dock = DockStyle.Fill;
         c.Panel2.Controls.Add(B);
         B.Dock = DockStyle.Fill;
         c.Height = height;
         return c;
      }
      static public SplitContainer MakePair(Control A, Control B, int bias)
      {
         int height = A.Height;
         SplitContainer c = new SplitContainer();
         c.SplitterDistance = A.Width;
         c.Panel1.Controls.Add(A);
         A.Dock = DockStyle.Fill;
         c.Panel2.Controls.Add(B);
         B.Dock = DockStyle.Fill;
         c.Height = height;
         c.SplitterDistance = bias;
         return c;
      }

   }
}
