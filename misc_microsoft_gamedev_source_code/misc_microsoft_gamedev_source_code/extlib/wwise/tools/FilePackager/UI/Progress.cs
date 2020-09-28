using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AkFilePackager
{
    public partial class Progress : Form
    {
        public event EventHandler<EventArgs> StopRequested;

        public Progress()
        {
            InitializeComponent();
        }

        public void ProgressStepHandler(object sender, StepChangeEventArgs e)
        {
            textBoxProgMsg.Text = e.StepName;
            progressBar.Maximum = e.NumSubSteps;
            progressBar.Step = 1;
            progressBar.Value = 0;
        }

        public void StepHandler(object sender, EventArgs e)
        {
            progressBar.PerformStep();

            // Force pump messages to allow user stop.
            Application.DoEvents();
        }

        public void LogMsgHandler(object sender, LogMsgEventArgs e)
        {
            LogMsg(e.Msg);
        }

        public void LogMsg(string in_szMsg)
        {
            listBoxMessageLog.Items.Add(in_szMsg);
            // Select the last item to force scrolling down (!)
            listBoxMessageLog.SelectedIndex = listBoxMessageLog.Items.Count - 1;
        }

        public void Finished()
        {
            buttonStopClose.Text = "Close";
            m_bInProgress = false;
        }

        private bool m_bInProgress = true;

        private void buttonStopClose_Click(object sender, EventArgs e)
        {
            if (sender != buttonStopClose)
                return;
            if (!m_bInProgress)
                base.Close();
            else
                OnStopRequested();
        }

        private void Progress_FormClosing(object sender, FormClosingEventArgs e)
        {
            e.Cancel = m_bInProgress;
        }

        private void OnStopRequested()
        {
            if (StopRequested != null)
                StopRequested(this, new EventArgs());
        }
    }
}