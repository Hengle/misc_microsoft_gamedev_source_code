using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Data;
using System.Text;
using System.Windows.Forms;

using System.IO;
using System.Xml.Serialization;

using EditorCore;
using PhoenixEditor;

namespace PhoenixEditor.ClientTabPages
{
   public partial class ControllerConfigurationsPage : EditorCore.BaseClientPage
   {
      private ControllerConfigsXML mConfigsData    = new ControllerConfigsXML();
      private ControllerConfigsXML mConfigsDataOld = new ControllerConfigsXML();
      private bool                 mInternal       = false;
      private int                  mLastIndex      = -1;
      private Collection<ComboBox> mDCBoxes        = new Collection<ComboBox>();
      
      // Ctor
      public ControllerConfigurationsPage()
      {
         InitializeComponent();

         // Load config data from XML         
         LoadXML();

         mConfigsData.Copy( mConfigsDataOld );

         // Initialize controls
         InitializeControls();
      }

      // Load data from XML file
      private void LoadXML()
      {
         try
         {
            XmlSerializer s        = new XmlSerializer( typeof( ControllerConfigsXML ), new Type[] { } );
            string        fileName = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\controllerconfigs.xml";
            Stream        st       = File.OpenRead( fileName );
            mConfigsDataOld        = (ControllerConfigsXML)s.Deserialize( st );
            st.Close();
         }
         catch ( System.Exception ex )
         {
            CoreGlobals.getErrorManager().OnException( ex );
         }
      }

      // Save data to XML file
      private void SaveXML()
      {
         mInternal = true;
         int index = lstBoxConfigs.SelectedIndex;
         int maxID = -1;
         lstBoxConfigs.Items.Clear();
         foreach( ConfigXML config in mConfigsData.mConfigs )
         {
            lstBoxConfigs.Items.Add( config.mConfigName );

            if( config.mID > maxID )
            {
               maxID = config.mID;
            }
         }
         lstBoxConfigs.SelectedIndex = index;
         mConfigsData.MaxConfigID    = maxID;
         mInternal                   = false;

         try
         {
            XmlSerializer s = new XmlSerializer( typeof( ControllerConfigsXML ), new Type[] { } );
            string fileName = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\controllerconfigs.xml";
            Stream st = File.Open( fileName, FileMode.Create );

            s.Serialize( st, mConfigsData );
            st.Close();

            XMBProcessor.CreateXMB(fileName, false);
         }
         catch ( System.Exception ex )
         {
            CoreGlobals.getErrorManager().OnException( ex );
         }

         // Reset stored data
         mConfigsDataOld.Copy( mConfigsData );
      }

      // Initialize controls
      public void InitializeControls()
      {
         mInternal = true;

         // Pause painting
         lstBoxConfigs.BeginUpdate();

         // Clear the list box
         lstBoxConfigs.Items.Clear();

         // Iterate through configs and add their names to the list box
         foreach( ConfigXML config in mConfigsData.mConfigs )
         {
            lstBoxConfigs.Items.Add( config.ToString() );
         }

         if( lstBoxConfigs.Items.Count > 0 )
         {
            lstBoxConfigs.SelectedIndex = 0;
            mLastIndex                  = 0;
         }

         // Don't draw position labels
         foreach( Control control in this.Controls )
         {
            if( control.GetType().Name == "Label" )
            {
               Label lbl = (Label)control;
               if( lbl.Text == "|" )
               {
                  lbl.Visible = false;
               }
            }
         }

         // Set standard
         rdbStandard.Checked = true;

         // Switch visible set
         SwitchVisibleSet();

         // Initialize boxes
         InitializeBoxes();

         // Update controls
         UpdateControls();

         // Continue painting
         lstBoxConfigs.EndUpdate();

         mInternal = false;
      }

      // Initialize the lists for the combo boxes
      private void InitializeBoxes()
      {
         foreach( Control control in this.Controls )
         {
            if( control.GetType().Name == "ComboBox" )
            {
               ComboBox cmbBox = (ComboBox)control;
               cmbBox.Items.Add( "<none>" );
               cmbBox.Sorted        = true;
               cmbBox.SelectedIndex = 0;
            }
         }

         // DC Boxes
         // Left Bumper
         mDCBoxes.Add( cmbBoxLeftBumper );
         mDCBoxes.Add( cmbBoxLeftBumperAlt );
         mDCBoxes.Add( cmbBoxLeftBumperDC );
         mDCBoxes.Add( cmbBoxLeftBumperDCAlt );
         mDCBoxes.Add( cmbBoxLeftBumperHold );
         mDCBoxes.Add( cmbBoxLeftBumperHoldAlt );

         // Right Bumper
         mDCBoxes.Add( cmbBoxRightBumper );
         mDCBoxes.Add( cmbBoxRightBumperAlt );
         mDCBoxes.Add( cmbBoxRightBumperDC );
         mDCBoxes.Add( cmbBoxRightBumperDCAlt );
         mDCBoxes.Add( cmbBoxRightBumperHold );
         mDCBoxes.Add( cmbBoxRightBumperHoldAlt );

         // DPad Up
         mDCBoxes.Add( cmbBoxDPadUp );
         mDCBoxes.Add( cmbBoxDPadUpAlt );
         mDCBoxes.Add( cmbBoxDPadUpDC );
         mDCBoxes.Add( cmbBoxDPadUpDCAlt );
         mDCBoxes.Add( cmbBoxDPadUpHold );
         mDCBoxes.Add( cmbBoxDPadUpHoldAlt );

         // DPad Right
         mDCBoxes.Add( cmbBoxDPadRight );
         mDCBoxes.Add( cmbBoxDPadRightAlt );
         mDCBoxes.Add( cmbBoxDPadRightDC );
         mDCBoxes.Add( cmbBoxDPadRightDCAlt );
         mDCBoxes.Add( cmbBoxDPadRightHold );
         mDCBoxes.Add( cmbBoxDPadRightHoldAlt );

         // DPad Down
         mDCBoxes.Add( cmbBoxDPadDown );
         mDCBoxes.Add( cmbBoxDPadDownAlt );
         mDCBoxes.Add( cmbBoxDPadDownDC );
         mDCBoxes.Add( cmbBoxDPadDownDCAlt );
         mDCBoxes.Add( cmbBoxDPadDownHold );
         mDCBoxes.Add( cmbBoxDPadDownHoldAlt );

         // DPad Left
         mDCBoxes.Add( cmbBoxDPadLeft );
         mDCBoxes.Add( cmbBoxDPadLeftAlt );
         mDCBoxes.Add( cmbBoxDPadLeftDC );
         mDCBoxes.Add( cmbBoxDPadLeftDCAlt );
         mDCBoxes.Add( cmbBoxDPadLeftHold );
         mDCBoxes.Add( cmbBoxDPadLeftHoldAlt );

         // Y
         mDCBoxes.Add( cmbBoxY );
         mDCBoxes.Add( cmbBoxYAlt );
         mDCBoxes.Add( cmbBoxYDC );
         mDCBoxes.Add( cmbBoxYDCAlt );
         mDCBoxes.Add( cmbBoxYHold );
         mDCBoxes.Add( cmbBoxYHoldAlt );

         // B
         mDCBoxes.Add( cmbBoxB );
         mDCBoxes.Add( cmbBoxBAlt );
         mDCBoxes.Add( cmbBoxBDC );
         mDCBoxes.Add( cmbBoxBDCAlt );
         mDCBoxes.Add( cmbBoxBHold );
         mDCBoxes.Add( cmbBoxBHoldAlt );

         // A
         mDCBoxes.Add( cmbBoxA );
         mDCBoxes.Add( cmbBoxAAlt );
         mDCBoxes.Add( cmbBoxADC );
         mDCBoxes.Add( cmbBoxADCAlt );
         mDCBoxes.Add( cmbBoxAHold );
         mDCBoxes.Add( cmbBoxAHoldAlt );

         // X
         mDCBoxes.Add( cmbBoxX );
         mDCBoxes.Add( cmbBoxXAlt );
         mDCBoxes.Add( cmbBoxXDC );
         mDCBoxes.Add( cmbBoxXDCAlt );
         mDCBoxes.Add( cmbBoxXHold );
         mDCBoxes.Add( cmbBoxXHoldAlt );

         // Add functions to DC boxes
         foreach( ComboBox cmbBox in mDCBoxes )
         {
            cmbBox.Items.Add( FindFunctionStringByName( "ScreenSelect" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "GlobalSelect" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "Ability" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "Powers" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "Selection" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "DoubleClickSelect" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "MultiSelect" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "Clear" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "DoWork" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "DoWorkDirection" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "GotoBase" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "GotoAlert" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "GotoScout" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "GotoArmy" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "AssignGroup1" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "AssignGroup2" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "AssignGroup3" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "AssignGroup4" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "SelectGroup1" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "SelectGroup2" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "SelectGroup3" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "SelectGroup4" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "GotoGroup1" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "GotoGroup2" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "GotoGroup3" ) );
            cmbBox.Items.Add( FindFunctionStringByName( "GotoGroup4" ) );
         }

         // Left trigger
         cmbBoxLeftTrigger.Items.Add( FindFunctionStringByName( "ActionModifier" ) );
         cmbBoxLeftTrigger.Items.Add( FindFunctionStringByName( "SpeedModifier" ) );

         // Right trigger
         cmbBoxRightTrigger.Items.Add( FindFunctionStringByName( "ActionModifier" ) );
         cmbBoxRightTrigger.Items.Add( FindFunctionStringByName( "SpeedModifier" ) );

         // Left stick
         cmbBoxLeftStick.Items.Add( FindFunctionStringByName( "Translation" ) );
         cmbBoxLeftStick.Items.Add( GetPanZoomString() );
         cmbBoxLeftStick.Items.Add( GetPanTiltString() );

         // Left stick click
         cmbBoxLeftStickClick.Items.Add( FindFunctionStringByName( "Flare" ) );
         cmbBoxLeftStickClick.Items.Add( FindFunctionStringByName( "ResetCamera" ) );

         // Right stick
         cmbBoxRightStick.Items.Add( FindFunctionStringByName( "Translation" ) );
         cmbBoxRightStick.Items.Add( GetPanZoomString() );
         cmbBoxRightStick.Items.Add( GetPanTiltString() );

         // Right stick click
         cmbBoxRightStickClick.Items.Add( FindFunctionStringByName( "Flare" ) );
         cmbBoxRightStickClick.Items.Add( FindFunctionStringByName( "ResetCamera" ) );

         // Back         
         cmbBoxBack.Items.Add( FindFunctionStringByName( "Back" ) );
         cmbBoxBack.SelectedIndex = 1;
         cmbBoxBack.Enabled       = false;

         // Start
         cmbBoxStart.Items.Add( FindFunctionStringByName( "Start" ) );
         cmbBoxStart.SelectedIndex = 1;
         cmbBoxStart.Enabled       = false;
      }

