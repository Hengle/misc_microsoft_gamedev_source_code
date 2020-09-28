namespace PhoenixEditor
{
   partial class NewScenario
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
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.simXTrackbar = new System.Windows.Forms.TrackBar();
         this.simZTrackbar = new System.Windows.Forms.TrackBar();
         this.NumXSimTilesLabel = new System.Windows.Forms.Label();
         this.NumZSimTilesLabel = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.label4 = new System.Windows.Forms.Label();
         this.simTileSpacingBar = new System.Windows.Forms.TrackBar();
         this.simTileSpacingLabel = new System.Windows.Forms.Label();
         this.VisualDensityBar = new System.Windows.Forms.TrackBar();
         this.visualDensityLabel = new System.Windows.Forms.Label();
         this.buttonOK = new System.Windows.Forms.Button();
         this.buttonNO = new System.Windows.Forms.Button();
         this.worldSpaceLabel = new System.Windows.Forms.Label();
         ((System.ComponentModel.ISupportInitialize)(this.simXTrackbar)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.simZTrackbar)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.simTileSpacingBar)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.VisualDensityBar)).BeginInit();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(31, 37);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(65, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "Sim X Tiles :";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(31, 64);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(65, 13);
         this.label2.TabIndex = 1;
         this.label2.Text = "Sim Z Tiles :";
         // 
         // simXTrackbar
         // 
         this.simXTrackbar.AutoSize = false;
         this.simXTrackbar.LargeChange = 1;
         this.simXTrackbar.Location = new System.Drawing.Point(142, 29);
         this.simXTrackbar.Name = "simXTrackbar";
         this.simXTrackbar.Size = new System.Drawing.Size(104, 21);
         this.simXTrackbar.TabIndex = 2;
         this.simXTrackbar.Scroll += new System.EventHandler(this.simXTrackbar_Scroll);
         // 
         // simZTrackbar
         // 
         this.simZTrackbar.AutoSize = false;
         this.simZTrackbar.LargeChange = 1;
         this.simZTrackbar.Location = new System.Drawing.Point(142, 56);
         this.simZTrackbar.Name = "simZTrackbar";
         this.simZTrackbar.Size = new System.Drawing.Size(104, 21);
         this.simZTrackbar.TabIndex = 3;
         this.simZTrackbar.Scroll += new System.EventHandler(this.trackBar1_Scroll);
         // 
         // NumXSimTilesLabel
         // 
         this.NumXSimTilesLabel.AutoSize = true;
         this.NumXSimTilesLabel.Location = new System.Drawing.Point(252, 37);
         this.NumXSimTilesLabel.Name = "NumXSimTilesLabel";
         this.NumXSimTilesLabel.Size = new System.Drawing.Size(13, 13);
         this.NumXSimTilesLabel.TabIndex = 4;
         this.NumXSimTilesLabel.Text = "0";
         // 
         // NumZSimTilesLabel
         // 
         this.NumZSimTilesLabel.AutoSize = true;
         this.NumZSimTilesLabel.Location = new System.Drawing.Point(252, 64);
         this.NumZSimTilesLabel.Name = "NumZSimTilesLabel";
         this.NumZSimTilesLabel.Size = new System.Drawing.Size(13, 13);
         this.NumZSimTilesLabel.TabIndex = 5;
         this.NumZSimTilesLabel.Text = "0";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(31, 90);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(73, 13);
         this.label3.TabIndex = 6;
         this.label3.Text = "Sim Tile Size :";
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(31, 144);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(79, 13);
         this.label4.TabIndex = 7;
         this.label4.Text = "Visual Density :";
         // 
         // simTileSpacingBar
         // 
         this.simTileSpacingBar.AutoSize = false;
         this.simTileSpacingBar.LargeChange = 1;
         this.simTileSpacingBar.Location = new System.Drawing.Point(142, 83);
         this.simTileSpacingBar.Name = "simTileSpacingBar";
         this.simTileSpacingBar.Size = new System.Drawing.Size(104, 21);
         this.simTileSpacingBar.TabIndex = 10;
         this.simTileSpacingBar.Scroll += new System.EventHandler(this.simTileSpacingBar_Scroll);
         // 
         // simTileSpacingLabel
         // 
         this.simTileSpacingLabel.AutoSize = true;
         this.simTileSpacingLabel.Location = new System.Drawing.Point(252, 90);
         this.simTileSpacingLabel.Name = "simTileSpacingLabel";
         this.simTileSpacingLabel.Size = new System.Drawing.Size(13, 13);
         this.simTileSpacingLabel.TabIndex = 11;
         this.simTileSpacingLabel.Text = "0";
         // 
         // VisualDensityBar
         // 
         this.VisualDensityBar.AutoSize = false;
         this.VisualDensityBar.LargeChange = 1;
         this.VisualDensityBar.Location = new System.Drawing.Point(142, 136);
         this.VisualDensityBar.Name = "VisualDensityBar";
         this.VisualDensityBar.Size = new System.Drawing.Size(104, 21);
         this.VisualDensityBar.TabIndex = 12;
         this.VisualDensityBar.Scroll += new System.EventHandler(this.VisualDensityBar_Scroll);
         // 
         // visualDensityLabel
         // 
         this.visualDensityLabel.AutoSize = true;
         this.visualDensityLabel.Location = new System.Drawing.Point(252, 139);
         this.visualDensityLabel.Name = "visualDensityLabel";
         this.visualDensityLabel.Size = new System.Drawing.Size(13, 13);
         this.visualDensityLabel.TabIndex = 13;
         this.visualDensityLabel.Text = "0";
         // 
         // buttonOK
         // 
         this.buttonOK.Location = new System.Drawing.Point(209, 232);
         this.buttonOK.Name = "buttonOK";
         this.buttonOK.Size = new System.Drawing.Size(75, 23);
         this.buttonOK.TabIndex = 14;
         this.buttonOK.Text = "Create";
         this.buttonOK.UseVisualStyleBackColor = true;
         this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
         // 
         // buttonNO
         // 
         this.buttonNO.Location = new System.Drawing.Point(290, 232);
         this.buttonNO.Name = "buttonNO";
         this.buttonNO.Size = new System.Drawing.Size(75, 23);
         this.buttonNO.TabIndex = 15;
         this.buttonNO.Text = "Cancel";
         this.buttonNO.UseVisualStyleBackColor = true;
         this.buttonNO.Click += new System.EventHandler(this.buttonNO_Click);
         // 
         // worldSpaceLabel
         // 
         this.worldSpaceLabel.AutoSize = true;
         this.worldSpaceLabel.Location = new System.Drawing.Point(31, 116);
         this.worldSpaceLabel.Name = "worldSpaceLabel";
         this.worldSpaceLabel.Size = new System.Drawing.Size(13, 13);
         this.worldSpaceLabel.TabIndex = 16;
         this.worldSpaceLabel.Text = "0";
         // 
         // NewScenario
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(377, 267);
         this.Controls.Add(this.worldSpaceLabel);
         this.Controls.Add(this.buttonNO);
         this.Controls.Add(this.buttonOK);
         this.Controls.Add(this.visualDensityLabel);
         this.Controls.Add(this.VisualDensityBar);
         this.Controls.Add(this.simTileSpacingLabel);
         this.Controls.Add(this.simTileSpacingBar);
         this.Controls.Add(this.label4);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.NumZSimTilesLabel);
         this.Controls.Add(this.NumXSimTilesLabel);
         this.Controls.Add(this.simZTrackbar);
         this.Controls.Add(this.simXTrackbar);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Name = "NewScenario";
         this.Text = "New Custom Scenario";
         this.Load += new System.EventHandler(this.NewScenario_Load);
         ((System.ComponentModel.ISupportInitialize)(this.simXTrackbar)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.simZTrackbar)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.simTileSpacingBar)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.VisualDensityBar)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.TrackBar simXTrackbar;
      private System.Windows.Forms.TrackBar simZTrackbar;
      private System.Windows.Forms.Label NumXSimTilesLabel;
      private System.Windows.Forms.Label NumZSimTilesLabel;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.TrackBar simTileSpacingBar;
      private System.Windows.Forms.Label simTileSpacingLabel;
      private System.Windows.Forms.TrackBar VisualDensityBar;
      private System.Windows.Forms.Label visualDensityLabel;
      private System.Windows.Forms.Button buttonOK;
      private System.Windows.Forms.Button buttonNO;
      private System.Windows.Forms.Label worldSpaceLabel;
   }
}