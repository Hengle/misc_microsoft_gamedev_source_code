using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

using EditorCore;

namespace PhoenixEditor
{
   class FractalUISettings
   {

      static void SetMetaData(ObjectEditorControl objectEditor )
      {
         //register type editors

         //objectEditor

         //basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "File", "FileFilter", "Scenario (*.scn)|*.scn");
         //basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "File", "RootDirectory", CoreGlobals.getWorkPaths().mGameScenarioDirectory);
         //basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "File", "StartingDirectory", CoreGlobals.getWorkPaths().mGameScenarioDirectory);
         //basicTypedSuperList1.SetTypeEditor("ScenarioInfoXml", "File", typeof(FileNameProperty));
         
         
         //basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "Type", "SimpleEnumeration", new string[] { "Playtest", "Development", "Test" });
         //basicTypedSuperList1.SetTypeEditor("ScenarioInfoXml", "Type", typeof(EnumeratedProperty));
         try
         {

            FileStream f = new FileStream(CoreGlobals.getWorkPaths().mEditorSettings + "\\FractaltUISettings.xml", FileMode.Open, FileAccess.Read);
            objectEditor.LoadSettingsFromStream(f);
         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().SendToErrorWarningViewer(ex.ToString());
         }
      }


   }
}
