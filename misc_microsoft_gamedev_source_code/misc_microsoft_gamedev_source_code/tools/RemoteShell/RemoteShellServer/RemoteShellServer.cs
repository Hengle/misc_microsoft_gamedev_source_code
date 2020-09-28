using System;
using System.Collections;
using System.Diagnostics;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Tcp;
using System.Runtime.Remoting.Messaging;
using System.Data;
using System.Configuration;
using System.Runtime.Serialization.Formatters;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace RemoteShell
{
   public class RemoteShellServer : MarshalByRefObject, IProcessManager
   {
      static void Main(string[] args)
      {
         Console.WriteLine("Remote Shell Server Starting...");

         try
         {
            BinaryServerFormatterSinkProvider serverProvider = new BinaryServerFormatterSinkProvider();
            serverProvider.TypeFilterLevel = TypeFilterLevel.Full;
            BinaryClientFormatterSinkProvider clientProvider = new BinaryClientFormatterSinkProvider();

            IDictionary props = new Hashtable();
            props["port"] = 8086;
            TcpChannel channel = new TcpChannel(props, clientProvider, serverProvider);
            ChannelServices.RegisterChannel(channel, false);

            RemotingConfiguration.RegisterWellKnownServiceType(typeof(RemoteShellServer), "RemoteShellServer", WellKnownObjectMode.Singleton);
         }
         catch (System.Exception)
         {
            Console.WriteLine("Failed setting up server...");
         }

         Console.WriteLine("Press any key to exit...");
         Console.ReadLine();
      }

      private static int mReadThreadSleepTime = 100;
      private static int mExitThreadSleepTime = 500;

      public delegate void ProcessDelegate(IProcessClient processClient, Process process);

      public int RunProcess(IProcessClient client, string fileName, string arguments, string workingDirectory, bool fork)
      {
         Console.WriteLine(String.Format("Running file: {0} args: {1} dir: {2} fork: {3}", fileName, arguments, workingDirectory, fork));

         // take the start info and fire off a process. Just let it do its thing.
         Process process = new Process();
         process.StartInfo.FileName = fileName;
         process.StartInfo.Arguments = arguments;
         process.StartInfo.WorkingDirectory = workingDirectory;

         if (!fork)
         {
            process.StartInfo.UseShellExecute = false;
            process.StartInfo.RedirectStandardOutput = true;
            process.StartInfo.RedirectStandardError = true;
            process.StartInfo.CreateNoWindow = true;
         }

         try
         {
            process.Start();

            // if they want to fork, fire and forget
            if (!fork)
            {
               new ProcessDelegate(ReadStdOut).BeginInvoke(client, process, null, null);
               new ProcessDelegate(ReadStdErr).BeginInvoke(client, process, null, null);
               new ProcessDelegate(WaitForExit).BeginInvoke(client, process, null, null);
            }

            return process.Id;
         }
         catch (System.Exception)
         {
            return 0;
         }
      }

      public bool KillProcess(string processName)
      {
         Console.WriteLine(String.Format("Killing: {0}", processName));

         try
         {
            Process[] processes = Process.GetProcessesByName(processName);

            foreach (Process p in processes)
               p.Kill();

            return true;
         }
         catch (System.Exception)
         {
            return false;
         }
      }

      public void ClientDisconnected(IProcessClient processClient, Process process)
      {
         Console.WriteLine("Client disconnected, cleaning up pid {0}", process.Id);
         // once the client has disconnected, kill its corresponding process
         try
         {
            process.Kill();
         }
         catch (System.Exception)
         {
            // we can get in here if the process has already exited
         }
      }

      private void ReadStdOut(IProcessClient processClient, Process process)
      {
         try
         {
            string str;
            while ((str = process.StandardOutput.ReadLine()) != null)
            {
               processClient.ProcessOutputReceived(str);
               Thread.Sleep(mReadThreadSleepTime);
            }
         }
         catch (System.Exception)
         {
            // if we get an exception, the client has disconnected, so kill the process
            ClientDisconnected(processClient, process);
         }
      }

      private void ReadStdErr(IProcessClient processClient, Process process)
      {
         try
         {
            string str;
            while ((str = process.StandardError.ReadLine()) != null)
            {
               processClient.ProcessOutputReceived(str);
               Thread.Sleep(mReadThreadSleepTime);
            }
         }
         catch (System.Exception)
         {
            // if we get an exception, the client has disconnected, so kill the process
            ClientDisconnected(processClient, process);
         }

      }

      private void WaitForExit(IProcessClient processClient, Process process)
      {
         try
         {
            // wait for the process to exit and make sure all the process output is sent
            while (!process.HasExited || !process.StandardOutput.EndOfStream || !process.StandardError.EndOfStream)
            {
               // ping the client. if this fails, an exception will be thrown
               processClient.Ping();

               Thread.Sleep(mExitThreadSleepTime);
            }

            processClient.ProcessExited(process.ExitCode);
         }
         catch (System.Exception)
         {
            // if we get an exception, the client has disconnected, so kill the process
            ClientDisconnected(processClient, process);
         }
      }
   }
}
