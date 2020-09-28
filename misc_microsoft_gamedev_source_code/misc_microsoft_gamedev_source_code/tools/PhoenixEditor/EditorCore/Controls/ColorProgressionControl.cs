using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Drawing.Drawing2D;
using System.IO;

using System.Xml;
using System.Xml.Serialization;

namespace EditorCore
{
   public partial class ColorProgressionControl : UserControl
   {
      public ColorProgressionControl()
      {
         InitializeComponent();
      }

      private int mSelectedPoint = -1;
      private ColorProgression data;
      private bool bInitialized = false;

      public void setData(ColorProgression e)
      {
         bInitialized = false;
         data = e;
         gradientControl1.setData(data.Stages);
         gradientControl1.Invalidate();
         getModifiedData();
         updateEnableStates();        
         bInitialized = true;

         if (gradientControl1.Gradient.Count <= 0)
         {
            gradientControl1.clearPoints();
            gradientControl1.addPoint(0.0f, Color.Black);
            gradientControl1.addPoint(1.0f, Color.White);
            gradientControl1.Invalidate();         
            refreshListBox();
            Invalidate();
         }
         else
         {
            refreshListBox();
            Invalidate();
         }
      }

      private void button1_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= gradientControl1.Gradient.Count)
            return;

         colorDialog1.FullOpen = true;
         DialogResult result = colorDialog1.ShowDialog();
         if (result == DialogResult.OK)
         {
            button1.BackColor = colorDialog1.Color;
            gradientControl1.Gradient[index].Color = colorDialog1.Color;
            refreshListBox();
            gradientControl1.Invalidate();
            Invalidate();
         }
      }

      private void listBox1_DrawItem(object sender, DrawItemEventArgs e)
      {
         if (!bInitialized)
            return;

         e.DrawFocusRectangle();
         int index = e.Index;
         if (index < 0 || index >= gradientControl1.Gradient.Count)
            return;

         int colorRectWidth = 40;
         int itemYAdditionalOffset = 4;
         GradientPoint item = gradientControl1.Gradient[index];
         //-- Draw the Color Rectangle 
         Rectangle rect = new Rectangle();
         rect.X = e.Bounds.X;
         rect.Y = e.Bounds.Y;
         rect.Width = colorRectWidth;
         rect.Height = e.Bounds.Height - itemYAdditionalOffset;


         //-- fill the rectangle 
         Brush colorBrush = new SolidBrush(item.Color);
         e.Graphics.FillRectangle(colorBrush, rect);
         colorBrush.Dispose();

         //-- Draw the border
         Pen pen = new Pen(Color.Gray, 2);
         pen.Alignment = PenAlignment.Inset;
         pen.DashStyle = DashStyle.Solid;
         pen.LineJoin = LineJoin.Round;
         e.Graphics.DrawRectangle(pen, rect);
         pen.Dispose();

         Brush brush = Brushes.Black;         
         if ((e.State & DrawItemState.Selected) == DrawItemState.Selected)
         {
            brush = Brushes.White;
         }
         
         //-- Draw the back ground
         Brush brushBackground = new SolidBrush(e.BackColor);

         if (listBox1.Enabled == false)
         {
            brush = Brushes.Gray;
            brushBackground = Brushes.DarkGray;
         }

         e.Graphics.FillRectangle(brushBackground, e.Bounds.X + 45, e.Bounds.Y, e.Bounds.Width - colorRectWidth, e.Bounds.Height - itemYAdditionalOffset);

         e.Graphics.DrawString(item.ToString(), e.Font, brush, e.Bounds.X + 45, e.Bounds.Y);
      }

      private void refreshListBox()
      {
         if (!bInitialized)
            return;

         int index = listBox1.SelectedIndex;
         listBox1.Items.Clear();
         for (int i = 0; i < gradientControl1.Gradient.Count; ++i)
         {
            listBox1.Items.Add(gradientControl1.Gradient[i]);
         }

         if (index < 0 || index >= gradientControl1.Gradient.Count)
            listBox1.SelectedIndex = 0;
         else
            listBox1.SelectedIndex = index;
      }

      private void updateEnableStates()
      {
         numericUpDown1.Enabled = winCheckBox1.Checked;         
      }

      private void getModifiedData()
      {
         winCheckBox1.Checked = data.Loop;
         numericUpDown1.Value = (decimal) data.Cycles;

      }

      private void setModifiedData()
      {
         if (!bInitialized)
            return;

         data.Loop = winCheckBox1.Checked;
         data.Cycles = (double) numericUpDown1.Value;
      }

      //-- Add
      private void button2_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         colorDialog1.FullOpen = true;
         DialogResult result = colorDialog1.ShowDialog();
         if (result == DialogResult.OK)
         {
            gradientControl1.addPoint(colorDialog1.Color);               
            refreshListBox();
            gradientControl1.Invalidate();
         }
      }

      //-- Delete
      private void button3_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= gradientControl1.Gradient.Count)
            return;

         if (gradientControl1.Gradient.Count <= 2)
         {
            MessageBox.Show(String.Format("A gradient must have at least 2 Gradient Points!"), "Warning!", MessageBoxButtons.OK);
            return;
         }

         gradientControl1.deletePoint(index);
         refreshListBox();
         gradientControl1.Invalidate();
      }

      private void listBox1_MeasureItem(object sender, MeasureItemEventArgs e)
      {
         if (!bInitialized)
            return;

         e.ItemHeight += 4; ;
      }

      private void listBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         winNumericTextBox2.Enabled = true;
         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= gradientControl1.Gradient.Count)
            return;

         button1.BackColor = gradientControl1.Gradient[index].Color;
         winNumericTextBox2.Value = gradientControl1.Gradient[index].Alpha;

         if (index == 0 || index == gradientControl1.Gradient.Count - 1)
            winNumericTextBox2.Enabled = false;
      }

      private void winNumericTextBox2_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= gradientControl1.Gradient.Count)
            return;

         gradientControl1.Gradient[index].Alpha = (double) winNumericTextBox2.Value;

         refreshListBox();
         gradientControl1.Invalidate();
      }

      //-- move up
      private void button4_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         int index = listBox1.SelectedIndex;
         if (index <= 0 || index >= gradientControl1.Gradient.Count)
            return;


         Color color = new Color();
         color = gradientControl1.Gradient[index - 1].Color;
         gradientControl1.Gradient[index - 1].Color = gradientControl1.Gradient[index].Color;
         gradientControl1.Gradient[index].Color = color;

         listBox1.SelectedIndex = index - 1;
         if (listBox1.SelectedIndex < 0)
            listBox1.SelectedIndex = 0;

         refreshListBox();
         gradientControl1.Invalidate();
      }

      //-- move down
      private void button5_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= gradientControl1.Gradient.Count -1)
            return;

         Color color = new Color();
         color = gradientControl1.Gradient[index + 1].Color;
         gradientControl1.Gradient[index + 1].Color = gradientControl1.Gradient[index].Color;
         gradientControl1.Gradient[index].Color = color;

         listBox1.SelectedIndex = index + 1;
         if (listBox1.SelectedIndex >= gradientControl1.Gradient.Count)
            listBox1.SelectedIndex = gradientControl1.Gradient.Count - 1; ;

         refreshListBox();
         gradientControl1.Invalidate();
      }

      private void gradientControl1_MouseDown(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         mSelectedPoint = -1;

         int selectionOffset = 5;
         for (int i = 1; i < gradientControl1.Gradient.Count -1; i++)
         {
            int pointX = (int) System.Math.Round(gradientControl1.Gradient[i].Alpha * (double) gradientControl1.ClientRectangle.Width);
            if (e.X >= pointX - selectionOffset && e.X <= pointX + selectionOffset)
               mSelectedPoint = i;
         }

         if (mSelectedPoint != -1)
            listBox1.SelectedIndex = mSelectedPoint;
      }

      private void gradientControl1_MouseMove(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         if (mSelectedPoint >= 0 && mSelectedPoint < gradientControl1.Gradient.Count - 1)
         {
            double translatedX = (double)e.X / (double)gradientControl1.ClientRectangle.Width;
            GradientPoint point = gradientControl1.Gradient[mSelectedPoint];
            point.Alpha = System.Math.Min(System.Math.Max(0, translatedX), 1.0f);

            //-- now clamp the x according to their neighbors
            int leftNeighborIndex = mSelectedPoint - 1;
            if (leftNeighborIndex >= 0 && leftNeighborIndex < gradientControl1.Gradient.Count)
            {
               double neighborAlpha = gradientControl1.Gradient[leftNeighborIndex].Alpha;
               point.Alpha = System.Math.Max(neighborAlpha, point.Alpha);
            }

            int rightNeighborIndex = mSelectedPoint + 1;
            if (rightNeighborIndex >= 0 && rightNeighborIndex < gradientControl1.Gradient.Count)
            {
               double neighborAlpha = gradientControl1.Gradient[rightNeighborIndex].Alpha;
               point.Alpha = System.Math.Min(neighborAlpha, point.Alpha);
            }

            point.Alpha = System.Math.Round(point.Alpha, 2);

            refreshListBox();
            gradientControl1.Invalidate();
         }
      }

      private void gradientControl1_MouseUp(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         mSelectedPoint = -1;
         refreshListBox();
         gradientControl1.Invalidate();
      }

      private void gradientControl1_MouseDoubleClick(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         //-- unselect 
         mSelectedPoint = -1;

         colorDialog1.FullOpen = true;
         DialogResult result = colorDialog1.ShowDialog();
         if (result != DialogResult.OK)
            return;

         Rectangle clientRect = gradientControl1.ClientRectangle;
         double translatedX = (double)e.X / (double)clientRect.Width;
         for (int i = 0; i < gradientControl1.Gradient.Count; i++)
         {
            GradientPoint point = gradientControl1.Gradient[i];
            if (translatedX < point.Alpha)
            {
               //-- now clamp the x according to their neighbors
               int leftNeighborIndex = i - 1;
               if (leftNeighborIndex >= 0 && leftNeighborIndex < gradientControl1.Gradient.Count)
               {
                  double neighborAlpha = gradientControl1.Gradient[leftNeighborIndex].Alpha;
                  translatedX = System.Math.Max(neighborAlpha, translatedX);
               }

               int rightNeighborIndex = i;
               if (rightNeighborIndex >= 0 && rightNeighborIndex < gradientControl1.Gradient.Count)
               {
                  double neighborAlpha = gradientControl1.Gradient[rightNeighborIndex].Alpha;
                  translatedX = System.Math.Min(neighborAlpha, translatedX);
               }

               translatedX = System.Math.Round(translatedX, 2);

               gradientControl1.insertPoint(i , translatedX, colorDialog1.Color);

               refreshListBox();
               gradientControl1.Invalidate();
               break;
            }
         }
      }

      private void listBox1_MouseDoubleClick(object sender, MouseEventArgs e)
      {
         if (!bInitialized)
            return;

         base.OnMouseDoubleClick(e);
         int index = listBox1.SelectedIndex;
         if (e.X >= 0 && e.X <= 40 && listBox1.SelectedIndex != -1)
         {
            colorDialog1.FullOpen = true;
            DialogResult result = colorDialog1.ShowDialog();
            if (result == DialogResult.OK)
            {
               gradientControl1.Gradient[index].Color = colorDialog1.Color;
               refreshListBox();
               gradientControl1.Invalidate();
               Invalidate();
            }
         }
      }

      private void winCheckBox1_CheckedChanged(object sender, EventArgs e)
      {
         data.Loop = winCheckBox1.Checked;
         updateEnableStates();         
      }

      private void numericUpDown1_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         setModifiedData();
      }

      //-- Load
      private void button6_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();
         d.InitialDirectory = EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectColorProgression;
         d.Filter = "ColorProgression (*.ECP)|*.ecp";
         if (d.ShowDialog() == DialogResult.OK)
         {
            EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectColorProgression = Path.GetDirectoryName(d.FileName);
            LoadProgression(d.FileName);
         }
      }

      //-- Save
      private void button7_Click(object sender, EventArgs e)
      {
         SaveFileDialog d = new SaveFileDialog();
         d.Filter = "ColorProgression (*.ECP)|*.ecp";
         d.InitialDirectory = EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectColorProgression;
         if (d.ShowDialog() == DialogResult.OK)
         {
            EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectColorProgression = Path.GetDirectoryName(d.FileName);
            SaveProgression(d.FileName);
         }
      }

      private void SaveProgression(string filename)
      {
         XmlSerializer s = new XmlSerializer(typeof(ColorProgression), new Type[] { });
         Stream st = File.Create(filename);
         s.Serialize(st, data);
         st.Close();

         //No need to xmb this
      }

      private void LoadProgression(string filename)
      {
         XmlSerializer s = new XmlSerializer(typeof(ColorProgression), new Type[] { });
         Stream st = File.OpenRead(filename);
         data = (ColorProgression)s.Deserialize(st);
         st.Close();

         bInitialized = false;
         gradientControl1.setData(data.Stages);
         getModifiedData();
         bInitialized = true;

         gradientControl1.Invalidate();
         refreshListBox();
         Invalidate();
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
   }
}
