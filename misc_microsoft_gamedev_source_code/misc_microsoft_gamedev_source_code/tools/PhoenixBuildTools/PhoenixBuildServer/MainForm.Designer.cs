namespace PhoenixBuildServer
{
   partial class MainForm
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
         this.mStartButton = new System.Windows.Forms.Button();
         this.mStatusLabel = new System.Windows.Forms.Label();
         this.mServerStatusTextBox = new System.Windows.Forms.TextBox();
         this.mBuildTimer = new System.Windows.Forms.Timer(this.components);
         this.mConsoleStatusTextBox = new System.Windows.Forms.TextBox();
         this.SuspendLayout();
         // 
         // mStartButton
         // 
         this.mStartButton.Location = new System.Drawing.Point(12, 10);
         this.mStartButton.Name = "mStartButton";
         this.mStartButton.Size = new System.Drawing.Size(75, 23);
         this.mStartButton.TabIndex = 0;
         this.mStartButton.Text = "Start";
         this.mStartButton.UseVisualStyleBackColor = true;
         this.mStartButton.Click += new System.EventHandler(this.mStartButton_Click);
         // 
         // mStatusLabel
         // 
         this.mStatusLabel.AutoSize = true;
         this.mStatusLabel.Location = new System.Drawing.Point(111, 15);
         this.mStatusLabel.Name = "mStatusLabel";
         this.mStatusLabel.Size = new System.Drawing.Size(40, 13);
         this.mStatusLabel.TabIndex = 1;
         this.mStatusLabel.Text = "Status:";
         // 
         // mServerStatusTextBox
         // 
         this.mServerStatusTextBox.Location = new System.Drawing.Point(157, 12);
         this.mServerStatusTextBox.Name = "mServerStatusTextBox";
         this.mServerStatusTextBox.ReadOnly = true;
         this.mServerStatusTextBox.Size = new System.Drawing.Size(535, 20);
         this.mServerStatusTextBox.TabIndex = 2;
         // 
         // mBuildTimer
         // 
         this.mBuildTimer.Interval = 1000;
         this.mBuildTimer.Tick += new System.EventHandler(this.mBuildTimer_Tick);
         // 
         // mConsoleStatusTextBox
         // 
         this.mConsoleStatusTextBox.BackColor = System.Drawing.Color.Black;
         this.mConsoleStatusTextBox.Font = new System.Drawing.Font("Lucida Console", 8F);
         this.mConsoleStatusTextBox.ForeColor = System.Drawing.Color.Lime;
         this.mConsoleStatusTextBox.Location = new System.Drawing.Point(12, 39);
         this.mConsoleStatusTextBox.Multiline = true;
         this.mConsoleStatusTextBox.Name = "mConsoleStatusTextBox";
         this.mConsoleStatusTextBox.ReadOnly = true;
         this.mConsoleStatusTextBox.Size = new System.Drawing.Size(680, 406);
         this.mConsoleStatusTextBox.TabIndex = 4;
         this.mConsoleStatusTextBox.WordWrap = false;
         // 
         // MainForm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(704, 457);
         this.Controls.Add(this.mConsoleStatusTextBox);
         this.Controls.Add(this.mServerStatusTextBox);
         this.Controls.Add(this.mStatusLabel);
         this.Controls.Add(this.mStartButton);
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
         this.Name = "MainForm";
         this.Text = "PhoenixBuildServer";
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button mStartButton;
      private System.Windows.Forms.Label mStatusLabel;
      private System.Windows.Forms.TextBox mServerStatusTextBox;
      private System.Windows.Forms.Timer mBuildTimer;
      private System.Windows.Forms.TextBox mConsoleStatusTextBox;
   }
}

