namespace PhoenixEditor.ScenarioEditor
{
   partial class TriggerEditor
   {
      /// <summary> 
      /// Required designer variable.
      /// </summary>
      private System.ComponentModel.IContainer components = null;

      /// <summary> 
      /// Clean up any resources being used.
      /// </summary>
      /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
      protected override void Dispose(bool disposing)
      {
         if (disposing && (components != null))
         {
            components.Dispose();
         }
         base.Dispose(disposing);
      }

      #region Component Designer generated code

      /// <summary> 
      /// Required method for Designer support - do not modify 
      /// the contents of this method with the code editor.
      /// </summary>
      private void InitializeComponent()
      {
         this.panel1 = new System.Windows.Forms.Panel();
         this.triggerHostArea1 = new PhoenixEditor.ScenarioEditor.TriggerHostArea();
         this.previewNavWindow1 = new PhoenixEditor.ScenarioEditor.PreviewNavWindow();
         this.MaximizeCheckBox = new System.Windows.Forms.CheckBox();
         this.ImportButton = new System.Windows.Forms.Button();
         this.ExportButton = new System.Windows.Forms.Button();
         this.ErrorPanel = new System.Windows.Forms.Panel();
         this.SuspendLayout();
         // 
         // panel1
         // 
         this.panel1.BackColor = System.Drawing.SystemColors.GradientInactiveCaption;
         this.panel1.Location = new System.Drawing.Point(451, 3);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(50, 34);
         this.panel1.TabIndex = 16;
         this.panel1.MouseLeave += new System.EventHandler(this.panel1_MouseLeave);
         this.panel1.MouseEnter += new System.EventHandler(this.panel1_MouseEnter);
         // 
         // triggerHostArea1
         // 
         this.triggerHostArea1.ActiveNodeHostControl = null;
         this.triggerHostArea1.AllowDrop = true;
         this.triggerHostArea1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.triggerHostArea1.AutoScroll = true;
         this.triggerHostArea1.BackColor = System.Drawing.SystemColors.Window;
         this.triggerHostArea1.Location = new System.Drawing.Point(3, 43);
         this.triggerHostArea1.Name = "triggerHostArea1";
         this.triggerHostArea1.Size = new System.Drawing.Size(1012, 674);
         this.triggerHostArea1.TabIndex = 19;
         // 
         // previewNavWindow1
         // 
         this.previewNavWindow1.AllowDrop = true;
         this.previewNavWindow1.BackColor = System.Drawing.SystemColors.ControlLightLight;
         this.previewNavWindow1.Location = new System.Drawing.Point(3, 3);
         this.previewNavWindow1.Name = "previewNavWindow1";
         this.previewNavWindow1.Size = new System.Drawing.Size(187, 123);
         this.previewNavWindow1.TabIndex = 22;
         // 
         // MaximizeCheckBox
         // 
         this.MaximizeCheckBox.AutoSize = true;
         this.MaximizeCheckBox.Location = new System.Drawing.Point(353, 20);
         this.MaximizeCheckBox.Name = "MaximizeCheckBox";
         this.MaximizeCheckBox.Size = new System.Drawing.Size(92, 17);
         this.MaximizeCheckBox.TabIndex = 23;
         this.MaximizeCheckBox.Text = "MaximizeView";
         this.MaximizeCheckBox.UseVisualStyleBackColor = true;
         this.MaximizeCheckBox.CheckedChanged += new System.EventHandler(this.MaximizeCheckBox_CheckedChanged);
         // 
         // ImportButton
         // 
         this.ImportButton.Location = new System.Drawing.Point(250, 3);
         this.ImportButton.Name = "ImportButton";
         this.ImportButton.Size = new System.Drawing.Size(46, 23);
         this.ImportButton.TabIndex = 24;
         this.ImportButton.Text = "Import";
         this.ImportButton.UseVisualStyleBackColor = true;
         this.ImportButton.Click += new System.EventHandler(this.ImportButton_Click);
         // 
         // ExportButton
         // 
         this.ExportButton.Location = new System.Drawing.Point(302, 4);
         this.ExportButton.Name = "ExportButton";
         this.ExportButton.Size = new System.Drawing.Size(45, 23);
         this.ExportButton.TabIndex = 25;
         this.ExportButton.Text = "Export";
         this.ExportButton.UseVisualStyleBackColor = true;
         this.ExportButton.Click += new System.EventHandler(this.ExportButton_Click);
         // 
         // ErrorPanel
         // 
         this.ErrorPanel.Location = new System.Drawing.Point(197, 4);
         this.ErrorPanel.Name = "ErrorPanel";
         this.ErrorPanel.Size = new System.Drawing.Size(47, 33);
         this.ErrorPanel.TabIndex = 26;
         // 
         // TriggerEditor
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.ErrorPanel);
         this.Controls.Add(this.ExportButton);
         this.Controls.Add(this.ImportButton);
         this.Controls.Add(this.MaximizeCheckBox);
         this.Controls.Add(this.panel1);
         this.Controls.Add(this.previewNavWindow1);
         this.Controls.Add(this.triggerHostArea1);
         this.Name = "TriggerEditor";
         this.Size = new System.Drawing.Size(1018, 720);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Panel panel1;
      private TriggerHostArea triggerHostArea1;
      private PreviewNavWindow previewNavWindow1;
      private System.Windows.Forms.CheckBox MaximizeCheckBox;
      private System.Windows.Forms.Button ImportButton;
      private System.Windows.Forms.Button ExportButton;
      private System.Windows.Forms.Panel ErrorPanel;


   }
}
