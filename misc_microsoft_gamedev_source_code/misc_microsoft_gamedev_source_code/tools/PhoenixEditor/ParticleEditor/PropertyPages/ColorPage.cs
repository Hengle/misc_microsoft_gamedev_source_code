using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Drawing.Drawing2D;
using Xceed.SmartUI.Controls.OptionList;

namespace ParticleSystem
{
   public partial class ColorPage : UserControl
   {
      private ParticleEmitter data;
      bool bInitialized = false;
      public ColorPage()
      {
         InitializeComponent();

         //-- register event handlers
         smartOptionList1.ItemClick += new Xceed.SmartUI.SmartItemClickEventHandler(smartOptionList1_ItemClick);
         for (int i = 0; i < smartOptionList1.Items.Count; i++)
         {
            smartOptionList1.Items[i].Tag = new ColorData.ColorTypeEnum();
         }

         //-- set the corresponding radiobutton tags
         smartOptionList1.Items[0].Tag = ColorData.ColorTypeEnum.eSingleColor;
         smartOptionList1.Items[1].Tag = ColorData.ColorTypeEnum.eProgression;
         smartOptionList1.Items[2].Tag = ColorData.ColorTypeEnum.ePalletteColor;
      }

      public void setData(ParticleEmitter e)
      {
         bInitialized = false;
         data = e;
         colorProgressionControl1.setData(data.Color.ColorProgression);
         getModifiedData();
         bInitialized = true;         
         refreshPalletteListBox();
         updateEnableStates();
      }

      private void getModifiedData()
      {
         button1.BackColor = data.Color.Color;
         button4.BackColor = data.Color.ColorVertex1;
         button5.BackColor = data.Color.ColorVertex2;
         button6.BackColor = data.Color.ColorVertex3;
         button7.BackColor = data.Color.ColorVertex4;

         label1.Text = data.Color.Color.ToString();

         for (int i = 0; i < smartOptionList1.Items.Count; i++)
         {
            if (data.Color.Type == (ColorData.ColorTypeEnum)smartOptionList1.Items[i].Tag)
            {
               ((RadioButtonNode)smartOptionList1.Items[i]).Checked = true;
               break;
            }
         }

         checkBox1.Checked = data.Color.PlayerColor;
         floatSliderEdit1.Value = data.Color.PlayerColorIntensity;

         checkBox2.Checked = data.Color.SunColor;
         floatSliderEdit2.Value = data.Color.SunColorIntensity;
      }

      private void setModifiedData()
      {

      }

      private void refreshPalletteListBox()
      {
         if (!bInitialized)
            return;

         listBox1.Items.Clear();
         int index = listBox1.SelectedIndex;         
         for (int i = 0; i < data.Color.ColorPallette.Count; ++i)
         {
            listBox1.Items.Add(data.Color.ColorPallette[i]);
         }

         //-- if there is something in the list box see if we can select something intelligently
         if (listBox1.Items.Count > 0)
         {
            if (index < 0 || index >= data.Color.ColorPallette.Count)
               listBox1.SelectedIndex = 0;
            else
               listBox1.SelectedIndex = index;
         }
      }

      private void updateEnableStates()
      {
         groupBox2.Enabled = true;
         groupBox3.Enabled = true;
         colorProgressionControl1.Enabled = true;
         if (data.Color.Type == ColorData.ColorTypeEnum.eSingleColor)
         {
            colorProgressionControl1.Enabled = false;
            groupBox3.Enabled = false;
         }
         else if (data.Color.Type == ColorData.ColorTypeEnum.eProgression)
         {
            groupBox2.Enabled = false;
            groupBox3.Enabled = false;
         }
         else if (data.Color.Type == ColorData.ColorTypeEnum.ePalletteColor)
         {
            colorProgressionControl1.Enabled = false;
            groupBox2.Enabled = false;
         }
         colorProgressionControl1.Invalidate();

         floatSliderEdit1.Enabled = checkBox1.Checked;
         floatSliderEdit2.Enabled = checkBox2.Checked;
      }

      private void smartOptionList1_ItemClick(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         if (!bInitialized)
            return;

         data.Color.Type = (ColorData.ColorTypeEnum)e.Item.Tag;
         updateEnableStates();
      }

      //-- Color Button
      private void button1_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         colorDialog1.FullOpen = true;
         DialogResult result = colorDialog1.ShowDialog();
         if (result == DialogResult.OK)
         {
            button1.BackColor = colorDialog1.Color;
            label1.Text = colorDialog1.Color.ToString();
            data.Color.Color = colorDialog1.Color;
         }
      }

