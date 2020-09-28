namespace PhoenixEditor.ScenarioEditor
{
   partial class ObjectivesControl
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
         this.listBoxObjectives = new System.Windows.Forms.ListBox();
         this.lblObjectives = new System.Windows.Forms.Label();
         this.lblObjectiveName = new System.Windows.Forms.Label();
         this.txtBoxObjectiveName = new System.Windows.Forms.TextBox();
         this.lblAssignedPlayers = new System.Windows.Forms.Label();
         this.chkBoxPlayer1 = new System.Windows.Forms.CheckBox();
         this.chkBoxPlayer2 = new System.Windows.Forms.CheckBox();
         this.chkBoxPlayer3 = new System.Windows.Forms.CheckBox();
         this.chkBoxPlayer4 = new System.Windows.Forms.CheckBox();
         this.chkBoxPlayer5 = new System.Windows.Forms.CheckBox();
         this.chkBoxPlayer6 = new System.Windows.Forms.CheckBox();
         this.lblRequiredObjective = new System.Windows.Forms.Label();
         this.chkBoxRequiredObjective = new System.Windows.Forms.CheckBox();
         this.txtBoxObjectiveDescription = new System.Windows.Forms.TextBox();
         this.txtBoxHintDescription = new System.Windows.Forms.TextBox();
         this.btnNew = new System.Windows.Forms.Button();
         this.btnDelete = new System.Windows.Forms.Button();
         this.btnApplyObjectiveName = new System.Windows.Forms.Button();
         this.mTxtDescriptionStringID = new System.Windows.Forms.TextBox();
         this.label1 = new System.Windows.Forms.Label();
         this.mTxtHintStringID = new System.Windows.Forms.TextBox();
         this.label2 = new System.Windows.Forms.Label();
         this.mGroupHint = new System.Windows.Forms.GroupBox();
         this.mBtnGetHintLocString = new System.Windows.Forms.Button();
         this.mLblStatusHint = new System.Windows.Forms.Label();
         this.mGroupDescription = new System.Windows.Forms.GroupBox();
         this.mBtnGetDescriptionLocString = new System.Windows.Forms.Button();
         this.mLblStatusDescription = new System.Windows.Forms.Label();
         this.txtBoxScore = new System.Windows.Forms.TextBox();
         this.lblScore = new System.Windows.Forms.Label();
         this.btnApplyScore = new System.Windows.Forms.Button();
         this.CountNeeded = new System.Windows.Forms.NumericUpDown();
         this.label5 = new System.Windows.Forms.Label();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.mBtnGetTrackerTextString = new System.Windows.Forms.Button();
         this.mTxtTrackerStringID = new System.Windows.Forms.TextBox();
         this.label4 = new System.Windows.Forms.Label();
         this.textTrackerText = new System.Windows.Forms.TextBox();
         this.trackerDuration = new System.Windows.Forms.NumericUpDown();
         this.label3 = new System.Windows.Forms.Label();
         this.minTrackerIncrement = new System.Windows.Forms.NumericUpDown();
         this.label6 = new System.Windows.Forms.Label();
         this.mGroupHint.SuspendLayout();
         this.mGroupDescription.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.CountNeeded)).BeginInit();
         this.groupBox1.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.trackerDuration)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.minTrackerIncrement)).BeginInit();
         this.SuspendLayout();
         // 
         // listBoxObjectives
         // 
         this.listBoxObjectives.FormattingEnabled = true;
         this.listBoxObjectives.Location = new System.Drawing.Point(19, 35);
         this.listBoxObjectives.Name = "listBoxObjectives";
         this.listBoxObjectives.Size = new System.Drawing.Size(128, 498);
         this.listBoxObjectives.Sorted = true;
         this.listBoxObjectives.TabIndex = 0;
         this.listBoxObjectives.SelectedIndexChanged += new System.EventHandler(this.listBoxObjectives_SelectedIndexChanged);
         // 
         // lblObjectives
         // 
         this.lblObjectives.AutoSize = true;
         this.lblObjectives.Location = new System.Drawing.Point(16, 16);
         this.lblObjectives.Name = "lblObjectives";
         this.lblObjectives.Size = new System.Drawing.Size(57, 13);
         this.lblObjectives.TabIndex = 1;
         this.lblObjectives.Text = "Objectives";
         // 
         // lblObjectiveName
         // 
         this.lblObjectiveName.AutoSize = true;
         this.lblObjectiveName.Location = new System.Drawing.Point(170, 16);
         this.lblObjectiveName.Name = "lblObjectiveName";
         this.lblObjectiveName.Size = new System.Drawing.Size(83, 13);
         this.lblObjectiveName.TabIndex = 2;
         this.lblObjectiveName.Text = "Objective Name";
         // 
         // txtBoxObjectiveName
         // 
         this.txtBoxObjectiveName.Location = new System.Drawing.Point(173, 35);
         this.txtBoxObjectiveName.MaxLength = 128;
         this.txtBoxObjectiveName.Name = "txtBoxObjectiveName";
         this.txtBoxObjectiveName.Size = new System.Drawing.Size(138, 20);
         this.txtBoxObjectiveName.TabIndex = 1;
         this.txtBoxObjectiveName.WordWrap = false;
         this.txtBoxObjectiveName.TextChanged += new System.EventHandler(this.txtBoxObjectiveName_TextChanged);
         this.txtBoxObjectiveName.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.txtBoxObjectiveName_KeyPress);
         // 
         // lblAssignedPlayers
         // 
         this.lblAssignedPlayers.AutoSize = true;
         this.lblAssignedPlayers.Location = new System.Drawing.Point(173, 62);
         this.lblAssignedPlayers.Name = "lblAssignedPlayers";
         this.lblAssignedPlayers.Size = new System.Drawing.Size(87, 13);
         this.lblAssignedPlayers.TabIndex = 4;
         this.lblAssignedPlayers.Text = "Assigned Players";
         // 
         // chkBoxPlayer1
         // 
         this.chkBoxPlayer1.AutoSize = true;
         this.chkBoxPlayer1.Checked = true;
         this.chkBoxPlayer1.CheckState = System.Windows.Forms.CheckState.Checked;
         this.chkBoxPlayer1.Location = new System.Drawing.Point(176, 79);
         this.chkBoxPlayer1.Name = "chkBoxPlayer1";
         this.chkBoxPlayer1.Size = new System.Drawing.Size(64, 17);
         this.chkBoxPlayer1.TabIndex = 3;
         this.chkBoxPlayer1.Text = "Player 1";
         this.chkBoxPlayer1.UseVisualStyleBackColor = true;
         this.chkBoxPlayer1.CheckedChanged += new System.EventHandler(this.chkBoxPlayer1_CheckedChanged);
         // 
         // chkBoxPlayer2
         // 
         this.chkBoxPlayer2.AutoSize = true;
         this.chkBoxPlayer2.Location = new System.Drawing.Point(247, 79);
         this.chkBoxPlayer2.Name = "chkBoxPlayer2";
         this.chkBoxPlayer2.Size = new System.Drawing.Size(64, 17);
         this.chkBoxPlayer2.TabIndex = 4;
         this.chkBoxPlayer2.Text = "Player 2";
         this.chkBoxPlayer2.UseVisualStyleBackColor = true;
         this.chkBoxPlayer2.CheckedChanged += new System.EventHandler(this.chkBoxPlayer2_CheckedChanged);
         // 
         // chkBoxPlayer3
         // 
         this.chkBoxPlayer3.AutoSize = true;
         this.chkBoxPlayer3.Location = new System.Drawing.Point(318, 79);
         this.chkBoxPlayer3.Name = "chkBoxPlayer3";
         this.chkBoxPlayer3.Size = new System.Drawing.Size(64, 17);
         this.chkBoxPlayer3.TabIndex = 5;
         this.chkBoxPlayer3.Text = "Player 3";
         this.chkBoxPlayer3.UseVisualStyleBackColor = true;
         this.chkBoxPlayer3.CheckedChanged += new System.EventHandler(this.chkBoxPlayer3_CheckedChanged);
         // 
         // chkBoxPlayer4
         // 
         this.chkBoxPlayer4.AutoSize = true;
         this.chkBoxPlayer4.Location = new System.Drawing.Point(389, 79);
         this.chkBoxPlayer4.Name = "chkBoxPlayer4";
         this.chkBoxPlayer4.Size = new System.Drawing.Size(64, 17);
         this.chkBoxPlayer4.TabIndex = 6;
         this.chkBoxPlayer4.Text = "Player 4";
         this.chkBoxPlayer4.UseVisualStyleBackColor = true;
         this.chkBoxPlayer4.CheckedChanged += new System.EventHandler(this.chkBoxPlayer4_CheckedChanged);
         // 
         // chkBoxPlayer5
         // 
         this.chkBoxPlayer5.AutoSize = true;
         this.chkBoxPlayer5.Location = new System.Drawing.Point(460, 79);
         this.chkBoxPlayer5.Name = "chkBoxPlayer5";
         this.chkBoxPlayer5.Size = new System.Drawing.Size(64, 17);
         this.chkBoxPlayer5.TabIndex = 7;
         this.chkBoxPlayer5.Text = "Player 5";
         this.chkBoxPlayer5.UseVisualStyleBackColor = true;
         this.chkBoxPlayer5.CheckedChanged += new System.EventHandler(this.chkBoxPlayer5_CheckedChanged);
         // 
         // chkBoxPlayer6
         // 
         this.chkBoxPlayer6.AutoSize = true;
         this.chkBoxPlayer6.Location = new System.Drawing.Point(531, 79);
         this.chkBoxPlayer6.Name = "chkBoxPlayer6";
         this.chkBoxPlayer6.Size = new System.Drawing.Size(64, 17);
         this.chkBoxPlayer6.TabIndex = 8;
         this.chkBoxPlayer6.Text = "Player 6";
         this.chkBoxPlayer6.UseVisualStyleBackColor = true;
         this.chkBoxPlayer6.CheckedChanged += new System.EventHandler(this.chkBoxPlayer6_CheckedChanged);
         // 
         // lblRequiredObjective
         // 
         this.lblRequiredObjective.AutoSize = true;
         this.lblRequiredObjective.Location = new System.Drawing.Point(173, 104);
         this.lblRequiredObjective.Name = "lblRequiredObjective";
         this.lblRequiredObjective.Size = new System.Drawing.Size(98, 13);
         this.lblRequiredObjective.TabIndex = 11;
         this.lblRequiredObjective.Text = "Required Objective";
         // 
         // chkBoxRequiredObjective
         // 
         this.chkBoxRequiredObjective.AutoSize = true;
         this.chkBoxRequiredObjective.Checked = true;
         this.chkBoxRequiredObjective.CheckState = System.Windows.Forms.CheckState.Checked;
         this.chkBoxRequiredObjective.Location = new System.Drawing.Point(176, 120);
         this.chkBoxRequiredObjective.Name = "chkBoxRequiredObjective";
         this.chkBoxRequiredObjective.Size = new System.Drawing.Size(65, 17);
         this.chkBoxRequiredObjective.TabIndex = 9;
         this.chkBoxRequiredObjective.Text = "Enabled";
         this.chkBoxRequiredObjective.UseVisualStyleBackColor = true;
         this.chkBoxRequiredObjective.CheckedChanged += new System.EventHandler(this.chkBoxRequiredObjective_CheckedChanged);
         // 
         // txtBoxObjectiveDescription
         // 
         this.txtBoxObjectiveDescription.Location = new System.Drawing.Point(13, 57);
         this.txtBoxObjectiveDescription.MaxLength = 1024;
         this.txtBoxObjectiveDescription.Multiline = true;
         this.txtBoxObjectiveDescription.Name = "txtBoxObjectiveDescription";
         this.txtBoxObjectiveDescription.ReadOnly = true;
         this.txtBoxObjectiveDescription.Size = new System.Drawing.Size(612, 60);
         this.txtBoxObjectiveDescription.TabIndex = 10;
         // 
         // txtBoxHintDescription
         // 
         this.txtBoxHintDescription.Location = new System.Drawing.Point(13, 61);
         this.txtBoxHintDescription.MaxLength = 1024;
         this.txtBoxHintDescription.Multiline = true;
         this.txtBoxHintDescription.Name = "txtBoxHintDescription";
         this.txtBoxHintDescription.ReadOnly = true;
         this.txtBoxHintDescription.Size = new System.Drawing.Size(612, 60);
         this.txtBoxHintDescription.TabIndex = 12;
         // 
         // btnNew
         // 
         this.btnNew.Location = new System.Drawing.Point(19, 540);
         this.btnNew.Name = "btnNew";
         this.btnNew.Size = new System.Drawing.Size(75, 23);
         this.btnNew.TabIndex = 14;
         this.btnNew.Text = "New";
         this.btnNew.UseVisualStyleBackColor = true;
         this.btnNew.Click += new System.EventHandler(this.btnNew_Click);
         // 
         // btnDelete
         // 
         this.btnDelete.Location = new System.Drawing.Point(19, 570);
         this.btnDelete.Name = "btnDelete";
         this.btnDelete.Size = new System.Drawing.Size(75, 23);
         this.btnDelete.TabIndex = 15;
         this.btnDelete.Text = "Delete";
         this.btnDelete.UseVisualStyleBackColor = true;
         this.btnDelete.Click += new System.EventHandler(this.Delete_Click);
         // 
         // btnApplyObjectiveName
         // 
         this.btnApplyObjectiveName.Location = new System.Drawing.Point(318, 32);
         this.btnApplyObjectiveName.Name = "btnApplyObjectiveName";
         this.btnApplyObjectiveName.Size = new System.Drawing.Size(75, 23);
         this.btnApplyObjectiveName.TabIndex = 2;
         this.btnApplyObjectiveName.Text = "Apply";
         this.btnApplyObjectiveName.UseVisualStyleBackColor = true;
         this.btnApplyObjectiveName.Click += new System.EventHandler(this.btnApplyObjectiveName_Click);
         // 
         // mTxtDescriptionStringID
         // 
         this.mTxtDescriptionStringID.Location = new System.Drawing.Point(98, 21);
         this.mTxtDescriptionStringID.Name = "mTxtDescriptionStringID";
         this.mTxtDescriptionStringID.Size = new System.Drawing.Size(100, 20);
         this.mTxtDescriptionStringID.TabIndex = 16;
         this.mTxtDescriptionStringID.TextChanged += new System.EventHandler(this.mTxtDescriptionStringID_TextChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(16, 24);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(72, 13);
         this.label1.TabIndex = 17;
         this.label1.Text = "Loc String ID:";
         // 
         // mTxtHintStringID
         // 
         this.mTxtHintStringID.Enabled = false;
         this.mTxtHintStringID.Location = new System.Drawing.Point(98, 24);
         this.mTxtHintStringID.Name = "mTxtHintStringID";
         this.mTxtHintStringID.Size = new System.Drawing.Size(100, 20);
         this.mTxtHintStringID.TabIndex = 18;
         this.mTxtHintStringID.TextChanged += new System.EventHandler(this.mTxtHintStringID_TextChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(16, 24);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(72, 13);
         this.label2.TabIndex = 19;
         this.label2.Text = "Loc String ID:";
         // 
         // mGroupHint
         // 
         this.mGroupHint.Controls.Add(this.mBtnGetHintLocString);
         this.mGroupHint.Controls.Add(this.mLblStatusHint);
         this.mGroupHint.Controls.Add(this.mTxtHintStringID);
         this.mGroupHint.Controls.Add(this.label2);
         this.mGroupHint.Controls.Add(this.txtBoxHintDescription);
         this.mGroupHint.Location = new System.Drawing.Point(173, 387);
         this.mGroupHint.Name = "mGroupHint";
         this.mGroupHint.Size = new System.Drawing.Size(644, 130);
         this.mGroupHint.TabIndex = 20;
         this.mGroupHint.TabStop = false;
         this.mGroupHint.Text = "Objective Description Hint";
         // 
         // mBtnGetHintLocString
         // 
         this.mBtnGetHintLocString.Location = new System.Drawing.Point(204, 22);
         this.mBtnGetHintLocString.Name = "mBtnGetHintLocString";
         this.mBtnGetHintLocString.Size = new System.Drawing.Size(23, 23);
         this.mBtnGetHintLocString.TabIndex = 19;
         this.mBtnGetHintLocString.Text = "...";
         this.mBtnGetHintLocString.UseVisualStyleBackColor = true;
         this.mBtnGetHintLocString.Click += new System.EventHandler(this.mBtnGetHintLocString_Click);
         // 
         // mLblStatusHint
         // 
         this.mLblStatusHint.Location = new System.Drawing.Point(328, 24);
         this.mLblStatusHint.Name = "mLblStatusHint";
         this.mLblStatusHint.Size = new System.Drawing.Size(297, 18);
         this.mLblStatusHint.TabIndex = 18;
         // 
         // mGroupDescription
         // 
         this.mGroupDescription.Controls.Add(this.mBtnGetDescriptionLocString);
         this.mGroupDescription.Controls.Add(this.mLblStatusDescription);
         this.mGroupDescription.Controls.Add(this.txtBoxObjectiveDescription);
         this.mGroupDescription.Controls.Add(this.label1);
         this.mGroupDescription.Controls.Add(this.mTxtDescriptionStringID);
         this.mGroupDescription.Location = new System.Drawing.Point(173, 161);
         this.mGroupDescription.Name = "mGroupDescription";
         this.mGroupDescription.Size = new System.Drawing.Size(644, 130);
         this.mGroupDescription.TabIndex = 21;
         this.mGroupDescription.TabStop = false;
         this.mGroupDescription.Text = "Objective Description";
         // 
         // mBtnGetDescriptionLocString
         // 
         this.mBtnGetDescriptionLocString.Location = new System.Drawing.Point(204, 19);
         this.mBtnGetDescriptionLocString.Name = "mBtnGetDescriptionLocString";
         this.mBtnGetDescriptionLocString.Size = new System.Drawing.Size(23, 23);
         this.mBtnGetDescriptionLocString.TabIndex = 19;
         this.mBtnGetDescriptionLocString.Text = "...";
         this.mBtnGetDescriptionLocString.UseVisualStyleBackColor = true;
         this.mBtnGetDescriptionLocString.Click += new System.EventHandler(this.mBtnGetDescriptionLocString_Click);
         // 
         // mLblStatusDescription
         // 
         this.mLblStatusDescription.Location = new System.Drawing.Point(325, 24);
         this.mLblStatusDescription.Name = "mLblStatusDescription";
         this.mLblStatusDescription.Size = new System.Drawing.Size(300, 18);
         this.mLblStatusDescription.TabIndex = 18;
         // 
         // txtBoxScore
         // 
         this.txtBoxScore.Location = new System.Drawing.Point(211, 526);
         this.txtBoxScore.MaxLength = 10;
         this.txtBoxScore.Name = "txtBoxScore";
         this.txtBoxScore.Size = new System.Drawing.Size(100, 20);
         this.txtBoxScore.TabIndex = 22;
         this.txtBoxScore.WordWrap = false;
         this.txtBoxScore.TextChanged += new System.EventHandler(this.txtBoxScore_TextChanged);
         this.txtBoxScore.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.txtBoxScore_KeyPress);
         // 
         // lblScore
         // 
         this.lblScore.AutoSize = true;
         this.lblScore.Location = new System.Drawing.Point(170, 529);
         this.lblScore.Name = "lblScore";
         this.lblScore.Size = new System.Drawing.Size(35, 13);
         this.lblScore.TabIndex = 23;
         this.lblScore.Text = "Score";
         // 
         // btnApplyScore
         // 
         this.btnApplyScore.Location = new System.Drawing.Point(317, 524);
         this.btnApplyScore.Name = "btnApplyScore";
         this.btnApplyScore.Size = new System.Drawing.Size(75, 23);
         this.btnApplyScore.TabIndex = 20;
         this.btnApplyScore.Text = "Apply";
         this.btnApplyScore.UseVisualStyleBackColor = true;
         this.btnApplyScore.Click += new System.EventHandler(this.btnApplyScore_Click);
         // 
         // CountNeeded
         // 
         this.CountNeeded.Location = new System.Drawing.Point(244, 570);
         this.CountNeeded.Maximum = new decimal(new int[] {
            1000,
            0,
            0,
            0});
         this.CountNeeded.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            -2147483648});
         this.CountNeeded.Name = "CountNeeded";
         this.CountNeeded.Size = new System.Drawing.Size(65, 20);
         this.CountNeeded.TabIndex = 29;
         this.CountNeeded.Value = new decimal(new int[] {
            1,
            0,
            0,
            -2147483648});
         this.CountNeeded.ValueChanged += new System.EventHandler(this.CountNeeded_ValueChanged);
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(171, 572);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(44, 13);
         this.label5.TabIndex = 30;
         this.label5.Text = "Counter";
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.label6);
         this.groupBox1.Controls.Add(this.minTrackerIncrement);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.trackerDuration);
         this.groupBox1.Controls.Add(this.mBtnGetTrackerTextString);
         this.groupBox1.Controls.Add(this.mTxtTrackerStringID);
         this.groupBox1.Controls.Add(this.label4);
         this.groupBox1.Controls.Add(this.textTrackerText);
         this.groupBox1.Location = new System.Drawing.Point(173, 297);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(644, 84);
         this.groupBox1.TabIndex = 21;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Progress Tracking";
         // 
         // mBtnGetTrackerTextString
         // 
         this.mBtnGetTrackerTextString.Location = new System.Drawing.Point(204, 22);
         this.mBtnGetTrackerTextString.Name = "mBtnGetTrackerTextString";
         this.mBtnGetTrackerTextString.Size = new System.Drawing.Size(23, 23);
         this.mBtnGetTrackerTextString.TabIndex = 19;
         this.mBtnGetTrackerTextString.Text = "...";
         this.mBtnGetTrackerTextString.UseVisualStyleBackColor = true;
         this.mBtnGetTrackerTextString.Click += new System.EventHandler(this.mBtnGetTrackerTextString_Click);
         // 
         // mTxtTrackerStringID
         // 
         this.mTxtTrackerStringID.Enabled = false;
         this.mTxtTrackerStringID.Location = new System.Drawing.Point(98, 24);
         this.mTxtTrackerStringID.Name = "mTxtTrackerStringID";
         this.mTxtTrackerStringID.Size = new System.Drawing.Size(100, 20);
         this.mTxtTrackerStringID.TabIndex = 18;
         this.mTxtTrackerStringID.TextChanged += new System.EventHandler(this.mTxtTrackerStringID_TextChanged);
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(16, 24);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(72, 13);
         this.label4.TabIndex = 19;
         this.label4.Text = "Loc String ID:";
         // 
         // textTrackerText
         // 
         this.textTrackerText.Location = new System.Drawing.Point(13, 50);
         this.textTrackerText.MaxLength = 1024;
         this.textTrackerText.Multiline = true;
         this.textTrackerText.Name = "textTrackerText";
         this.textTrackerText.ReadOnly = true;
         this.textTrackerText.Size = new System.Drawing.Size(612, 25);
         this.textTrackerText.TabIndex = 12;
         // 
         // trackerDuration
         // 
         this.trackerDuration.Location = new System.Drawing.Point(298, 25);
         this.trackerDuration.Maximum = new decimal(new int[] {
            60000,
            0,
            0,
            0});
         this.trackerDuration.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
         this.trackerDuration.Name = "trackerDuration";
         this.trackerDuration.Size = new System.Drawing.Size(65, 20);
         this.trackerDuration.TabIndex = 30;
         this.trackerDuration.Value = new decimal(new int[] {
            8000,
            0,
            0,
            0});
         this.trackerDuration.ValueChanged += new System.EventHandler(this.trackerDuration_ValueChanged);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(242, 27);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(50, 13);
         this.label3.TabIndex = 31;
         this.label3.Text = "Duration:";
         // 
         // minTrackerIncrement
         // 
         this.minTrackerIncrement.Location = new System.Drawing.Point(560, 25);
         this.minTrackerIncrement.Maximum = new decimal(new int[] {
            1000,
            0,
            0,
            0});
         this.minTrackerIncrement.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
         this.minTrackerIncrement.Name = "minTrackerIncrement";
         this.minTrackerIncrement.Size = new System.Drawing.Size(65, 20);
         this.minTrackerIncrement.TabIndex = 32;
         this.minTrackerIncrement.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
         this.minTrackerIncrement.ValueChanged += new System.EventHandler(this.minTrackerIncrement_ValueChanged);
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Location = new System.Drawing.Point(378, 27);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(176, 13);
         this.label6.TabIndex = 33;
         this.label6.Text = "Minumum Increment for Notification:";
         // 
         // ObjectivesControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.SystemColors.ControlDark;
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.label5);
         this.Controls.Add(this.CountNeeded);
         this.Controls.Add(this.btnApplyScore);
         this.Controls.Add(this.lblScore);
         this.Controls.Add(this.txtBoxScore);
         this.Controls.Add(this.mGroupDescription);
         this.Controls.Add(this.mGroupHint);
         this.Controls.Add(this.btnApplyObjectiveName);
         this.Controls.Add(this.btnDelete);
         this.Controls.Add(this.btnNew);
         this.Controls.Add(this.chkBoxRequiredObjective);
         this.Controls.Add(this.lblRequiredObjective);
         this.Controls.Add(this.chkBoxPlayer6);
         this.Controls.Add(this.chkBoxPlayer5);
         this.Controls.Add(this.chkBoxPlayer4);
         this.Controls.Add(this.chkBoxPlayer3);
         this.Controls.Add(this.chkBoxPlayer2);
         this.Controls.Add(this.chkBoxPlayer1);
         this.Controls.Add(this.lblAssignedPlayers);
         this.Controls.Add(this.txtBoxObjectiveName);
         this.Controls.Add(this.lblObjectiveName);
         this.Controls.Add(this.lblObjectives);
         this.Controls.Add(this.listBoxObjectives);
         this.Name = "ObjectivesControl";
         this.Size = new System.Drawing.Size(842, 608);
         this.mGroupHint.ResumeLayout(false);
         this.mGroupHint.PerformLayout();
         this.mGroupDescription.ResumeLayout(false);
         this.mGroupDescription.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.CountNeeded)).EndInit();
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.trackerDuration)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.minTrackerIncrement)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ListBox listBoxObjectives;
      private System.Windows.Forms.Label lblObjectives;
      private System.Windows.Forms.Label lblObjectiveName;
      private System.Windows.Forms.TextBox txtBoxObjectiveName;
      private System.Windows.Forms.Label lblAssignedPlayers;
      private System.Windows.Forms.CheckBox chkBoxPlayer1;
      private System.Windows.Forms.CheckBox chkBoxPlayer2;
      private System.Windows.Forms.CheckBox chkBoxPlayer3;
      private System.Windows.Forms.CheckBox chkBoxPlayer4;
      private System.Windows.Forms.CheckBox chkBoxPlayer5;
      private System.Windows.Forms.CheckBox chkBoxPlayer6;
      private System.Windows.Forms.Label lblRequiredObjective;
      private System.Windows.Forms.CheckBox chkBoxRequiredObjective;
      private System.Windows.Forms.TextBox txtBoxObjectiveDescription;
      private System.Windows.Forms.TextBox txtBoxHintDescription;
      private System.Windows.Forms.Button btnNew;
      private System.Windows.Forms.Button btnDelete;
      private System.Windows.Forms.Button btnApplyObjectiveName;
      private System.Windows.Forms.TextBox mTxtDescriptionStringID;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.TextBox mTxtHintStringID;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.GroupBox mGroupHint;
      private System.Windows.Forms.GroupBox mGroupDescription;
      private System.Windows.Forms.Label mLblStatusHint;
      private System.Windows.Forms.Label mLblStatusDescription;
      private System.Windows.Forms.Button mBtnGetHintLocString;
      private System.Windows.Forms.Button mBtnGetDescriptionLocString;
      private System.Windows.Forms.TextBox txtBoxScore;
      private System.Windows.Forms.Label lblScore;
      private System.Windows.Forms.Button btnApplyScore;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.NumericUpDown CountNeeded;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Button mBtnGetTrackerTextString;
      private System.Windows.Forms.TextBox mTxtTrackerStringID;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.TextBox textTrackerText;
      private System.Windows.Forms.NumericUpDown trackerDuration;
      private System.Windows.Forms.Label label6;
      private System.Windows.Forms.NumericUpDown minTrackerIncrement;
      private System.Windows.Forms.Label label3;
   }
}
