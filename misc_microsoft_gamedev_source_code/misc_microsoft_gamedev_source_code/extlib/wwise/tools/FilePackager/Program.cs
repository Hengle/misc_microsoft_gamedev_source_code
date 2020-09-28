using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace AkFilePackager
{
    static class Program
    {
        public const string APP_NAME = "File packager"; 
       

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static int Main( string[] in_args )
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            bool bGenerateMode = false;
            bool bHideProgressUI = false;
            string szInfoFile = "";
            string szLayoutFile = "";
            string szSpecifiedLanguage = "";
            CmdLinePackageSettings cmdLineSettings = new CmdLinePackageSettings();
                                
            for (int i = 0; i < in_args.Length; i++)
            {
                if (0 == String.Compare(in_args[i], "-generate", true))
                {
                    bGenerateMode = true;
                }
                else if (0 == String.Compare(in_args[i], "-info", true))
                {
                    if (i < in_args.Length - 1)
                        szInfoFile = in_args[i + 1];
                }
                else if (0 == String.Compare(in_args[i], "-layout", true))
                {
                    try
                    {
                        if (i < in_args.Length - 1)
                            szLayoutFile = in_args[i + 1];
                        else
                            throw new Exception("Value expected.");
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show("Error -layout: " + ex.Message, APP_NAME);
                        return 1;
                    }
                }
                else if (0 == String.Compare(in_args[i], "-output", true))
                {
                    if (i < in_args.Length - 1)
                        cmdLineSettings.FilePackage = in_args[i + 1];
                }
                else if (0 == String.Compare(in_args[i], "-blocksize", true))
                {
                    try
                    {
                        if (i < in_args.Length - 1)
                            cmdLineSettings.DefaultBlockSize = uint.Parse(in_args[i + 1]);
                        else
                            throw new Exception("Value expected.");
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show("Error -blocksize: " + ex.Message, APP_NAME);
                        return 1;
                    }
                }
                else if (0 == String.Compare(in_args[i], "-hideprogressui", true))
                {
                    try
                    {
                        if (i < in_args.Length - 1)
                            bHideProgressUI = bool.Parse(in_args[i + 1]);
                        else
                            throw new Exception("Value expected.");
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show("Error -hideprogressui: " + ex.Message, APP_NAME);
                        return 1;
                    }
                }
                else if (0 == String.Compare(in_args[i], "-SpecifiedLanguage", true))
                {
                   if (i < in_args.Length - 1)
                      szSpecifiedLanguage = in_args[i + 1];
                }
            }

            if (bGenerateMode)
            {
                GenerateModeAppContext appCtx = new GenerateModeAppContext();
                int iReturn = appCtx.Generate(szInfoFile, szLayoutFile, cmdLineSettings, bHideProgressUI, szSpecifiedLanguage);
                if (appCtx.RunApplicationLoop)
                    Application.Run(appCtx);
                return iReturn;
            }
            else
            {
                Application.Run(new EditModeForm());
                return 0;
            }
        }

    }
}