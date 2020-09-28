using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using ParticleSystem;
using Xceed.Editors;
using Xceed.SmartUI;
using Xceed.SmartUI.Controls.OptionList;


namespace ParticleSystem
{
   public partial class EmitterPage : UserControl
   {
      private ParticleEmitter data;
      bool bInitialized = false;
      public EmitterPage()
      {
         InitializeComponent();

         tabControlEX1.SelectedIndex = 0;

         //-- register event handlers
         smartOptionList1.ItemClick += new Xceed.SmartUI.SmartItemClickEventHandler(smartOptionList1_ItemClick);
         smartOptionList2.ItemClick += new Xceed.SmartUI.SmartItemClickEventHandler(smartOptionList2_ItemClick);

         smartOptionList5.ItemClick += new Xceed.SmartUI.SmartItemClickEventHandler(smartOptionList5_ItemClick);         
         for (int i = 0; i < smartOptionList1.Items.Count; i++)
         {
            smartOptionList1.Items[i].Tag = new EmitterData.EmitterParticleTypeEnum();
         }

         for (int j = 0; j < smartOptionList2.Items.Count; j++)
         {
            smartOptionList2.Items[j].Tag = new EmitterData.EmitterBlendModeEnum();
         }

         for (int k = 0; k < smartOptionList3.Items.Count; k++)
         {
            smartOptionList3.Items[k].Tag = new EmitterData.EmitterTrailEmissionType();
         }

         for (int l= 0; l < smartOptionList4.Items.Count; l++)
         {
            smartOptionList4.Items[l].Tag = new EmitterData.EmitterTrailUVType();
         }

         for (int m = 0; m < smartOptionList5.Items.Count; m++)
         {
            smartOptionList5.Items[m].Tag = new EmitterData.EmitterBeamAlignmentType();
         }

         //-- set the corresponding radiobutton tags
         smartOptionList1.Items[0].Tag = EmitterData.EmitterParticleTypeEnum.eBillBoard;
         smartOptionList1.Items[1].Tag = EmitterData.EmitterParticleTypeEnum.eOrientedAxialBillboard;
         smartOptionList1.Items[2].Tag = EmitterData.EmitterParticleTypeEnum.eUpfacing;
         smartOptionList1.Items[3].Tag = EmitterData.EmitterParticleTypeEnum.eTrail;
         smartOptionList1.Items[4].Tag = EmitterData.EmitterParticleTypeEnum.eTrailCross;
         smartOptionList1.Items[5].Tag = EmitterData.EmitterParticleTypeEnum.eBeam;
         smartOptionList1.Items[6].Tag = EmitterData.EmitterParticleTypeEnum.eVelocityAligned;
         smartOptionList1.Items[7].Tag = EmitterData.EmitterParticleTypeEnum.ePFX;
         smartOptionList1.Items[8].Tag = EmitterData.EmitterParticleTypeEnum.eTerrainPatch;         
                  
         smartOptionList2.Items[0].Tag = EmitterData.EmitterBlendModeEnum.eAlphaBlend;
         smartOptionList2.Items[1].Tag = EmitterData.EmitterBlendModeEnum.eAdditive;
         smartOptionList2.Items[2].Tag = EmitterData.EmitterBlendModeEnum.eSubtractive;
         smartOptionList2.Items[3].Tag = EmitterData.EmitterBlendModeEnum.eDistortion;
         smartOptionList2.Items[4].Tag = EmitterData.EmitterBlendModeEnum.ePremultipliedAlpha;


         smartOptionList3.Items[0].Tag = EmitterData.EmitterTrailEmissionType.eEmitByLength;
         smartOptionList3.Items[1].Tag = EmitterData.EmitterTrailEmissionType.eEmitByTime;

         smartOptionList4.Items[0].Tag = EmitterData.EmitterTrailUVType.eStretch;
         smartOptionList4.Items[1].Tag = EmitterData.EmitterTrailUVType.eFaceMap;

         smartOptionList5.Items[0].Tag = EmitterData.EmitterBeamAlignmentType.eBeamAlignToCamera;
         smartOptionList5.Items[1].Tag = EmitterData.EmitterBeamAlignmentType.eBeamAlignVertical;
         smartOptionList5.Items[2].Tag = EmitterData.EmitterBeamAlignmentType.eBeamAlignHorizontal;
      }
      
