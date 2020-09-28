namespace PhoenixEditor.ScenarioEditor
{
   partial class TriggerControl
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
         this.panel1 = new System.Windows.Forms.Panel();
         this.ReNameCancelButton = new System.Windows.Forms.Button();
         this.ReNameOKButton = new System.Windows.Forms.Button();
         this.ReNameTextBox = new System.Windows.Forms.TextBox();
         this.TriggerIDLabel = new System.Windows.Forms.Label();
         this.ResizeButton = new System.Windows.Forms.Button();
         this.TriggerTitleLabel = new System.Windows.Forms.Label();
         this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
         this.VarOutputHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.VarInputHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.TriggerInHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.TriggerOutHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.conditionsList1 = new PhoenixEditor.ScenarioEditor.ConditionsList();
         this.effectsList1 = new PhoenixEditor.ScenarioEditor.EffectsList();
         this.effectsList2 = new PhoenixEditor.ScenarioEditor.EffectsList();
         this.panel1.SuspendLayout();
         this.tableLayoutPanel1.SuspendLayout();
         this.SuspendLayout();
         // 
         // panel1
         // 
         this.panel1.BackColor = System.Drawing.SystemColors.ControlLight;
         this.tableLayoutPanel1.SetColumnSpan(this.panel1, 2);
         this.panel1.Controls.Add(this.ReNameCancelButton);
         this.panel1.Controls.Add(this.ReNameOKButton);
         this.panel1.Controls.Add(this.ReNameTextBox);
         this.panel1.Controls.Add(this.TriggerIDLabel);
         this.panel1.Controls.Add(this.ResizeButton);
         this.panel1.Controls.Add(this.TriggerTitleLabel);
         this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.panel1.Location = new System.Drawing.Point(16, 2);
         this.panel1.Margin = new System.Windows.Forms.Padding(0);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(364, 25);
         this.panel1.TabIndex = 0;
         this.panel1.MouseUp += new System.Windows.Forms.MouseEventHandler(this.panel1_MouseUp);
         // 
         // ReNameCancelButton
         // 
         this.ReNameCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.ReNameCancelButton.Location = new System.Drawing.Point(306, 1);
         this.ReNameCancelButton.Name = "ReNameCancelButton";
         this.ReNameCancelButton.Size = new System.Drawing.Size(53, 23);
         this.ReNameCancelButton.TabIndex = 5;
         this.ReNameCancelButton.Text = "Cancel";
         this.ReNameCancelButton.UseVisualStyleBackColor = true;
         this.ReNameCancelButton.Visible = false;
         this.ReNameCancelButton.Click += new System.EventHandler(this.ReNameCancelButton_Click);
         // 
         // ReNameOKButton
         // 
         this.ReNameOKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.ReNameOKButton.Location = new System.Drawing.Point(272, 1);
         this.ReNameOKButton.Name = "ReNameOKButton";
         this.ReNameOKButton.Size = new System.Drawing.Size(31, 23);
         this.ReNameOKButton.TabIndex = 4;
         this.ReNameOKButton.Text = "OK";
         this.ReNameOKButton.UseVisualStyleBackColor = true;
         this.ReNameOKButton.Visible = false;
         this.ReNameOKButton.Click += new System.EventHandler(this.ReNameOKButton_Click);
         // 
         // ReNameTextBox
         // 
         this.ReNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.ReNameTextBox.Location = new System.Drawing.Point(6, 2);
         this.ReNameTextBox.Name = "ReNameTextBox";
         this.ReNameTextBox.Size = new System.Drawing.Size(260, 20);
         this.ReNameTextBox.TabIndex = 3;
         this.ReNameTextBox.Visible = false;
         this.ReNameTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.ReNameTextBox_KeyPress);
         // 
         // TriggerIDLabel
         // 
         this.TriggerIDLabel.AutoSize = true;
         this.TriggerIDLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.TriggerIDLabel.Location = new System.Drawing.Point(3, 6);
         this.TriggerIDLabel.Margin = new System.Windows.Forms.Padding(3, 0, 0, 0);
         this.TriggerIDLabel.Name = "TriggerIDLabel";
         this.TriggerIDLabel.Size = new System.Drawing.Size(41, 13);
         this.TriggerIDLabel.TabIndex = 2;
         this.TriggerIDLabel.Text = "label1";
         // 
         // ResizeButton
         // 
         this.ResizeButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.ResizeButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
         this.ResizeButton.Location = new System.Drawing.Point(330, 3);
         this.ResizeButton.Name = "ResizeButton";
         this.ResizeButton.Size = new System.Drawing.Size(25, 19);
         this.ResizeButton.TabIndex = 1;
         this.ResizeButton.Text = "V";
         this.ResizeButton.UseVisualStyleBackColor = true;
         this.ResizeButton.Click += new System.EventHandler(this.ResizeButton_Click);
         // 
         // TriggerTitleLabel
         // 
         this.TriggerTitleLabel.AutoEllipsis = true;
         this.TriggerTitleLabel.AutoSize = true;
         this.TriggerTitleLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.TriggerTitleLabel.Location = new System.Drawing.Point(91, 6);
         this.TriggerTitleLabel.Name = "TriggerTitleLabel";
         this.TriggerTitleLabel.Size = new System.Drawing.Size(99, 13);
         this.TriggerTitleLabel.TabIndex = 0;
         this.TriggerTitleLabel.Text = "label1dsfsdfsdfa";
         this.TriggerTitleLabel.TextAlign = System.Drawing.ContentAlignment.TopCenter;
         this.TriggerTitleLabel.MouseLeave += new System.EventHandler(this.TriggerTitleLabel_MouseLeave);
         this.TriggerTitleLabel.Click += new System.EventHandler(this.TriggerTitleLabel_Click);
         this.TriggerTitleLabel.MouseEnter += new System.EventHandler(this.TriggerTitleLabel_MouseEnter);
         // 
         // tableLayoutPanel1
         // 
         this.tableLayoutPanel1.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.tableLayoutPanel1.ColumnCount = 4;
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 16F));
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 16F));
         this.tableLayoutPanel1.Controls.Add(this.panel1, 1, 1);
         this.tableLayoutPanel1.Controls.Add(this.VarOutputHardPointsBar, 0, 4);
         this.tableLayoutPanel1.Controls.Add(this.VarInputHardPointsBar, 0, 0);
         this.tableLayoutPanel1.Controls.Add(this.TriggerInHardPointsBar, 0, 1);
         this.tableLayoutPanel1.Controls.Add(this.TriggerOutHardPointsBar, 3, 1);
         this.tableLayoutPanel1.Controls.Add(this.conditionsList1, 1, 2);
         this.tableLayoutPanel1.Controls.Add(this.effectsList1, 2, 2);
         this.tableLayoutPanel1.Controls.Add(this.effectsList2, 2, 3);
         this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
         this.tableLayoutPanel1.Name = "tableLayoutPanel1";
         this.tableLayoutPanel1.RowCount = 5;
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 2F));
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 25F));
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 2F));
         this.tableLayoutPanel1.Size = new System.Drawing.Size(396, 105);
         this.tableLayoutPanel1.TabIndex = 1;
         // 
         // VarOutputHardPointsBar
         // 
         this.VarOutputHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.tableLayoutPanel1.SetColumnSpan(this.VarOutputHardPointsBar, 4);
         this.VarOutputHardPointsBar.Dock = System.Windows.Forms.DockStyle.Fill;
         this.VarOutputHardPointsBar.HorizontalLayout = true;
         this.VarOutputHardPointsBar.Location = new System.Drawing.Point(0, 101);
         this.VarOutputHardPointsBar.Margin = new System.Windows.Forms.Padding(0);
         this.VarOutputHardPointsBar.MarginSize = 25;
         this.VarOutputHardPointsBar.Name = "VarOutputHardPointsBar";
         this.VarOutputHardPointsBar.Size = new System.Drawing.Size(396, 4);
         this.VarOutputHardPointsBar.TabIndex = 1;
         // 
         // VarInputHardPointsBar
         // 
         this.VarInputHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.tableLayoutPanel1.SetColumnSpan(this.VarInputHardPointsBar, 4);
         this.VarInputHardPointsBar.Dock = System.Windows.Forms.DockStyle.Fill;
         this.VarInputHardPointsBar.HorizontalLayout = true;
         this.VarInputHardPointsBar.Location = new System.Drawing.Point(0, 0);
         this.VarInputHardPointsBar.Margin = new System.Windows.Forms.Padding(0);
         this.VarInputHardPointsBar.MarginSize = 25;
         this.VarInputHardPointsBar.Name = "VarInputHardPointsBar";
         this.VarInputHardPointsBar.Size = new System.Drawing.Size(396, 2);
         this.VarInputHardPointsBar.TabIndex = 2;
         // 
         // TriggerInHardPointsBar
         // 
         this.TriggerInHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.TriggerInHardPointsBar.Dock = System.Windows.Forms.DockStyle.Fill;
         this.TriggerInHardPointsBar.HorizontalLayout = false;
         this.TriggerInHardPointsBar.Location = new System.Drawing.Point(0, 2);
         this.TriggerInHardPointsBar.Margin = new System.Windows.Forms.Padding(0);
         this.TriggerInHardPointsBar.MarginSize = 0;
         this.TriggerInHardPointsBar.Name = "TriggerInHardPointsBar";
         this.tableLayoutPanel1.SetRowSpan(this.TriggerInHardPointsBar, 3);
         this.TriggerInHardPointsBar.Size = new System.Drawing.Size(16, 99);
         this.TriggerInHardPointsBar.TabIndex = 3;
         // 
         // TriggerOutHardPointsBar
         // 
         this.TriggerOutHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.TriggerOutHardPointsBar.Dock = System.Windows.Forms.DockStyle.Fill;
         this.TriggerOutHardPointsBar.HorizontalLayout = false;
         this.TriggerOutHardPointsBar.Location = new System.Drawing.Point(380, 2);
         this.TriggerOutHardPointsBar.Margin = new System.Windows.Forms.Padding(0);
         this.TriggerOutHardPointsBar.MarginSize = 0;
         this.TriggerOutHardPointsBar.Name = "TriggerOutHardPointsBar";
         this.tableLayoutPanel1.SetRowSpan(this.TriggerOutHardPointsBar, 3);
         this.TriggerOutHardPointsBar.Size = new System.Drawing.Size(16, 99);
         this.TriggerOutHardPointsBar.TabIndex = 4;
         // 
         // conditionsList1
         // 
         this.conditionsList1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.conditionsList1.GridColor = System.Drawing.SystemColors.ControlText;
         this.conditionsList1.Location = new System.Drawing.Point(16, 27);
         this.conditionsList1.Margin = new System.Windows.Forms.Padding(0);
         this.conditionsList1.Name = "conditionsList1";
         this.tableLayoutPanel1.SetRowSpan(this.conditionsList1, 2);
         this.conditionsList1.Size = new System.Drawing.Size(182, 74);
         this.conditionsList1.TabIndex = 5;
         // 
         // effectsList1
         // 
         this.effectsList1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.effectsList1.GridColor = System.Drawing.Color.LightSkyBlue;
         this.effectsList1.Location = new System.Drawing.Point(198, 27);
         this.effectsList1.Margin = new System.Windows.Forms.Padding(0);
         this.effectsList1.Name = "effectsList1";
         this.effectsList1.Size = new System.Drawing.Size(182, 42);
         this.effectsList1.TabIndex = 6;
         // 
         // effectsList2
         // 
         this.effectsList2.Dock = System.Windows.Forms.DockStyle.Fill;
         this.effectsList2.GridColor = System.Drawing.Color.LightSalmon;
         this.effectsList2.Location = new System.Drawing.Point(198, 69);
         this.effectsList2.Margin = new System.Windows.Forms.Padding(0);
         this.effectsList2.Name = "effectsList2";
         this.effectsList2.Size = new System.Drawing.Size(182, 32);
         this.effectsList2.TabIndex = 7;
         // 
         // TriggerControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.Controls.Add(this.tableLayoutPanel1);
         this.Name = "TriggerControl";
         this.Size = new System.Drawing.Size(396, 105);
         this.panel1.ResumeLayout(false);
         this.panel1.PerformLayout();
         this.tableLayoutPanel1.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
      private System.Windows.Forms.Label TriggerTitleLabel;
      private HardPointsBar VarOutputHardPointsBar;
      private HardPointsBar VarInputHardPointsBar;
      private HardPointsBar TriggerInHardPointsBar;
      private HardPointsBar TriggerOutHardPointsBar;
      private System.Windows.Forms.Button ResizeButton;
      private ConditionsList conditionsList1;
      private EffectsList effectsList1;
      private System.Windows.Forms.Label TriggerIDLabel;
      private System.Windows.Forms.Button ReNameCancelButton;
      private System.Windows.Forms.Button ReNameOKButton;
      private System.Windows.Forms.TextBox ReNameTextBox;
      private EffectsList effectsList2;

   }
}
