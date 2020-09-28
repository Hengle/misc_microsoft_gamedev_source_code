using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace EditorCore
{
   public class CustomPanel : Panel
   {
      protected override bool IsInputKey(Keys key)
      {
         
         switch (key)
         {

            case Keys.Up:
            case Keys.Down:
            case Keys.Right:
            case Keys.Left:
            case Keys.Up | Keys.Shift:
            case Keys.Down | Keys.Shift:
            case Keys.Right | Keys.Shift:
            case Keys.Left | Keys.Shift:
            case Keys.Shift:
               return true;

         }
         return base.IsInputKey(key);
      }

   }
}
