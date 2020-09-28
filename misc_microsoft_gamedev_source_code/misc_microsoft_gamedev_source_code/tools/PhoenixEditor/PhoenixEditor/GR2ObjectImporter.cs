using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.IO;
using System.Text.RegularExpressions;
using System.Net.Mail;

using EditorCore;
using SimEditor;
using ModelSystem;


namespace PhoenixEditor
{
   public partial class GR2ObjectImporter : Form
   {
      public GR2ObjectImporter(string filename)
      {
         InitializeComponent();

         try
         {
            if (CoreGlobals.UsingPerforce)
            {
               CoreGlobals.getPerforce().CleanEmptyChangeLists("Auto import gr2: ");
            }

         }
         catch (System.Exception ex)
         {

         }




         bool badGranny = false;

         //ObjectNameTextBox.Enabled = false;
         ObjectNameTextBox.ReadOnly = true;

         SimpleFileStatus status =  CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(CoreGlobals.getWorkPaths().GetProtoObjectFile());
         if(status.CheckedOutThisUser == true)
         {
            ShowError("Please check in objects.xml and try again",false);
            badGranny = true;
         }

         if (Path.GetExtension(filename).ToLower() == ".gr2")
         {
            mImportType = eImportType.GR2;
         }
         else if (Path.GetExtension(filename).ToLower() == ".pfx")
         {
            mImportType = eImportType.Effect;
         }
         else if (Path.GetExtension(filename).ToLower() == ".lgt")
         {
            mImportType = eImportType.Lightset;
         }
         else
         {
            badGranny = true;            
         }

         if (mImportType == eImportType.GR2)
         {


            //Process Mesh
            try
            {
               //if (filename.Contains(@"work\art\effects"))
               //{
               //   mImportSubMode = eImportSubMode.EffectLikeObject;
               //}               
               mGrannyMesh = ModelSystem.GrannyManager.getOrLoadGR2(filename, true);
            }
            catch (System.Exception ex)
            {
               ShowError(ex.ToString(), false);
               badGranny = true;
            }
            //if (mGrannyMesh.GetNumPrimitives() == 0)
            //{
            //   ShowError("No models loaded for: " + filename, false);
            //   badGranny = true;
            //}
         }


         //Load Proto objects
         try
         {
            loadProtoXML();
            if(SetupForImport(filename) == false)
            {
               badGranny = true;
            }
 
         }
         catch(System.Exception ex)
         {
            ShowError( ex.ToString(), false );
            badGranny = true;
         }

         if (badGranny)
         {
            this.ImportButton.Enabled = false;
            
         }
      }
      
      public void ShowError(string text, bool bAdvancedError)
      {
         ErrorListBox.Items.Add(text);
         if (bAdvancedError == false)
         {
            BasicErrorListBox.Items.Add(text);
         }
      }
      

      BRenderGrannyMesh mGrannyMesh = new BRenderGrannyMesh();

      XmlDocument mProtoObjectsDoc = null;
      //public int mHightestID = 0;
      Dictionary<string, bool> mObjectNames = new Dictionary<string, bool>();
      Dictionary<string, UnitHolder> mTemplates = new Dictionary<string, UnitHolder>();

      PerforceChangeList mChangeList = null;

      PerforceChangeList mPostCheckoutList = null;

      public void loadProtoXML()
      {
         string fileName = CoreGlobals.getWorkPaths().GetProtoObjectFile();

         mProtoObjectsDoc = new XmlDocument();
         //mProtoObjectsDoc.PreserveWhitespace = true;
         mProtoObjectsDoc.Load(fileName);

         mObjectNames.Clear();
         TemplateListBox.Items.Clear();
         mTemplates.Clear();

         XmlNodeList units = mProtoObjectsDoc.GetElementsByTagName("Object");
         foreach (XmlNode unit in units)
         {
            XmlAttribute nameAttr = (XmlAttribute)unit.Attributes.GetNamedItem("name");
            if (nameAttr == null)
               continue;
            mObjectNames[nameAttr.Value.ToLower()] = true;

            if (!nameAttr.Value.Contains("template_"))
            {
               continue;
            }

            XmlNode animFile = ((XmlElement)unit).GetElementsByTagName("Visual")[0];

            if (animFile != null)
            {
               string animFileString = animFile.InnerText;
               UnitHolder holder = new UnitHolder((string)nameAttr.Value, unit);
               this.TemplateListBox.Items.Add(holder);
               mTemplates[holder.mName] = holder;
              
            }
         }

         this.TemplateListBox.SelectedIndex = 0;
      }

      class UnitHolder
      {
         public XmlNode mNode = null;
         public string mName = "";
         public UnitHolder(string name, XmlNode node)
         {
            mNode = node;
            mName = name;
         }
         public override string ToString()
         {
            return mName;
         }
      }

      string mNewGR2Path = "";
      string mObjectName = "";
      string mObjectPrefix = "";
      string mAnimUnitFile = "";

      public bool UsingPerforce
      {
         get
         {
            return this.AddToPerforceCheckBox.CheckState == CheckState.Checked;
         }
      }

      enum eImportType 
      {
         GR2,
         Effect,
         Lightset,
         Invalid
      };
      enum eImportSubMode
      {
         none,
         EffectLikeObject
      }

      eImportType mImportType = eImportType.Invalid;
      eImportSubMode mImportSubMode = eImportSubMode.none;

