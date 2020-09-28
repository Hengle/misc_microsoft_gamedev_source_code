namespace PhoenixEditor
{
   partial class LocStringDlg
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
         this.mBtnOK = new System.Windows.Forms.Button();
         this.mBtnCancel = new System.Windows.Forms.Button();
         this.mLocStringPicker = new EditorCore.Controls.LocStringPicker();
         this.SuspendLayout();
         // 
         // mBtnOK
         // 
         this.mBtnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.mBtnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
         this.mBtnOK.Location = new System.Drawing.Point(228, 179);
         this.mBtnOK.Name = "mBtnOK";
         this.mBtnOK.Size = new System.Drawing.Size(75, 23);
         this.mBtnOK.TabIndex = 0;
         this.mBtnOK.Text = "OK";
         this.mBtnOK.UseVisualStyleBackColor = true;
         // 
         // mBtnCancel
         // 
         this.mBtnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.mBtnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
         this.mBtnCancel.Location = new System.Drawing.Point(330, 179);
         this.mBtnCancel.Name = "mBtnCancel";
         this.mBtnCancel.Size = new System.Drawing.Size(75, 23);
         this.mBtnCancel.TabIndex = 1;
         this.mBtnCancel.Text = "Cancel";
         this.mBtnCancel.UseVisualStyleBackColor = true;
         // 
         // mLocStringPicker
         // 
         this.mLocStringPicker.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.mLocStringPicker.Location = new System.Drawing.Point(12, 12);
         this.mLocStringPicker.Name = "mLocStringPicker";
         this.mLocStringPicker.Size = new System.Drawing.Size(393, 150);
         this.mLocStringPicker.TabIndex = 2;
         // 
         // LocStringDlg
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(423, 215);
         this.ControlBox = false;
         this.Controls.Add(this.mLocStringPicker);
         this.Controls.Add(this.mBtnCancel);
         this.Controls.Add(this.mBtnOK);
         this.Name = "LocStringDlg";
         this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
         this.Text = "Loc String Picker";
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button mBtnOK;
      private System.Windows.Forms.Button mBtnCancel;
      private EditorCore.Controls.LocStringPicker mLocStringPicker;
   }
}