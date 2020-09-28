
using System.Management;
using System.Collections.Generic;

namespace EditorCore
{
   public class ProcessorInfo
   {
      int mNumPhysicalProcessors = 0;
      public int NumPhysicalProcessors
      {
         get 
         {
            return mNumPhysicalProcessors;
         }
      }

      int mNumLogicalProcessors = 0;
      public int NumLogicalProcessors
      {
         get
         {
            return mNumLogicalProcessors;
         }
      }

      bool mHyperThreadingEnabled = false;
      public bool HyperThreadingEnabled
      {
         get
         {
            return mHyperThreadingEnabled;
         }
      }


      public ProcessorInfo()
      {
         List<string> Sockets = new List<string>();

         ManagementClass mc = new ManagementClass("Win32_Processor");
         ManagementObjectCollection moc = mc.GetInstances();//Populate class with Processor objects

         //Iterate through logical processors
         foreach (ManagementObject mo in moc)
         {
            mNumLogicalProcessors++;

            string SocketDesignation = mo.Properties["SocketDesignation"].Value.ToString();
            //We will count the unique SocketDesignations to find
            //the number of physical CPUs in the system.
            if (!Sockets.Contains(SocketDesignation))
               Sockets.Add(SocketDesignation);
         }

         mNumPhysicalProcessors = Sockets.Count;

         //Are there more logical than physical cpus?
         //If so, obviously we are hyperthreading.
         mHyperThreadingEnabled = (mNumLogicalProcessors > mNumPhysicalProcessors);
         
      }
   };
}