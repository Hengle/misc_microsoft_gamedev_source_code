using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ComponentPickerHelp : UserControl
   {
      public ComponentPickerHelp()
      {
         InitializeComponent();
      }

      public TriggerComponentDefinition Component
      {
         set
         {
            TriggerComponentDefinition comp = value;

            this.helpLabel.Text = comp.HelpText;
            this.documentationLabel.Text = comp.Documentation;

         }

      }

   }
}