      public bool SetupForImport(string gr2Path)
      {
         mNewGR2Path = gr2Path;
         mObjectName = Path.GetFileNameWithoutExtension(mNewGR2Path);

         if( this.GetArtPrefixFromPath(mNewGR2Path, true, out mObjectPrefix, true) == false)
         {
            return false;

         }


         this.PathTextBox.Text = mNewGR2Path;
         this.ObjectNameTextBox.Text = mObjectPrefix + mObjectName;

         string workPath = CoreGlobals.getWorkPaths().mGameDirectory;

         mAnimUnitFile = Path.ChangeExtension(mNewGR2Path, CoreGlobals.getWorkPaths().mUnitDataExtension);
         SetupFilePickerOptions(mAnimUnitFile, this.AnimUnitFileTextBox, AnimNewRadioButton, AnimExistingRadioButton);       
         
         if(UsingPerforce)
         {
            ManditoryChangeListBox.Items.Add("objects.xml");

            //ManditoryChangeListBox.Items.Add(Path.GetFileNameWithoutExtension(mNewGR2Path) + ".xml");


            ConsiderResourceForSourceControl(mNewGR2Path,true);

            string importMessage = "Auto Import";

            if (mImportType == eImportType.GR2)
            {
               //This section is a feature requested by artists that the importer search for files to check in that may or may not exists and it may or may not matter.

               importMessage = "Auto import gr2: ";

               ConsiderResourceForSourceControl(Path.ChangeExtension(mNewGR2Path, "max"), true);
               ConsiderResourceForSourceControl(Path.ChangeExtension(mNewGR2Path, "ugx"), true);
               //Artists requested to not search for wrk files anymore
               //ConsiderResourceForSourceControl(Path.ChangeExtension(mNewGR2Path, "wrk"), true);
               foreach (string file in mGrannyMesh.mAllFilenames)
               {
                  ResourcePathInfo pathinfo = new ResourcePathInfo(file);

                  string attemptedFile = file;
                  
                  //Try to fix art relative paths...
                  if(pathinfo.IsRelativePath && !pathinfo.IsWorkRelativePath)
                  {
                     attemptedFile = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, pathinfo.RelativePath);
                     pathinfo = new ResourcePathInfo(attemptedFile);
                     if (!File.Exists(attemptedFile))
                     {
                        pathinfo = new ResourcePathInfo(file);
                     }
                  }
                  if (pathinfo.IsTexture && pathinfo.IsWorkRelativePath)
                  {
                     ConsiderResourceForSourceControl(Path.Combine(workPath, pathinfo.RelativePathNoExt + ".tga"), true);
                     ConsiderResourceForSourceControl(Path.Combine(workPath, pathinfo.RelativePathNoExt + ".ddx"), true);
                     ConsiderResourceForSourceControl(Path.Combine(workPath, pathinfo.RelativePathNoExt + ".psd"), true);
                  }
                  else 
                  {
                     ConsiderResourceForSourceControl(pathinfo.Value, true);
                  }
               }            
            }
            else if (mImportType == eImportType.Effect)
            {
               importMessage = "Auto import Effect: ";

            }
            else if (mImportType == eImportType.Lightset)
            {
               importMessage = "Auto import Lightset: ";
            }

            mChangeList = CoreGlobals.getPerforce().GetNewChangeList(importMessage + mNewGR2Path);

            if(mChangeList == null)
            {
               MessageBox.Show("Error connecting to perforce.  This import will not work be added to source control.");

               //this.ImportButton.Enabled = false;

               this.AddToPerforceCheckBox.CheckState = CheckState.Unchecked;
            }

            if (ValidatePerforceFileList(true) == false)
               OtherAddLabel.ForeColor = Color.Red;
            else
               OtherAddLabel.ForeColor = Color.Black;

            SetBasicUI();

         }
         return true;
      }

      public void ConsiderResourceForSourceControl(string filename, bool verifyFileExists)
      {
         SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(filename);
         if (status.InPerforce == true)
         {
            this.InPerforceListBox.Items.Add(filename);
         }
         else
         {
            ResourcePathInfo pathInfo = new ResourcePathInfo(filename);
            if (pathInfo.IsRelativePath == false )
            {
               AddFileToList(filename, "File not under work root.",@" Non local path (not under work\):" + Path.GetFileName(filename));
            }
            else if (verifyFileExists && File.Exists(filename) == false)
            {
               AddFileToList(filename,"File not found.", " Missing:" + Path.GetFileName(filename));
            }
            else
            {
               AddFileToList(filename, "", "");
            }
         }
      }

      bool mbUseIgnoreButtons = true; //warning don't set this to false yet
      public void AddFileToList(string filename, string errorType, string errorMessage)
      {
         if (errorMessage != "" || errorType != "")
         {
            int index = this.ToAddListBox.Items.Add(filename, CheckState.Indeterminate);
            ShowError(errorMessage,true);

            if (mbUseIgnoreButtons)
            {
               mFileResolvers.Add(new FileErrorResolver(this, filename, errorType));
            }
         }
         else
         {
            this.ToAddListBox.Items.Add(filename, CheckState.Checked);
         }

      }

      public bool SetFileStatus(string filename, CheckState state)
      {        
         for(int i=0; i< ToAddListBox.Items.Count;i++)
         {
            object item = ToAddListBox.Items[i];
            if(item.ToString() == filename)
            {
               ToAddListBox.SetItemCheckState(i, state);
               return true;
            }
         }
         return false;
      }

      List<FileErrorResolver> mFileResolvers = new List<FileErrorResolver>();
      public class FileErrorResolver
      {
         public string mFileName;
         public string mStatusMessage;
         public bool mbIgnore = false;

         List<Control> mControls = new List<Control>();
         Label mFileLabel;
         Label mStatusLabel;
         Button mIgnoreButton;
         GR2ObjectImporter mParent;
         public FileErrorResolver(GR2ObjectImporter parent, string filename, string statusMessage)
         {
            mParent = parent;
            mFileName = filename;
            mStatusMessage = statusMessage;

            mFileLabel = new Label();
            mFileLabel.Text = filename;

            mFileLabel.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            mFileLabel.Font = new Font(mFileLabel.Font, (mbIgnore)?FontStyle.Strikeout:FontStyle.Regular);
            mControls.Add(mFileLabel);

            mStatusLabel = new Label();
            mStatusLabel.Text = statusMessage;
 
            mStatusLabel.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            mStatusLabel.ForeColor = Color.Red;
            mControls.Add(mStatusLabel);

            mIgnoreButton = new Button();
            mIgnoreButton.Text = "Ignore";
            mIgnoreButton.Click += new EventHandler(mIgnoreButton_Click);
            mControls.Add(mIgnoreButton);
         }

         void mIgnoreButton_Click(object sender, EventArgs e)
         {
            mbIgnore = !mbIgnore;
            mFileLabel.Font = new Font(mFileLabel.Font, (mbIgnore) ? FontStyle.Strikeout : FontStyle.Regular);

            mParent.SetFileStatus(mFileName, (mbIgnore) ? CheckState.Unchecked : CheckState.Checked);
         }

         public List<Control> GetUI()
         {

            return mControls;
         }
         public override string ToString()
         {
            return mFileName;
         }
      }

      public void SetBasicUI()
      {
         this.tableLayoutPanel1.Controls.Clear();
         this.tableLayoutPanel1.RowCount = mFileResolvers.Count;
         int row = 0;
         foreach(FileErrorResolver f in mFileResolvers)
         {
            tableLayoutPanel1.Controls.Add(f.GetUI()[0], 0, row);
            tableLayoutPanel1.Controls.Add(f.GetUI()[1], 1, row);
            tableLayoutPanel1.Controls.Add(f.GetUI()[2], 2, row);
            
            row ++;
         }
      }

