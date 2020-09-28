using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class MapPropFrm : UserControl
   {

      MapTreeNode mOwnerNode = null;
      public MapPropFrm()
      {
         InitializeComponent();
      }

      public void setOwner(MapTreeNode owner)
      {
         mOwnerNode = owner;

         glsTextBox.Text = mOwnerNode.mGLSFileToUse;
      }

      private void button1_Click(object sender, EventArgs e)
      {
          OpenFileDialog d = new OpenFileDialog();

          d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameLightsetBaseDirectory;

         d.Filter = "ES Lightset File (*.gls)|*.gls";
         d.FilterIndex = 0;

         if (d.ShowDialog() == DialogResult.OK)
         {
            //remove anything before 'work'
            if(d.FileName.Contains(CoreGlobals.getWorkPaths().mGameDirectory))
               d.FileName = d.FileName.Remove(0, CoreGlobals.getWorkPaths().mGameDirectory.Length+1);
            

            glsTextBox.Text = d.FileName;
            mOwnerNode.mGLSFileToUse = glsTextBox.Text;
         }
      }
   }
}
