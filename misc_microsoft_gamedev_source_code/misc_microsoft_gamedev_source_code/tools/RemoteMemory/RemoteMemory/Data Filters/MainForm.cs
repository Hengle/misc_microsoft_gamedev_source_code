using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Net;
using System.Threading;
using System.Diagnostics;

using WinFormsUI.Docking;

namespace RemoteMemory
{
   public partial class MainForm : Form, ITcpServerListener
   {
      //private defines for winforms
      
      //============================================================================
      // MainForm
      //============================================================================
      public MainForm()
      {
         InitializeComponent();

         timer1.Enabled = true;
      }


      List<MemViewLarge> mMemViewWindows = new List<MemViewLarge>();

      ThreadSafeCallbackList mMessageCallbackList = new ThreadSafeCallbackList();
      TcpServerConnection mTCPClient = null;

      //============================================================================
      // onClientConnected
      //============================================================================
      public void onClientConnected(string title, string pdbFile)
      {
         AllocLogStream.pauseProcessing();

         ThreadSafeCallbackList.MessagePacket pak = new ThreadSafeCallbackList.MessagePacket();
         pak.mCallback = this.handleClientConnected;
         pak.mDataObject = pdbFile;
         pak.mMessageID = title;
         mMessageCallbackList.enqueueMessage(pak);
      }

      //============================================================================
      // onClientDisconnected
      //============================================================================
      public void onClientDisconnected()
      {
         ThreadSafeCallbackList.MessagePacket pak = new ThreadSafeCallbackList.MessagePacket();
         pak.mCallback = this.handleClientDisconnected;
         pak.mDataObject = 0;
         pak.mMessageID = "disconnected";
         mMessageCallbackList.enqueueMessage(pak);
      }

      //============================================================================
      // handleClientConnected
      //============================================================================
      public void handleClientConnected(string messageID, object objData)
      {
         //THIS SHOULD BE BACK ON THE UI THREAD!
         string pdbFile = (string)objData;

         MemViewLarge mvl = new MemViewLarge();
         mvl.Text = "Client : " + messageID;
         mvl.TabText = "Client : " + messageID;
         mvl.Show(dockPanel);
         mvl.onClientConnected( messageID,pdbFile);
         mMemViewWindows.Add(mvl);
      }

      //============================================================================
      // handleClientDisconnected
      //============================================================================
      public void handleClientDisconnected(string messageID, object objData)
      {
         //THIS SHOULD BE BACK ON THE UI THREAD!
         for (int i = 0; i < mMemViewWindows.Count; i++)
         {
            mMemViewWindows[i].onClientDisconnected();
         }
      }


      //============================================================================
      // onMessageRecieved
      //============================================================================
      public void onMessageRecieved(PacketWrapper packet)
      {
         ThreadSafeCallbackList.MessagePacket pak = new ThreadSafeCallbackList.MessagePacket();
         pak.mCallback = this.handleMessageRecieved;
         pak.mDataObject = packet;
         pak.mMessageID = "packet";

         if(GlobalSettings.PlaybackSpeed != GlobalSettings.ePlaybackSpeeds.eASAP)
         {
            //spinloop here while we're waiting for commands to be processed..
            while (mMessageCallbackList.Count() > 1024)
            {
               if (!AllocLogStream.isProcessing())
                  break;
         //      Thread.Sleep(1000);
            }
         }
         mMessageCallbackList.enqueueMessage(pak);
      }

      //============================================================================
      // handleMessageRecieved
      //============================================================================
      public void handleMessageRecieved(string messageID, object objData)
      {
         //THIS SHOULD BE BACK ON THE UI THREAD!
         PacketWrapper packet = (PacketWrapper)objData;

         IEnumerable<IDockContent> documents = dockPanel.Documents;

         foreach (DockContent c in documents)
         {
            if (c is ITcpServerListener)
               ((ITcpServerListener)c).onMessageRecieved(packet);
         }

      }

      //============================================================================
      // timer1_Tick
      //============================================================================
      private void timer1_Tick(object sender, EventArgs e)
      {
         if (mMessageCallbackList.Count() > 0)
         {
            for (int i = 0; i < mMessageCallbackList.Count(); i++)
            {
               mMessageCallbackList.dequeueProcessMessage();
            }
         }
      }

      //============================================================================
      // onStopPressed
      //============================================================================
      private void onStopPressed(string messageID, object objData)
      {
         stopProcessButton.Enabled = false;
         toolStripButton1.Enabled = true;
         toolStripButton2.Enabled = true;

         toolStripButton4.Enabled = true;
      }