      private void ImportButton_Click(object sender, EventArgs e)
      {
         if (TemplateListBox.SelectedItem == null)
         {
            //MessageBox.Show("error 39 Abort or OK the Yes Again?", "error 45", MessageBoxButtons.YesNoCancel);
            MessageBox.Show("Please pick a template type");
            return;
         }

         UnitHolder templateObject = TemplateListBox.SelectedItem as UnitHolder;
         string templateName = templateObject.mName;
         int selectedindex = TemplateListBox.SelectedIndex; 

         if (this.UsingPerforce)
         {
            if(mChangeList != null)
            {
               if (mChangeList.SyncFile(CoreGlobals.getWorkPaths().GetProtoObjectFile()) == false)
               {
                  MessageBox.Show("Perforce error: " + mChangeList.GetLastError());
                  return;
               } 
            }
            loadProtoXML();
            TemplateListBox.SelectedIndex = selectedindex;
         }


         if (UsingPerforce && ValidatePerforceFileList(false) == false)
         {
            return;
         }

         if (ValidateName(this.ObjectNameTextBox.Text, false) == false && mbUpdateAssetOnly == false)
         {

            return;
         }
         if (ValidateUnitDataFile(mAnimUnitFile, false) == false)
         {
            return;
         }
   


         //get latest db from perforce
         if (this.UsingPerforce)
         {
            if(mChangeList.EditFile(CoreGlobals.getWorkPaths().GetProtoObjectFile()) == false)
            {
               MessageBox.Show("Perforce error: " + mChangeList.GetLastError());
               return;
            }

            loadProtoXML();
            TemplateListBox.SelectedIndex = selectedindex;

         }
         //validate again that the template is still there
         if (mTemplates.ContainsKey(templateName) && mTemplates[templateName] != null)
         {
            templateObject = mTemplates[templateName];
         }
         else 
         {
            MessageBox.Show("Error in template object");
            return;
         }
         
         mObjectName = this.ObjectNameTextBox.Text;
         if (CreateNewObject(templateObject.mNode, mObjectName, mNewGR2Path, mbUpdateAssetOnly))
         {
            if (this.UsingPerforce)
            {
               if (UsingPerforce && DontCheckInCheckBox.Checked == true)
               {
                  if (DontCheckInCheckBox.Checked == false)
                     mChangeList.Revert();
               }
               else
               {
                  DoCheckin();
                  SendNotifyEmail(this.ObjectNameTextBox.Text);
               }
            }
                        
            DialogResult = DialogResult.OK;
            this.Close();
         }
         else
         {
            if (this.UsingPerforce)
            {
               mChangeList.Revert();
            }
         }

         try
         {
            if (CoreGlobals.getEditorMain() != null && CoreGlobals.getEditorMain().mPhoenixScenarioEditor != null)
            {
               CoreGlobals.getEditorMain().mPhoenixScenarioEditor.ReloadVisibleObjects();
            }
         }
         catch(System.Exception ex) 
         {
            MessageBox.Show("Import complete, but there was an error reloading the new data.  Please restart the editor.");
            ex.ToString();
         }
      }

      public void SendNotifyEmail(String name)
      {
         try
         {
            MailMessage oMsg = new MailMessage();
            oMsg.From = new MailAddress("PhoenixEditor@ensemblestudios.com", System.Environment.UserName);
            oMsg.To.Add("cvandoren@ensemblestudios.com");
            oMsg.To.Add("swinsett@ensemblestudios.com");
            
            oMsg.Subject = String.Concat("Asset added.");
            oMsg.IsBodyHtml = true;

            oMsg.Body = "An art asset has been added to the game.<br>";

            oMsg.Body += "<b>Machine:</b> " + System.Environment.MachineName + "<br>";
            oMsg.Body += "<b>User:</b> " + System.Environment.UserName + "<br>";
            oMsg.Body += "<b>Asset:</b> " + name + "<br>";


            oMsg.Priority = MailPriority.Normal;



            // TODO: Replace with the name of your remote SMTP server.
            SmtpClient client = new SmtpClient("ensexch.ENS.Ensemble-Studios.com");
            client.UseDefaultCredentials = true;
            client.Send(oMsg);

            oMsg = null;
         }
         catch (Exception e)
         {
            // Andrew prefers I do nothing here.
            //MessageBox.Show(String.Concat(e, " Exception caught."));
         }
      }

      public void DoCheckin()
      {
         System.Threading.Thread.Sleep(2000);

         foreach(object item in ToAddListBox.CheckedItems)
         {
            mChangeList.AddFile(item.ToString());  
         }

         //mChangeList.AddFile(mAnimUnitFile + ".xmb");
         //mChangeList.AddFile(CoreGlobals.getWorkPaths().GetProtoObjectFile() + ".xmb");

         if (DontCheckInCheckBox.Checked == false)
         {
            mChangeList.Submitchanges();

            if (CheckOutAfterSubmitCheckBox.Checked == true)
            {
               mPostCheckoutList = CoreGlobals.getPerforce().GetNewChangeList("Auto import gr2: " + mNewGR2Path);

               foreach (object item in ToAddListBox.CheckedItems)
               {
                  mPostCheckoutList.EditFile(item.ToString());
               }
               foreach (object item in this.InPerforceListBox.Items)
               {
                  mPostCheckoutList.EditFile(item.ToString());
               }
               mPostCheckoutList.EditFile(mAnimUnitFile);
            }
         }   
      }

      public bool ValidatePerforceFileList(bool bNoPopups)
      {
         return ValidatePerforceFileList(bNoPopups, false);
      }

      /// <summary>
      /// Verify that check decisions are resolved
      /// </summary>
      /// <param name="bNoPopups">Hide dialog warnings</param>
      /// <param name="bIgnoreOneError">Skip one error.  Needed only for lag in checkbox list</param>
      /// <returns></returns>
      public bool ValidatePerforceFileList(bool bNoPopups, bool bIgnoreOneError)
      {
         if (mbUseIgnoreButtons == false)
         {
            return true;
         }

         for (int i = 0; i < ToAddListBox.Items.Count; i++ )
         {
            if(ToAddListBox.GetItemCheckState(i) == CheckState.Indeterminate)
            {
               if (bIgnoreOneError)
               {
                  bIgnoreOneError = false;
               }
               else
               {
                  if (!bNoPopups)
                     MessageBox.Show("Please resolve check in list.");
                  return false;

               }
            }
         }
         return true;
      }

      private void CancelImportButton_Click(object sender, EventArgs e)
      {
         CleanUp();
         DialogResult = DialogResult.Cancel;
         this.Close();
      }

