using System;
using System.Windows.Forms;

namespace EnsembleStudios.RemoteGameDebugger
{
   public class RemoteProfilerUI
   {

   }

//   public class RadioSelection
//   {
//      ArrayList mItems;
//      
//      public RadioSelection(UIRegion baseRegion)
//      {
//
//      }
//
//      void SetSelectedItem(UIRegion selected)
//      {
//
//      }
//
//   }
   
   //bind enum to combo.. write code.
}

namespace EnsembleStudios.UI
{
   public class MenuHelper
   {
      public delegate void eventPointer(object o ,System.EventArgs a);
      static public MenuItem createMenuItem(string name, eventPointer target)
      {
         MenuItem item = new MenuItem();
         item.Text = name;
         if(target != null)
         {
            item.Click += new System.EventHandler(target);
         }
         return item;
      }
   }      

}