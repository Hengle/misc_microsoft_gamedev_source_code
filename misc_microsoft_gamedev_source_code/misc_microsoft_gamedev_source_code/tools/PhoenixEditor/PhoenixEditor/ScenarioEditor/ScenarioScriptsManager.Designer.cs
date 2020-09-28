namespace PhoenixEditor.ScenarioEditor
{
   partial class ScenarioScriptsManager
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
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.bakeFinalButton = new System.Windows.Forms.Button();
         this.buildModeCombo = new System.Windows.Forms.ComboBox();
         this.label3 = new System.Windows.Forms.Label();
         this.label4 = new System.Windows.Forms.Label();
         this.label5 = new System.Windows.Forms.Label();
         this.label6 = new System.Windows.Forms.Label();
         this.InternalScriptsList = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.ExternalScriptsList = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.bakeGroupBox = new System.Windows.Forms.GroupBox();
         this.bakeGroupBox.SuspendLayout();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(490, 180);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(80, 13);
         this.label1.TabIndex = 1;
         this.label1.Text = "External Scripts";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(6, 180);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(77, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = "Internal Scripts";
         // 
         // bakeFinalButton
         // 
         this.bakeFinalButton.Location = new System.Drawing.Point(173, 33);
         this.bakeFinalButton.Name = "bakeFinalButton";
         this.bakeFinalButton.Size = new System.Drawing.Size(104, 23);
         this.bakeFinalButton.TabIndex = 4;
         this.bakeFinalButton.Text = "Bake Script";
         this.bakeFinalButton.UseVisualStyleBackColor = true;
         this.bakeFinalButton.Click += new System.EventHandler(this.bakeFinalButton_Click);
         // 
         // buildModeCombo
         // 
         this.buildModeCombo.FormattingEnabled = true;
         this.buildModeCombo.Location = new System.Drawing.Point(17, 35);
         this.buildModeCombo.Name = "buildModeCombo";
         this.buildModeCombo.Size = new System.Drawing.Size(150, 21);
         this.buildModeCombo.TabIndex = 5;
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Font = new System.Drawing.Font("Microsoft Sans Serif", 15.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label3.ForeColor = System.Drawing.SystemColors.ActiveCaption;
         this.label3.Location = new System.Drawing.Point(12, 68);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(936, 25);
         this.label3.TabIndex = 6;
         this.label3.Text = "Instructions: Save first if you made any changes.  Pick build mode.  Press Bake S" +
             "cript.  ";
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(17, 21);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(58, 13);
         this.label4.TabIndex = 7;
         this.label4.Text = "build mode";
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label5.ForeColor = System.Drawing.Color.Red;
         this.label5.Location = new System.Drawing.Point(25, 103);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(451, 24);
         this.label5.TabIndex = 8;
         this.label5.Text = "note: you must re-bake after any normal saving.";
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label6.ForeColor = System.Drawing.Color.Red;
         this.label6.Location = new System.Drawing.Point(40, 127);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(403, 24);
         this.label6.TabIndex = 9;
         this.label6.Text = "       normal save overwrites the baked file.";
         // 
         // InternalScriptsList
         // 
         this.InternalScriptsList.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.InternalScriptsList.Location = new System.Drawing.Point(9, 196);
         this.InternalScriptsList.Name = "InternalScriptsList";
         this.InternalScriptsList.Size = new System.Drawing.Size(389, 456);
         this.InternalScriptsList.TabIndex = 3;
         this.InternalScriptsList.UseLabels = true;
         this.InternalScriptsList.WrapContents = false;
         // 
         // ExternalScriptsList
         // 
         this.ExternalScriptsList.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.ExternalScriptsList.Location = new System.Drawing.Point(490, 196);
         this.ExternalScriptsList.Name = "ExternalScriptsList";
         this.ExternalScriptsList.Size = new System.Drawing.Size(389, 456);
         this.ExternalScriptsList.TabIndex = 0;
         this.ExternalScriptsList.UseLabels = true;
         this.ExternalScriptsList.WrapContents = false;
         // 
         // bakeGroupBox
         // 
         this.bakeGroupBox.Controls.Add(this.buildModeCombo);
         this.bakeGroupBox.Controls.Add(this.label6);
         this.bakeGroupBox.Controls.Add(this.bakeFinalButton);
         this.bakeGroupBox.Controls.Add(this.label5);
         this.bakeGroupBox.Controls.Add(this.label3);
         this.bakeGroupBox.Controls.Add(this.label4);
         this.bakeGroupBox.Location = new System.Drawing.Point(9, 3);
         this.bakeGroupBox.Name = "bakeGroupBox";
         this.bakeGroupBox.Size = new System.Drawing.Size(940, 163);
         this.bakeGroupBox.TabIndex = 10;
         this.bakeGroupBox.TabStop = false;
         // 
         // ScenarioScriptsManager
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.bakeGroupBox);
         this.Controls.Add(this.InternalScriptsList);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.ExternalScriptsList);
         this.Name = "ScenarioScriptsManager";
         this.Size = new System.Drawing.Size(969, 671);
         this.bakeGroupBox.ResumeLayout(false);
         this.bakeGroupBox.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private BasicTypedSuperList ExternalScriptsList;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private BasicTypedSuperList InternalScriptsList;
      private System.Windows.Forms.Button bakeFinalButton;
      private System.Windows.Forms.ComboBox buildModeCombo;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.Label label6;
      private System.Windows.Forms.GroupBox bakeGroupBox;

   }
}