      private void CleanUp()
      {
         if (UsingPerforce)
         {
            if (DontCheckInCheckBox.Checked == false)
            {
               if (mChangeList != null)
                  mChangeList.RemoveListAndRevert();
            }
         }
      }

      bool mbUpdateAssetOnly = false;
      //Regex validName = new Regex(@"^[\w]*$");
      Regex validName = new Regex(@"^(?<maindir>[0-9a-zA-Z]+)_(?<subdir>[0-9a-zA-Z]+)_(?<name>[0-9a-zA-Z]+(_(?<id>[0-9]*[1-9]+[0]*))?)$");
      Regex validName4Dir = new Regex(@"^(?<maindir>[0-9a-zA-Z]+)_(?<middir>[0-9a-zA-Z]+)_(?<subdir>[0-9a-zA-Z]+)_(?<name>[0-9a-zA-Z]+(_(?<id>[0-9]*[1-9]+[0]*))?)$");
      //Regex validName =      new Regex(@"^(?<maindir>[0-9a-zA-Z]+)(_(?<subdir>[0-9a-zA-Z]+))?(_(?<subdir>[0-9a-zA-Z]+))_(?<name>[0-9a-zA-Z]+(_(?<id>[0-9]*[1-9]+[0]*))?)$");
      Regex validNameEffect = new Regex(@"^(?<maindir>[0-9a-zA-Z]+)_(?<subdir1>[0-9a-zA-Z]+)(_(?<subdir2>[0-9a-zA-Z]+))?_(?<name>[0-9a-zA-Z]+(_(?<id>[0-9]*[1-9]+[0]*))?)$");
      Regex validTestName = new Regex(@"^test_([0-9a-zA-Z_]+)$");
      public bool ValidateName(string protoName, bool bNoPopups)
      {
         if( mObjectNames.ContainsKey(protoName.ToLower()) == true)
         {
            if (!bNoPopups)
            {
             //  MessageBox.Show("Unit name: " + protoName + " already in use.");
               if(MessageBox.Show("Update art target only?" ,protoName + " is already in the database.", MessageBoxButtons.OKCancel) == DialogResult.OK)
               {
                  mbUpdateAssetOnly = true;

               }

            }
            return false;
         }

         if (mConstraintModel == ConstraintModel.Production)
         {
            if (mImportType == eImportType.GR2 && (mImportSubMode != eImportSubMode.EffectLikeObject))
            {
               Match nameScan = validName.Match(protoName);
               Match nameScan4Directories = validName4Dir.Match(protoName);

               // if 3 directories does not match try 4.
               if (nameScan.Success == false)
               {
                  nameScan = nameScan4Directories;
               }

               if (nameScan.Success == false)
               {
                  if (!bNoPopups)
                     MessageBox.Show("Invalid unit name.  See help button for folders and naming conventions.  Example: warthog_01 ");
                  return false;
               }
               else
               {
                  string subshortname = "";

                  bool nameOk = true;
                  if (nameScan.Groups["maindir"].Value != mForcedMainDir)
                  {
                     if (!bNoPopups)
                        MessageBox.Show("Directory should be " + mForcedMainDir + "  See help button for folders and naming conventions.");
                     nameOk =  false;
                  }
                  if (nameScan.Groups["subdir"].Value != mForcedSubDir )
                  {
                     string shortName = "";
                     if (mShortNames.ContainsKey(mForcedSubDir))
                     {
                        shortName = mShortNames[mForcedSubDir];
                        if (nameScan.Groups["subdir"].Value != shortName)
                        {
                           if (!bNoPopups)
                              MessageBox.Show("Sub Directory should be " + mForcedSubDir + "  See help button for folders and naming conventions.");
                        }         
                     }
                     else
                     {
                        if (!bNoPopups)
                           MessageBox.Show("Sub Directory should be " + mForcedSubDir + "  See help button for folders and naming conventions.");
                        nameOk = false;
                     }
                  }
                  if (nameScan.Groups["name"].Value != mForcedName)
                  {
                     if (!bNoPopups)
                        MessageBox.Show("Name should be " + mForcedName + "  See help button for folders and naming conventions.");
                     nameOk = false;
                  }
                  if(nameOk == false)
                  {
                     return false;
                  }
               }
            }
            else if (mImportType == eImportType.Effect || mImportType == eImportType.Lightset  )
            {
               Match nameScan = validNameEffect.Match(protoName);

               if (nameScan.Success == false)
               {
                  if (!bNoPopups)
                     MessageBox.Show("Invalid unit name.  Example: warthog_01 ");
                  return false;
               }
               else
               {
                  if (nameScan.Groups["maindir"].Value != mForcedMainDir)
                  {
                     if (!bNoPopups)
                        MessageBox.Show("Directory should be " + mForcedMainDir + "  See help button for folders and naming conventions.");
                     return false;
                  }  
               }
            }
            else if ((mImportSubMode == eImportSubMode.EffectLikeObject))
            {
               return true;
            }

         }
         else if (mConstraintModel == ConstraintModel.Test)
         {
            Match nameScan = validTestName.Match(protoName);
            if(nameScan.Success == false)
            {
               if (!bNoPopups)
                  MessageBox.Show("Test objects must start with test_");
               return false;
            }

         }
         return true;
      }
      public bool ValidateUnitDataFile(string file, bool bNoPopups)
      {
         ResourcePathInfo path = new ResourcePathInfo(file);

         if (!path.IsFilePath)
         {
            if (!bNoPopups)
               MessageBox.Show(path.Value + " is invalid.");
            return false;
         }
         else if(!path.IsWorkRelativePath)
         {
            if (!bNoPopups)
               MessageBox.Show("UnitData file must be under //work/");
            return false;
         }
         else if (Path.GetExtension(path.Value) != CoreGlobals.getWorkPaths().mUnitDataExtension)
         {
            if (!bNoPopups)
               MessageBox.Show("UnitData file must be of ." + CoreGlobals.getWorkPaths().mUnitDataExtension);
            return false;
         }
         return true;
      }

