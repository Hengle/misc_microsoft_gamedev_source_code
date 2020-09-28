using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

using Xceed.Grid;
using Xceed.Grid.Viewers;

using EditorCore;

namespace PhoenixEditor
{
   public partial class ScenarioSourceControl : UserControl
   {
      public ScenarioSourceControl()
      {
         InitializeComponent();
      }

      List<string> mFiles = new List<string>();

      public void SetFileList(List<string> files)
      {
         mFiles = files;
         
         mChangeList = null;
         //this.FileslistBox.Items.Add(

         ResetUI();


         UpdateUIStatus();
      }
      List<SimpleFileStatus> mStatus = new List<SimpleFileStatus>();
      private void UpdateUIStatus()
      {
         int numInPerforce = 0;
         int numCheckedoutByOther = 0;
         int numCheckedOutByUser = 0;
         int numCheckedOutByBoth = 0;

         mStatus.Clear();


         if (mFiles.Count == 0)
            return;


         foreach (string file in mFiles)
         {
            SimpleFileStatus s = CoreGlobals.getPerforce().GetFileStatusSimple(file);
            mStatus.Add(s);

            if (s.InPerforce)
               numInPerforce++;
            if (s.State == eFileState.CheckedOutByUserAndOther)
               numCheckedOutByBoth++;
            else if(s.State == eFileState.CheckedOutByOther)
               numCheckedoutByOther++;
            else if (s.State == eFileState.CheckedOutByUser)
               numCheckedOutByUser++;           
         }

         FileListGridControl.DataSource = mStatus;


         EnumViewer<eFileState> item = new EnumViewer<eFileState>();
         item.mImageList = imageList1;
         CellViewerManager manager = new CellViewerManager(item, "Value");
         FileListGridControl.Columns["State"].CellViewerManager = manager;


         if (numInPerforce < mFiles.Count && numInPerforce > 0)
         {
            mMode = ePerforcePageMode.SomeFilesInPerforce;
            AddOptions();
         }
         else if(numInPerforce == 0)
         {
            mMode = ePerforcePageMode.NoFilesInPerforce;
            AddOptions();
         }
         else if(numCheckedoutByOther > 0)
         {
            mMode = ePerforcePageMode.SomeCheckedOutByOther;
            CheckedOutByOtherOptions();
         }
         else if (numCheckedoutByOther == mFiles.Count)
         {
            mMode = ePerforcePageMode.AllCheckedOutByOther;
            CheckedOutByOtherOptions();
         }
         else if (numCheckedOutByUser == mFiles.Count)
         {
            mMode = ePerforcePageMode.AllCheckedOutByUser;

            CheckedOutByUserOptions();
         }
         else if ((numInPerforce == mFiles.Count) && (numCheckedOutByUser > 0) && (numCheckedoutByOther == 0))
         {
            mMode = ePerforcePageMode.SomeCheckedOutByUser;
            AllCheckedInOptions();
         }
         else if ((numInPerforce == mFiles.Count) && (numCheckedoutByOther == 0))
         {
            mMode = ePerforcePageMode.AllCheckedIn;
            AllCheckedInOptions();
         }
         else
         {
            mMode = ePerforcePageMode.OtherState;
            //Not set up to handle this situation
            ResetUI();
         }
         
      }
      public ePerforcePageMode mMode = ePerforcePageMode.NotSet;

      //[FlagsAttribute]
      public enum ePerforcePageMode
      {
         NotSet,
         NoFilesInPerforce,
         SomeFilesInPerforce,
         AllCheckedIn,
         SomeCheckedOutByOther,
         AllCheckedOutByOther,
         SomeCheckedOutByUser,
         AllCheckedOutByUser,
         OtherState

      }

      private void AddOptions()
      {
         ResetUI();
         AddButton.Enabled = true;
      }
      private void AllCheckedInOptions()
      {
         ResetUI();
         CheckoutButton.Enabled = true;
      }
      private void CheckedOutByUserOptions()
      {
         ResetUI();
         SubmitButton.Enabled = true;
         CancelChangesButton.Enabled = true;
         DescriptionTextBox.Enabled = true;

         //create new changelist if in default changelist

         //else, get owing changelist
         
         if ((PerforceStatusChangedEvent != null)  &&  (mFiles.Count != 0))
            PerforceStatusChangedEvent(mFiles[0]);
      }

