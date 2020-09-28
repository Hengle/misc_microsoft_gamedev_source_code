using System;
using System.Collections;
using System.Threading;
using System.Web;
using System.IO;
using System.Configuration;
using System.Diagnostics;

using InterProcessComm.InterProcessComm;
using InterProcessComm.NamedPipes;


namespace xfs
{

   public sealed class ServerNamedPipe : IDisposable
   {
      internal Thread PipeThread;
      internal ServerPipeConnection PipeConnection;
      internal bool Listen = true;
      internal DateTime LastAction;
      private bool disposed = false;


      private void PipeListener()
      {
         CheckIfDisposed();
         try
         {

            Listen = Program.mPipeManager.Listen;
            while (Listen)
            {
               LastAction = DateTime.Now;
               string request = PipeConnection.Read();
               LastAction = DateTime.Now;
               if (request.Trim() != "")
               {
                  PipeConnection.Write(Program.mPipeManager.HandleRequest(request));
               }
               else
               {
                  PipeConnection.Write("Error: bad request");
               }
               LastAction = DateTime.Now;
               PipeConnection.Disconnect();
               if (Listen)
               {
                  //	Form1.ActivityRef.AppendText("Pipe " + this.PipeConnection.NativeHandle.ToString() + ": listening" + Environment.NewLine);
                  Connect();
               }
               Program.mPipeManager.WakeUp();
            }
         }
         catch (System.Threading.ThreadAbortException ) { }
         catch (System.Threading.ThreadStateException ) { }
         catch (Exception )
         {
            // Log exception
         }
         finally
         {
            this.Close();
         }
      }
      internal void Connect()
      {
         CheckIfDisposed();
         PipeConnection.Connect();
      }
      internal void Close()
      {
         CheckIfDisposed();
         this.Listen = false;
         Program.mPipeManager.RemoveServerChannel(this.PipeConnection.NativeHandle);
         this.Dispose();
      }
      internal void Start()
      {
         CheckIfDisposed();
         PipeThread.Start();
      }
      private void CheckIfDisposed()
      {
         if (this.disposed)
         {
            throw new ObjectDisposedException("ServerNamedPipe");
         }
      }
      public void Dispose()
      {
         Dispose(true);
         GC.SuppressFinalize(this);
      }
      private void Dispose(bool disposing)
      {
         if (!this.disposed)
         {
            PipeConnection.Dispose();
            if (PipeThread != null)
            {
               try
               {
                  PipeThread.Abort();
               }
               catch (System.Threading.ThreadAbortException ) { }
               catch (System.Threading.ThreadStateException ) { }
               catch (Exception )
               {
                  // Log exception
               }
            }
         }
         disposed = true;
      }
      ~ServerNamedPipe()
      {
         Dispose(false);
      }
      internal ServerNamedPipe(string name, uint outBuffer, uint inBuffer, int maxReadBytes, bool secure)
      {
         PipeConnection = new ServerPipeConnection(name, outBuffer, inBuffer, maxReadBytes, secure);
         PipeThread = new Thread(new ThreadStart(PipeListener));
         PipeThread.IsBackground = true;
         PipeThread.Name = "Pipe Thread " + this.PipeConnection.NativeHandle.ToString();
         LastAction = DateTime.Now;
      }
   }


   public sealed class PipeManager : InterProcessComm.InterProcessComm.IChannelManager
   {

      public Hashtable Pipes;

      private uint NumberPipes = 5;
      private uint OutBuffer = 512;
      private uint InBuffer = 512;
      private const int MAX_READ_BYTES = 5000;
      private bool _listen = true;
      public bool Listen
      {
         get
         {
            return _listen;
         }
         set
         {
            _listen = value;
         }
      }
      private int numChannels = 0;
      private Hashtable _pipes = new Hashtable();
      private Thread MainThread;
      private string PipeName = "XFSCommPipe";
      private ManualResetEvent Mre;
      private const int PIPE_MAX_STUFFED_TIME = 5000;

      public object SyncRoot = new object();

      public void Initialize()
      {
         Pipes = Hashtable.Synchronized(_pipes);
         Mre = new ManualResetEvent(false);
         MainThread = new Thread(new ThreadStart(Start));
         MainThread.IsBackground = true;
         MainThread.Name = "XFS Pipe Thread";
         MainThread.Start();
         Thread.Sleep(1000);
      }




      public string HandleRequest(string request)
      {
         Program.mMainForm.addInterprocessCommand(request);
         return "msgRecieved";
      }


      private void Start()
      {
         try
         {
            while (_listen)
            {
               int[] keys = new int[Pipes.Keys.Count];
               Pipes.Keys.CopyTo(keys, 0);
               foreach (int key in keys)
               {
                  ServerNamedPipe serverPipe = (ServerNamedPipe)Pipes[key];
                  if (serverPipe != null && DateTime.Now.Subtract(serverPipe.LastAction).Milliseconds > PIPE_MAX_STUFFED_TIME && serverPipe.PipeConnection.GetState() != InterProcessConnectionState.WaitingForClient)
                  {
                     serverPipe.Listen = false;
                     serverPipe.PipeThread.Abort();
                     RemoveServerChannel(serverPipe.PipeConnection.NativeHandle);
                  }
               }
               if (numChannels <= NumberPipes)
               {
                  ServerNamedPipe pipe = new ServerNamedPipe(PipeName, OutBuffer, InBuffer, MAX_READ_BYTES, false);
                  try
                  {
                     pipe.Connect();
                     pipe.LastAction = DateTime.Now;
                     System.Threading.Interlocked.Increment(ref numChannels);
                     pipe.Start();
                     Pipes.Add(pipe.PipeConnection.NativeHandle, pipe);
                  }
                  catch (InterProcessIOException )
                  {
                     RemoveServerChannel(pipe.PipeConnection.NativeHandle);
                     pipe.Dispose();
                  }
               }
               else
               {
                  Mre.Reset();
                  Mre.WaitOne(1000, false);
               }
            }
         }
         catch
         {
            // Log exception
         }
      }
      public void Stop()
      {
         _listen = false;
         Mre.Set();
         try
         {
            int[] keys = new int[Pipes.Keys.Count];
            Pipes.Keys.CopyTo(keys, 0);
            foreach (int key in keys)
            {
               ((ServerNamedPipe)Pipes[key]).Listen = false;
            }
            int i = numChannels * 3;
            for (int j = 0; j < i; j++)
            {
               StopServerPipe();
            }
            Pipes.Clear();
            Mre.Close();
            Mre = null;
         }
         catch
         {
            // Log exception
         }
      }

      public void WakeUp()
      {
         if (Mre != null)
         {
            Mre.Set();
         }
      }
      private void StopServerPipe()
      {
         try
         {
            ClientPipeConnection pipe = new ClientPipeConnection(PipeName);
            if (pipe.TryConnect())
            {
               pipe.Close();
            }
         }
         catch
         {
            // Log exception
         }
      }

      public void RemoveServerChannel(object param)
      {
         int handle = (int)param;
         System.Threading.Interlocked.Decrement(ref numChannels);
         Pipes.Remove(handle);
         this.WakeUp();
      }
   }
}