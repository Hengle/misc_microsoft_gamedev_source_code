namespace PhoenixEditor.ClientTabPages
{
   partial class TriggerTemplatePage
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
         this.SaveButton = new System.Windows.Forms.Button();
         this.LoadButton = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.label4 = new System.Windows.Forms.Label();
         this.TemplateAttributesPropertyGrid = new EditorCore.BetterPropertyGrid();
         this.triggerHostArea2 = new PhoenixEditor.ScenarioEditor.TriggerHostArea();
         this.OutputTriggersBasicTypedSuperList = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.InputTriggersBasicTypedSuperList = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.OutputVarsBasicTypedSuperList = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.InputVarsBasicTypedSuperList = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.previewNavWindow1 = new PhoenixEditor.ScenarioEditor.PreviewNavWindow();
         this.triggerHostArea1 = new PhoenixEditor.ScenarioEditor.TriggerHostArea();
         this.SaveAsButton = new System.Windows.Forms.Button();
         this.ReloadTemplatesButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // SaveButton
         // 
         this.SaveButton.Location = new System.Drawing.Point(3, 32);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(75, 23);
         this.SaveButton.TabIndex = 5;
         this.SaveButton.Text = "Save";
         this.SaveButton.UseVisualStyleBackColor = true;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // LoadButton
         // 
         this.LoadButton.Location = new System.Drawing.Point(3, 3);
         this.LoadButton.Name = "LoadButton";
         this.LoadButton.Size = new System.Drawing.Size(75, 23);
         this.LoadButton.TabIndex = 4;
         this.LoadButton.Text = "Load";
         this.LoadButton.UseVisualStyleBackColor = true;
         this.LoadButton.Click += new System.EventHandler(this.LoadButton_Click);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(14, 165);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(74, 13);
         this.label1.TabIndex = 12;
         this.label1.Text = "InputVariables";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(14, 341);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(82, 13);
         this.label2.TabIndex = 13;
         this.label2.Text = "OutputVariables";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(14, 520);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(69, 13);
         this.label3.TabIndex = 14;
         this.label3.Text = "InputTriggers";
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(11, 678);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(77, 13);
         this.label4.TabIndex = 15;
         this.label4.Text = "OutputTriggers";
         // 
         // TemplateAttributesPropertyGrid
         // 
         this.TemplateAttributesPropertyGrid.LastRowHack = false;
         this.TemplateAttributesPropertyGrid.Location = new System.Drawing.Point(94, 13);
         this.TemplateAttributesPropertyGrid.Name = "TemplateAttributesPropertyGrid";
         this.TemplateAttributesPropertyGrid.Size = new System.Drawing.Size(281, 165);
         this.TemplateAttributesPropertyGrid.TabIndex = 16;
         // 
         // triggerHostArea2
         // 
         this.triggerHostArea2.ActiveNodeHostControl = null;
         this.triggerHostArea2.AllowDrop = true;
         this.triggerHostArea2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.triggerHostArea2.AutoScroll = true;
         this.triggerHostArea2.BackColor = System.Drawing.SystemColors.Window;
         this.triggerHostArea2.Location = new System.Drawing.Point(388, 13);
         this.triggerHostArea2.Name = "triggerHostArea2";
         this.triggerHostArea2.Size = new System.Drawing.Size(641, 165);
         this.triggerHostArea2.TabIndex = 17;
         // 
         // OutputTriggersBasicTypedSuperList
         // 
         this.OutputTriggersBasicTypedSuperList.AutoScroll = true;
         this.OutputTriggersBasicTypedSuperList.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.OutputTriggersBasicTypedSuperList.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.OutputTriggersBasicTypedSuperList.Location = new System.Drawing.Point(14, 694);
         this.OutputTriggersBasicTypedSuperList.Name = "OutputTriggersBasicTypedSuperList";
         this.OutputTriggersBasicTypedSuperList.Size = new System.Drawing.Size(361, 159);
         this.OutputTriggersBasicTypedSuperList.TabIndex = 11;
         this.OutputTriggersBasicTypedSuperList.UseLabels = true;
         this.OutputTriggersBasicTypedSuperList.WrapContents = false;
         // 
         // InputTriggersBasicTypedSuperList
         // 
         this.InputTriggersBasicTypedSuperList.AutoScroll = true;
         this.InputTriggersBasicTypedSuperList.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.InputTriggersBasicTypedSuperList.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.InputTriggersBasicTypedSuperList.Location = new System.Drawing.Point(14, 536);
         this.InputTriggersBasicTypedSuperList.Name = "InputTriggersBasicTypedSuperList";
         this.InputTriggersBasicTypedSuperList.Size = new System.Drawing.Size(361, 139);
         this.InputTriggersBasicTypedSuperList.TabIndex = 10;
         this.InputTriggersBasicTypedSuperList.UseLabels = true;
         this.InputTriggersBasicTypedSuperList.WrapContents = false;
         // 
         // OutputVarsBasicTypedSuperList
         // 
         this.OutputVarsBasicTypedSuperList.AutoScroll = true;
         this.OutputVarsBasicTypedSuperList.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.OutputVarsBasicTypedSuperList.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.OutputVarsBasicTypedSuperList.Location = new System.Drawing.Point(14, 357);
         this.OutputVarsBasicTypedSuperList.Name = "OutputVarsBasicTypedSuperList";
         this.OutputVarsBasicTypedSuperList.Size = new System.Drawing.Size(361, 160);
         this.OutputVarsBasicTypedSuperList.TabIndex = 9;
         this.OutputVarsBasicTypedSuperList.UseLabels = true;
         this.OutputVarsBasicTypedSuperList.WrapContents = false;
         // 
         // InputVarsBasicTypedSuperList
         // 
         this.InputVarsBasicTypedSuperList.AutoScroll = true;
         this.InputVarsBasicTypedSuperList.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.InputVarsBasicTypedSuperList.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.InputVarsBasicTypedSuperList.Location = new System.Drawing.Point(17, 184);
         this.InputVarsBasicTypedSuperList.Name = "InputVarsBasicTypedSuperList";
         this.InputVarsBasicTypedSuperList.Size = new System.Drawing.Size(361, 152);
         this.InputVarsBasicTypedSuperList.TabIndex = 8;
         this.InputVarsBasicTypedSuperList.UseLabels = true;
         this.InputVarsBasicTypedSuperList.WrapContents = false;
         // 
         // previewNavWindow1
         // 
         this.previewNavWindow1.AllowDrop = true;
         this.previewNavWindow1.BackColor = System.Drawing.SystemColors.ActiveCaptionText;
         this.previewNavWindow1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.previewNavWindow1.Location = new System.Drawing.Point(388, 184);
         this.previewNavWindow1.Name = "previewNavWindow1";
         this.previewNavWindow1.Size = new System.Drawing.Size(160, 96);
         this.previewNavWindow1.TabIndex = 7;
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
         this.triggerHostArea1.Location = new System.Drawing.Point(388, 184);
         this.triggerHostArea1.Name = "triggerHostArea1";
         this.triggerHostArea1.Size = new System.Drawing.Size(641, 669);
         this.triggerHostArea1.TabIndex = 6;
         // 
         // SaveAsButton
         // 
         this.SaveAsButton.Location = new System.Drawing.Point(4, 62);
         this.SaveAsButton.Name = "SaveAsButton";
         this.SaveAsButton.Size = new System.Drawing.Size(75, 23);
         this.SaveAsButton.TabIndex = 18;
         this.SaveAsButton.Text = "Save As...";
         this.SaveAsButton.UseVisualStyleBackColor = true;
         this.SaveAsButton.Click += new System.EventHandler(this.SaveAsButton_Click);
         // 
         // ReloadTemplatesButton
         // 
         this.ReloadTemplatesButton.Location = new System.Drawing.Point(4, 130);
         this.ReloadTemplatesButton.Name = "ReloadTemplatesButton";
         this.ReloadTemplatesButton.Size = new System.Drawing.Size(75, 23);
         this.ReloadTemplatesButton.TabIndex = 19;
         this.ReloadTemplatesButton.Text = "ReloadAll";
         this.ReloadTemplatesButton.UseVisualStyleBackColor = true;
         this.ReloadTemplatesButton.Click += new System.EventHandler(this.ReloadTemplatesButton_Click);
         // 
         // TriggerTemplatePage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.ReloadTemplatesButton);
         this.Controls.Add(this.SaveAsButton);
         this.Controls.Add(this.triggerHostArea2);
         this.Controls.Add(this.TemplateAttributesPropertyGrid);
         this.Controls.Add(this.label4);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.OutputTriggersBasicTypedSuperList);
         this.Controls.Add(this.InputTriggersBasicTypedSuperList);
         this.Controls.Add(this.OutputVarsBasicTypedSuperList);
         this.Controls.Add(this.InputVarsBasicTypedSuperList);
         this.Controls.Add(this.previewNavWindow1);
         this.Controls.Add(this.triggerHostArea1);
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.LoadButton);
         this.Name = "TriggerTemplatePage";
         this.Size = new System.Drawing.Size(1032, 856);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private PhoenixEditor.ScenarioEditor.PreviewNavWindow previewNavWindow1;
      private PhoenixEditor.ScenarioEditor.TriggerHostArea triggerHostArea1;
      private System.Windows.Forms.Button SaveButton;
      private System.Windows.Forms.Button LoadButton;
      private PhoenixEditor.ScenarioEditor.BasicTypedSuperList InputVarsBasicTypedSuperList;
      private PhoenixEditor.ScenarioEditor.BasicTypedSuperList OutputVarsBasicTypedSuperList;
      private PhoenixEditor.ScenarioEditor.BasicTypedSuperList InputTriggersBasicTypedSuperList;
      private PhoenixEditor.ScenarioEditor.BasicTypedSuperList OutputTriggersBasicTypedSuperList;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Label label4;
      private EditorCore.BetterPropertyGrid TemplateAttributesPropertyGrid;
      private PhoenixEditor.ScenarioEditor.TriggerHostArea triggerHostArea2;
      private System.Windows.Forms.Button SaveAsButton;
      private System.Windows.Forms.Button ReloadTemplatesButton;
   }
}