      // Get pan/tilt string
      private string GetPanTiltString()
      {
         return( FindFunctionStringByName( "Pan" ) + "/" + FindFunctionStringByName( "Tilt" ) + " + " + FindFunctionStringByName( "Zoom" ) );
      }

      // Get pan/zoom string
      private string GetPanZoomString()
      {
         return( FindFunctionStringByName( "Pan" ) + "/" + FindFunctionStringByName( "Zoom" ) + " + " + FindFunctionStringByName( "Tilt" ) );
      }

      // Set combo box back color
      private void SetComboBoxBackColor( ConfigXML config, System.Windows.Forms.ComboBox cmbBox )
      {
         if( config.mModifiedControls.Contains( cmbBox.Name ) )
         {
            cmbBox.BackColor = SystemColors.ActiveBorder;
         }
         else
         {
            cmbBox.BackColor = SystemColors.Window;
         }
      }         

      // Set control modified flag
      private void SetControlModifiedFlag( ConfigXML config, System.Windows.Forms.ComboBox cmbBox )
      {
         if( ( cmbBox.BackColor == SystemColors.ActiveBorder ) && !config.mModifiedControls.Contains( cmbBox.Name ) )
         {
            config.mModifiedControls.Add( cmbBox.Name );
         }
         else if( ( cmbBox.BackColor == SystemColors.Window ) && config.mModifiedControls.Contains( cmbBox.Name ) )
         {
            config.mModifiedControls.Remove( cmbBox.Name );
         }
      }

      // Test to see if any controls are modified
      private bool AnyModifiedControls()
      {
         if( lstBoxConfigs.SelectedItem == null )
         {
            BlankControls();
            return( false );
         }

         ConfigXML config = mConfigsData.mConfigs[lstBoxConfigs.SelectedIndex];

         if( config.mModifiedControls.Count > 0 )
         {
            return( true );
         }

         return( false );
      }

