using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace EnsembleStudios.RemoteGameDebugger.Profiler
{
	/// <summary>
	/// Summary description for TimelineSettings.
	/// </summary>
	public class TimelineSettings : System.Windows.Forms.Form
	{
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Button button1;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public TimelineSettings()
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
         this.label1 = new System.Windows.Forms.Label();
         this.button1 = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.Location = new System.Drawing.Point(16, 16);
         this.label1.Name = "label1";
         this.label1.TabIndex = 1;
         this.label1.Text = "Plots";
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(88, 296);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(120, 23);
         this.button1.TabIndex = 2;
         this.button1.Text = "RemoveSelected";
         // 
         // TimelineSettings
         // 
         this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
         this.ClientSize = new System.Drawing.Size(424, 330);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.label1);
         this.Name = "TimelineSettings";
         this.Text = "TimelineSettings";
         this.ResumeLayout(false);

      }
		#endregion
	}
}