      public bool CreateNewObject(XmlNode templateObject, string objectName, string gr2Path, bool updateAssetOnly)
      {
         try
         {
            string gr2Directory = Path.GetDirectoryName(gr2Path);
            string artpath = CoreGlobals.getWorkPaths().mGameArtDirectory + "\\";
            //string animFileName = Path.Combine(gr2Directory.Substring(artpath.Length) , Path.GetFileNameWithoutExtension(gr2Path) + ".xml");
            string animFileRelativePath = mAnimUnitFile.Substring(artpath.Length);

            string gr2FileRelativePath = gr2Path.Substring(artpath.Length);
            gr2FileRelativePath = gr2FileRelativePath.Replace(".gr2", "");


            //BuildNode
            XmlNode unit = null;
            if (updateAssetOnly == false)
            {
               unit = templateObject.Clone();

               //mProtoObjectsDoc.CreateAttribute()
               XmlAttribute nameAttr = (XmlAttribute)unit.Attributes.GetNamedItem("name");
               nameAttr.Value = objectName;

               //We don't want or use "id"...
               //((XmlElement)unit).SetAttribute("id", id.ToString());
               ((XmlElement)unit).SetAttribute("dbid", ""); //this will be updated by the id tool

            }
            else  //update and existing asset
            {
               //.SelectNodes("//asset[@type='Model']/file");
               XmlNodeList nodes = mProtoObjectsDoc.SelectNodes("//Object[@name='" + objectName +"']");
               if (nodes.Count == 1)
               {
                  //string newGr2File = gr2file.Replace(".g", "");
                  unit = nodes[0];
               }
               else
               {
                  return false;
               }
            }

            XmlNode animFile = ((XmlElement)unit).GetElementsByTagName("Visual")[0];

            if (AnimExistingRadioButton.Checked)
            {     
               //do nothing.
            }
            else
            {
               bool useExisting = false;
               if (File.Exists(mAnimUnitFile))
               {
                  if (MessageBox.Show("A .vis file already exists for this asset. Would you like to use it?", "Warning", MessageBoxButtons.YesNo) == DialogResult.Yes)
                  {
                     useExisting = true;
                  }
                  else
                  {
                     try
                     {
                        bool res = mChangeList.AddOrEdit(mAnimUnitFile);
                        string lasterror = mChangeList.GetLastError();
                        File.SetAttributes(mAnimUnitFile, FileAttributes.Normal);
                        File.Delete(mAnimUnitFile);
                     }
                     catch (System.Exception ex)
                     {
                        MessageBox.Show("Error deleting vis file.  You may need to check it out first.");
                        return false;
                     }
                  }
               }
               if (!useExisting)
               {
                  if (mImportType == eImportType.GR2)
                  {
                     CreateNewAnimFile(animFile.InnerText, animFileRelativePath, gr2FileRelativePath);
                  }
                  else if(mImportType == eImportType.Effect)
                  {
                     CreateNewEffectVisFile(animFile.InnerText, animFileRelativePath, gr2FileRelativePath);
                  }
                  else if(mImportType == eImportType.Lightset)
                  {
                     CreateNewLightVisFile(animFile.InnerText, animFileRelativePath, gr2FileRelativePath);
                  }
               }
            }
            if (this.UsingPerforce)
            {
               mChangeList.AddOrEdit(mAnimUnitFile + ".xmb");
            }

            System.Threading.Thread.Sleep(1000);

            EditorCore.XMBProcessor.CreateXMB(mAnimUnitFile, false);

            if (UsingPerforce)
            {
               if(mChangeList.AddOrEdit(mAnimUnitFile) == false)
               {
                  MessageBox.Show("Perforce error: " + mChangeList.GetLastError());
                  return false;
               }
            }

            animFile.InnerText = animFileRelativePath;

            //Add new object to File

            if (updateAssetOnly == false)
            {               
               XmlNode protoNode = mProtoObjectsDoc.GetElementsByTagName("Objects")[0];
               protoNode.AppendChild(unit);
            }
            try
            {

               XmlWriterSettings settings = new XmlWriterSettings();
               settings.IndentChars = "   ";
               settings.CloseOutput = true;
               settings.Indent = true;
               settings.OmitXmlDeclaration = true;
               XmlWriter w = XmlWriter.Create(CoreGlobals.getWorkPaths().GetProtoObjectFile(), settings);
               mProtoObjectsDoc.Save(w);
               w.Close();

               if (this.UsingPerforce)
               {
                  mChangeList.AddOrEdit(CoreGlobals.getWorkPaths().GetProtoObjectFile() + ".xmb");
               }
               DBIDHelperTool.Run();

               EditorCore.XMBProcessor.CreateXMB(CoreGlobals.getWorkPaths().GetProtoObjectFile(), false);

            }
            catch(UnauthorizedAccessException unEx)
            {
               unEx.ToString();
               //MessageBox.Show("Can't write to objects.xml");
               if(MessageBox.Show("objects.xml is readonly.  Overwrite?","error", MessageBoxButtons.OKCancel) == DialogResult.OK)
               {
                  try
                  {
                     //set writable and try 
                     File.SetAttributes(CoreGlobals.getWorkPaths().GetProtoObjectFile(), FileAttributes.Normal);
                     File.Delete(CoreGlobals.getWorkPaths().GetProtoObjectFile());

                     XmlWriterSettings settings = new XmlWriterSettings();
                     settings.IndentChars = "   ";
                     settings.CloseOutput = true;
                     settings.Indent = true;
                     settings.OmitXmlDeclaration = true;
                     XmlWriter w = XmlWriter.Create(CoreGlobals.getWorkPaths().GetProtoObjectFile(), settings);                      
                     mProtoObjectsDoc.Save(w);
                     w.Close();

                     if (this.UsingPerforce)
                     {
                        mChangeList.AddFile(CoreGlobals.getWorkPaths().GetProtoObjectFile() + ".xmb");
                     }
                     System.Threading.Thread.Sleep(1000);
                     DBIDHelperTool.Run();

                     EditorCore.XMBProcessor.CreateXMB(CoreGlobals.getWorkPaths().GetProtoObjectFile(), false);

                     return true;
                  }
                  catch (UnauthorizedAccessException unEx2)
                  {
                     MessageBox.Show("Error writing to objects.xml.");
                  }

               }
               return false;
            }

            return true;
            
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
            return false;
         }
      }
      public void CreateNewAnimFile(string templateAnimFile, string newAnimFile, string gr2file)
      {
         XmlDocument animTemplateDoc = new XmlDocument();
         animTemplateDoc.Load(CoreGlobals.getWorkPaths().mGameArtDirectory + "\\template\\object" + CoreGlobals.getWorkPaths().mUnitDataExtension);

         //XmlNodeList nodes = animTemplateDoc.SelectNodes("//assetreference[@type='GrannyModel']/file");
         XmlNodeList nodes = animTemplateDoc.SelectNodes("//asset[@type='Model']/file");

         if (nodes.Count > 0)
         {
            //string newGr2File = gr2file.Replace(".g", "");
            nodes[0].InnerText = gr2file;
         }
         string newFullFileName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, newAnimFile);
         animTemplateDoc.Save(newFullFileName);



      }

      public void CreateNewAnimFileReal(string templateAnimFile, string newAnimFile)
      {
         XmlDocument mAnimTemplateDoc = new XmlDocument();
         mAnimTemplateDoc.Load(templateAnimFile);

         //build list of stuff to match up...  //dropdowns.. errors.  

      }

