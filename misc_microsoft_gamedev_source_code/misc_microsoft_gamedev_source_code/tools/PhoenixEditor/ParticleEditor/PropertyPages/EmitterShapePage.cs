using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Xceed.SmartUI.Controls.OptionList;

namespace ParticleSystem
{
   public partial class EmitterShape : UserControl
   {
      private ParticleEmitter data;
      private bool bInitialized = false;
      public EmitterShape()
      {
         InitializeComponent();
         smartOptionList1.ItemClick += new Xceed.SmartUI.SmartItemClickEventHandler(smartOptionList1_ItemClick);
         for (int i = 0; i < smartOptionList1.Items.Count; i++)
         {
            smartOptionList1.Items[i].Tag = new ShapeData.ShapeTypeEnum();            
         }

         smartOptionList1.Items[0].Tag = ShapeData.ShapeTypeEnum.ePoint;
         smartOptionList1.Items[1].Tag = ShapeData.ShapeTypeEnum.eBox;
         smartOptionList1.Items[2].Tag = ShapeData.ShapeTypeEnum.eCylinder;
         smartOptionList1.Items[3].Tag = ShapeData.ShapeTypeEnum.eSphere;
         smartOptionList1.Items[4].Tag = ShapeData.ShapeTypeEnum.eHalfSphere;
         smartOptionList1.Items[5].Tag = ShapeData.ShapeTypeEnum.eRectangle;
         smartOptionList1.Items[6].Tag = ShapeData.ShapeTypeEnum.eCircle;
      }     
      
      public void setData(ParticleEmitter e)
      {
         bInitialized = false;
         data = e;
         getModifiedData();
         bInitialized = true;
         updateEnableStates();
      }

      public void setModifiedData()
      {
         if (!bInitialized)
            return;

         //-- Shape 
         data.Shape.XSize = (double)numericUpDown1.Value;
         data.Shape.YSize = (double)numericUpDown2.Value;
         data.Shape.ZSize = (double)numericUpDown3.Value;

         data.Shape.XPosOffset = (double)numericUpDown4.Value;
         data.Shape.YPosOffset = (double)numericUpDown5.Value;
         data.Shape.ZPosOffset = (double)numericUpDown6.Value;

         data.Shape.EmitFromSurface = winCheckBox1.Checked;
         data.Shape.EmitFromSurfaceRadius = (double)numericUpDown12.Value;

         //-- Trajectory
         data.Shape.TrajectoryInnerAngle = (double)numericUpDown7.Value;
         data.Shape.TrajectoryOuterAngle = (double)numericUpDown8.Value;
         data.Shape.TrajectoryPitch = (double)numericUpDown9.Value;
         data.Shape.TrajectoryYaw   = (double)numericUpDown10.Value;
         data.Shape.TrajectoryBank  = (double)numericUpDown11.Value;
      }

      public void getModifiedData()
      {
         //-- Shape          
         numericUpDown1.Value = Math.Round((decimal)data.Shape.XSize, 2, MidpointRounding.ToEven);
         numericUpDown2.Value =  Math.Round((decimal)data.Shape.YSize, 2, MidpointRounding.ToEven);
         numericUpDown3.Value =  Math.Round((decimal)data.Shape.ZSize, 2, MidpointRounding.ToEven);

         numericUpDown4.Value =  Math.Round((decimal)data.Shape.XPosOffset, 2, MidpointRounding.ToEven);
         numericUpDown5.Value =  Math.Round((decimal)data.Shape.YPosOffset, 2, MidpointRounding.ToEven);
         numericUpDown6.Value =  Math.Round((decimal)data.Shape.ZPosOffset, 2, MidpointRounding.ToEven);

         winCheckBox1.Checked = data.Shape.EmitFromSurface;
         numericUpDown12.Value =  Math.Round((decimal)data.Shape.EmitFromSurfaceRadius, 2, MidpointRounding.ToEven);

         //-- Trajectory
         numericUpDown7.Value =  Math.Round((decimal)data.Shape.TrajectoryInnerAngle, 2, MidpointRounding.ToEven);
         numericUpDown8.Value =  Math.Round((decimal)data.Shape.TrajectoryOuterAngle, 2, MidpointRounding.ToEven);
         numericUpDown9.Value =  Math.Round((decimal)data.Shape.TrajectoryPitch, 2, MidpointRounding.ToEven);
         numericUpDown10.Value=  Math.Round((decimal)data.Shape.TrajectoryYaw, 2, MidpointRounding.ToEven);
         numericUpDown11.Value=  Math.Round((decimal)data.Shape.TrajectoryBank, 2, MidpointRounding.ToEven);

         for (int i = 0; i < smartOptionList1.Items.Count; ++i)
         {
            if (data.Shape.ShapeType == (ShapeData.ShapeTypeEnum)smartOptionList1.Items[i].Tag)
            {
               ((RadioButtonNode)smartOptionList1.Items[i]).Checked = true;
               break;
            }
         }
      }

      private void winCheckBox1_CheckedChanged(object sender, EventArgs e)
      {
         data.Shape.EmitFromSurface = winCheckBox1.Checked;
         numericUpDown12.Enabled = data.Shape.EmitFromSurface;
      }

      private void numericUpDown_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         setModifiedData();
      }

      private void smartOptionList1_ItemClick(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         data.Shape.ShapeType = (ShapeData.ShapeTypeEnum)e.Item.Tag;
         updateEnableStates();
      }

      private void numericUpDown_Enter(object sender, EventArgs e)
      {
         NumericUpDown control = (NumericUpDown)sender;
         control.Select(0, control.Text.Length);
      }

      private void updateEnableStates()
      {
         groupBox1.Enabled = true;
         groupBoxEX3.Enabled = true;
         groupBoxEX4.Enabled = true;
         numericUpDown1.Enabled = true;
         numericUpDown2.Enabled = true;
         numericUpDown3.Enabled = true;
         if (data.Shape.ShapeType == ShapeData.ShapeTypeEnum.ePoint)
         {
            groupBox1.Enabled = false;
            groupBoxEX3.Enabled = false;
            numericUpDown1.Enabled = false;
            numericUpDown2.Enabled = false;
            numericUpDown3.Enabled = false;
         }
         else if (data.Shape.ShapeType == ShapeData.ShapeTypeEnum.eBox)
         {
            //-- everything enabled
         }
         else if (data.Shape.ShapeType == ShapeData.ShapeTypeEnum.eCylinder)
         {
            numericUpDown3.Enabled = false;
         }
         else if (data.Shape.ShapeType == ShapeData.ShapeTypeEnum.eSphere)
         {
            numericUpDown2.Enabled = false;
            numericUpDown3.Enabled = false;
         }
         else if (data.Shape.ShapeType == ShapeData.ShapeTypeEnum.eHalfSphere)
         {
            numericUpDown2.Enabled = false;
            numericUpDown3.Enabled = false;
         }
         else if (data.Shape.ShapeType == ShapeData.ShapeTypeEnum.eRectangle)
         {
            numericUpDown3.Enabled = false;
         }
         else if (data.Shape.ShapeType == ShapeData.ShapeTypeEnum.eCircle)
         {
            numericUpDown3.Enabled = false;
         }
      }      
   }
}
