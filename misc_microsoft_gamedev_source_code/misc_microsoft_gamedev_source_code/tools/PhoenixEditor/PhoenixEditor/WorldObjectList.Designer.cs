namespace PhoenixEditor
{
   partial class WorldObjectList
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
         this.listView1 = new System.Windows.Forms.ListView();
         this.PlayerFilterComboBox = new System.Windows.Forms.ComboBox();
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.TypeComboBox = new System.Windows.Forms.ComboBox();
         this.ListVisibleCheckBox = new System.Windows.Forms.CheckBox();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.BBToggleCheckBox = new System.Windows.Forms.CheckBox();
         this.splitContainer1 = new System.Windows.Forms.SplitContainer();
         this.betterPropertyGrid1 = new EditorCore.BetterPropertyGrid();
         this.onlyRenderList = new System.Windows.Forms.CheckBox();
         this.PropertiesChangedTimer = new System.Windows.Forms.Timer(this.components);
         this.ListFilterButton = new System.Windows.Forms.Button();
         this.ShowNamesCheckBox = new System.Windows.Forms.CheckBox();
         this.GroupButton = new System.Windows.Forms.Button();
         this.LockByListCheckBox = new System.Windows.Forms.CheckBox();
         this.MovementButton = new System.Windows.Forms.Button();
         this.RenderNamesCheckBox = new System.Windows.Forms.CheckBox();
         this.splitContainer1.Panel1.SuspendLayout();
         this.splitContainer1.Panel2.SuspendLayout();
         this.splitContainer1.SuspendLayout();
         this.SuspendLayout();
         // 
         // listView1
         // 
         this.listView1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.listView1.HideSelection = false;
         this.listView1.Location = new System.Drawing.Point(0, 0);
         this.listView1.Name = "listView1";
         this.listView1.Size = new System.Drawing.Size(243, 258);
         this.listView1.TabIndex = 0;
         this.listView1.UseCompatibleStateImageBehavior = false;
         this.listView1.View = System.Windows.Forms.View.List;
         this.listView1.DoubleClick += new System.EventHandler(this.listView1_DoubleClick);
         this.listView1.SelectedIndexChanged += new System.EventHandler(this.listView1_SelectedIndexChanged);
         this.listView1.KeyDown += new System.Windows.Forms.KeyEventHandler(this.listView1_KeyDown);
         this.listView1.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.listView1_KeyPress);
         // 
         // PlayerFilterComboBox
         // 
         this.PlayerFilterComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.PlayerFilterComboBox.FormattingEnabled = true;
         this.PlayerFilterComboBox.Items.AddRange(new object[] {
            "All",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8"});
         this.PlayerFilterComboBox.Location = new System.Drawing.Point(45, 3);
         this.PlayerFilterComboBox.Name = "PlayerFilterComboBox";
         this.PlayerFilterComboBox.Size = new System.Drawing.Size(56, 21);
         this.PlayerFilterComboBox.TabIndex = 1;
         this.PlayerFilterComboBox.SelectedIndexChanged += new System.EventHandler(this.PlayerFilterComboBox_SelectedIndexChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(3, 7);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(36, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "Player";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(107, 7);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(31, 13);
         this.label2.TabIndex = 3;
         this.label2.Text = "Type";
         // 
         // TypeComboBox
         // 
         this.TypeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.TypeComboBox.FormattingEnabled = true;
         this.TypeComboBox.Items.AddRange(new object[] {
            "All",
            "Local Lights",
            "Player Start",
            "Sim Objects"});
         this.TypeComboBox.Location = new System.Drawing.Point(144, 3);
         this.TypeComboBox.Name = "TypeComboBox";
         this.TypeComboBox.Size = new System.Drawing.Size(95, 21);
         this.TypeComboBox.TabIndex = 4;
         this.TypeComboBox.SelectedIndexChanged += new System.EventHandler(this.TypeComboBox_SelectedIndexChanged);
         // 
         // ListVisibleCheckBox
         // 
         this.ListVisibleCheckBox.AutoSize = true;
         this.ListVisibleCheckBox.Location = new System.Drawing.Point(3, 30);
         this.ListVisibleCheckBox.Name = "ListVisibleCheckBox";
         this.ListVisibleCheckBox.Size = new System.Drawing.Size(118, 17);
         this.ListVisibleCheckBox.TabIndex = 5;
         this.ListVisibleCheckBox.Text = "List on Screen Only";
         this.ListVisibleCheckBox.UseVisualStyleBackColor = true;
         this.ListVisibleCheckBox.CheckedChanged += new System.EventHandler(this.ListVisibleCheckBox_CheckedChanged);
         // 
         // timer1
         // 
         this.timer1.Enabled = true;
         this.timer1.Interval = 1000;
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // BBToggleCheckBox
         // 
         this.BBToggleCheckBox.AutoSize = true;
         this.BBToggleCheckBox.Location = new System.Drawing.Point(135, 30);
         this.BBToggleCheckBox.Name = "BBToggleCheckBox";
         this.BBToggleCheckBox.Size = new System.Drawing.Size(85, 17);
         this.BBToggleCheckBox.TabIndex = 7;
         this.BBToggleCheckBox.Text = "Show Boxes";
         this.BBToggleCheckBox.UseVisualStyleBackColor = true;
         this.BBToggleCheckBox.CheckedChanged += new System.EventHandler(this.BBToggleCheckBox_CheckedChanged);
         // 
         // splitContainer1
         // 
         this.splitContainer1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.splitContainer1.Location = new System.Drawing.Point(3, 108);
         this.splitContainer1.Name = "splitContainer1";
         this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
         // 
         // splitContainer1.Panel1
         // 
         this.splitContainer1.Panel1.Controls.Add(this.listView1);
         // 
         // splitContainer1.Panel2
         // 
         this.splitContainer1.Panel2.Controls.Add(this.betterPropertyGrid1);
         this.splitContainer1.Size = new System.Drawing.Size(243, 526);
         this.splitContainer1.SplitterDistance = 258;
         this.splitContainer1.TabIndex = 9;
         // 
         // betterPropertyGrid1
         // 
         this.betterPropertyGrid1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.betterPropertyGrid1.Location = new System.Drawing.Point(0, 0);
         this.betterPropertyGrid1.Name = "betterPropertyGrid1";
         this.betterPropertyGrid1.Size = new System.Drawing.Size(243, 264);
         this.betterPropertyGrid1.TabIndex = 9;
         // 
         // onlyRenderList
         // 
         this.onlyRenderList.AutoSize = true;
         this.onlyRenderList.Location = new System.Drawing.Point(135, 61);
         this.onlyRenderList.Name = "onlyRenderList";
         this.onlyRenderList.Size = new System.Drawing.Size(82, 17);
         this.onlyRenderList.TabIndex = 10;
         this.onlyRenderList.Text = "Hide By List";
         this.onlyRenderList.UseVisualStyleBackColor = true;
         this.onlyRenderList.CheckedChanged += new System.EventHandler(this.onlyRenderList_CheckedChanged);
         // 
         // PropertiesChangedTimer
         // 
         this.PropertiesChangedTimer.Enabled = true;
         this.PropertiesChangedTimer.Tick += new System.EventHandler(this.PropertiesChangedTimer_Tick);
         // 
         // ListFilterButton
         // 
         this.ListFilterButton.Location = new System.Drawing.Point(3, 78);
         this.ListFilterButton.Name = "ListFilterButton";
         this.ListFilterButton.Size = new System.Drawing.Size(63, 23);
         this.ListFilterButton.TabIndex = 11;
         this.ListFilterButton.Text = "Filter...";
         this.ListFilterButton.UseVisualStyleBackColor = true;
         this.ListFilterButton.Click += new System.EventHandler(this.ListFilterButton_Click);
         // 
         // ShowNamesCheckBox
         // 
         this.ShowNamesCheckBox.AutoSize = true;
         this.ShowNamesCheckBox.Checked = true;
         this.ShowNamesCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.ShowNamesCheckBox.Location = new System.Drawing.Point(3, 45);
         this.ShowNamesCheckBox.Name = "ShowNamesCheckBox";
         this.ShowNamesCheckBox.Size = new System.Drawing.Size(93, 17);
         this.ShowNamesCheckBox.TabIndex = 12;
         this.ShowNamesCheckBox.Text = "Names/Types";
         this.ShowNamesCheckBox.UseVisualStyleBackColor = true;
         this.ShowNamesCheckBox.CheckedChanged += new System.EventHandler(this.ShowNamesCheckBox_CheckedChanged);
         // 
         // GroupButton
         // 
         this.GroupButton.Location = new System.Drawing.Point(71, 78);
         this.GroupButton.Name = "GroupButton";
         this.GroupButton.Size = new System.Drawing.Size(58, 23);
         this.GroupButton.TabIndex = 13;
         this.GroupButton.Text = "Groups...";
         this.GroupButton.UseVisualStyleBackColor = true;
         this.GroupButton.Click += new System.EventHandler(this.GroupButton_Click);
         // 
         // LockByListCheckBox
         // 
         this.LockByListCheckBox.AutoSize = true;
         this.LockByListCheckBox.Location = new System.Drawing.Point(135, 45);
         this.LockByListCheckBox.Name = "LockByListCheckBox";
         this.LockByListCheckBox.Size = new System.Drawing.Size(84, 17);
         this.LockByListCheckBox.TabIndex = 14;
         this.LockByListCheckBox.Text = "Lock By List";
         this.LockByListCheckBox.UseVisualStyleBackColor = true;
         this.LockByListCheckBox.CheckedChanged += new System.EventHandler(this.LockByListCheckBox_CheckedChanged);
         // 
         // MovementButton
         // 
         this.MovementButton.Location = new System.Drawing.Point(135, 78);
         this.MovementButton.Name = "MovementButton";
         this.MovementButton.Size = new System.Drawing.Size(104, 23);
         this.MovementButton.TabIndex = 15;
         this.MovementButton.Text = "Movement ...";
         this.MovementButton.UseVisualStyleBackColor = true;
         this.MovementButton.Click += new System.EventHandler(this.MovementButton_Click);
         // 
         // RenderNamesCheckBox
         // 
         this.RenderNamesCheckBox.AutoSize = true;
         this.RenderNamesCheckBox.Location = new System.Drawing.Point(3, 61);
         this.RenderNamesCheckBox.Name = "RenderNamesCheckBox";
         this.RenderNamesCheckBox.Size = new System.Drawing.Size(97, 17);
         this.RenderNamesCheckBox.TabIndex = 16;
         this.RenderNamesCheckBox.Text = "Render Names";
         this.RenderNamesCheckBox.UseVisualStyleBackColor = true;
         this.RenderNamesCheckBox.CheckedChanged += new System.EventHandler(this.RenderNamesCheckBox_CheckedChanged);
         // 
         // WorldObjectList
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.RenderNamesCheckBox);
         this.Controls.Add(this.MovementButton);
         this.Controls.Add(this.LockByListCheckBox);
         this.Controls.Add(this.GroupButton);
         this.Controls.Add(this.ShowNamesCheckBox);
         this.Controls.Add(this.ListFilterButton);
         this.Controls.Add(this.onlyRenderList);
         this.Controls.Add(this.splitContainer1);
         this.Controls.Add(this.BBToggleCheckBox);
         this.Controls.Add(this.ListVisibleCheckBox);
         this.Controls.Add(this.TypeComboBox);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.PlayerFilterComboBox);
         this.Key = "WorldObjectList";
         this.Name = "WorldObjectList";
         this.Size = new System.Drawing.Size(249, 634);
         this.Text = "WorldObjectList";
         this.splitContainer1.Panel1.ResumeLayout(false);
         this.splitContainer1.Panel2.ResumeLayout(false);
         this.splitContainer1.ResumeLayout(false);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ListView listView1;
      private System.Windows.Forms.ComboBox PlayerFilterComboBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.ComboBox TypeComboBox;
      private System.Windows.Forms.CheckBox ListVisibleCheckBox;
      private System.Windows.Forms.Timer timer1;
      private System.Windows.Forms.CheckBox BBToggleCheckBox;
      private System.Windows.Forms.SplitContainer splitContainer1;
      private EditorCore.BetterPropertyGrid betterPropertyGrid1;
      private System.Windows.Forms.CheckBox onlyRenderList;
      private System.Windows.Forms.Timer PropertiesChangedTimer;
      private System.Windows.Forms.Button ListFilterButton;
      private System.Windows.Forms.CheckBox ShowNamesCheckBox;
      private System.Windows.Forms.Button GroupButton;
      private System.Windows.Forms.CheckBox LockByListCheckBox;
      private System.Windows.Forms.Button MovementButton;
      private System.Windows.Forms.CheckBox RenderNamesCheckBox;
   }
}