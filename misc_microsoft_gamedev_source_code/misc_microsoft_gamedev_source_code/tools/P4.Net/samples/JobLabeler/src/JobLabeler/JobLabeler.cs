/*
 * P4.Net *
Copyright (c) 2006 Shawn Hladky

Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without 
restriction, including without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */


using System;
using System.Collections.Generic;
using System.Text;
using P4API;
using System.Diagnostics;

namespace JobLabeler
{
    class JobLabeler
    {
        private string _p4port = null;
        private string _p4user = null;
        private string _p4password = null;
        private string _labelTemplate = null;
        private string _p4counter = null;
        private string _labelNameFormat = null;
        private bool _useLogin = false;


        //Constructor... just store off the configuration settings.
        public JobLabeler(string P4Port, string P4User, string P4Password, string LabelTemplate,
            string P4Counter, string LabelNameFormat, bool UseLogin)
        {
            _p4port = P4Port;
            _p4user = P4User;
            _p4password = P4Password;
            _labelTemplate = LabelTemplate;
            _p4counter = P4Counter;
            _useLogin = UseLogin;
            _labelNameFormat = LabelNameFormat;
        }

        //Guts of the logic.  This is where the app will dynamicaly build the job labels
        public void RunLabeler(EventLog log)
        {
            P4Connection p4 = null;
            try
            {
                p4 = new P4Connection();
                p4.Port = _p4port;
                p4.User = _p4user;
                if (!_useLogin)
                {
                    p4.Password = _p4password;
                }
                p4.Connect();
                if (_useLogin)
                {
                    p4.Login(_p4password);
                }

                // Don't throw an exception on a Perforce error.  We handle these manually.
                p4.ExceptionLevel = P4ExceptionLevels.NoExceptionOnErrors;

                //Use unparsed b/c older server versions do not support
                //parsed output for the counter command.  And, it's easy
                //to parse by hand.
                P4UnParsedRecordSet counter = p4.RunUnParsed("counter", _p4counter);
                
                int counterValue = int.Parse(counter[0]);
                int LastSequenceNumber = 0;
                List<string> JobList = new List<string>();
                
                //Run p4 logger to find all the changes and jobs since the last run
                P4RecordSet loggers = p4.Run("logger", "-c", counterValue.ToString());
                
                //spin the results, and get a unique list of jobs
                foreach (P4Record r in loggers)
                {
                    if (r.Fields["key"] == "job")
                    {
                        string JobName = r.Fields["attr"];
                        if (!JobList.Contains(JobName)) JobList.Add(JobName);
                    }
                    LastSequenceNumber = int.Parse(r.Fields["sequence"]);
                }

                // We're done with loggers, so lets remove the reference and 
                // give the GC a chance to clean memory.  If it's been a long time
                // since the last run, it could be thousands of jobs to catch up.
                loggers = null;

                // Now spin all the jobs and build the label
                foreach (string JobName in JobList)
                {
                    BuildLabel(JobName, p4, log);
                }

                //Now we update the counter to the last sequence number from logger.
                p4.RunUnParsed("counter", _p4counter, LastSequenceNumber.ToString());

            }
            catch (Exception e)
            {
                // rethrow... b/c I'm lazy :-)
                throw e;
            }
            finally
            {
                // All done here... time to disconnect.
                p4.Disconnect();
            }
        }

        private void BuildLabel(string JobName, P4Connection p4, EventLog log)
        {
            string LabelName = string.Format(_labelNameFormat, JobName);

            // delete the label... it may not exist, but finding that out is worse performance
            // than just deleting and assuming it doesn't exist.
            // The '-f' flag may cause problems if the p4 account running this command isn't a
            // super user.  You should be able to remove the flag... so long as no one 
            // manually monkey's with the labels.
            P4UnParsedRecordSet labelDel = p4.RunUnParsed("label", "-f", "-d", LabelName);

            List<int> JobChanges = new List<int>();
            
            // Run a fixes to get all the changelists we need to add
            P4RecordSet fixes = p4.Run("fixes", "-j", JobName);
            
            // Spin the recordset to build a unique list of changelists
            foreach (P4Record fix in fixes)
            {
                JobChanges.Add(int.Parse(fix["Change"]));
            }

            // Sort them to be certain they are ascending
            JobChanges.Sort();

            // only build the label if there are indeed fixes
            if (JobChanges.Count > 0)
            {
                //re-create the label
                P4Form labelForm = p4.Fetch_Form("label", "-t", _labelTemplate, LabelName);

                // make sure the form is unlocked
                labelForm["Options"] = "unlocked";

                // make sure we're the owner
                labelForm["Owner"] = _p4user;
                p4.Save_Form(labelForm);

                int ChangesAdded = 0;

                //now need to labelsync to all latest changes
                foreach (int change in JobChanges)
                {
                    // using tag here so a valid client spec is not needed.
                    // for older servers, you could substitue for labelsync,
                    // but you'd need to pass in and set a valid client spec
                    P4UnParsedRecordSet ls = p4.RunUnParsed("tag", "-l", LabelName, string.Format("@={0}", change));

                    // this is why we set exception level to NoExceptionOnErrors
                    if (ls.HasErrors())
                    {
                        if (ls.ErrorMessage.StartsWith("Can't use a pending changelist number for this command."))
                        {
                            // Nothing to worry about.  p4 fixes returns fix records for pending changelists,
                            // but we don't know that going into this.  It's likely more performant to just 
                            // assume it is a submitted changelist and ignore this error.
                        }
                        else
                        {
                            // Something's gone ary.  Should throw an Exception.
                            // But I'm lazy so we just log the error.
                            log.WriteEntry(ls.ErrorMessage);
                        }
                    }
                    else
                    {
                        ChangesAdded++;
                    }
                }

                // If ChangesAdded is still 0, then we should delete the label since 
                // there are no files anyway
                if (ChangesAdded == 0)
                {
                    p4.RunUnParsed("label", "-f", "-d", LabelName);
                }
                else
                {
                    // now lock the label
                    labelForm["Options"] = "locked";
                    p4.Save_Form(labelForm);

                    // Note this trick.  You can re-save a form as many times as you need,
                    // no need to fetch before each save.  In fact you can use the form to save 
                    // new objects... like:
                    //   labelForm["Label"] = "newLabelName";
                    //   p4.Save_Form(labelForm);
                    // and you created a new label named newLabelName.  
                    // Cool, huh?  Well, I don't deserve the credit... it's the way the 
                    // native C++ API works.
                }
            }
        }
    }
}
