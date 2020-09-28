using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using Xceed;

using Xceed.Chart.GraphicsCore;
using Xceed.Chart.Standard;
using Xceed.Chart;
using Xceed.Chart.Core;
using Xceed.Chart.Utilities;

namespace EditorCore
{
   public partial class VectorProgressionControl : UserControl
   {
      enum StageType
      {
         eXStage = 0,
         eYStage,
         eZStage,
         eStageTotal
      };
            
      private VectorProgression data;
      private bool bInitialized = false;

      public VectorProgressionControl()
      {
         InitializeComponent();
      }

      public void setData(VectorProgression v, float minY, float maxY, float defaultValue0, float defaultValue1, bool enableLooping)
      {
         bInitialized = false;
         data = v;
         initControl(scalarProgressionControl1, v.XProgression, Color.Red, Color.Yellow, "X Progression", minY, maxY, defaultValue0, defaultValue1, enableLooping);
         initControl(scalarProgressionControl2, v.YProgression, Color.Green, Color.LimeGreen, "Y Progression", minY, maxY, defaultValue0, defaultValue1, enableLooping);
         initControl(scalarProgressionControl3, v.ZProgression, Color.Blue, Color.Cyan, "Z Progression", minY, maxY, defaultValue0, defaultValue1, enableLooping);
         bInitialized = true;
      }

      public void initControl(ScalarProgressionControl control, FloatProgression p, Color startColor, Color endColor, string name, double AxisMinY, double AxisMaxY, double default0, double default1, bool enableLooping)
      {
         control.AxisMinY = AxisMinY;
         control.AxisMaxY = AxisMaxY;
         control.DefaultValue0 = default0;
         control.DefaultValue1 = default1;
         control.ChartStartColor = startColor;
         control.ChartEndColor = endColor;
         control.ProgressionName = name;
         control.LoopControl = enableLooping;
         control.setData(p);
      }

      public void EnableXProgression(bool value)
      {
         scalarProgressionControl1.Enabled = value;
      }

      public void EnableYProgression(bool value)
      {
         scalarProgressionControl2.Enabled = value;
      }

      public void EnableZProgression(bool value)
      {
         scalarProgressionControl3.Enabled = value;
      }
      
