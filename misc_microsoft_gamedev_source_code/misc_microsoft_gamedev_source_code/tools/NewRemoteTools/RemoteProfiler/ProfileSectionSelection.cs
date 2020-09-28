using System;
using System.IO;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;


using EnsembleStudios.RemoteGameDebugger.Profiler;
using RemoteTools;

namespace EnsembleStudios.RemoteGameDebugger.Profiler
{
	/// <summary>
	/// Summary description for ProfileSectionSelection.
	/// </summary>
	public class ProfileSectionSelection : System.Windows.Forms.Form
	{
      private System.Windows.Forms.CheckedListBox EnabledSectionListBox;
      private System.Windows.Forms.Button ApplyButton;
      private System.Windows.Forms.Button SaveButton;
      private System.Windows.Forms.Button LoadButton;
      private System.Windows.Forms.CheckBox EnableCheckBox;
      private System.Windows.Forms.Button SetGroupButton;
      private System.Windows.Forms.TextBox SubStringTextBox;
      private System.Windows.Forms.Button OKButton;
      private System.Windows.Forms.Button CancelButton1;
      private System.Windows.Forms.Button EnableAllButton;
      private System.Windows.Forms.Button DisableallButton;


		private System.ComponentModel.Container components = null;

		public ProfileSectionSelection()
		{
			InitializeComponent();
		}

	
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
         this.EnabledSectionListBox = new System.Windows.Forms.CheckedListBox();
         this.ApplyButton = new System.Windows.Forms.Button();
         this.CancelButton1 = new System.Windows.Forms.Button();
         this.SaveButton = new System.Windows.Forms.Button();
         this.LoadButton = new System.Windows.Forms.Button();
         this.OKButton = new System.Windows.Forms.Button();
         this.SubStringTextBox = new System.Windows.Forms.TextBox();
         this.EnableCheckBox = new System.Windows.Forms.CheckBox();
         this.SetGroupButton = new System.Windows.Forms.Button();
         this.EnableAllButton = new System.Windows.Forms.Button();
         this.DisableallButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // EnabledSectionListBox
         // 
         this.EnabledSectionListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
         this.EnabledSectionListBox.CheckOnClick = true;
         this.EnabledSectionListBox.Location = new System.Drawing.Point(8, 56);
         this.EnabledSectionListBox.Name = "EnabledSectionListBox";
         this.EnabledSectionListBox.Size = new System.Drawing.Size(384, 184);
         this.EnabledSectionListBox.TabIndex = 0;
         // 
         // ApplyButton
         // 
         this.ApplyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.ApplyButton.Location = new System.Drawing.Point(248, 304);
         this.ApplyButton.Name = "ApplyButton";
         this.ApplyButton.TabIndex = 1;
         this.ApplyButton.Text = "Apply";
         this.ApplyButton.Click += new System.EventHandler(this.ApplyButton_Click);
         // 
         // CancelButton1
         // 
         this.CancelButton1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.CancelButton1.Location = new System.Drawing.Point(160, 304);
         this.CancelButton1.Name = "CancelButton1";
         this.CancelButton1.TabIndex = 2;
         this.CancelButton1.Text = "Cancel";
         this.CancelButton1.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // SaveButton
         // 
         this.SaveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.SaveButton.Location = new System.Drawing.Point(96, 248);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.TabIndex = 3;
         this.SaveButton.Text = "Save";
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // LoadButton
         // 
         this.LoadButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.LoadButton.Location = new System.Drawing.Point(8, 248);
         this.LoadButton.Name = "LoadButton";
         this.LoadButton.TabIndex = 4;
         this.LoadButton.Text = "Load";
         this.LoadButton.Click += new System.EventHandler(this.LoadButton_Click);
         // 
         // OKButton
         // 
         this.OKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.OKButton.DialogResult = System.Windows.Forms.DialogResult.OK;
         this.OKButton.Location = new System.Drawing.Point(72, 304);
         this.OKButton.Name = "OKButton";
         this.OKButton.TabIndex = 5;
         this.OKButton.Text = "OK";
         this.OKButton.Click += new System.EventHandler(this.OKButton_Click);
         // 
         // SubStringTextBox
         // 
         this.SubStringTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
         this.SubStringTextBox.Location = new System.Drawing.Point(88, 24);
         this.SubStringTextBox.Name = "SubStringTextBox";
         this.SubStringTextBox.Size = new System.Drawing.Size(212, 20);
         this.SubStringTextBox.TabIndex = 6;
         this.SubStringTextBox.Text = "subString";
         // 
         // EnableCheckBox
         // 
         this.EnableCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.EnableCheckBox.Location = new System.Drawing.Point(304, 24);
         this.EnableCheckBox.Name = "EnableCheckBox";
         this.EnableCheckBox.TabIndex = 7;
         this.EnableCheckBox.Text = "Enabled";
         // 
         // SetGroupButton
         // 
         this.SetGroupButton.Location = new System.Drawing.Point(8, 24);
         this.SetGroupButton.Name = "SetGroupButton";
         this.SetGroupButton.TabIndex = 8;
         this.SetGroupButton.Text = "Set Group";
         this.SetGroupButton.Click += new System.EventHandler(this.SetGroupButton_Click);
         // 
         // EnableAllButton
         // 
         this.EnableAllButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.EnableAllButton.Location = new System.Drawing.Point(184, 248);
         this.EnableAllButton.Name = "EnableAllButton";
         this.EnableAllButton.TabIndex = 9;
         this.EnableAllButton.Text = "Enable All";
         this.EnableAllButton.Click += new System.EventHandler(this.EnableAllButton_Click);
         // 
         // DisableallButton
         // 
         this.DisableallButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.DisableallButton.Location = new System.Drawing.Point(272, 248);
         this.DisableallButton.Name = "DisableallButton";
         this.DisableallButton.TabIndex = 10;
         this.DisableallButton.Text = "Disable All";
         this.DisableallButton.Click += new System.EventHandler(this.DisableallButton_Click);
         // 
         // ProfileSectionSelection
         // 
         this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
         this.ClientSize = new System.Drawing.Size(408, 342);
         this.Controls.Add(this.DisableallButton);
         this.Controls.Add(this.EnableAllButton);
         this.Controls.Add(this.SetGroupButton);
         this.Controls.Add(this.EnableCheckBox);
         this.Controls.Add(this.SubStringTextBox);
         this.Controls.Add(this.OKButton);
         this.Controls.Add(this.LoadButton);
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.CancelButton1);
         this.Controls.Add(this.ApplyButton);
         this.Controls.Add(this.EnabledSectionListBox);
         this.Name = "ProfileSectionSelection";
         this.Text = "ProfileSectionSelection";
         this.ResumeLayout(false);

      }
		#endregion

      private void SaveButton_Click(object sender, System.EventArgs e)
      {
         
      }

      private void LoadButton_Click(object sender, System.EventArgs e)
      {
         
      }

      public void SetListBoxData(BProfileSection[] sections)
      {
         EnabledSectionListBox.Items.Clear();
         for(int i = 0; i < mNumSections; i++)
         {
            BProfileSection section = sections[i];
            EnabledSectionListBox.Items.Add(section.mName,section.mEnabled);
         }
      }
      public void GetListBoxData(ref BProfileSection[] sections)
      {
         for(int i = 0; i < mNumSections; i++)
         {
            string name = (string)(EnabledSectionListBox.Items[i]);         
            bool enabled = EnabledSectionListBox.GetItemChecked(i);

            for(int j = 0; j < mNumSections; j++)
            {
               BProfileSection section = sections[j];
               if(name == section.mName)
               {
                  section.mEnabled = enabled;
               }
            }            
         }
      }

      BProfileSection[] mSections;
      int mNumSections;
      public void SetData(BProfileSection[] sections, int numSections)
      {
         mNumSections = numSections;
         mSections = sections;
         SetListBoxData(mSections);       
      }
      private void SetGroupButton_Click(object sender, System.EventArgs e)
      {
         for(int i = 0; i < mNumSections; i++)
         {
            string name = (string)(EnabledSectionListBox.Items[i]);     
            if(name.IndexOf(SubStringTextBox.Text) >= 0)
            {
               EnabledSectionListBox.SetItemChecked(i,EnableCheckBox.Checked);
            }           
         }     
      }

      //public MainForm mMainForm;
      //public void SetMainForm(MainForm mainForm)
      //{
      //   mMainForm = mainForm;
      //}

      NetServiceConnection mActiveClient = null;
      public NetServiceConnection ActiveClient
      {
         set
         {
            mActiveClient = value;
         }
      }
      private bool Connected()
      {
         return (mActiveClient != null);
      }

      public void UpdateSections()
      {
         this.GetListBoxData(ref mSections);
         this.SendSectionInfo(ref mSections);
      }



      public bool SendSectionInfo(ref BProfileSection[] sections)
      {
         try
         {
            if (!Connected())
            {
               MessageBox.Show(this,"Error, no connection to game");
               return false;
            }

            MemoryStream memStream = new MemoryStream();
            ArrayList ids = new ArrayList();
            ArrayList values = new ArrayList();

            for(int j = 0; j < mNumSections; j++)
            {
               BProfileSection section = sections[j];
               ids.Add((int)section.mID);
               values.Add((section.mEnabled)?(byte)1:(byte)0);
         
            }
            TimelineControlPacket timeControl = new TimelineControlPacket(memStream, TimelineControlPacket.TimelineControlValues.cConfigureSections,ids,values );

            
            mActiveClient.write(memStream);

            return true;
         }
         catch(System.Exception ex)
         {
            ErrorHandler.Error(ex.ToString());
            //todo: log this error
            MessageBox.Show(this,"Error Sending Message: " + ex.ToString());
            return false;
         }
      }

      private void ApplyButton_Click(object sender, System.EventArgs e)
      {
         UpdateSections();
      }

      private void OKButton_Click(object sender, System.EventArgs e)
      {
         UpdateSections();
         this.Close();

      }

      private void CancelButton_Click(object sender, System.EventArgs e)
      {
         this.Close();
      }

      private void EnableAllButton_Click(object sender, System.EventArgs e)
      {
         for(int i = 0; i < mNumSections; i++)
         {
               EnabledSectionListBox.SetItemChecked(i,true);
         }
      }

      private void DisableallButton_Click(object sender, System.EventArgs e)
      {
         for(int i = 0; i < mNumSections; i++)
         {
            EnabledSectionListBox.SetItemChecked(i,false);
         }      
      }
	}
}
