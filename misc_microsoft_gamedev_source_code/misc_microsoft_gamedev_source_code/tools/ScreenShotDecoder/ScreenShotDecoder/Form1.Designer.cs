namespace ScreenShotDecoder
{
   partial class Form1
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.DecodeFromHex = new System.Windows.Forms.Button();
         this.HexCode2 = new System.Windows.Forms.TextBox();
         this.label1 = new System.Windows.Forms.Label();
         this.HexCode1 = new System.Windows.Forms.TextBox();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.ClearBits = new System.Windows.Forms.Button();
         this.DecodeFromImage = new System.Windows.Forms.Button();
         this.panel1 = new System.Windows.Forms.Panel();
         this.XUIDCheckInner12 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner11 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner10 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner09 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner08 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner07 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner06 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner05 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner04 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner03 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner02 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckInner01 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter12 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter11 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter10 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter09 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter08 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter07 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter06 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter05 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter04 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter03 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter02 = new System.Windows.Forms.CheckBox();
         this.XUIDCheckOuter01 = new System.Windows.Forms.CheckBox();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         this.label4 = new System.Windows.Forms.Label();
         this.XUIDResult2 = new System.Windows.Forms.TextBox();
         this.label3 = new System.Windows.Forms.Label();
         this.XUIDResult1 = new System.Windows.Forms.TextBox();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         this.panel1.SuspendLayout();
         this.groupBox3.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.DecodeFromHex);
         this.groupBox1.Controls.Add(this.HexCode2);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.HexCode1);
         this.groupBox1.Location = new System.Drawing.Point(2, 4);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(281, 119);
         this.groupBox1.TabIndex = 13;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Decode from Hex Codes";
         // 
         // DecodeFromHex
         // 
         this.DecodeFromHex.Location = new System.Drawing.Point(84, 84);
         this.DecodeFromHex.Name = "DecodeFromHex";
         this.DecodeFromHex.Size = new System.Drawing.Size(148, 23);
         this.DecodeFromHex.TabIndex = 6;
         this.DecodeFromHex.Text = "Decode from Hex Codes";
         this.DecodeFromHex.UseVisualStyleBackColor = true;
         this.DecodeFromHex.Click += new System.EventHandler(this.DecodeFromHex_Click);
         // 
         // HexCode2
         // 
         this.HexCode2.Location = new System.Drawing.Point(35, 58);
         this.HexCode2.Name = "HexCode2";
         this.HexCode2.Size = new System.Drawing.Size(231, 20);
         this.HexCode2.TabIndex = 5;
         this.HexCode2.Text = "9B4EC13FB";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(11, 16);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(163, 13);
         this.label1.TabIndex = 4;
         this.label1.Text = "Input Bottom/Right hex numbers:";
         // 
         // HexCode1
         // 
         this.HexCode1.Location = new System.Drawing.Point(35, 32);
         this.HexCode1.Name = "HexCode1";
         this.HexCode1.Size = new System.Drawing.Size(231, 20);
         this.HexCode1.TabIndex = 3;
         this.HexCode1.Text = "AA9A0A87D";
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.ClearBits);
         this.groupBox2.Controls.Add(this.DecodeFromImage);
         this.groupBox2.Controls.Add(this.panel1);
         this.groupBox2.Location = new System.Drawing.Point(289, 4);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(285, 283);
         this.groupBox2.TabIndex = 14;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Decode from Mini-Map";
         // 
         // ClearBits
         // 
         this.ClearBits.Location = new System.Drawing.Point(172, 254);
         this.ClearBits.Name = "ClearBits";
         this.ClearBits.Size = new System.Drawing.Size(106, 23);
         this.ClearBits.TabIndex = 14;
         this.ClearBits.Text = "Clear Image Bits";
         this.ClearBits.UseVisualStyleBackColor = true;
         this.ClearBits.Click += new System.EventHandler(this.ClearBits_Click);
         // 
         // DecodeFromImage
         // 
         this.DecodeFromImage.Location = new System.Drawing.Point(17, 254);
         this.DecodeFromImage.Name = "DecodeFromImage";
         this.DecodeFromImage.Size = new System.Drawing.Size(148, 23);
         this.DecodeFromImage.TabIndex = 13;
         this.DecodeFromImage.Text = "Decode from Image Bits";
         this.DecodeFromImage.UseVisualStyleBackColor = true;
         this.DecodeFromImage.Click += new System.EventHandler(this.DecodeFromImage_Click);
         // 
         // panel1
         // 
         this.panel1.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("panel1.BackgroundImage")));
         this.panel1.Controls.Add(this.XUIDCheckInner12);
         this.panel1.Controls.Add(this.XUIDCheckInner11);
         this.panel1.Controls.Add(this.XUIDCheckInner10);
         this.panel1.Controls.Add(this.XUIDCheckInner09);
         this.panel1.Controls.Add(this.XUIDCheckInner08);
         this.panel1.Controls.Add(this.XUIDCheckInner07);
         this.panel1.Controls.Add(this.XUIDCheckInner06);
         this.panel1.Controls.Add(this.XUIDCheckInner05);
         this.panel1.Controls.Add(this.XUIDCheckInner04);
         this.panel1.Controls.Add(this.XUIDCheckInner03);
         this.panel1.Controls.Add(this.XUIDCheckInner02);
         this.panel1.Controls.Add(this.XUIDCheckInner01);
         this.panel1.Controls.Add(this.XUIDCheckOuter12);
         this.panel1.Controls.Add(this.XUIDCheckOuter11);
         this.panel1.Controls.Add(this.XUIDCheckOuter10);
         this.panel1.Controls.Add(this.XUIDCheckOuter09);
         this.panel1.Controls.Add(this.XUIDCheckOuter08);
         this.panel1.Controls.Add(this.XUIDCheckOuter07);
         this.panel1.Controls.Add(this.XUIDCheckOuter06);
         this.panel1.Controls.Add(this.XUIDCheckOuter05);
         this.panel1.Controls.Add(this.XUIDCheckOuter04);
         this.panel1.Controls.Add(this.XUIDCheckOuter03);
         this.panel1.Controls.Add(this.XUIDCheckOuter02);
         this.panel1.Controls.Add(this.XUIDCheckOuter01);
         this.panel1.Location = new System.Drawing.Point(17, 19);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(260, 229);
         this.panel1.TabIndex = 11;
         // 
         // XUIDCheckInner12
         // 
         this.XUIDCheckInner12.AutoSize = true;
         this.XUIDCheckInner12.Location = new System.Drawing.Point(83, 43);
         this.XUIDCheckInner12.Name = "XUIDCheckInner12";
         this.XUIDCheckInner12.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner12.TabIndex = 32;
         this.XUIDCheckInner12.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner11
         // 
         this.XUIDCheckInner11.AutoSize = true;
         this.XUIDCheckInner11.Location = new System.Drawing.Point(65, 72);
         this.XUIDCheckInner11.Name = "XUIDCheckInner11";
         this.XUIDCheckInner11.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner11.TabIndex = 31;
         this.XUIDCheckInner11.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner10
         // 
         this.XUIDCheckInner10.AutoSize = true;
         this.XUIDCheckInner10.Location = new System.Drawing.Point(51, 100);
         this.XUIDCheckInner10.Name = "XUIDCheckInner10";
         this.XUIDCheckInner10.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner10.TabIndex = 30;
         this.XUIDCheckInner10.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner09
         // 
         this.XUIDCheckInner09.AutoSize = true;
         this.XUIDCheckInner09.Location = new System.Drawing.Point(65, 134);
         this.XUIDCheckInner09.Name = "XUIDCheckInner09";
         this.XUIDCheckInner09.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner09.TabIndex = 29;
         this.XUIDCheckInner09.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner08
         // 
         this.XUIDCheckInner08.AutoSize = true;
         this.XUIDCheckInner08.Location = new System.Drawing.Point(83, 162);
         this.XUIDCheckInner08.Name = "XUIDCheckInner08";
         this.XUIDCheckInner08.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner08.TabIndex = 28;
         this.XUIDCheckInner08.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner07
         // 
         this.XUIDCheckInner07.AutoSize = true;
         this.XUIDCheckInner07.Location = new System.Drawing.Point(121, 179);
         this.XUIDCheckInner07.Name = "XUIDCheckInner07";
         this.XUIDCheckInner07.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner07.TabIndex = 27;
         this.XUIDCheckInner07.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner06
         // 
         this.XUIDCheckInner06.AutoSize = true;
         this.XUIDCheckInner06.Location = new System.Drawing.Point(155, 162);
         this.XUIDCheckInner06.Name = "XUIDCheckInner06";
         this.XUIDCheckInner06.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner06.TabIndex = 26;
         this.XUIDCheckInner06.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner05
         // 
         this.XUIDCheckInner05.AutoSize = true;
         this.XUIDCheckInner05.Location = new System.Drawing.Point(181, 134);
         this.XUIDCheckInner05.Name = "XUIDCheckInner05";
         this.XUIDCheckInner05.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner05.TabIndex = 25;
         this.XUIDCheckInner05.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner04
         // 
         this.XUIDCheckInner04.AutoSize = true;
         this.XUIDCheckInner04.Location = new System.Drawing.Point(195, 100);
         this.XUIDCheckInner04.Name = "XUIDCheckInner04";
         this.XUIDCheckInner04.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner04.TabIndex = 24;
         this.XUIDCheckInner04.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner03
         // 
         this.XUIDCheckInner03.AutoSize = true;
         this.XUIDCheckInner03.Location = new System.Drawing.Point(181, 72);
         this.XUIDCheckInner03.Name = "XUIDCheckInner03";
         this.XUIDCheckInner03.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner03.TabIndex = 23;
         this.XUIDCheckInner03.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner02
         // 
         this.XUIDCheckInner02.AutoSize = true;
         this.XUIDCheckInner02.Location = new System.Drawing.Point(155, 43);
         this.XUIDCheckInner02.Name = "XUIDCheckInner02";
         this.XUIDCheckInner02.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner02.TabIndex = 22;
         this.XUIDCheckInner02.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckInner01
         // 
         this.XUIDCheckInner01.AutoSize = true;
         this.XUIDCheckInner01.Location = new System.Drawing.Point(121, 32);
         this.XUIDCheckInner01.Name = "XUIDCheckInner01";
         this.XUIDCheckInner01.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckInner01.TabIndex = 21;
         this.XUIDCheckInner01.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter12
         // 
         this.XUIDCheckOuter12.AutoSize = true;
         this.XUIDCheckOuter12.Location = new System.Drawing.Point(65, 18);
         this.XUIDCheckOuter12.Name = "XUIDCheckOuter12";
         this.XUIDCheckOuter12.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter12.TabIndex = 20;
         this.XUIDCheckOuter12.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter11
         // 
         this.XUIDCheckOuter11.AutoSize = true;
         this.XUIDCheckOuter11.Location = new System.Drawing.Point(33, 53);
         this.XUIDCheckOuter11.Name = "XUIDCheckOuter11";
         this.XUIDCheckOuter11.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter11.TabIndex = 19;
         this.XUIDCheckOuter11.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter10
         // 
         this.XUIDCheckOuter10.AutoSize = true;
         this.XUIDCheckOuter10.Location = new System.Drawing.Point(20, 100);
         this.XUIDCheckOuter10.Name = "XUIDCheckOuter10";
         this.XUIDCheckOuter10.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter10.TabIndex = 18;
         this.XUIDCheckOuter10.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter09
         // 
         this.XUIDCheckOuter09.AutoSize = true;
         this.XUIDCheckOuter09.Location = new System.Drawing.Point(33, 149);
         this.XUIDCheckOuter09.Name = "XUIDCheckOuter09";
         this.XUIDCheckOuter09.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter09.TabIndex = 17;
         this.XUIDCheckOuter09.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter08
         // 
         this.XUIDCheckOuter08.AutoSize = true;
         this.XUIDCheckOuter08.Location = new System.Drawing.Point(65, 184);
         this.XUIDCheckOuter08.Name = "XUIDCheckOuter08";
         this.XUIDCheckOuter08.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter08.TabIndex = 16;
         this.XUIDCheckOuter08.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter07
         // 
         this.XUIDCheckOuter07.AutoSize = true;
         this.XUIDCheckOuter07.Location = new System.Drawing.Point(121, 199);
         this.XUIDCheckOuter07.Name = "XUIDCheckOuter07";
         this.XUIDCheckOuter07.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter07.TabIndex = 15;
         this.XUIDCheckOuter07.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter06
         // 
         this.XUIDCheckOuter06.AutoSize = true;
         this.XUIDCheckOuter06.Location = new System.Drawing.Point(172, 184);
         this.XUIDCheckOuter06.Name = "XUIDCheckOuter06";
         this.XUIDCheckOuter06.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter06.TabIndex = 14;
         this.XUIDCheckOuter06.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter05
         // 
         this.XUIDCheckOuter05.AutoSize = true;
         this.XUIDCheckOuter05.Location = new System.Drawing.Point(206, 149);
         this.XUIDCheckOuter05.Name = "XUIDCheckOuter05";
         this.XUIDCheckOuter05.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter05.TabIndex = 13;
         this.XUIDCheckOuter05.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter04
         // 
         this.XUIDCheckOuter04.AutoSize = true;
         this.XUIDCheckOuter04.Location = new System.Drawing.Point(216, 100);
         this.XUIDCheckOuter04.Name = "XUIDCheckOuter04";
         this.XUIDCheckOuter04.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter04.TabIndex = 12;
         this.XUIDCheckOuter04.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter03
         // 
         this.XUIDCheckOuter03.AutoSize = true;
         this.XUIDCheckOuter03.Location = new System.Drawing.Point(206, 53);
         this.XUIDCheckOuter03.Name = "XUIDCheckOuter03";
         this.XUIDCheckOuter03.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter03.TabIndex = 11;
         this.XUIDCheckOuter03.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter02
         // 
         this.XUIDCheckOuter02.AutoSize = true;
         this.XUIDCheckOuter02.Location = new System.Drawing.Point(172, 18);
         this.XUIDCheckOuter02.Name = "XUIDCheckOuter02";
         this.XUIDCheckOuter02.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter02.TabIndex = 10;
         this.XUIDCheckOuter02.UseVisualStyleBackColor = true;
         // 
         // XUIDCheckOuter01
         // 
         this.XUIDCheckOuter01.AutoSize = true;
         this.XUIDCheckOuter01.Location = new System.Drawing.Point(121, 3);
         this.XUIDCheckOuter01.Name = "XUIDCheckOuter01";
         this.XUIDCheckOuter01.Size = new System.Drawing.Size(15, 14);
         this.XUIDCheckOuter01.TabIndex = 9;
         this.XUIDCheckOuter01.UseVisualStyleBackColor = true;
         // 
         // groupBox3
         // 
         this.groupBox3.Controls.Add(this.label4);
         this.groupBox3.Controls.Add(this.XUIDResult2);
         this.groupBox3.Controls.Add(this.label3);
         this.groupBox3.Controls.Add(this.XUIDResult1);
         this.groupBox3.Location = new System.Drawing.Point(114, 293);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(352, 86);
         this.groupBox3.TabIndex = 15;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Resulting XUID";
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(16, 50);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(91, 13);
         this.label4.TabIndex = 10;
         this.label4.Text = "Unsigned Integer:";
         // 
         // XUIDResult2
         // 
         this.XUIDResult2.Location = new System.Drawing.Point(113, 50);
         this.XUIDResult2.Name = "XUIDResult2";
         this.XUIDResult2.Size = new System.Drawing.Size(231, 20);
         this.XUIDResult2.TabIndex = 9;
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(16, 25);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(29, 13);
         this.label3.TabIndex = 8;
         this.label3.Text = "Hex:";
         // 
         // XUIDResult1
         // 
         this.XUIDResult1.Location = new System.Drawing.Point(113, 18);
         this.XUIDResult1.Name = "XUIDResult1";
         this.XUIDResult1.Size = new System.Drawing.Size(231, 20);
         this.XUIDResult1.TabIndex = 7;
         // 
         // Form1
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(583, 390);
         this.Controls.Add(this.groupBox3);
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox1);
         this.Name = "Form1";
         this.Text = "Find the scumbag...";
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         this.panel1.ResumeLayout(false);
         this.panel1.PerformLayout();
         this.groupBox3.ResumeLayout(false);
         this.groupBox3.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Button DecodeFromHex;
      private System.Windows.Forms.TextBox HexCode2;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.TextBox HexCode1;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.Button ClearBits;
      private System.Windows.Forms.Button DecodeFromImage;
      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.CheckBox XUIDCheckInner12;
      private System.Windows.Forms.CheckBox XUIDCheckInner11;
      private System.Windows.Forms.CheckBox XUIDCheckInner10;
      private System.Windows.Forms.CheckBox XUIDCheckInner09;
      private System.Windows.Forms.CheckBox XUIDCheckInner08;
      private System.Windows.Forms.CheckBox XUIDCheckInner07;
      private System.Windows.Forms.CheckBox XUIDCheckInner06;
      private System.Windows.Forms.CheckBox XUIDCheckInner05;
      private System.Windows.Forms.CheckBox XUIDCheckInner04;
      private System.Windows.Forms.CheckBox XUIDCheckInner03;
      private System.Windows.Forms.CheckBox XUIDCheckInner02;
      private System.Windows.Forms.CheckBox XUIDCheckInner01;
      private System.Windows.Forms.CheckBox XUIDCheckOuter12;
      private System.Windows.Forms.CheckBox XUIDCheckOuter11;
      private System.Windows.Forms.CheckBox XUIDCheckOuter10;
      private System.Windows.Forms.CheckBox XUIDCheckOuter09;
      private System.Windows.Forms.CheckBox XUIDCheckOuter08;
      private System.Windows.Forms.CheckBox XUIDCheckOuter07;
      private System.Windows.Forms.CheckBox XUIDCheckOuter06;
      private System.Windows.Forms.CheckBox XUIDCheckOuter05;
      private System.Windows.Forms.CheckBox XUIDCheckOuter04;
      private System.Windows.Forms.CheckBox XUIDCheckOuter03;
      private System.Windows.Forms.CheckBox XUIDCheckOuter02;
      private System.Windows.Forms.CheckBox XUIDCheckOuter01;
      private System.Windows.Forms.GroupBox groupBox3;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.TextBox XUIDResult2;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.TextBox XUIDResult1;
   }
}