//<?xml version="1.0"?>
//<visual xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" defaultmodel="Default">
//  <model name="Default">
//    <component>
//      <asset type="Model">
//        <file>effects\fx_dummy</file>
//      </asset>
//      <attach type="ParticleFile" name="effects\environment\harvest\lavaflareup_01" tobone="bone_main" syncanims="false" disregardorient="false" />
//    </component>
//  </model>
//</visual>

      //public void CreateNewEffectVisFile(string templateAnimFile, string newAnimFile, string gr2file)
      //{
      //   XmlDocument animTemplateDoc = new XmlDocument();
      //   animTemplateDoc.Load(CoreGlobals.getWorkPaths().mGameArtDirectory + "\\template\\effect" + CoreGlobals.getWorkPaths().mUnitDataExtension);

      //   XmlNodeList nodes = animTemplateDoc.SelectNodes("//attach[@type='ParticleFile']");

      //   if (nodes.Count > 0)
      //   {
      //      string noext = gr2file.Replace(".pfx", "");
      //      nodes[0].Attributes["name"].InnerText = noext;
      //   }
      //   string newFullFileName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, newAnimFile);
      //   animTemplateDoc.Save(newFullFileName);
      //}

//<?xml version="1.0"?>
//<visual xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" defaultmodel="Default">
//  <model name="Default">
//    <component>
//      <asset type="Particle">
//        <file>effects\test</file>
//      </asset>
//    </component>
//  </model>
//</visual>
      public void CreateNewEffectVisFile(string templateAnimFile, string newAnimFile, string gr2file)
      {
         XmlDocument animTemplateDoc = new XmlDocument();
         animTemplateDoc.Load(CoreGlobals.getWorkPaths().mGameArtDirectory + "\\template\\effect" + CoreGlobals.getWorkPaths().mUnitDataExtension);

         XmlNodeList nodes = animTemplateDoc.SelectNodes("//asset[@type='Particle']/file");

         if (nodes.Count > 0)
         {
            nodes[0].InnerText = gr2file.Replace(".pfx", ""); ;
         }
         string newFullFileName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, newAnimFile);
         animTemplateDoc.Save(newFullFileName);


      }
