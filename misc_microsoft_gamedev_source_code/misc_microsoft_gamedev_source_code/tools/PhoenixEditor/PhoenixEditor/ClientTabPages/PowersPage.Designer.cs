namespace PhoenixEditor.ClientTabPages
{
   partial class ScriptPage
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

      #region Windows Form Designer generated code

      /// <summary>
      /// Required method for Designer support - do not modify
      /// the contents of this method with the code editor.
      /// </summary>
      private void InitializeComponent()
      {
         this.LoadButton = new System.Windows.Forms.Button();
         this.SaveButton = new System.Windows.Forms.Button();
         this.ErrorPanel = new System.Windows.Forms.Panel();
         this.SaveAsButton = new System.Windows.Forms.Button();
         this.buildMode = new System.Windows.Forms.ComboBox();
         this.previewNavWindow1 = new PhoenixEditor.ScenarioEditor.PreviewNavWindow();
         this.triggerHostArea1 = new PhoenixEditor.ScenarioEditor.TriggerHostArea();
         this.SuspendLayout();
         // 
         // LoadButton
         // 
         this.LoadButton.Location = new System.Drawing.Point(171, 3);
         this.LoadButton.Name = "LoadButton";
         this.LoadButton.Size = new System.Drawing.Size(39, 23);
         this.LoadButton.TabIndex = 0;
         this.LoadButton.Text = "Load";
         this.LoadButton.UseVisualStyleBackColor = true;
         this.LoadButton.Click += new System.EventHandler(this.LoadButton_Click);
         // 
         // SaveButton
         // 
         this.SaveButton.Location = new System.Drawing.Point(216, 3);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(41, 23);
         this.SaveButton.TabIndex = 1;
         this.SaveButton.Text = "Save";
         this.SaveButton.UseVisualStyleBackColor = true;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // ErrorPanel
         // 
         this.ErrorPanel.Location = new System.Drawing.Point(334, 3);
         this.ErrorPanel.Name = "ErrorPanel";
         this.ErrorPanel.Size = new System.Drawing.Size(47, 23);
         this.ErrorPanel.TabIndex = 4;
         // 
         // SaveAsButton
         // 
         this.SaveAsButton.Location = new System.Drawing.Point(263, 3);
         this.SaveAsButton.Name = "SaveAsButton";
         this.SaveAsButton.Size = new System.Drawing.Size(65, 23);
         this.SaveAsButton.TabIndex = 5;
         this.SaveAsButton.Text = "Save As";
         this.SaveAsButton.UseVisualStyleBackColor = true;
         this.SaveAsButton.Click += new System.EventHandler(this.SaveAsButton_Click);
         // 
         // buildMode
         // 
         this.buildMode.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.buildMode.FormattingEnabled = true;
         this.buildMode.Location = new System.Drawing.Point(388, 5);
         this.buildMode.Name = "buildMode";
         this.buildMode.Size = new System.Drawing.Size(173, 21);
         this.buildMode.TabIndex = 6;
         // 
         // previewNavWindow1
         // 
         this.previewNavWindow1.AllowDrop = true;
         this.previewNavWindow1.BackColor = System.Drawing.SystemColors.ActiveCaptionText;
         this.previewNavWindow1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.previewNavWindow1.Location = new System.Drawing.Point(5, 5);
         this.previewNavWindow1.Name = "previewNavWindow1";
         this.previewNavWindow1.Size = new System.Drawing.Size(160, 96);
         this.previewNavWindow1.TabIndex = 3;
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
         this.triggerHostArea1.Location = new System.Drawing.Point(3, 32);
         this.triggerHostArea1.Name = "triggerHostArea1";
         this.triggerHostArea1.Size = new System.Drawing.Size(726, 496);
         this.triggerHostArea1.TabIndex = 2;
         // 
         // ScriptPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.Controls.Add(this.buildMode);
         this.Controls.Add(this.SaveAsButton);
         this.Controls.Add(this.ErrorPanel);
         this.Controls.Add(this.previewNavWindow1);
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.LoadButton);
         this.Controls.Add(this.triggerHostArea1);
         this.Name = "ScriptPage";
         this.Size = new System.Drawing.Size(732, 531);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button LoadButton;
      private System.Windows.Forms.Button SaveButton;
      private PhoenixEditor.ScenarioEditor.TriggerHostArea triggerHostArea1;
      private PhoenixEditor.ScenarioEditor.PreviewNavWindow previewNavWindow1;
      private System.Windows.Forms.Panel ErrorPanel;
      private System.Windows.Forms.Button SaveAsButton;
      private System.Windows.Forms.ComboBox buildMode;
   }
}