      //============================================================================
      // toolStripButton2_Click
      //============================================================================
      class threadStartParms
      {
         public string filename;
         public string pdbfilename;
         public uint throttleTime;
      };
      private void toolStripButton2_Click(object sender, EventArgs e)
      {
         openFileOptions ofo = new openFileOptions();
         if(ofo.ShowDialog() == DialogResult.OK)
         {
            threadStartParms tsp = new threadStartParms();
            tsp.filename = ofo.FileName;
            tsp.pdbfilename = ofo.SymbolFileName;
            tsp.throttleTime = (uint)ofo.ThrottleTime;

            Thread clientThread = new Thread(new ParameterizedThreadStart(fileLoadThread));
            clientThread.Start(tsp);

            stopProcessButton.Enabled = true;
            toolStripButton2.Enabled = false;
            toolStripButton4.Enabled = false;

            if(mTCPClient!=null)
            {
               mTCPClient.stopListener();
               mTCPClient = null;
            }
            
         }
         
         
      }

      //============================================================================
      // fileLoadThread
      //============================================================================
      private void fileLoadThread(object obj)
      {
         threadStartParms parms = (threadStartParms)obj;
         string filename = parms.filename;
         string pdbfilename = parms.pdbfilename;
         //open the file, and inact the log stream
         FileStream fs = new FileStream(filename, FileMode.Open);
         BinaryReader br = new BinaryReader(fs);
         AllocLogStream.processBinaryStream(br, pdbfilename, parms.throttleTime);

         fs.Close();

         ThreadSafeCallbackList.MessagePacket pak = new ThreadSafeCallbackList.MessagePacket();
         pak.mCallback = this.onStopPressed;
         pak.mDataObject = null;
         pak.mMessageID = "stopped";
         mMessageCallbackList.enqueueMessage(pak);
      }

      //============================================================================
      // toolStripButton1_Click
      //============================================================================
      private void toolStripButton1_Click(object sender, EventArgs e)
      {
         xboxConnectOptions xco = new xboxConnectOptions();
         if(xco.ShowDialog() == DialogResult.OK)
         {
             mTCPClient = new TcpServerConnection();
             mTCPClient.waitFor360ToConnect(xco.SymbolFileName);

            toolStripButton1.Enabled = false;
            stopProcessButton.Enabled = true;
            toolStripButton4.Enabled = false;
         }
      }

      //============================================================================
      // MainForm_FormClosing
      //============================================================================
      private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
      {
         AllocLogStream.stopProcessing();
         if (mTCPClient != null)
         {
            mTCPClient.stopListener();
            mTCPClient = null;
         }
      }

      //============================================================================
      // stopProcessButton_Click
      //============================================================================
      private void stopProcessButton_Click(object sender, EventArgs e)
      {
         AllocLogStream.stopProcessing();
         if (mTCPClient != null)
         {
            mTCPClient.stopListener();
            mTCPClient = null;
         }
         onStopPressed(null, null);
         
      }

      //============================================================================
      // toolStripButton4_Click
      //============================================================================
      private void toolStripButton4_Click(object sender, EventArgs e)
      {
         SaveFileDialog sfd = new SaveFileDialog();
         sfd.Filter = "Alloc Log File (*.bin)|*.bin";
         if(sfd.ShowDialog() == DialogResult.OK)
         {
            AllocLogStream.saveStreamAs(sfd.FileName);
         }
      }
      //============================================================================
      // toolStripButton3_Click
      //============================================================================
      private void toolStripButton3_Click(object sender, EventArgs e)
      {
         Bitmap bmpScreenshot = new Bitmap(Screen.PrimaryScreen.Bounds.Width, Screen.PrimaryScreen.Bounds.Height, PixelFormat.Format32bppArgb);

         Graphics gfxScreenshot = Graphics.FromImage(bmpScreenshot);

         gfxScreenshot.CopyFromScreen(Screen.PrimaryScreen.Bounds.X, Screen.PrimaryScreen.Bounds.Y, 0, 0, Screen.PrimaryScreen.Bounds.Size, CopyPixelOperation.SourceCopy);

         
         string filepath = AppDomain.CurrentDomain.BaseDirectory + "screenshots\\";
         if (!Directory.Exists(filepath))
            Directory.CreateDirectory(filepath);

         string filename = "RMV_" + DateTime.Now.Hour.ToString() + DateTime.Now.Minute.ToString() + DateTime.Now.Second.ToString() + DateTime.Now.Millisecond.ToString() + ".bmp";
         while(File.Exists(filepath + filename))
         {
            filename = "RMV_" + DateTime.Now.Hour.ToString() + DateTime.Now.Minute.ToString() + DateTime.Now.Second.ToString() + DateTime.Now.Millisecond.ToString() + ".bmp";
         }
         try
         {
            EncoderParameters ep = new EncoderParameters();
            
            bmpScreenshot.Save(filepath + filename, ImageFormat.Bmp);
            Process.Start(filepath);
         }
         catch(Exception ee)
         {
            MessageBox.Show(ee.ToString());
         }

         
      }

   }
}