      public void setData(ParticleEmitter e)
      {
         bInitialized = false;
         data = e;
         getModifiedData();         
         bInitialized = true;
         updateEnableStates();         
      }

      private void setModifiedData()
      {
         if (!bInitialized)
            return;

         //-- Base Tab
         data.Emitter.UpdateRadius     = (double)numericUpDown15.Value;
         data.Emitter.GlobalFadeIn     = (double)numericUpDown16.Value;
         data.Emitter.GlobalFadeOut    = (double)numericUpDown17.Value;
         data.Emitter.GlobalFadeInVar  = (double)numericUpDown18.Value;
         data.Emitter.GlobalFadeOutVar = (double)numericUpDown19.Value;

         //-- Emission Tab
         //-- values
         data.Emitter.MaxParticles  = (int)    numericUpDown1.Value;
         data.Emitter.ParticleLife  = (double) numericUpDown2.Value;
         data.Emitter.EmissionRate  = (double) numericUpDown3.Value;
         data.Emitter.StartDelay    = (double) numericUpDown4.Value;
         data.Emitter.InitialUpdate = (double) numericUpDown5.Value;
         data.Emitter.EmissionTime  = (double) numericUpDown6.Value;
         data.Emitter.LoopDelay     = (double) numericUpDown7.Value;

         //-- variances
         data.Emitter.MaxParticlesVar  = (double)numericUpDown8.Value;
         data.Emitter.ParticleLifeVar  = (double)numericUpDown9.Value;
         data.Emitter.EmissionRateVar  = (double)numericUpDown10.Value;
         data.Emitter.StartDelayVar    = (double)numericUpDown11.Value;
         data.Emitter.InitialUpdateVar = (double)numericUpDown12.Value;
         data.Emitter.EmissionTimeVar  = (double)numericUpDown13.Value;
         data.Emitter.LoopDelayVar     = (double)numericUpDown14.Value;

         //-- Motion Tab
         data.Emitter.Velocity           = (double)numericUpDown20.Value;
         data.Emitter.Acceleration       = (double)numericUpDown21.Value;
         data.Emitter.InitialDistance    = (double)numericUpDown22.Value;
         data.Emitter.VelocityVar        = (double)numericUpDown23.Value;
         data.Emitter.AccelerationVar    = (double)numericUpDown24.Value;
         data.Emitter.InitialDistanceVar = (double)numericUpDown25.Value;

         data.Emitter.Loop          = winCheckBox1.Checked;
         data.Emitter.TiedToEmitter = winCheckBox2.Checked;
         data.Emitter.AlwaysActive  = winCheckBox3.Checked;
         data.Emitter.AlwaysRender = checkBox7.Checked;

         //-- Forces Tab2
         data.Forces.UseTumble = checkBox4.Checked;
         data.Forces.RandomOrientation = checkBox3.Checked;
         data.Forces.TumbleBothDirections = checkBox5.Checked;
         data.Forces.UseInternalGravity = checkBox1.Checked;         

         data.Forces.MinAngularTumbleVelocity = (double) numericUpDown34.Value;
         data.Forces.MaxAngularTumbleVelocity = (double) numericUpDown35.Value;

         data.Forces.InternalGravity = (double)numericUpDown26.Value;
         data.Forces.InternalGravityVar = (double)numericUpDown27.Value;
         
         data.Emitter.TrailSegmentLength = (double)numericUpDown37.Value;

         data.Emitter.EmitterAttraction = (double)numericUpDown36.Value;
         data.Emitter.EmitterAttractionVar = (double)numericUpDown38.Value;

         data.Emitter.BeamTesselation = (int) numericUpDown28.Value;
         data.Emitter.BeamTangent1X = (double)numericUpDown29.Value;
         data.Emitter.BeamTangent1Y = (double)numericUpDown30.Value;
         data.Emitter.BeamTangent1Z = (double)numericUpDown31.Value;

         data.Emitter.BeamTangent2X = (double)numericUpDown39.Value;
         data.Emitter.BeamTangent2Y = (double)numericUpDown33.Value;
         data.Emitter.BeamTangent2Z = (double)numericUpDown32.Value;
         
         data.Emitter.BeamColorByLength = checkBox2.Checked;
         data.Emitter.BeamOpacityByLength = checkBox6.Checked;
         data.Emitter.BeamIntensityByLength = checkBox8.Checked;

         data.Emitter.PFXFilePath = fileBrowseControl1.FileName;

         data.Emitter.CollisionDetectionTerrain = checkBox9.Checked;
         data.Emitter.CollisionEnergyLoss = (double)numericUpDown40.Value;
         data.Emitter.CollisionEnergyLossVar = (double)numericUpDown41.Value;
         data.Emitter.CollisionOffset = (double)numericUpDown42.Value;

         data.Emitter.SortParticles = checkBox10.Checked;
         data.Emitter.FillOptimized = checkBox11.Checked;
         data.Emitter.SoftParticles = checkBox12.Checked;

         data.Emitter.TerrainDecalTesselation = (double)floatSliderEdit1.Value;
         data.Emitter.TerrainDecalYOffset = (double)numericUpDown44.Value;

         data.Emitter.SoftParticleFadeRange = (double)floatSliderEdit2.Value;

         data.Emitter.LightBuffer = checkBox13.Checked;
         data.Emitter.LightBufferIntensityScale = (double)numericUpDown43.Value;

         data.Emitter.KillImmediatelyOnRelease = checkBox14.Checked;
      }