      // Reset the combo boxes to the values saved in the XML for the current configuration
      private void ResetBoxes()
      {
         ConfigXML config = mConfigsData.mConfigs[lstBoxConfigs.SelectedIndex];

         foreach( Control control in this.Controls )
         {
            if( control.GetType().Name == "ComboBox" )
            {
               if( ExcludedControl( control ) )
               {
                  continue;
               }
               ComboBox cmbBox      = (ComboBox)control;
               cmbBox.SelectedIndex = 0;

               SetComboBoxBackColor( config, cmbBox );
            }
         }

         FunctionXML function = null;

         // Left trigger
         function = FindFunctionByKey( "TriggerLeft", false, false, false );
         if( function != null )
         {
            cmbBoxLeftTrigger.SelectedIndex = cmbBoxLeftTrigger.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Right trigger
         function = FindFunctionByKey( "TriggerRight", false, false, false );
         if( function != null )
         {
            cmbBoxRightTrigger.SelectedIndex = cmbBoxRightTrigger.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Left Bumper
         function = FindFunctionByKey( "ButtonShoulderLeft", false, false, false );
         if( function != null )
         {
            cmbBoxLeftBumper.SelectedIndex = cmbBoxLeftBumper.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );           
         }

         // Left Bumper Double Click
         function = FindFunctionByKey( "ButtonShoulderLeft", true, false, false );
         if( function != null )
         {
            cmbBoxLeftBumperDC.SelectedIndex = cmbBoxLeftBumperDC.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }         

         // Left Bumper Hold
         function = FindFunctionByKey( "ButtonShoulderLeft", false, true, false );
         if( function != null )
         {
            cmbBoxLeftBumperHold.SelectedIndex = cmbBoxLeftBumperHold.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Left Bumper Alt
         function = FindFunctionByKey( "ButtonShoulderLeft", false, false, true );
         if( function != null )
         {
            cmbBoxLeftBumperAlt.SelectedIndex = cmbBoxLeftBumperAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Left Bumper Double Click Alt
         function = FindFunctionByKey( "ButtonShoulderLeft", true, false, true );
         if( function != null )
         {
            cmbBoxLeftBumperDCAlt.SelectedIndex = cmbBoxLeftBumperDCAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Left Bumper Hold Alt
         function = FindFunctionByKey( "ButtonShoulderLeft", false, true, true );
         if( function != null )
         {
            cmbBoxLeftBumperHoldAlt.SelectedIndex = cmbBoxLeftBumperHoldAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Right Bumper
         function = FindFunctionByKey( "ButtonShoulderRight", false, false, false );
         if( function != null )
         {
            cmbBoxRightBumper.SelectedIndex = cmbBoxRightBumper.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Right Bumper Double Click
         function = FindFunctionByKey( "ButtonShoulderRight", true, false, false );
         if( function != null )
         {
            cmbBoxRightBumperDC.SelectedIndex = cmbBoxRightBumperDC.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }                  

         // Right Bumper Hold
         function = FindFunctionByKey( "ButtonShoulderRight", false, true, false );
         if( function != null )
         {
            cmbBoxRightBumperHold.SelectedIndex = cmbBoxRightBumperHold.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Right Bumper Alt
         function = FindFunctionByKey( "ButtonShoulderRight", false, false, true );
         if( function != null )
         {
            cmbBoxRightBumperAlt.SelectedIndex = cmbBoxRightBumperAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Right Bumper Double Click Alt
         function = FindFunctionByKey( "ButtonShoulderRight", true, false, true );
         if( function != null )
         {
            cmbBoxRightBumperDCAlt.SelectedIndex = cmbBoxRightBumperDCAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Right Bumper Hold Alt
         function = FindFunctionByKey( "ButtonShoulderRight", false, true, true );
         if( function != null )
         {
            cmbBoxRightBumperHoldAlt.SelectedIndex = cmbBoxRightBumperHoldAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Back
         //function = FindFunctionByKey( "ButtonBack", false, false, false );
         //if( function != null )
         //{
         //   cmbBoxBack.SelectedIndex = cmbBoxBack.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         //}

         // Start
         //function = FindFunctionByKey( "ButtonStart", false, false, false );
         //if( function != null )
         //{
         //   cmbBoxStart.SelectedIndex = cmbBoxStart.Items.IndexOf( FindeFunctionStringByName( function.ToString() ) );
         //}

         // Y
         function = FindFunctionByKey( "ButtonY", false, false, false );
         if( function != null )
         {
            cmbBoxY.SelectedIndex = cmbBoxY.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Y Double Click
         function = FindFunctionByKey( "ButtonY", true, false, false );
         if( function != null )
         {
            cmbBoxYDC.SelectedIndex = cmbBoxYDC.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }           

         // Y Hold
         function = FindFunctionByKey( "ButtonY", false, true, false );
         if( function != null )
         {
            cmbBoxYHold.SelectedIndex = cmbBoxYHold.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Y Alt
         function = FindFunctionByKey( "ButtonY", false, false, true );
         if( function != null )
         {
            cmbBoxYAlt.SelectedIndex = cmbBoxYAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Y Double Click Alt
         function = FindFunctionByKey( "ButtonY", true, false, true );
         if( function != null )
         {
            cmbBoxYDCAlt.SelectedIndex = cmbBoxYDCAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Y Hold Alt
         function = FindFunctionByKey( "ButtonY", false, true, true );
         if( function != null )
         {
            cmbBoxYHoldAlt.SelectedIndex = cmbBoxYHoldAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }            

         // B
         function = FindFunctionByKey( "ButtonB", false, false, false );
         if( function != null )
         {
            cmbBoxB.SelectedIndex = cmbBoxB.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // B Double Click
         function = FindFunctionByKey( "ButtonB", true, false, false );
         if( function != null )
         {
            cmbBoxBDC.SelectedIndex = cmbBoxBDC.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }       

         // B Hold
         function = FindFunctionByKey( "ButtonB", false, true, false );
         if( function != null )
         {
            cmbBoxBHold.SelectedIndex = cmbBoxBHold.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // B Alt
         function = FindFunctionByKey( "ButtonB", false, false, true );
         if( function != null )
         {
            cmbBoxBAlt.SelectedIndex = cmbBoxBAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // B Double Click Alt
         function = FindFunctionByKey( "ButtonB", true, false, true );
         if( function != null )
         {
            cmbBoxBDCAlt.SelectedIndex = cmbBoxBDCAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // B Hold Alt
         function = FindFunctionByKey( "ButtonB", false, true, true );
         if( function != null )
         {
            cmbBoxBHoldAlt.SelectedIndex = cmbBoxBHoldAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }            

         // A
         function = FindFunctionByKey( "ButtonA", false, false, false );
         if( function != null )
         {
            cmbBoxA.SelectedIndex = cmbBoxA.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // A Double Click
         function = FindFunctionByKey( "ButtonA", true, false, false );
         if( function != null )
         {
            cmbBoxADC.SelectedIndex = cmbBoxADC.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }                    

         // A Hold
         function = FindFunctionByKey( "ButtonA", false, true, false );
         if( function != null )
         {
            cmbBoxAHold.SelectedIndex = cmbBoxAHold.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // A Alt
         function = FindFunctionByKey( "ButtonA", false, false, true );
         if( function != null )
         {
            cmbBoxAAlt.SelectedIndex = cmbBoxAAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // A Double Click Alt
         function = FindFunctionByKey( "ButtonA", true, false, true );
         if( function != null )
         {
            cmbBoxADCAlt.SelectedIndex = cmbBoxADCAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // A Hold Alt
         function = FindFunctionByKey( "ButtonA", false, true, true );
         if( function != null )
         {
            cmbBoxAHoldAlt.SelectedIndex = cmbBoxAHoldAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }                           

         // X
         function = FindFunctionByKey( "ButtonX", false, false, false );
         if( function != null )
         {
            cmbBoxX.SelectedIndex = cmbBoxX.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // X Double Click
         function = FindFunctionByKey( "ButtonX", true, false, false );
         if( function != null )
         {
            cmbBoxXDC.SelectedIndex = cmbBoxXDC.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }           

         // X Hold
         function = FindFunctionByKey( "ButtonX", false, true, false );
         if( function != null )
         {
            cmbBoxXHold.SelectedIndex = cmbBoxXHold.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // X Alt
         function = FindFunctionByKey( "ButtonX", false, false, true );
         if( function != null )
         {
            cmbBoxXAlt.SelectedIndex = cmbBoxXAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // X Double Click Alt
         function = FindFunctionByKey( "ButtonX", true, false, true );
         if( function != null )
         {
            cmbBoxXDCAlt.SelectedIndex = cmbBoxXDCAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // X Hold Alt
         function = FindFunctionByKey( "ButtonX", false, true, true );
         if( function != null )
         {
            cmbBoxXHoldAlt.SelectedIndex = cmbBoxXHoldAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }            

         // DPad Up
         function = FindFunctionByKey( "DpadUp", false, false, false );
         if( function != null )
         {
            cmbBoxDPadUp.SelectedIndex = cmbBoxDPadUp.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Up Double Click
         function = FindFunctionByKey( "DpadUp", true, false, false );
         if( function != null )
         {
            cmbBoxDPadUpDC.SelectedIndex = cmbBoxDPadUpDC.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Up Hold
         function = FindFunctionByKey( "DpadUp", false, true, false );
         if( function != null )
         {
            cmbBoxDPadUpHold.SelectedIndex = cmbBoxDPadUpHold.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Up Alt
         function = FindFunctionByKey( "DpadUp", false, false, true );
         if( function != null )
         {
            cmbBoxDPadUpAlt.SelectedIndex = cmbBoxDPadUpAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Up Double Click Alt
         function = FindFunctionByKey( "DpadUp", true, false, true );
         if( function != null )
         {
            cmbBoxDPadUpDCAlt.SelectedIndex = cmbBoxDPadUpDCAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Up Hold Alt
         function = FindFunctionByKey( "DpadUp", false, true, true );
         if( function != null )
         {
            cmbBoxDPadUpHoldAlt.SelectedIndex = cmbBoxDPadUpHoldAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Right
         function = FindFunctionByKey( "DpadRight", false, false, false );
         if( function != null )
         {
            cmbBoxDPadRight.SelectedIndex = cmbBoxDPadRight.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Right Double Click
         function = FindFunctionByKey( "DpadRight", true, false, false );
         if( function != null )
         {
            cmbBoxDPadRightDC.SelectedIndex = cmbBoxDPadRightDC.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Right Hold
         function = FindFunctionByKey( "DpadRight", false, true, false );
         if( function != null )
         {
            cmbBoxDPadRightHold.SelectedIndex = cmbBoxDPadRightHold.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Right Alt
         function = FindFunctionByKey( "DpadRight", false, false, true );
         if( function != null )
         {
            cmbBoxDPadRightAlt.SelectedIndex = cmbBoxDPadRightAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Right Double Click Alt
         function = FindFunctionByKey( "DpadRight", true, false, true );
         if( function != null )
         {
            cmbBoxDPadRightDCAlt.SelectedIndex = cmbBoxDPadRightDCAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Right Hold Alt
         function = FindFunctionByKey( "DpadRight", false, true, true );
         if( function != null )
         {
            cmbBoxDPadRightHoldAlt.SelectedIndex = cmbBoxDPadRightHoldAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Down
         function = FindFunctionByKey( "DpadDown", false, false, false );
         if( function != null )
         {
            cmbBoxDPadDown.SelectedIndex = cmbBoxDPadDown.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Down Double Click
         function = FindFunctionByKey( "DpadDown", true, false, false );
         if( function != null )
         {
            cmbBoxDPadDownDC.SelectedIndex = cmbBoxDPadDownDC.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Down Hold
         function = FindFunctionByKey( "DpadDown", false, true, false );
         if( function != null )
         {
            cmbBoxDPadDownHold.SelectedIndex = cmbBoxDPadDownHold.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Down Alt
         function = FindFunctionByKey( "DpadDown", false, false, true );
         if( function != null )
         {
            cmbBoxDPadDownAlt.SelectedIndex = cmbBoxDPadDownAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Down Double Click Alt
         function = FindFunctionByKey( "DpadDown", true, false, true );
         if( function != null )
         {
            cmbBoxDPadDownDCAlt.SelectedIndex = cmbBoxDPadDownDCAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Down Hold Alt
         function = FindFunctionByKey( "DpadDown", false, true, true );
         if( function != null )
         {
            cmbBoxDPadDownHoldAlt.SelectedIndex = cmbBoxDPadDownHoldAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Left
         function = FindFunctionByKey( "DpadLeft", false, false, false );
         if( function != null )
         {
            cmbBoxDPadLeft.SelectedIndex = cmbBoxDPadLeft.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Left Double Click
         function = FindFunctionByKey( "DpadLeft", true, false, false );
         if( function != null )
         {
            cmbBoxDPadLeftDC.SelectedIndex = cmbBoxDPadLeftDC.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }                  

         // DPad Left Hold
         function = FindFunctionByKey( "DpadLeft", false, true, false );
         if( function != null )
         {
            cmbBoxDPadLeftHold.SelectedIndex = cmbBoxDPadLeftHold.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Left Alt
         function = FindFunctionByKey( "DpadLeft", false, false, true );
         if( function != null )
         {
            cmbBoxDPadLeftAlt.SelectedIndex = cmbBoxDPadLeftAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Left Double Click Alt
         function = FindFunctionByKey( "DpadLeft", true, false, true );
         if( function != null )
         {
            cmbBoxDPadLeftDCAlt.SelectedIndex = cmbBoxDPadLeftDCAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // DPad Left Hold Alt
         function = FindFunctionByKey( "DpadLeft", false, true, true );
         if( function != null )
         {
            cmbBoxDPadLeftHoldAlt.SelectedIndex = cmbBoxDPadLeftHoldAlt.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Left stick click
         function = FindFunctionByKey( "ButtonThumbLeft", false, false, false );
         if( function != null )
         {
            cmbBoxLeftStickClick.SelectedIndex = cmbBoxLeftStickClick.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Right stick click
         function = FindFunctionByKey( "ButtonThumbRight", false, false, false );
         if( function != null )
         {
            cmbBoxRightStickClick.SelectedIndex = cmbBoxRightStickClick.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         // Left and Right Sticks
         function = FindFunctionByName( "Translation" );
         if( ( function != null ) && function.hasKey( "StickLeft" ) )
         {
            cmbBoxLeftStick.SelectedIndex = cmbBoxLeftStick.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }
         if( ( function != null ) && function.hasKey( "StickRight" ) )
         {
            cmbBoxRightStick.SelectedIndex = cmbBoxRightStick.Items.IndexOf( FindFunctionStringByName( function.ToString() ) );
         }

         function = FindFunctionByName( "Tilt" );
         if( ( cmbBoxLeftStick.SelectedIndex == 0 ) && ( function != null ) && function.hasKey( "StickLeft" ) && !function.hasKey( FindAltKey() ) )
         {
            cmbBoxLeftStick.SelectedIndex = cmbBoxLeftStick.Items.IndexOf( GetPanTiltString() );
         }
         if( ( cmbBoxRightStick.SelectedIndex == 0 ) && ( function != null ) && function.hasKey( "StickRight" ) && !function.hasKey( FindAltKey() ) )
         {
            cmbBoxRightStick.SelectedIndex = cmbBoxRightStick.Items.IndexOf( GetPanTiltString() );
         }

         function = FindFunctionByName( "Zoom" );
         if( ( cmbBoxLeftStick.SelectedIndex == 0 ) && ( function != null ) && function.hasKey( "StickLeft" ) && !function.hasKey( FindAltKey() ) )
         {
            cmbBoxLeftStick.SelectedIndex = cmbBoxLeftStick.Items.IndexOf( GetPanZoomString() );
         }
         if( ( cmbBoxRightStick.SelectedIndex == 0 ) && ( function != null ) && function.hasKey( "StickRight" ) && !function.hasKey( FindAltKey() ) )
         {
            cmbBoxRightStick.SelectedIndex = cmbBoxRightStick.Items.IndexOf( GetPanZoomString() );
         }         
      }

      // Find function string from function name
      private string FindFunctionStringByName( string name )
      {
         string result = null;

         foreach( FunctionStringXML functionString in mConfigsData.mFunctionStrings.mFunctionString )
         {
            if( functionString.mFunctionStringName == name )
            {
               result = functionString.ToString();
               break;
            }
         }

         return( result );
      }

      // Find function name from function string
      private string FindFunctionNameByString( string funcString )
      {
         string result = null;

         foreach( FunctionStringXML functionString in mConfigsData.mFunctionStrings.mFunctionString )
         {
            if( functionString.ToString() == funcString )
            {
               result = functionString.mFunctionStringName;
               break;
            }
         }

         return( result );
      }

      // Update all of the dependent controls
      private void UpdateControls()
      {
         if( lstBoxConfigs.SelectedItem == null )
         {
            BlankControls();
            return;
         }

         // Set internal changes flag
         mInternal = true;

         // Enable controls
         EnableControls( true );

         // Update config name text box                    
         txtBoxConfigName.Text = mConfigsData.mConfigs[lstBoxConfigs.SelectedIndex].mConfigName;
         if( mConfigsData.mConfigs[lstBoxConfigs.SelectedIndex].mNameModified )
         {
            txtBoxConfigName.BackColor = SystemColors.ActiveBorder;
         }
         else
         {
            txtBoxConfigName.BackColor = SystemColors.Window;
         }

         // Update combo boxes
         ResetBoxes();

         if( AnyModifiedControls() || mConfigsData.mConfigs[lstBoxConfigs.SelectedIndex].mNameModified )
         {
            btnSave.Enabled  = true;
            btnReset.Enabled = true;
         }
         else
         {
            btnSave.Enabled  = false;
            btnReset.Enabled = false;
         }

         // Set internal changes flag
         mInternal = false;
      }

      // Blank the controls
      private void BlankControls()
      {
         // Set internal change flag
         mInternal = true;
         
         txtBoxConfigName.Text = "";
         btnSave.Enabled       = false;
         btnReset.Enabled      = false;

         txtBoxConfigName.BackColor = SystemColors.Window;

         // Reset combo box selections
         foreach( Control control in this.Controls )
         {
            if( control.GetType().Name == "ComboBox" )
            {
               if( ExcludedControl( control ) )
               {
                  continue;
               }
               ComboBox cmbBox      = (ComboBox)control;
               cmbBox.SelectedIndex = 0;
            }
         }

         // Disable controls
         EnableControls( false );

         ResetControlsColors();

         // Set internal change flag
         mInternal = false;
      }

      // Enable/Disable controls
      private void EnableControls( bool enable )
      {
         txtBoxConfigName.Enabled = enable;

         foreach( Control control in this.Controls )
         {
            if( control.GetType().Name == "ComboBox" )
            {
               if( ExcludedControl( control ) )
               {
                  continue;
               }
               ComboBox cmbBox = (ComboBox)control;
               cmbBox.Enabled  = enable;
            }
         }
      }

      // Find function by key
      private FunctionXML FindFunctionByKey( string key, bool dc, bool hold, bool alt )
      {
         if( lstBoxConfigs.SelectedItem == null )
         {
            BlankControls();
            return( null );
         }

         ConfigXML   config   = mConfigsData.mConfigs[lstBoxConfigs.SelectedIndex];
         string      altKey   = FindAltKey();
         bool        isAltKey = false;
         if( key == altKey )
         {
            alt      = false;
            altKey   = "";
            isAltKey = true;
         }
         foreach( FunctionXML function in config.mFunctions )
         {
            // Does function match key?
            if( ( function != null ) && function.hasKey( key ) )
            {
               if( isAltKey )
               {
                  return( function );
               }

               // Does function alternate usage match?
               if( alt != function.hasKey( altKey ) )
               {
                  continue;
               }

               // Does the double click usage match?
               if( dc != function.hasKey( "DoubleClick" ) )
               {
                  continue;
               }

               // Does the hold usage match?
               if( hold != function.hasKey( "Hold" ) )
               {
                  continue;
               }

               return( function );
            }
         }

         return( null );
      }

      // Find modify action key (alternate)
      private string FindAltKey()
      {
         FunctionXML actionModify = FindFunctionByName( "ActionModifier" );
         if( ( actionModify != null ) && ( actionModify.mKeys.Count > 0 ) )
         {
            return( actionModify.mKeys[0] );
         }

         return( "" );
      }

      // Find function by name
      private FunctionXML FindFunctionByName( string name )
      {
         if( lstBoxConfigs.SelectedItem == null )
         {
            BlankControls();
            return( null );
         }

         FunctionXML retFunction = null;
         ConfigXML   config      = mConfigsData.mConfigs[lstBoxConfigs.SelectedIndex];
         foreach( FunctionXML function in config.mFunctions )
         {
            if( function.mFunctionName == name )
            {
               retFunction = function;
               break;
            }
         }

         return( retFunction );
      }

      // Save data to XML file
      private void btnSave_Click( object sender, EventArgs e )
      {
         if( lstBoxConfigs.SelectedItem == null )
         {
            BlankControls();
            return;
         }

         // Set data
         SetConfigData();

         // Clear flags
         ClearAllModifiedFlags();

         // Save to XML file
         SaveXML();

         btnSave.Enabled  = false;
         btnReset.Enabled = false;

         ResetControlsColors();

         // Update dependent controls
         UpdateControls();
      }

      // Clear all modified flags
      private void ClearAllModifiedFlags()
      {
         foreach( ConfigXML config in mConfigsData.mConfigs )
         {
            config.mNameModified = false;
            config.mModifiedControls.Clear();
         }
      }

      // Clear function keys for currently selected configuration
      private void ClearFunctionKeys()
      {
         ConfigXML config = mConfigsData.mConfigs[lstBoxConfigs.SelectedIndex];
         foreach( FunctionXML function in config.mFunctions )
         {
            function.mKeys.Clear();
         }
      }

      // Validate Function
      private void ValidateFunction( FunctionXML function )
      {
         if( function.mKeys.Count > 0 )
         {
            MessageBox.Show( "Function: " + FindFunctionStringByName( function.mFunctionName ) + ", has been assigned to multiple keys!  This may cause errors and should be fixed before saving.", "Function With Multiple Key Assignments", MessageBoxButtons.OK, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button1, MessageBoxOptions.DefaultDesktopOnly );
         }
      }

      // New configuration has been selected
      private void lstBoxConfigs_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( mInternal )
         {
            return;
         }

         mInternal = true;

         int newIndex = lstBoxConfigs.SelectedIndex;

         // Set data
         lstBoxConfigs.SelectedIndex = mLastIndex;
         SetConfigData();

         lstBoxConfigs.SelectedIndex = newIndex;
         mLastIndex                  = newIndex;

         mInternal = false;

         // Update dependent controls
         UpdateControls();
      }

      // Set the config data based on the current controls' settings
      private void SetConfigData()
      {
         ConfigXML config = mConfigsData.mConfigs[lstBoxConfigs.SelectedIndex];

         // Name
         config.mConfigName = txtBoxConfigName.Text;
         if( txtBoxConfigName.BackColor == SystemColors.ActiveBorder )
         {
            config.mNameModified = true;
         }

         foreach( Control control in this.Controls )
         {
            if( control.GetType().Name == "ComboBox" )
            {
               if( ExcludedControl( control ) )
               {
                  continue;
               }
               ComboBox cmbBox = (ComboBox)control;

               SetControlModifiedFlag( config, cmbBox );
            }
         }         

         // Clear function keys
         ClearFunctionKeys();

         FunctionXML function = null;

         // Left Trigger
         if( cmbBoxLeftTrigger.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxLeftTrigger.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "TriggerLeft" );
            }
         }

         // Right Trigger
         if( cmbBoxRightTrigger.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxRightTrigger.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "TriggerRight" );
            }
         }

         // Warn against not having an action modified assigned
         if( FindAltKey() == "" )
         {
            MessageBox.Show( "Function: " + FindFunctionStringByName( "ActionModifier" ) + ", has not been assigned a key!  This may cause errors and should be fixed before saving.", FindFunctionStringByName( "ActionModifier" ) + " Needs Assignment", MessageBoxButtons.OK, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button1, MessageBoxOptions.DefaultDesktopOnly );
         }

         // Left Bumper
         if( cmbBoxLeftBumper.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxLeftBumper.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderLeft" );
            }
         }

         // Left Bumper Double Click
         if( cmbBoxLeftBumperDC.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxLeftBumperDC.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderLeft" );
               function.setKey( true, "DoubleClick" );
            }
         }

         // Left Bumper Hold
         if( cmbBoxLeftBumperHold.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxLeftBumperHold.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderLeft" );
               function.setKey( true, "Hold" );
            }
         }

         // Left Bumper Alt
         if( cmbBoxLeftBumperAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxLeftBumperAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderLeft" );
               function.setKey( true, FindAltKey() );
            }
         }

         // Left Bumper Double Click Alt
         if( cmbBoxLeftBumperDCAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxLeftBumperDCAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderLeft" );
               function.setKey( true, "DoubleClick" );
               function.setKey( true, FindAltKey() );
            }
         }

         // Left Bumper Hold Alt
         if( cmbBoxLeftBumperHoldAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxLeftBumperHoldAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderLeft" );
               function.setKey( true, "Hold" );
               function.setKey( true, FindAltKey() );
            }
         }

         // Right Bumper
         if( cmbBoxRightBumper.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxRightBumper.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderRight" );
            }
         }

         // Right Bumper Double Click
         if( cmbBoxRightBumperDC.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxRightBumperDC.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderRight" );
               function.setKey( true, "DoubleClick" );
            }
         }               

         // Right Bumper Hold
         if( cmbBoxRightBumperHold.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxRightBumperHold.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderRight" );
               function.setKey( true, "Hold" );
            }
         }

         // Right Bumper Alt
         if( cmbBoxRightBumperAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxRightBumperAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderRight" );
               function.setKey( true, FindAltKey() );
            }
         }

         // Right Bumper Double Click Alt
         if( cmbBoxRightBumperDCAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxRightBumperDCAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderRight" );
               function.setKey( true, "DoubleClick" );
               function.setKey( true, FindAltKey() );
            }
         }

         // Right Bumper Hold Alt
         if( cmbBoxRightBumperHoldAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxRightBumperHoldAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonShoulderRight" );
               function.setKey( true, "Hold" );
               function.setKey( true, FindAltKey() );
            }
         }

         // Back
         if( cmbBoxBack.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxBack.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonBack" );
            }
         }

         // Start
         if( cmbBoxStart.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxStart.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonStart" );
            }
         }

         // Y
         if( cmbBoxY.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxY.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonY" );
            }
         }

         // Y Double Click
         if( cmbBoxYDC.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxYDC.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonY" );
               function.setKey( true, "DoubleClick" );
            }
         }       

         // Y Hold
         if( cmbBoxYHold.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxYHold.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonY" );
               function.setKey( true, "Hold" );
            }
         }

         // Y Alt
         if( cmbBoxYAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxYAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonY" );
               function.setKey( true, FindAltKey() );
            }
         }

         // Y Double Click Alt
         if( cmbBoxYDCAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxYDCAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonY" );
               function.setKey( true, "DoubleClick" );
               function.setKey( true, FindAltKey() );
            }
         }

         // Y Hold Alt
         if( cmbBoxYHoldAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxYHoldAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonY" );
               function.setKey( true, "Hold" );
               function.setKey( true, FindAltKey() );
            }
         }            

         // B
         if( cmbBoxB.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxB.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonB" );
            }
         }

         // B Double Click
         if( cmbBoxBDC.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxBDC.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonB" );
               function.setKey( true, "DoubleClick" );
            }
         }           

         // B Hold
         if( cmbBoxBHold.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxBHold.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonB" );
               function.setKey( true, "Hold" );
            }
         }

         // B Alt
         if( cmbBoxBAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxBAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonB" );
               function.setKey( true, FindAltKey() );
            }
         }

         // B Double Click Alt
         if( cmbBoxBDCAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxBDCAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonB" );
               function.setKey( true, "DoubleClick" );
               function.setKey( true, FindAltKey() );
            }
         }

         // B Hold Alt
         if( cmbBoxBHoldAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxBHoldAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonB" );
               function.setKey( true, "Hold" );
               function.setKey( true, FindAltKey() );
            }
         }       

         // A
         if( cmbBoxA.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxA.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonA" );
            }
         }

         // A Double Click
         if( cmbBoxADC.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxADC.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonA" );
               function.setKey( true, "DoubleClick" );
            }
         }                    

         // A Hold
         if( cmbBoxAHold.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxAHold.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonA" );
               function.setKey( true, "Hold" );
            }
         }

         // A Alt
         if( cmbBoxAAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxAAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonA" );
               function.setKey( true, FindAltKey() );
            }
         }

         // A Double Click Alt
         if( cmbBoxADCAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxADCAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonA" );
               function.setKey( true, "DoubleClick" );
               function.setKey( true, FindAltKey() );
            }
         }

         // A Hold Alt
         if( cmbBoxAHoldAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxAHoldAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonA" );
               function.setKey( true, "Hold" );
               function.setKey( true, FindAltKey() );
            }
         }                        

         // X
         if( cmbBoxX.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxX.SelectedItem.ToString() ) );
            if ( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonX" );
            }
         }

         // X Double Click
         if( cmbBoxXDC.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxXDC.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonX" );
               function.setKey( true, "DoubleClick" );
            }
         }      

         // X Hold
         if( cmbBoxXHold.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxXHold.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonX" );
               function.setKey( true, "Hold" );
            }
         }

         // X Alt
         if( cmbBoxXAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxXAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonX" );
               function.setKey( true, FindAltKey() );
            }
         }

         // X Double Click Alt
         if( cmbBoxXDCAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxXDCAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonX" );
               function.setKey( true, "DoubleClick" );
               function.setKey( true, FindAltKey() );
            }
         }

         // X Hold Alt
         if( cmbBoxXHoldAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxXHoldAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonX" );
               function.setKey( true, "Hold" );
               function.setKey( true, FindAltKey() );
            }
         }        

         // DPad Up
         if( cmbBoxDPadUp.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadUp.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadUp" );
            }
         }

         // DPad Up Double Click
         if( cmbBoxDPadUpDC.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadUpDC.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadUp" );
               function.setKey( true, "DoubleClick" );
            }
         }

         // DPad Up Hold
         if( cmbBoxDPadUpHold.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadUpHold.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadUp" );
               function.setKey( true, "Hold" );
            }
         }

         // DPad Up Alt
         if( cmbBoxDPadUpAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadUpAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadUp" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Up Double Click Alt
         if( cmbBoxDPadUpDCAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadUpDCAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadUp" );
               function.setKey( true, "DoubleClick" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Up Hold Alt
         if( cmbBoxDPadUpHoldAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadUpHoldAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadUp" );
               function.setKey( true, "Hold" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Right
         if( cmbBoxDPadRight.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadRight.SelectedItem.ToString() ) );
            if ( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadRight" );
            }
         }

         // DPad Right Double Click
         if( cmbBoxDPadRightDC.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadRightDC.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadRight" );
               function.setKey( true, "DoubleClick" );
            }
         }

         // DPad Right Hold
         if( cmbBoxDPadRightHold.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadRightHold.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadRight" );
               function.setKey( true, "Hold" );
            }
         }

         // DPad Right Alt
         if( cmbBoxDPadRightAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadRightAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadRight" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Right Double Click Alt
         if( cmbBoxDPadRightDCAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadRightDCAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadRight" );
               function.setKey( true, "DoubleClick" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Right Hold Alt
         if( cmbBoxDPadRightHoldAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadRightHoldAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadRight" );
               function.setKey( true, "Hold" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Down
         if( cmbBoxDPadDown.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadDown.SelectedItem.ToString() ) );
            if ( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadDown" );
            }
         }

         // DPad Down Double Click
         if( cmbBoxDPadDownDC.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadDownDC.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadDown" );
               function.setKey( true, "DoubleClick" );
            }
         }

         // DPad Down Hold
         if( cmbBoxDPadDownHold.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadDownHold.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadDown" );
               function.setKey( true, "Hold" );
            }
         }

         // DPad Down Alt
         if( cmbBoxDPadDownAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadDownAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadDown" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Down Double Click Alt
         if( cmbBoxDPadDownDCAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadDownDCAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadDown" );
               function.setKey( true, "DoubleClick" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Down Hold Alt
         if( cmbBoxDPadDownHoldAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadDownHoldAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadDown" );
               function.setKey( true, "Hold" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Left
         if( cmbBoxDPadLeft.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadLeft.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadLeft" );
            }
         }

         // DPad Left Double Click
         if( cmbBoxDPadLeftDC.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadLeftDC.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadLeft" );
               function.setKey( true, "DoubleClick" );
            }
         }

         // DPad Left Hold
         if( cmbBoxDPadLeftHold.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadLeftHold.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadLeft" );
               function.setKey( true, "Hold" );
            }
         }

         // DPad Left Alt
         if( cmbBoxDPadLeftAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadLeftAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadLeft" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Left Double Click Alt
         if( cmbBoxDPadLeftDCAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadLeftDCAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadLeft" );
               function.setKey( true, "DoubleClick" );
               function.setKey( true, FindAltKey() );
            }
         }

         // DPad Left Hold Alt
         if( cmbBoxDPadLeftHoldAlt.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxDPadLeftHoldAlt.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "DpadLeft" );
               function.setKey( true, "Hold" );
               function.setKey( true, FindAltKey() );
            }
         }
         
         // Left Stick Click
         if( cmbBoxLeftStickClick.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxLeftStickClick.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonThumbLeft" );
            }
         }

         // Right Stick Click
         if( cmbBoxRightStickClick.SelectedItem != null )
         {
            function = FindFunctionByName( FindFunctionNameByString( cmbBoxRightStickClick.SelectedItem.ToString() ) );
            if( function != null )
            {
               ValidateFunction( function );
               function.setKey( true, "ButtonThumbRight" );
            }
         }

         // Left Stick
         if( cmbBoxLeftStick.SelectedItem != null )
         {
            if( cmbBoxLeftStick.SelectedItem.ToString() == GetPanTiltString() )
            {
               function = FindFunctionByName( "Pan" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickLeft" );
               }

               function = FindFunctionByName( "Tilt" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickLeft" );
               }

               function = FindFunctionByName( "Zoom" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickLeft" );
                  function.setKey( true, FindAltKey() );
               }
            }
            else if( cmbBoxLeftStick.SelectedItem.ToString() == GetPanZoomString() )
            {
               function = FindFunctionByName( "Pan" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickLeft" );
               }

               function = FindFunctionByName( "Tilt" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickLeft" );
                  function.setKey( true, FindAltKey() );
               }

               function = FindFunctionByName( "Zoom" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickLeft" );
               }
            }
            else if( cmbBoxLeftStick.SelectedItem.ToString() == FindFunctionStringByName( "Translation" ) )
            {
               function = FindFunctionByName( "Translation" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickLeft" );
               }
            }            
         }

         // Right Stick
         if( cmbBoxRightStick.SelectedItem != null )
         {
            if( cmbBoxRightStick.SelectedItem.ToString() == GetPanTiltString() )
            {
               function = FindFunctionByName( "Pan" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickRight" );
               }

               function = FindFunctionByName( "Tilt" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickRight" );
               }

               function = FindFunctionByName( "Zoom" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickRight" );
                  function.setKey( true, FindAltKey() );
               }
            }
            else if( cmbBoxRightStick.SelectedItem.ToString() == GetPanZoomString() )
            {
               function = FindFunctionByName( "Pan" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickRight" );
               }

               function = FindFunctionByName( "Tilt" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickRight" );
                  function.setKey( true, FindAltKey() );
               }

               function = FindFunctionByName( "Zoom" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickRight" );
               }
            }
            else if( cmbBoxRightStick.SelectedItem.ToString() == FindFunctionStringByName( "Translation" ) )
            {
               function = FindFunctionByName( "Translation" );
               if( function != null )
               {
                  ValidateFunction( function );
                  function.setKey( true, "StickRight" );
               }
            }            
         }
      }

      // Create a new controller configuration
      private void btnNew_Click( object sender, EventArgs e )
      {
         ConfigXML newConfig     = new ConfigXML();
         ConfigXML defaultConfig = FindDefaultConfig();

         if( defaultConfig != null )
         {
            newConfig.Copy( defaultConfig );
            newConfig.mModifiedControls.Clear();
         }

         // Set new name         
         mConfigsData.MaxConfigID++;
         string name             = "Config" + FormatCount( mConfigsData.MaxConfigID );
         newConfig.mConfigName   = name;
         newConfig.mID           = mConfigsData.MaxConfigID;
         newConfig.mNameModified = false;

         // Add new config to data list
         mConfigsData.mConfigs.Add( newConfig );

         mInternal = true;

         // Pause painting
         lstBoxConfigs.BeginUpdate();

         // Add to config list box
         lstBoxConfigs.Items.Add( newConfig.mConfigName );

         ResetControlsColors();

         mInternal = false;

         // Resume painting
         lstBoxConfigs.EndUpdate();

         // Reset index triggering an update
         lstBoxConfigs.SelectedIndex = lstBoxConfigs.Items.Count - 1;

         // Enable save button so new config can be saved to XML
         btnSave.Enabled = true;
      }

      // Return the default config if it exists
      private ConfigXML FindDefaultConfig()
      {
         ConfigXML result = null;

         foreach( ConfigXML config in mConfigsData.mConfigs )
         {
            if( config.mConfigName == "Default" )
            {
               result = config;
               break;
            }
         }

         return( result );
      }

      // Return appropriate number string
      private string FormatCount( int count )
      {
         if( count < 10 )
         {
            return( "0" + count.ToString() );
         }

         return( count.ToString() );
      }

      private void btnDelete_Click( object sender, EventArgs e )
      {
         if( lstBoxConfigs.SelectedItem == null )
         {
            BlankControls();
            return;
         }

         // Verify delete         
         DialogResult result = MessageBox.Show( "Are you sure you want to delete this configuration?", "Verify Delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button2 );
         if( result == DialogResult.No )
         {
            return;
         }

         int       oldIndex = lstBoxConfigs.SelectedIndex;
         ConfigXML config   = mConfigsData.mConfigs[oldIndex];

         if( config != null )
         {
            // Decrement max id if possible
            if( config.mID == mConfigsData.MaxConfigID )
            {
               mConfigsData.MaxConfigID--;
            }

            // Remove config from data list
            mConfigsData.mConfigs.Remove( config );
         }

         mInternal = true;         

         // Pause painting
         lstBoxConfigs.BeginUpdate();

         // Remove from config list box
         lstBoxConfigs.Items.Remove( lstBoxConfigs.SelectedItem );

         // Select the next config in the list box
         int count = lstBoxConfigs.Items.Count;
         if( ( oldIndex + 1 ) < count )
         {
            lstBoxConfigs.SelectedIndex = oldIndex + 1;
         }
         else if( count >= 2 )
         {
            lstBoxConfigs.SelectedIndex = oldIndex - 1;
         }
         mLastIndex = lstBoxConfigs.SelectedIndex;

         mInternal = false;

         // Resume painting
         lstBoxConfigs.EndUpdate();

         // Save the delete?
         result = MessageBox.Show( "Would you like to save your current changes?", "Verify Save", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button1 );
         if( result == DialogResult.Yes )
         {
            ClearAllModifiedFlags();

            // Save data to XML file
            SaveXML();            

            btnSave.Enabled  = false;
            btnReset.Enabled = false;
         }

         ResetControlsColors();

         // Update dependent controls
         UpdateControls();
      }

      // Return the config if it exists
      private ConfigXML FindConfigByName( string name )
      {
         ConfigXML result = null;         

         foreach( ConfigXML config in mConfigsData.mConfigs )
         {
            if( config.mConfigName == name )
            {
               result = config;
               break;
            }
         }

         return( result );
      }

      private bool ControlModified()
      {
         if( mInternal )
         {
            return( false );
         }

         if( lstBoxConfigs.SelectedItem == null )
         {
            BlankControls(); 
            return( false );
         }

         btnSave.Enabled  = true;
         btnReset.Enabled = true;

         return( true );
      }

      private void txtBoxConfigName_TextChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            txtBoxConfigName.BackColor = SystemColors.ActiveBorder;            
         }
      }

      private void cmbBoxLeftTrigger_SelectedValueChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxLeftTrigger.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxLeftBumper_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxLeftBumper.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxLeftBumperDC_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxLeftBumperDC.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxLeftBumperHold_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxLeftBumperHold.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxLeftStick_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxLeftStick.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxLeftStickClick_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxLeftStickClick.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxBack_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxBack.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadUp_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadUp.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadUpDC_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadUpDC.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadUpHold_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadUpHold.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadLeft_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadLeft.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadLeftDC_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadLeftDC.BackColor = SystemColors.ActiveBorder;
         }
      }      

      private void cmbBoxDPadLeftHold_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadLeftHold.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadRight_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadRight.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadRightDC_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadRightDC.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadRightHold_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadRightHold.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadDown_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadDown.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadDownDC_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadDownDC.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadDownHold_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadDownHold.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxRightTrigger_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxRightTrigger.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxRightBumper_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxRightBumper.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxRightBumperDC_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxRightBumperDC.BackColor = SystemColors.ActiveBorder;            
         }
      }

