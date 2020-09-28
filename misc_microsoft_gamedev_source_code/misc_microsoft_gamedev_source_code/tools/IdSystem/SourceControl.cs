using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;


namespace IdSystem
{
    public class SourceControl
    {
        private bool mVerbose;

        public SourceControl(bool verbose)
        {
            mVerbose = verbose;
        }

        public bool Checkout(string filename)
        {
            string command = "-s edit " +filename;
            return ExecuteCommand(command);
        }

        public bool Lock(string filename)
        {
            string command = "-s lock " +filename;
            return ExecuteCommand(command);
        }

        public bool Revert(string filename)
        {
            string command = "-s revert " +filename;
            return ExecuteCommand(command);
        }

        public bool Sync(string filename)
        {
            string command = "-s sync " + filename;
            return ExecuteCommand(command);
        }

        public bool Checkin(string filename, string description)
        {
            string command = "-s submit -d \"" +description +"\" " +filename;
            return ExecuteCommand(command);
        }


        protected bool ExecuteCommand(string command)
        {
            Process proc = new Process();
            proc.StartInfo.UseShellExecute = false;
            proc.StartInfo.RedirectStandardOutput = true;
//            proc.StartInfo.RedirectStandardInput = true;
            proc.StartInfo.RedirectStandardError = true;
            proc.StartInfo.CreateNoWindow = true;
            proc.StartInfo.FileName = @"p4.exe";
            proc.StartInfo.Arguments = (command);
            proc.Start();

            proc.WaitForExit();

            string stdOutput="";
            string stdError="";

            // grab std out and std err
            stdOutput=proc.StandardOutput.ReadToEnd();
            stdError=proc.StandardError.ReadToEnd();

            if (mVerbose)
            {
                Console.WriteLine(stdOutput);
            }
            Console.WriteLine(stdError);

            // did we have an error?
            if (stdError.Length > 0)
                return false;

            return true;
        }

    }
}
