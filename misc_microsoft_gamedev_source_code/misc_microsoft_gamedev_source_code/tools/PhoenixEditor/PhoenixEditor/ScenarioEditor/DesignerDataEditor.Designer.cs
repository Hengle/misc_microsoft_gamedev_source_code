namespace PhoenixEditor.ScenarioEditor
{
   partial class DesignerDataEditor
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
         this.betterPropertyGrid1 = new EditorCore.BetterPropertyGrid();
         this.SuspendLayout();
         // 
         // betterPropertyGrid1
         // 
         this.betterPropertyGrid1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.betterPropertyGrid1.Location = new System.Drawing.Point(3, 3);
         this.betterPropertyGrid1.Name = "betterPropertyGrid1";
         this.betterPropertyGrid1.Size = new System.Drawing.Size(222, 252);
         this.betterPropertyGrid1.TabIndex = 0;
         // 
         // DesignerDataEditor
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.betterPropertyGrid1);
         this.Name = "DesignerDataEditor";
         this.Size = new System.Drawing.Size(228, 258);
         this.ResumeLayout(false);

      }

      #endregion

      private EditorCore.BetterPropertyGrid betterPropertyGrid1;

   }
}