      private void cmbBoxRightBumperHold_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxRightBumperHold.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxY_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxY.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxYDC_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxYDC.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxYHold_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxYHold.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxB_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxB.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxBDC_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxBDC.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxBHold_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxBHold.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxX_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxX.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxXDC_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxXDC.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxXHold_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxXHold.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxA_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxA.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxADC_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxADC.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxAHold_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxAHold.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxStart_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxStart.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxRightStick_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxRightStick.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxRightStickClick_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxRightStickClick.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxLeftBumperAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxLeftBumperAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxLeftBumperDCAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxLeftBumperDCAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxLeftBumperHoldAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxLeftBumperHoldAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadUpAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadUpAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadUpDCAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadUpDCAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadUpHoldAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadUpHoldAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadLeftAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadLeftAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadLeftDCAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadLeftDCAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadLeftHoldAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadLeftHoldAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadRightAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadRightAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadRightDCAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadRightDCAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadRightHoldAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadRightHoldAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadDownAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadDownAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadDownDCAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadDownDCAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxDPadDownHoldAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxDPadDownHoldAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxRightBumperAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxRightBumperAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxRightBumperDCAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxRightBumperDCAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxRightBumperHoldAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxRightBumperHoldAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxYAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxYAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxYDCAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxYDCAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxYHoldAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxYHoldAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxBAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxBAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxBDCAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxBDCAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxBHoldAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxBHoldAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxXAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxXAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxXDCAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxXDCAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxXHoldAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxXHoldAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxAAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxAAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxADCAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxADCAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void cmbBoxAHoldAlt_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( ControlModified() )
         {
            cmbBoxAHoldAlt.BackColor = SystemColors.ActiveBorder;
         }
      }

