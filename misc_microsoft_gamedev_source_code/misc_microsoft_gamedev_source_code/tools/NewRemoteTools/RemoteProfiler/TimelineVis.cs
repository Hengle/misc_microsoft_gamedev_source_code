using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;
using System.Threading;


using EnsembleStudios.RemoteGameDebugger.Profiler;
using RemoteTools;


namespace EnsembleStudios.RemoteGameDebugger
{

   enum TimelinePacketTypes
   { 
      cFirstPacketType = 50,//Packets.XS.PacketTypes.cNumberOfTypedPackets,
      cTimelineControl = cFirstPacketType,
      cRetiredSamplesPacket,
      cSampleIdentifierPacket,
      cCompressedFramePacket,
      cNumberOfTimelineTypedPackets 
   };

   public class TimelineVis : UserControl/*WeifenLuo.WinFormsUI.DockContent*/ , IFrameObserver, IConnectionHandler
   {
      #region Forms Autocreated Stuff
      private System.Windows.Forms.Button StartPerfSendButton;
      private System.Windows.Forms.Button StopPerfSendButton;
      private EnsembleStudios.RemoteGameDebugger.ProfilerFrameView profilerFrameView1;
      private EnsembleStudios.RemoteGameDebugger.Profiler.FastGraph fastGraph1;
      private System.Windows.Forms.CheckBox AALinesCheckBox;
      private System.Windows.Forms.HScrollBar FrameHorizontalScrollBar;
      private System.Windows.Forms.Button CompressionTestButton;
      private System.Windows.Forms.TextBox TestResultsTextBox;
      private EnsembleStudios.RemoteGameDebugger.Profiler.FastBarGraph SectionBarGraph;
      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.Button SaveButton;
      private EnsembleStudios.RemoteGameDebugger.Profiler.FastBarGraph fastBarGraph1;
      private System.Windows.Forms.Button FilterSectionsButton;
      private System.Windows.Forms.Panel panel3;
      private System.Windows.Forms.Panel panel6;
      private EnsembleStudios.RemoteGameDebugger.Profiler.FastBarGraph GPUSectionBarGraph;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Button RestartButton;
      private System.Windows.Forms.Button ExportToCSVButton;
      private System.Windows.Forms.CheckBox DiffCheckBox;
      private System.Windows.Forms.TextBox SnapCommentsTextBox;
      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.CheckBox SortBarResultsCheckBox;
      private System.Windows.Forms.Button ResetStatsButton;
      private System.Windows.Forms.CheckBox LiveUpdateCheckBox;
      private System.Windows.Forms.MainMenu mainMenu1;
      private System.Windows.Forms.MenuItem PauseMenuItem;
      private System.Windows.Forms.Button DiagnosticsButton;
      private System.Windows.Forms.ComboBox AverageTypeComboBox;
      private System.Windows.Forms.Button TestButton;
      private System.Windows.Forms.Button SaveAsButton;
      private System.Windows.Forms.Button LoadButton;
      private System.Windows.Forms.HScrollBar Timeline1hScrollBar;
      private System.Windows.Forms.ComboBox TopFrameViewTypeCombo;
      private System.Windows.Forms.ComboBox TopFrameDrawTypeCombo;
      private System.Windows.Forms.ComboBox TimelineGroupCombo;
      private System.Windows.Forms.Panel panel5;
      private System.Windows.Forms.Panel panel7;
      private System.Windows.Forms.Splitter splitter4;
      private System.Windows.Forms.Panel panel9;
      private System.Windows.Forms.Button ClearAllButton;
      private SplitContainer splitContainer1;
      private SplitContainer splitContainer2;
      private CheckBox AutoFilterCheckBox;
      private Label FilterThreasholdLabel;
      private TrackBar FilterThreasholdTrackBar;
      private VScrollBar SectionDataScrollbar;
      private Button NavNextFrameButton;
      private Button NavLastFrameButton;
      private TrackBar GraphZoomYBar;
      private TrackBar FrameViewYZoomBar;
      private CheckBox SectionsZoomCheckBox;
      private CheckBox SectionsShowGPUCheckBox;
      private Button FilterButton;
      private System.ComponentModel.IContainer components = null;

      public TimelineVis()
      {
         InitializeComponent();
 
         this.MouseWheel+=new MouseEventHandler(TimelineVis_MouseWheel);

         CustomExceptionHandler eh = new CustomExceptionHandler();
         Application.ThreadException += new ThreadExceptionEventHandler(eh.OnThreadException);   
      
         InitOnce();

      }

      //The Error Handler class      
      internal class CustomExceptionHandler 
      {
         //Handle the exception event
         public void OnThreadException(object sender, ThreadExceptionEventArgs t) 
         {
            DialogResult result = DialogResult.Cancel;
            try 
            {
               result = this.ShowThreadExceptionDialog(t.Exception);
            }
            catch 
            {
               try 
               {
                  MessageBox.Show("Fatal Error", "Fatal Error", MessageBoxButtons.AbortRetryIgnore, MessageBoxIcon.Stop);
               }
               finally 
               {
                  Application.Exit();
               }
            }

            if (result == DialogResult.Abort)
               Application.Exit();
         }
         //The simple dialog that is displayed when this class catches and exception
         private DialogResult ShowThreadExceptionDialog(Exception e) 
         {
            string errorMsg = "An error occurred please contact the adminstrator with" +
               " the following information:\n\n";
            errorMsg += e.Message + "\n\nStack Trace:\n" + e.StackTrace;
            return MessageBox.Show(errorMsg, "Application Error",  MessageBoxButtons.AbortRetryIgnore, MessageBoxIcon.Stop);
         }
      }




      protected override void Dispose( bool disposing )
      {
         DestroyGraphics();
         ShutdownInternalClasses();
         //System.Threading.Thread.Sleep(100);

         if( disposing )
         {
            if (components != null) 
            {
               components.Dispose();
            }
         }
         base.Dispose( disposing );
      }
      #endregion

