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
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.ServiceProcess;
using System.Text;

namespace JobLabeler
{
    public partial class Service1 : ServiceBase
    {
        private JobLabeler _JobLabeler = null;
        public Service1()
        {
            InitializeComponent();
        }

        protected override void OnStart(string[] args)
        {
            // TODO: Add code here to start your service.
            LoadConfig();
            timerRun.Enabled = true;
            timerRun.Start();
        }

        protected override void OnStop()
        {
            // TODO: Add code here to perform any tear-down necessary to stop your service.
        }

        private void timerRun_Tick(object sender, EventArgs e)
        {
            this.EventLog.WriteEntry("running");
            //Disable the timer in case there are long-running jobs... we don't want them to queue up
            timerRun.Enabled = false;
            try
            {
                _JobLabeler.RunLabeler(this.EventLog);
            }
            catch (Exception err)
            {
                this.EventLog.WriteEntry(String.Format("Error running JobLabeler.\n{0}", err.Message), EventLogEntryType.Error);
            }


            // re-enable the timer
            timerRun.Enabled = true;
            timerRun.Start();
        }

        private void LoadConfig()
        {
            System.Configuration.AppSettingsReader ar = new System.Configuration.AppSettingsReader();

            string P4Port = (string)ar.GetValue("P4PORT", typeof(string));
            string P4User= (string)ar.GetValue("P4USER", typeof(string));
            string P4Password= (string)ar.GetValue("P4PASSWORD", typeof(string));
            string LabelTemplate= (string)ar.GetValue("LabelTemplate", typeof(string));
            string P4Counter= (string)ar.GetValue("P4Counter", typeof(string));
            string LabelNameFormat= (string)ar.GetValue("LabelNameFormat", typeof(string));
            bool UseLogin = ("true"==(string)ar.GetValue("P4UseLogin", typeof(string)));

            timerRun.Interval = 1000 * (int)ar.GetValue("SleepTime", typeof(int));
            
            _JobLabeler = new JobLabeler(P4Port, P4User, P4Password, LabelTemplate, P4Counter, LabelNameFormat, UseLogin);
            this.EventLog.WriteEntry(string.Format("timer interval: {0}", timerRun.Interval) );
            return;

        }
    }
}
