using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using SimEditor;
using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class TriggerComponentWorksheet : UserControl
   {
      public TriggerComponentWorksheet()
      {
         InitializeComponent();


         //this.HelpTextBox.ReadOnly = true ;
         this.HelpTextBox.TabStop = false;
         this.DocumentationTextBox.TabStop = false;
      }
      List<TriggerParameterControl> mParamcontrols = new List<TriggerParameterControl>();
      TriggerNamespace mParentTriggerNamespace = null;
      TriggerComponent mTriggerComponent = null;
      TriggerComponentDefinition mDefinition = null;



      public bool Setup(TriggerNamespace triggerNamespace , TriggerComponent component)
      {
         mParentTriggerNamespace = triggerNamespace;
         mTriggerComponent = component;

         if (TriggerSystemMain.mTriggerDefinitions.TryGetDefinition(component.DBID, component.Version, out mDefinition) == false)
            return false;

         this.ComponentNameLabel.Text = mDefinition.Type;



         int column1X = 0;
         int column2X = 100;
         int column3X = 200;
         int row1Y = 50;

         int rowN = row1Y;
         int deltaY = 20;

         Dictionary<int, TriggerValue> values = mParentTriggerNamespace.GetValues();        
         foreach (TriggerVariable v in component.Parameter)
         {
            Label ioLabel = new Label();
            if(v is TriggersInputVariable)
            {
               ioLabel.Text = "Input";
            }
            else if(v is TriggersOutputVariable)
            {
               ioLabel.Text = "Output";
            }
            ioLabel.Height -= 5;

            Label nameLabel = new Label();
            nameLabel.Text = v.Name;
            nameLabel.Height -= 5;

            Label typeLabel = new Label();
            typeLabel.Text = values[v.ID].Type;
            typeLabel.Height -= 5;


            TriggerParameterControl c = new TriggerParameterControl();
            c.Init(component, v, values[v.ID], mParentTriggerNamespace);
            c.LabelChanged += new EventHandler(c_LabelChanged);

            AddControl(InputPanel, ioLabel, column1X, rowN);
            AddControl(TypesPanel, typeLabel, column1X, rowN);
            AddControl(NamePanel, nameLabel, column1X, rowN);
            AddControl(ParameterPanel, c, column1X, rowN);


            nameLabel.Width = nameLabel.Parent.Width;
            c.Width = c.Parent.Width;



            mParamcontrols.Add(c);

            rowN += deltaY;
         }

         //smart help text

         string helptext = mDefinition.HelpText;
         //string newHelpText;
         //int startOffset  = helptext.IndexOf("{",startOffset);
         //int lastSpot = 0;
         //while(startOffset != -1) 
         //{
         //   newHelpText += helptext.Substring(lastSpot, startOffset - lastSpot);
         //   newHelpText += 

         //   startOffset = helptext.IndexOf("{", startOffset);
         //}

         UpdateHelpText();

         return true;
      }

      void c_LabelChanged(object sender, EventArgs e)
      {
         UpdateHelpText();
      }

      public void UpdateHelpText()
      {
         string newHelpText;

         List<object> paramsString = new List<object>();
        
         try
         { 
            Dictionary<int,string> paramNames = new Dictionary<int,string>();

            foreach (TriggerParameterControl c in mParamcontrols)
            {
               paramNames[c.GetVariable().SigID] = c.TextValue;
            }
            for (int i = 0; i < TriggerSystemDefinitions.cMaxParameters; i++)
            {
               if (paramNames.ContainsKey(i) == false)
               {
                  paramsString.Add("##ERRORPARAM=" + i.ToString());
               }
               else
               {
                  paramsString.Add(paramNames[i]);
               }
            }
         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());
         }
         try
         {
            newHelpText = string.Format(mDefinition.HelpText, paramsString.ToArray());
            this.HelpTextBox.Text = newHelpText;
         }
         catch(System.Exception ex)
         {
            this.HelpTextBox.Text = "#FORMAT ERROR:" + mDefinition.HelpText;
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());
         }
         try
         {
            newHelpText = string.Format(mDefinition.Documentation, paramsString.ToArray());
            this.DocumentationTextBox.Text = newHelpText;
         }
         catch (System.Exception ex)
         {
            this.DocumentationTextBox.Text = "#FORMAT ERROR:" + mDefinition.Documentation;
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());
         }

        

      }




      protected void AddControl(Panel p, Control c, int x, int y)
      {
         p.Controls.Add(c);
         c.Top = y;
         c.Left = x;

      }


   }
}
