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

using Xceed.Chart.GraphicsCore;
using Xceed.Chart.Standard;
using Xceed.Chart;
using Xceed.Chart.Core;
using Xceed.Chart.Utilities;

namespace EditorCore
{
   public partial class ScalarProgressionControl : UserControl
   {
      private Xceed.Chart.Core.Chart m_Chart;
      private AreaSeries m_Line;
      private int m_SelectedPoint = 0;
      private Color mChartGradientStartColor;
      private Color mChartGradientEndColor;
      private string mProgressionName;

      [Browsable(false)]
      private double mMinX;

      [Browsable(false)]
      private double mMaxX;

      [Browsable(false)]
      private double mMinY;

      [Browsable(false)]
      private double mMaxY;

      [Browsable(false)]
      private double mDefaultValue0;

      [Browsable(false)]
      private double mDefaultValue1;

      [Browsable(false)]
      private bool mLoopControl = false;

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      private FloatProgression data;
      private bool bInitialized = false;

      public ScalarProgressionControl()
      {
         InitializeComponent();
         m_Chart = chartControl1.Charts[0];

         m_Line = (AreaSeries)m_Chart.Series[0];
         m_Line.XValues.Clear();
         m_Line.Values.Clear();
         m_Line.UseXValues = true;         
         m_Line.Values.ValueFormatting.Format = ValueFormat.CustomNumber;
         m_Line.Values.ValueFormatting.CustomFormat = "0.00";
         m_Line.XValues.ValueFormatting.Format = ValueFormat.CustomNumber;
         m_Line.XValues.ValueFormatting.CustomFormat = "0.00";

         mChartGradientStartColor = Color.Red;
         mChartGradientEndColor = Color.Yellow;

         m_Line.AreaFillEffect.BeginColor = mChartGradientStartColor;
         m_Line.AreaFillEffect.EndColor = mChartGradientEndColor;

         m_Line.DataLabels.Mode = DataLabelsMode.None;

         mMinX =  0;
         mMaxX =  100;
         mMinY = -100;
         mMaxY =  100;
         mDefaultValue0 = mMinY;
         mDefaultValue1 = mMaxY;

         setAxisMinMax(mMinX, mMaxX, mMinY, mMaxY);

         enableLoopControl(LoopControl);
      }
      public void enableLoopControl(bool enable)
      {
         checkBox1.Enabled = enable;
         label5.Enabled = enable;
         numericUpDown3.Enabled = enable;

         checkBox1.Visible = enable;
         label5.Visible = enable;
         numericUpDown3.Visible = enable;
      }

      public void setData(FloatProgression p)
      {
         bInitialized = false;         
         data = p;

         m_Line.AreaFillEffect.BeginColor = ChartStartColor;
         m_Line.AreaFillEffect.EndColor = ChartEndColor;

         setAxisMinMax(mMinX, mMaxX, mMinY, mMaxY);
         groupBox1.Text = ProgressionName;

         if (data.Stages.Count == 0)
         {
            //-- by default we always have two end points that cannot be deleted
            data.addStage(mDefaultValue0, 0, 0);
            data.addStage(mDefaultValue1, 0, 100);
         }

         //--normalize the data
         for (int i = 0; i < data.Stages.Count; ++i )
         {
            if (data.Stages[i].Value > mMaxY)
               data.Stages[i].Value = mMaxY;
            else if (data.Stages[i].Value < mMinY)
               data.Stages[i].Value = mMinY;            
         }

         getModifiedData();
         bInitialized = true;
         refreshListBox();
         refreshTextBoxes();
         refreshGraph();
         chartControl1.Refresh();
         enableLoopControl(mLoopControl);
         updateEnableState();
         
      }

      public bool LoopControl
      {
         get { return mLoopControl;   }
         set { mLoopControl = value;  }
      }

      public Color ChartStartColor
      {
         get { return mChartGradientStartColor; }
         set { mChartGradientStartColor = value; }
      }

      public Color ChartEndColor
      {
         get { return mChartGradientEndColor; }
         set { mChartGradientEndColor = value; }
      }