//<?xml version="1.0"?>
//<visual xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" defaultmodel="Default">
//  <model name="Default">
//    <component>
//      <asset type="Light">
//        <file>unsc\vehicle\warthog_01\warthog_01</file>
//      </asset>
//    </component>
//  </model>
//</visual>      
      
      public void CreateNewLightVisFile(string templateAnimFile, string newAnimFile, string gr2file)
      {
         XmlDocument animTemplateDoc = new XmlDocument();
         animTemplateDoc.Load(CoreGlobals.getWorkPaths().mGameArtDirectory + "\\template\\light" + CoreGlobals.getWorkPaths().mUnitDataExtension);

         XmlNodeList nodes = animTemplateDoc.SelectNodes("//asset[@type='Light']/file");

         if (nodes.Count > 0)
         {
            nodes[0].InnerText = gr2file.Replace(".lgt", "").Replace(".LGT",""); 
         }
         string newFullFileName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, newAnimFile);
         animTemplateDoc.Save(newFullFileName);


      }




      private void ObjectNameTextBox_TextChanged(object sender, EventArgs e)
      {
         if (ValidateName(ObjectNameTextBox.Text, true) == false)
         {
            //ObjectNameTextBox.ForeColor = Color.Red;
            ObjectNameTextBox.BackColor = Color.Red;
         }
         else
         {
            //ObjectNameTextBox.ForeColor = Color.Black;
            ObjectNameTextBox.BackColor = Color.White;

         }

      }
  
      public Dictionary<string, string> LoadShortNames()
      {
         Dictionary<string, string> shortNames = new Dictionary<string, string>();

         string[] lines = File.ReadAllLines(Path.Combine(CoreGlobals.getWorkPaths().mEditorSettings, "ArtDirectoryShortNames.txt"));
         foreach(string line in lines)
         {
            string[] pair = line.Split(' ');
            if(pair.Length == 2)
            {
               shortNames[pair[0].ToLower()] = pair[1];
            }
         }
         return shortNames;
      }

      //public string GetArtPrefixFromPath(string path, bool letUserAddAliases)
      //{
      //   string prefix = "";
         
      //   string directory = Path.GetDirectoryName(path);
      //   directory = Path.GetDirectoryName(directory);  //peel off top dir
      //   string artRoot = CoreGlobals.getWorkPaths().mGameArtDirectory + "\\";
      //   string relativePath = directory.Substring(artRoot.Length);  //peel off lower dirs
      //   string[] folders = relativePath.Split('\\');

      //   Dictionary<string, string> shortNames = LoadShortNames();
      //   Dictionary<string, string> missingNames = new Dictionary<string,string>();
      //   string missingNamesString = "";
      //   foreach(string folderName in folders)
      //   {
      //      string shortName = folderName;

      //      if (shortNames.ContainsKey(folderName))
      //      {
      //         shortName = shortNames[folderName];
      //      }
      //      else if (letUserAddAliases)
      //      {
      //         missingNames[folderName] = folderName;
      //         missingNamesString += (folderName + "  ");
      //      }

      //      prefix += (shortName + "_");

      //   }

      //   if (missingNames.Count > 0)
      //   {
      //      //MessageBox.Show("Folder alias(es) missing for: " + missingNamesString + " ", "Error generating name!");
      //   }
      //   return prefix;
      //}

      string mForcedMainDir = "";
      string mForcedSubDir = "";
      string mForcedName = "";
      string mForcedID = "";
      ConstraintModel mConstraintModel = ConstraintModel.Production;

      enum ConstraintModel
      {
         Production,
         Test      
      };

      Dictionary<string, string> mShortNames = null;
      public bool GetArtPrefixFromPath(string path, bool letUserAddAliases, out string prefix, bool bSetDirectories)
      {
         bool bCheckDirectoryCount = true;
         int requiredDirctoryCountMin = 3;
         int requiredDirctoryCountMax = 3;
         bool bCheckShortNames = true;
         bool bForcedLastSubFolderName = true;

         //now gr2s get 4..
         if (mImportType == eImportType.GR2 || mImportType == eImportType.Effect || mImportType == eImportType.Lightset || (mImportSubMode == eImportSubMode.EffectLikeObject))
         {
            bForcedLastSubFolderName = false;
            requiredDirctoryCountMax = 4;  //was 4
         }


         bool bSuccess = false;
         prefix = "";

         string directory = Path.GetDirectoryName(path);
         //directory = Path.GetDirectoryName(directory);  //peel off top dir
         string artRoot = CoreGlobals.getWorkPaths().mGameArtDirectory + "\\";
         string relativePath = directory.Substring(artRoot.Length);  //peel off lower dirs
         string[] folders = relativePath.Split('\\');

         mShortNames = LoadShortNames();
         Dictionary<string, string> missingNames = new Dictionary<string, string>();
         string missingNamesString = "";

         int nameIndex = folders.Length - 1;
         int forcedDirIndex = nameIndex - 1;

         if(folders.Length > 0 && folders[0] == "test")
         {
            mConstraintModel = ConstraintModel.Test;
            mForcedMainDir = "test";
            //ObjectNameTextBox.Enabled = true;
            ObjectNameTextBox.ReadOnly = false;
            bSuccess =  true;
         }
         else if (bCheckDirectoryCount && (folders.Length < requiredDirctoryCountMin || folders.Length > requiredDirctoryCountMax))
         {
            MessageBox.Show(this, "Must be 3 folders deep in the art directory. ex: 'resource\\relics\\node_01'. The path " + relativePath + " is " + folders.Length + " folders deep.", "Incorrect folder depth.");
            bSuccess = false;
         }
         else if (bCheckShortNames && mShortNames.ContainsKey(folders[0].ToLower()) == false)
         {
            MessageBox.Show(this, "This importer can only import files from a restricted set of directories.  Primary game objects will be added by the design department.", "Locked Directory");
            bSuccess = false;
         }
         else
         {
            if (bSetDirectories)
            {
               mForcedMainDir = mShortNames[folders[0]];
               mForcedSubDir = folders[forcedDirIndex];

               //if (shortNames.ContainsKey(mForcedSubDir))
               //{
               //   mForcedSubDir = shortNames[mForcedSubDir];
               //}


               mForcedName = folders[nameIndex];
               bSuccess = true;
            }
            else if (bForcedLastSubFolderName && mForcedName != folders[nameIndex])
            {
               bSuccess = false;
            }
            else
            {
               bSuccess = true;
            }

         }
         //string fileName = Path.GetFileNameWithoutExtension(path);
         //string[] filenameParts = fileName.Split('_');
         //if (filenameParts.Length == 1)
         //{
         //}
         //else if (filenameParts.Length == 2)
         //{
         //}
         //else
         //{
         //   MessageBox.Show(this, "Invalid name", "Invalid Name");
         //   return false;
         //}

         //foreach (string folderName in folders)

         int foldersToUseInName = folders.Length - 1;

         if (mImportType == eImportType.Effect || mImportType == eImportType.Lightset || (mImportSubMode == eImportSubMode.EffectLikeObject))
         {
            foldersToUseInName = folders.Length;
         }
         if (mImportType == eImportType.GR2)
         {
            foldersToUseInName = folders.Length - 1;
         }

         for (int i = 0; i < foldersToUseInName; i++)
         {
            string shortName = folders[i];// folderName;
            string folderName = folders[i];
            if (mShortNames.ContainsKey(folderName))
            {
               shortName = mShortNames[folderName];
            }
            else if (letUserAddAliases)
            {
               missingNames[folderName] = folderName;
               missingNamesString += (folderName + "  ");
            }

            prefix += (shortName + "_");

         }

         if (missingNames.Count > 0)
         {
            //MessageBox.Show("Folder alias(es) missing for: " + missingNamesString + " ", "Error generating name!");
         }
         return bSuccess;
      }


      public bool mbWarningHasBeenDisplayed = false;
      private void ToAddListBox_ItemCheck(object sender, ItemCheckEventArgs e)
      {
         bool ignoreOneError = false;
         string fileName = ToAddListBox.Items[e.Index].ToString();
         //bool isRelativePath = CoreGlobals.mWorkTextureFileEX.Match(fileName).Groups["relativeFilePath"].Success;
         ResourcePathInfo pathinfo = new ResourcePathInfo(fileName);
         bool isRelativePath = pathinfo.IsRelativePath;
         if (e.CurrentValue == CheckState.Indeterminate)
         {

            if (mbWarningHasBeenDisplayed)
            {
               e.NewValue = CheckState.Unchecked;
               ignoreOneError = true;
              
            }
            else if (File.Exists(fileName) == false && MessageBox.Show("This file may be needed.  Are you sure you want to leave it out?", "Missing File!", MessageBoxButtons.YesNo) == DialogResult.Yes)
            //else if (File.Exists(fileName) == false && MessageBox.Show("File does not exist.  Are you sure you want to leave this file out?", "Missing File!", MessageBoxButtons.YesNo) == DialogResult.Yes)
            {
               e.NewValue = CheckState.Unchecked;
               ignoreOneError = true;
            }
            //else if (isRelativePath == false && MessageBox.Show(@"File does not have a relative path to work\  Are you sure you want to leave this file out?", "Bad File Path", MessageBoxButtons.YesNo) == DialogResult.Yes)
            else if (isRelativePath == false && MessageBox.Show(@"This file may be needed.  Are you sure you want to leave it out?", "Bad File Path (not under work directory)", MessageBoxButtons.YesNo) == DialogResult.Yes)
            {
               e.NewValue = CheckState.Unchecked;
               ignoreOneError = true;
            }
            else
            {
               e.NewValue = CheckState.Indeterminate;
     
            }
            mbWarningHasBeenDisplayed = true;
           
         }
         else if (e.CurrentValue == CheckState.Unchecked && File.Exists(ToAddListBox.Items[e.Index].ToString()) == false)
         {
            e.NewValue = CheckState.Indeterminate;
         }
         else if (e.CurrentValue == CheckState.Unchecked && isRelativePath == false)
         {
            e.NewValue = CheckState.Indeterminate;
         }

         ToAddListBox.Update();
         if (ValidatePerforceFileList(true, ignoreOneError) == false)
            OtherAddLabel.ForeColor = Color.Red;
         else
            OtherAddLabel.ForeColor = Color.Black;


         if(e.NewValue == CheckState.Indeterminate) // stupid checked list boxes dont update....
            OtherAddLabel.ForeColor = Color.Red;

      }

      private void AddToPerforceCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         if(AddToPerforceCheckBox.CheckState == CheckState.Checked)
         {
            foreach(Control c in DetailsTabPage.Controls)
            {
               c.Enabled = true;
            }
            this.tableLayoutPanel1.Enabled = true;
         }
         else
         {
            foreach (Control c in DetailsTabPage.Controls)
            {
               c.Enabled = false;
            }
            this.tableLayoutPanel1.Enabled = false;
         }


      }

      private void GR2ObjectImporter_FormClosing(object sender, FormClosingEventArgs e)
      {
         CleanUp();

      }

      private void OnAnimPerforceRadio(object sender, EventArgs e)
      {
         AnimExistingRadioButton.Checked = false;
         AnimNewRadioButton.Checked = false;
         
         RadioButton b = sender as RadioButton;
         if (b != null)
            b.Checked = true;
      }

      private void PickAnimFileButton_Click(object sender, EventArgs e)
      {
         SaveFileDialog d = new SaveFileDialog();
         d.InitialDirectory = Path.GetDirectoryName(mNewGR2Path);
         d.Filter = "Unitdata files (*" + CoreGlobals.getWorkPaths().mUnitDataExtension + ")|*" + CoreGlobals.getWorkPaths().mUnitDataExtension;
         d.FilterIndex = 0;
         if(d.ShowDialog() == DialogResult.OK)
         {
            mAnimUnitFile = d.FileName;
            SetupFilePickerOptions(mAnimUnitFile, this.AnimUnitFileTextBox, AnimNewRadioButton, AnimExistingRadioButton);

         }    
      }
      private void SetupFilePickerOptions(string fileName, TextBox fileTextBox, RadioButton newOrOverWrite, RadioButton useExisting)
      {
         SetupFilePickerOptions(fileName, fileTextBox, newOrOverWrite, useExisting, false);

      }
      private void SetupFilePickerOptions(string fileName, TextBox fileTextBox, RadioButton newOrOverWrite, RadioButton useExisting, bool bDontSetText)
      {
         if (!bDontSetText)
            fileTextBox.Text = fileName;

         if(File.Exists(fileName))
         {
            newOrOverWrite.Text = "Overwrite";
            newOrOverWrite.Checked = true;
            newOrOverWrite.ForeColor = Color.Green;
            useExisting.Enabled = true;
         }
         else
         {
            newOrOverWrite.ForeColor = Color.Black;

            newOrOverWrite.Text = "Create New";
            useExisting.Enabled = false;
            newOrOverWrite.Checked = true;
         } 

         //extra checking...
         if (this.GetArtPrefixFromPath(fileName, true, out mObjectPrefix, false) == true)
         {
            mObjectName = Path.GetFileNameWithoutExtension(fileName);

            //return false;
            this.ObjectNameTextBox.Text = mObjectPrefix + mObjectName;
         }
         else
         {
            this.ObjectNameTextBox.Text = "invalid";
            //ObjectNameTextBox.BackColor = Color.Red;
         }


      }

      private void AnimUnitFileTextBox_TextChanged(object sender, EventArgs e)
      {
         if(ValidateUnitDataFile(AnimUnitFileTextBox.Text,true) == false)
         {
            AnimUnitFileTextBox.ForeColor = Color.Red;
         }
         else
         {
            AnimUnitFileTextBox.ForeColor = Color.Black;
         }
         mAnimUnitFile = AnimUnitFileTextBox.Text;
         
         SetupFilePickerOptions(mAnimUnitFile, this.AnimUnitFileTextBox, AnimNewRadioButton, AnimExistingRadioButton, true);
      }

      private void AdvancedErrorsButton_Click(object sender, EventArgs e)
      {
         this.tabControl1.SelectedIndex = 1;
      }

      private void BasicRetryButton_Click(object sender, EventArgs e)
      {

      }

      private void Helpbutton1_Click(object sender, EventArgs e)
      {
         try
         {
            string text = File.ReadAllText(CoreGlobals.getWorkPaths().mEditorSettings + "\\GR2Help.txt");

            MessageBox.Show(text);
         }
         catch (System.Exception ex)
         {
            ex.ToString();
         }
      }



