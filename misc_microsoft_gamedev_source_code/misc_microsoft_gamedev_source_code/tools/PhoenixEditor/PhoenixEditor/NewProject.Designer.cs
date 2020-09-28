namespace PhoenixEditor
{
   partial class NewProject
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(NewProject));
         System.Windows.Forms.ListViewItem listViewItem1 = new System.Windows.Forms.ListViewItem("256", 0);
         System.Windows.Forms.ListViewItem listViewItem2 = new System.Windows.Forms.ListViewItem("512", 0);
         System.Windows.Forms.ListViewItem listViewItem3 = new System.Windows.Forms.ListViewItem("1024", 0);
         System.Windows.Forms.ListViewItem listViewItem4 = new System.Windows.Forms.ListViewItem("2048", 0);
         System.Windows.Forms.ListViewItem listViewItem5 = new System.Windows.Forms.ListViewItem("4096", 0);
         this.imageList2 = new System.Windows.Forms.ImageList(this.components);
         this.OKButton = new System.Windows.Forms.Button();
         this.CancelNewButton = new System.Windows.Forms.Button();
         this.imageList1 = new System.Windows.Forms.ImageList(this.components);
         this.listView1 = new System.Windows.Forms.ListView();
         this.SuspendLayout();
         // 
         // imageList2
         // 
         this.imageList2.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList2.ImageStream")));
         this.imageList2.TransparentColor = System.Drawing.Color.Transparent;
         this.imageList2.Images.SetKeyName(0, "phx48.bmp");
         // 
         // OKButton
         // 
         this.OKButton.Location = new System.Drawing.Point(25, 233);
         this.OKButton.Name = "OKButton";
         this.OKButton.Size = new System.Drawing.Size(75, 23);
         this.OKButton.TabIndex = 7;
         this.OKButton.Text = "OK";
         this.OKButton.UseVisualStyleBackColor = true;
         // 
         // CancelNewButton
         // 
         this.CancelNewButton.Location = new System.Drawing.Point(327, 233);
         this.CancelNewButton.Name = "CancelNewButton";
         this.CancelNewButton.Size = new System.Drawing.Size(75, 23);
         this.CancelNewButton.TabIndex = 8;
         this.CancelNewButton.Text = "Cancel";
         this.CancelNewButton.UseVisualStyleBackColor = true;
         // 
         // imageList1
         // 
         this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
         this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
         this.imageList1.Images.SetKeyName(0, "folder.bmp");
         this.imageList1.Images.SetKeyName(1, "phx16.bmp");
         // 
         // listView1
         // 
         this.listView1.Items.AddRange(new System.Windows.Forms.ListViewItem[] {
            listViewItem1,
            listViewItem2,
            listViewItem3,
            listViewItem4,
            listViewItem5});
         this.listView1.LargeImageList = this.imageList2;
         this.listView1.Location = new System.Drawing.Point(25, 12);
         this.listView1.Name = "listView1";
         this.listView1.Size = new System.Drawing.Size(377, 198);
         this.listView1.TabIndex = 10;
         this.listView1.UseCompatibleStateImageBehavior = false;
         // 
         // NewProject
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(427, 303);
         this.Controls.Add(this.listView1);
         this.Controls.Add(this.OKButton);
         this.Controls.Add(this.CancelNewButton);
         this.Name = "NewProject";
         this.Text = "NewProject";
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button OKButton;
      private System.Windows.Forms.Button CancelNewButton;
      private System.Windows.Forms.ImageList imageList1;
      private System.Windows.Forms.ImageList imageList2;
      private System.Windows.Forms.ListView listView1;
   }
}