using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using C2C.FileSystem;

namespace DirectoryTreeView
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class Form1 : System.Windows.Forms.Form
	{
      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.TextBox txtDirectory;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Button btnDirectory;
      private C2C.FileSystem.FileSystemTreeView tree;
      private System.Windows.Forms.Panel treePanel;
      private Button btnDiff;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public Form1()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
         this.panel1 = new System.Windows.Forms.Panel();
         this.btnDirectory = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.txtDirectory = new System.Windows.Forms.TextBox();
         this.treePanel = new System.Windows.Forms.Panel();
         this.btnDiff = new System.Windows.Forms.Button();
         this.panel1.SuspendLayout();
         this.SuspendLayout();
         // 
         // panel1
         // 
         this.panel1.Controls.Add(this.btnDiff);
         this.panel1.Controls.Add(this.btnDirectory);
         this.panel1.Controls.Add(this.label1);
         this.panel1.Controls.Add(this.txtDirectory);
         this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
         this.panel1.Location = new System.Drawing.Point(0, 0);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(322, 57);
         this.panel1.TabIndex = 0;
         // 
         // btnDirectory
         // 
         this.btnDirectory.Location = new System.Drawing.Point(241, 26);
         this.btnDirectory.Name = "btnDirectory";
         this.btnDirectory.Size = new System.Drawing.Size(30, 21);
         this.btnDirectory.TabIndex = 2;
         this.btnDirectory.Text = "...";
         this.btnDirectory.Click += new System.EventHandler(this.btnDirectory_Click);
         // 
         // label1
         // 
         this.label1.Location = new System.Drawing.Point(9, 9);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(102, 18);
         this.label1.TabIndex = 1;
         this.label1.Text = "Directory:";
         // 
         // txtDirectory
         // 
         this.txtDirectory.Location = new System.Drawing.Point(9, 27);
         this.txtDirectory.Name = "txtDirectory";
         this.txtDirectory.Size = new System.Drawing.Size(226, 20);
         this.txtDirectory.TabIndex = 0;
         this.txtDirectory.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.txtDirectory_KeyPress);
         this.txtDirectory.KeyDown += new System.Windows.Forms.KeyEventHandler(this.txtDirectory_KeyDown);
         // 
         // treePanel
         // 
         this.treePanel.AutoSize = true;
         this.treePanel.Dock = System.Windows.Forms.DockStyle.Fill;
         this.treePanel.Location = new System.Drawing.Point(0, 57);
         this.treePanel.Name = "treePanel";
         this.treePanel.Size = new System.Drawing.Size(322, 530);
         this.treePanel.TabIndex = 1;
         // 
         // btnDiff
         // 
         this.btnDiff.Location = new System.Drawing.Point(277, 26);
         this.btnDiff.Name = "btnDiff";
         this.btnDiff.Size = new System.Drawing.Size(30, 21);
         this.btnDiff.TabIndex = 3;
         this.btnDiff.Text = "diff";
         this.btnDiff.Click += new System.EventHandler(this.btnDiff_Click);
         // 
         // Form1
         // 
         this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
         this.ClientSize = new System.Drawing.Size(322, 587);
         this.Controls.Add(this.treePanel);
         this.Controls.Add(this.panel1);
         this.Name = "Form1";
         this.Text = "DepotDiff";
         this.Load += new System.EventHandler(this.Form1_Load);
         this.panel1.ResumeLayout(false);
         this.panel1.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new Form1());
		}

      private void Form1_Load(object sender, System.EventArgs e)  
      {
         tree = new  C2C.FileSystem.FileSystemTreeView();         
         treePanel.Controls.Add( tree );
         tree.Dock = DockStyle.Fill;
         //tree.ShowFiles = false;
      }
      

      private void btnDirectory_Click(object sender, System.EventArgs e)
      {
         FolderBrowserDialog dlg = new FolderBrowserDialog();

         if( dlg.ShowDialog() == DialogResult.OK )
         {
            txtDirectory.Text = dlg.SelectedPath;
            tree.Load( txtDirectory.Text );
         }
      }

      private void txtDirectory_KeyPress(object sender, System.Windows.Forms.KeyPressEventArgs e)
      {
                
         
      }

      private void txtDirectory_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
      {
         if(  e.KeyData == Keys.Enter )
         {
            if( System.IO.Directory.Exists( txtDirectory.Text ) == false )
            {
               MessageBox.Show( "Directory Does Not Exist", "Invalid Directory", MessageBoxButtons.OK, MessageBoxIcon.Information );
               return;
            }
            tree.Load( txtDirectory.Text );
         }
      }

      private void btnDiff_Click(object sender, EventArgs e)
      {
         string selPath = tree.SelectedNode.FullPath.ToString();

         string[] split = selPath.Split(new Char[] { '\\' });
         selPath = "";
         for (int i = 1; i < split.Length; ++i )
         {
            selPath += String.Concat("\\", split[i]);
         }

         selPath = String.Concat(txtDirectory.Text, selPath);

         System.Diagnostics.Process proc = new System.Diagnostics.Process();
         proc.EnableRaisingEvents = false;
         proc.StartInfo.FileName = "havenot.bat";
         proc.StartInfo.Arguments = selPath;
         proc.StartInfo.CreateNoWindow = true;
         proc.Start();
         proc.WaitForExit();

         proc.StartInfo.FileName = "notepad.exe";
         proc.StartInfo.Arguments = "c:\\havenot.txt";
         proc.Start();
      }
	}
}
