using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using System.Threading;

using EditorCore;
using SimEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ObjectivesControl : UserControl
   {
      // Ctor
      public ObjectivesControl()
      {
         InitializeComponent();         

         Application.DoEvents();
      }

      bool mInternal = false;

      public void ObjectivesChanged()
      {
         if (CoreGlobals.getEditorMain().mPhoenixScenarioEditor.CheckTopicPermission("Sim") == false)
         {
            return;
         }
      }


      // Initialize the objectives list box
      public void InitializeObjectivesListBox()
      {
         // Pause painting
         listBoxObjectives.BeginUpdate();

         // Clear the list box
         listBoxObjectives.Items.Clear();
         
         // Iterate through objectives and add their names to the list box
         foreach( ObjectiveXML objective in SimGlobals.getSimMain().ObjectivesData.mObjectives )
         {
            listBoxObjectives.Items.Add( objective.mObjectiveName );
         }

         // Update controls
         UpdateControls();

         // Continue painting
         listBoxObjectives.EndUpdate();
      }

      // Get selected objective data
      private ObjectiveXML GetSelectedObjective()
      {
         if( listBoxObjectives.SelectedItem != null )
         {
            return FindObjectiveByName( listBoxObjectives.SelectedItem.ToString() );
         }

         return null;
      }

      // Find objective data by name
      private ObjectiveXML FindObjectiveByName( string name )
      {
         ObjectiveXML retObjective = null;

         foreach( ObjectiveXML objective in SimGlobals.getSimMain().ObjectivesData.mObjectives )
         {
            if( objective.mObjectiveName == name )
            {
               retObjective = objective;
               break;   
            }
         }

         return retObjective;
      }

      // Enable/Disable controls
      private void EnableControls( bool enable )
      {
         txtBoxObjectiveName.Enabled        = enable;
         chkBoxPlayer1.Enabled              = enable;
         chkBoxPlayer2.Enabled              = enable;
         chkBoxPlayer3.Enabled              = enable;
         chkBoxPlayer4.Enabled              = enable;
         chkBoxPlayer5.Enabled              = enable;
         chkBoxPlayer6.Enabled              = enable;
         chkBoxRequiredObjective.Enabled    = enable;
         //txtBoxObjectiveDescription.Enabled = enable;
         mGroupDescription.Enabled          = enable;
         trackerDuration.Enabled            = enable;
         minTrackerIncrement.Enabled        = enable;
         //txtBoxHintDescription.Enabled      = enable;
         // mGroupHint.Enabled                 = enable;
         txtBoxScore.Enabled                = enable;
         CountNeeded.Enabled                = enable;
      }

      // Blank the controls
      private void BlankControls()
      {
         // Set internal change flag
         mInternal = true;

         txtBoxObjectiveName.Text             = "";
         chkBoxPlayer1.Checked                = false;
         chkBoxPlayer2.Checked                = false;
         chkBoxPlayer3.Checked                = false;
         chkBoxPlayer4.Checked                = false;
         chkBoxPlayer5.Checked                = false;
         chkBoxPlayer6.Checked                = false;
         chkBoxRequiredObjective.Checked      = false;

         SetObjectiveDescription("", "");
         // mTxtDescriptionStringID.Text         = "";
         // txtBoxObjectiveDescription.Text      = "";

         SetObjectiveTrackerText("", "");
         trackerDuration.Value = 8000;
         minTrackerIncrement.Value = 1;

         SetObjectiveHint("", "");
         // mTxtHintStringID.Text                = "";
         // txtBoxHintDescription.Text           = "";

         txtBoxScore.Text = "";

         btnApplyObjectiveName.Enabled        = false;
         btnApplyScore.Enabled = false;

         txtBoxObjectiveName.BackColor        = SystemColors.Window;
         //txtBoxObjectiveDescription.BackColor = SystemColors.Window;
         // txtBoxHintDescription.BackColor      = SystemColors.Window;
         txtBoxScore.BackColor = SystemColors.Window;

         CountNeeded.Value = -1;

         // Disable controls
         EnableControls( false );

         // Set internal change flag
         mInternal = false;
      }

      // Update all of the dependent controls
      private void UpdateControls()
      {
         if( listBoxObjectives.SelectedItem == null )
         {
            BlankControls();
            return;
         }

         // Grab local objective data
         ObjectiveXML objective = GetSelectedObjective();

         if( objective == null )
         {
            //Halwes - 12/1/2006 -  This is a problem handle it.
            BlankControls();
            return;
         }

         // Set internal changes flag
         mInternal = true;

         // Enable controls
         EnableControls( true );

         // Update objective name text box         
         txtBoxObjectiveName.Text = listBoxObjectives.SelectedItem.ToString();

         // Disable objective name apply button and score apply button
         btnApplyObjectiveName.Enabled = false;
         btnApplyScore.Enabled = false;

         // Update check boxes
         chkBoxPlayer1.Checked           = objective.hasFlag( "Player1" );
         chkBoxPlayer2.Checked           = objective.hasFlag( "Player2" );
         chkBoxPlayer3.Checked           = objective.hasFlag( "Player3" );
         chkBoxPlayer4.Checked           = objective.hasFlag( "Player4" );
         chkBoxPlayer5.Checked           = objective.hasFlag( "Player5" );
         chkBoxPlayer6.Checked           = objective.hasFlag( "Player6" );         
         chkBoxRequiredObjective.Checked = objective.hasFlag( "Required" );

         // Update objective description text box
         int locID = DecodeLocStringID(objective.mDescription);
         // if (IsStringID(objective.mDescription))
         if ( (locID >= 0) && IsStringID(locID) )
         {
            SetObjectiveDescription(locID.ToString(), GetLocString(locID.ToString()));
         }
         else
         {
            if (objective.mHint.Length > 0)
               mLblStatusDescription.Text = "Please replace this with a Loc String ID !";

            SetObjectiveDescription("", objective.mDescription);
         }

         // Update objective tracker text box
         locID = DecodeLocStringID(objective.mTrackerText);
         if( (locID > 0) && IsStringID(locID) )
         {
            SetObjectiveTrackerText(locID.ToString(), GetLocString(locID.ToString()));
         }
         else
         {
            if (objective.mTrackerText.Length > 0)
               textTrackerText.Text = "Please replace this with a Loc String ID !";

            SetObjectiveTrackerText("", objective.mTrackerText);
         }

         trackerDuration.Value = objective.mTrackerDuration;
         minTrackerIncrement.Value = objective.mMinTrackerIncrement;

         // Update objective hint text box
         locID = DecodeLocStringID(objective.mHint);
         //if (IsStringID(objective.mHint))
         if ( (locID > 0) && IsStringID(locID))
         {
            SetObjectiveHint(locID.ToString(), GetLocString(locID.ToString()));
         }
         else
         {
            if (objective.mHint.Length > 0)
               mLblStatusHint.Text = "Please replace this with a Loc String ID !";

            SetObjectiveHint("", objective.mHint);
         }

         // Update score text box
         txtBoxScore.Text = objective.mScore.ToString();

         CountNeeded.Value = objective.mFinalCount;

         // Set internal changes flag
         mInternal = false;
      }

      // Assign ID based on max ID from scenario
      private void AssignID( ObjectiveXML objective )
      {
         // Bump current ID level
         SimGlobals.getSimMain().ObjectivesData.MaxObjectiveID++;

         // Assign ID
         objective.mID = SimGlobals.getSimMain().ObjectivesData.MaxObjectiveID;
      }

      // Delete an objective from the list box
      private void Delete_Click( object sender, EventArgs e )
      {
         if( listBoxObjectives.SelectedItem == null )
         {
            return;
         }

         // Pause painting
         listBoxObjectives.BeginUpdate();

         // Delete objective from local list         
         SimGlobals.getSimMain().ObjectivesData.mObjectives.Remove( FindObjectiveByName( listBoxObjectives.SelectedItem.ToString() ) );

         // Grab objective from list box
         object deletedObjective = listBoxObjectives.SelectedItem;

         // Select the next objective in the list box         
         if( ( listBoxObjectives.SelectedIndex + 1 ) < listBoxObjectives.Items.Count )
         {
            listBoxObjectives.SelectedIndex += 1;
         }
         else if( listBoxObjectives.Items.Count >= 2 )
         {
            listBoxObjectives.SelectedIndex -= 1;
         }
         
         // Delete the objective from list box
         listBoxObjectives.Items.Remove( deletedObjective );

         // Continue painting
         listBoxObjectives.EndUpdate();


         ObjectivesChanged();
      }

      // Add proper prefix to objective name
      private string FixupNamePrefix( string name, bool required )
      {
         string retName = name;

         // Is this a required objective without the required prefix?
         if(  required && ( retName.Substring( 0, 4 ) != "rojv" ) )
         {
            // Does it already have the objective prefix?
            if( retName.Substring( 0, 3 ) == "ojv" )
            {
               retName = retName.Insert( 0, "r" );
            }
            else
            {
               retName = retName.Insert( 0, "rojv" );
            }

            // Capitalize character after prefix
            string cap = retName[4].ToString();
            retName    = retName.Remove( 4, 1 );
            retName    = retName.Insert( 4, cap.ToUpper() );
         }
         // Is this a non-required objective without the objective prefix?
         else if( !required && retName.Substring( 0, 3 ) != "ojv" )
         {
            // Does it already have the required objective prefix?
            if( retName.Substring( 0, 4 ) == "rojv" )
            {
               retName = retName.Remove( 0, 1 );
            }
            else
            {
               retName = retName.Insert( 0, "ojv" );
            }

            // Capitalize character after prefix
            string cap = retName[3].ToString();
            retName    = retName.Remove( 3, 1 );
            retName    = retName.Insert( 3, cap.ToUpper() );
         }

         return retName;
      }

      // Return appropriate number string
      private string FormatCount( int count )
      {
         if( count < 10 )
         {
            return "0" + count.ToString();
         }

         return count.ToString();
      }

      // Create a new objective in the local data and add it to the list box
      private void btnNew_Click( object sender, EventArgs e )
      {
         ObjectiveXML newObjective = new ObjectiveXML();

         // Set default name
         int    offset = 1;
         string name   = FixupNamePrefix( "Objective" + FormatCount( listBoxObjectives.Items.Count + offset ), true );
         while( FindObjectiveByName( name ) != null )
         {
            offset++;
            name = FixupNamePrefix( "Objective" + FormatCount( listBoxObjectives.Items.Count + offset ), true );
         }
         newObjective.mObjectiveName = name;

         // Set default flags
         newObjective.setFlag( true, "Required" );
         newObjective.setFlag( true, "Player1" );

         // Set default description
         // fixme - encode?
         newObjective.mDescription = "";

         // Set default hint
         newObjective.mHint = "";

         // Set the default score
         newObjective.mScore = 0;

         // Assign objective ID
         AssignID( newObjective );

         // Add to data
         SimGlobals.getSimMain().ObjectivesData.mObjectives.Add( newObjective );

         // Pause painting
         listBoxObjectives.BeginUpdate();

         // Add to list box
         listBoxObjectives.Items.Add( newObjective.mObjectiveName );

         // Assign list box selection to the new objective
         listBoxObjectives.SelectedIndex = listBoxObjectives.Items.IndexOf( newObjective.mObjectiveName );

         // Resume painting
         listBoxObjectives.EndUpdate();


         ObjectivesChanged();
      }

      // Replace objective string in list box
      private void ReplaceListBoxObjective( string name )
      {
         listBoxObjectives.Items.Add( name );
         listBoxObjectives.Items.Remove( listBoxObjectives.SelectedItem );
         listBoxObjectives.SelectedIndex = listBoxObjectives.Items.IndexOf( name );
      }

      // When an item in the list box is selected
      private void listBoxObjectives_SelectedIndexChanged( object sender, EventArgs e )
      {
         // Update controls
         UpdateControls();


         //ObjectivesChanged();
      }
      
      // When a check box is changed
      private void CheckBoxChanged( CheckBox chkBox, string flag )
      {
         ObjectiveXML objective = GetSelectedObjective();

         if( ( objective != null ) && !mInternal )
         {
            objective.setFlag( chkBox.Checked, flag );
         }

         //ObjectivesChanged();
      }

      private void chkBoxPlayer1_CheckedChanged( object sender, EventArgs e )
      {
         CheckBoxChanged( (CheckBox)sender, "Player1" );
      }

      private void chkBoxPlayer2_CheckedChanged( object sender, EventArgs e )
      {
         CheckBoxChanged( (CheckBox)sender, "Player2" );
      }

      private void chkBoxPlayer3_CheckedChanged( object sender, EventArgs e )
      {
         CheckBoxChanged( (CheckBox)sender, "Player3" );
      }

      private void chkBoxPlayer4_CheckedChanged( object sender, EventArgs e )
      {
         CheckBoxChanged( (CheckBox)sender, "Player4" );
      }

      private void chkBoxPlayer5_CheckedChanged( object sender, EventArgs e )
      {
         CheckBoxChanged( (CheckBox)sender, "Player5" );
      }

      private void chkBoxPlayer6_CheckedChanged( object sender, EventArgs e )
      {
         CheckBoxChanged( (CheckBox)sender, "Player6" );
      }

      private void chkBoxRequiredObjective_CheckedChanged( object sender, EventArgs e )
      {
         CheckBoxChanged( (CheckBox)sender, "Required" );

         ObjectiveXML objective = GetSelectedObjective();

         if( ( objective != null ) && !mInternal )
         {
            // Get proper name prefix
            string nameFixup = FixupNamePrefix( objective.mObjectiveName, chkBoxRequiredObjective.Checked );

            // Set local data to new name
            objective.mObjectiveName = nameFixup;

            // Set list box to new name
            ReplaceListBoxObjective( nameFixup );
         }
      }

      private void txtBoxObjectiveName_TextChanged( object sender, EventArgs e )
      {
         ObjectiveXML objective = GetSelectedObjective();

         if( ( objective != null ) && !mInternal && ( objective.mObjectiveName != txtBoxObjectiveName.Text ) && ( txtBoxObjectiveName.Text.Length > 0 ) )
         {
            // Activate objective name apply button
            btnApplyObjectiveName.Enabled = true;

            // Darken background of text box
            txtBoxObjectiveName.BackColor = SystemColors.ActiveBorder;
         }
         else
         {
            btnApplyObjectiveName.Enabled = false;

            // Reset text box background color
            txtBoxObjectiveName.BackColor = SystemColors.Window;
         }

         //ObjectivesChanged();
      }

      private void btnApplyObjectiveName_Click( object sender, EventArgs e )
      {
         ObjectiveXML objective = GetSelectedObjective();

         // Get proper name prefix
         string nameFixup = FixupNamePrefix( txtBoxObjectiveName.Text, objective.hasFlag( "Required" ) );

         // See if name already exists
         if( FindObjectiveByName( nameFixup ) != null )
         {
            MessageBox.Show( "This name already exists in the list.  Please use a different name.", // Message
                             "Duplicate Name",                                                      // Caption
                             MessageBoxButtons.OK,                                                  // Buttons
                             MessageBoxIcon.Exclamation,                                            // Icons
                             MessageBoxDefaultButton.Button1 );                                     // Default button

            return;
         }

         // See if name contains a space character
         if( nameFixup.Contains( " " ) )
         {
            MessageBox.Show( "Please do not use spaces in the name.", // Message
                             "Invalid Characters",                    // Caption
                             MessageBoxButtons.OK,                    // Buttons
                             MessageBoxIcon.Exclamation,              // Icons
                             MessageBoxDefaultButton.Button1 );       // Default button

            return;
         }

         // Set internal change flag
         mInternal = true;

         // Set local data to new name
         objective.mObjectiveName = nameFixup;

         // Set text box to new name
         txtBoxObjectiveName.Text = nameFixup;

         // Set internal change flag
         mInternal = false;

         // Update list box with new name
         ReplaceListBoxObjective( nameFixup );


         ObjectivesChanged();
      }

      private void txtBoxObjectiveName_KeyPress( object sender, KeyPressEventArgs e )
      {
         if ( btnApplyObjectiveName.Enabled && ( e.KeyChar == (char)Keys.Return ) )
         {
            btnApplyObjectiveName_Click( sender, e );
         }

         ObjectivesChanged();
      }

      private void RetrieveObjectiveDescription()
      {
         ObjectiveXML objective = GetSelectedObjective();

         // Set internal change flag
         mInternal = true;

         // Set local data to new name
         if (IsStringID(mTxtDescriptionStringID.Text))
         {
            string encodedLocStringID = EncodeLocStringID(mTxtDescriptionStringID.Text);
            objective.mDescription = encodedLocStringID;
         }
         else
         {
            objective.mDescription = txtBoxObjectiveDescription.Text;
         }

         // Set internal change flag
         mInternal = false;
      }

      /// <summary>
      /// put a $$ on either end of the loc ID
      /// </summary>
      /// <param name="stringID"></param>
      /// <returns></returns>
      private string EncodeLocStringID(string stringID)
      {
         string locStringID;
         locStringID = string.Format("$${0}$$", stringID.Trim());
         return locStringID;
      }

      /// <summary>
      /// put a $$ on either end of the loc ID
      /// </summary>
      /// <param name="stringID"></param>
      /// <returns></returns>
      private string EncodeLocStringID(int stringID)
      {
         string locStringID;
         locStringID = string.Format("$${0}$$", stringID);
         return locStringID;
      }

      /// <summary>
      /// return the number if the loc string is encoded like $$N$$ where N is a number
      /// </summary>
      /// <param name="locStringID"></param>
      /// <returns></returns>
      private int DecodeLocStringID(string locStringID)
      {
         if (!locStringID.StartsWith("$$"))
            return -1;

         // we need at least $$n$$ for the chars
         if (locStringID.Length < 5)
            return -1;

         //
         int index = locStringID.IndexOf("$$", 3);
         if (index < 3)
            return -1;

         index -= 2; // adjust for the 2 $$ chars at the front
         string locString = locStringID.Substring(2, index /* actually count */);

         int locID = -1;
         try 
         {
            locID = int.Parse(locString);
         }
         catch (Exception)
         {
            locID = -1;
         }

         return locID;
      }


      private void RetrieveObjectiveHint()
      {
         ObjectiveXML objective = GetSelectedObjective();

         // Set internal change flag
         mInternal = true;

         // Set local data to new name
         if (IsStringID(mTxtHintStringID.Text))
         {
            string encodedLocStringID = EncodeLocStringID(mTxtHintStringID.Text);
            objective.mHint = encodedLocStringID;
         }
         else
            objective.mHint = txtBoxHintDescription.Text;


         // Set internal change flag
         mInternal = false;
      }

      private void SetObjectiveHint(string locID, string locString)
      {
         mInternal = true;

         mTxtHintStringID.Text = locID;
         txtBoxHintDescription.Text = locString;

         mInternal = false;
      }

      private void SetObjectiveDescription(string locID, string locString)
      {
         mInternal = true;

         mTxtDescriptionStringID.Text = locID;
         txtBoxObjectiveDescription.Text = locString;

         mInternal = false;
      }

      private void mTxtDescriptionStringID_TextChanged(object sender, EventArgs e)
      {
         if (mInternal)
            return;

         string locID = mTxtDescriptionStringID.Text;
         //string text = "{not set}";
         string text = "";
         mLblStatusDescription.Text = "";
         if ((locID != null) && (locID.Length > 0))
         {
            text = GetLocString(locID);
            if (text == null)
            {
               mLblStatusDescription.Text = "String Not Found !";
               text = "";
            }
         }

         txtBoxObjectiveDescription.Text = text;

         RetrieveObjectiveDescription();

         ObjectivesChanged();
      }

      private void mTxtHintStringID_TextChanged(object sender, EventArgs e)
      {
         if (mInternal)
            return;

         string locID = mTxtHintStringID.Text;
         string text = "";
         mLblStatusHint.Text = "";
         if ((locID != null) && (locID.Length > 0))
         {
            text = GetLocString(locID);
            if (text == null)
            {
               mLblStatusHint.Text = "String Not Found !";
               text = "";
            }
         }

         txtBoxHintDescription.Text = text;

         RetrieveObjectiveHint();


         ObjectivesChanged();
      }

      private string GetLocString(string locID)
      {
         string locString = null;
         LocString locStringObject;
         if (CoreGlobals.getGameResources().mStringTable.mStringsByID.TryGetValue(locID, out locStringObject))
            locString = locStringObject.mString;

         return locString;
      }


      private bool IsStringID(string stringID)
      {
         // do a number conversion test
         bool isStringID = true;
         try
         {
            int locID = int.Parse(stringID);
         }
         catch (System.Exception e)
         {
            isStringID = false;
         }

         if (isStringID)
         {
            LocString locString;
            if (!CoreGlobals.getGameResources().mStringTable.mStringsByID.TryGetValue(stringID, out locString))
               isStringID = false;     // this was the final test.
         }

         return isStringID;
      }

      private bool IsStringID(int stringID)
      {
         // do a number conversion test
         LocString locString;
         if (!CoreGlobals.getGameResources().mStringTable.mStringsByID.TryGetValue(stringID.ToString(), out locString))
            return false;     // this was the final test.

         return true;
      }


      private void mBtnGetDescriptionLocString_Click(object sender, EventArgs e)
      {
         PopulateStringIDFromDlg(mTxtDescriptionStringID);
         RetrieveObjectiveDescription();

         ObjectivesChanged();
      }

      private void mBtnGetHintLocString_Click(object sender, EventArgs e)
      {
         PopulateStringIDFromDlg(mTxtHintStringID);
         RetrieveObjectiveHint();

         ObjectivesChanged();
      }


      private void PopulateStringIDFromDlg(System.Windows.Forms.TextBox tb)
      {
         LocStringDlg dlg = new LocStringDlg();
         if (dlg.ShowDialog(this) == DialogResult.Cancel)
            return;

         if (dlg.LocStringID == null)
            return;

         tb.Text = dlg.LocStringID;
      }

      private void txtBoxScore_TextChanged( object sender, EventArgs e )
      {
         ObjectiveXML objective = GetSelectedObjective();
         
         if( ( objective != null ) && !mInternal && ( objective.mScore.ToString() != txtBoxScore.Text ) && ( txtBoxScore.Text.Length > 0 ) )
         {
            // Activate objective score apply button
            btnApplyScore.Enabled = true;

            // Darken background of text box
            txtBoxScore.BackColor = SystemColors.ActiveBorder;
         }
         else
         {
            // Deactivate objective score apply button
            btnApplyScore.Enabled = false;

            // Reset text box background color
            txtBoxScore.BackColor = SystemColors.Window;
         }

         //ObjectivesChanged();
      }

      private void btnApplyScore_Click( object sender, EventArgs e )
      {
         ObjectiveXML objective = GetSelectedObjective();

         // Get string as integer
         uint score = 0;
         bool validScore = uint.TryParse(txtBoxScore.Text, out score);         

         // If not a valid score
         if (!validScore)
         {
            MessageBox.Show( "This is not a valid score.  Please use numbers only.", // Message
                             "Invalid Score",                                        // Caption
                             MessageBoxButtons.OK,                                   // Buttons
                             MessageBoxIcon.Exclamation,                             // Icons
                             MessageBoxDefaultButton.Button1 );                      // Default button

            return;
         }

         // Set local data to new name
         objective.mScore = (uint)score;

         // Deactivate objective score apply button
         btnApplyScore.Enabled = false;

         // Reset text box background color
         txtBoxScore.BackColor = SystemColors.Window;

         ObjectivesChanged();

         // Update controls
         UpdateControls();
      }

      private void txtBoxScore_KeyPress(object sender, KeyPressEventArgs e)
      {
         if (btnApplyScore.Enabled && (e.KeyChar == (char)Keys.Return))
         {
            btnApplyScore_Click( sender, e );
         }

         ObjectivesChanged();
      }

      private void CountNeeded_ValueChanged(object sender, EventArgs e)
      {
         ObjectiveXML objective = GetSelectedObjective();
         if (objective == null)
            return;

         objective.mFinalCount = (int)CountNeeded.Value;

         ObjectivesChanged();
      }

      private void mBtnGetTrackerTextString_Click(object sender, EventArgs e)
      {
         PopulateStringIDFromDlg(mTxtTrackerStringID);
         RetrieveObjectiveTrackerText();

         ObjectivesChanged();
      }

      private void RetrieveObjectiveTrackerText()
      {
         ObjectiveXML objective = GetSelectedObjective();

         // Set internal change flag
         mInternal = true;

         // Set local data to new name
         if (IsStringID(mTxtTrackerStringID.Text))
         {
            string encodedLocStringID = EncodeLocStringID(mTxtTrackerStringID.Text);
            objective.mTrackerText = encodedLocStringID;
         }
         else
         {
            objective.mTrackerText = txtBoxObjectiveDescription.Text;
         }

         // Set internal change flag
         mInternal = false;
      }

      private void SetObjectiveTrackerText(string locID, string locString)
      {
         mInternal = true;

         mTxtTrackerStringID.Text = locID;
         textTrackerText.Text = locString;

         mInternal = false;
      }

      private void mTxtTrackerStringID_TextChanged(object sender, EventArgs e)
      {
         if (mInternal)
            return;

         string locID = mTxtTrackerStringID.Text;
         //string text = "{not set}";
         string text = "";
         mLblStatusDescription.Text = "";
         if ((locID != null) && (locID.Length > 0))
         {
            text = GetLocString(locID);
            if (text == null)
            {
               mLblStatusDescription.Text = "String Not Found !";
               text = "";
            }
         }

         textTrackerText.Text = text;

         RetrieveObjectiveTrackerText();

         ObjectivesChanged();
      }
      
      private void trackerDuration_ValueChanged(object sender, EventArgs e)
      {
         ObjectiveXML objective = GetSelectedObjective();
         if (objective == null)
            return;

         objective.mTrackerDuration = (int)trackerDuration.Value;

         ObjectivesChanged();
      }

      private void minTrackerIncrement_ValueChanged(object sender, EventArgs e)
      {
         ObjectiveXML objective = GetSelectedObjective();
         if (objective == null)
            return;

         objective.mMinTrackerIncrement = (int)minTrackerIncrement.Value;

         ObjectivesChanged();
      }
   }
}