      private void btnReset_Click( object sender, EventArgs e )
      {
         if( lstBoxConfigs.SelectedItem == null )
         {
            BlankControls(); 
            return;
         }

         ConfigXML config = mConfigsData.mConfigs[lstBoxConfigs.SelectedIndex];
         if( config == null )
         {
            return;
         }

         ConfigXML oldConfig = FindOldConfigByID( config.mID );
         if( oldConfig != null )
         {
            config.Copy( oldConfig );
         }

         config.mNameModified = false;
         config.mModifiedControls.Clear();

         ResetControlsColors();

         UpdateControls();
      }

      // Reset control background color
      private void ResetControlsColors()
      {
         txtBoxConfigName.BackColor = SystemColors.Window;

         foreach( Control control in this.Controls )
         {
            if( control.GetType().Name == "ComboBox" )
            {
               if( ExcludedControl( control ) )
               {
                  continue;
               }
               ComboBox cmbBox  = (ComboBox)control;
               cmbBox.BackColor = SystemColors.Window;
            }
         }
      }

      // Test to see if these controls are currently excluded
      private bool ExcludedControl( Control control )
      {
         if
         ( 
            //( control == cmbBoxLeftBumperDC )  ||
            //( control == cmbBoxRightBumperDC ) ||
            //( control == cmbBoxYDC )           ||
            //( control == cmbBoxBDC )           ||
            //( control == cmbBoxXDC )           ||
            //( control == cmbBoxADC )           ||
            //( control == cmbBoxYHold )         ||
            //( control == cmbBoxBHold )         ||
            //( control == cmbBoxXHold )         ||
            //( control == cmbBoxAHold )         ||
            //( control == cmbBoxDPadUpDC )      ||
            //( control == cmbBoxDPadLeftDC )    ||
            //( control == cmbBoxDPadRightDC )   ||
            //( control == cmbBoxDPadDownDC )    ||
            ( control == cmbBoxBack )          ||
            ( control == cmbBoxStart )         //||
            //( control == btnNew )              ||
            //( control == btnDelete ) 
         )
         {
            return( true );
         }

         //if( control.Name.Contains( "Alt" ) )
         //{
         //   return( true );
         //}

         return( false );
      }

