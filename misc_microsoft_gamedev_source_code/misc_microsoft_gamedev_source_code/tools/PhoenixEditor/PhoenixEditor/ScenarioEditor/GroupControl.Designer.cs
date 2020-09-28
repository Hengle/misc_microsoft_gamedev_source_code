namespace PhoenixEditor.ScenarioEditor
{
   partial class GroupControl
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
         this.GoupInputHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.GroupOutpuHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.panel1 = new System.Windows.Forms.Panel();
         this.GroupTitleTextBox = new System.Windows.Forms.TextBox();
         this.panel2 = new System.Windows.Forms.Panel();
         this.SharedVariableListBox = new System.Windows.Forms.ListBox();
         this.label1 = new System.Windows.Forms.Label();
         this.ResizePanel = new System.Windows.Forms.Panel();
         this.panel1.SuspendLayout();
         this.panel2.SuspendLayout();
         this.SuspendLayout();
         // 
         // GoupInputHardPointsBar
         // 
         this.GoupInputHardPointsBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.GoupInputHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.GoupInputHardPointsBar.HorizontalLayout = false;
         this.GoupInputHardPointsBar.Location = new System.Drawing.Point(0, 0);
         this.GoupInputHardPointsBar.Margin = new System.Windows.Forms.Padding(0);
         this.GoupInputHardPointsBar.MarginSize = 25;
         this.GoupInputHardPointsBar.Name = "GoupInputHardPointsBar";
         this.GoupInputHardPointsBar.Size = new System.Drawing.Size(16, 203);
         this.GoupInputHardPointsBar.TabIndex = 0;
         // 
         // GroupOutpuHardPointsBar
         // 
         this.GroupOutpuHardPointsBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.GroupOutpuHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.GroupOutpuHardPointsBar.HorizontalLayout = false;
         this.GroupOutpuHardPointsBar.Location = new System.Drawing.Point(238, 0);
         this.GroupOutpuHardPointsBar.Margin = new System.Windows.Forms.Padding(0);
         this.GroupOutpuHardPointsBar.MarginSize = 25;
         this.GroupOutpuHardPointsBar.Name = "GroupOutpuHardPointsBar";
         this.GroupOutpuHardPointsBar.Size = new System.Drawing.Size(16, 203);
         this.GroupOutpuHardPointsBar.TabIndex = 1;
         // 
         // panel1
         // 
         this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panel1.BackColor = System.Drawing.Color.NavajoWhite;
         this.panel1.Controls.Add(this.GroupTitleTextBox);
         this.panel1.Location = new System.Drawing.Point(16, 0);
         this.panel1.Margin = new System.Windows.Forms.Padding(0);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(222, 31);
         this.panel1.TabIndex = 3;
         // 
         // GroupTitleTextBox
         // 
         this.GroupTitleTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.GroupTitleTextBox.BackColor = System.Drawing.Color.NavajoWhite;
         this.GroupTitleTextBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.GroupTitleTextBox.Location = new System.Drawing.Point(3, 6);
         this.GroupTitleTextBox.Name = "GroupTitleTextBox";
         this.GroupTitleTextBox.Size = new System.Drawing.Size(216, 22);
         this.GroupTitleTextBox.TabIndex = 3;
         this.GroupTitleTextBox.TextChanged += new System.EventHandler(this.GroupTitleTextBox_TextChanged);
         // 
         // panel2
         // 
         this.panel2.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panel2.BackColor = System.Drawing.Color.OldLace;
         this.panel2.Controls.Add(this.SharedVariableListBox);
         this.panel2.Controls.Add(this.label1);
         this.panel2.Location = new System.Drawing.Point(16, 31);
         this.panel2.Margin = new System.Windows.Forms.Padding(0);
         this.panel2.Name = "panel2";
         this.panel2.Size = new System.Drawing.Size(221, 172);
         this.panel2.TabIndex = 4;
         // 
         // SharedVariableListBox
         // 
         this.SharedVariableListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.SharedVariableListBox.FormattingEnabled = true;
         this.SharedVariableListBox.Location = new System.Drawing.Point(3, 26);
         this.SharedVariableListBox.Name = "SharedVariableListBox";
         this.SharedVariableListBox.Size = new System.Drawing.Size(215, 134);
         this.SharedVariableListBox.TabIndex = 1;
         this.SharedVariableListBox.Visible = false;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(3, 10);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(87, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "Shared Variables";
         this.label1.Visible = false;
         // 
         // ResizePanel
         // 
         this.ResizePanel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.ResizePanel.BackColor = System.Drawing.Color.NavajoWhite;
         this.ResizePanel.Location = new System.Drawing.Point(215, 184);
         this.ResizePanel.Margin = new System.Windows.Forms.Padding(0);
         this.ResizePanel.Name = "ResizePanel";
         this.ResizePanel.Size = new System.Drawing.Size(22, 19);
         this.ResizePanel.TabIndex = 2;
         // 
         // GroupControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.ResizePanel);
         this.Controls.Add(this.panel2);
         this.Controls.Add(this.panel1);
         this.Controls.Add(this.GroupOutpuHardPointsBar);
         this.Controls.Add(this.GoupInputHardPointsBar);
         this.Name = "GroupControl";
         this.Size = new System.Drawing.Size(254, 203);
         this.panel1.ResumeLayout(false);
         this.panel1.PerformLayout();
         this.panel2.ResumeLayout(false);
         this.panel2.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private HardPointsBar GoupInputHardPointsBar;
      private HardPointsBar GroupOutpuHardPointsBar;
      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.Panel panel2;
      private System.Windows.Forms.ListBox SharedVariableListBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.TextBox GroupTitleTextBox;
      private System.Windows.Forms.Panel ResizePanel;
   }
}
