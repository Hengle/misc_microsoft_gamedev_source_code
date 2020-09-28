using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

using EditorCore;

namespace PhoenixEditor
{
   public partial class WorkTopicControl : UserControl
   {
      public WorkTopicControl()
      {
         InitializeComponent();

         SourceActionButton.Enabled = false;

         mStatusIcon.mImageList = imageList1;

         this.Controls.Add(mStatusIcon);
         mStatusIcon.Top = this.TopicNameLabel.Top;
         mStatusIcon.Left = TopicNameLabel.Left - 20;
         mStatusIcon.Width = 16;
         mStatusIcon.Height = 16;

         this.RevertButton.Visible = false;
         this.SourceActionButton.Visible = false;

      }
      EnumViewer<WorkTopic.eWorkTopicFileState> mStatusIcon = new EnumViewer<WorkTopic.eWorkTopicFileState>();

      bool mbPaused = false;


      WorkTopic mTopic;
     
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public WorkTopic Topic
      {
         get
         {
            return mTopic;
         }
         set
         {
            mTopic = value;
            UpdateUI();
         }
      }

      bool mbDoNotSaveOption = true;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public bool DoNotSaveOption
      {
         get
         {
            return mbDoNotSaveOption;
         }
         set
         {
            mbDoNotSaveOption = value;
            this.DontSaveCheckBox.Visible = value;            
         }
      }

      bool mbOverwriteOption = true;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public bool OverwriteOption
      {
         get
         {
            return mbOverwriteOption;
         }
         set
         {
            mbOverwriteOption = value;
            this.OverwriteButton.Visible = value;
         }
      }

      bool mbCheckInOption = true;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public bool CheckInOption
      {
         get
         {
            return mbCheckInOption;
         }
         set
         {
            mbCheckInOption = value;
            this.CheckInButton.Visible = value;
         }
      }

      string GetTopicAlias(string input)
      {
         //Alias hack
         if (input == "Sim")
            return "DesignObjects";
         else if (input == "Sounds")
            return "SoundObjects";
         return input;
      }


      public void UpdateUI()
      {
         mbPaused = true;
         if (mTopic == null)
         {
            TopicNameLabel.Text = "error";
            return;
         }

         this.TopicNameLabel.Text = GetTopicAlias(mTopic.Name);






         this.TopicDescriptionLabel.Text = mTopic.Description;
         this.FileDescriptionLabel.Text = mTopic.FileDescription;

         this.StatusLabel.Text = mTopic.mState.ToString();
         mStatusIcon.Value = mTopic.mState;

         this.DontSaveCheckBox.Visible = false;
         this.CheckInButton.Visible = false;
         this.CheckInButton.Enabled = false;
         this.SourceActionButton.Visible = false;
         this.RevertButton.Visible = false;
         this.OverwriteButton.Visible = false;
         this.WriteStatusLabel.Visible = false;
         this.OutOfDateLabel.Visible = false;




         if (mbDoNotSaveOption &&
            (mTopic.mState == WorkTopic.eWorkTopicFileState.cLocalReadOnly
          || mTopic.mState == WorkTopic.eWorkTopicFileState.cCheckedOutByOther
          || mTopic.mState == WorkTopic.eWorkTopicFileState.cCheckedIn))
         {
            this.DontSaveCheckBox.Visible = true;
            this.DontSaveCheckBox.Checked = mTopic.DoNotSave;
            this.DontSaveCheckBox.AutoSize = true;

            if (mbOverwriteOption && CoreGlobals.getScenarioWorkTopicManager().sAllowOverwrite)
               this.OverwriteButton.Visible = true;

         }
         else  
         {
            mTopic.DoNotSave = false;
         }

         string message = "";
         string owner = mTopic.mOwner;//"asdf";

         //if (mTopic.Writeable == true && CoreGlobals.getScenarioWorkTopicManager().sAllowOverwrite
         //&& mTopic.mState == WorkTopic.eWorkTopicFileState.cCheckedIn && mTopic.mVersionStatus == WorkTopic.eVersionStatus.cCurrent)
         //{
         //   this.SourceActionButton.Text = "Check Out";
         //   this.SourceActionButton.Visible = true;
         //   this.SourceActionButton.Enabled = true;
         //   message = "checked in (+Writeable)";
         //}
         //else 

         if (mTopic.Writeable == true && mbOverwriteOption
         && (mTopic.mState != WorkTopic.eWorkTopicFileState.cCheckedOutByUser))//CoreGlobals.getScenarioWorkTopicManager().sAllowOverwrite)
         {
            this.WriteStatusLabel.Visible = true;
         }


         if (mTopic.mState == WorkTopic.eWorkTopicFileState.cCheckedIn  && mTopic.mVersionStatus == WorkTopic.eVersionStatus.cCurrent)
         {
            this.SourceActionButton.Text = "Check Out";
            this.SourceActionButton.Visible = true;
            this.SourceActionButton.Enabled = true;
            message = "checked in";
            
         }
         else if (mTopic.mState != WorkTopic.eWorkTopicFileState.cCheckedIn && mTopic.mVersionStatus == WorkTopic.eVersionStatus.cOutofdate)
         {
            this.SourceActionButton.Text = "Out of date";
            this.SourceActionButton.Visible = true;
            this.SourceActionButton.Enabled = false;
            this.CheckInButton.Enabled = false;

            message = "Opened files out of date";

            this.OutOfDateLabel.Visible = true;

            if (mbOverwriteOption && CoreGlobals.getScenarioWorkTopicManager().sAllowOverwrite)
               this.OverwriteButton.Visible = true;


         }
         else if (mTopic.mState == WorkTopic.eWorkTopicFileState.cCheckedIn && mTopic.mVersionStatus == WorkTopic.eVersionStatus.cOutofdate)
         {
            this.SourceActionButton.Text = "Check Out";
            this.SourceActionButton.Visible = true;
            this.SourceActionButton.Enabled = false;
            message = "checked in (out of date)";

            this.OutOfDateLabel.Visible = true;

         }
         else if (mTopic.mState == WorkTopic.eWorkTopicFileState.cCheckedOutByOther)
         {
            this.SourceActionButton.Text = "";
            message = "checked out by " + owner;
         }
         else if (mTopic.mState == WorkTopic.eWorkTopicFileState.cCheckedOutByUser)
         {
            if (mbCheckInOption)
            {
               this.CheckInButton.Visible = true;
               this.CheckInButton.Enabled = true;

               this.RevertButton.Visible = true;


            }

            message = "checked out by you";
         }
         else if (mTopic.mState == WorkTopic.eWorkTopicFileState.cLocalReadOnly)
         {
            message = "not in perfoce, Read Only";
         }
         else if (mTopic.mState == WorkTopic.eWorkTopicFileState.cLocalWriteable)
         {
            message = "not in perforce, Writeable";
         }
         else if (mTopic.mState == WorkTopic.eWorkTopicFileState.cNoFiles)
         {
            message = "no files yet.";
         }

         this.StatusLabel.Text = message;

         mbPaused = false;
      }

