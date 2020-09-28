using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace graphapp
{
    public partial class Device_FileOutputDlg : Form
    {

        public Device_FileOutput mOwningDevice = null;
        public Device_FileOutputDlg()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (mOwningDevice==null)
                return;

            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = getFilterFromFormatSettings();
            sfd.InitialDirectory = AppDomain.CurrentDomain.BaseDirectory;
            if (sfd.ShowDialog() == DialogResult.OK)
            {
                textBox1.Text = sfd.FileName;
            }
        }


        string getFilterFromFormatSettings()
        {
            if (formatComboBox.SelectedIndex==0) return "Raw 8 bit(*.raw)|*.raw";
            if (formatComboBox.SelectedIndex == 1) return "Raw 16 bit(*.r16)|*.r16";
            if (formatComboBox.SelectedIndex == 2) return "Raw 32 bit float(*.r32)|*.r32";

            return null;
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (textBox1.Text == "")
            {
                MessageBox.Show("Please input filename");
                return;
            }


            OutputGenerationParams ogp = new OutputGenerationParams();
            ogp.Width = (int)numericUpDown1.Value;
            ogp.Height = (int)numericUpDown2.Value;

            if (mOwningDevice.computeOutput(null, ogp))
            {
                if (mOwningDevice.writeToFile(textBox1.Text, ogp))
                {
                    MessageBox.Show("File " + textBox1.Text + " written successfully!");
                    this.Close();
                }
            }
            else
            {
                MessageBox.Show("This node is not connected properly.");
            }
        }

        private void Device_FileOutputDlg_Load(object sender, EventArgs e)
        {
            formatComboBox.SelectedIndex = 0;
        }
    }
}