      #region Designer generated code
      /// <summary>
      /// Required method for Designer support - do not modify
      /// the contents of this method with the code editor.
      /// </summary>
      private void InitializeComponent()
      {
         this.components = new System.ComponentModel.Container();
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TimelineVis));
         this.StartPerfSendButton = new System.Windows.Forms.Button();
         this.StopPerfSendButton = new System.Windows.Forms.Button();
         this.AALinesCheckBox = new System.Windows.Forms.CheckBox();
         this.FrameHorizontalScrollBar = new System.Windows.Forms.HScrollBar();
         this.CompressionTestButton = new System.Windows.Forms.Button();
         this.TestResultsTextBox = new System.Windows.Forms.TextBox();
         this.Timeline1hScrollBar = new System.Windows.Forms.HScrollBar();
         this.panel1 = new System.Windows.Forms.Panel();
         this.GraphZoomYBar = new System.Windows.Forms.TrackBar();
         this.NavNextFrameButton = new System.Windows.Forms.Button();
         this.NavLastFrameButton = new System.Windows.Forms.Button();
         this.TimelineGroupCombo = new System.Windows.Forms.ComboBox();
         this.TestButton = new System.Windows.Forms.Button();
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.RestartButton = new System.Windows.Forms.Button();
         this.fastGraph1 = new EnsembleStudios.RemoteGameDebugger.Profiler.FastGraph();
         this.LoadButton = new System.Windows.Forms.Button();
         this.SaveAsButton = new System.Windows.Forms.Button();
         this.DiagnosticsButton = new System.Windows.Forms.Button();
         this.FilterSectionsButton = new System.Windows.Forms.Button();
         this.ExportToCSVButton = new System.Windows.Forms.Button();
         this.SaveButton = new System.Windows.Forms.Button();
         this.splitContainer1 = new System.Windows.Forms.SplitContainer();
         this.splitContainer2 = new System.Windows.Forms.SplitContainer();
         this.panel6 = new System.Windows.Forms.Panel();
         this.FilterThreasholdLabel = new System.Windows.Forms.Label();
         this.AutoFilterCheckBox = new System.Windows.Forms.CheckBox();
         this.ClearAllButton = new System.Windows.Forms.Button();
         this.TopFrameDrawTypeCombo = new System.Windows.Forms.ComboBox();
         this.TopFrameViewTypeCombo = new System.Windows.Forms.ComboBox();
         this.LiveUpdateCheckBox = new System.Windows.Forms.CheckBox();
         this.profilerFrameView1 = new EnsembleStudios.RemoteGameDebugger.ProfilerFrameView();
         this.FilterThreasholdTrackBar = new System.Windows.Forms.TrackBar();
         this.FrameViewYZoomBar = new System.Windows.Forms.TrackBar();
         this.panel3 = new System.Windows.Forms.Panel();
         this.FilterButton = new System.Windows.Forms.Button();
         this.SectionsShowGPUCheckBox = new System.Windows.Forms.CheckBox();
         this.SectionsZoomCheckBox = new System.Windows.Forms.CheckBox();
         this.SectionDataScrollbar = new System.Windows.Forms.VScrollBar();
         this.panel5 = new System.Windows.Forms.Panel();
         this.panel9 = new System.Windows.Forms.Panel();
         this.GPUSectionBarGraph = new EnsembleStudios.RemoteGameDebugger.Profiler.FastBarGraph();
         this.label2 = new System.Windows.Forms.Label();
         this.splitter4 = new System.Windows.Forms.Splitter();
         this.panel7 = new System.Windows.Forms.Panel();
         this.SectionBarGraph = new EnsembleStudios.RemoteGameDebugger.Profiler.FastBarGraph();
         this.label1 = new System.Windows.Forms.Label();
         this.AverageTypeComboBox = new System.Windows.Forms.ComboBox();
         this.SortBarResultsCheckBox = new System.Windows.Forms.CheckBox();
         this.ResetStatsButton = new System.Windows.Forms.Button();
         this.SnapCommentsTextBox = new System.Windows.Forms.TextBox();
         this.DiffCheckBox = new System.Windows.Forms.CheckBox();
         this.mainMenu1 = new System.Windows.Forms.MainMenu(this.components);
         this.PauseMenuItem = new System.Windows.Forms.MenuItem();
         this.fastBarGraph1 = new EnsembleStudios.RemoteGameDebugger.Profiler.FastBarGraph();
         this.panel1.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.GraphZoomYBar)).BeginInit();
         this.splitContainer1.Panel1.SuspendLayout();
         this.splitContainer1.Panel2.SuspendLayout();
         this.splitContainer1.SuspendLayout();
         this.splitContainer2.Panel1.SuspendLayout();
         this.splitContainer2.Panel2.SuspendLayout();
         this.splitContainer2.SuspendLayout();
         this.panel6.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.FilterThreasholdTrackBar)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.FrameViewYZoomBar)).BeginInit();
         this.panel3.SuspendLayout();
         this.panel5.SuspendLayout();
         this.panel9.SuspendLayout();
         this.panel7.SuspendLayout();
         this.SuspendLayout();
         // 
         // StartPerfSendButton
         // 
         this.StartPerfSendButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
         this.StartPerfSendButton.Image = ((System.Drawing.Image)(resources.GetObject("StartPerfSendButton.Image")));
         this.StartPerfSendButton.Location = new System.Drawing.Point(8, 8);
         this.StartPerfSendButton.Name = "StartPerfSendButton";
         this.StartPerfSendButton.Size = new System.Drawing.Size(24, 24);
         this.StartPerfSendButton.TabIndex = 30;
         this.StartPerfSendButton.Click += new System.EventHandler(this.StartPerfSendButton_Click_1);
         // 
         // StopPerfSendButton
         // 
         this.StopPerfSendButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
         this.StopPerfSendButton.Image = ((System.Drawing.Image)(resources.GetObject("StopPerfSendButton.Image")));
         this.StopPerfSendButton.Location = new System.Drawing.Point(40, 8);
         this.StopPerfSendButton.Name = "StopPerfSendButton";
         this.StopPerfSendButton.Size = new System.Drawing.Size(24, 24);
         this.StopPerfSendButton.TabIndex = 0;
         this.StopPerfSendButton.Click += new System.EventHandler(this.StopPerfSendButton_Click);
         // 
         // AALinesCheckBox
         // 
         this.AALinesCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.AALinesCheckBox.Checked = true;
         this.AALinesCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.AALinesCheckBox.Location = new System.Drawing.Point(612, 225);
         this.AALinesCheckBox.Name = "AALinesCheckBox";
         this.AALinesCheckBox.Size = new System.Drawing.Size(104, 24);
         this.AALinesCheckBox.TabIndex = 9;
         this.AALinesCheckBox.Text = "AALines";
         this.AALinesCheckBox.Visible = false;
         this.AALinesCheckBox.CheckedChanged += new System.EventHandler(this.AALinesCheckBox_CheckedChanged);
         // 
         // FrameHorizontalScrollBar
         // 
         this.FrameHorizontalScrollBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.FrameHorizontalScrollBar.LargeChange = 1000;
         this.FrameHorizontalScrollBar.Location = new System.Drawing.Point(8, 390);
         this.FrameHorizontalScrollBar.Maximum = 10000;
         this.FrameHorizontalScrollBar.Name = "FrameHorizontalScrollBar";
         this.FrameHorizontalScrollBar.Size = new System.Drawing.Size(753, 15);
         this.FrameHorizontalScrollBar.TabIndex = 12;
         this.FrameHorizontalScrollBar.Value = 5000;
         this.FrameHorizontalScrollBar.Scroll += new System.Windows.Forms.ScrollEventHandler(this.FrameHorizontalScrollBar_Scroll);
         // 
         // CompressionTestButton
         // 
         this.CompressionTestButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.CompressionTestButton.Location = new System.Drawing.Point(916, 225);
         this.CompressionTestButton.Name = "CompressionTestButton";
         this.CompressionTestButton.Size = new System.Drawing.Size(72, 23);
         this.CompressionTestButton.TabIndex = 13;
         this.CompressionTestButton.Text = "Test Compression";
         this.CompressionTestButton.Visible = false;
         this.CompressionTestButton.Click += new System.EventHandler(this.CompressionTestButton_Click);
         // 
         // TestResultsTextBox
         // 
         this.TestResultsTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.TestResultsTextBox.Location = new System.Drawing.Point(996, 225);
         this.TestResultsTextBox.Name = "TestResultsTextBox";
         this.TestResultsTextBox.Size = new System.Drawing.Size(64, 20);
         this.TestResultsTextBox.TabIndex = 14;
         this.TestResultsTextBox.Visible = false;
         // 
         // Timeline1hScrollBar
         // 
         this.Timeline1hScrollBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.Timeline1hScrollBar.LargeChange = 1000;
         this.Timeline1hScrollBar.Location = new System.Drawing.Point(8, 201);
         this.Timeline1hScrollBar.Maximum = 10000;
         this.Timeline1hScrollBar.Name = "Timeline1hScrollBar";
         this.Timeline1hScrollBar.Size = new System.Drawing.Size(900, 17);
         this.Timeline1hScrollBar.TabIndex = 15;
         this.Timeline1hScrollBar.Scroll += new System.Windows.Forms.ScrollEventHandler(this.Timeline1hScrollBar_Scroll);
         // 
         // panel1
         // 
         this.panel1.Controls.Add(this.GraphZoomYBar);
         this.panel1.Controls.Add(this.NavNextFrameButton);
         this.panel1.Controls.Add(this.NavLastFrameButton);
         this.panel1.Controls.Add(this.TimelineGroupCombo);
         this.panel1.Controls.Add(this.TestButton);
         this.panel1.Controls.Add(this.textBox1);
         this.panel1.Controls.Add(this.RestartButton);
         this.panel1.Controls.Add(this.fastGraph1);
         this.panel1.Controls.Add(this.Timeline1hScrollBar);
         this.panel1.Controls.Add(this.TestResultsTextBox);
         this.panel1.Controls.Add(this.CompressionTestButton);
         this.panel1.Controls.Add(this.AALinesCheckBox);
         this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.panel1.Location = new System.Drawing.Point(0, 0);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(1092, 249);
         this.panel1.TabIndex = 18;
         // 
         // GraphZoomYBar
         // 
         this.GraphZoomYBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.GraphZoomYBar.Location = new System.Drawing.Point(1056, 8);
         this.GraphZoomYBar.Maximum = 100;
         this.GraphZoomYBar.Name = "GraphZoomYBar";
         this.GraphZoomYBar.Orientation = System.Windows.Forms.Orientation.Vertical;
         this.GraphZoomYBar.Size = new System.Drawing.Size(45, 100);
         this.GraphZoomYBar.TabIndex = 37;
         this.GraphZoomYBar.TickFrequency = 10;
         this.GraphZoomYBar.Value = 50;
         this.GraphZoomYBar.Scroll += new System.EventHandler(this.trackBar1_Scroll);
         // 
         // NavNextFrameButton
         // 
         this.NavNextFrameButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.NavNextFrameButton.Location = new System.Drawing.Point(942, 198);
         this.NavNextFrameButton.Name = "NavNextFrameButton";
         this.NavNextFrameButton.Size = new System.Drawing.Size(27, 21);
         this.NavNextFrameButton.TabIndex = 36;
         this.NavNextFrameButton.Text = "+F";
         this.NavNextFrameButton.UseVisualStyleBackColor = true;
         this.NavNextFrameButton.Click += new System.EventHandler(this.NavNextFrameButton_Click);
         // 
         // NavLastFrameButton
         // 
         this.NavLastFrameButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.NavLastFrameButton.Location = new System.Drawing.Point(911, 198);
         this.NavLastFrameButton.Name = "NavLastFrameButton";
         this.NavLastFrameButton.Size = new System.Drawing.Size(27, 21);
         this.NavLastFrameButton.TabIndex = 35;
         this.NavLastFrameButton.Text = "-F";
         this.NavLastFrameButton.UseVisualStyleBackColor = true;
         this.NavLastFrameButton.Click += new System.EventHandler(this.NavLastFrameButton_Click);
         // 
         // TimelineGroupCombo
         // 
         this.TimelineGroupCombo.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.TimelineGroupCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.TimelineGroupCombo.Location = new System.Drawing.Point(908, 169);
         this.TimelineGroupCombo.Name = "TimelineGroupCombo";
         this.TimelineGroupCombo.Size = new System.Drawing.Size(144, 21);
         this.TimelineGroupCombo.TabIndex = 34;
         this.TimelineGroupCombo.SelectedIndexChanged += new System.EventHandler(this.TimelineGroupCombo_SelectedIndexChanged);
         // 
         // TestButton
         // 
         this.TestButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.TestButton.Location = new System.Drawing.Point(804, 225);
         this.TestButton.Name = "TestButton";
         this.TestButton.Size = new System.Drawing.Size(48, 23);
         this.TestButton.TabIndex = 31;
         this.TestButton.Text = "TestButton";
         this.TestButton.Visible = false;
         this.TestButton.Click += new System.EventHandler(this.TestButton_Click);
         // 
         // textBox1
         // 
         this.textBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.textBox1.BackColor = System.Drawing.SystemColors.Control;
         this.textBox1.Location = new System.Drawing.Point(8, 225);
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(524, 20);
         this.textBox1.TabIndex = 22;
         this.textBox1.Text = "Status: Not Started";
         // 
         // RestartButton
         // 
         this.RestartButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.RestartButton.Location = new System.Drawing.Point(852, 225);
         this.RestartButton.Name = "RestartButton";
         this.RestartButton.Size = new System.Drawing.Size(48, 23);
         this.RestartButton.TabIndex = 21;
         this.RestartButton.Text = "Restart";
         this.RestartButton.Visible = false;
         this.RestartButton.Click += new System.EventHandler(this.RestartButton_Click);
         // 
         // fastGraph1
         // 
         this.fastGraph1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.fastGraph1.BackColor = System.Drawing.SystemColors.AppWorkspace;
         this.fastGraph1.Location = new System.Drawing.Point(8, 8);
         this.fastGraph1.Name = "fastGraph1";
         this.fastGraph1.Size = new System.Drawing.Size(1044, 185);
         this.fastGraph1.TabIndex = 3;
         this.fastGraph1.NewestFrameSelected += new EnsembleStudios.RemoteGameDebugger.Profiler.FastGraph.FrameHandler(this.fastGraph1_NewestFrameSelected_1);
         this.fastGraph1.OldFrameSelected += new EnsembleStudios.RemoteGameDebugger.Profiler.FastGraph.FrameHandler(this.fastGraph1_OldFrameSelected_1);
         // 
         // LoadButton
         // 
         this.LoadButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
         this.LoadButton.Image = ((System.Drawing.Image)(resources.GetObject("LoadButton.Image")));
         this.LoadButton.Location = new System.Drawing.Point(72, 8);
         this.LoadButton.Name = "LoadButton";
         this.LoadButton.Size = new System.Drawing.Size(24, 24);
         this.LoadButton.TabIndex = 33;
         this.LoadButton.Click += new System.EventHandler(this.LoadButton_Click);
         // 
         // SaveAsButton
         // 
         this.SaveAsButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
         this.SaveAsButton.Image = ((System.Drawing.Image)(resources.GetObject("SaveAsButton.Image")));
         this.SaveAsButton.Location = new System.Drawing.Point(104, 8);
         this.SaveAsButton.Name = "SaveAsButton";
         this.SaveAsButton.Size = new System.Drawing.Size(24, 23);
         this.SaveAsButton.TabIndex = 32;
         this.SaveAsButton.Click += new System.EventHandler(this.SaveAsButton_Click);
         // 
         // DiagnosticsButton
         // 
         this.DiagnosticsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.DiagnosticsButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
         this.DiagnosticsButton.Location = new System.Drawing.Point(681, 8);
         this.DiagnosticsButton.Name = "DiagnosticsButton";
         this.DiagnosticsButton.Size = new System.Drawing.Size(75, 23);
         this.DiagnosticsButton.TabIndex = 23;
         this.DiagnosticsButton.Text = "Debug Info";
         this.DiagnosticsButton.Click += new System.EventHandler(this.DiagnosticsButton_Click);
         // 
         // FilterSectionsButton
         // 
         this.FilterSectionsButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
         this.FilterSectionsButton.Location = new System.Drawing.Point(216, 8);
         this.FilterSectionsButton.Name = "FilterSectionsButton";
         this.FilterSectionsButton.Size = new System.Drawing.Size(72, 23);
         this.FilterSectionsButton.TabIndex = 20;
         this.FilterSectionsButton.Text = "Filter";
         this.FilterSectionsButton.Click += new System.EventHandler(this.FilterSectionsButton_Click);
         // 
         // ExportToCSVButton
         // 
         this.ExportToCSVButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.ExportToCSVButton.Location = new System.Drawing.Point(16, 375);
         this.ExportToCSVButton.Name = "ExportToCSVButton";
         this.ExportToCSVButton.Size = new System.Drawing.Size(96, 23);
         this.ExportToCSVButton.TabIndex = 20;
         this.ExportToCSVButton.Text = "Excel Snapshot";
         this.ExportToCSVButton.Click += new System.EventHandler(this.ExportToCSVButton_Click_1);
         // 
         // SaveButton
         // 
         this.SaveButton.Location = new System.Drawing.Point(0, 0);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(75, 23);
         this.SaveButton.TabIndex = 0;
         // 
         // splitContainer1
         // 
         this.splitContainer1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
         this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.splitContainer1.Location = new System.Drawing.Point(0, 0);
         this.splitContainer1.Name = "splitContainer1";
         this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
         // 
         // splitContainer1.Panel1
         // 
         this.splitContainer1.Panel1.Controls.Add(this.splitContainer2);
         // 
         // splitContainer1.Panel2
         // 
         this.splitContainer1.Panel2.Controls.Add(this.panel1);
         this.splitContainer1.Size = new System.Drawing.Size(1096, 670);
         this.splitContainer1.SplitterDistance = 413;
         this.splitContainer1.TabIndex = 21;
         // 
         // splitContainer2
         // 
         this.splitContainer2.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
         this.splitContainer2.Dock = System.Windows.Forms.DockStyle.Fill;
         this.splitContainer2.Location = new System.Drawing.Point(0, 0);
         this.splitContainer2.Name = "splitContainer2";
         // 
         // splitContainer2.Panel1
         // 
         this.splitContainer2.Panel1.Controls.Add(this.panel6);
         // 
         // splitContainer2.Panel2
         // 
         this.splitContainer2.Panel2.Controls.Add(this.panel3);
         this.splitContainer2.Size = new System.Drawing.Size(1096, 413);
         this.splitContainer2.SplitterDistance = 794;
         this.splitContainer2.TabIndex = 0;
         // 
         // panel6
         // 
         this.panel6.Controls.Add(this.FilterThreasholdLabel);
         this.panel6.Controls.Add(this.AutoFilterCheckBox);
         this.panel6.Controls.Add(this.ClearAllButton);
         this.panel6.Controls.Add(this.TopFrameDrawTypeCombo);
         this.panel6.Controls.Add(this.TopFrameViewTypeCombo);
         this.panel6.Controls.Add(this.LiveUpdateCheckBox);
         this.panel6.Controls.Add(this.FrameHorizontalScrollBar);
         this.panel6.Controls.Add(this.StartPerfSendButton);
         this.panel6.Controls.Add(this.StopPerfSendButton);
         this.panel6.Controls.Add(this.LoadButton);
         this.panel6.Controls.Add(this.SaveAsButton);
         this.panel6.Controls.Add(this.FilterSectionsButton);
         this.panel6.Controls.Add(this.DiagnosticsButton);
         this.panel6.Controls.Add(this.profilerFrameView1);
         this.panel6.Controls.Add(this.FilterThreasholdTrackBar);
         this.panel6.Controls.Add(this.FrameViewYZoomBar);
         this.panel6.Dock = System.Windows.Forms.DockStyle.Fill;
         this.panel6.Location = new System.Drawing.Point(0, 0);
         this.panel6.Name = "panel6";
         this.panel6.Size = new System.Drawing.Size(790, 409);
         this.panel6.TabIndex = 0;
         // 
         // FilterThreasholdLabel
         // 
         this.FilterThreasholdLabel.AutoSize = true;
         this.FilterThreasholdLabel.Enabled = false;
         this.FilterThreasholdLabel.Location = new System.Drawing.Point(589, 8);
         this.FilterThreasholdLabel.Name = "FilterThreasholdLabel";
         this.FilterThreasholdLabel.Size = new System.Drawing.Size(99, 13);
         this.FilterThreasholdLabel.TabIndex = 37;
         this.FilterThreasholdLabel.Text = "60 samples/section";
         // 
         // AutoFilterCheckBox
         // 
         this.AutoFilterCheckBox.AutoSize = true;
         this.AutoFilterCheckBox.Location = new System.Drawing.Point(421, 8);
         this.AutoFilterCheckBox.Name = "AutoFilterCheckBox";
         this.AutoFilterCheckBox.Size = new System.Drawing.Size(73, 17);
         this.AutoFilterCheckBox.TabIndex = 35;
         this.AutoFilterCheckBox.Text = "Auto Filter";
         this.AutoFilterCheckBox.UseVisualStyleBackColor = true;
         this.AutoFilterCheckBox.CheckedChanged += new System.EventHandler(this.AutoFilterCheckBox_CheckedChanged);
         // 
         // ClearAllButton
         // 
         this.ClearAllButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
         this.ClearAllButton.Location = new System.Drawing.Point(136, 8);
         this.ClearAllButton.Name = "ClearAllButton";
         this.ClearAllButton.Size = new System.Drawing.Size(75, 23);
         this.ClearAllButton.TabIndex = 34;
         this.ClearAllButton.Text = "Clear";
         this.ClearAllButton.Click += new System.EventHandler(this.ClearAllButton_Click);
         // 
         // TopFrameDrawTypeCombo
         // 
         this.TopFrameDrawTypeCombo.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.TopFrameDrawTypeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.TopFrameDrawTypeCombo.Location = new System.Drawing.Point(665, 362);
         this.TopFrameDrawTypeCombo.Name = "TopFrameDrawTypeCombo";
         this.TopFrameDrawTypeCombo.Size = new System.Drawing.Size(88, 21);
         this.TopFrameDrawTypeCombo.TabIndex = 14;
         this.TopFrameDrawTypeCombo.SelectedIndexChanged += new System.EventHandler(this.TopFrameDrawTypeCombo_SelectedIndexChanged);
         // 
         // TopFrameViewTypeCombo
         // 
         this.TopFrameViewTypeCombo.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.TopFrameViewTypeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.TopFrameViewTypeCombo.Location = new System.Drawing.Point(553, 362);
         this.TopFrameViewTypeCombo.Name = "TopFrameViewTypeCombo";
         this.TopFrameViewTypeCombo.Size = new System.Drawing.Size(104, 21);
         this.TopFrameViewTypeCombo.TabIndex = 13;
         this.TopFrameViewTypeCombo.SelectedIndexChanged += new System.EventHandler(this.TopFrameViewTypeCombo_SelectedIndexChanged);
         // 
         // LiveUpdateCheckBox
         // 
         this.LiveUpdateCheckBox.BackColor = System.Drawing.Color.Transparent;
         this.LiveUpdateCheckBox.Checked = true;
         this.LiveUpdateCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.LiveUpdateCheckBox.Location = new System.Drawing.Point(296, 8);
         this.LiveUpdateCheckBox.Name = "LiveUpdateCheckBox";
         this.LiveUpdateCheckBox.Size = new System.Drawing.Size(128, 16);
         this.LiveUpdateCheckBox.TabIndex = 0;
         this.LiveUpdateCheckBox.Text = "Show Current Frame";
         this.LiveUpdateCheckBox.UseVisualStyleBackColor = false;
         this.LiveUpdateCheckBox.CheckedChanged += new System.EventHandler(this.LiveUpdateCheckBox_CheckedChanged);
         // 
         // profilerFrameView1
         // 
         this.profilerFrameView1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.profilerFrameView1.BackColor = System.Drawing.SystemColors.AppWorkspace;
         this.profilerFrameView1.DrawTypeLayout = "Full";
         this.profilerFrameView1.Location = new System.Drawing.Point(8, 40);
         this.profilerFrameView1.Name = "profilerFrameView1";
         this.profilerFrameView1.Size = new System.Drawing.Size(753, 346);
         this.profilerFrameView1.TabIndex = 2;
         this.profilerFrameView1.ViewTypeLayout = "PerCPUView";
         // 
         // FilterThreasholdTrackBar
         // 
         this.FilterThreasholdTrackBar.Enabled = false;
         this.FilterThreasholdTrackBar.Location = new System.Drawing.Point(488, 1);
         this.FilterThreasholdTrackBar.Maximum = 300;
         this.FilterThreasholdTrackBar.Minimum = 10;
         this.FilterThreasholdTrackBar.Name = "FilterThreasholdTrackBar";
         this.FilterThreasholdTrackBar.Size = new System.Drawing.Size(104, 45);
         this.FilterThreasholdTrackBar.SmallChange = 10;
         this.FilterThreasholdTrackBar.TabIndex = 36;
         this.FilterThreasholdTrackBar.TickFrequency = 20;
         this.FilterThreasholdTrackBar.Value = 60;
         this.FilterThreasholdTrackBar.Scroll += new System.EventHandler(this.FilterThreasholdTrackBar_Scroll);
         // 
         // FrameViewYZoomBar
         // 
         this.FrameViewYZoomBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.FrameViewYZoomBar.Location = new System.Drawing.Point(759, 42);
         this.FrameViewYZoomBar.Maximum = 100;
         this.FrameViewYZoomBar.Name = "FrameViewYZoomBar";
         this.FrameViewYZoomBar.Orientation = System.Windows.Forms.Orientation.Vertical;
         this.FrameViewYZoomBar.Size = new System.Drawing.Size(45, 148);
         this.FrameViewYZoomBar.TabIndex = 38;
         this.FrameViewYZoomBar.TickFrequency = 10;
         this.FrameViewYZoomBar.Value = 50;
         this.FrameViewYZoomBar.Scroll += new System.EventHandler(this.FrameViewYZoomBar_Scroll);
         // 
         // panel3
         // 
         this.panel3.Controls.Add(this.FilterButton);
         this.panel3.Controls.Add(this.SectionsShowGPUCheckBox);
         this.panel3.Controls.Add(this.SectionsZoomCheckBox);
         this.panel3.Controls.Add(this.SectionDataScrollbar);
         this.panel3.Controls.Add(this.panel5);
         this.panel3.Controls.Add(this.AverageTypeComboBox);
         this.panel3.Controls.Add(this.SortBarResultsCheckBox);
         this.panel3.Controls.Add(this.ResetStatsButton);
         this.panel3.Controls.Add(this.SnapCommentsTextBox);
         this.panel3.Controls.Add(this.ExportToCSVButton);
         this.panel3.Controls.Add(this.DiffCheckBox);
         this.panel3.Dock = System.Windows.Forms.DockStyle.Fill;
         this.panel3.Location = new System.Drawing.Point(0, 0);
         this.panel3.Name = "panel3";
         this.panel3.Size = new System.Drawing.Size(294, 409);
         this.panel3.TabIndex = 17;
         // 
         // FilterButton
         // 
         this.FilterButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.FilterButton.Location = new System.Drawing.Point(227, 312);
         this.FilterButton.Name = "FilterButton";
         this.FilterButton.Size = new System.Drawing.Size(59, 23);
         this.FilterButton.TabIndex = 31;
         this.FilterButton.Text = "Filter ...";
         this.FilterButton.UseVisualStyleBackColor = true;
         this.FilterButton.Click += new System.EventHandler(this.FilterButton_Click);
         // 
         // SectionsShowGPUCheckBox
         // 
         this.SectionsShowGPUCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.SectionsShowGPUCheckBox.AutoSize = true;
         this.SectionsShowGPUCheckBox.Checked = true;
         this.SectionsShowGPUCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.SectionsShowGPUCheckBox.Location = new System.Drawing.Point(136, 312);
         this.SectionsShowGPUCheckBox.Name = "SectionsShowGPUCheckBox";
         this.SectionsShowGPUCheckBox.Size = new System.Drawing.Size(79, 17);
         this.SectionsShowGPUCheckBox.TabIndex = 30;
         this.SectionsShowGPUCheckBox.Text = "Show GPU";
         this.SectionsShowGPUCheckBox.UseVisualStyleBackColor = true;
         this.SectionsShowGPUCheckBox.CheckedChanged += new System.EventHandler(this.SectionsShowGPUCheckBox_CheckedChanged);
         // 
         // SectionsZoomCheckBox
         // 
         this.SectionsZoomCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.SectionsZoomCheckBox.AutoSize = true;
         this.SectionsZoomCheckBox.Location = new System.Drawing.Point(16, 312);
         this.SectionsZoomCheckBox.Name = "SectionsZoomCheckBox";
         this.SectionsZoomCheckBox.Size = new System.Drawing.Size(88, 17);
         this.SectionsZoomCheckBox.TabIndex = 29;
         this.SectionsZoomCheckBox.Text = "Show Values";
         this.SectionsZoomCheckBox.UseVisualStyleBackColor = true;
         this.SectionsZoomCheckBox.CheckedChanged += new System.EventHandler(this.SectionsZoomCheckBox_CheckedChanged);
         // 
         // SectionDataScrollbar
         // 
         this.SectionDataScrollbar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.SectionDataScrollbar.Location = new System.Drawing.Point(275, 8);
         this.SectionDataScrollbar.Name = "SectionDataScrollbar";
         this.SectionDataScrollbar.Size = new System.Drawing.Size(17, 287);
         this.SectionDataScrollbar.TabIndex = 28;
         this.SectionDataScrollbar.Value = 100;
         this.SectionDataScrollbar.Scroll += new System.Windows.Forms.ScrollEventHandler(this.SectionDataScrollbar_Scroll);
         // 
         // panel5
         // 
         this.panel5.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panel5.Controls.Add(this.panel9);
         this.panel5.Controls.Add(this.splitter4);
         this.panel5.Controls.Add(this.panel7);
         this.panel5.Location = new System.Drawing.Point(8, 8);
         this.panel5.Name = "panel5";
         this.panel5.Size = new System.Drawing.Size(264, 287);
         this.panel5.TabIndex = 27;
         // 
         // panel9
         // 
         this.panel9.Controls.Add(this.GPUSectionBarGraph);
         this.panel9.Controls.Add(this.label2);
         this.panel9.Dock = System.Windows.Forms.DockStyle.Fill;
         this.panel9.Location = new System.Drawing.Point(186, 0);
         this.panel9.Name = "panel9";
         this.panel9.Size = new System.Drawing.Size(78, 287);
         this.panel9.TabIndex = 2;
         // 
         // GPUSectionBarGraph
         // 
         this.GPUSectionBarGraph.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.GPUSectionBarGraph.BackColor = System.Drawing.SystemColors.AppWorkspace;
         this.GPUSectionBarGraph.Location = new System.Drawing.Point(8, 24);
         this.GPUSectionBarGraph.Name = "GPUSectionBarGraph";
         this.GPUSectionBarGraph.Size = new System.Drawing.Size(68, 255);
         this.GPUSectionBarGraph.TabIndex = 17;
         // 
         // label2
         // 
         this.label2.Location = new System.Drawing.Point(8, 8);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(56, 16);
         this.label2.TabIndex = 19;
         this.label2.Text = "GPU";
         // 
         // splitter4
         // 
         this.splitter4.BackColor = System.Drawing.SystemColors.ControlDarkDark;
         this.splitter4.Location = new System.Drawing.Point(182, 0);
         this.splitter4.Name = "splitter4";
         this.splitter4.Size = new System.Drawing.Size(4, 287);
         this.splitter4.TabIndex = 1;
         this.splitter4.TabStop = false;
         // 
         // panel7
         // 
         this.panel7.Controls.Add(this.SectionBarGraph);
         this.panel7.Controls.Add(this.label1);
         this.panel7.Dock = System.Windows.Forms.DockStyle.Left;
         this.panel7.Location = new System.Drawing.Point(0, 0);
         this.panel7.Name = "panel7";
         this.panel7.Size = new System.Drawing.Size(182, 287);
         this.panel7.TabIndex = 0;
         // 
         // SectionBarGraph
         // 
         this.SectionBarGraph.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.SectionBarGraph.BackColor = System.Drawing.SystemColors.AppWorkspace;
         this.SectionBarGraph.Location = new System.Drawing.Point(8, 24);
         this.SectionBarGraph.Name = "SectionBarGraph";
         this.SectionBarGraph.Size = new System.Drawing.Size(166, 255);
         this.SectionBarGraph.TabIndex = 16;
         // 
         // label1
         // 
         this.label1.Location = new System.Drawing.Point(8, 8);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(100, 16);
         this.label1.TabIndex = 18;
         this.label1.Text = "CPU";
         // 
         // AverageTypeComboBox
         // 
         this.AverageTypeComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.AverageTypeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.AverageTypeComboBox.Items.AddRange(new object[] {
            "5 Frame Average",
            "All Frames Average",
            "Current Frame"});
         this.AverageTypeComboBox.Location = new System.Drawing.Point(16, 335);
         this.AverageTypeComboBox.Name = "AverageTypeComboBox";
         this.AverageTypeComboBox.Size = new System.Drawing.Size(121, 21);
         this.AverageTypeComboBox.TabIndex = 26;
         this.AverageTypeComboBox.SelectedIndexChanged += new System.EventHandler(this.AverageTypeComboBox_SelectedIndexChanged);
         // 
         // SortBarResultsCheckBox
         // 
         this.SortBarResultsCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.SortBarResultsCheckBox.Location = new System.Drawing.Point(16, 292);
         this.SortBarResultsCheckBox.Name = "SortBarResultsCheckBox";
         this.SortBarResultsCheckBox.Size = new System.Drawing.Size(104, 24);
         this.SortBarResultsCheckBox.TabIndex = 25;
         this.SortBarResultsCheckBox.Text = "SortResults";
         this.SortBarResultsCheckBox.CheckedChanged += new System.EventHandler(this.SortBarResultsCheckBox_CheckedChanged);
         // 
         // ResetStatsButton
         // 
         this.ResetStatsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.ResetStatsButton.Location = new System.Drawing.Point(144, 335);
         this.ResetStatsButton.Name = "ResetStatsButton";
         this.ResetStatsButton.Size = new System.Drawing.Size(112, 23);
         this.ResetStatsButton.TabIndex = 24;
         this.ResetStatsButton.Text = "Reset Stats";
         this.ResetStatsButton.Click += new System.EventHandler(this.ResetStatsButton_Click);
         // 
         // SnapCommentsTextBox
         // 
         this.SnapCommentsTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.SnapCommentsTextBox.Location = new System.Drawing.Point(120, 375);
         this.SnapCommentsTextBox.Name = "SnapCommentsTextBox";
         this.SnapCommentsTextBox.Size = new System.Drawing.Size(166, 20);
         this.SnapCommentsTextBox.TabIndex = 23;
         this.SnapCommentsTextBox.Text = "comments about frame";
         // 
         // DiffCheckBox
         // 
         this.DiffCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.DiffCheckBox.Location = new System.Drawing.Point(136, 292);
         this.DiffCheckBox.Name = "DiffCheckBox";
         this.DiffCheckBox.Size = new System.Drawing.Size(136, 24);
         this.DiffCheckBox.TabIndex = 22;
         this.DiffCheckBox.Text = "Diff Top - Bottom";
         this.DiffCheckBox.CheckedChanged += new System.EventHandler(this.DiffCheckBox_CheckedChanged);
         // 
         // mainMenu1
         // 
         this.mainMenu1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
            this.PauseMenuItem});
         // 
         // PauseMenuItem
         // 
         this.PauseMenuItem.Index = 0;
         this.PauseMenuItem.Shortcut = System.Windows.Forms.Shortcut.CtrlP;
         this.PauseMenuItem.Text = "Pause";
         this.PauseMenuItem.Select += new System.EventHandler(this.PauseMenuItem_Select);
         // 
         // fastBarGraph1
         // 
         this.fastBarGraph1.BackColor = System.Drawing.SystemColors.AppWorkspace;
         this.fastBarGraph1.Location = new System.Drawing.Point(0, 0);
         this.fastBarGraph1.Name = "fastBarGraph1";
         this.fastBarGraph1.Size = new System.Drawing.Size(184, 168);
         this.fastBarGraph1.TabIndex = 0;
         // 
         // TimelineVis
         // 
         this.Controls.Add(this.splitContainer1);
         this.Name = "TimelineVis";
         this.Size = new System.Drawing.Size(1096, 670);
         this.panel1.ResumeLayout(false);
         this.panel1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.GraphZoomYBar)).EndInit();
         this.splitContainer1.Panel1.ResumeLayout(false);
         this.splitContainer1.Panel2.ResumeLayout(false);
         this.splitContainer1.ResumeLayout(false);
         this.splitContainer2.Panel1.ResumeLayout(false);
         this.splitContainer2.Panel2.ResumeLayout(false);
         this.splitContainer2.ResumeLayout(false);
         this.panel6.ResumeLayout(false);
         this.panel6.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.FilterThreasholdTrackBar)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.FrameViewYZoomBar)).EndInit();
         this.panel3.ResumeLayout(false);
         this.panel3.PerformLayout();
         this.panel5.ResumeLayout(false);
         this.panel9.ResumeLayout(false);
         this.panel7.ResumeLayout(false);
         this.ResumeLayout(false);

      }
      #endregion


    
      private ProfilerConnectionHandler mMessageHandler;
      private SampleManager mSampleManager;
      private bool mLiveUpdate = true;
      private bool mInitialized = false;
      bool mbHideAckPopupError = false;

      enum SectionViewType
      {
         cTopFrame,
         cBottomFrame,
         cTopDiffBottom
      }
      private Frame mTopSelectedFrame;
      private Frame mBottomSeletedFrame;
      private SectionViewType mSectionViewType = SectionViewType.cTopFrame;

      ProfileSectionSelection mSectionSelection = null;
      DiagnosticsForm mDiagnosticsForm = null;
      bool mbShowDiff = false;

      DataWorkspace mCurrentWorkspace = null;

      #region Internal classes initilization code
      public void InitializeIfNeeded()
      {
         if(mInitialized == false)
         {
            InitializeGraphics(this);
            InitInternalClasses();
            mInitialized = true;
         }
         InitilizeAlways();
      }

      bool mbFirstDraw = true;
      protected override void OnPaint(PaintEventArgs e)
      {
         if(mbFirstDraw)
         {     
            mbFirstDraw = false;
            OnFirstDraw();
         }
         base.OnPaint (e);
      }

      protected void OnFirstDraw()
      {
         InitializeIfNeeded();

         //I tried to fine a nice way to make the timeline device update at the right time, but only came up with this...
         panel1.Height = panel1.Height+1;
      }


      public void InitilizeAlways()
      {
         mSampleManager.ClearSections();
         profilerFrameView1.SetScrollBars(null, FrameHorizontalScrollBar); //FrameViewYZoomBar
         profilerFrameView1.SetZoomBars(null, FrameViewYZoomBar); //FrameViewYZoomBar
         //profilerFrameView1.SetScrollBars(FrameViewZoomScrollBar, FrameHorizontalScrollBar);

         //profilerFrameView2.SetScrollBars(Frame2ZoomScrollBar, Frame2HorizontalScrollBar);
         fastGraph1.SetScrollBars(Timeline1hScrollBar);
      }

      private void InitOnce()
      {
//         string[] vtypes = Enum.GetNames(typeof(ProfilerFrameView.ViewType));
//         ArrayList vtypesData = new ArrayList();
//         foreach(string s in vtypes)
//         {
//            vtypesData.Add(new LambdaProperty(s));
//        
//         }
//         TopFrameViewTypeCombo.DataSource = vtypesData;
//         TopFrameViewTypeCombo.SelectedIndex = 0;
//         TopFrameViewTypeCombo.ValueMember = "LambdaValue";
//         TopFrameViewTypeCombo.DataBindings.Add(new Binding("SelectedValue",profilerFrameView1,"ViewTypeLayout"));

         TopFrameViewTypeCombo.DataSource = Enum.GetNames(typeof(ProfilerFrameView.ViewType));
         TopFrameViewTypeCombo.SelectedIndex = 1;
         //TopFrameViewTypeCombo.SelectedItem
         //TopFrameViewTypeCombo.DataBindings.Add(new Binding("SelectedItem",profilerFrameView1,"ViewTypeLayout"));

         TopFrameDrawTypeCombo.DataSource = Enum.GetNames(typeof(ProfilerFrameView.DataRenderOptions));
         TopFrameDrawTypeCombo.SelectedIndex = 1;
         //TopFrameDrawTypeCombo.DataBindings.Add(new Binding("Text",profilerFrameView1,"DrawTypeLayout"));


         TimelineGroupCombo.DataSource = Enum.GetNames(typeof(TimelineDataManager.ViewGroups));
         TimelineGroupCombo.SelectedIndex = 0;

         AverageTypeComboBox.SelectedIndex = 2;

         //SectionsShowGPUCheckBox.Checked = false;

      }

      public void InitInternalClasses()
      {
         mSampleManager = new SampleManager();
         mCurrentWorkspace = new DataWorkspace(mSampleManager);
         mSampleManager.AddObserver(this);

         mMessageHandler = new ProfilerConnectionHandler();
         mMessageHandler.AddObserver(mSampleManager);

         mbHideAckPopupError = true;

         this.fastGraph1.mXOffset = 0;
      }

      public void ShutdownInternalClasses()
      {
         mSampleManager = null;
         mMessageHandler = null;
      }

      public void InitializeGraphics(Control containter)
      {
         foreach(System.Windows.Forms.Control c in containter.Controls)
         {
            if(c is DXManagedControl)
            {
               ((DXManagedControl)c).InitializeGraphics();
            }
            else if(c is Form)
            {
               InitializeGraphics(c);
            }
            else if(c is Panel)
            {
               InitializeGraphics(c);
            }
            else if (c is SplitContainer)
            {
               InitializeGraphics(c);
            }
         }   
      }

      public void DestroyGraphics()
      {
         foreach(System.Windows.Forms.Control c in this.Controls)
         {
            if(c is DXManagedControl)
            {
               ((DXManagedControl)c).DestroyGraphics();
            }
         }   
      }
      #endregion

      #region Core functionality

      //All data is recieved from the parent window through the IFrameObserver interface
      public void HandleMessage(byte type, BinaryReader reader)
      {
         try
         {
            if(mMessageHandler != null)
            {
               mMessageHandler.HandleMessage(type, reader);
            }
         }
         catch(System.Exception ex)
         {
            ErrorHandler.Error(ex.ToString());
         }
      }

      //The Internal classes send complete frame data back to this window through the IFrameObserver interface
      bool lastState = false;
      const int cCheckSectionsRate = 10;
      int mNextCheckup = cCheckSectionsRate;
      int mSectionFilterThreashold = 60;
      public void NewFrame(Frame f)
      {

         mCurrentWorkspace.mTimelineDataManager.PopulateGraphData(f); 
         mCurrentWorkspace.mTimelineDataManager.SetGraphData(fastGraph1,mSampleManager.mFrames);

         //Detect a hotkey the manual way since NOTHING else seems to work
         bool keyState = KeyHelper.CheckHotKey(KeyHelper.VKeyCodes.VK_PAUSE);
         if(lastState != keyState)
         {
            lastState = !lastState;
            if(keyState == true)
            {
               mLiveUpdate = !mLiveUpdate;
               this.LiveUpdateCheckBox.Checked = mLiveUpdate;
               fastGraph1.mbShowLatestData = mLiveUpdate;

            }
         }



         if(mLiveUpdate)
         {   
            mTopSelectedFrame = f;
            profilerFrameView1.SetFrame(f, ref mSampleManager.mSections, mSampleManager.mIdentiferCount);
            mCurrentWorkspace.mSectionStatsManager.SetSectionStats(SectionBarGraph, GPUSectionBarGraph, mTopSelectedFrame, mBottomSeletedFrame, mSectionViewType, true);
            
            mNextCheckup--;
            if (mNextCheckup == 0)
            {
               mNextCheckup = cCheckSectionsRate;
               List<ushort> overburdenedSections = mCurrentWorkspace.mSectionStatsManager.GetSectionsWithMoreThanNSamples(mSectionFilterThreashold);

               if (overburdenedSections.Count > 0 && Connected() && this.AutoFilterCheckBox.Checked == true)
               {
                  MemoryStream memStream = new MemoryStream();
                  ArrayList ids = new ArrayList();
                  ArrayList values = new ArrayList();

                  for (int j = 0; j < overburdenedSections.Count; j++)
                  {
                     if (overburdenedSections[j] < f.mSections.Length)
                     {
                        f.mSections[overburdenedSections[j]].mEnabled = false;
                     }
                     ids.Add((int)overburdenedSections[j]);
                     values.Add((byte)0);
                  }
                  TimelineControlPacket timeControl = new TimelineControlPacket(memStream, TimelineControlPacket.TimelineControlValues.cConfigureSections, ids, values);
                  mActiveClient.write(memStream);
               }

            }
            fastGraph1.SetTopViewLive();
         }
     
         
         //afo textBox1.Text = mSampleManager.FrameReport() + " " + mSampleManager.FrameRates();
         //ack every 5 frames.
         if((mSampleManager != null) && ((mSampleManager.mFrames.Count % 5) == 0))
         {
            SendAck((ulong)f.mFrameNumber);
         }
      }

      private void SendAck(ulong frameCount)
      {
         if(!Connected())
         {
            if(!mbHideAckPopupError)
               MessageBox.Show(this,"Error, no connection to game");
            return;
         }
         MemoryStream memStream = new MemoryStream();
         TimelineControlPacket timeControl = new TimelineControlPacket(memStream, TimelineControlPacket.TimelineControlValues.cFrameAck, frameCount);
         mActiveClient.write(memStream);
      }

      #endregion

      public void SetConnection(NetServiceConnection connection)
      {
         mActiveClient = connection;

         mActiveClient.mReadDelegates += new NetServiceConnection.ReadDelegate(readData);
      }
      public void readData(NetServiceConnection clientConnection, byte type, BinaryReader reader)
      {
         HandleMessage(type, reader);

      }



      NetServiceConnection mActiveClient= null;
      //public NetServiceConnection ActiveClient
      //{
      //   set
      //   {
      //      mActiveClient = value;
      //      mActiveClient.mReadDelegates += new NetServiceConnection.ReadDelegate(readData);
      //   }
      //}


      //public void readData(NetServiceConnection clientConnection, byte type, BinaryReader reader)
      //{
      //   HandleMessage(type, reader);

      //}


      #region Transition to multiple timelines... work in progress
      //To support multiple data sets I am doing some refactoring.
      
      class DataWorkspace
      {
         //load
         //save

         public DataWorkspace(SampleManager sampleManager)
         {
            mSampleManager = sampleManager;
            mTimelineDataManager = new TimelineDataManager();
            mSectionStatsManager = new SectionStatsManager(sampleManager);
         }


         SampleManager mSampleManager;
         public TimelineDataManager mTimelineDataManager;         
         public SectionStatsManager mSectionStatsManager;
       
      }
      
      //refactor this to work with other stats
      class SectionStatsManager
      {
         const int cMaxSections = 512;
         Bar[] mSectionStats = new Bar[cMaxSections];
         Bar[] mGPUSectionStats = new Bar[cMaxSections];
         Bar[] mSectionStats2 = new Bar[cMaxSections];
         Bar[] mGPUSectionStats2 = new Bar[cMaxSections];
         Bar[] mSectionStats3 = new Bar[cMaxSections];
         Bar[] mGPUSectionStats3 = new Bar[cMaxSections];
         double[] mTempCPU = new double[cMaxSections];
         double[] mTempGPU = new double[cMaxSections];

         ushort[] mCountBySectionID = new ushort[cMaxSections];
         int mNumMeasuredSections = 0;

         FastBarGraph mSectionBarGraph;
         FastBarGraph mGPUSectionBarGraph;


         SampleManager mSampleManager = null;
         public SectionStatsManager(SampleManager sampleManager)
         {
            mSampleManager = sampleManager;
            InitSectionStats(); 
         }

         void InitSectionStats()
         {
            for(int i=0; i < mSectionStats.Length; i++)
            {
               mSectionStats[i] = new Bar();
               mGPUSectionStats[i] = new Bar();
               mSectionStats2[i] = new Bar();
               mGPUSectionStats2[i] = new Bar();
               mSectionStats3[i] = new Bar();
               mGPUSectionStats3[i] = new Bar();
            }
         }

         public void ResetSectionStats()
         {
            if(mSectionStats == null)
               return;
            for(int i=0; i < mSectionStats.Length; i++)
            {
               if(mSectionStats[i] == null)
                  return;            
               mSectionStats[i].mTotal = 0;
               mSectionStats2[i].mTotal = 0;
               mSectionStats3[i].mTotal = 0;
               mSectionStats[i].mCount = 0;
               mSectionStats2[i].mCount = 0;
               mSectionStats3[i].mCount = 0;
               mSectionStats[i].mPeakValue = 0;
               mSectionStats2[i].mPeakValue = 0;
               mSectionStats3[i].mPeakValue = 0;
            
               mGPUSectionStats[i].mTotal =  0 ;
               mGPUSectionStats2[i].mTotal =  0 ;
               mGPUSectionStats3[i].mTotal =  0 ;
               mGPUSectionStats[i].mCount = 0 ;
               mGPUSectionStats2[i].mCount = 0 ;
               mGPUSectionStats3[i].mCount = 0 ;
               mGPUSectionStats[i].mPeakValue = 0 ;
               mGPUSectionStats2[i].mPeakValue =  0 ;
               mGPUSectionStats3[i].mPeakValue = 0 ;
            }
         }

         public List<ushort> GetSectionsWithMoreThanNSamples(int n)
         {
            List<ushort> results = new List<ushort>();
            for (ushort i = 0; i < mNumMeasuredSections; i++)
            {
               if (mCountBySectionID[i] > n)
                  results.Add(i);
            }
            return results;
         }

         public void SetSectionStats(FastBarGraph sectionBarGraph, FastBarGraph gpuSectionBarGraph ,Frame topFrame, Frame bottomFrame, SectionViewType viewType, bool bIsNewData)
         {
            mSectionBarGraph = sectionBarGraph;
            mGPUSectionBarGraph = gpuSectionBarGraph;
            mNumMeasuredSections = mSampleManager.mIdentiferCount;
            BRetiredProfileSample sample;


            bool measureAllFrames = true;

            if (bIsNewData == true)
            {
               for (ushort i = 0; i < mNumMeasuredSections; i++)
               {
                  mCountBySectionID[i] = 0;
               }
               for (int i = 0; i < topFrame.mNumSamples; i++)
               {
                  sample = topFrame.mSamples[i];
                  if (sample.mSectionID > mNumMeasuredSections)
                  {
                     ErrorHandler.Error("Invalid Section ID: " + sample.ToString());
                     continue;
                  }
                  mCountBySectionID[sample.mSectionID]++;
               }
               foreach(Frame threadFrame in topFrame.mThreadFrames)
               {
                  for (int i = 0; i < threadFrame.mNumSamples; i++)
                  {
                     sample = threadFrame.mSamples[i];//topFrame.mSamples[i];
                     if (sample.mSectionID > mNumMeasuredSections)
                     {
                        ErrorHandler.Error("Invalid Section ID: " + sample.ToString());
                        continue;
                     }
                     mCountBySectionID[sample.mSectionID]++;
                  }

               }
            }

            if(((viewType == SectionViewType.cBottomFrame) && (bIsNewData == true))  
               || ((viewType == SectionViewType.cTopDiffBottom) && (bottomFrame == null)) )
            {
               //throw new System.Exception("SetSectionStats Operation Not supported");
               return;
            }



            if(viewType == SectionViewType.cTopFrame || viewType == SectionViewType.cTopDiffBottom) 
            {
               if(topFrame == null || topFrame.mSamples == null)
               {
                  ErrorHandler.Error("Error: topFrame.mSamples == null " + topFrame.ToString());
                  return;               
               }               
               
               for(int i=0; i < mNumMeasuredSections; i++)
               {
                  mSectionStats[i].mValue = 0;
                  mSectionStats[i].mName = mSampleManager.mSections[i].mName;
                  mSectionStats[i].mColor = mSectionBarGraph.GetColor(mSampleManager.mSections[i].mID);
                  mGPUSectionStats[i].mValue = 0;
                  mGPUSectionStats[i].mName = mSampleManager.mSections[i].mName;
                  mGPUSectionStats[i].mColor = mSectionBarGraph.GetColor(mSampleManager.mSections[i].mID);

                  mTempCPU[i] = 0;
                  mTempGPU[i] = 0;

               }



               for (int i = 0; i < topFrame.mNumSamples; i++)
               {
                  sample = topFrame.mSamples[i];
                  if (sample.mSectionID > mNumMeasuredSections)
                  {
                     ErrorHandler.Error("Invalid Section ID: " + sample.ToString());
                     continue;
                  }
                  mTempCPU[sample.mSectionID] += (sample.mCPUEndTime - sample.mCPUStartTime);
                  mTempGPU[sample.mSectionID] += (sample.mGPUEndTime - sample.mGPUStartTime);

               }
               if (measureAllFrames)
               {
                  foreach (Frame threadFrame in topFrame.mThreadFrames)
                  {
                     for (int i = 0; i < threadFrame.mNumSamples; i++)
                     {
                        sample = threadFrame.mSamples[i];
                        if (sample.mSectionID > mNumMeasuredSections)
                        {
                           ErrorHandler.Error("Invalid Section ID: " + sample.ToString());
                           continue;
                        }
                        mTempCPU[sample.mSectionID] += (sample.mCPUEndTime - sample.mCPUStartTime);
                        mTempGPU[sample.mSectionID] += (sample.mGPUEndTime - sample.mGPUStartTime);
                     }

                  }
               }

               for(int i=0; i < mNumMeasuredSections; i++)
               {
                  mSectionStats[i].SetValue(mTempCPU[i], bIsNewData);
                  mGPUSectionStats[i].SetValue(mTempGPU[i], bIsNewData);
               }
               mSectionBarGraph.SetData(mSectionStats, mNumMeasuredSections);
               mGPUSectionBarGraph.SetData(mGPUSectionStats, mNumMeasuredSections);

            }
            if(viewType == SectionViewType.cBottomFrame || viewType == SectionViewType.cTopDiffBottom)
            {
               if(bottomFrame == null || bottomFrame.mSamples == null)
               {
                  ErrorHandler.Error("Error: bottomFrame.mSamples == null " + bottomFrame.ToString());
                  return;               
               }               
               for(int i=0; i < mNumMeasuredSections; i++)
               {
                  mSectionStats2[i].mValue = 0;
                  mSectionStats2[i].mName = mSampleManager.mSections[i].mName;
                  mSectionStats2[i].mColor = mSectionBarGraph.GetColor(mSampleManager.mSections[i].mID);
                  mGPUSectionStats2[i].mValue = 0;
                  mGPUSectionStats2[i].mName = mSampleManager.mSections[i].mName;
                  mGPUSectionStats2[i].mColor = mSectionBarGraph.GetColor(mSampleManager.mSections[i].mID);
               }

               for(int i=0; i < bottomFrame.mNumSamples; i++)
               {
                  sample = bottomFrame.mSamples[i];
                  if(sample.mSectionID > mNumMeasuredSections)
                  {
                     ErrorHandler.Error("Invalid Section ID: " + sample.ToString());
                     continue;
                  }
                  mSectionStats2[sample.mSectionID].mValue += (sample.mCPUEndTime - sample.mCPUStartTime);
                  mGPUSectionStats2[sample.mSectionID].mValue += (sample.mGPUEndTime - sample.mGPUStartTime);
               }
               if (measureAllFrames)
               {
                  foreach (Frame threadFrame in topFrame.mThreadFrames)
                  {
                     for (int i = 0; i < threadFrame.mNumSamples; i++)
                     {
                        sample = threadFrame.mSamples[i];
                        if (sample.mSectionID > mNumMeasuredSections)
                        {
                           ErrorHandler.Error("Invalid Section ID: " + sample.ToString());
                           continue;
                        }
                        mSectionStats2[sample.mSectionID].mValue += (sample.mCPUEndTime - sample.mCPUStartTime);
                        mGPUSectionStats2[sample.mSectionID].mValue += (sample.mGPUEndTime - sample.mGPUStartTime);
                     }
                  }
               }

               mSectionBarGraph.SetData(mSectionStats2, mNumMeasuredSections);
               mGPUSectionBarGraph.SetData(mGPUSectionStats2, mNumMeasuredSections);
            }         
            if(viewType == SectionViewType.cTopDiffBottom)
            {
               for(int i=0; i < mNumMeasuredSections; i++)
               {
                  mSectionStats3[i].mValue = 0;
                  mSectionStats3[i].mName = mSampleManager.mSections[i].mName;
                  mSectionStats3[i].mColor = mSectionBarGraph.GetColor(mSampleManager.mSections[i].mID);
                  mGPUSectionStats3[i].mValue = 0;
                  mGPUSectionStats3[i].mName = mSampleManager.mSections[i].mName;
                  mGPUSectionStats3[i].mColor = mSectionBarGraph.GetColor(mSampleManager.mSections[i].mID);
               }

               for(int i=0; i < mNumMeasuredSections; i++)
               {
                  mSectionStats3[i].mValue = mSectionStats[i].mValue - mSectionStats2[i].mValue;
                  mGPUSectionStats3[i].mValue = mGPUSectionStats[i].mValue - mGPUSectionStats2[i].mValue;
               
                  //               mSectionStats3[i].mTotal = mSectionStats[i].mTotal - mSectionStats2[i].mTotal;
                  //               mGPUSectionStats3[i].mTotal = mGPUSectionStats[i].mTotal - mGPUSectionStats2[i].mTotal;
                  //               mSectionStats3[i].mCount = mSectionStats[i].mCount;
                  //               mGPUSectionStats3[i].mCount = mSectionStats[i].mCount;   
               }
               mSectionBarGraph.SetData(mSectionStats3, mNumMeasuredSections);
               mGPUSectionBarGraph.SetData(mGPUSectionStats3, mNumMeasuredSections);
            }

            mSectionBarGraph.UpdateIfNeeded(); 
            mGPUSectionBarGraph.UpdateIfNeeded(); 
         }

         string mCSVFileName = "Measuements.txt";
         

         //todo: process viewType
         public void ExportToCSV(string comment, SectionViewType viewType)
         {
            //if(mTopSelectedFrame == null)
            //   return;
            int numSections = mSampleManager.mIdentiferCount;

            if(numSections <=0)
               return;
      
            StreamWriter writer;
            if(File.Exists(mCSVFileName) == false)
            {
               writer = new StreamWriter(mCSVFileName);
               //write header...
               writer.Write("comments");
               for(int i=0; i < numSections; i++)
               {
                  writer.Write("," + mSampleManager.mSections[i].mName);
               }
               writer.WriteLine("");
               writer.Close();
            }
            writer = new StreamWriter(mCSVFileName,true);
            comment  = comment.Replace(","," ");
            writer.Write(comment); 
            for(int i=0; i < numSections; i++)
            {
               writer.Write(String.Format(",{0}", mSectionStats[i].mValue));
            }
            writer.WriteLine("");
            writer.Close();
         }



      }

      class TimelineDataManager
      {

         ColorGenerator mColorGen = new ColorGenerator();

         public TimelineDataManager()
         {
            InitilizeScalarPlots();
         }
         public void InitilizeScalarPlots( )
         {   
            //reset start color...
            mColorGen.Reset();

            mTimePlots.Add( new FrameStatPlot(null,mColorGen.GetNextColor(),FrameStatPlot.ScalarPlots.CPU_Total));
            mTimePlots.Add( new FrameStatPlot(null,mColorGen.GetNextColor(),FrameStatPlot.ScalarPlots.GPU_Total));
            mTimePlots.Add( new FrameStatPlot(null,mColorGen.GetNextColor(),FrameStatPlot.ScalarPlots.Both_Total));
            mTimePlots.Add( new FrameStatPlot(null,mColorGen.GetNextColor(),FrameStatPlot.ScalarPlots.CPU_GPU_Delta));
            
            mColorGen.Reset();
            mCoreDutyPlots.Add( new DutyCyclePlot(null,mColorGen.GetNextColor(),DutyCyclePlot.DutyCyclePlots.CORE0));
            mCoreDutyPlots.Add( new DutyCyclePlot(null,mColorGen.GetNextColor(),DutyCyclePlot.DutyCyclePlots.CORE1));
            mCoreDutyPlots.Add( new DutyCyclePlot(null,mColorGen.GetNextColor(),DutyCyclePlot.DutyCyclePlots.CORE2));
            
            mColorGen.Reset();
            mCPUDutyPlots.Add( new DutyCyclePlot(null,mColorGen.GetNextColor(),DutyCyclePlot.DutyCyclePlots.CPU0));
            mCPUDutyPlots.Add( new DutyCyclePlot(null,mColorGen.GetNextColor(),DutyCyclePlot.DutyCyclePlots.CPU1));
            mCPUDutyPlots.Add( new DutyCyclePlot(null,mColorGen.GetNextColor(),DutyCyclePlot.DutyCyclePlots.CPU2));
            mCPUDutyPlots.Add( new DutyCyclePlot(null,mColorGen.GetNextColor(),DutyCyclePlot.DutyCyclePlots.CPU3));
            mCPUDutyPlots.Add( new DutyCyclePlot(null,mColorGen.GetNextColor(),DutyCyclePlot.DutyCyclePlots.CPU4));
            mCPUDutyPlots.Add( new DutyCyclePlot(null,mColorGen.GetNextColor(),DutyCyclePlot.DutyCyclePlots.CPU5));
            
            mGpuDutyPlots.Add( new DutyCyclePlot(null,mColorGen.GetNextColor(),DutyCyclePlot.DutyCyclePlots.GPU));
            

            mDataPlots.AddRange(mTimePlots);
            mDataPlots.AddRange(mCPUDutyPlots);
            mDataPlots.AddRange(mCoreDutyPlots);
            mDataPlots.AddRange(mGpuDutyPlots);
            mDataPlots.AddRange(mSectionPlots);


            SetViewGroup(ViewGroups.Time);
         }

         public ArrayList mDataPlots = new ArrayList();
         ArrayList mTimePlots = new ArrayList();
         ArrayList mCPUDutyPlots = new ArrayList();
         ArrayList mCoreDutyPlots = new ArrayList();
         ArrayList mGpuDutyPlots = new ArrayList();
         ArrayList mSectionPlots = new ArrayList();
         ViewGroups mViewGroup;

         public ArrayList mVisiblePlots = new ArrayList();
         public void SetViewGroup(ViewGroups group)
         {
            mViewGroup = group;

            mVisiblePlots.Clear();

            if(group == ViewGroups.DutyCycle_CPU)
               mVisiblePlots.AddRange(mCPUDutyPlots);
            if(group == ViewGroups.DutyCycle_Core)
               mVisiblePlots.AddRange(mCoreDutyPlots);
            if(group == ViewGroups.Time)
               mVisiblePlots.AddRange(mTimePlots);
            
            if(group == ViewGroups.DutyCycle_Core || group == ViewGroups.DutyCycle_CPU)
               mVisiblePlots.AddRange(mGpuDutyPlots);

            if (group == ViewGroups.Selected_Sections)
            {

            }
            
         }
         
         public enum ViewGroups
         {
            Time,
            DutyCycle_CPU,
            DutyCycle_Core,
            Selected_Sections
         }
         public ArrayList GetGroupOptions()
         {
            ArrayList options = new ArrayList();
            options.AddRange(Enum.GetNames(typeof(TimelineDataManager.ViewGroups)));
            return options;
         }   
         public void SetGroup(string group)
         {
            SetViewGroup((ViewGroups)Enum.Parse(typeof(ViewGroups),group,true));            
         }

//         public ArrayList GetOptions()
//         {
//            ArrayList options = new ArrayList();
//            options.AddRange(ExpandEnum(typeof(DutyCyclePlot.DutyCyclePlots)));
//   
//         }
//         public ArrayList ExpandEnum(Type enumType)
//         {
//            ArrayList options = new ArrayList();
//            string[] plotNames = Enum.GetNames(enumType);            
//            string name = enumType.Name;
//            foreach(string s in plotNames)
//            {
//               options.Add(name + "." + s);
//            }
//         }

         public void PopulateGraphData(Frame f)
         {
            foreach(FrameCentricDataPlot plot in mDataPlots)
            {  
               plot.NewFrameData(f);
            }
         }
         public void SetGraphData(FastGraph timelineGraph, ArrayList frames)
         {
            if (mViewGroup == ViewGroups.Time)
            {
               timelineGraph.mbTimeMarkers = true;
            }
            else
            {
               timelineGraph.mbTimeMarkers = false;
            }

            timelineGraph.SetScalarPlots((ScalarPlotData[])mVisiblePlots.ToArray(typeof(ScalarPlotData)), frames);
            timelineGraph.UpdateGraphData();
         }

/////////////////////////////

         class FrameCentricDataPlot : ScalarPlotData
         {
            public bool visible = false;
            public FrameCentricDataPlot(string name, int color) : base(name,color){}
            public virtual void NewFrameData(ArrayList mAllFrames)
            {
               foreach(Frame f in mAllFrames)
               {
                  NewFrameData(f);
               }
            }
            //duty cycle for frame numbers...
            public virtual void NewFrameData(Frame f){}   
       
         };
         class DutyCyclePlot : FrameCentricDataPlot
         {
            public enum DutyCyclePlots
            {
               CPU0 = 0,
               CPU1,
               CPU2,
               CPU3,
               CPU4,
               CPU5,
               CORE0,
               CORE1,
               CORE2,
               GPU
            } 
            DutyCyclePlots mPlotType;

            public DutyCyclePlot(string name, int color, DutyCyclePlots plotType) : base(name,color)
            {
               mPlotType = plotType;
               if(name == null)
               {
                  this.mName = plotType.ToString() + "(%)";
               }
            }
            public DutyCyclePlots cpuToCore(DutyCyclePlots cpu)
            {
               if(cpu == DutyCyclePlots.CPU0 || cpu == DutyCyclePlots.CPU1)
                  return DutyCyclePlots.CORE0;
               else if(cpu == DutyCyclePlots.CPU2 || cpu == DutyCyclePlots.CPU3)
                  return DutyCyclePlots.CORE1;
               else if(cpu == DutyCyclePlots.CPU4 || cpu == DutyCyclePlots.CPU5)
                  return DutyCyclePlots.CORE2;
               return cpu;
            }
            public override void NewFrameData(Frame f)
            {
               double duration = f.mNextFrameStartTime - f.mStartTime;
               double start = f.mStartTime;

               ArrayList cycles = new ArrayList();

               double startTime = double.MaxValue;

               if(mPlotType == DutyCyclePlots.GPU)
               {
                  cycles.Add(f.mGpuDutyCycle);
               }
               else
               {
                  //main thread
                  if((f.mCPUID == (int)mPlotType) || (cpuToCore((DutyCyclePlots)f.mCPUID) == mPlotType))
                  { 
                     cycles.Add(f.mDutyCycle);
                     if(f.mStartTime < startTime)
                     {
                        startTime = f.mStartTime;
                     }
                  }

                  //threadFrames
                  foreach(Frame t in f.mThreadFrames)
                  {
                     if((t.mCPUID == (int)mPlotType) || (cpuToCore((DutyCyclePlots)t.mCPUID) == mPlotType))
                     { 
                        cycles.Add(t.mDutyCycle);
                        if(t.mStartTime < startTime)
                        {
                           startTime = t.mStartTime;
                        }
                     }
                  }
               }
               if(startTime > f.mStartTime)
               {
                  startTime = f.mStartTime;
               }

               Frame.DutyCycle total = new Frame.DutyCycle();
               total.mStartTime = startTime;

               foreach(Frame.DutyCycle c in cycles)
               {
                  if(total.MergeDutyCycle(c) == false)
                  {
                     throw new System.Exception("error merging duty cycles");
                  }
               }
               double timeBusy = total.Evaluate(f.mStartTime,f.mNextFrameStartTime);
               Add(0.1f * timeBusy/duration);
               
            }
         }
         class SectionStatPlot : FrameCentricDataPlot
         {
            public enum FrameCentricDataPlots
            {
               CPU0 = 0,
               CPU1,
               CPU2,
               CPU3,
               CPU4,
               CPU5,
               CORE0,
               CORE1,
               CORE2,
               GPU,
               AllCpu,
               ThreadID
            } 
            public int mSectionID;
            public SectionStatPlot(string name, int color, int SectionID ) : base(name,color)
            {
               mSectionID = SectionID;
               if(name == null)
               {
                  this.mName = mSectionID.ToString();
               }
            }
            public override void NewFrameData(Frame f)
            {
               //optimize this later by grouping calculations together
               //Consider all threads.. is there ever a reason to isolate stuff.?
               
//               ResourceHandle h = f.SafeGetBufferHandle();
//               uint data = 0;
//               BRetiredProfileSample sample;
//               for(int i=0; i < f.mNumSamples; i++)
//               {
//                  sample = f.mSamples[i];
//                  if(sample.mSectionID == mSectionID)
//                  {
//                     data += (sample.mCPUEndTime - sample.mCPUStartTime);
//                  }
//               } 
//               Add(data );// / Constants.cUIntScaleToMS);


               uint data = 0;

               data = f.mSectionStats[mSectionID];

               Add(data);
            }  
            
         }
         class FrameStatPlot : FrameCentricDataPlot
         {
            public enum ScalarPlots
            {
               CPU_Total = 0,
               GPU_Total,
               Both_Total,
               CPU_GPU_Delta
            }
            public ScalarPlots mPlotType;
            public FrameStatPlot(string name, int color, ScalarPlots plotType) : base(name,color)
            {
               mPlotType = plotType;
               if(name == null)
               {
                  this.mName = plotType.ToString();
               }
            }
            public override void NewFrameData(Frame f)
            {
               if(mPlotType == ScalarPlots.CPU_Total)
                  Add(     f.getCPUEndTime() - f.getCPUStartTime());
               else if(mPlotType == ScalarPlots.GPU_Total)
                  Add(     f.getGPUEndTime() - f.getGPUStartTime());
               else if(mPlotType == ScalarPlots.GPU_Total)
                  Add(    f.getGPUEndTime() - f.getCPUStartTime());
               else if(mPlotType == ScalarPlots.GPU_Total)
                  Add( f.getGPUStartTime() - f.getCPUStartTime());
            }
          };
      }

      #endregion

      public void Start()
      {
         ErrorHandler.Error("Sendingws Start Command@" + System.DateTime.Now.ToString());
         InitializeIfNeeded();
         SetTimelineState(TimelineControlPacket.TimelineControlValues.cStart);
      }
      public void Stop()
      {
         ErrorHandler.Error("Sending End Command@" + System.DateTime.Now.ToString());
         SetTimelineState(TimelineControlPacket.TimelineControlValues.cStop);         

      }

      #region UI Code.  Event handling and helpers

      private void StartPerfSendButton_Click_1(object sender, System.EventArgs e)
      {
         Start();
      }
      private void StopPerfSendButton_Click(object sender, System.EventArgs e)
      {
         Stop();
      }

      private void RestartButton_Click(object sender, System.EventArgs e)
      {
         SetTimelineState(TimelineControlPacket.TimelineControlValues.cStop);         
         InitializeGraphics(this);
         InitInternalClasses();
         mInitialized = true;
         InitilizeAlways();   
         SetTimelineState(TimelineControlPacket.TimelineControlValues.cStart);
      }

      private bool Connected()
      {
         return (mActiveClient != null);
      }

      private void SetTimelineState(TimelineControlPacket.TimelineControlValues state)
      {
         if(!Connected())
         {
            if (state != TimelineControlPacket.TimelineControlValues.cStop)
               MessageBox.Show(this,"No remote machine connected (Use game console command: attachRemoteDebugger(\"10.10.36.???\") with your PC's IP.");
            return;
         }
         MemoryStream memStream = new MemoryStream();
         TimelineControlPacket timeControl = new TimelineControlPacket(memStream, state);
         mActiveClient.write(memStream);
      }

      private void fastGraph1_OldFrameSelected_1(object sender, EnsembleStudios.RemoteGameDebugger.Frame f, MouseButtons context)
      {
         try
         {
            if(MouseButtons.Left == context)
            {
               mLiveUpdate = false;
               mTopSelectedFrame = f;
               mSectionViewType =  SectionViewType.cTopFrame;
               this.profilerFrameView1.SetFrame(f, ref mSampleManager.mSections, mSampleManager.mIdentiferCount);
            }
            else if(MouseButtons.Right == context)
            {
               mBottomSeletedFrame = f;
               mSectionViewType =  SectionViewType.cBottomFrame;
               //this.profilerFrameView2.SetFrame(f, ref mSampleManager.mSections, mSampleManager.mIdentiferCount);
            }
            if(mbShowDiff == true)
            {
               mSectionViewType = SectionViewType.cTopDiffBottom;
            }

            mCurrentWorkspace.mSectionStatsManager.SetSectionStats(SectionBarGraph, GPUSectionBarGraph, mTopSelectedFrame, mBottomSeletedFrame, mSectionViewType, false);
         }
         catch(System.Exception ex)
         {
            ErrorHandler.Error(ex.ToString());
            //MessageBox.Show(this,"Error selecting frame:" + ex.ToString());
         }
      }

      private void fastGraph1_NewestFrameSelected_1(object sender, EnsembleStudios.RemoteGameDebugger.Frame f, MouseButtons context)
      {
         try
         {
            if(MouseButtons.Left == context)
            {
               mLiveUpdate = true;  
               mTopSelectedFrame = f;
               mSectionViewType =  SectionViewType.cTopFrame;
               this.profilerFrameView1.SetFrame(f, ref mSampleManager.mSections, mSampleManager.mIdentiferCount);  
            }
            else if(MouseButtons.Right == context)
            {
               mBottomSeletedFrame = f;
               mSectionViewType =  SectionViewType.cBottomFrame;
               //this.profilerFrameView2.SetFrame(f, ref mSampleManager.mSections, mSampleManager.mIdentiferCount);  
            }

            if(mbShowDiff == true)
            {
               mSectionViewType = SectionViewType.cTopDiffBottom;
            }

            mCurrentWorkspace.mSectionStatsManager.SetSectionStats(SectionBarGraph, GPUSectionBarGraph, mTopSelectedFrame, mBottomSeletedFrame, mSectionViewType, false);
         }
         catch(System.Exception ex)
         {
            ErrorHandler.Error(ex.ToString());
            //MessageBox.Show(this,"Error selecting frame:" + ex.ToString());
         }
      }

      private void NavLastFrameButton_Click(object sender, EventArgs e)
      {
         if (mTopSelectedFrame == null || mTopSelectedFrame.mLastFrame == null)
            return;

         mTopSelectedFrame = mTopSelectedFrame.mLastFrame;
         mSectionViewType = SectionViewType.cTopFrame;
         this.profilerFrameView1.SetFrame(mTopSelectedFrame, ref mSampleManager.mSections, mSampleManager.mIdentiferCount);

         mCurrentWorkspace.mSectionStatsManager.SetSectionStats(SectionBarGraph, GPUSectionBarGraph, mTopSelectedFrame, mBottomSeletedFrame, mSectionViewType, false);

      }

      private void NavNextFrameButton_Click(object sender, EventArgs e)
      {

         if (mTopSelectedFrame == null || mTopSelectedFrame.mNextFrame == null)
            return;

         mTopSelectedFrame = mTopSelectedFrame.mNextFrame;
         mSectionViewType = SectionViewType.cTopFrame;
         this.profilerFrameView1.SetFrame(mTopSelectedFrame, ref mSampleManager.mSections, mSampleManager.mIdentiferCount);

         mCurrentWorkspace.mSectionStatsManager.SetSectionStats(SectionBarGraph, GPUSectionBarGraph, mTopSelectedFrame, mBottomSeletedFrame, mSectionViewType, false);

      }
            
      private void FrameViewYZoomBar_Scroll(object sender, EventArgs e)
      {
         profilerFrameView1.ZoomX(FrameViewYZoomBar.Value,true); 
      }
      private void FrameHorizontalScrollBar_Scroll(object sender, System.Windows.Forms.ScrollEventArgs e)
      {
         profilerFrameView1.ScrollX(e.NewValue, FrameHorizontalScrollBar.Maximum);
      }
      private void Frame2ZoomScrollBar_Scroll(object sender, System.Windows.Forms.ScrollEventArgs e)
      {
         //profilerFrameView2.ZoomX(e.NewValue,true);  
      }
      private void Timeline1hScrollBar_Scroll(object sender, System.Windows.Forms.ScrollEventArgs e)
      {
         mLiveUpdate = false;
         this.LiveUpdateCheckBox.Checked = mLiveUpdate;

         fastGraph1.ScrollX(e.NewValue, Timeline1hScrollBar.Maximum);
      }
      private void Frame2HorizontalScrollBar_Scroll(object sender, System.Windows.Forms.ScrollEventArgs e)
      {
         //profilerFrameView2.ScrollX(e.NewValue, FrameHorizontalScrollBar.Maximum);
      }
      private void trackBar1_Scroll(object sender, EventArgs e)
      {
         fastGraph1.ZoomY(GraphZoomYBar.Value, true);

      }

      private void SectionDataScrollbar_Scroll(object sender, ScrollEventArgs e)
      {
         SectionBarGraph.ScrollY(e.NewValue, SectionDataScrollbar.Maximum);
      }

      private void AALinesCheckBox_CheckedChanged(object sender, System.EventArgs e)
      {
         foreach(System.Windows.Forms.Control c in this.Controls)
         {
            if(c is DXManagedControl)
            {
               ((DXManagedControl)c). mbDrawAALines = AALinesCheckBox.Checked;
            }
         }
      }

      private void CompressionTestButton_Click(object sender, System.EventArgs e)
      {
         //CompressionTest test = new CompressionTest();
         //test.SimpleTest();

         bool bUseGraphics = true;
         if(bUseGraphics == false)
         {
            mSampleManager = new SampleManager();
            //InitSectionStats();
            mMessageHandler = new ProfilerConnectionHandler();
            mMessageHandler.AddObserver(mSampleManager);
            InitilizeAlways();
         }
         else
         {
            mInitialized = false;
            InitializeIfNeeded();
         }
         TimeTest t = new TimeTest();
         FileNetworkReader test = new FileNetworkReader();
         double t2 = test.Go(this,1);
         TestResultsTextBox.Text = t.GetElapsed().ToString() + " " + t2.ToString();
      }
      private void TimelineVis_MouseWheel(object sender, MouseEventArgs e)
      {
         Rectangle r = new Rectangle(e.X,e.Y,0,0);
         r = this.RectangleToScreen(r);
         DoMouseWheel(this,r.Left,r.Top, e);
      }
      //Without this, you would have to click on each control before the mouse wheel would work. yuck
      public void DoMouseWheel(Control containter, int x, int y, MouseEventArgs e)
      {
         foreach(System.Windows.Forms.Control c in containter.Controls)
         {
            try
            {            
               if(c is Form)
               {
                  DoMouseWheel(c,x,y,e);
               }
               else if(c is Panel)
               {
                  DoMouseWheel(c,x,y,e);
               }
               else if (c is SplitContainer)
               {
                  DoMouseWheel(c, x, y, e);
               }
               else if(c is FastGraph)
               {
                  Rectangle r = c.RectangleToScreen(c.ClientRectangle);
                  if(r.Contains(x,y))
                  {
                     FastGraph g = (FastGraph)c;
                     //GraphZoomScrollBar.Value += (e.Delta / 10);
                     GraphZoomYBar.Value = (int)Math.Max(Math.Min(GraphZoomYBar.Value + (e.Delta / 10), GraphZoomYBar.Maximum), GraphZoomYBar.Minimum);
                     fastGraph1.ZoomY(GraphZoomYBar.Value, true);
                  }
               }
               else if(c is ProfilerFrameView)
               {
                  Rectangle r = c.RectangleToScreen(c.ClientRectangle);

                  if(r.Contains(x,y))
                  {
                     ProfilerFrameView view = (ProfilerFrameView)c;
                     view.WheelZoom(e);
                  }
               }
            }
            catch(System.ArgumentException argex)
            {
               //we dont care for now
               argex.ToString();
               //ErrorHandler.Error(argex.ToString());
            }
            catch(System.Exception ex)
            {
               ErrorHandler.Error(ex.ToString());
            }         
         }   
      }
      private void FilterSectionsButton_Click(object sender, System.EventArgs e)
      {
         if(mSampleManager == null || mSampleManager.mSections == null)
            return;

         if((mSectionSelection == null) || (mSectionSelection.IsDisposed))
         {
            mSectionSelection = new ProfileSectionSelection();
            mSectionSelection.SetData(mSampleManager.mSections, mSampleManager.mIdentiferCount);
            mSectionSelection.ActiveClient = mActiveClient;
            mSectionSelection.Show();         
         }
      }

      private void AverageTypeComboBox_SelectedIndexChanged(object sender, System.EventArgs e)
      {

         if(AverageTypeComboBox.Text.IndexOf("5 Frame Average") != -1)
         {
            SectionBarGraph.mMode = FastBarGraph.StatMode.cAverageNSamples;
            GPUSectionBarGraph.mMode = FastBarGraph.StatMode.cAverageNSamples;
         }
         else if(AverageTypeComboBox.Text.IndexOf("All Frames Average") != -1)
         {
            SectionBarGraph.mMode = FastBarGraph.StatMode.cAverageTotal;
            GPUSectionBarGraph.mMode = FastBarGraph.StatMode.cAverageTotal;
         }
         else if(AverageTypeComboBox.Text.IndexOf("Current Frame") != -1)
         {
            SectionBarGraph.mMode = FastBarGraph.StatMode.cNoAverage;
            GPUSectionBarGraph.mMode = FastBarGraph.StatMode.cNoAverage;
         }
         SectionBarGraph.UpdateIfNeeded();
         GPUSectionBarGraph.UpdateIfNeeded();    
      }


      SectionViewType mLastSectionType = SectionViewType.cTopFrame;
      private void DiffCheckBox_CheckedChanged(object sender, System.EventArgs e)
      {
         mbShowDiff = DiffCheckBox.Checked;
         SectionBarGraph.mbDiffView = mbShowDiff;
         GPUSectionBarGraph.mbDiffView = mbShowDiff;
         if(mbShowDiff == true)
         {
            mLastSectionType = mSectionViewType;
            mSectionViewType = SectionViewType.cTopDiffBottom;
            mCurrentWorkspace.mSectionStatsManager.SetSectionStats(SectionBarGraph, GPUSectionBarGraph, mTopSelectedFrame, mBottomSeletedFrame, mSectionViewType, false);
         }
         else
         {
            mSectionViewType = mLastSectionType;
            mCurrentWorkspace.mSectionStatsManager.SetSectionStats(SectionBarGraph, GPUSectionBarGraph, mTopSelectedFrame, mBottomSeletedFrame, mSectionViewType, false);
         }
      }

      private void SectionsZoomCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         SectionBarGraph.mbZoom = SectionsZoomCheckBox.Checked;
         GPUSectionBarGraph.mbZoom = SectionsZoomCheckBox.Checked;

         SectionBarGraph.UpdateIfNeeded();
         GPUSectionBarGraph.UpdateIfNeeded(); 
      }

      private void SortBarResultsCheckBox_CheckedChanged(object sender, System.EventArgs e)
      {
         SectionBarGraph.mbSortList = SortBarResultsCheckBox.Checked;
         GPUSectionBarGraph.mbSortList = SortBarResultsCheckBox.Checked;

         SectionBarGraph.UpdateIfNeeded(); 
         GPUSectionBarGraph.UpdateIfNeeded(); 
      }

      private void ResetStatsButton_Click(object sender, System.EventArgs e)
      {
         mCurrentWorkspace.mSectionStatsManager.ResetSectionStats();
         
         SectionBarGraph.UpdateIfNeeded(); 
         GPUSectionBarGraph.UpdateIfNeeded(); 
      }
      private void ExportToCSVButton_Click(object sender, System.EventArgs e)
      {
         string comment = SnapCommentsTextBox.Text;
         mCurrentWorkspace.mSectionStatsManager.ExportToCSV(comment,SectionViewType.cTopFrame);
      }

      private void LiveUpdateCheckBox_CheckedChanged(object sender, System.EventArgs e)
      {
         mLiveUpdate = LiveUpdateCheckBox.Checked; 
         fastGraph1.mbShowLatestData = mLiveUpdate;
         if(mLiveUpdate == false)
         {
//this should no longer be needed with the new system
            //seal in freshness...   Decompress the current live frame to a different buffer
//            if((mSampleManager != null) && (mTopSelectedFrame != null))
//               mSampleManager.ExpandFrame(mTopSelectedFrame,1);
         }
      }
      

      private void PauseMenuItem_Select(object sender, System.EventArgs e)
      {
         mLiveUpdate = !LiveUpdateCheckBox.Checked; 
         LiveUpdateCheckBox.Checked = mLiveUpdate;
      }

      private void DiagnosticsButton_Click(object sender, System.EventArgs e)
      {
         if((mDiagnosticsForm == null) || (mDiagnosticsForm.IsDisposed))
         {
            mDiagnosticsForm = new DiagnosticsForm();
         }
         mDiagnosticsForm.Show();   
      }      
      
      private void TestButton_Click(object sender, System.EventArgs e)
      {
//         if(mSampleManager != null)
//         {
//            mSampleManager.SaveData("asdf",0,10);
//         }

//         InitializeIfNeeded();
//         if(mSampleManager != null)
//         {
//            mSampleManager.LoadData("asdf");
//         }
         //mKeyHelper.check();

         KeyHelper.IsShiftDown();
      }
      private void SaveAsButton_Click(object sender, System.EventArgs e)
      {
         SaveFileDialog g = new SaveFileDialog();
         g.Filter = "data files (*.data)|*.data|All files (*.*)|*.*";
         g.FilterIndex = 1;

         if(g.ShowDialog() == DialogResult.OK)
         {
            if(mSampleManager != null)
            {
               mSampleManager.SaveData(g.FileName,0,10);
            }
         }
      }
      private void LoadButton_Click(object sender, System.EventArgs e)
      {
         OpenFileDialog g = new OpenFileDialog();
         g.Filter = "data files (*.data)|*.data|All files (*.*)|*.*";
         g.FilterIndex = 1;

         if(g.ShowDialog() == DialogResult.OK)
         {
            System.DateTime d = System.DateTime.Now;

            mInitialized = false;
            InitializeIfNeeded();
            if(mSampleManager != null)
            {
               mSampleManager.LoadData(g.FileName);
            }  
            //MessageBox.Show(this,((TimeSpan)(System.DateTime.Now - d)).ToString());

         }
         mInitialized = false;
      }

      private void TopFrameViewTypeCombo_SelectedIndexChanged(object sender, System.EventArgs e)
      {
         profilerFrameView1.ViewTypeLayout = TopFrameViewTypeCombo.Text;
      }

      private void TopFrameDrawTypeCombo_SelectedIndexChanged(object sender, System.EventArgs e)
      {
         profilerFrameView1.DrawTypeLayout = TopFrameDrawTypeCombo.Text;
      }
      private void TimelineGroupCombo_SelectedIndexChanged(object sender, System.EventArgs e)
      {
         if(mCurrentWorkspace != null)
         {
            mCurrentWorkspace.mTimelineDataManager.SetGroup(TimelineGroupCombo.Text);
            mCurrentWorkspace.mTimelineDataManager.SetGraphData(fastGraph1,mSampleManager.mFrames);
         }
      }
      #endregion

      private void ExportToCSVButton_Click_1(object sender, System.EventArgs e)
      {
      
      }

      private void ClearAllButton_Click(object sender, System.EventArgs e)
      {
         if(Connected())
            SetTimelineState(TimelineControlPacket.TimelineControlValues.cStop);         

         InitializeGraphics(this);
         InitInternalClasses();
         mInitialized = true;
         InitilizeAlways();   

         if(Connected())
            SetTimelineState(TimelineControlPacket.TimelineControlValues.cStart);

         profilerFrameView1.Clear();
         fastGraph1.Clear();
         
         SectionBarGraph.Clear();
         GPUSectionBarGraph.Clear();

         //mCurrentWorkspace.mTimelineDataManager.PopulateGraphData(null); 
         //mCurrentWorkspace.mTimelineDataManager.SetGraphData(fastGraph1,null);
//         profilerFrameView1.SetFrame(null, ref mSampleManager.mSections, mSampleManager.mIdentiferCount);
//         mCurrentWorkspace.mSectionStatsManager.SetSectionStats(SectionBarGraph, GPUSectionBarGraph, mTopSelectedFrame, mBottomSeletedFrame, mSectionViewType, true);
//         fastGraph1.SetTopViewLive();
      
      }

      private void AutoFilterCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         if(AutoFilterCheckBox.Checked == true)
         {
            FilterThreasholdLabel.Enabled = true;
            FilterThreasholdTrackBar.Enabled = true;
         }
         else
         {
            FilterThreasholdLabel.Enabled = false;
            FilterThreasholdTrackBar.Enabled = false;
         }
      }

      private void FilterThreasholdTrackBar_Scroll(object sender, EventArgs e)
      {
         mSectionFilterThreashold = FilterThreasholdTrackBar.Value;
         FilterThreasholdLabel.Text = FilterThreasholdTrackBar.Value + " samples/section";
      }

      private void SectionsShowGPUCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         if (SectionsShowGPUCheckBox.Checked == false)
         {
            try
            {

               panel7.Width = panel5.Width;
            }
            catch
            {
               //stupid forms
            }
            this.GPUSectionBarGraph.Enabled = false;

         }
         else
         {
            panel7.Width = panel5.Width / 2;
            this.GPUSectionBarGraph.Enabled = true;
         }
      }


      //Todo:  let the user select which threads to include
      private void FilterButton_Click(object sender, EventArgs e)
      {
         //Form f = new Form();
         //CheckedListBox cb = new CheckedListBox();
         //if(mTopSelectedFrame == null)
         //   return;
         //cb.Items.Add(mTopSelectedFrame.mThreadID.ToString(),true);
         //foreach(
         //{

         //}
      }
















   }

   #region Helper classes

   //just hardcoded a few colors that look different for now
   public class ColorGenerator
   {
      ArrayList mColorList = new ArrayList();
      int mIndex = 0;

      public ColorGenerator()
      {
         mColorList.Add(Color.DeepSkyBlue);
         mColorList.Add(Color.Cyan);
         mColorList.Add(Color.Yellow);
         mColorList.Add(Color.Magenta);
         mColorList.Add(Color.LightGreen);
         mColorList.Add(Color.Red);
         mColorList.Add(Color.Tan);
         mColorList.Add(Color.Orange);
      }
      public void Reset()
      {
         mIndex = 0;
      }
      public int GetNextColor()
      {
         mIndex++;
         if(mIndex >= mColorList.Count)
            mIndex=0;
         return ((Color) (mColorList[mIndex]) ).ToArgb();
      }
   }

   public class TimeTest
   {
      public TimeTest()
      {
         Start();
      }
      public void Start()
      {
         mStartTime = System.DateTime.Now;
      }
      public double GetElapsed()
      {
         mEndTime = System.DateTime.Now;
         return ((System.TimeSpan)(mEndTime - mStartTime)).TotalMilliseconds;
      }
      DateTime mStartTime;
      DateTime mEndTime;
   }

   public class FileNetworkReader
   {
      public FileNetworkReader()
      {
      }
      public double Go(TimelineVis visApp, int repeat)
      {
         FileStream f = File.OpenRead("2blah.stream");
         byte[] buffer = new byte[f.Length];
         f.Read(buffer,0,(int)f.Length);
         
         TimeTest t = new TimeTest();

         long bytesRead = 0;
         for(int j=0; j<repeat; j++)
         {

            MemoryStream m = new MemoryStream(buffer,0,(int)f.Length,false,true);
            BinaryReader reader = new BinaryReader(m);

            byte[] _buffer = new byte[3000];
         
            long lastOffset = 0;
            try
            {
               while(reader.PeekChar() != -1)
               {
                  lastOffset = bytesRead;
                  int _Size = reader.ReadInt32();//-1; // minus one for the type byte below
                  byte _Type = reader.ReadByte();  
                  reader.ReadBytes(_Size).CopyTo(_buffer,0);
    
                  bytesRead = bytesRead + 5 + _Size;

                  MemoryStream memStream = new MemoryStream(_buffer);
                  BinaryReader breader = new BinaryReader(memStream, System.Text.Encoding.Unicode);
                  visApp.HandleMessage(_Type,breader);  
               }
            }
            catch(System.Exception ex)
            {
               ErrorHandler.Error(ex.ToString());
               //search if the frame was to short or too long
            }
         }
         return t.GetElapsed();
      }
   }

   class CursorHelper
   {
      public struct C_POINT
      {
         public int x;
         public int y;
      }
      
      [System.Runtime.InteropServices.DllImport("user32.dll",EntryPoint="GetCursorPos",SetLastError=true)]
      public static extern int GetCursorPos(ref C_POINT screenPoint);
      
      public C_POINT GetCursorPos()
      {
         C_POINT pCurrCoord = new C_POINT();
         int chk = GetCursorPos(ref pCurrCoord);
         return pCurrCoord;
      }
   }

   class KeyHelper
   {
      public enum VKeyCodes
      {
         VK_PAUSE = 0x13,
         VK_SHIFT = 0x10
      }
      [System.Runtime.InteropServices.DllImport("user32.dll",EntryPoint="GetAsyncKeyState",SetLastError=true)]
      public static extern long GetAsyncKeyState (long vKey);

      static public bool CheckHotKey(VKeyCodes key)
      {
//         if((GetAsyncKeyState((long)key) & 0x8001)!=0)
//         {
//            return true;
//         }
         return false;         
      }
      static public bool IsShiftDown()
      {
         if((GetAsyncKeyState((long)VKeyCodes.VK_SHIFT) & 0x8000)!=0)
         {
            return true;
         }
         return false;
      }
   }

   //for data binding
   public class LambdaProperty
   {
      private object mO;
      public LambdaProperty(object o){mO=o;}
      public object LambdaValue
      {
         get
         {
            return mO;
         }
         set
         {
            mO = value;
         }
      }
   }
   #endregion
}

