namespace PhoenixEditor.ScenarioEditor
{
   partial class DesignerObjectsPanel
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
         this.PlaceSphereButton = new System.Windows.Forms.Button();
         this.PlacePathStartButton = new System.Windows.Forms.Button();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.PlacePointButton = new System.Windows.Forms.Button();
         this.EditLinesButton = new System.Windows.Forms.Button();
         this.AddPointsButton = new System.Windows.Forms.Button();
         this.InsertPointButton = new System.Windows.Forms.Button();
         this.CloseLineLoopButton = new System.Windows.Forms.Button();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // PlaceSphereButton
         // 
         this.PlaceSphereButton.Location = new System.Drawing.Point(7, 19);
         this.PlaceSphereButton.Name = "PlaceSphereButton";
         this.PlaceSphereButton.Size = new System.Drawing.Size(105, 23);
         this.PlaceSphereButton.TabIndex = 0;
         this.PlaceSphereButton.Text = "Place Sphere";
         this.PlaceSphereButton.UseVisualStyleBackColor = true;
         this.PlaceSphereButton.Click += new System.EventHandler(this.PlaceSphereButton_Click);
         // 
         // PlacePathStartButton
         // 
         this.PlacePathStartButton.Location = new System.Drawing.Point(7, 77);
         this.PlacePathStartButton.Name = "PlacePathStartButton";
         this.PlacePathStartButton.Size = new System.Drawing.Size(105, 23);
         this.PlacePathStartButton.TabIndex = 1;
         this.PlacePathStartButton.Text = "Place Path Start";
         this.PlacePathStartButton.UseVisualStyleBackColor = true;
         this.PlacePathStartButton.Click += new System.EventHandler(this.PlacePathStartButton_Click);
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox1.Controls.Add(this.PlacePointButton);
         this.groupBox1.Controls.Add(this.PlaceSphereButton);
         this.groupBox1.Controls.Add(this.PlacePathStartButton);
         this.groupBox1.Location = new System.Drawing.Point(3, 3);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(337, 107);
         this.groupBox1.TabIndex = 2;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Place Objects";
         // 
         // PlacePointButton
         // 
         this.PlacePointButton.Enabled = false;
         this.PlacePointButton.Location = new System.Drawing.Point(7, 48);
         this.PlacePointButton.Name = "PlacePointButton";
         this.PlacePointButton.Size = new System.Drawing.Size(105, 23);
         this.PlacePointButton.TabIndex = 2;
         this.PlacePointButton.Text = "Place Point";
         this.PlacePointButton.UseVisualStyleBackColor = true;
         this.PlacePointButton.Click += new System.EventHandler(this.PlacePointButton_Click);
         // 
         // EditLinesButton
         // 
         this.EditLinesButton.Location = new System.Drawing.Point(10, 163);
         this.EditLinesButton.Name = "EditLinesButton";
         this.EditLinesButton.Size = new System.Drawing.Size(105, 23);
         this.EditLinesButton.TabIndex = 3;
         this.EditLinesButton.Text = "Edit Lines";
         this.EditLinesButton.UseVisualStyleBackColor = true;
         this.EditLinesButton.Click += new System.EventHandler(this.EditLinesButton_Click);
         // 
         // AddPointsButton
         // 
         this.AddPointsButton.Location = new System.Drawing.Point(10, 192);
         this.AddPointsButton.Name = "AddPointsButton";
         this.AddPointsButton.Size = new System.Drawing.Size(105, 23);
         this.AddPointsButton.TabIndex = 4;
         this.AddPointsButton.Text = "Add Points";
         this.AddPointsButton.UseVisualStyleBackColor = true;
         this.AddPointsButton.Click += new System.EventHandler(this.AddPointsButton_Click);
         // 
         // InsertPointButton
         // 
         this.InsertPointButton.Location = new System.Drawing.Point(10, 221);
         this.InsertPointButton.Name = "InsertPointButton";
         this.InsertPointButton.Size = new System.Drawing.Size(105, 23);
         this.InsertPointButton.TabIndex = 5;
         this.InsertPointButton.Text = "Insert Point";
         this.InsertPointButton.UseVisualStyleBackColor = true;
         this.InsertPointButton.Click += new System.EventHandler(this.InsertPointButton_Click);
         // 
         // CloseLineLoopButton
         // 
         this.CloseLineLoopButton.Location = new System.Drawing.Point(11, 250);
         this.CloseLineLoopButton.Name = "CloseLineLoopButton";
         this.CloseLineLoopButton.Size = new System.Drawing.Size(104, 23);
         this.CloseLineLoopButton.TabIndex = 6;
         this.CloseLineLoopButton.Text = "Close Line";
         this.CloseLineLoopButton.UseVisualStyleBackColor = true;
         this.CloseLineLoopButton.Click += new System.EventHandler(this.CloseLineLoopButton_Click);
         // 
         // DesignerObjectsPanel
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.CloseLineLoopButton);
         this.Controls.Add(this.InsertPointButton);
         this.Controls.Add(this.AddPointsButton);
         this.Controls.Add(this.EditLinesButton);
         this.Controls.Add(this.groupBox1);
         this.Key = "DesignerObjectsPanel";
         this.Name = "DesignerObjectsPanel";
         this.Size = new System.Drawing.Size(343, 540);
         this.groupBox1.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button PlaceSphereButton;
      private System.Windows.Forms.Button PlacePathStartButton;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Button EditLinesButton;
      private System.Windows.Forms.Button AddPointsButton;
      private System.Windows.Forms.Button PlacePointButton;
      private System.Windows.Forms.Button InsertPointButton;
      private System.Windows.Forms.Button CloseLineLoopButton;
   }
}
