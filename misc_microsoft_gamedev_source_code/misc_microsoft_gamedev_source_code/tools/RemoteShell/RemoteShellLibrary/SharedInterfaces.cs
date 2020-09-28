using System;
using System.Collections;
using System.Diagnostics;
using System.Collections.Generic;
using System.Text;
using System.Runtime.Remoting.Messaging;

// a process that gets used to run remotely
namespace RemoteShell
{
   public interface IProcessManager
   {
      int RunProcess(IProcessClient client, string fileName, string arguments, string workingDirectory, bool fork);
      bool KillProcess(string processNames);
   }

   public interface IProcessClient
   {
      bool Ping();
      void ProcessOutputReceived(string output);
      void ProcessExited(int exitCode);
   }
}
