using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using EditorCore;
using System.Threading;
using System.Diagnostics;
using Terrain;
using Rendering;
using SimEditor;
using ModelSystem;
using Export360;
namespace PhoenixEditor
{
   public partial class BatchExporter : Form
   {
      public int mStatusResult = 0; 

      public BatchExporter()
      {
         InitializeComponent();
      }
      
      
      static BatchExporter mThis = null;
      
      public bool mainInit(String[] args)
      {
         bool bCommandLineOnly;
         return mainInit(args, out bCommandLineOnly);        
      }
      public bool mainInit(String[] args, out bool bCommandLineOnly)
      {
         bCommandLineOnly = true;
         mWindowless = true;
         try
         {
            //throw new System.Exception("Test batch exception handling");
            mThis = this;
            if (args != null)
               parseArgsForExportParams(args);

            //if the user has specified files for us to export
            //then don't pop up our window. Instead, just export and close us.
            if (mScenarioFilesToExport.Count != 0)
            {
               CoreGlobals.ConsoleMode = true;
               if (mDoAoSection)
                  exportAOFileList();
               else if (mBuildAoSections)
                  buildAOSections();
               else
                  exportFromCommandLine();

               return true;
            }
            else
            {
               bCommandLineOnly = false;
               mWindowless = false;
               Init();
            }
          
         }
         catch (System.Exception ex)
         {
            OnException(ex);
         }
         return false;
      }


      bool mWindowless = false;
      bool mUsingFileList = false;
      bool mCheckoutFromPerforce = false;
      bool mCheckinWhenFinished = false;
      bool mIgnorePerforce = false;
      int mUseCustomExportSettings = -1;     //-1 = scenario defined , 0 = custom, 1 = quick, 2 = final
      string outputLog = null;
      string outputDir = null;

      bool mDoAoSection = false;
      bool mBuildAoSections = false;
      int mAoSectionIndex = 0;
      int mAoNumSections = 1;

      int argStringContains(string[] args, String searchString)
      {
         for (int i = 0; i < args.Length; i++)
         {
            if (args[i] == searchString)
               return i;
         }
         return -1;
      }
      void parseArgsForExportParams(String[] args)
      {


         if (argStringContains(args, "-file") != -1)
         {
            int index = argStringContains(args, "-file");
            index++;
            String fileName = args[index];
            if (fileName[0] == '\"')
               fileName = fileName.Substring(1, fileName.Length - 2);
            if (File.Exists(fileName))
            {
               addFile(fileName);
            }
         }

         //if (argStringContains(args, "-fileList") != -1)
         //{
         //   int index = argStringContains(args, "-fileList");
         //   index++;
         //   String fileName = args[index];
         //   if (File.Exists(fileName))
         //   {
         //      mUsingFileList = true;
         //   }
         //   else
         //   {
         //      MessageBox.Show("Specified file list " + fileName + " Not Found.. Opening batch exportation window.");
         //      this.Visible = true;
         //      this.Show();
         //      mWindowless = false;
         //      mUsingFileList = false;
         //   }
         //}

         if (argStringContains(args, "-logfile") != -1)
         {
            int index = argStringContains(args, "-logfile");
            index++;
            outputLog = args[index];
         }

         if (argStringContains(args, "-outdir") != -1)
         {
            int index = argStringContains(args, "-outdir");
            index++;
            outputDir = args[index];
         }


         if (argStringContains(args, "-skipXTD") != -1) doXTD.Checked = false;
         if (argStringContains(args, "-skipXTT") != -1) doXTT.Checked = false;
         if (argStringContains(args, "-skipXTH") != -1) doXTH.Checked = false;
         if (argStringContains(args, "-skipXSD") != -1) doXSD.Checked = false;
         if (argStringContains(args, "-skipDEP") != -1) doDEP.Checked = false;
         if (argStringContains(args, "-skipTAG") != -1) doTAG.Checked = false;
         if (argStringContains(args, "-skipXMB") != -1) doXMB.Checked = false;
         if (argStringContains(args, "-skipLRP") != -1) doLRP.Checked = false;

         if (argStringContains(args, "-finalExport") != -1) mUseCustomExportSettings = 2;
         if (argStringContains(args, "-quickExport") != -1) mUseCustomExportSettings = 1;

         if (argStringContains(args, "-checkoutfiles") != -1) mCheckoutFromPerforce = true;
         if (argStringContains(args, "-checkinwhenfinished") != -1) mCheckinWhenFinished = true;
         if (argStringContains(args, "-ignoreperforce") != -1)
         {
            mIgnorePerforce = true;
            mCheckinWhenFinished = false;
            mCheckoutFromPerforce = false;
            CoreGlobals.UsingPerforce = false;
         }
         else
         {
            CoreGlobals.UsingPerforce = true;
         }


         //AO Section Generation
         if (argStringContains(args, "-doaosection") != -1) mDoAoSection = true;
         if (argStringContains(args, "-aosectionindex") != -1)
         {
            int index = argStringContains(args, "-aosectionindex");
            index++;
            mAoSectionIndex = System.Convert.ToInt32(args[index], 10);
         }
         if (argStringContains(args, "-aonumsections") != -1)
         {
            int index = argStringContains(args, "-aonumsections");
            index++;
            mAoNumSections = System.Convert.ToInt32(args[index], 10);
         }
         if (argStringContains(args, "-buildaosections") != -1) mBuildAoSections = true;
          
      }
     
