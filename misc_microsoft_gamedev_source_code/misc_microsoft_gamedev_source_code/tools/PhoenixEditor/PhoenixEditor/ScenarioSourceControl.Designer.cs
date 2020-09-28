namespace PhoenixEditor
{
   partial class ScenarioSourceControl
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
         this.components = new System.ComponentModel.Container();
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ScenarioSourceControl));
         this.AddButton = new System.Windows.Forms.Button();
         this.DescriptionTextBox = new System.Windows.Forms.TextBox();
         this.label1 = new System.Windows.Forms.Label();
         this.ChangeListLabel = new System.Windows.Forms.Label();
         this.SubmitButton = new System.Windows.Forms.Button();
         this.CancelChangesButton = new System.Windows.Forms.Button();
         this.CheckoutButton = new System.Windows.Forms.Button();
         this.imageList1 = new System.Windows.Forms.ImageList(this.components);
         this.FileListGridControl = new Xceed.Grid.GridControl();
         this.colActionOwner = new Xceed.Grid.DataBoundColumn();
         this.colCheckedOut = new Xceed.Grid.DataBoundColumn();
         this.colInPerforce = new Xceed.Grid.DataBoundColumn();
         this.colCheckedOutOtherUser = new Xceed.Grid.DataBoundColumn();
         this.colState = new Xceed.Grid.DataBoundColumn();
         this.colCheckedOutThisUser = new Xceed.Grid.DataBoundColumn();
         this.colDepotFile = new Xceed.Grid.DataBoundColumn();
         this.colClientFile = new Xceed.Grid.DataBoundColumn();
         this.colIsLatestRevision = new Xceed.Grid.DataBoundColumn();
         this.colUserChangeListNumber = new Xceed.Grid.DataBoundColumn();
         this.dataRowTemplate1 = new Xceed.Grid.DataRow();
         this.celldataRowTemplate1ActionOwner = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1CheckedOut = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1InPerforce = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1CheckedOutOtherUser = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1State = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1CheckedOutThisUser = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1DepotFile = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1ClientFile = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1IsLatestRevision = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1UserChangeListNumber = new Xceed.Grid.DataCell();
         this.simpleFileStatusBindingSource = new System.Windows.Forms.BindingSource(this.components);
         this.groupByRow1 = new Xceed.Grid.GroupByRow();
         this.columnManagerRow1 = new Xceed.Grid.ColumnManagerRow();
         this.cellcolumnManagerRow1ActionOwner = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1CheckedOut = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1InPerforce = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1CheckedOutOtherUser = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1State = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1CheckedOutThisUser = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1DepotFile = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1ClientFile = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1IsLatestRevision = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1UserChangeListNumber = new Xceed.Grid.ColumnManagerCell();
         this.group1 = new Xceed.Grid.Group();
         this.groupManagerRow1 = new Xceed.Grid.GroupManagerRow();
         ((System.ComponentModel.ISupportInitialize)(this.FileListGridControl)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.simpleFileStatusBindingSource)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).BeginInit();
         this.SuspendLayout();
         // 
         // AddButton
         // 
         this.AddButton.Enabled = false;
         this.AddButton.Location = new System.Drawing.Point(27, 19);
         this.AddButton.Name = "AddButton";
         this.AddButton.Size = new System.Drawing.Size(161, 23);
         this.AddButton.TabIndex = 0;
         this.AddButton.Text = "Add To Source Control";
         this.AddButton.UseVisualStyleBackColor = true;
         this.AddButton.Click += new System.EventHandler(this.AddButton_Click);
         // 
         // DescriptionTextBox
         // 
         this.DescriptionTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.DescriptionTextBox.Enabled = false;
         this.DescriptionTextBox.Location = new System.Drawing.Point(27, 373);
         this.DescriptionTextBox.Multiline = true;
         this.DescriptionTextBox.Name = "DescriptionTextBox";
         this.DescriptionTextBox.Size = new System.Drawing.Size(685, 75);
         this.DescriptionTextBox.TabIndex = 2;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(27, 357);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(60, 13);
         this.label1.TabIndex = 3;
         this.label1.Text = "Description";
         // 
         // ChangeListLabel
         // 
         this.ChangeListLabel.AutoSize = true;
         this.ChangeListLabel.Location = new System.Drawing.Point(27, 59);
         this.ChangeListLabel.Name = "ChangeListLabel";
         this.ChangeListLabel.Size = new System.Drawing.Size(66, 13);
         this.ChangeListLabel.TabIndex = 4;
         this.ChangeListLabel.Text = "Change List:";
         // 
         // SubmitButton
         // 
         this.SubmitButton.Enabled = false;
         this.SubmitButton.Location = new System.Drawing.Point(30, 469);
         this.SubmitButton.Name = "SubmitButton";
         this.SubmitButton.Size = new System.Drawing.Size(114, 23);
         this.SubmitButton.TabIndex = 5;
         this.SubmitButton.Text = "Submit";
         this.SubmitButton.UseVisualStyleBackColor = true;
         this.SubmitButton.Click += new System.EventHandler(this.SubmitButton_Click);
         // 
         // CancelChangesButton
         // 
         this.CancelChangesButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.CancelChangesButton.Enabled = false;
         this.CancelChangesButton.Location = new System.Drawing.Point(594, 469);
         this.CancelChangesButton.Name = "CancelChangesButton";
         this.CancelChangesButton.Size = new System.Drawing.Size(118, 23);
         this.CancelChangesButton.TabIndex = 6;
         this.CancelChangesButton.Text = "Cancel Changes";
         this.CancelChangesButton.UseVisualStyleBackColor = true;
         this.CancelChangesButton.Click += new System.EventHandler(this.CancelChangesButton_Click);
         // 
         // CheckoutButton
         // 
         this.CheckoutButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.CheckoutButton.Enabled = false;
         this.CheckoutButton.Location = new System.Drawing.Point(575, 19);
         this.CheckoutButton.Name = "CheckoutButton";
         this.CheckoutButton.Size = new System.Drawing.Size(137, 23);
         this.CheckoutButton.TabIndex = 7;
         this.CheckoutButton.Text = "Checkout";
         this.CheckoutButton.UseVisualStyleBackColor = true;
         this.CheckoutButton.Click += new System.EventHandler(this.CheckoutButton_Click);
         // 
         // imageList1
         // 
         this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
         this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
         this.imageList1.Images.SetKeyName(0, "nosource.bmp");
         this.imageList1.Images.SetKeyName(1, "checkedin.bmp");
         this.imageList1.Images.SetKeyName(2, "checkedoutother.bmp");
         this.imageList1.Images.SetKeyName(3, "checkout.bmp");
         this.imageList1.Images.SetKeyName(4, "checkoutmulti.bmp");
         // 
         // FileListGridControl
         // 
         this.FileListGridControl.AllowCellNavigation = false;
         this.FileListGridControl.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.FileListGridControl.Columns.Add(this.colActionOwner);
         this.FileListGridControl.Columns.Add(this.colCheckedOut);
         this.FileListGridControl.Columns.Add(this.colInPerforce);
         this.FileListGridControl.Columns.Add(this.colCheckedOutOtherUser);
         this.FileListGridControl.Columns.Add(this.colState);
         this.FileListGridControl.Columns.Add(this.colCheckedOutThisUser);
         this.FileListGridControl.Columns.Add(this.colDepotFile);
         this.FileListGridControl.Columns.Add(this.colClientFile);
         this.FileListGridControl.Columns.Add(this.colIsLatestRevision);
         this.FileListGridControl.Columns.Add(this.colUserChangeListNumber);
         this.FileListGridControl.DataRowTemplate = this.dataRowTemplate1;
         this.FileListGridControl.DataSource = this.simpleFileStatusBindingSource;
         this.FileListGridControl.FixedHeaderRows.Add(this.groupByRow1);
         this.FileListGridControl.FixedHeaderRows.Add(this.columnManagerRow1);
         this.FileListGridControl.GroupTemplates.Add(this.group1);
         this.FileListGridControl.Location = new System.Drawing.Point(30, 88);
         this.FileListGridControl.Name = "FileListGridControl";
         this.FileListGridControl.Size = new System.Drawing.Size(682, 256);
         this.FileListGridControl.TabIndex = 8;
         // 
         // colActionOwner
         // 
         this.colActionOwner.SortDirection = Xceed.Grid.SortDirection.None;
         this.colActionOwner.Title = "ActionOwner";
         this.colActionOwner.VisibleIndex = 2;
         this.colActionOwner.Initialize("ActionOwner");
         // 
         // colCheckedOut
         // 
         this.colCheckedOut.SortDirection = Xceed.Grid.SortDirection.None;
         this.colCheckedOut.Title = "CheckedOut";
         this.colCheckedOut.Visible = false;
         this.colCheckedOut.VisibleIndex = 4;
         this.colCheckedOut.Width = 35;
         this.colCheckedOut.Initialize("CheckedOut");
         // 
         // colInPerforce
         // 
         this.colInPerforce.SortDirection = Xceed.Grid.SortDirection.None;
         this.colInPerforce.Title = "InPerforce";
         this.colInPerforce.Visible = false;
         this.colInPerforce.VisibleIndex = 7;
         this.colInPerforce.Width = 48;
         this.colInPerforce.Initialize("InPerforce");
         // 
         // colCheckedOutOtherUser
         // 
         this.colCheckedOutOtherUser.SortDirection = Xceed.Grid.SortDirection.None;
         this.colCheckedOutOtherUser.Title = "CheckedOutOtherUser";
         this.colCheckedOutOtherUser.Visible = false;
         this.colCheckedOutOtherUser.VisibleIndex = 5;
         this.colCheckedOutOtherUser.Width = 43;
         this.colCheckedOutOtherUser.Initialize("CheckedOutOtherUser");
         // 
         // colState
         // 
         this.colState.ForeColor = System.Drawing.SystemColors.MenuBar;
         this.colState.SortDirection = Xceed.Grid.SortDirection.None;
         this.colState.VisibleIndex = 0;
         this.colState.Width = 35;
         this.colState.Initialize("State");
         // 
         // colCheckedOutThisUser
         // 
         this.colCheckedOutThisUser.SortDirection = Xceed.Grid.SortDirection.None;
         this.colCheckedOutThisUser.Title = "CheckedOutThisUser";
         this.colCheckedOutThisUser.Visible = false;
         this.colCheckedOutThisUser.VisibleIndex = 6;
         this.colCheckedOutThisUser.Width = 52;
         this.colCheckedOutThisUser.Initialize("CheckedOutThisUser");
         // 
         // colDepotFile
         // 
         this.colDepotFile.SortDirection = Xceed.Grid.SortDirection.None;
         this.colDepotFile.Title = "DepotFile";
         this.colDepotFile.VisibleIndex = 1;
         this.colDepotFile.Width = 411;
         this.colDepotFile.Initialize("DepotFile");
         // 
         // colClientFile
         // 
         this.colClientFile.SortDirection = Xceed.Grid.SortDirection.None;
         this.colClientFile.Title = "ClientFile";
         this.colClientFile.VisibleIndex = 3;
         this.colClientFile.Width = 363;
         this.colClientFile.Initialize("ClientFile");
         // 
         // colIsLatestRevision
         // 
         this.colIsLatestRevision.SortDirection = Xceed.Grid.SortDirection.None;
         this.colIsLatestRevision.Title = "IsLatestRevision";
         this.colIsLatestRevision.VisibleIndex = 8;
         this.colIsLatestRevision.Initialize("IsLatestRevision");
         // 
         // colUserChangeListNumber
         // 
         this.colUserChangeListNumber.SortDirection = Xceed.Grid.SortDirection.None;
         this.colUserChangeListNumber.Title = "UserChangeListNumber";
         this.colUserChangeListNumber.VisibleIndex = 9;
         this.colUserChangeListNumber.Initialize("UserChangeListNumber");
         // 
         // dataRowTemplate1
         // 
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1ActionOwner);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1CheckedOut);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1InPerforce);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1CheckedOutOtherUser);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1State);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1CheckedOutThisUser);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1DepotFile);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1ClientFile);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1IsLatestRevision);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1UserChangeListNumber);
         this.celldataRowTemplate1ActionOwner.Initialize("ActionOwner");
         this.celldataRowTemplate1CheckedOut.Initialize("CheckedOut");
         this.celldataRowTemplate1InPerforce.Initialize("InPerforce");
         this.celldataRowTemplate1CheckedOutOtherUser.Initialize("CheckedOutOtherUser");
         this.celldataRowTemplate1State.Initialize("State");
         this.celldataRowTemplate1CheckedOutThisUser.Initialize("CheckedOutThisUser");
         this.celldataRowTemplate1DepotFile.Initialize("DepotFile");
         this.celldataRowTemplate1ClientFile.Initialize("ClientFile");
         this.celldataRowTemplate1IsLatestRevision.Initialize("IsLatestRevision");
         this.celldataRowTemplate1UserChangeListNumber.Initialize("UserChangeListNumber");
         // 
         // simpleFileStatusBindingSource
         // 
         this.simpleFileStatusBindingSource.DataSource = typeof(EditorCore.SimpleFileStatus);
         // 
         // columnManagerRow1
         // 
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1ActionOwner);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1CheckedOut);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1InPerforce);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1CheckedOutOtherUser);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1State);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1CheckedOutThisUser);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1DepotFile);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1ClientFile);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1IsLatestRevision);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1UserChangeListNumber);
         this.cellcolumnManagerRow1ActionOwner.Initialize("ActionOwner");
         // 
         // cellcolumnManagerRow1CheckedOut
         // 
         this.cellcolumnManagerRow1CheckedOut.Visible = false;
         this.cellcolumnManagerRow1CheckedOut.Initialize("CheckedOut");
         // 
         // cellcolumnManagerRow1InPerforce
         // 
         this.cellcolumnManagerRow1InPerforce.Visible = false;
         this.cellcolumnManagerRow1InPerforce.Initialize("InPerforce");
         // 
         // cellcolumnManagerRow1CheckedOutOtherUser
         // 
         this.cellcolumnManagerRow1CheckedOutOtherUser.Visible = false;
         this.cellcolumnManagerRow1CheckedOutOtherUser.Initialize("CheckedOutOtherUser");
         this.cellcolumnManagerRow1State.Initialize("State");
         // 
         // cellcolumnManagerRow1CheckedOutThisUser
         // 
         this.cellcolumnManagerRow1CheckedOutThisUser.Visible = false;
         this.cellcolumnManagerRow1CheckedOutThisUser.Initialize("CheckedOutThisUser");
         this.cellcolumnManagerRow1DepotFile.Initialize("DepotFile");
         this.cellcolumnManagerRow1ClientFile.Initialize("ClientFile");
         this.cellcolumnManagerRow1IsLatestRevision.Initialize("IsLatestRevision");
         this.cellcolumnManagerRow1UserChangeListNumber.Initialize("UserChangeListNumber");
         // 
         // group1
         // 
         this.group1.GroupBy = "State";
         this.group1.HeaderRows.Add(this.groupManagerRow1);
         // 
         // ScenarioSourceControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.FileListGridControl);
         this.Controls.Add(this.CheckoutButton);
         this.Controls.Add(this.CancelChangesButton);
         this.Controls.Add(this.SubmitButton);
         this.Controls.Add(this.ChangeListLabel);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.DescriptionTextBox);
         this.Controls.Add(this.AddButton);
         this.Enabled = false;
         this.Name = "ScenarioSourceControl";
         this.Size = new System.Drawing.Size(779, 564);
         ((System.ComponentModel.ISupportInitialize)(this.FileListGridControl)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.simpleFileStatusBindingSource)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button AddButton;
      private System.Windows.Forms.TextBox DescriptionTextBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label ChangeListLabel;
      private System.Windows.Forms.Button SubmitButton;
      private System.Windows.Forms.Button CancelChangesButton;
      private System.Windows.Forms.Button CheckoutButton;
      private System.Windows.Forms.ImageList imageList1;
      private Xceed.Grid.GridControl FileListGridControl;
      private Xceed.Grid.DataRow dataRowTemplate1;
      private System.Windows.Forms.BindingSource simpleFileStatusBindingSource;
      private Xceed.Grid.GroupByRow groupByRow1;
      private Xceed.Grid.ColumnManagerRow columnManagerRow1;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1ActionOwner;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1State;
      private Xceed.Grid.DataBoundColumn colActionOwner;
      private Xceed.Grid.DataBoundColumn colState;
      private Xceed.Grid.DataBoundColumn colDepotFile;
      private Xceed.Grid.DataBoundColumn colClientFile;
      private Xceed.Grid.DataCell celldataRowTemplate1ActionOwner;
      private Xceed.Grid.DataCell celldataRowTemplate1State;
      private Xceed.Grid.DataCell celldataRowTemplate1DepotFile;
      private Xceed.Grid.DataCell celldataRowTemplate1ClientFile;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1DepotFile;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1ClientFile;
      private Xceed.Grid.DataBoundColumn colCheckedOut;
      private Xceed.Grid.DataBoundColumn colInPerforce;
      private Xceed.Grid.DataBoundColumn colCheckedOutOtherUser;
      private Xceed.Grid.DataBoundColumn colCheckedOutThisUser;
      private Xceed.Grid.DataCell celldataRowTemplate1CheckedOut;
      private Xceed.Grid.DataCell celldataRowTemplate1InPerforce;
      private Xceed.Grid.DataCell celldataRowTemplate1CheckedOutOtherUser;
      private Xceed.Grid.DataCell celldataRowTemplate1CheckedOutThisUser;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1CheckedOut;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1InPerforce;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1CheckedOutOtherUser;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1CheckedOutThisUser;
      private Xceed.Grid.DataBoundColumn colIsLatestRevision;
      private Xceed.Grid.DataBoundColumn colUserChangeListNumber;
      private Xceed.Grid.DataCell celldataRowTemplate1IsLatestRevision;
      private Xceed.Grid.DataCell celldataRowTemplate1UserChangeListNumber;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1IsLatestRevision;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1UserChangeListNumber;
      private Xceed.Grid.Group group1;
      private Xceed.Grid.GroupManagerRow groupManagerRow1;
   }
}
