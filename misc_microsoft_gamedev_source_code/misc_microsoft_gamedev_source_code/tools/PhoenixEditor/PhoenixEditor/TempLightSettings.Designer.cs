namespace PhoenixEditor
{
   partial class TempLightSettings
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
         this.components = new System.ComponentModel.Container();
         this.LoadButton = new System.Windows.Forms.Button();
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.SaveButton = new System.Windows.Forms.Button();
         this.SaveAsButton = new System.Windows.Forms.Button();
         this.button1 = new System.Windows.Forms.Button();
         this.button2 = new System.Windows.Forms.Button();
         this.button3 = new System.Windows.Forms.Button();
         this.LiveUpdateCheckBox = new System.Windows.Forms.CheckBox();
         this.LiveUpdateTimer = new System.Windows.Forms.Timer(this.components);
         this.PickSkyboxButton = new System.Windows.Forms.Button();
         this.SkyBoxFileTextBox = new System.Windows.Forms.TextBox();
         this.PickEnvMapboxButton = new System.Windows.Forms.Button();
         this.EnvMapFileTextBox = new System.Windows.Forms.TextBox();
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.ExportButton = new System.Windows.Forms.Button();
         this.label3 = new System.Windows.Forms.Label();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.button4 = new System.Windows.Forms.Button();
         this.betterPropertyGrid1 = new EditorCore.BetterPropertyGrid();
         this.ClearSkyDomeButton = new System.Windows.Forms.Button();
         this.ClearTerrainEnvButton = new System.Windows.Forms.Button();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // LoadButton
         // 
         this.LoadButton.Location = new System.Drawing.Point(6, 3);
         this.LoadButton.Name = "LoadButton";
         this.LoadButton.Size = new System.Drawing.Size(118, 23);
         this.LoadButton.TabIndex = 3;
         this.LoadButton.Text = "Import GLS Settings";
         this.LoadButton.UseVisualStyleBackColor = true;
         this.LoadButton.Click += new System.EventHandler(this.Import_Click);
         // 
         // textBox1
         // 
         this.textBox1.Location = new System.Drawing.Point(502, 272);
         this.textBox1.Name = "textBox1";
         this.textBox1.ReadOnly = true;
         this.textBox1.Size = new System.Drawing.Size(251, 20);
         this.textBox1.TabIndex = 4;
         this.textBox1.Visible = false;
         // 
         // SaveButton
         // 
         this.SaveButton.Location = new System.Drawing.Point(547, 330);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(75, 23);
         this.SaveButton.TabIndex = 5;
         this.SaveButton.Text = "Save";
         this.SaveButton.UseVisualStyleBackColor = true;
         this.SaveButton.Visible = false;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // SaveAsButton
         // 
         this.SaveAsButton.Location = new System.Drawing.Point(678, 351);
         this.SaveAsButton.Name = "SaveAsButton";
         this.SaveAsButton.Size = new System.Drawing.Size(75, 23);
         this.SaveAsButton.TabIndex = 6;
         this.SaveAsButton.Text = "SaveAs";
         this.SaveAsButton.UseVisualStyleBackColor = true;
         this.SaveAsButton.Visible = false;
         this.SaveAsButton.Click += new System.EventHandler(this.SaveAsButton_Click);
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(719, 223);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 7;
         this.button1.Text = "button1";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Visible = false;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(521, 223);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 8;
         this.button2.Text = "button2";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Visible = false;
         // 
         // button3
         // 
         this.button3.Location = new System.Drawing.Point(615, 223);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(75, 23);
         this.button3.TabIndex = 9;
         this.button3.Text = "button3";
         this.button3.UseVisualStyleBackColor = true;
         this.button3.Visible = false;
         this.button3.Click += new System.EventHandler(this.button3_Click);
         // 
         // LiveUpdateCheckBox
         // 
         this.LiveUpdateCheckBox.AutoSize = true;
         this.LiveUpdateCheckBox.Location = new System.Drawing.Point(502, 171);
         this.LiveUpdateCheckBox.Name = "LiveUpdateCheckBox";
         this.LiveUpdateCheckBox.Size = new System.Drawing.Size(307, 17);
         this.LiveUpdateCheckBox.TabIndex = 10;
         this.LiveUpdateCheckBox.Text = "Fast Update (Temporary.  Use only when Active Sync is off)";
         this.LiveUpdateCheckBox.UseVisualStyleBackColor = true;
         this.LiveUpdateCheckBox.Visible = false;
         this.LiveUpdateCheckBox.CheckedChanged += new System.EventHandler(this.LiveUpdateCheckBox_CheckedChanged);
         // 
         // LiveUpdateTimer
         // 
         this.LiveUpdateTimer.Enabled = true;
         this.LiveUpdateTimer.Interval = 1000;
         this.LiveUpdateTimer.Tick += new System.EventHandler(this.LiveUpdateTimer_Tick);
         // 
         // PickSkyboxButton
         // 
         this.PickSkyboxButton.Location = new System.Drawing.Point(541, 16);
         this.PickSkyboxButton.Name = "PickSkyboxButton";
         this.PickSkyboxButton.Size = new System.Drawing.Size(26, 23);
         this.PickSkyboxButton.TabIndex = 1;
         this.PickSkyboxButton.Text = "...";
         this.PickSkyboxButton.UseVisualStyleBackColor = true;
         this.PickSkyboxButton.Click += new System.EventHandler(this.PickSkyboxButton_Click);
         // 
         // SkyBoxFileTextBox
         // 
         this.SkyBoxFileTextBox.Location = new System.Drawing.Point(138, 19);
         this.SkyBoxFileTextBox.Name = "SkyBoxFileTextBox";
         this.SkyBoxFileTextBox.Size = new System.Drawing.Size(397, 20);
         this.SkyBoxFileTextBox.TabIndex = 0;
         this.SkyBoxFileTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.SkyBoxFileTextBox_KeyPress);
         // 
         // PickEnvMapboxButton
         // 
         this.PickEnvMapboxButton.Location = new System.Drawing.Point(541, 45);
         this.PickEnvMapboxButton.Name = "PickEnvMapboxButton";
         this.PickEnvMapboxButton.Size = new System.Drawing.Size(26, 23);
         this.PickEnvMapboxButton.TabIndex = 14;
         this.PickEnvMapboxButton.Text = "...";
         this.PickEnvMapboxButton.UseVisualStyleBackColor = true;
         this.PickEnvMapboxButton.Click += new System.EventHandler(this.PickEnvMapboxButton_Click);
         // 
         // EnvMapFileTextBox
         // 
         this.EnvMapFileTextBox.Location = new System.Drawing.Point(138, 45);
         this.EnvMapFileTextBox.Name = "EnvMapFileTextBox";
         this.EnvMapFileTextBox.Size = new System.Drawing.Size(397, 20);
         this.EnvMapFileTextBox.TabIndex = 13;
         this.EnvMapFileTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.EnvMapFileTextBox_KeyPress);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(40, 24);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(92, 13);
         this.label1.TabIndex = 15;
         this.label1.Text = "Sky Dome Vis File";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(6, 50);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(126, 13);
         this.label2.TabIndex = 16;
         this.label2.Text = "Terrain Environment Map";
         // 
         // ExportButton
         // 
         this.ExportButton.Location = new System.Drawing.Point(130, 3);
         this.ExportButton.Name = "ExportButton";
         this.ExportButton.Size = new System.Drawing.Size(122, 23);
         this.ExportButton.TabIndex = 17;
         this.ExportButton.Text = "Export GLS Settings";
         this.ExportButton.UseVisualStyleBackColor = true;
         this.ExportButton.Click += new System.EventHandler(this.ExportButton_Click);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(3, 78);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(72, 13);
         this.label3.TabIndex = 18;
         this.label3.Text = "GLS Settings:";
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.ClearTerrainEnvButton);
         this.groupBox1.Controls.Add(this.ClearSkyDomeButton);
         this.groupBox1.Controls.Add(this.SkyBoxFileTextBox);
         this.groupBox1.Controls.Add(this.PickSkyboxButton);
         this.groupBox1.Controls.Add(this.EnvMapFileTextBox);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.PickEnvMapboxButton);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Location = new System.Drawing.Point(342, 3);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(643, 85);
         this.groupBox1.TabIndex = 19;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Scenario (.scn) Settings:";
         // 
         // button4
         // 
         this.button4.Location = new System.Drawing.Point(130, 32);
         this.button4.Name = "button4";
         this.button4.Size = new System.Drawing.Size(122, 23);
         this.button4.TabIndex = 19;
         this.button4.Text = "Generate FLS";
         this.button4.UseVisualStyleBackColor = true;
         this.button4.Click += new System.EventHandler(this.button4_Click);
         // 
         // betterPropertyGrid1
         // 
         this.betterPropertyGrid1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.betterPropertyGrid1.LastRowHack = false;
         this.betterPropertyGrid1.Location = new System.Drawing.Point(3, 94);
         this.betterPropertyGrid1.Name = "betterPropertyGrid1";
         this.betterPropertyGrid1.Size = new System.Drawing.Size(982, 467);
         this.betterPropertyGrid1.TabIndex = 12;
         // 
         // ClearSkyDomeButton
         // 
         this.ClearSkyDomeButton.Location = new System.Drawing.Point(573, 16);
         this.ClearSkyDomeButton.Name = "ClearSkyDomeButton";
         this.ClearSkyDomeButton.Size = new System.Drawing.Size(51, 23);
         this.ClearSkyDomeButton.TabIndex = 17;
         this.ClearSkyDomeButton.Text = "clear";
         this.ClearSkyDomeButton.UseVisualStyleBackColor = true;
         this.ClearSkyDomeButton.Click += new System.EventHandler(this.ClearSkyDomeButton_Click);
         // 
         // ClearTerrainEnvButton
         // 
         this.ClearTerrainEnvButton.Location = new System.Drawing.Point(573, 45);
         this.ClearTerrainEnvButton.Name = "ClearTerrainEnvButton";
         this.ClearTerrainEnvButton.Size = new System.Drawing.Size(51, 23);
         this.ClearTerrainEnvButton.TabIndex = 18;
         this.ClearTerrainEnvButton.Text = "clear";
         this.ClearTerrainEnvButton.UseVisualStyleBackColor = true;
         this.ClearTerrainEnvButton.Click += new System.EventHandler(this.ClearTerrainEnvButton_Click);
         // 
         // TempLightSettings
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.button4);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.ExportButton);
         this.Controls.Add(this.betterPropertyGrid1);
         this.Controls.Add(this.LiveUpdateCheckBox);
         this.Controls.Add(this.button3);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.SaveAsButton);
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.textBox1);
         this.Controls.Add(this.LoadButton);
         this.Name = "TempLightSettings";
         this.Size = new System.Drawing.Size(1001, 580);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button LoadButton;
      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.Button SaveButton;
      private System.Windows.Forms.Button SaveAsButton;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.Button button3;
      private System.Windows.Forms.CheckBox LiveUpdateCheckBox;
      private System.Windows.Forms.Timer LiveUpdateTimer;
      private System.Windows.Forms.Button PickSkyboxButton;
      private System.Windows.Forms.TextBox SkyBoxFileTextBox;
      private EditorCore.BetterPropertyGrid betterPropertyGrid1;
      private System.Windows.Forms.Button PickEnvMapboxButton;
      private System.Windows.Forms.TextBox EnvMapFileTextBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Button ExportButton;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Button button4;
      private System.Windows.Forms.Button ClearTerrainEnvButton;
      private System.Windows.Forms.Button ClearSkyDomeButton;

   }
}