#if false
      private void AddViaAnimButton_Click(object sender, EventArgs e)
      {
         try
         {
            string filename = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "xobjects.xml");

            if (File.GetAttributes(filename) == FileAttributes.ReadOnly)
            {
               MessageBox.Show(filename + " is readonly.  please check it out");
            }

            if (NodeComboBox.SelectedItem == null)
               return;
            UnitHolder copyholder = NodeComboBox.SelectedItem as UnitHolder;
            if (copyholder == null)
               return;


            OpenFileDialog d = new OpenFileDialog();
            d.Filter = "AnimXML (*.xml)|*.xml";
            d.FilterIndex = 0;
            d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameArtDirectory;

            if (d.ShowDialog() == DialogResult.OK)
            {
               //Validate xml
               List<string> models = SimUnitXML.getGrannyFileNames(d.FileName);
               if (models.Count == 0)
               {
                  if (MessageBox.Show(String.Format("No models found in {0} add anyway?", d.FileName), "", MessageBoxButtons.YesNo) == DialogResult.Yes)
                  {

                  }
                  else
                  {
                     return;
                  }

               }

               string name = Path.GetFileNameWithoutExtension(d.FileName);

               //check for duplicates
               foreach (UnitHolder holder in this.NodeComboBox.Items)
               {
                  if (holder.mName == name)
                  {
                     MessageBox.Show(name + " has already been added to xobjects");
                     return;
                  }
               }

               mHightestID++;
               int id = mHightestID;

               string artpath = CoreGlobals.getWorkPaths().mGameArtDirectory + "\\";
               string animFileName = d.FileName.Substring(artpath.Length);

               //BuildNode
               XmlNode unit = copyholder.mNode.Clone();
               XmlAttribute nameAttr = (XmlAttribute)unit.Attributes.GetNamedItem("name");
               nameAttr.Value = name;
               XmlAttribute idAttr = (XmlAttribute)unit.Attributes.GetNamedItem("id");
               idAttr.Value = id.ToString();
               XmlNode animFile = ((XmlElement)unit).GetElementsByTagName("Visual")[0];
               animFile.InnerText = animFileName;

               //Add to File
               XmlNode protoNode = mProtoObjectsDoc.GetElementsByTagName("Proto")[0];

               protoNode.AppendChild(unit);
               mProtoObjectsDoc.Save(filename);
               CoreGlobals.getSaveLoadPaths().mGameArtDirectory = Path.GetDirectoryName(d.FileName);

               listBox1.Items.Add(String.Format("Added: {0}", name));

               this.NodeComboBox.Items.Add(new UnitHolder(name, unit));

               foreach (string modelname in models)
               {

                  listBox1.Items.Add(String.Format("    Model: {0}", modelname));
               }

            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }
#endif
   }
}
