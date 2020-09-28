using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace PublishIt
{
    public partial class P4Login : Form
    {
        public string _p4user = null;
        public string _p4password = null;
        
        public P4Login()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            _p4user = textBoxUser.Text;
            _p4password = textBoxPassword.Text;
            
            Form1 form = new Form1();
            form.Show();
            this.Hide();
        }
    }
}