      public void Init()
      {
         string[] files = Directory.GetFiles(CoreGlobals.getWorkPaths().mGameScenarioDirectory, "*.SCN", SearchOption.AllDirectories);
         for (int i = 0; i < files.Length; i++)
         {
            ToExportListBox.Items.Add(files[i]);
         }
            
         for (int i = 0; i < ToExportListBox.Items.Count; i++)
            ToExportListBox.SelectedIndices.Add(i);

         progressBar1.Maximum = ToExportListBox.Items.Count;
      }

      public void OnException(System.Exception ex)
      {
         try
         {
            //throw new System.Exception("test exceptions 2");
            if (mWindowless == false)
            {
               EditorCore.CoreGlobals.getErrorManager().OnException(ex);
            }
            else
            {
               this.outputMessage(ex.ToString());

            }
         }
         catch (System.Exception ex2)
         {
            Console.WriteLine(ex.ToString() + "---------------outer exception---------" + ex2.ToString());
         }

      }




      #region BATCH EXPORT MANAGEMENT
      class BatchExportScenarioFile
      {
         public string mFilename;
         public int mTerrainFileIndex;
      }
      class BatchExportTerrainFile
      {
         public string mFilename;
         public bool mHasBeenExported;
      }

      List<BatchExportScenarioFile> mScenarioFilesToExport = new List<BatchExportScenarioFile>();
      List<BatchExportTerrainFile> mTerrainFilesToExport = new List<BatchExportTerrainFile>();