      public event EventHandler StateChanged = null;

      private void CheckOutButton_Click(object sender, EventArgs e)
      {
         Cursor.Current = Cursors.WaitCursor;
                                   
         CoreGlobals.getScenarioWorkTopicManager().AddOrEdit(mTopic.Name);
         
         mTopic.UpdateState();         

         UpdateUI();

         if (!mbPaused && StateChanged != null)
         {
            StateChanged.Invoke(this, null);
         }

         Cursor.Current = Cursors.Default;
      }
      private void CheckInButton_Click(object sender, EventArgs e)
      {
         using (PerfSection p10 = new PerfSection("Perforce - Check in scenario component: " + mTopic.Name))
         {
            Cursor.Current = Cursors.WaitCursor;

            PerforceChangeList newlist = CoreGlobals.getPerforce().GetNewChangeList("Scenario Topic: " + GetTopicAlias(mTopic.Name) + "   Scenario: " + Path.GetFileNameWithoutExtension(CoreGlobals.ScenarioFile));
            foreach (string file in mTopic.Files)
            {
               if (newlist.ReOpenFile(file) == false)
               {
                  CoreGlobals.ShowMessage("Error re-opening: " + file);
                  return;
               }
            }

            mTopic.mbPauseFileCheck = true;

            bool updateMemoryVersion = false;
            if (newlist.Submitchanges() == false)
            {
               CoreGlobals.ShowMessage("Error checking in files");
            }
            using (PerfSection p1 = new PerfSection("Clean list"))
            {
               //CoreGlobals.getPerforce().CleanEmptyChangeLists("Scenario Topic: ");
               if (CoreGlobals.getPerforce().HasFilesOpen(newlist.ID) == false)
               {
                  CoreGlobals.getPerforce().getConnection().P4DeleteList(newlist.ID);
                  updateMemoryVersion = true;  
               }
            }

            ///////Double check that this is good

            mTopic.UpdateState(updateMemoryVersion);
            UpdateUI();

            if (!mbPaused && StateChanged != null)
            {
               StateChanged.Invoke(this, null);
            }

            Cursor.Current = Cursors.Default;

            mTopic.mbPauseFileCheck = false;

         }
      }
      private void DontSaveCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         mTopic.DoNotSave = DontSaveCheckBox.Checked;

         if (!mbPaused && StateChanged != null)
         {
            StateChanged.Invoke(this, null);
         }
      }

      private void WorkTopicControl_Load(object sender, EventArgs e)
      {

      }

      private void OverwriteButton_Click(object sender, EventArgs e)
      {
         mTopic.SetFilesWriteable();

         mTopic.UpdateState(true);
         UpdateUI();

         if (!mbPaused && StateChanged != null)
         {
            StateChanged.Invoke(this, null);
         }

      }

      private void RevertButton_Click(object sender, EventArgs e)
      {
         using (PerfSection p10 = new PerfSection("Perforce - Check in scenario component: " + mTopic.Name))
         {

            Cursor.Current = Cursors.WaitCursor;

            PerforceChangeList newlist = CoreGlobals.getPerforce().GetNewChangeList("Scenario Topic: " + GetTopicAlias(mTopic.Name) + "   Scenario: " + Path.GetFileNameWithoutExtension(CoreGlobals.ScenarioFile));
            foreach (string file in mTopic.Files)
            {
               if (newlist.ReOpenFile(file) == false)
               {
                  CoreGlobals.ShowMessage("Error re-opening: " + file);
                  return;
               }
            }
            if (newlist.Revert() == false)
            {
               CoreGlobals.ShowMessage("Error reverting in files");

            }

            mTopic.UpdateState();
            UpdateUI();

            if (!mbPaused && StateChanged != null)
            {
               StateChanged.Invoke(this, null);
            }
            using (PerfSection p1 = new PerfSection("Clean list"))
            {
               //CoreGlobals.getPerforce().CleanEmptyChangeLists("Scenario Topic: ");
               if (CoreGlobals.getPerforce().HasFilesOpen(newlist.ID) == false)
               {
                  CoreGlobals.getPerforce().getConnection().P4DeleteList(newlist.ID);
               }
            }
            Cursor.Current = Cursors.Default;
         }
      }


   }


}
