using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.IO;
using System.Diagnostics; 

namespace EditorCore
{
   public class PerfHarness
   {
      private static PerfHarness mMainHarness = new PerfHarness();
      bool mbLogPerf = true;
      PerfCounters mCounters = new PerfCounters();
      static public PerfHarness Harness
      {
         get
         {
            return mMainHarness;
         }
      }

      public PerfHarness()
      {
         if (!File.Exists(Path.Combine(CoreGlobals.getWorkPaths().mLogsDirectory, "EnableLogs.txt")))
         {
            mbLogPerf = false;
         }
         else
         {
            PerfLogWrite("Time: " + System.DateTime.Now.ToString(),false);
         }
      }

      int mStackLevel = 0;
      public void SectionStart(PerfSection section)
      {


         mStackLevel++;
         if (mbLogPerf)
         {
            PerfLogWrite(format(mStackLevel) + section.ToString(),true);
         }
      }
      public void SectionStop(PerfSection section)
      {
         if (mbLogPerf)
         {
            PerfLogWrite(format(mStackLevel) + section.ToString(),true);
         }
         mStackLevel--;
      }

      string GetSpacing(int len)
      {
         string spacing = "                                                                                                ";
         return spacing.Substring(0, len);
      }

      string format(int level)
      {
         string s = "";
         for (int i = 0; i < level; i++)
         {
            s += "---";
         }
         return s;
      }


      private void PerfLogWrite(string text, bool mem)
      {
         lock (this)
         {
            string filename = Path.Combine(CoreGlobals.getWorkPaths().mLogsDirectory, "PerfLogNew.txt");
            //StreamWriter w = File.AppendText(text);
            StreamWriter w = new StreamWriter(filename, true);

            if (mem)
            {
               int l = 60 - text.Length;
               if (l <= 5)
                  l = 5;
               text = text + GetSpacing(l) + mCounters.GetRamStats();

            }

            w.WriteLine(text);
            w.Close();
         }
      }

      public void PerfLogAddNote(string text)
      {
         if (mbLogPerf)
         {
            PerfLogWrite("Note: " + text, true);
         }
      }
   }



   public class PerfSection : IDisposable
   {
      DateTime mStartTime;
      double mElapsedtime = -1;
      string mName;
      public PerfSection(string name)
      {
         mStartTime = System.DateTime.Now;
         mName = name;

         PerfHarness.Harness.SectionStart(this);
      }
      ~PerfSection()
      {
         if (mElapsedtime == -1)
            Complete();
      }
      public void Complete()
      {
         mElapsedtime = ((TimeSpan)(System.DateTime.Now - mStartTime)).TotalMilliseconds;

         PerfHarness.Harness.SectionStop(this);

      }
      public override string ToString()
      {
         if (mElapsedtime != -1)
         {
            return mName + " " + mElapsedtime.ToString();
         }
         return mName;
      }

      #region IDisposable Members

      public void Dispose()
      {
         if (mElapsedtime == -1)
            Complete();
      }

      #endregion
   }


   public class PerfCounters
   {

      protected PerformanceCounter mProcessMemoryPrivate;
      protected PerformanceCounter mProcessMemoryWorking;
      protected PerformanceCounter mProcessMemoryVirtual;
      const float cGB = 1073741824; 
      const float cMB = 1048576;

      public PerfCounters()
      {
         string instance = AppDomain.CurrentDomain.FriendlyName.Replace(".exe", "");

         mProcessMemoryPrivate = new PerformanceCounter();
         mProcessMemoryPrivate.CategoryName = "Process";
         mProcessMemoryPrivate.CounterName = "Private Bytes";
         mProcessMemoryPrivate.InstanceName = instance;

         mProcessMemoryWorking = new PerformanceCounter();
         mProcessMemoryWorking.CategoryName = "Process";
         mProcessMemoryWorking.CounterName = "Working Set";
         mProcessMemoryWorking.InstanceName = instance;

         mProcessMemoryVirtual = new PerformanceCounter();
         mProcessMemoryVirtual.CategoryName = "Process";
         mProcessMemoryVirtual.CounterName = "Virtual Bytes";
         mProcessMemoryVirtual.InstanceName = instance;
      }

      public string GetRamStats()
      {
         return String.Format(" Private: {0:F0} Working: {1:F0} Virtual: {2:F0}", mProcessMemoryPrivate.NextValue() / cMB, mProcessMemoryWorking.NextValue() / cMB, mProcessMemoryVirtual.NextValue() / cMB);
      }  

   }

//<OBJECT ID="DISystemMonitor1" WIDTH="100%" HEIGHT="100%"
//CLASSID="CLSID:C4D2D8E0-D1DD-11CE-940F-008029004347">
//   <PARAM NAME="Counter00001.Path" VALUE="\\VOLTRON\.NET CLR Memory(PhoenixEditor)\# Bytes in all Heaps">
//   <PARAM NAME="Counter00002.Path" VALUE="\\VOLTRON\.NET CLR Memory(PhoenixEditor)\# of Pinned Objects">
//   <PARAM NAME="Counter00003.Path" VALUE="\Process(PhoenixEditor)\Private Bytes">
//   <PARAM NAME="Counter00004.Path" VALUE="\\VOLTRON\Process(PhoenixEditor)\Virtual Bytes">
//   <PARAM NAME="Counter00005.Path" VALUE="\Process(PhoenixEditor)\Working Set">
//   <PARAM NAME="Selected" VALUE="\\VOLTRON\.NET CLR Memory(PhoenixEditor)\# of Pinned Objects">
//</OBJECT>

}