      // Find the old config by ID
      private ConfigXML FindOldConfigByID( int ID )
      {
         ConfigXML result = null;

         foreach( ConfigXML config in mConfigsDataOld.mConfigs )
         {
            if( config.mID == ID )
            {
               result = config;
               break;
            }
         }

         return( result );
      }

      private void picBoxController_Paint( object sender, PaintEventArgs e )
      {
         Pen          pen          = new Pen( Color.FromArgb( 255, 0, 0, 0 ), 2 );
         GraphicsPath path         = new GraphicsPath();        
         const int    widthOffset  = 5;
         int          heightOffset = ( lblLeftTriggerPos.Bounds.Height / 2 ) - 2;

         pen.Alignment = PenAlignment.Center;
         pen.LineJoin  = LineJoin.Round;
         pen.SetLineCap( LineCap.Round, LineCap.Round, DashCap.Flat );

         // Left Trigger         
         int x1 = lblLeftTriggerPos.Bounds.Left;
         int y1 = lblLeftTriggerPos.Bounds.Top + heightOffset; 
         int x2 = lblLeftTriggerJoint.Bounds.Left;
         int y2 = lblLeftTriggerJoint.Bounds.Top + heightOffset;
         int x3 = cmbBoxLeftTrigger.Bounds.Right + widthOffset;
         int y3 = cmbBoxLeftTrigger.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );         
         e.Graphics.DrawPath( pen, path );

