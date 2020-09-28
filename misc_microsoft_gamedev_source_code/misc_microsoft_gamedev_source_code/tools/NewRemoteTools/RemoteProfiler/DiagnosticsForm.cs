using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;


using RemoteTools;

namespace EnsembleStudios.RemoteGameDebugger.Profiler
{

   public class DiagnosticsForm : System.Windows.Forms.Form//WeifenLuo.WinFormsUI.DockContent /*System.Windows.Forms.Form*/
	{

		private System.ComponentModel.Container components = null;

		public DiagnosticsForm()
		{

			InitializeComponent();

         ErrorHandler.OnError+=new ErrorEvent(ErrorHandler_OnError);

		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
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
         this.OutputTextBox = new System.Windows.Forms.TextBox();
         this.ClearButton = new System.Windows.Forms.Button();
         this.panel1 = new System.Windows.Forms.Panel();
         this.panel1.SuspendLayout();
         this.SuspendLayout();
         // 
         // OutputTextBox
         // 
         this.OutputTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
         this.OutputTextBox.Location = new System.Drawing.Point(8, 8);
         this.OutputTextBox.Multiline = true;
         this.OutputTextBox.Name = "OutputTextBox";
         this.OutputTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
         this.OutputTextBox.Size = new System.Drawing.Size(744, 242);
         this.OutputTextBox.TabIndex = 0;
         this.OutputTextBox.Text = "";
         // 
         // ClearButton
         // 
         this.ClearButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.ClearButton.Location = new System.Drawing.Point(8, 258);
         this.ClearButton.Name = "ClearButton";
         this.ClearButton.TabIndex = 1;
         this.ClearButton.Text = "Clear";
         this.ClearButton.Click += new System.EventHandler(this.ClearButton_Click);
         // 
         // panel1
         // 
         this.panel1.Controls.Add(this.OutputTextBox);
         this.panel1.Controls.Add(this.ClearButton);
         this.panel1.Location = new System.Drawing.Point(0, 0);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(776, 296);
         this.panel1.TabIndex = 2;
         // 
         // DiagnosticsForm
         // 
         this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
         this.ClientSize = new System.Drawing.Size(792, 318);
         this.Controls.Add(this.panel1);
         this.Name = "DiagnosticsForm";
         this.Text = "DiagnosticsForm";
         this.Load += new System.EventHandler(this.DiagnosticsForm_Load);
         this.Enter += new System.EventHandler(this.DiagnosticsForm_Enter);
         this.panel1.ResumeLayout(false);
         this.ResumeLayout(false);

      }
      #endregion

      private System.Windows.Forms.TextBox OutputTextBox;
      private System.Windows.Forms.Button ClearButton;
      private System.Windows.Forms.Panel panel1;

      string mOutput = "";
      private void ErrorHandler_OnError(string error)
      {
         mOutput += "\r\n" + error;
         OutputTextBox.Text = mOutput;
      }

      private void ClearButton_Click(object sender, System.EventArgs e)
      {
         mOutput = "";
         OutputTextBox.Text = mOutput;
      }

      private void DiagnosticsForm_Load(object sender, System.EventArgs e)
      {
//         if(this.Height > 50)
//            panel1.Height = this.Height - 50;
//         if(this.Width > 50)
//            panel1.Width = this.Width - 50;   
         //panel1.Height = this.Height;
         panel1.Width = this.Width;

         panel1.Update();     
      }

      private void DiagnosticsForm_Enter(object sender, System.EventArgs e)
      {
//         if(this.Height > 50)
//            panel1.Height = this.Height - 50;
//         if(this.Width > 50)
//            panel1.Width = this.Width - 50;   
         //panel1.Height = this.Height;
         panel1.Width = this.Width;
         panel1.Update();
      }

   }
}
