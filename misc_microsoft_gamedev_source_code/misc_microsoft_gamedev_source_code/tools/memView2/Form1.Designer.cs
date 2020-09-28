namespace memView2
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
         this.components = new System.ComponentModel.Container();
         this.treeView1 = new System.Windows.Forms.TreeView();
         this.contextMenuStrip1 = new System.Windows.Forms.ContextMenuStrip(this.components);
         this.copyThisStackToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.copyThisNodesChildrenOnlyToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.menuStrip1 = new System.Windows.Forms.MenuStrip();
         this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.openXMLToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator();
         this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.filterToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.mFilterLarge = new System.Windows.Forms.ToolStripMenuItem();
         this.mFilterIncreasingNonTrivial = new System.Windows.Forms.ToolStripMenuItem();
         this.mFilterIncreasing = new System.Windows.Forms.ToolStripMenuItem();
         this.mFilterAlwaysIncreasing = new System.Windows.Forms.ToolStripMenuItem();
         this.mFilterColorOnly = new System.Windows.Forms.ToolStripMenuItem();
         this.listBox1 = new System.Windows.Forms.ListBox();
         this.contextMenuStrip2 = new System.Windows.Forms.ContextMenuStrip(this.components);
         this.copyToClipboardToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripMenuItem2 = new System.Windows.Forms.ToolStripSeparator();
         this.selectAllToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripMenuItem3 = new System.Windows.Forms.ToolStripSeparator();
         this.safeToDiskToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.groupBox4 = new System.Windows.Forms.GroupBox();
         this.label2 = new System.Windows.Forms.Label();
         this.label1 = new System.Windows.Forms.Label();
         this.numericUpDown2 = new System.Windows.Forms.NumericUpDown();
         this.numericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.checkBox2 = new System.Windows.Forms.CheckBox();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         this.textBox2 = new System.Windows.Forms.TextBox();
         this.button1 = new System.Windows.Forms.Button();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.tabPage1 = new System.Windows.Forms.TabPage();
         this.tabPage2 = new System.Windows.Forms.TabPage();
         this.button2 = new System.Windows.Forms.Button();
         this.contextMenuStrip1.SuspendLayout();
         this.menuStrip1.SuspendLayout();
         this.contextMenuStrip2.SuspendLayout();
         this.groupBox1.SuspendLayout();
         this.groupBox4.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).BeginInit();
         this.groupBox3.SuspendLayout();
         this.groupBox2.SuspendLayout();
         this.tabControl1.SuspendLayout();
         this.tabPage1.SuspendLayout();
         this.tabPage2.SuspendLayout();
         this.SuspendLayout();
         // 
         // treeView1
         // 
         this.treeView1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.treeView1.ContextMenuStrip = this.contextMenuStrip1;
         this.treeView1.Location = new System.Drawing.Point(8, 35);
         this.treeView1.Name = "treeView1";
         this.treeView1.Size = new System.Drawing.Size(1245, 765);
         this.treeView1.TabIndex = 0;
         this.treeView1.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
         // 
         // contextMenuStrip1
         // 
         this.contextMenuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.copyThisStackToolStripMenuItem,
            this.copyThisNodesChildrenOnlyToolStripMenuItem});
         this.contextMenuStrip1.Name = "contextMenuStrip1";
         this.contextMenuStrip1.Size = new System.Drawing.Size(259, 48);
         // 
         // copyThisStackToolStripMenuItem
         // 
         this.copyThisStackToolStripMenuItem.Name = "copyThisStackToolStripMenuItem";
         this.copyThisStackToolStripMenuItem.Size = new System.Drawing.Size(258, 22);
         this.copyThisStackToolStripMenuItem.Text = "Copy stack with this node as a pivot";
         this.copyThisStackToolStripMenuItem.Click += new System.EventHandler(this.copyThisStackToolStripMenuItem_Click);
         // 
         // copyThisNodesChildrenOnlyToolStripMenuItem
         // 
         this.copyThisNodesChildrenOnlyToolStripMenuItem.Name = "copyThisNodesChildrenOnlyToolStripMenuItem";
         this.copyThisNodesChildrenOnlyToolStripMenuItem.Size = new System.Drawing.Size(258, 22);
         this.copyThisNodesChildrenOnlyToolStripMenuItem.Text = "Copy this node\'s children only";
         this.copyThisNodesChildrenOnlyToolStripMenuItem.Click += new System.EventHandler(this.copyThisNodesChildrenOnlyToolStripMenuItem_Click);
         // 
         // menuStrip1
         // 
         this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.filterToolStripMenuItem});
         this.menuStrip1.Location = new System.Drawing.Point(0, 0);
         this.menuStrip1.Name = "menuStrip1";
         this.menuStrip1.Size = new System.Drawing.Size(1414, 24);
         this.menuStrip1.TabIndex = 1;
         this.menuStrip1.Text = "menuStrip1";
         // 
         // fileToolStripMenuItem
         // 
         this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openXMLToolStripMenuItem,
            this.toolStripMenuItem1,
            this.exitToolStripMenuItem});
         this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
         this.fileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
         this.fileToolStripMenuItem.Text = "File";
         // 
         // openXMLToolStripMenuItem
         // 
         this.openXMLToolStripMenuItem.Name = "openXMLToolStripMenuItem";
         this.openXMLToolStripMenuItem.Size = new System.Drawing.Size(133, 22);
         this.openXMLToolStripMenuItem.Text = "Open XML";
         this.openXMLToolStripMenuItem.Click += new System.EventHandler(this.openXMLToolStripMenuItem_Click);
         // 
         // toolStripMenuItem1
         // 
         this.toolStripMenuItem1.Name = "toolStripMenuItem1";
         this.toolStripMenuItem1.Size = new System.Drawing.Size(130, 6);
         // 
         // exitToolStripMenuItem
         // 
         this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
         this.exitToolStripMenuItem.Size = new System.Drawing.Size(133, 22);
         this.exitToolStripMenuItem.Text = "Exit";
         this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
         // 
         // filterToolStripMenuItem
         // 
         this.filterToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mFilterLarge,
            this.mFilterIncreasingNonTrivial,
            this.mFilterIncreasing,
            this.mFilterAlwaysIncreasing,
            this.mFilterColorOnly});
         this.filterToolStripMenuItem.Name = "filterToolStripMenuItem";
         this.filterToolStripMenuItem.Size = new System.Drawing.Size(43, 20);
         this.filterToolStripMenuItem.Text = "Filter";
         // 
         // mFilterLarge
         // 
         this.mFilterLarge.CheckOnClick = true;
         this.mFilterLarge.Name = "mFilterLarge";
         this.mFilterLarge.Size = new System.Drawing.Size(189, 22);
         this.mFilterLarge.Text = "Large";
         this.mFilterLarge.Click += new System.EventHandler(this.mFilterLarge_Click);
         // 
         // mFilterIncreasingNonTrivial
         // 
         this.mFilterIncreasingNonTrivial.CheckOnClick = true;
         this.mFilterIncreasingNonTrivial.Name = "mFilterIncreasingNonTrivial";
         this.mFilterIncreasingNonTrivial.Size = new System.Drawing.Size(189, 22);
         this.mFilterIncreasingNonTrivial.Text = "Increasing Non-Trivial";
         // 
         // mFilterIncreasing
         // 
         this.mFilterIncreasing.CheckOnClick = true;
         this.mFilterIncreasing.Name = "mFilterIncreasing";
         this.mFilterIncreasing.Size = new System.Drawing.Size(189, 22);
         this.mFilterIncreasing.Text = "Increasing";
         // 
         // mFilterAlwaysIncreasing
         // 
         this.mFilterAlwaysIncreasing.CheckOnClick = true;
         this.mFilterAlwaysIncreasing.Name = "mFilterAlwaysIncreasing";
         this.mFilterAlwaysIncreasing.Size = new System.Drawing.Size(189, 22);
         this.mFilterAlwaysIncreasing.Text = "Always Increasing";
         // 
         // mFilterColorOnly
         // 
         this.mFilterColorOnly.CheckOnClick = true;
         this.mFilterColorOnly.Name = "mFilterColorOnly";
         this.mFilterColorOnly.Size = new System.Drawing.Size(189, 22);
         this.mFilterColorOnly.Text = "Color Only";
         // 
         // listBox1
         // 
         this.listBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.listBox1.ContextMenuStrip = this.contextMenuStrip2;
         this.listBox1.FormattingEnabled = true;
         this.listBox1.Location = new System.Drawing.Point(206, 23);
         this.listBox1.Name = "listBox1";
         this.listBox1.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
         this.listBox1.Size = new System.Drawing.Size(1186, 810);
         this.listBox1.TabIndex = 2;
         // 
         // contextMenuStrip2
         // 
         this.contextMenuStrip2.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.copyToClipboardToolStripMenuItem,
            this.toolStripMenuItem2,
            this.selectAllToolStripMenuItem,
            this.toolStripMenuItem3,
            this.safeToDiskToolStripMenuItem});
         this.contextMenuStrip2.Name = "contextMenuStrip2";
         this.contextMenuStrip2.Size = new System.Drawing.Size(174, 82);
         // 
         // copyToClipboardToolStripMenuItem
         // 
         this.copyToClipboardToolStripMenuItem.Name = "copyToClipboardToolStripMenuItem";
         this.copyToClipboardToolStripMenuItem.Size = new System.Drawing.Size(173, 22);
         this.copyToClipboardToolStripMenuItem.Text = "Copy To Clipboard";
         this.copyToClipboardToolStripMenuItem.Click += new System.EventHandler(this.copyToClipboardToolStripMenuItem_Click);
         // 
         // toolStripMenuItem2
         // 
         this.toolStripMenuItem2.Name = "toolStripMenuItem2";
         this.toolStripMenuItem2.Size = new System.Drawing.Size(170, 6);
         // 
         // selectAllToolStripMenuItem
         // 
         this.selectAllToolStripMenuItem.Name = "selectAllToolStripMenuItem";
         this.selectAllToolStripMenuItem.Size = new System.Drawing.Size(173, 22);
         this.selectAllToolStripMenuItem.Text = "Select All";
         this.selectAllToolStripMenuItem.Click += new System.EventHandler(this.selectAllToolStripMenuItem_Click);
         // 
         // toolStripMenuItem3
         // 
         this.toolStripMenuItem3.Name = "toolStripMenuItem3";
         this.toolStripMenuItem3.Size = new System.Drawing.Size(170, 6);
         // 
         // safeToDiskToolStripMenuItem
         // 
         this.safeToDiskToolStripMenuItem.Name = "safeToDiskToolStripMenuItem";
         this.safeToDiskToolStripMenuItem.Size = new System.Drawing.Size(173, 22);
         this.safeToDiskToolStripMenuItem.Text = "Save to disk";
         this.safeToDiskToolStripMenuItem.Click += new System.EventHandler(this.safeToDiskToolStripMenuItem_Click);
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.groupBox1.Controls.Add(this.groupBox4);
         this.groupBox1.Controls.Add(this.checkBox2);
         this.groupBox1.Controls.Add(this.groupBox3);
         this.groupBox1.Controls.Add(this.button1);
         this.groupBox1.Controls.Add(this.checkBox1);
         this.groupBox1.Controls.Add(this.groupBox2);
         this.groupBox1.Location = new System.Drawing.Point(6, 23);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(200, 816);
         this.groupBox1.TabIndex = 3;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Search";
         this.groupBox1.Enter += new System.EventHandler(this.groupBox1_Enter);
         // 
         // groupBox4
         // 
         this.groupBox4.Controls.Add(this.label2);
         this.groupBox4.Controls.Add(this.label1);
         this.groupBox4.Controls.Add(this.numericUpDown2);
         this.groupBox4.Controls.Add(this.numericUpDown1);
         this.groupBox4.Location = new System.Drawing.Point(6, 133);
         this.groupBox4.Name = "groupBox4";
         this.groupBox4.Size = new System.Drawing.Size(188, 73);
         this.groupBox4.TabIndex = 4;
         this.groupBox4.TabStop = false;
         this.groupBox4.Text = "Memory Range";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(6, 49);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(57, 13);
         this.label2.TabIndex = 3;
         this.label2.Text = "Max Value";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(6, 21);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(54, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "Min Value";
         // 
         // numericUpDown2
         // 
         this.numericUpDown2.Increment = new decimal(new int[] {
            1024,
            0,
            0,
            0});
         this.numericUpDown2.Location = new System.Drawing.Point(107, 47);
         this.numericUpDown2.Maximum = new decimal(new int[] {
            419430400,
            0,
            0,
            0});
         this.numericUpDown2.Name = "numericUpDown2";
         this.numericUpDown2.Size = new System.Drawing.Size(72, 20);
         this.numericUpDown2.TabIndex = 1;
         this.numericUpDown2.Value = new decimal(new int[] {
            419430400,
            0,
            0,
            0});
         // 
         // numericUpDown1
         // 
         this.numericUpDown1.Increment = new decimal(new int[] {
            1024,
            0,
            0,
            0});
         this.numericUpDown1.Location = new System.Drawing.Point(107, 19);
         this.numericUpDown1.Maximum = new decimal(new int[] {
            419430400,
            0,
            0,
            0});
         this.numericUpDown1.Name = "numericUpDown1";
         this.numericUpDown1.Size = new System.Drawing.Size(72, 20);
         this.numericUpDown1.TabIndex = 0;
         // 
         // checkBox2
         // 
         this.checkBox2.AutoSize = true;
         this.checkBox2.Checked = true;
         this.checkBox2.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox2.Location = new System.Drawing.Point(105, 259);
         this.checkBox2.Name = "checkBox2";
         this.checkBox2.Size = new System.Drawing.Size(93, 17);
         this.checkBox2.TabIndex = 3;
         this.checkBox2.Text = "Print Summary";
         this.checkBox2.UseVisualStyleBackColor = true;
         // 
         // groupBox3
         // 
         this.groupBox3.Controls.Add(this.textBox2);
         this.groupBox3.Location = new System.Drawing.Point(6, 76);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(188, 51);
         this.groupBox3.TabIndex = 2;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Ignore String";
         // 
         // textBox2
         // 
         this.textBox2.Location = new System.Drawing.Point(6, 19);
         this.textBox2.Name = "textBox2";
         this.textBox2.Size = new System.Drawing.Size(176, 20);
         this.textBox2.TabIndex = 0;
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(113, 282);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 1;
         this.button1.Text = "Search";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Checked = true;
         this.checkBox1.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox1.Location = new System.Drawing.Point(105, 236);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(83, 17);
         this.checkBox1.TabIndex = 2;
         this.checkBox1.Text = "Sort Results";
         this.checkBox1.UseVisualStyleBackColor = true;
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.textBox1);
         this.groupBox2.Location = new System.Drawing.Point(6, 19);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(188, 51);
         this.groupBox2.TabIndex = 1;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Contains String";
         // 
         // textBox1
         // 
         this.textBox1.Location = new System.Drawing.Point(6, 19);
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(176, 20);
         this.textBox1.TabIndex = 0;
         // 
         // tabControl1
         // 
         this.tabControl1.Controls.Add(this.tabPage1);
         this.tabControl1.Controls.Add(this.tabPage2);
         this.tabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.tabControl1.Location = new System.Drawing.Point(0, 24);
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(1414, 868);
         this.tabControl1.TabIndex = 4;
         // 
         // tabPage1
         // 
         this.tabPage1.Controls.Add(this.groupBox1);
         this.tabPage1.Controls.Add(this.listBox1);
         this.tabPage1.Location = new System.Drawing.Point(4, 22);
         this.tabPage1.Name = "tabPage1";
         this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage1.Size = new System.Drawing.Size(1406, 842);
         this.tabPage1.TabIndex = 0;
         this.tabPage1.Text = "Search View";
         this.tabPage1.UseVisualStyleBackColor = true;
         // 
         // tabPage2
         // 
         this.tabPage2.Controls.Add(this.button2);
         this.tabPage2.Controls.Add(this.treeView1);
         this.tabPage2.Location = new System.Drawing.Point(4, 22);
         this.tabPage2.Name = "tabPage2";
         this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage2.Size = new System.Drawing.Size(1406, 842);
         this.tabPage2.TabIndex = 1;
         this.tabPage2.Text = "Tree View";
         this.tabPage2.UseVisualStyleBackColor = true;
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(8, 6);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 1;
         this.button2.Text = "populate";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // Form1
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(1414, 892);
         this.Controls.Add(this.tabControl1);
         this.Controls.Add(this.menuStrip1);
         this.Name = "Form1";
         this.Text = "Mem View 2";
         this.contextMenuStrip1.ResumeLayout(false);
         this.menuStrip1.ResumeLayout(false);
         this.menuStrip1.PerformLayout();
         this.contextMenuStrip2.ResumeLayout(false);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox4.ResumeLayout(false);
         this.groupBox4.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).EndInit();
         this.groupBox3.ResumeLayout(false);
         this.groupBox3.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         this.tabControl1.ResumeLayout(false);
         this.tabPage1.ResumeLayout(false);
         this.tabPage2.ResumeLayout(false);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TreeView treeView1;
      private System.Windows.Forms.MenuStrip menuStrip1;
      private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem openXMLToolStripMenuItem;
      private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
      private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
      private System.Windows.Forms.ContextMenuStrip contextMenuStrip1;
      private System.Windows.Forms.ToolStripMenuItem copyThisStackToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem copyThisNodesChildrenOnlyToolStripMenuItem;
      private System.Windows.Forms.ListBox listBox1;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.GroupBox groupBox3;
      private System.Windows.Forms.TextBox textBox2;
      private System.Windows.Forms.ContextMenuStrip contextMenuStrip2;
      private System.Windows.Forms.ToolStripMenuItem copyToClipboardToolStripMenuItem;
      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.TabPage tabPage1;
      private System.Windows.Forms.TabPage tabPage2;
      private System.Windows.Forms.ToolStripSeparator toolStripMenuItem2;
      private System.Windows.Forms.ToolStripMenuItem selectAllToolStripMenuItem;
      private System.Windows.Forms.CheckBox checkBox2;
      private System.Windows.Forms.GroupBox groupBox4;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.NumericUpDown numericUpDown2;
      private System.Windows.Forms.NumericUpDown numericUpDown1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.ToolStripSeparator toolStripMenuItem3;
      private System.Windows.Forms.ToolStripMenuItem safeToDiskToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem filterToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem mFilterLarge;
      private System.Windows.Forms.ToolStripMenuItem mFilterIncreasing;
      private System.Windows.Forms.ToolStripMenuItem mFilterColorOnly;
      private System.Windows.Forms.ToolStripMenuItem mFilterIncreasingNonTrivial;
      private System.Windows.Forms.ToolStripMenuItem mFilterAlwaysIncreasing;
   }
}