      public string ProgressionName
      {
         get { return mProgressionName; }
         set { mProgressionName = value; }
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

      public double DefaultValue0
      {
         get { return mDefaultValue0;  }
         set { mDefaultValue0 = value; }
      }

      public double DefaultValue1
      {
         get { return mDefaultValue1;  }
         set { mDefaultValue1 = value; }
      }

      private void setAxisMinMax(double minX, double maxX, double minY, double maxY)
      {
         Axis primaryX = chartControl1.Charts[0].Axis(StandardAxis.PrimaryX);
         primaryX.NumericScale.AutoMin = false;
         primaryX.NumericScale.AutoMax = false;
         primaryX.NumericScale.Min = AxisMinX;
         primaryX.NumericScale.Max = AxisMaxX;

         Axis primaryY = chartControl1.Charts[0].Axis(StandardAxis.PrimaryY);
         primaryY.NumericScale.AutoMin = false;
         primaryY.NumericScale.AutoMax = false;
         primaryY.NumericScale.Min = AxisMinY;
         primaryY.NumericScale.Max = AxisMaxY;

         numericUpDown1.Minimum = (decimal)Math.Round((decimal)AxisMinY, 2, MidpointRounding.ToEven);
         numericUpDown1.Maximum = (decimal)Math.Round((decimal)AxisMaxY, 2, MidpointRounding.ToEven);

         chartControl1.Refresh();
      }

      private void refreshTextBoxes()
      {
         numericUpDown3.Value = (decimal)Math.Round((decimal)data.Cycles, 2, MidpointRounding.ToEven);         

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= data.Stages.Count)
         {
            numericUpDown1.Value = 0;
            numericUpDown2.Value = 0;
            numericUpDown5.Value = 0;
            return;
         }

         FloatProgressionStage stage = data.Stages[index];
         numericUpDown1.Value = (decimal)Math.Round((decimal)stage.Value, 2, MidpointRounding.ToEven);
         numericUpDown2.Value = (decimal)Math.Round((decimal)stage.Alpha, 2, MidpointRounding.ToEven);
         numericUpDown5.Value = (decimal)Math.Round((decimal)stage.ValueVariance, 2, MidpointRounding.ToEven);

         
      }

      private void refreshListBox()
      {
         int index = listBox1.SelectedIndex;
         listBox1.Items.Clear();
         for (int i = 0; i < data.Stages.Count; i++)
         {
            listBox1.Items.Add(data.Stages[i].ToString());
         }

         if (data.Stages.Count > 0)
         {
            if (index < 0 || index >= data.Stages.Count)
               listBox1.SelectedIndex = 0;
            else
               listBox1.SelectedIndex = index;
         }
      }

      private void refreshGraph()
      {
         if (!bInitialized)
            return;

         //-- Now Add the Points
         m_Line.XValues.Clear();
         m_Line.Values.Clear();

         for (int i = 0; i < data.Stages.Count; i++)
         {
            FloatProgressionStage stage = data.Stages[i];

            //- First add the start point at the current location
            m_Line.AddXY(stage.Value, stage.Alpha);
         }

         if (chartControl1.Enabled)
         {
            m_Line.AreaFillEffect.BeginColor = ChartStartColor;
            m_Line.AreaFillEffect.EndColor = ChartEndColor;
         }
         else
         {
            m_Line.AreaFillEffect.BeginColor = Color.DarkGray;
            m_Line.AreaFillEffect.EndColor = Color.DarkGray;
         }

         chartControl1.Refresh();
      }

      private void setModifiedData()
      {
         if (!bInitialized)
            return;

         data.Cycles = (double)numericUpDown3.Value;

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= data.Stages.Count)
            return;

         data.Stages[index].Alpha = (double)numericUpDown2.Value;
         data.Stages[index].Value = (double)numericUpDown1.Value;
         data.Stages[index].ValueVariance = (double)numericUpDown5.Value;

         refreshListBox();
      }

      private void getModifiedData()
      {
         checkBox1.Checked = data.Loop;
         numericUpDown3.Value = (decimal)Math.Round((decimal)data.Cycles, 2, MidpointRounding.ToEven);         

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= data.Stages.Count)
            return;