      private void getModifiedData()
      {
         //-- Base Tab
         
         numericUpDown15.Value = Math.Round((decimal)data.Emitter.UpdateRadius, 2, MidpointRounding.ToEven);;
         numericUpDown16.Value = Math.Round((decimal)data.Emitter.GlobalFadeIn, 2, MidpointRounding.ToEven);;
         numericUpDown17.Value = Math.Round((decimal)data.Emitter.GlobalFadeOut, 2, MidpointRounding.ToEven);;
         numericUpDown18.Value = Math.Round((decimal)data.Emitter.GlobalFadeInVar, 2, MidpointRounding.ToEven);;
         numericUpDown19.Value = Math.Round((decimal)data.Emitter.GlobalFadeOutVar, 2, MidpointRounding.ToEven);;

         //-- Emission Tab
         //-- values
         numericUpDown1.Value = Math.Round((decimal) data.Emitter.MaxParticles, 2, MidpointRounding.ToEven);;
         numericUpDown2.Value = Math.Round((decimal)data.Emitter.ParticleLife, 2, MidpointRounding.ToEven);;
         numericUpDown3.Value = Math.Round((decimal)data.Emitter.EmissionRate, 2, MidpointRounding.ToEven);;
         numericUpDown4.Value = Math.Round((decimal)data.Emitter.StartDelay, 2, MidpointRounding.ToEven);;
         numericUpDown5.Value = Math.Round((decimal)data.Emitter.InitialUpdate, 2, MidpointRounding.ToEven);;
         numericUpDown6.Value = Math.Round((decimal)data.Emitter.EmissionTime, 2, MidpointRounding.ToEven);;
         numericUpDown7.Value = Math.Round((decimal)data.Emitter.LoopDelay, 2, MidpointRounding.ToEven);;

         //-- variances
         numericUpDown8.Value  = Math.Round((decimal)data.Emitter.MaxParticlesVar, 2, MidpointRounding.ToEven);;
         numericUpDown9.Value  = Math.Round((decimal)data.Emitter.ParticleLifeVar, 2, MidpointRounding.ToEven);;
         numericUpDown10.Value = Math.Round((decimal)data.Emitter.EmissionRateVar, 2, MidpointRounding.ToEven);;
         numericUpDown11.Value = Math.Round((decimal)data.Emitter.StartDelayVar, 2, MidpointRounding.ToEven);;
         numericUpDown12.Value = Math.Round((decimal)data.Emitter.InitialUpdateVar, 2, MidpointRounding.ToEven);;
         numericUpDown13.Value = Math.Round((decimal)data.Emitter.EmissionTimeVar, 2, MidpointRounding.ToEven);;
         numericUpDown14.Value = Math.Round((decimal)data.Emitter.LoopDelayVar, 2, MidpointRounding.ToEven);;

         //-- Motion Tab
         numericUpDown20.Value = Math.Round((decimal)data.Emitter.Velocity, 2, MidpointRounding.ToEven);;
         numericUpDown21.Value = Math.Round((decimal)data.Emitter.Acceleration, 2, MidpointRounding.ToEven);;
         numericUpDown22.Value = Math.Round((decimal)data.Emitter.InitialDistance, 2, MidpointRounding.ToEven);;
         numericUpDown23.Value = Math.Round((decimal)data.Emitter.VelocityVar, 2, MidpointRounding.ToEven);;
         numericUpDown24.Value = Math.Round((decimal)data.Emitter.AccelerationVar, 2, MidpointRounding.ToEven);;
         numericUpDown25.Value = Math.Round((decimal)data.Emitter.InitialDistanceVar, 2, MidpointRounding.ToEven);;

         winCheckBox1.Checked = data.Emitter.Loop;
         winCheckBox2.Checked = data.Emitter.TiedToEmitter;
         winCheckBox3.Checked = data.Emitter.AlwaysActive;
         checkBox7.Checked = data.Emitter.AlwaysRender;

         for (int i = 0; i < smartOptionList1.Items.Count; i++)
         {
            if (data.Emitter.ParticleType == (EmitterData.EmitterParticleTypeEnum)smartOptionList1.Items[i].Tag)
            {
               ((RadioButtonNode)smartOptionList1.Items[i]).Checked = true;
               break;
            }
         }

         for (int j = 0; j < smartOptionList2.Items.Count; j++)
         {
            if (data.Emitter.BlendMode == (EmitterData.EmitterBlendModeEnum) smartOptionList2.Items[j].Tag)
            {
               ((RadioButtonNode)smartOptionList2.Items[j]).Checked = true;
               break;
            }
         }

         for (int k = 0; k < smartOptionList5.Items.Count; k++)
         {
            if (data.Emitter.BeamAlignmentType == (EmitterData.EmitterBeamAlignmentType)smartOptionList5.Items[k].Tag)
            {
               ((RadioButtonNode)smartOptionList5.Items[k]).Checked = true;
               break;
            }
         }

         // Forces
         checkBox4.Checked = data.Forces.UseTumble;
         checkBox3.Checked = data.Forces.RandomOrientation;
         checkBox5.Checked = data.Forces.TumbleBothDirections;
         checkBox1.Checked = data.Forces.UseInternalGravity;         

         numericUpDown34.Value = Math.Round((decimal)data.Forces.MinAngularTumbleVelocity, 2, MidpointRounding.ToEven);;
         numericUpDown35.Value = Math.Round((decimal)data.Forces.MaxAngularTumbleVelocity, 2, MidpointRounding.ToEven);;

         numericUpDown26.Value = Math.Round((decimal)data.Forces.InternalGravity, 2, MidpointRounding.ToEven);;
         numericUpDown27.Value = Math.Round((decimal)data.Forces.InternalGravityVar, 2, MidpointRounding.ToEven);;         

         //-- trail
         numericUpDown37.Value = Math.Round((decimal)data.Emitter.TrailSegmentLength, 2, MidpointRounding.ToEven);

         numericUpDown36.Value = Math.Round((decimal)data.Emitter.EmitterAttraction, 2, MidpointRounding.ToEven);
         numericUpDown38.Value = Math.Round((decimal)data.Emitter.EmitterAttractionVar, 2, MidpointRounding.ToEven);
         
         numericUpDown28.Value = data.Emitter.BeamTesselation;
         numericUpDown29.Value = Math.Round((decimal)data.Emitter.BeamTangent1X, 2, MidpointRounding.ToEven);
         numericUpDown30.Value = Math.Round((decimal)data.Emitter.BeamTangent1Y, 2, MidpointRounding.ToEven);
         numericUpDown31.Value = Math.Round((decimal)data.Emitter.BeamTangent1Z, 2, MidpointRounding.ToEven);

         numericUpDown39.Value = Math.Round((decimal)data.Emitter.BeamTangent2X, 2, MidpointRounding.ToEven);
         numericUpDown33.Value = Math.Round((decimal)data.Emitter.BeamTangent2Y, 2, MidpointRounding.ToEven);
         numericUpDown32.Value = Math.Round((decimal)data.Emitter.BeamTangent2Z, 2, MidpointRounding.ToEven);

         numericUpDown40.Value = Math.Round((decimal)data.Emitter.CollisionEnergyLoss, 2, MidpointRounding.ToEven);
         numericUpDown41.Value = Math.Round((decimal)data.Emitter.CollisionEnergyLossVar, 2, MidpointRounding.ToEven);
         numericUpDown42.Value = Math.Round((decimal)data.Emitter.CollisionOffset, 2, MidpointRounding.ToEven);

         numericUpDown43.Value = Math.Round((decimal)data.Emitter.LightBufferIntensityScale, 2, MidpointRounding.ToEven);

         floatSliderEdit1.Value = (float)data.Emitter.TerrainDecalTesselation;
         numericUpDown44.Value = Math.Round((decimal)data.Emitter.TerrainDecalYOffset, 3, MidpointRounding.ToEven);

         floatSliderEdit2.Value = (float)data.Emitter.SoftParticleFadeRange;         

         checkBox2.Checked = data.Emitter.BeamColorByLength;
         checkBox6.Checked = data.Emitter.BeamOpacityByLength;
         checkBox8.Checked = data.Emitter.BeamIntensityByLength;
         checkBox9.Checked = data.Emitter.CollisionDetectionTerrain;
         checkBox10.Checked = data.Emitter.SortParticles;
         checkBox11.Checked = data.Emitter.FillOptimized;
         checkBox12.Checked = data.Emitter.SoftParticles;
         checkBox13.Checked = data.Emitter.LightBuffer;
         checkBox14.Checked = data.Emitter.KillImmediatelyOnRelease;

         fileBrowseControl1.FileName = data.Emitter.PFXFilePath;
      }

