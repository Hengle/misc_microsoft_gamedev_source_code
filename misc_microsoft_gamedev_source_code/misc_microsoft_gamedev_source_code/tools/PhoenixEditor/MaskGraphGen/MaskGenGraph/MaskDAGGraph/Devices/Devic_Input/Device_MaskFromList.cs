using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
//using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using EditorCore;

namespace graphapp
{
   public class Device_MaskFromList : MaskDevice
   {
      MaskParam mOutputMask = new MaskParam();
      [XmlIgnore]
      [ConnectionType("Output", "Primary Output")]
      public DAGMask OutputMask
      {
         get { return mOutputMask.Value; }
         set { mOutputMask.Value = value; }
      }


      string mSelectedMaskName = "";
      public string SelectedMaskName
      {
         get { return mSelectedMaskName; }
         set { mSelectedMaskName = value; }
      }

      public Device_MaskFromList()
      { }

      public Device_MaskFromList(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {

         base.Text = "Mask From List";
         mColorTop = Color.White;
         mColorBottom = Color.HotPink;
         mBorderSize = 1;

         mSize.Width = 60;
         mSize.Height = 20;


         generateConnectionPoints();
         resizeFromConnections();
      }

      public override bool load(MaskDAGGraphNode fromNode)
      {
         Device_MaskFromList dc = fromNode as Device_MaskFromList;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         SelectedMaskName = dc.SelectedMaskName;

         return true;
      }

      override public void displayPropertiesDlg()
      {
         Device_MaskFromListDlg dlg = new Device_MaskFromListDlg(this);
         dlg.ShowDialog();
      }
      override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
         if (!verifyInputConnections())
            return false;

         if(SelectedMaskName == null || SelectedMaskName == "")
            return false;

         gatherInputAndParameters(parms);

         ArrayBasedMask terrMask = null;

         IMask msk = CoreGlobals.getEditorMain().mIMaskPickerUI.GetMask(SelectedMaskName);
         if (msk == null)
            return false;
         if(msk is ArrayBasedMask)
         {
            terrMask = msk as ArrayBasedMask;
         }
         else if (msk is GraphBasedMask)
         {
            //we have to generate this mask...
            if(((GraphBasedMask)msk).loadAndExecute())
               terrMask = ((GraphBasedMask)msk).getOutputMask();
         }

         

         int numXVerts = CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts();
         int numZVerts = CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts();
         float[] fk = new float[numXVerts * numZVerts];
         for (int x = 0; x < numXVerts; x++)
         {
            for (int y = 0; y < numZVerts; y++)
            {
               fk[x + numXVerts * y] = terrMask.GetValue(x * numXVerts + y);
            }
         }


         if (numXVerts != parms.Width || numZVerts != parms.Height)
         {
            float[] resizedHeights = ImageManipulation.resizeF32Img(fk, numXVerts, numZVerts, parms.Width, parms.Height, ImageManipulation.eFilterType.cFilter_Linear);
            fk = (float[])resizedHeights.Clone();
         }



         MaskParam mp = ((MaskParam)(connPoint.ParamType));
         mp.Value = new DAGMask(parms.Width, parms.Height);

         for (int x = 0; x < parms.Width; x++)
         {
            for (int y = 0; y < parms.Height; y++)
            {
               float k = fk[x + parms.Width * y];
               mp.Value[x, y] = k;
            }
         }

         terrMask = null;
         msk = null;

         return true;
      }

      override public string getDeviceDescriptor()
      {
         return "CurrentMask : This takes the current terrain mask as the input";
      }
      override public string getDeviceHelp()
      {
         return "CurrentMask : This description has not yet been implimented";
      }
   }
}