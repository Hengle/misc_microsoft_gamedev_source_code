namespace EditorCore
{
   partial class BetterPropertyGrid
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
         this.MainTableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
         this.SuspendLayout();
         // 
         // MainTableLayoutPanel
         // 
         this.MainTableLayoutPanel.AutoScroll = true;
         this.MainTableLayoutPanel.CellBorderStyle = System.Windows.Forms.TableLayoutPanelCellBorderStyle.Inset;
         this.MainTableLayoutPanel.ColumnCount = 2;
         this.MainTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
         this.MainTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
         this.MainTableLayoutPanel.Dock = System.Windows.Forms.DockStyle.Fill;
         this.MainTableLayoutPanel.Location = new System.Drawing.Point(0, 0);
         this.MainTableLayoutPanel.Name = "MainTableLayoutPanel";
         this.MainTableLayoutPanel.RowCount = 1;
         this.MainTableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
         this.MainTableLayoutPanel.Size = new System.Drawing.Size(475, 488);
         this.MainTableLayoutPanel.TabIndex = 0;
         // 
         // BetterPropertyGrid
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.MainTableLayoutPanel);
         this.Name = "BetterPropertyGrid";
         this.Size = new System.Drawing.Size(475, 488);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.TableLayoutPanel MainTableLayoutPanel;
   }
}
