using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using EnsembleStudios.RemoteGameDebugger;

using RemoteTools;

namespace NewRemoteTools
{
   public partial class MainForm : Form
   {
      LobbyLogic mLobbyLogic = new LobbyLogic();
      TimelineVis mRemoteProfiler = new TimelineVis();
      public MainForm()
      {
         InitializeComponent();
         this.FormClosing += new FormClosingEventHandler(Form1_FormClosing);

         this.Controls.Add(mRemoteProfiler);
         mRemoteProfiler.Dock = DockStyle.Fill;

         mLobbyLogic.DebuggerConnected += new NetServiceHost.ConnectedDelegate(OnNewDebuggerConnection);

         this.WindowState = FormWindowState.Normal;

         this.Shown += new EventHandler(MainForm_Shown);
      }

      void MainForm_Shown(object sender, EventArgs e)
      {
         this.WindowState = FormWindowState.Maximized;
      }

      void Form1_FormClosing(object sender, FormClosingEventArgs e)
      {
         mRemoteProfiler.Stop();
         System.Threading.Thread.Sleep(1000);
         if (mLobbyLogic != null)
         {
            mLobbyLogic.Dispose();
            mLobbyLogic = null;
         }
      }

      public void OnNewDebuggerConnection(NetServiceConnection clientConnection)
      {
         mRemoteProfiler.SetConnection(mLobbyLogic.ActiveClient);
         mRemoteProfiler.Start();         
      }

      private void button1_Click(object sender, EventArgs e)
      {         
         //mServiceListClient = new NetServiceListClient("10.10.36.103", 1337);
         //mLobbyLogic.ConnectDebugger(0);
      }

      private void button2_Click(object sender, EventArgs e)
      {
         //mRemoteProfiler.ActiveClient = mLobbyLogic.ActiveClient;
         //mRemoteProfiler.SetConnection(mLobbyLogic.ActiveClient);         
      }



   }
}