         // Left Bumper Click
         x1 = lblLeftBumperPos.Bounds.Left;
         y1 = lblLeftBumperPos.Bounds.Top + heightOffset;
         x2 = lblLeftBumperJoint.Bounds.Left;
         y2 = lblLeftBumperJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxLeftBumper.Bounds.Right + widthOffset;
         y3 = cmbBoxLeftBumper.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Left Bumper Double Click
         x3 = cmbBoxLeftBumperDC.Bounds.Right + widthOffset;
         y3 = cmbBoxLeftBumperDC.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Left Bumper Hold
         x3 = cmbBoxLeftBumperHold.Bounds.Right + widthOffset;
         y3 = cmbBoxLeftBumperHold.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Left Stick Axis
         x1 = lblLeftStickPos.Bounds.Left;
         y1 = lblLeftStickPos.Bounds.Top + heightOffset;
         x2 = lblLeftStickJoint.Bounds.Left;
         y2 = lblLeftStickJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxLeftStick.Bounds.Right + widthOffset;
         y3 = cmbBoxLeftStick.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Left Stick Click
         x3 = cmbBoxLeftStickClick.Bounds.Right + widthOffset;
         y3 = cmbBoxLeftStickClick.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Back
         x1 = lblBackPos.Bounds.Left;
         y1 = lblBackPos.Bounds.Top + heightOffset;
         x2 = lblBackJoint.Bounds.Left;
         y2 = lblBackJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxBack.Bounds.Right + widthOffset;
         y3 = cmbBoxBack.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Up Click
         x1 = lblDPadUpPos.Bounds.Left;
         y1 = lblDPadUpPos.Bounds.Top + heightOffset;
         x2 = lblDPadUpJoint.Bounds.Left;
         y2 = lblDPadUpJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxDPadUp.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadUp.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Up Double Click
         x3 = cmbBoxDPadUpDC.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadUpDC.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Up Hold
         x3 = cmbBoxDPadUpHold.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadUpHold.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Left Click
         x1 = lblDPadLeftPos.Bounds.Left;
         y1 = lblDPadLeftPos.Bounds.Top + heightOffset;
         x2 = lblDPadLeftJoint.Bounds.Left;
         y2 = lblDPadLeftJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxDPadLeft.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadLeft.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Left Double Click
         x3 = cmbBoxDPadLeftDC.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadLeftDC.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Left Hold
         x3 = cmbBoxDPadLeftHold.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadLeftHold.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Right Click
         x1 = lblDPadRightPos.Bounds.Left;
         y1 = lblDPadRightPos.Bounds.Top + heightOffset;
         x2 = lblDPadRightJoint.Bounds.Left;
         y2 = lblDPadRightJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxDPadRight.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadRight.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Right Double Click
         x3 = cmbBoxDPadRightDC.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadRightDC.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Right Hold
         x3 = cmbBoxDPadRightHold.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadRightHold.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Down Click
         x1 = lblDPadDownPos.Bounds.Left;
         y1 = lblDPadDownPos.Bounds.Top + heightOffset;
         x2 = lblDPadDownJoint.Bounds.Left;
         y2 = lblDPadDownJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxDPadDown.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadDown.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Down Double Click
         x3 = cmbBoxDPadDownDC.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadDownDC.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // DPad Down Hold
         x3 = cmbBoxDPadDownHold.Bounds.Right + widthOffset;
         y3 = cmbBoxDPadDownHold.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Right Trigger
         x1 = lblRightTriggerPos.Bounds.Right;
         y1 = lblRightTriggerPos.Bounds.Top + heightOffset;
         x2 = lblRightTriggerJoint.Bounds.Right;
         y2 = lblRightTriggerJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxRightTrigger.Bounds.Left - widthOffset;
         y3 = cmbBoxRightTrigger.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Right Bumper
         x1 = lblRightBumperPos.Bounds.Right;
         y1 = lblRightBumperPos.Bounds.Top + heightOffset;
         x2 = lblRightBumperJoint.Bounds.Right;
         y2 = lblRightBumperJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxRightBumper.Bounds.Left - widthOffset;
         y3 = cmbBoxRightBumper.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Right Bumper Double Click
         x3 = cmbBoxRightBumperDC.Bounds.Left - widthOffset;
         y3 = cmbBoxRightBumperDC.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Right Bumper Hold
         x3 = cmbBoxRightBumperHold.Bounds.Left - widthOffset;
         y3 = cmbBoxRightBumperHold.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Y
         x1 = lblYPos.Bounds.Right;
         y1 = lblYPos.Bounds.Top + heightOffset;
         x2 = lblYJoint.Bounds.Right;
         y2 = lblYJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxY.Bounds.Left - widthOffset;
         y3 = cmbBoxY.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Y Double Click
         x3 = cmbBoxYDC.Bounds.Left - widthOffset;
         y3 = cmbBoxYDC.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Y Hold
         x3 = cmbBoxYHold.Bounds.Left - widthOffset;
         y3 = cmbBoxYHold.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // B
         x1 = lblBPos.Bounds.Right;
         y1 = lblBPos.Bounds.Top + heightOffset;
         x2 = lblBJoint.Bounds.Right;
         y2 = lblBJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxB.Bounds.Left - widthOffset;
         y3 = cmbBoxB.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // B Double Click
         x3 = cmbBoxBDC.Bounds.Left - widthOffset;
         y3 = cmbBoxBDC.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // B Hold
         x3 = cmbBoxBHold.Bounds.Left - widthOffset;
         y3 = cmbBoxBHold.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // X
         x1 = lblXPos.Bounds.Right;
         y1 = lblXPos.Bounds.Top + heightOffset;
         x2 = lblXJoint.Bounds.Right;
         y2 = lblXJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxX.Bounds.Left - widthOffset;
         y3 = cmbBoxX.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // X Double Click
         x3 = cmbBoxXDC.Bounds.Left - widthOffset;
         y3 = cmbBoxXDC.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // X Hold
         x3 = cmbBoxXHold.Bounds.Left - widthOffset;
         y3 = cmbBoxXHold.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // A
         x1 = lblAPos.Bounds.Right;
         y1 = lblAPos.Bounds.Top + heightOffset;
         x2 = lblAJoint.Bounds.Right;
         y2 = lblAJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxA.Bounds.Left - widthOffset;
         y3 = cmbBoxA.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // A Double Click
         x3 = cmbBoxADC.Bounds.Left - widthOffset;
         y3 = cmbBoxADC.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // A Hold
         x3 = cmbBoxAHold.Bounds.Left - widthOffset;
         y3 = cmbBoxAHold.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Start
         x1 = lblStartPos.Bounds.Right;
         y1 = lblStartPos.Bounds.Top + heightOffset;
         x2 = lblStartJoint.Bounds.Right;
         y2 = lblStartJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxStart.Bounds.Left - widthOffset;
         y3 = cmbBoxStart.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Right Stick
         x1 = lblRightStickPos.Bounds.Right;
         y1 = lblRightStickPos.Bounds.Top + heightOffset;
         x2 = lblRightStickJoint.Bounds.Right;
         y2 = lblRightStickJoint.Bounds.Top + heightOffset;
         x3 = cmbBoxRightStick.Bounds.Left - widthOffset;
         y3 = cmbBoxRightStick.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x1, y1 ), new Point( x2, y2 ) );
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         // Right Stick Click
         x3 = cmbBoxRightStickClick.Bounds.Left - widthOffset;
         y3 = cmbBoxRightStickClick.Bounds.Top + heightOffset;

         path.StartFigure();
         path.AddLine( new Point( x2, y2 ), new Point( x3, y3 ) );
         e.Graphics.DrawPath( pen, path );

         pen.Dispose();
      }

      private void ControllerConfigurationsPage_Paint( object sender, PaintEventArgs e )
      {
      }

      private void SwitchVisibleSet()
      {
         foreach( Control control in this.Controls )
         {
            if( control.GetType().Name == "ComboBox" )
            {
               ComboBox cmbBox = (ComboBox)control;
               if( cmbBox.Name.Contains( "Alt" ) )
               {
                  cmbBox.Visible = rdbAlternate.Checked;
               }
               else
               {
                  cmbBox.Visible = rdbStandard.Checked;
               }
            }
         }
      }

      private void rdbStandard_CheckedChanged( object sender, EventArgs e )
      {
         if( mInternal )
         {
            return;
         }

         SwitchVisibleSet();
      }

      private void rdbAlternate_CheckedChanged( object sender, EventArgs e )
      {
         if( mInternal )
         {
            return;
         }

         SwitchVisibleSet();
      }
   }
}
