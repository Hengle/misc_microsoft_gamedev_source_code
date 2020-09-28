using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using AK.Wwise.FilePackager.InfoFile;

namespace AkFilePackager
{
    /// <summary>
    /// Application context in Generate mode.
    /// A progress bar is shown if in_bVerbose is true only.
    /// The user shall call Application.Run() with this context in order to create the 
    /// message queue needed to process and wait for the "Close" button of the progress dialog.
    /// If there is no progress dialog, this.RunApplicationLoop is false and should not
    /// be called, so that the process ends by itself when the generation is complete.
    /// </summary>
    class GenerateModeAppContext : ApplicationContext
    {
        public int Generate(string in_szInfoFile, string in_szLayoutFile, CmdLinePackageSettings in_cmdLineSettings, bool in_bHideProgressUI, string in_szSpecifiedLanguage)
        {
            FilePackageGenerator generator = new FilePackageGenerator();

            // Display the progress bar (and log) only if the verbose switch was specified.
            if (!in_bHideProgressUI)
            {
                m_progressDlg = new Progress();

                generator.StepChange += m_progressDlg.ProgressStepHandler;
                generator.SubStep += m_progressDlg.StepHandler;
                generator.LogMsg += m_progressDlg.LogMsgHandler;

                m_progressDlg.StopRequested += generator.StopRequestedHandler;
                m_progressDlg.Closed += new EventHandler(OnUIClosed);

                m_progressDlg.Show();
            }

            int iReturn = 0;
            try
            {
                // In order to successfully start packaging:
                // - an info file must have been specified;
                // - either the file package name OR a layout file must have been specified.
                if (in_szInfoFile.Length > 0
                    && (in_cmdLineSettings.FilePackagePathSpecified || in_szLayoutFile.Length > 0))
                {
                    SoundBanksInfo data = InfoFileHelpers.LoadInfoFile(in_szInfoFile, in_szSpecifiedLanguage);
                    if (data != null)
                    {
                        PackageSettings settings;
                        List<OrderedFile> listOrderedFiles = null;
                        if (in_szLayoutFile.Length > 0)
                        {
                            PackagingEditorModel layoutModel = new PackagingEditorModel();
                            layoutModel.LoadLayout(in_szLayoutFile);

                            listOrderedFiles = layoutModel.OrderedFiles;
                            settings = layoutModel.Settings;

                            // Override layout's settings with each command line setting that was specified.
                            in_cmdLineSettings.OverrideSettingsWithCmdLine(ref settings);
                        }
                        else
                        {
                            settings = in_cmdLineSettings.GetSettings();
                        }

                        bool bNoFileMissing = generator.Generate(data, settings, listOrderedFiles);

                        // Do not display warning if the user does not wish to see UI.
                        if (m_progressDlg != null)
                        {
                            if (!bNoFileMissing)
                            {
                                m_progressDlg.LogMsg("WARNING: some files referenced in " + in_szInfoFile + " could not be found.");
                            }
                            m_progressDlg.LogMsg("SUCCESSFUL file package generation: " + settings.szFilePackageFilename);
                        }
                    }
                }
                else
                {
                    string szInvalidParam = "";
                    if (in_szInfoFile.Length == 0)
                        szInvalidParam = "No info file specified (-info).\n";
                    if (!in_cmdLineSettings.FilePackagePathSpecified || in_szLayoutFile.Length == 0)
                        szInvalidParam += "Neither the output file path (-output) or a layout file (-layout) was specified.\n";
                    throw new Exception("Some of the required input parameters are not valid:\n" + szInvalidParam);
                }
            }
            catch (Exception ex)
            {
                if (m_progressDlg != null)
                {
                    m_progressDlg.LogMsg("File packaging FAILED! " + ex.Message);
                    MessageBox.Show("File packaging FAILED!\n\n" + ex.Message, Program.APP_NAME);
                }
                iReturn = 1;
            }

            if (m_progressDlg != null)
            {
                // Has UI. Let user know that it should run the message loop until the user closes the form.
                m_progressDlg.Finished();
                m_bRunApplicationLoop = true;
            }
            else
            {
                // No UI. Let user know that it does not need to start the message loop.
                m_bRunApplicationLoop = false;
            }

            return iReturn;
        }

        private void OnUIClosed(object sender, EventArgs e)
        {
            ExitThread();
        }

        public bool RunApplicationLoop
        {
            get { return m_bRunApplicationLoop; }
        }

        private bool m_bRunApplicationLoop = true;
        private Progress m_progressDlg = null;
    }
}