      private void winCheckBox1_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.Loop = (bool)winCheckBox1.Checked;
         updateEnableStates();
      }                 

      private void winCheckBox2_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.TiedToEmitter = (bool)winCheckBox2.Checked;
         updateEnableStates();
      }
      
      private void winCheckBox3_CheckedChanged(object sender, EventArgs e)
      {         
         data.Emitter.AlwaysActive  = (bool)winCheckBox3.Checked;
         updateEnableStates();
      }

      private void smartOptionList1_ItemClick(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         data.Emitter.ParticleType = (EmitterData.EmitterParticleTypeEnum) e.Item.Tag;
         updateEnableStates();
      }

      private void smartOptionList2_ItemClick(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         data.Emitter.BlendMode = (EmitterData.EmitterBlendModeEnum)e.Item.Tag;
         updateEnableStates();
      }
      
      private void numericUpDown_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;
         setModifiedData();
      }

      private void updateEnableStates()
      {
         //-- Tumble
         numericUpDown34.Enabled = data.Forces.UseTumble;
         numericUpDown35.Enabled = data.Forces.UseTumble;      

         //-- Gravity
         numericUpDown26.Enabled = data.Forces.UseInternalGravity;
         numericUpDown27.Enabled = data.Forces.UseInternalGravity;         

         bool isTrailType = (data.Emitter.ParticleType == EmitterData.EmitterParticleTypeEnum.eTrail) ||
                            (data.Emitter.ParticleType == EmitterData.EmitterParticleTypeEnum.eTrailCross);
         groupBox4.Enabled = isTrailType;

         checkBox10.Enabled = (data.Emitter.BlendMode == EmitterData.EmitterBlendModeEnum.eAlphaBlend) &&
                              (data.Emitter.ParticleType == EmitterData.EmitterParticleTypeEnum.eBillBoard ||
                               data.Emitter.ParticleType == EmitterData.EmitterParticleTypeEnum.eOrientedAxialBillboard ||
                               data.Emitter.ParticleType == EmitterData.EmitterParticleTypeEnum.eUpfacing ||
                               data.Emitter.ParticleType == EmitterData.EmitterParticleTypeEnum.eVelocityAligned);


         checkBox13.Enabled = (data.Emitter.BlendMode == EmitterData.EmitterBlendModeEnum.eAlphaBlend) ||
                              (data.Emitter.BlendMode == EmitterData.EmitterBlendModeEnum.ePremultipliedAlpha);

         numericUpDown43.Enabled = checkBox13.Enabled;

         //-- 
         numericUpDown36.Enabled = data.Emitter.TiedToEmitter;
         numericUpDown38.Enabled = data.Emitter.TiedToEmitter;

         numericUpDown40.Enabled = data.Emitter.CollisionDetectionTerrain;
         numericUpDown41.Enabled = data.Emitter.CollisionDetectionTerrain;
         numericUpDown42.Enabled = data.Emitter.CollisionDetectionTerrain;
      }

      private void checkBox_CheckedChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         setModifiedData();

         updateEnableStates();
      }

      private void numericUpDown_Enter(object sender, EventArgs e)
      {
         NumericUpDown control = (NumericUpDown)sender;
         control.Select(0, control.Text.Length);         
      }

      private void smartOptionList3_ItemClick(object sender, SmartItemClickEventArgs e)
      {
         data.Emitter.TrailEmissionType = (EmitterData.EmitterTrailEmissionType)e.Item.Tag;
      }

      private void smartOptionList4_ItemClick(object sender, SmartItemClickEventArgs e)
      {
         data.Emitter.TrailUVType = (EmitterData.EmitterTrailUVType)e.Item.Tag;
      }

      private void smartOptionList5_ItemClick(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         data.Emitter.BeamAlignmentType = (EmitterData.EmitterBeamAlignmentType)e.Item.Tag;
         updateEnableStates();
      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.BeamColorByLength = (bool)checkBox2.Checked;
      }

      private void checkBox6_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.BeamOpacityByLength = (bool)checkBox6.Checked;
      }

      private void checkBox8_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.BeamIntensityByLength = (bool)checkBox8.Checked;
      }

      private void fileBrowseControl1_ValueChanged(object sender, EventArgs e)
      {
         data.Emitter.PFXFilePath = fileBrowseControl1.FileName;
      }

      private void checkBox9_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.CollisionDetectionTerrain = (bool)checkBox9.Checked;
         updateEnableStates();
      }

      private void checkBox10_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.SortParticles = (bool)checkBox10.Checked;         
      }

      private void checkBox11_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.FillOptimized = (bool)checkBox11.Checked;
      }

      private void floatSliderEdit1_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         data.Emitter.TerrainDecalTesselation = (double) floatSliderEdit1.Value;
      }

      private void checkBox12_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.SoftParticles = (bool)checkBox12.Checked;
      }

      private void floatSliderEdit2_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         data.Emitter.SoftParticleFadeRange = (double)floatSliderEdit2.Value;
      }

      private void checkBox13_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.LightBuffer = (bool)checkBox13.Checked;
         updateEnableStates();
      }

      private void checkBox14_CheckedChanged(object sender, EventArgs e)
      {
         data.Emitter.KillImmediatelyOnRelease = (bool)checkBox14.Checked;         
      }      
   }
}
