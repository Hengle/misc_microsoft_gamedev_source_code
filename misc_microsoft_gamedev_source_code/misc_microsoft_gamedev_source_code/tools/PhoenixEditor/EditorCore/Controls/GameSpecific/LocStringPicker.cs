using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls
{
   public partial class LocStringPicker : UserControl, ILocStringPicker
   {
      public LocStringPicker()
      {
         InitializeComponent();

         ScenarioComboBox.Items.Add("Any");
         foreach (string scenario in CoreGlobals.getGameResources().mStringTable.mStringsByScenario.Keys)
         {
            ScenarioComboBox.Items.Add(scenario);
         }
         if (mLastScenario != "")
         {
            ScenarioComboBox.SelectedItem = mLastScenario;
         }

         CategoryComboBox.Items.Add("Any");
         foreach (string category in CoreGlobals.getGameResources().mStringTable.mStringsByCategory.Keys)
         {
            CategoryComboBox.Items.Add(category);
         }
         if (mLastCategory != "")
         {
            CategoryComboBox.SelectedItem = mLastCategory;
         }
      }

      public const int cInvalidID = int.MinValue;

      static private string mLastScenario = "";
      static private string mLastCategory = "";

      public delegate void LocEvent(object sender, LocString locString);
      public event LocEvent IDChanged = null;

      private LocString mLocString = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public LocString LocString
      {
         get
         {
            return mLocString;
         }
         set
         {
            mLocString = value;
            SetUI(mLocString);
         }
      }
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public int LocStringID
      {
         get
         {
            if (mLocString == null)
               return cInvalidID;
            return mLocString.mLocID;
         }
         set
         {
            LocString val;
            if (CoreGlobals.getGameResources().mStringTable.mStringsByID.TryGetValue(value.ToString(), out val))
            {
               mLocString = val;

               SetUI(mLocString);
            }
         }
      }

      private void SetUI(LocString value)
      { 
         StringTextLabel.Text = value.mString;
         IDTextBox.Text = value.mLocID.ToString();
         StringTextLabel.ForeColor = System.Drawing.Color.Black;
      }

      private void IDTextBox_TextChanged(object sender, EventArgs e)
      {
         int id = 0;
         if (int.TryParse(IDTextBox.Text, out id))
         {
            LocString value;
            if (CoreGlobals.getGameResources().mStringTable.mStringsByID.TryGetValue(id.ToString(), out value))
            {
               mLocString = value;
               StringTextLabel.Text = value.mString;
               if (IDChanged != null)
               {
                  IDChanged.Invoke(this, mLocString);
               }
               StringTextLabel.ForeColor = System.Drawing.Color.Black;
               return;
            }
            else
            {
               StringTextLabel.Text = "###ID not found";
               StringTextLabel.ForeColor = System.Drawing.Color.Red;
            }
         }
         else
         {
            StringTextLabel.Text = "###Please type an id number";
            StringTextLabel.ForeColor = System.Drawing.Color.Red;
         }
      }

      private void ScenarioComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {

         Filter();
      }

      private void CategoryComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         Filter();
      }
      public void Filter()
      {
         locStringList.Items.Clear();

         Dictionary<string, LocString> stringsToUse = new Dictionary<string,LocString>();

         bool anyany = false;
         bool checkScenarioStrings = false;
         Dictionary<string, LocString> scenarioStrings = null;
         if (ScenarioComboBox.SelectedItem != null && ScenarioComboBox.SelectedItem.ToString() != "")
         {
            string scenarioName = ScenarioComboBox.SelectedItem.ToString();
            if (CoreGlobals.getGameResources().mStringTable.mStringsByScenario.TryGetValue(scenarioName, out scenarioStrings))
            {
               checkScenarioStrings = true;
               mLastScenario = scenarioName;
            }
            if (scenarioName == "Any")
            {
               mLastScenario = "Any";
               anyany = true;
            }
         }

         bool checkCategoryStrings = false;
         Dictionary<string, LocString> categoryStrings = null;
         if (CategoryComboBox.SelectedItem != null && CategoryComboBox.SelectedItem.ToString() != "")
         {
            string CategoryName = CategoryComboBox.SelectedItem.ToString();
            if (CoreGlobals.getGameResources().mStringTable.mStringsByCategory.TryGetValue(CategoryName, out categoryStrings))
            {
               checkCategoryStrings = true;
               mLastCategory = CategoryName;

               //merge(intersection) results with scenario strings
               if (checkScenarioStrings)
               {
                  foreach (LocString s in categoryStrings.Values)
                  {
                     if (scenarioStrings.ContainsValue(s) == true)
                     {
                        stringsToUse.Add(s.mLocID.ToString(), s);
                     }
                  }
               }
               else
               {
                  stringsToUse = categoryStrings;
               }
            }

            if (CategoryName == "Any")
            {
               mLastCategory = "Any";
            }
            else
            {
               anyany = false;
            }
         }

         if (checkScenarioStrings == true && checkCategoryStrings == false)
         {
            stringsToUse = scenarioStrings;
         }
         if (anyany)
         {
            locStringList.Items.Add("You must set at least one filter (category or settings)");
         }

         foreach (LocString s in stringsToUse.Values)
         {
            locStringList.Items.Add(s);
         }
      }


      private void listBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (locStringList.SelectedItem is LocString)
         {
            this.LocString = (LocString)locStringList.SelectedItem;
         }

      }



      

   }
}
