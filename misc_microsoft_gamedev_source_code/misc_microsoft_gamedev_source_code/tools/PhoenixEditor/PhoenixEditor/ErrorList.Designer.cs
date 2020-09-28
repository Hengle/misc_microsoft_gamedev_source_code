namespace PhoenixEditor
{
   partial class ErrorList
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
         this.listView1 = new System.Windows.Forms.ListView();
         this.columnHeader1 = new System.Windows.Forms.ColumnHeader();
         this.Clearbutton = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // listView1
         // 
         this.listView1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.listView1.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1});
         this.listView1.Location = new System.Drawing.Point(3, 30);
         this.listView1.Name = "listView1";
         this.listView1.Size = new System.Drawing.Size(1075, 161);
         this.listView1.TabIndex = 0;
         this.listView1.UseCompatibleStateImageBehavior = false;
         this.listView1.View = System.Windows.Forms.View.Details;
         this.listView1.DoubleClick += new System.EventHandler(this.listView1_DoubleClick);
         this.listView1.KeyDown += new System.Windows.Forms.KeyEventHandler(this.listView1_KeyDown);
         this.listView1.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.listView1_KeyPress);
         // 
         // columnHeader1
         // 
         this.columnHeader1.Text = "Text";
         this.columnHeader1.Width = 943;
         // 
         // Clearbutton
         // 
         this.Clearbutton.Location = new System.Drawing.Point(4, 4);
         this.Clearbutton.Name = "Clearbutton";
         this.Clearbutton.Size = new System.Drawing.Size(75, 23);
         this.Clearbutton.TabIndex = 1;
         this.Clearbutton.Text = "Clear";
         this.Clearbutton.UseVisualStyleBackColor = true;
         this.Clearbutton.Click += new System.EventHandler(this.Clearbutton_Click);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(101, 11);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(35, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "label1";
         // 
         // ErrorList
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.label1);
         this.Controls.Add(this.Clearbutton);
         this.Controls.Add(this.listView1);
         this.FloatingWindowBounds = new System.Drawing.Rectangle(0, 0, 1085, 196);
         this.Key = "ErrorList";
         this.Name = "ErrorList";
         this.Size = new System.Drawing.Size(1085, 196);
         this.Text = "ErrorList";
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ListView listView1;
      private System.Windows.Forms.Button Clearbutton;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.ColumnHeader columnHeader1;

   }
}