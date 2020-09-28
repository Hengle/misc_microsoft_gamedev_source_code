using System;
using System.Collections;
using System.Diagnostics;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Tcp;
using System.Runtime.Remoting.Messaging;
using System.Collections.Generic;
using System.Runtime.Serialization.Formatters;
using System.Threading;
using System.Text;

namespace RemoteShell
{
   public class RemoteShellClient : MarshalByRefObject, IProcessClient
   {
      static void Usage()
      {
         Console.WriteLine("Usage:");
         Console.WriteLine("\tTo run a process: (use -f to fork and get no output)");
         Console.WriteLine("\tRemoteShellClient.exe <machine> -r <command> <argument(s)> [-w <workingDirectory>] [-f]");
         Console.WriteLine("\tTo kill a process:");
         Console.WriteLine("\tRemoteShellClient.exe <machine> -k <processNameToKill>");
         System.Environment.Exit(1);
      }

      static void Main(string[] args)
      {
         if (args.Length < 3)
            Usage();

         // we need to create a client object the server can ping to make sure we're alive
         try
         {
            BinaryServerFormatterSinkProvider serverProvider = new BinaryServerFormatterSinkProvider();
            serverProvider.TypeFilterLevel = TypeFilterLevel.Full;
            BinaryClientFormatterSinkProvider clientProvider = new BinaryClientFormatterSinkProvider();

            IDictionary props = new Hashtable();
            props["port"] = 0;
            TcpChannel channel = new TcpChannel(props, clientProvider, serverProvider);
            ChannelServices.RegisterChannel(channel, false);
         }
         catch (System.Exception)
         {
            Console.WriteLine("Failed setting up client for pings...");
         }

         string machine = args[0];
         string remoteAddress = String.Format("tcp://{0}:8086/RemoteShellServer", machine);
         IProcessManager processManager = (IProcessManager)RemotingServices.Connect(typeof(IProcessManager), remoteAddress);

         Console.WriteLine("Remoting to " + machine);

         if (args[1] == "-r")
         {
            RunProcess(args, processManager);
         }
         else if (args[1] == "-k")
         {
            KillProcess(args, processManager);
         }
         else
         {
            Usage();
         }
      }

      public static void RunProcess(string[] args, IProcessManager processManager)
      {
         ArrayList mainArgs = new ArrayList(args);
         string fileName = (string)mainArgs[2];
         string arguments = "";
         string workingDirectory = "";
         bool fork = false;

         if (mainArgs.Contains("-w"))
         {
            int index = mainArgs.IndexOf("-w");

            // make sure it's not the last index...
            if (index + 1 == mainArgs.Count)
               Usage();

            workingDirectory = (string)mainArgs[index + 1];

            // pull 'em out
            mainArgs.RemoveAt(index);
            mainArgs.RemoveAt(index);
         }

         if (mainArgs.Contains("-f"))
         {
            mainArgs.RemoveAt(mainArgs.IndexOf("-f"));
            fork = true;
         }

         // now that the optional arguments are gone, the rest of the array is the process arguments
         if (mainArgs.Count > 3)
            arguments = String.Join(" ", (string[])mainArgs.GetRange(3, mainArgs.Count - 3).ToArray(typeof(String)));

         try
         {
            Console.WriteLine(String.Format("Running file: {0} args: {1} dir: {2} fork: {3}", fileName, arguments, workingDirectory, fork));
            int pid = processManager.RunProcess(new RemoteShellClient(), fileName, arguments, workingDirectory, fork);

            // if we want to fork, just return now. if we got a 0 back for the process id, it failed to start
            if (fork)
            {
               int forkExitCode = (pid == 0) ? 256 : 0;
               System.Environment.Exit(forkExitCode);
            }
            else
            {
               // block until the process completes. This line just feels dirty...
               Thread.Sleep(System.Threading.Timeout.Infinite);
            }
         }
         catch (System.Exception)
         {
            Console.WriteLine("Failed trying to remote run process");
            System.Environment.Exit(1);
         }

         System.Environment.Exit(0);
      }

      public static void KillProcess(string[] args, IProcessManager processManager)
      {
         string processName = args[2];

         Console.WriteLine(String.Format("Killing {0}", processName));
         try
         {
            if (!processManager.KillProcess(processName))
               System.Environment.Exit(1);
         }
         catch (System.Exception)
         {
            Console.WriteLine("Failed trying to kill process");
            System.Environment.Exit(1);
         }

         System.Environment.Exit(0);
      }
      
      public void ProcessOutputReceived(string output)
      {
         Console.WriteLine(output);
      }

      public void ProcessExited(int exitCode)
      {
         System.Environment.Exit(exitCode);
      }

      public bool Ping()
      {
         return true;
      }
   }
}