      /*
      private Xceed.Chart.Core.Chart m_XChart;
      private Xceed.Chart.Core.Chart m_YChart;
      private Xceed.Chart.Core.Chart m_ZChart;
      private AreaSeries m_XLine;
      private AreaSeries m_YLine;
      private AreaSeries m_ZLine;
      private int mSelectedPointX = 0;
      private int mSelectedPointY = 0;
      private int mSelectedPointZ = 0;

      [Browsable(false)]
      private double mMinX;

      [Browsable(false)]
      private double mMaxX;

      [Browsable(false)]
      private double mMinY;

      [Browsable(false)]
      private double mMaxY;

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      private VectorProgression data;
      private bool bInitialized = false;

      public VectorProgressionControl()
      {
         InitializeComponent();
         m_XChart = chartControl1.Charts[0];
         m_YChart = chartControl2.Charts[0];
         m_ZChart = chartControl3.Charts[0];

         m_XLine = (AreaSeries)m_XChart.Series[0];
         m_YLine = (AreaSeries)m_YChart.Series[0];
         m_ZLine = (AreaSeries)m_ZChart.Series[0];

         initSeries(m_XLine);
         initSeries(m_YLine);
         initSeries(m_ZLine);
                  
         mMinX =  0;
         mMaxX =  100;
         mMinY = -100;
         mMaxY =  100;

         setAxisMinMax(mMinX, mMaxX, mMinY, mMaxY, chartControl1.Charts[0]);
         setAxisMinMax(mMinX, mMaxX, mMinY, mMaxY, chartControl2.Charts[0]);
         setAxisMinMax(mMinX, mMaxX, mMinY, mMaxY, chartControl3.Charts[0]);
      }
      private void initSeries(AreaSeries line)
      {
         line.XValues.Clear();
         line.Values.Clear();
         line.UseXValues = true;
         line.Values.ValueFormatting.Format = ValueFormat.CustomNumber;
         line.Values.ValueFormatting.CustomFormat = "0.00";
         line.XValues.ValueFormatting.Format = ValueFormat.CustomNumber;
         line.XValues.ValueFormatting.CustomFormat = "0.00";
      }

      public void setData(VectorProgression p)
      {
         bInitialized = false;
         data = p;
         
         if (data.XStages.Count == 0)
         {
            //-- by default we always have two end points that cannot be deleted
            data.addXStage(100, 0, 0);
            data.addXStage(-100, 0, 100);
         }

         if (data.YStages.Count == 0)
         {
            //-- by default we always have two end points that cannot be deleted
            data.addYStage(100, 0, 0);
            data.addYStage(-100, 0, 100);
         }

         if (data.ZStages.Count == 0)
         {
            //-- by default we always have two end points that cannot be deleted
            data.addZStage(100, 0, 0);
            data.addZStage(-100, 0, 100);
         }

         getModifiedData();
         bInitialized = true;
         refreshListBox();
         refreshTextBoxes();
         refreshGraphs();
         chartControl1.Refresh();
         chartControl2.Refresh();
         chartControl3.Refresh();
         updateEnableState();
      }

      public double AxisMinX
      {
         get { return this.mMinX; }
         set { this.mMinX = value; }
      }

      public double AxisMaxX
      {
         get { return this.mMaxX; }
         set { this.mMaxX = value; }
      }

      public double AxisMinY
      {
         get { return this.mMinY; }
         set { this.mMinY = value; }
      }

      public double AxisMaxY
      {
         get { return this.mMaxY; }
         set { this.mMaxY = value; }
      }

      private void setAxisMinMax(double minX, double maxX, double minY, double maxY, Chart chart)
      {         
         Axis primaryX = chart.Axis(StandardAxis.PrimaryX);
         primaryX.NumericScale.AutoMin = false;
         primaryX.NumericScale.AutoMax = false;
         primaryX.NumericScale.Min = AxisMinX;
         primaryX.NumericScale.Max = AxisMaxX;

         Axis primaryY = chart.Axis(StandardAxis.PrimaryY);
         primaryY.NumericScale.AutoMin = false;
         primaryY.NumericScale.AutoMax = false;
         primaryY.NumericScale.Min = AxisMinY;
         primaryY.NumericScale.Max = AxisMaxY;
      }

      private void refreshTextBoxes()
      {
         numericUpDown3.Value = (decimal)Math.Round((decimal)data.CycleTime, 2, MidpointRounding.ToEven);
         numericUpDown4.Value = (decimal)Math.Round((decimal)data.CycleVariance, 2, MidpointRounding.ToEven);

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= data.XStages.Count)
         {
            numericUpDown1.Value = 0;
            numericUpDown2.Value = 0;
            numericUpDown5.Value = 0;
            return;
         }

         FloatProgressionStage stage = data.XStages[index];
         numericUpDown1.Value = (decimal)Math.Round((decimal)stage.Value, 2, MidpointRounding.ToEven);
         numericUpDown2.Value = (decimal)Math.Round((decimal)stage.Alpha, 2, MidpointRounding.ToEven);
         numericUpDown5.Value = (decimal)Math.Round((decimal)stage.ValueVariance, 2, MidpointRounding.ToEven);

         
      }

      private void refreshListBox(StageType type)
      {
         ListBox listbox = null;
         List<FloatProgressionStage> stages = null;
         if (type == StageType.eXStage)
         {
            listbox = listBox1;
            stages = data.XStages;
         }
         else if (type == StageType.eYStage)
         {
            listbox = listBox2;
            stages = data.YStages;
         }
         else if (type == StageType.eZStage)
         {
            listBox = listBox3;
            stages = data.ZStages;
         }
         else
            return;

         int index = listbox.SelectedIndex;
         listBox1.Items.Clear();
         for (int i = 0; i < stages.Count; i++)
         {
            listbox.Items.Add(stages[i].ToString());
         }

         if (index < 0 || index >= stages.Count)
            listbox.SelectedIndex = 0;
         else
            listbox.SelectedIndex = index;
      }

      private void refreshGraphs()
      {
         if (!bInitialized)
            return;

         refreshGraph(m_XLine, chartControl1, data.XStages);
         refreshGraph(m_YLine, chartControl2, data.YStages);
         refreshGraph(m_ZLine, chartControl3, data.ZStages);
      }

      private void refreshGraph(AreaSeries line, ChartControl control, List<FloatProgressionStage> stages)
      {
         if (!bInitialized)
            return;

         //-- Now Add the Points
         line.XValues.Clear();
         line.Values.Clear();

         for (int i = 0; i < stages.Count; i++)
         {
            FloatProgressionStage stage = stages[i];

            //- First add the start point at the current location
            line.AddXY(stage.Value, stage.Alpha);
         }

         control.Refresh();
      }

      private void setModifiedData()
      {
         if (!bInitialized)
            return;

         data.CycleTime = (double)numericUpDown3.Value;
         data.CycleVariance = (double)numericUpDown4.Value;

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= data.XStages.Count)
            return;

         data.XStages[index].Alpha = (double)numericUpDown2.Value;
         data.XStages[index].Value = (double)numericUpDown1.Value;
         data.XStages[index].ValueVariance = (double)numericUpDown5.Value;

         refreshListBox();
      }

      private void getModifiedData()
      {
         checkBox1.Checked = data.Loop;
         numericUpDown3.Value = (decimal)Math.Round((decimal)data.CycleTime, 2, MidpointRounding.ToEven);
         numericUpDown4.Value = (decimal)Math.Round((decimal)data.CycleVariance, 2, MidpointRounding.ToEven);

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= data.XStages.Count)
            return;

         numericUpDown1.Value = (decimal)Math.Round((decimal)data.XStages[index].Value, 2, MidpointRounding.ToEven);
         numericUpDown2.Value = (decimal)Math.Round((decimal)data.XStages[index].Alpha, 2, MidpointRounding.ToEven);
         numericUpDown5.Value = (decimal)Math.Round((decimal)data.XStages[index].ValueVariance, 2, MidpointRounding.ToEven);
      }

      private void addStage(StageType type)
      {
         ListBox listbox = null;
         List<FloatProgressionStage> stages = null;
         if (type == StageType.eXStage)
         {
            listbox = listBox1;
            stages = data.XStages;
         }
         else if (type == StageType.eYStage)
         {
            listbox = listBox2;
            stages = data.YStages;
         }
         else if (type == StageType.eZStage)
         {
            listbox = listBox3;
            stages = data.ZStages;
         }

         if (listbox == null || stages == null)
            return;

         int selectedIndex = listbox.SelectedIndex;
         int index = -1;
         int nextIndex = -1;
         if (selectedIndex < 0 || selectedIndex >= stages.Count - 1)
         {
            nextIndex = stages.Count - 1;
            index = stages.Count - 2;
         }
         else
         {
            index = selectedIndex;
            nextIndex = index + 1;
         }

         if (index < 0 || nextIndex < 0 || index >= stages.Count || nextIndex >= stages.Count)
            return;

         double newAlpha = (stages[index].Alpha + stages[nextIndex].Alpha) * 0.5f;
         double newValue = (stages[index].Value + stages[nextIndex].Value) * 0.5f;

         if (type == StageType.eXStage)
            data.insertXStage(newValue, 0, newAlpha);
         else if (type == StageType.eYStage)
            data.insertYStage(newValue, 0, newAlpha);
         else if (type == StageType.eZStage)
            data.insertZStage(newValue, 0, newAlpha);
      }
      //-- Add Button
      private void button1_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         addStage(StageType.eXStage);         
         refreshListBox(StageType.eXStage);

         //-- refresh graph
         refreshGraphs();
      }

      //-- Delete
      private void deleteStage(StageType type, List<FloatProgressionStage> stages, ListBox listbox)
      {
         ListBox listbox = null;
         List<FloatProgressionStage> stages = null;
         int index = -1;
         if (type == StageType.eXStage)
         {
            listbox = listBox1;
            index = listbox.SelectedIndex;
            stages = data.XStages;
            data.deleteXStage(index);
            
         }
         else if (type == StageType.eYStage)
         {
            listbox = listBox2;
            index = listbox.SelectedIndex;
            stages = data.YStages;
            data.deleteYStage(index);
         }
         else if (type == StageType.eZStage)
         {
            listbox = listBox3;
            index = listbox.SelectedIndex;
            stages = data.Y
         }

         refreshListBox(type);

         if (data.XStages.Count > 0)
         {
            index -= 1; // data.XStages.Count - 1;
            if (index < 0)
               index = 0;

            listbox.SelectedIndex = index;
         }
      }
      private void button2_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         

         refreshGraphs();
      }

      //-- Move up
      private void button3_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         if (data.XStages.Count > 1)
         {
            int index = listBox1.SelectedIndex;

            //-- we are not allowed to move the endpoints up and down
            if (index == 0 || index == data.XStages.Count - 1)
               return;

            data.moveXStageUp(index);
            refreshListBox();


            index -= 1;
            if (index < 0)
               index = 0;
            listBox1.SelectedIndex = index;
            refreshGraphs();
         }

      }

      //-- Move Down
      private void button4_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         if (data.XStages.Count > 1)
         {
            int index = listBox1.SelectedIndex;

            //-- we are not allowed to move the end points
            if (index == 0 || index == data.XStages.Count - 1)
               return;

            data.moveXStageDown(index);
            refreshListBox();

            index += 1;
            if (index >= data.XStages.Count)
               index = data.XStages.Count - 1;
            if (index < 0)
               index = 0;
            listBox1.SelectedIndex = index;
         }
         refreshGraphs();
      }

      private void updateEnableState()
      {
         numericUpDown3.Enabled = checkBox1.Checked;
         numericUpDown4.Enabled = checkBox1.Checked;
      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         updateEnableState();
      }

      private void listBox1_SelectedIndexChanged(object sender, EventArgs e)
      {         
         refreshTextBoxes();
      }

      private void VectorProgressionControl_Paint(object sender, PaintEventArgs e)
      {

         refreshGraphs();
      }

      private void chartControl1_MouseDown(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         //-- unselect by default
         mSelectedPointX = -1;

         HitTestResult hitTest = chartControl1.HitTest(e.X, e.Y);
         mSelectedPointX = hitTest.DataPointIndex;

         if (mSelectedPointX < 0 || mSelectedPointX >= data.XStages.Count)
            mSelectedPointX = -1;

         listBox1.SelectedIndex = mSelectedPointX;
      }

      private void chartControl1_MouseUp(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         mSelectedPointX = -1;
         
         refreshListBox();
         refreshTextBoxes();
      }

      private void chartControl1_MouseMove(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         if (mSelectedPointX >= 0 && mSelectedPointX < data.XStages.Count)
         {
            Rectangle clientRect = chartControl1.ClientRectangle;
            FloatProgressionStage stage = data.XStages[mSelectedPointX];

            double widthAlpha = (((double)e.X) / (double)clientRect.Width);
            double heightAlpha = (((double)clientRect.Height - e.Y) / (double)clientRect.Height);
            double translatedX = lerp(mMinX, mMaxX, widthAlpha);
            double translatedY = lerp(mMinY, mMaxY, heightAlpha);
            //double translatedX = 100.0f * (((double)e.X) / (double)clientRect.Width);
            //ouble translatedY = 100.0f * (((double)clientRect.Height - e.Y) / (double)clientRect.Height);

            //-- Clamp the values within the range
            stage.Alpha = System.Math.Min(System.Math.Max(mMinX, translatedX), mMaxX);
            stage.Value = System.Math.Min(System.Math.Max(mMinY, translatedY), mMaxY);

            //-- now clamp the x according to their neighbors
            int leftNeighborIndex = mSelectedPointX - 1;
            if (leftNeighborIndex >= 0 && leftNeighborIndex < data.XStages.Count)
            {
               double neighborAlpha = data.XStages[leftNeighborIndex].Alpha;
               stage.Alpha = System.Math.Max(neighborAlpha, stage.Alpha);
            }

            int rightNeighborIndex = mSelectedPointX + 1;
            if (rightNeighborIndex >= 0 && rightNeighborIndex < data.XStages.Count)
            {
               double neighborAlpha = data.XStages[rightNeighborIndex].Alpha;
               stage.Alpha = System.Math.Min(neighborAlpha, stage.Alpha);
            }

            //-- clamp the end points -- they can only affect their Value but not their alpha
            if (mSelectedPointX == 0)
               stage.Alpha = mMinX;
            else if (mSelectedPointX == data.XStages.Count - 1)
               stage.Alpha = mMaxX;

            stage.Alpha = System.Math.Round(stage.Alpha, 2);
            stage.Value = System.Math.Round(stage.Value, 2);

            refreshGraphs();
            refreshTextBoxes();
         }
      }

      private void chartControl1_MouseLeave(object sender, EventArgs e)
      {
         mSelectedPointX = -1;
      }

      private void chartControl1_MouseEnter(object sender, EventArgs e)
      {
         //-- unselect by default
         mSelectedPointX = -1;
      }

      public double lerp(double v1, double v2, double alpha)
      {
         double result = (v1 * (1.0f - alpha)) + (v2 * alpha);
         return result;
      }

      private void chartControl1_MouseDoubleClick(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         //-- unselect 
         mSelectedPointX = -1;

         handleMouseDoubleClick(chartControl1, data.XStages, e);
         

      }
      private void handleMouseDoubleClick(ChartControl chartControl, List<FloatProgressionStage> stages, MouseEventArgs e)
      {
         Rectangle clientRect = chartControl.ClientRectangle;
         double widthAlpha = (((double)e.X) / (double)clientRect.Width);
         double heightAlpha = (((double)clientRect.Height - e.Y) / (double)clientRect.Height);
         double translatedX = lerp(mMinX, mMaxX, widthAlpha);
         double translatedY = lerp(mMinY, mMaxY, heightAlpha);

         for (int i = 0; i < data.XStages.Count; i++)
         {
            FloatProgressionStage stage = stages[i];
            if (translatedX < stage.Alpha)
            {
               //-- now clamp the x according to their neighbors
               int leftNeighborIndex = i - 1;
               if (leftNeighborIndex >= 0 && leftNeighborIndex < stages.Count)
               {
                  double neighborAlpha = stages[leftNeighborIndex].Alpha;
                  translatedX = System.Math.Max(neighborAlpha, translatedX);
               }

               int rightNeighborIndex = i;
               if (rightNeighborIndex >= 0 && rightNeighborIndex < stages.Count)
               {
                  double neighborAlpha = stages[rightNeighborIndex].Alpha;
                  translatedX = System.Math.Min(neighborAlpha, translatedX);
               }
               //-- clamp Y
               translatedY = System.Math.Min(System.Math.Max(mMinY, translatedY), mMaxY);


               translatedX = System.Math.Round(translatedX, 2);
               translatedY = System.Math.Round(translatedY, 2);

               data.insertXStage(i, translatedY, 0, translatedX);

               refreshGraphs();

               break;
            }
         }
      }

      private void numericUpDown3_ValueChanged(object sender, EventArgs e)
      {
         data.CycleTime = (double)numericUpDown3.Value;
      }
      
      private void numericUpDown4_ValueChanged(object sender, EventArgs e)
      {
         data.CycleVariance = (double) numericUpDown4.Value;
      }
      
      private void button5_Click(object sender, EventArgs e)
      {
         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= data.XStages.Count)
            return;
         
         data.XStages[index].Value = (double)numericUpDown1.Value;
         data.XStages[index].ValueVariance = (double)numericUpDown5.Value;

         if (index > 0 && index < data.XStages.Count - 1)
            data.XStages[index].Alpha = (double)numericUpDown2.Value;

         refreshListBox();
         refreshGraphs();
         chartControl1.Refresh();
      }
       */
   }
}
