using System;
using System.Collections.Generic;
using System.Text;

using Microsoft.Ink;
using Microsoft.StylusInput;
using Microsoft.StylusInput.PluginData;

using System.Windows.Forms;
using System.Drawing;

using System.IO;

namespace EditorCore
{
   class TabletSupport
   {
      RealTimeStylus mRTStylus = null;
      Control mParent = null;
      SimpleStylusPlugin mPlugin = null;

      public TabletSupport(Control parent)
      {
         mParent = parent;
      }
      public void Start()
      {
         try
         {
            if (File.Exists(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "NoStylus")))
               return;
            
            mRTStylus = new RealTimeStylus(mParent);
            mPlugin = new SimpleStylusPlugin(this);
            mRTStylus.SyncPluginCollection.Add(mPlugin);
            Guid[] g = new Guid[] { PacketProperty.X, PacketProperty.Y, PacketProperty.NormalPressure, PacketProperty.XTiltOrientation, PacketProperty.YTiltOrientation };
            mRTStylus.SetDesiredPacketDescription(g);


            Tablets theTablets = new Tablets();
            string theReport = Environment.NewLine;
            foreach (Tablet theTablet in theTablets)
            {
               if (theTablet.IsPacketPropertySupported(PacketProperty.NormalPressure))
               {
                  TabletPropertyMetrics theMetrics = theTablet.GetPropertyMetrics(PacketProperty.NormalPressure);
                  mMaxPressure = theMetrics.Maximum;
               }
            }



            mRTStylus.Enabled = true;

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());

            File.Create(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "NoStylus")).Close();

         }
      }
      public void Stop()
      {
         mRTStylus.Enabled = false;
      }
      public void Pause()
      {
      }
      public void UnPause()
      {
      }

      int mMaxPressure = 255;
      float mPressure = 0;
      public float Pressure
      {
         get
         {
            return mPressure;
         }
         protected set
         {
            mPressure = value / mMaxPressure;
         }
      }

      class SimpleStylusPlugin : IStylusSyncPlugin
      {


         TabletSupport mParent = null;
         public SimpleStylusPlugin(TabletSupport parent)
         {
            mParent = parent;
         }

         DataInterestMask mIntrests = DataInterestMask.Packets | DataInterestMask.StylusDown | DataInterestMask.InAirPackets;
         public DataInterestMask DataInterest
         {
            get { return mIntrests; }
            set { mIntrests = value; }
         }

         public void StylusDown(RealTimeStylus sender, StylusDownData data)
         {

         }
         public void Packets(RealTimeStylus sender, PacketsData data)
         {
            if (data.Count >= 3)
            {
               mParent.Pressure = data[2];
            }
         }
         public void StylusUp(RealTimeStylus sender, StylusUpData data) { }
         public void CustomStylusDataAdded(RealTimeStylus sender, CustomStylusData data) { }
         public void Error(RealTimeStylus sender, ErrorData data) { }
         public void InAirPackets(RealTimeStylus sender, InAirPacketsData data) { }
         public void RealTimeStylusDisabled(RealTimeStylus sender, RealTimeStylusDisabledData data) { }
         public void RealTimeStylusEnabled(RealTimeStylus sender, RealTimeStylusEnabledData data) { }
         public void StylusButtonDown(RealTimeStylus sender, StylusButtonDownData data) { }
         public void StylusButtonUp(RealTimeStylus sender, StylusButtonUpData data) { }
         public void StylusInRange(RealTimeStylus sender, StylusInRangeData data) { }
         public void StylusOutOfRange(RealTimeStylus sender, StylusOutOfRangeData data) { }
         public void SystemGesture(RealTimeStylus sender, SystemGestureData data) { }
         public void TabletAdded(RealTimeStylus sender, TabletAddedData data) { }
         public void TabletRemoved(RealTimeStylus sender, TabletRemovedData data) { }
      }



   }
}
