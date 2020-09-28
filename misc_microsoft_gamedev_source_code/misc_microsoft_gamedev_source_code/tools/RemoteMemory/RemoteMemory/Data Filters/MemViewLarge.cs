using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Diagnostics;
using System.Text;
using System.Windows.Forms;
using System.Net;
using WinFormsUI.Docking;
using System.Threading;

namespace RemoteMemory
{
   public partial class MemViewLarge : DockContent, ITcpServerListener
   {

      AutoResetEvent mSymbolProcEvent = new AutoResetEvent(false);

      public MemViewLarge()
      {
         InitializeComponent();
      }

      //============================================================================
      // LoadSymbolsParams
      //============================================================================
      class LoadSymbolsParams
      {
         public int mProcHandle = 0;
         public string mSymbolFileName = "";
      };

      void bw_DoWork(object sender, DoWorkEventArgs e)
      {

         LoadingSymbolsWait lsw = new LoadingSymbolsWait();
         lsw.Show();
         do
         {
            lsw.Update();
         }
         while(!mSymbolProcEvent.WaitOne(3,false));

         lsw.Close();
         lsw = null;

    

      }

      //============================================================================
      // onClientConnected
      //============================================================================
      public void onClientConnected(string name,string pdbFilename)
      {
         AllocStats.clearHeaps();
         

         LoadSymbolsParams lsp = new LoadSymbolsParams();
         lsp.mProcHandle = 0x00ABBEEF; // Process.GetCurrentProcess().Id;
         lsp.mSymbolFileName = pdbFilename;//string sName = @"C:\depot\phoenix\xbox\code\xgame\xbox\debug\xgameD.exe";

         //since this takes so long, spend display a 'wait' bar to the user..
         {
            mSymbolProcEvent.Reset();
            
            BackgroundWorker mWorkerThread = new BackgroundWorker();
            mWorkerThread.WorkerReportsProgress = false;
            //mWorkerThread.ProgressChanged += bw_ProgressChanged;
            mWorkerThread.WorkerSupportsCancellation = false;
            //mWorkerThread.RunWorkerCompleted += bw_RunWorkerCompleted;
            mWorkerThread.DoWork += bw_DoWork;
            mWorkerThread.RunWorkerAsync(lsp);

            HaloWarsMem.loadSymbolInfo((uint)lsp.mProcHandle, lsp.mSymbolFileName);

            mSymbolProcEvent.Set();
         }
         AllocLogStream.unpauseProcessing();

         
         heapLines.onConnect();
         heapKey.onConnect();
         heapFileView.onConnect();
         topAllocs.onConnect();
         mFileTimelines.onConnect();
         mFileGroupings.onConnect();
      }

      //============================================================================
      // onClientDisconnected
      //============================================================================
      public void onClientDisconnected()
      {
         heapLines.onDisconnect();
         heapKey.onDisconnect();
         heapFileView.onDisconnect();
         topAllocs.onDisconnect();
         mFileTimelines.onDisconnect();
         mFileGroupings.onDisconnect();

         //HaloWarsMem.closeSymbolInfo();

         MessageBox.Show(null, "Playback Completed!", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
      }


      //============================================================================
      // onMessageRecieved
      //============================================================================
      public void onMessageRecieved(PacketWrapper packet)
      {
         HaloWarsMem.eALPacketType type = (HaloWarsMem.eALPacketType)packet.mPacketType;

         switch(type)
         {
            //================================================
            case HaloWarsMem.eALPacketType.cALNew:
            {
               HaloWarsMem.BALPacketNew pkt = new HaloWarsMem.BALPacketNew(packet.mPacketSize, packet.mPacketData);

               HeapAlloc pHeap = AllocStats.getHeapFromBasePtr(pkt.mpHeap);
               if (pHeap == null)
                  return;

               pHeap.allocMem(pkt.mSize, pkt.mpBlock, pkt.mBlockSize,pkt.mContext);

               //pass on
               mFileGroupings.onNew(pkt.mpHeap, pkt.mSize, pkt.mpBlock, pkt.mBlockSize, pkt.mContext);
               break;
            }
            //================================================  
            case HaloWarsMem.eALPacketType.cALResize:
            {
                HaloWarsMem.BALPacketResize pkt = new HaloWarsMem.BALPacketResize(packet.mPacketSize, packet.mPacketData);

                HeapAlloc pHeap = AllocStats.getHeapFromBasePtr(pkt.mpHeap);
               if (pHeap == null)
                  return;


               //pass on
               mFileGroupings.onResize(pkt.mpHeap, pkt.mpOrigBlock, pkt.mNewSize, pkt.mpNewBlock, pkt.mContext);


               //this needs to be done last
               pHeap.resizeMem(pkt.mpOrigBlock,pkt.mNewSize,pkt.mpNewBlock,pkt.mContext);
               


               
               break;
            }
         //================================================
            case HaloWarsMem.eALPacketType.cALDelete:
            {
                HaloWarsMem.BALPacketDelete pkt = new HaloWarsMem.BALPacketDelete(packet.mPacketSize, packet.mPacketData);

                HeapAlloc pHeap = AllocStats.getHeapFromBasePtr(pkt.mpHeap);
               if (pHeap == null)
                  return;


               //pass on
               mFileGroupings.onDelete(pkt.mpHeap, pkt.mpBlock, pkt.mContext);
               //need to pass on before deletion to ensure any queries occur
               pHeap.deleteMem(pkt.mpBlock,pkt.mContext);

               
               break;
            }
         //================================================
            case HaloWarsMem.eALPacketType.cALRegisterHeap:
            {
                HaloWarsMem.BALPacketRegisterHeap pkt = new HaloWarsMem.BALPacketRegisterHeap(packet.mPacketSize, packet.mPacketData);

                AllocStats.registerHeap(pkt);

               //propagate to those that care..
               heapLines.onHeapRegister(pkt.mPtr, pkt.mFlags, pkt.mName);
               heapFileView.onHeapRegister(pkt.mPtr, pkt.mFlags, pkt.mName);
               break;
            }
            //================================================
            case HaloWarsMem.eALPacketType.cALIgnoreLeaf:
               {
                  HaloWarsMem.BALPacketIgnoreLeaf pkt = new HaloWarsMem.BALPacketIgnoreLeaf(packet.mPacketSize, packet.mPacketData);
                  HaloWarsMem.getSymbolInfo().addIgnoreSymbol(pkt.mSymbolName);

                  break;
               }

            case HaloWarsMem.eALPacketType.cALEOF:
            {
               heapLines.onDisconnect();
               heapKey.onDisconnect();
               topAllocs.onDisconnect();
               heapFileView.onDisconnect();
               mFileTimelines.onDisconnect();
               mFileGroupings.onDisconnect();
               break;
            }


         };

      }

      private void button1_Click(object sender, EventArgs e)
      {
         SaveFileDialog sfd = new SaveFileDialog();
         sfd.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
         sfd.Filter = "CSV File *.csv|*.csv";
         if (sfd.ShowDialog() == DialogResult.OK)
         {
            topAllocs.dumpEntireList(sfd.FileName);
         }  
      }

     
   }
}