         numericUpDown1.Value = (decimal)Math.Round((decimal)data.Stages[index].Value, 2, MidpointRounding.ToEven);
         numericUpDown2.Value = (decimal)Math.Round((decimal)data.Stages[index].Alpha, 2, MidpointRounding.ToEven);
         numericUpDown5.Value = (decimal)Math.Round((decimal)data.Stages[index].ValueVariance, 2, MidpointRounding.ToEven);
      }

      //-- Add
      private void button1_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         int selectedIndex = listBox1.SelectedIndex;
         int index = -1;
         int nextIndex = -1;
         if (selectedIndex < 0 || selectedIndex >= data.Stages.Count - 1)
         {
            nextIndex = data.Stages.Count - 1;
            index = data.Stages.Count - 2;
         }
         else
         {
            index = selectedIndex;
            nextIndex = index + 1;
         }

         if (index < 0 || nextIndex < 0 || index >= data.Stages.Count || nextIndex >= data.Stages.Count)
            return;

         double newAlpha = (data.Stages[index].Alpha + data.Stages[nextIndex].Alpha) * 0.5f;
         double newValue = (data.Stages[index].Value + data.Stages[nextIndex].Value) * 0.5f;


         data.insertStage(newValue, 0, newAlpha);
         refreshListBox();

         //-- refresh graph
         refreshGraph();
      }

      //-- Delete
      private void button2_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         int index = listBox1.SelectedIndex;
         data.deleteStage(index);
         refreshListBox();

         if (data.Stages.Count > 0)
         {
            index -= 1; // data.Stages.Count - 1;
            if (index < 0)
               index = 0;

            listBox1.SelectedIndex = index;
         }

         refreshGraph();
      }

      //-- Move up
      private void button3_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         if (data.Stages.Count > 1)
         {
            int index = listBox1.SelectedIndex;

            //-- we are not allowed to move the endpoints up and down
            if (index == 0 || index == data.Stages.Count - 1)
               return;

            data.moveStageUp(index);
            refreshListBox();


            index -= 1;
            if (index < 0)
               index = 0;
            listBox1.SelectedIndex = index;
            refreshGraph();
         }

      }

      //-- Move Down
      private void button4_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         if (data.Stages.Count > 1)
         {
            int index = listBox1.SelectedIndex;

            //-- we are not allowed to move the end points
            if (index == 0 || index == data.Stages.Count - 1)
               return;

            data.moveStageDown(index);
            refreshListBox();

            index += 1;
            if (index >= data.Stages.Count)
               index = data.Stages.Count - 1;
            if (index < 0)
               index = 0;
            listBox1.SelectedIndex = index;
         }
         refreshGraph();
      }

      private void updateEnableState()
      {
         numericUpDown3.Enabled = checkBox1.Checked;
      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         data.Loop = checkBox1.Checked;
         updateEnableState();
      }

      private void listBox1_SelectedIndexChanged(object sender, EventArgs e)
      {         
         refreshTextBoxes();
      }

      private void ScalarProgressionControl_Paint(object sender, PaintEventArgs e)
      {

         refreshGraph();
      }

      private void chartControl1_MouseDown(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         if (e.Button == MouseButtons.Left)
         {
            //-- unselect by default
            m_SelectedPoint = -1;
            
            double fX = 0;
            double fY = 0;
            convertMouseXYtoClientXY(e.X, e.Y, ref fX, ref fY);

            double offset = 2.5f;
            double hitBoxMinX = Math.Max(0.0f,   fX-offset);
            double hitBoxMaxX = Math.Min(100.0f, fX+offset);
            double hitBoxMinY = Math.Max(mMinY, fY - offset);
            double hitBoxMaxY = Math.Min(mMaxY, fY+offset);

            double yRange = mMaxY - mMinY;

            for (int i = 0; i < data.Stages.Count; ++i)
            {
               double valueX = data.Stages[i].Alpha;               
               double valueY = data.Stages[i].Value;

               if ((valueX >= hitBoxMinX) && (valueX <= hitBoxMaxX) &&
                   (valueY >= hitBoxMinY) && (valueY <= hitBoxMaxY))
               {
                  m_SelectedPoint = i;
                  break;
               }
            }
            
            if (m_SelectedPoint < 0 || m_SelectedPoint >= data.Stages.Count)
               m_SelectedPoint = -1;
            else
               listBox1.SelectedIndex = m_SelectedPoint;
         }

         //-- initiate a drag drop if it isn't a point
         if (m_SelectedPoint == -1 && e.Button == MouseButtons.Right)
         {
            ScalarProgressionDragDropPacket packet = new ScalarProgressionDragDropPacket();
            packet.data = data;
            packet.sender = chartControl1;
            //chartControl1.DoDragDrop(data, DragDropEffects.Copy);
            chartControl1.DoDragDrop(packet, DragDropEffects.Copy);
         }
      }

      private void chartControl1_MouseUp(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         m_SelectedPoint = -1;
         
         refreshListBox();
         refreshTextBoxes();
      }

      private void convertMouseXYtoClientXY(int X, int Y, ref double translatedX, ref double translatedY)
      {
         Point mousePoint = new Point();
         mousePoint.X = X;
         mousePoint.Y = Y;

         Axis horzAxis  = m_Chart.Axis(StandardAxis.PrimaryX);
         Axis vertAxis  = m_Chart.Axis(StandardAxis.PrimaryY);
         Axis depthAxis = m_Chart.Axis(StandardAxis.Depth);

         Vector vecModelPoint = new Vector();

         m_Chart.MapClientPointToModelPlane(horzAxis, vertAxis, depthAxis, 0.0f, mousePoint, ref vecModelPoint);

         translatedX = horzAxis.ConvertModelToScaleCoordinate(vecModelPoint.x, true);
         translatedY = vertAxis.ConvertModelToScaleCoordinate(vecModelPoint.y, true);
      }

      private void chartControl1_MouseMove(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         if (m_SelectedPoint >= 0 && m_SelectedPoint < data.Stages.Count)
         {
            Rectangle clientRect = chartControl1.ClientRectangle;
            FloatProgressionStage stage = data.Stages[m_SelectedPoint];
            
            double fX = 0;
            double fY = 0;
            convertMouseXYtoClientXY(e.X, e.Y, ref fX, ref fY);

            double translatedX = fX;
            double translatedY = fY;            

            //-- Clamp the values within the range
            stage.Alpha = System.Math.Min(System.Math.Max(mMinX, translatedX), mMaxX);
            stage.Value = System.Math.Min(System.Math.Max(mMinY, translatedY), mMaxY);

            //-- now clamp the x according to their neighbors
            int leftNeighborIndex = m_SelectedPoint - 1;
            if (leftNeighborIndex >= 0 && leftNeighborIndex < data.Stages.Count)
            {
               double neighborAlpha = data.Stages[leftNeighborIndex].Alpha;
               stage.Alpha = System.Math.Max(neighborAlpha, stage.Alpha);
            }

            int rightNeighborIndex = m_SelectedPoint + 1;
            if (rightNeighborIndex >= 0 && rightNeighborIndex < data.Stages.Count)
            {
               double neighborAlpha = data.Stages[rightNeighborIndex].Alpha;
               stage.Alpha = System.Math.Min(neighborAlpha, stage.Alpha);
            }

            //-- clamp the end points -- they can only affect their Value but not their alpha
            if (m_SelectedPoint == 0)
               stage.Alpha = mMinX;
            else if (m_SelectedPoint == data.Stages.Count - 1)
               stage.Alpha = mMaxX;

            stage.Alpha = System.Math.Round(stage.Alpha, 2);
            stage.Value = System.Math.Round(stage.Value, 2);

            refreshGraph();
            refreshTextBoxes();
         }
      }

      private void chartControl1_MouseLeave(object sender, EventArgs e)
      {
         m_SelectedPoint = -1;
      }

      private void chartControl1_MouseEnter(object sender, EventArgs e)
      {
         //-- unselect by default
         m_SelectedPoint = -1;
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

         if (e.Button != MouseButtons.Left)
            return;

         //-- unselect 
         m_SelectedPoint = -1;
         
         double fX = 0.0f;
         double fY = 0.0f;
         convertMouseXYtoClientXY(e.X, e.Y, ref fX, ref fY);

         double translatedX = fX;
         double translatedY = fY;         

         for (int i = 0; i < data.Stages.Count; i++)
         {
            FloatProgressionStage stage = data.Stages[i];
            if (translatedX < stage.Alpha)
            {
               //-- now clamp the x according to their neighbors
               int leftNeighborIndex = i - 1;
               if (leftNeighborIndex >= 0 && leftNeighborIndex < data.Stages.Count)
               {
                  double neighborAlpha = data.Stages[leftNeighborIndex].Alpha;
                  translatedX = System.Math.Max(neighborAlpha, translatedX);
               }

               int rightNeighborIndex = i;
               if (rightNeighborIndex >= 0 && rightNeighborIndex < data.Stages.Count)
               {
                  double neighborAlpha = data.Stages[rightNeighborIndex].Alpha;
                  translatedX = System.Math.Min(neighborAlpha, translatedX);
               }
               //-- clamp Y
               translatedY = System.Math.Min(System.Math.Max(mMinY, translatedY), mMaxY);

               translatedX = System.Math.Round(translatedX, 2);
               translatedY = System.Math.Round(translatedY, 2);

               data.insertStage(i, translatedY, 0, translatedX);

               refreshGraph();

               break;
            }
         }
      }

      private void numericUpDown3_ValueChanged(object sender, EventArgs e)
      {
         data.Cycles = (double)numericUpDown3.Value;
      }
                  
      private void button5_Click(object sender, EventArgs e)
      {
         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= data.Stages.Count)
            return;
         
         data.Stages[index].Value = (double)numericUpDown1.Value;
         data.Stages[index].ValueVariance = (double)numericUpDown5.Value;

         if (index > 0 && index < data.Stages.Count - 1)
            data.Stages[index].Alpha = (double)numericUpDown2.Value;

         refreshListBox();
         refreshGraph();
         chartControl1.Refresh();
      }

      // load progression
      private void button6_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();
         d.InitialDirectory = EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectFloatProgression;
         d.Filter = "FloatProgression (*.EFP)|*.EFP";
         if (d.ShowDialog() == DialogResult.OK)
         {
            EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectFloatProgression = Path.GetDirectoryName(d.FileName);
            LoadProgression(d.FileName);
         }
      }

      // save progression
      private void button7_Click(object sender, EventArgs e)
      {
         SaveFileDialog d = new SaveFileDialog();
         d.InitialDirectory = EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectFloatProgression;
         d.Filter = "FloatProgression (*.EFP)|*.EFP";
         if (d.ShowDialog() == DialogResult.OK)
         {
            EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectFloatProgression = Path.GetDirectoryName(d.FileName);
            SaveProgression(d.FileName);
         }
      }

      private void SaveProgression(string filename)
      {
         XmlSerializer s = new XmlSerializer(typeof(FloatProgression), new Type[] { });
         Stream st = File.Create(filename);
         s.Serialize(st, data);
         st.Close();

         //No need to xmb this
      }

      private void LoadProgression(string filename)
      {
         XmlSerializer s = new XmlSerializer(typeof(FloatProgression), new Type[] { });
         Stream st = File.OpenRead(filename);
         FloatProgression loadedProgression = new FloatProgression();
         loadedProgression = (FloatProgression)s.Deserialize(st);
         st.Close();

         data.Copy(loadedProgression);

         bInitialized = false;
         getModifiedData();
         bInitialized = true;
         refreshListBox();
         refreshTextBoxes();
         refreshGraph();
         chartControl1.Refresh();
         updateEnableState();
      }

      private void chartControl1_DragEnter(object sender, System.Windows.Forms.DragEventArgs e)
      {
         e.Effect = DragDropEffects.None;

         ScalarProgressionDragDropPacket packet = (ScalarProgressionDragDropPacket)e.Data.GetData(typeof(ScalarProgressionDragDropPacket));
         if (packet == null)
            return;

         ChartControl senderControl = (ChartControl)packet.sender;
         if (chartControl1.Handle == senderControl.Handle)
            return;
         
         if (e.Data.GetDataPresent(typeof(ScalarProgressionDragDropPacket)))
            e.Effect = DragDropEffects.Copy;
      }

      private void chartControl1_DragDrop(object sender, System.Windows.Forms.DragEventArgs e)
      {
         ScalarProgressionDragDropPacket packet = (ScalarProgressionDragDropPacket)e.Data.GetData(typeof(ScalarProgressionDragDropPacket));
         if (packet == null)
            return;

         ChartControl senderControl = (ChartControl)packet.sender;
         if (chartControl1.Handle == senderControl.Handle)
            return;

         FloatProgression newData = (FloatProgression)packet.data;
         if (newData == null)
            return;
        
         data.Copy(newData);

         bInitialized = false;
         getModifiedData();
         bInitialized = true;
         refreshListBox();
         refreshTextBoxes();
         refreshGraph();
         chartControl1.Refresh();
         updateEnableState();
      }

      private void chartControl1_DragLeave(object sender, EventArgs e)
      {        
         //chartControl1.DoDragDrop(data, DragDropEffects.Copy);
      }

      private void numericUpDown_Enter(object sender, EventArgs e)
      {
         NumericUpDown control = (NumericUpDown)sender;
         control.Select(0, control.Text.Length);
      }

      private void listBox1_EnabledChanged(object sender, EventArgs e)
      {
         Color lastColor = listBox1.BackColor;
         if (listBox1.Enabled)
            listBox1.BackColor = Color.White;
         else
            listBox1.BackColor = Color.DarkGray;

         if (lastColor != listBox1.BackColor)
            listBox1.Invalidate();
      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         if (checkBox2.Checked)
            m_Line.DataLabels.Mode = DataLabelsMode.Every;
         else
            m_Line.DataLabels.Mode = DataLabelsMode.None;
         chartControl1.Refresh();
      }   
   }

   public class ScalarProgressionDragDropPacket
   {
      public FloatProgression data;
      public object sender;
   };
}