      private void buttonVertexColor1_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         colorDialog1.FullOpen = true;
         DialogResult result = colorDialog1.ShowDialog();
         if (result == DialogResult.OK)
         {            
            button4.BackColor = colorDialog1.Color;            
            data.Color.ColorVertex1 = colorDialog1.Color;
         }
      }

      private void buttonVertexColor2_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         colorDialog1.FullOpen = true;
         DialogResult result = colorDialog1.ShowDialog();
         if (result == DialogResult.OK)
         {
            button5.BackColor = colorDialog1.Color;
            data.Color.ColorVertex2 = colorDialog1.Color;
         }
      }

      private void buttonVertexColor3_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         colorDialog1.FullOpen = true;
         DialogResult result = colorDialog1.ShowDialog();
         if (result == DialogResult.OK)
         {
            button6.BackColor = colorDialog1.Color;
            data.Color.ColorVertex3 = colorDialog1.Color;
         }
      }

      private void buttonVertexColor4_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         colorDialog1.FullOpen = true;
         DialogResult result = colorDialog1.ShowDialog();
         if (result == DialogResult.OK)
         {
            button7.BackColor = colorDialog1.Color;
            data.Color.ColorVertex4 = colorDialog1.Color;
         }
      }

      private void listBox1_DrawItem(object sender, DrawItemEventArgs e)
      {
         if (!bInitialized)
            return;

         e.DrawFocusRectangle();
         int index = e.Index;
         if (index < 0 || index >= data.Color.ColorPallette.Count)
            return;

         int colorRectWidth = 40;
         int itemYAdditionalOffset = 4;
         PalletteColor color = data.Color.ColorPallette[index];
         //-- Draw the Color Rectangle 
         Rectangle rect = new Rectangle();
         rect.X = e.Bounds.X;
         rect.Y = e.Bounds.Y;
         rect.Width = colorRectWidth;
         rect.Height = e.Bounds.Height - itemYAdditionalOffset;


         //-- fill the rectangle 
         Brush colorBrush = new SolidBrush(color.Color);
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
         e.Graphics.FillRectangle(brushBackground, e.Bounds.X + 45, e.Bounds.Y, e.Bounds.Width - colorRectWidth, e.Bounds.Height - itemYAdditionalOffset);

         e.Graphics.DrawString(color.ToString(), e.Font, brush, e.Bounds.X + 45, e.Bounds.Y);
      }

      // Add Pallette Color
      private void button2_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         colorDialog1.FullOpen = true;
         DialogResult result = colorDialog1.ShowDialog();
         if (result == DialogResult.OK)
         {
            PalletteColor palletteColor = new PalletteColor();
            palletteColor.Color = colorDialog1.Color;
            palletteColor.Weight = 1.0f;
            data.Color.ColorPallette.Add(palletteColor);
            refreshPalletteListBox();
         }
      }

      //-- delete Pallette Color
      private void button3_Click(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         int index = listBox1.SelectedIndex;
         if (index < 0 || index >= data.Color.ColorPallette.Count)
            return;

         data.Color.ColorPallette.RemoveAt(index);
         refreshPalletteListBox();
      }

      private void listBox1_MeasureItem(object sender, MeasureItemEventArgs e)
      {
         e.ItemHeight += 4; ;
      }

      private void listBox1_MouseDoubleClick(object sender, MouseEventArgs e)
      {
         base.OnMouseDoubleClick(e);
         int index = listBox1.SelectedIndex;
         if (e.X >= 0 && e.X <= 40 && listBox1.SelectedIndex != -1)
         {
            colorDialog1.FullOpen = true;
            DialogResult result = colorDialog1.ShowDialog();
            if (result == DialogResult.OK)
            {
               data.Color.ColorPallette[index].Color = colorDialog1.Color;
               refreshPalletteListBox();
            }
         }
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

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         data.Color.PlayerColor = checkBox1.Checked;
         updateEnableStates();
      }

      private void floatSliderEdit1_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         data.Color.PlayerColorIntensity = floatSliderEdit1.Value;
      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         data.Color.SunColor = checkBox2.Checked;
         updateEnableStates();
      }

      private void floatSliderEdit2_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         data.Color.SunColorIntensity = floatSliderEdit2.Value;
      }     
   }
}