      string giveLowestTEDPathInPath(string path)
      {
         //CLM This will recursivly search the path for a given .TED file
         // it will return the one closest to the origional path
         if (path == CoreGlobals.getWorkPaths().mGameScenarioDirectory)
         {
            return "";// WTF ""; //we didn't find any..
         }

         string[] files = Directory.GetFiles(path, "*.TED");
         if (files.Length == 0)
         {
            //we didn't find anything here, move up a level.
            path = path.Substring(0, path.LastIndexOf(@"\"));
            return giveLowestTEDPathInPath(path);
         }

         return path;
      }
      void addFile(string scenarioFileName)
      {
         string directory = Path.GetDirectoryName(scenarioFileName);
         string terrainFile = Path.ChangeExtension(Path.GetFileName(scenarioFileName), "TED");
         string terrainPath = giveLowestTEDPathInPath(directory);

         if (terrainPath == "")//we didn't find a ted above us, so we must be a unique export
         {
            terrainPath = directory;
         }
         else//We found another .TED Above us, ensure it's not one already in our root
         {
            string[] files = Directory.GetFiles(terrainPath, "*.TED");
            if (files.Length != 0)
               terrainFile = Path.GetFileName(files[0]);
         }
         terrainFile = Path.Combine(terrainPath, terrainFile);

         if (!scenarioListContains(scenarioFileName))
         {
            BatchExportScenarioFile bef = new BatchExportScenarioFile();
            bef.mFilename = scenarioFileName;
            
            int terrainIndex = terrainListContains(terrainFile);
            if (terrainIndex == -1)
            {
               BatchExportTerrainFile bef2 = new BatchExportTerrainFile();
               bef2.mHasBeenExported = false;
               bef2.mFilename = terrainFile;
               mTerrainFilesToExport.Add(bef2);
               terrainIndex = mTerrainFilesToExport.Count - 1;
            }

            bef.mTerrainFileIndex = terrainIndex;
            mScenarioFilesToExport.Add(bef);
         }
      }
      bool scenarioListContains(string scenarioFile)
      {
         for (int i = 0; i < mScenarioFilesToExport.Count; i++)
         {
            if (mScenarioFilesToExport[i].mFilename == scenarioFile)
               return true;
         }
         return false;
      }
      int terrainListContains(string terrainFile)
      {
         for (int i = 0; i < mTerrainFilesToExport.Count; i++)
         {
            if (mTerrainFilesToExport[i].mFilename == terrainFile)
               return i;
         }
         return -1;
      }
      void populateExportListFromListBox()
      {
         for (int i = 0; i < ToExportListBox.SelectedItems.Count; i++)
         {
            addFile(ToExportListBox.SelectedItems[i].ToString());
         }
      }

      bool hasBeenExported(int index)
      {
         return mTerrainFilesToExport[mScenarioFilesToExport[index].mTerrainFileIndex].mHasBeenExported;
      }
      void markScenarioExported(int index)
      {
         mTerrainFilesToExport[mScenarioFilesToExport[index].mTerrainFileIndex].mHasBeenExported = true;
      }

      void clearList()
      {
         mScenarioFilesToExport.Clear();
         mTerrainFilesToExport.Clear();
      }

      #endregion

      bool xtd, xth, xtt;
      void saveCheckOptions()
      {
           xtd=doXTD.Checked;
           xtt=doXTT.Checked;
           xth = doXTH.Checked;
      }
      void restoreCheckOptions()
      {
         doXTD.Checked = xtd;
         doXTT.Checked = xtt;
         doXTH.Checked = xth;
      }

      StreamWriter mExportLogStream = null;
      void outputMessage(string msg)
      {
         if (mExportLogStream != null)
         {
            mExportLogStream.WriteLine(msg);
         }
         if (mWindowless)
         {
            Console.WriteLine(msg);
         }
         else
         {
            listBox1.Items.Add(msg);
         }
      }


      //----------------------------
      private void exportFileList()
      {
         String batchExportLog = outputLog;
         if (batchExportLog == null)
            batchExportLog = Path.Combine(CoreGlobals.getWorkPaths().mLogsDirectory, "batchExportLog." + System.DateTime.Now.ToFileTimeUtc() + ".txt");
         mExportLogStream = new StreamWriter(batchExportLog, true);
         listBox1.Items.Clear();

         
         DateTime start = DateTime.Now;

         SimTerrainType.loadTerrainTypes();


         Exportbutton.Enabled = false;
         progressBar1.Value = 0;
         progressBar1.Maximum = mScenarioFilesToExport.Count;// ToExportListBox.SelectedItems.Count;



         //export settings
         Export360.ExportSettings settings = null;
         if (mUseCustomExportSettings == -1)
         {
            settings = null;
         }
         else if (mUseCustomExportSettings == 0)
         {
            ExportDialog mExportDialog = new ExportDialog();
            mExportDialog.mExportSettings = new Export360.ExportSettings();
            mExportDialog.mExportSettings.SettingsQuick();
            mExportDialog.mIsQuickExport = true;
            if (mExportDialog.ShowDialog() == DialogResult.OK)
               settings = mExportDialog.mExportSettings;
         }
         else if (mUseCustomExportSettings == 1)
         {
            settings = new Export360.ExportSettings();
            settings.SettingsQuick();
         }
         else if (mUseCustomExportSettings == 2)
         {
            settings = new Export360.ExportSettings();
            settings.SettingsFinal();
         }





         bool createOwnDevice = BRenderDevice.getDevice() == null;

         if (createOwnDevice)
            BRenderDevice.createDevice(this, 640, 480, false);

         GrannyManager2.init();

         saveCheckOptions();

         for (int fileCount = 0; fileCount < mScenarioFilesToExport.Count; fileCount++)// (string file in ToExportListBox.SelectedItems)
         {
            string scenarioName = mScenarioFilesToExport[fileCount].mFilename;
            string terrainName = mTerrainFilesToExport[mScenarioFilesToExport[fileCount].mTerrainFileIndex].mFilename;

            bool terrainAlreadyExported = hasBeenExported(fileCount);

            //if our owner terrain has already been exported, and we're not generating XSD files, bail.
            if (terrainAlreadyExported && !doXSD.Checked)
               continue;

            bool canEdit = mIgnorePerforce ? true : P4CanEdit(scenarioName, terrainName, mCheckoutFromPerforce);

            if (canEdit)
            {
               //toggle our check boxes for already exported terrain files
               doXTD.Checked &= !terrainAlreadyExported;
               doXTT.Checked &= !terrainAlreadyExported;
               doXTH.Checked &= !terrainAlreadyExported;

               bool okExported = TEDIO.TEDto360(terrainName, scenarioName,
                                          outputDir,
                                          settings,
                                          doXTD.Checked,
                                          doXTT.Checked,
                                          doXTH.Checked,
                                          doXSD.Checked,
                                          doLRP.Checked,
                                          doDEP.Checked,
                                          doTAG.Checked,
                                          doXMB.Checked);



               if (okExported)
               {
                  outputMessage(Path.GetFileNameWithoutExtension(scenarioName) + ": EXPORT SUCCEEDED!--------------------");

               }
               else
               {
                  mStatusResult = 2;
                  outputMessage(Path.GetFileNameWithoutExtension(scenarioName) + ": ABORTED! There was a problem exporting the files");
                  P4RevertAllNeededFiles(scenarioName, terrainName);
               }
            }

            progressBar1.Invoke(updateProgress);
            restoreCheckOptions();
            markScenarioExported(fileCount);
         }




         if (createOwnDevice)
            BRenderDevice.destroyDevice();

         if (!mIgnorePerforce && mCheckinWhenFinished)
         {
            outputMessage("Checking in files");
            P4SubmitChangelist();
         }


         Exportbutton.Enabled = true;

         TimeSpan ts = DateTime.Now - start;
         outputMessage("====Time : " + ts.TotalMinutes + " Minutes");
         outputMessage("====Export Finished====");

         //   GrannyManager2.deinit();

         mExportLogStream.Close();

         if (mStatusResult == 0)
            File.Delete(batchExportLog);

         clearList();

        
      }

      //----------------------------
      //----------------------------
      #region progressInc
      static public void ProgressInc()
      {
         mThis.progressBar1.Value++;
      }
      public delegate void _ProgressInc();
      static public _ProgressInc updateProgress = new _ProgressInc(ProgressInc);
      #endregion



      //----------------------------
      //----------------------------
      //----------------------------
      void exportFromCommandLine()
      {
         exportFileList();
      }

      //----------------------------
      //----------------------------
      void checkDependencies()
      {
         if (doLRP.Checked)
            doXSD.Checked = true;

         if (doTAG.Checked)
         {
            doXTT.Checked = true;
            doXTD.Checked = true;
            doXTH.Checked = true;
            doXSD.Checked = true;
         }

         if (doDEP.Checked)
            doXTT.Checked = true;
      }
      #region UI Based export
      private void Exportbutton_Click(object sender, EventArgs e)
      {
         checkDependencies();

         populateExportListFromListBox();

         //copy our UI over to our options list..
         mIgnorePerforce = !usePerforce.Checked;
         CoreGlobals.UsingPerforce = !mIgnorePerforce;

         mCheckoutFromPerforce = checkBox1.Checked && !mIgnorePerforce;
         mCheckinWhenFinished = checkBox2.Checked && !mIgnorePerforce;
         
         

         exportFileList();
      }
      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.Close();
      }
      #endregion
      //----------------------------
      //----------------------------
      #region AO Section Export
      private void exportAOFileList()
      {
         String batchExportLog = outputLog;
         if (batchExportLog == null)
            batchExportLog = Path.Combine(CoreGlobals.getWorkPaths().mLogsDirectory, "batchExportLog." + System.DateTime.Now.ToFileTimeUtc() + ".txt");
         mExportLogStream = new StreamWriter(batchExportLog, true);


         //export settings
         Export360.ExportSettings settings = null;
         if (mUseCustomExportSettings == -1 || mUseCustomExportSettings ==0)
         {
            settings = null;
         }
         else if (mUseCustomExportSettings == 1)
         {
            settings = new Export360.ExportSettings();
            settings.SettingsQuick();
         }
         else if (mUseCustomExportSettings == 2)
         {
            settings = new Export360.ExportSettings();
            settings.SettingsFinal();
         }


         bool createOwnDevice = BRenderDevice.getDevice() == null;

         if (createOwnDevice)
            BRenderDevice.createDevice(this, 640, 480, false);

         GrannyManager2.init();

         for (int fileCount = 0; fileCount < mScenarioFilesToExport.Count; fileCount++)// (string file in ToExportListBox.SelectedItems)
         {
            string scenarioName = mScenarioFilesToExport[fileCount].mFilename;
            string terrainName = mTerrainFilesToExport[mScenarioFilesToExport[fileCount].mTerrainFileIndex].mFilename;

            bool okExported = TEDIO.TEDtoAOSection(terrainName, scenarioName, outputDir, mAoNumSections, mAoSectionIndex, settings);
         }

         if (createOwnDevice)
            BRenderDevice.destroyDevice();

         mExportLogStream.Close();

         if (mStatusResult == 0)
            File.Delete(batchExportLog);

         clearList();
      }
      private void buildAOSections()
      {
         for (int fileCount = 0; fileCount < mScenarioFilesToExport.Count; fileCount++)// (string file in ToExportListBox.SelectedItems)
         {
            string scenarioName = mScenarioFilesToExport[fileCount].mFilename;
            string terrainName = mTerrainFilesToExport[mScenarioFilesToExport[fileCount].mTerrainFileIndex].mFilename;

            NetworkedAOGen aoGen = new NetworkedAOGen();
            aoGen.buildAOFiles(terrainName, mAoNumSections);
            aoGen = null;
         }

         

      }
      #endregion

      //----------------------------
      //----------------------------
      #region perforce interaction
      private string mChangelistDesc = "Batch Converter Automated Checkout";
      private int mChangelistID = -1;
      private void P4SubmitChangelist()
      {
         if (mChangelistID!=-1)
            CoreGlobals.getPerforce().getConnection().P4Submit(mChangelistID);
      }
      private bool P4AddFileToChangeList(string filename)
      {
         PerforceChangeList list = null;
         if (mChangelistID == -1)
         {
            list = CoreGlobals.getPerforce().GetNewChangeList(mChangelistDesc);
            if (list == null)               return false;
            mChangelistID = list.ID;
         }
         else
         {
            list = CoreGlobals.getPerforce().GetExistingChangeList(mChangelistID);
            if (list == null) return false;
         }

         list.AddOrEdit(filename, true);

         outputMessage(Path.GetFileName(filename) + " has been checked out from perforce.");
         return true;
      }

      private void P4RevertFileInChangeList(string filename)
      {
         PerforceChangeList list = null;
         if (mChangelistID == -1)
            return;//there should be an existing changelist

         {
            list = CoreGlobals.getPerforce().GetExistingChangeList(mChangelistID);
         }

         list.RevertFile(filename);

         outputMessage(Path.GetFileName(filename) + " has been reverted in perforce.");
      }

      private void P4RevertAllNeededFiles(string scenarioName, string terrainName)
      {
         if (doXTD.Checked)
         {
            P4RevertFileInChangeList(Path.ChangeExtension(terrainName, ".XTD"));
         }
         if (doXTT.Checked)
         {
            P4RevertFileInChangeList(Path.ChangeExtension(terrainName, ".XTT"));
            P4RevertFileInChangeList(Path.ChangeExtension(terrainName, ".DEP"));
         }
         if (doXTH.Checked)
         {
            P4RevertFileInChangeList(Path.ChangeExtension(terrainName, ".XTH"));
         }
         if (doXSD.Checked)
         {
            P4RevertFileInChangeList(Path.ChangeExtension(scenarioName, ".XSD"));
         }
         if (doLRP.Checked)
         {
            P4RevertFileInChangeList(Path.ChangeExtension(scenarioName, ".LRP"));
         }


         
         if (doTAG.Checked)
         {
            P4RevertFileInChangeList(Path.ChangeExtension(scenarioName, ".TAG"));
         }

         if (doXMB.Checked)
         {
            P4RevertFileInChangeList(Path.ChangeExtension(scenarioName, ".SCN.XMB"));
            P4RevertFileInChangeList(Path.ChangeExtension(scenarioName, ".GLS.XMB"));
            P4RevertFileInChangeList(Path.ChangeExtension(scenarioName, ".DEP.XMB"));
         }

      }
      private bool P4CheckoutAllNeededFiles(string scenarioName, string terrainName)
      {
         if (doXTD.Checked)
         {
            if (!P4AddFileToChangeList(Path.ChangeExtension(terrainName, ".XTD"))) return false;
         }
         if (doXTT.Checked )
         {
            if (!P4AddFileToChangeList(Path.ChangeExtension(terrainName, ".XTT"))) return false;
            if (!P4AddFileToChangeList(Path.ChangeExtension(terrainName, ".DEP"))) return false;
         }
         if (doXTH.Checked )
         {
           if (!P4AddFileToChangeList(Path.ChangeExtension(terrainName, ".XTH"))) return false;
         }
         if (doXSD.Checked )
         {
            if (!P4AddFileToChangeList(Path.ChangeExtension(scenarioName, ".XSD"))) return false;
         }
         if (doLRP.Checked)
         {
            if (!P4AddFileToChangeList(Path.ChangeExtension(scenarioName, ".LRP"))) return false;
         }

         if (doTAG.Checked )
         {
            if (!P4AddFileToChangeList(Path.ChangeExtension(scenarioName, ".TAG"))) return false;
         }

         if (doXMB.Checked )
         {
            if (!P4AddFileToChangeList(Path.ChangeExtension(scenarioName, ".SCN.XMB"))) return false;
            if (!P4AddFileToChangeList(Path.ChangeExtension(scenarioName, ".GLS.XMB"))) return false;
            if (!P4AddFileToChangeList(Path.ChangeExtension(scenarioName, ".DEP.XMB"))) return false;
         }
         return true;
      }

      private bool P4CanEditFile(string filename, ref bool inPerforce)
      {
         SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(filename);
         inPerforce |= status.InPerforce;
         if (status.InPerforce == false) return true;
         if (status.CheckedOutOtherUser == true)
         {
            outputMessage(Path.GetFileName(filename) + " is checked out by " + status.ActionOwner + ". Cannot convert");
            mStatusResult = 1;
            return false;
         }
         
         return true;
      }
      private bool P4CanEdit(string scenarioName, string terrainName, bool checkFromPerforce)
      {
         bool inPerforce = false;
         if (doXTD.Checked)
         {
            if (!P4CanEditFile(Path.ChangeExtension(terrainName, ".XTD"), ref inPerforce)) return false;
            
         }
         if (doXTT.Checked)
         {
            if (!P4CanEditFile(Path.ChangeExtension(terrainName, ".XTT"), ref inPerforce)) return false;
            if (!P4CanEditFile(Path.ChangeExtension(terrainName, ".DEP"), ref inPerforce)) return false;
         }
         if (doXTH.Checked)
         {
            if (!P4CanEditFile(Path.ChangeExtension(terrainName, ".XTH"), ref inPerforce)) return false;
         }
         if (doXSD.Checked)
         {
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".XSD"), ref inPerforce)) return false;
         }
         if (doLRP.Checked)
         {
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".LRP"), ref inPerforce)) return false;
         }

         if (doTAG.Checked)
         {
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".TAG"), ref inPerforce)) return false;
         }

         if (doXMB.Checked)
         {
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".SCN.XMB"), ref inPerforce)) return false;
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".GLS.XMB"), ref inPerforce)) return false;
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".DEP.XMB"), ref inPerforce)) return false;
         }


         if (checkFromPerforce)
         {
            return P4CheckoutAllNeededFiles(scenarioName, terrainName);
         }
         else if (!checkFromPerforce && inPerforce)
         {
            return false;
         }
         
         return true;
      }
      #endregion

      //----------------------------
      //----------------------------
      private void radioButton2_CheckedChanged(object sender, EventArgs e)
      {
         mUseCustomExportSettings = 0;
      }
      private void radioButton1_CheckedChanged(object sender, EventArgs e)
      {
         mUseCustomExportSettings = -1;
      }
      private void radioButton4_CheckedChanged(object sender, EventArgs e)
      {
         mUseCustomExportSettings = 1;
      }
      private void radioButton3_CheckedChanged(object sender, EventArgs e)
      {
         mUseCustomExportSettings = 2;
      }
      
      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         mCheckinWhenFinished = checkBox2.Checked;
      }
      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         mCheckoutFromPerforce = checkBox1.Checked;
      }
      private void usePerforce_CheckedChanged(object sender, EventArgs e)
      {
         mIgnorePerforce = !usePerforce.Checked;
         CoreGlobals.UsingPerforce = !mIgnorePerforce;

         checkBox1.Enabled = usePerforce.Checked;
         checkBox2.Enabled = usePerforce.Checked;
      }

      private void doLRP_CheckedChanged(object sender, EventArgs e)
      {
         if (doLRP.Checked)
            doXSD.Checked = true;
      }

      private void doTAG_CheckedChanged(object sender, EventArgs e)
      {
         if(doTAG.Checked)
         {
            doXTT.Checked = true;
            doXTD.Checked = true;
            doXTH.Checked = true;
            doXSD.Checked = true;
         }
      }

      private void doDEP_CheckedChanged(object sender, EventArgs e)
      {
         if (doDEP.Checked)
            doXTT.Checked = true;
      }

      
     
   }
}
