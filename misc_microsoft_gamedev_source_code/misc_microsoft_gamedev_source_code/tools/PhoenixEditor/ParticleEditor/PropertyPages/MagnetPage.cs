using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Xceed.SmartUI;
using Xceed.SmartUI.Controls.OptionList;

namespace ParticleSystem
{
   public partial class MagnetPage : UserControl
   {
      public MagnetPage()
      {
         InitializeComponent();

         smartOptionList1.ItemClick += new Xceed.SmartUI.SmartItemClickEventHandler(smartOptionList1_ItemClick);
         for (int i = 0; i < smartOptionList1.Items.Count; i++)
         {
            smartOptionList1.Items[i].Tag = new MagnetData.MagnetTypeEnum();
         }
         smartOptionList1.Items[0].Tag = MagnetData.MagnetTypeEnum.eSphere;
         smartOptionList1.Items[1].Tag = MagnetData.MagnetTypeEnum.eCylinder;

      }

      private MagnetData data;
      private bool bInitialized = false;

      public void setData(MagnetData e)
      {
         bInitialized = false;
         data = e;

         getModifiedData();
         updateEnableStates();
         bInitialized = true;
      }

      private void getModifiedData()
      {
         numericUpDown1.Value = (decimal)Math.Round((decimal)data.Force,2, MidpointRounding.ToEven);
         numericUpDown2.Value = (decimal)Math.Round((decimal)data.Radius, 2, MidpointRounding.ToEven);
         numericUpDown3.Value = (decimal)Math.Round((decimal)data.XPosOffset, 2, MidpointRounding.ToEven);
         numericUpDown4.Value = (decimal)Math.Round((decimal)data.YPosOffset, 2, MidpointRounding.ToEven);
         numericUpDown5.Value = (decimal)Math.Round((decimal)data.ZPosOffset, 2, MidpointRounding.ToEven);
         numericUpDown6.Value = (decimal)Math.Round((decimal)data.Height, 2, MidpointRounding.ToEven);
         numericUpDown7.Value = (decimal)Math.Round((decimal)data.Dampening, 2, MidpointRounding.ToEven);
         numericUpDown8.Value = (decimal)Math.Round((decimal)data.Turbulence, 2, MidpointRounding.ToEven);
         numericUpDown9.Value = (decimal)Math.Round((decimal)data.RotationalForce, 2, MidpointRounding.ToEven);

         numericUpDown10.Value = (decimal)Math.Round((decimal)data.XRotation, 2, MidpointRounding.ToEven);
         numericUpDown11.Value = (decimal)Math.Round((decimal)data.YRotation, 2, MidpointRounding.ToEven);
         numericUpDown12.Value = (decimal)Math.Round((decimal)data.ZRotation, 2, MidpointRounding.ToEven);

         for (int i = 0; i < smartOptionList1.Items.Count; i++)
         {
            if (data.MagnetType == (MagnetData.MagnetTypeEnum)smartOptionList1.Items[i].Tag)
            {
               ((RadioButtonNode)smartOptionList1.Items[i]).Checked = true;
               break;
            }
         }

      }

      private void numericUpDown1_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.Force = (double)numericUpDown1.Value;
      }

      private void numericUpDown2_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.Radius = (double)numericUpDown2.Value;
      }

      private void numericUpDown3_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.XPosOffset = (double)numericUpDown3.Value;
      }

      private void numericUpDown4_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.YPosOffset = (double)numericUpDown4.Value;
      }

      private void numericUpDown5_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.ZPosOffset = (double)numericUpDown5.Value;
      }

      private void numericUpDown6_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.Height = (double)numericUpDown6.Value;
      }

      private void numericUpDown7_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.Dampening = (double)numericUpDown7.Value;
      }

      private void numericUpDown8_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.Turbulence = (double)numericUpDown8.Value;
      }

      private void numericUpDown9_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.RotationalForce = (double)numericUpDown9.Value;
      }

      private void smartOptionList1_ItemClick(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         if (!bInitialized)
            return;
         data.MagnetType = (MagnetData.MagnetTypeEnum)e.Item.Tag;
         updateEnableStates();
      }
      private void updateEnableStates()
      {
         bool bIsCylinder = (data.MagnetType == MagnetData.MagnetTypeEnum.eCylinder);
         numericUpDown6.Enabled  = bIsCylinder;
         numericUpDown9.Enabled  = bIsCylinder;
         numericUpDown10.Enabled = bIsCylinder;
         numericUpDown11.Enabled = bIsCylinder;
         numericUpDown12.Enabled = bIsCylinder;
      }

      private void numericUpDown10_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.XRotation = (double)numericUpDown10.Value;
      }

      private void numericUpDown11_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.YRotation = (double)numericUpDown11.Value;
      }

      private void numericUpDown12_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         data.ZRotation = (double)numericUpDown12.Value;
      }      
   }
}