      private void CheckedOutByOtherOptions()
      {
         ResetUI(); 
         //todo
         //show message in red that other user has files.
         //

      }

      private void ResetUI() 
      {
         this.DescriptionTextBox.Text = "";

         AddButton.Enabled = false;
         SubmitButton.Enabled = false;
         CancelChangesButton.Enabled = false;
         CheckoutButton.Enabled = false;

         DescriptionTextBox.Enabled = false;
      }


      private void AddButton_Click(object sender, EventArgs e)
      {
         if (mFiles.Count == 0) return;
         if (mChangeList == null)
            mChangeList = CoreGlobals.getPerforce().GetNewChangeList(GetDescription());

         foreach(string file in mFiles)
         {
            mChangeList.AddOrEdit(file,true);
         }

         UpdateUIStatus();
         
      }

      private void CheckoutButton_Click(object sender, EventArgs e)
      {
         if (mFiles.Count == 0) return;
         if (mChangeList == null)
         {
            mChangeList = CoreGlobals.getPerforce().GetNewChangeList("Automated action: " + Path.GetFileNameWithoutExtension(mFiles[0]));
         }

         foreach (string file in mFiles)
         {
            mChangeList.AddOrEdit(file,true);
         }

         UpdateUIStatus();

      }

      public bool ValidateFiles()
      {
         bool result = true;

         result &= ValidateFilesExist();

         return result;


      }

      public bool ValidateFilesExist()
      {
         List<string> mMissingFiles = new List<string>();
         foreach (string file in mFiles)
         {
            if (File.Exists(file) == false)
            {
               mMissingFiles.Add(file);
               //return false;
            }
         }
         if(mMissingFiles.Count > 0)
         {
            MissingFiles(mMissingFiles);
            return false;
         }

         return true;
      }


      public bool SubmitFiles()
      {
         if (ValidateFiles() == false)
         {

            return false;
         }

         if (mChangeList == null)
         {
            mChangeList = CoreGlobals.getPerforce().GetNewChangeList(GetDescription());
         }
         else
         {
            mChangeList.Description = GetDescription();//wtf??? why does this remove files...?
         }

         foreach (string file in mFiles)
         {
            mChangeList.AddOrEdit(file, true);
         }

         mChangeList.Submitchanges();

         UpdateUIStatus();

         return true;
      }



      private void SubmitButton_Click(object sender, EventArgs e)
      {
         SubmitFiles();
      }
      public string GetDescription()
      {
         return "Automated action: " + Path.GetFileNameWithoutExtension(mFiles[0]) + "\r\n " + DescriptionTextBox.Text;
      }

      private void CancelChangesButton_Click(object sender, EventArgs e)
      {
         if (mChangeList == null)
         {
            //we could try to use and existing change list, but that could get tricky

            //if (mStatus.Count == 0)
            //   return;
            //int id = mStatus[0].UserChangeListNumber;
            //if(id == PerforceConnection.cDefaultChangeList)
            //{
            //   mChangeList = CoreGlobals.getPerforce().GetNewChangeList("Automated action: " + Path.GetFileNameWithoutExtension(mFiles[0]));
            //}
            //else
            //{
            //   mChangeList = CoreGlobals.getPerforce().GetExistingChangeList("Automated action: " + Path.GetFileNameWithoutExtension(mFiles[0]));
            //}

            mChangeList = CoreGlobals.getPerforce().GetNewChangeList(GetDescription());

         }

         foreach (string file in mFiles)
         {
            mChangeList.AddOrEdit(file, true);
         }  

         mChangeList.RemoveListAndRevert();
         mChangeList = null;

         UpdateUIStatus();
      }

      PerforceChangeList mChangeList = null;

 
      public delegate void PerforceStatusChanged(string fileName);
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      public event PerforceStatusChanged PerforceStatusChangedEvent = null;

      public delegate void FileEvent(List<string> fileNames);
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never) ]
      public event FileEvent MissingFiles = null;

   }
       

   




}
