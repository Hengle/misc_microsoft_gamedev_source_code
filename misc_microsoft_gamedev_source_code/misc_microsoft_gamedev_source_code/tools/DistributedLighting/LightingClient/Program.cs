using System;

using System.Collections;
using System.Collections.Generic;

using System.Xml;
using System.Xml.Serialization;

using System.IO;
using System.Threading;
using System.Windows.Forms;
using System.Diagnostics;

using System.Net;
using System.Web.Mail;

using Rendering;

namespace LightingClient
{
   public class Program
   {
      const string cVersion = "1.3.2";


      #region XML for file reading
      [XmlRoot("pendingJob")]
      public class pendingJobXML
      {
         [XmlAttribute]
         public DateTime lastModified;   //datetime

         [XmlAttribute]
         public string issuingSystemName;
         [XmlAttribute]
         public string issuingSourceFile;
         [XmlAttribute]
         public string workingJobGUID;

         [XmlAttribute]
         public AmbientOcclusion.eAOQuality quality;

         [XmlAttribute]
         public int totalNumberOfSections;
         [XmlAttribute]
         public int targetSectionIndex;


      };
      #endregion



      static bool mStayAlive = true;

      static void syncPerforce()
      {
         Console.WriteLine(".....Syncing Perforce");

         Process proc = new Process();
     
         proc.StartInfo.FileName = "cmd.exe";
         proc.StartInfo.Arguments = "/C " + AppDomain.CurrentDomain.BaseDirectory + @"\syncPerforce.bat";
         proc.Start();

         proc.WaitForExit(60000);

         Console.WriteLine(".....Perforce Sync Complete");
      }
      public static bool processJob(Stream fileStream, string filename,pendingJobXML pendingJob)
      {
         DateTime timeNow = DateTime.Now;
          //Do our AO calcs//////////////////////////////////////////////////////////////////////////
         Console.WriteLine(".Starting AO Gen");
         
            LightingClientMain client = new LightingClientMain();
            LightingClientMain.eReturnCodes ret = client.generateLighting(pendingJob.issuingSourceFile,
                                                   networkAOInterface.ResultDir + pendingJob.issuingSystemName,
                                                   pendingJob.issuingSystemName,
                                                   pendingJob.quality,
                                                   pendingJob.totalNumberOfSections,
                                                   pendingJob.targetSectionIndex);

            if (ret!= LightingClientMain.eReturnCodes.cRC_OK)
            {
               //invalidate our job, incase someone else tries to grab it before we delete it
               fileStream.Position = 0;
               fileStream.WriteByte(128);
               fileStream.WriteByte(128);
               fileStream.WriteByte(128);
               fileStream.Close();

               //if the error was a missing / corrupted TDL, delete our job so no one else grabs it
               if(ret == LightingClientMain.eReturnCodes.cRC_TDL_LOAD_ERROR ||
                  ret == LightingClientMain.eReturnCodes.cRC_TDL_NOT_FOUND)
                  try { File.Delete(filename); }         catch (Exception e) { };

               Console.ForegroundColor = ConsoleColor.Red;
               Console.WriteLine("....AO Gen FAILED");
               Console.ForegroundColor = ConsoleColor.White;

             
              
               

               return false;
            }
            else
            {
               Console.WriteLine("....AO Gen Complete");

               //This is tricky.. Hopefully no one will be scanning for availability as soon as we do this...
               //invalidate our job, incase someone else tries to grab it before we delete it
               fileStream.Position = 0;
               fileStream.WriteByte(128);
               fileStream.WriteByte(128);
               fileStream.WriteByte(128);
               fileStream.Close();
               try { File.Delete(filename); }catch (Exception ed) { };


               TimeSpan ts = DateTime.Now - timeNow;
               float totalTime = (float)ts.TotalMinutes;
               Console.WriteLine("Work Finished; Total Time = " + totalTime + " minutes");
               return true;
            }
            return true;
      }
      public static Stream findAvailableJob(ref string filename)
      {

         Stream fileStream = null;
         string[] files = null;
         
         try 
         {
            files = Directory.GetFiles(networkAOInterface.JobsDir, "*.aojob");
         }
         catch (Exception e)
         {

         }
         

         for (int i = 0; i < files.Length; i++)
         {
            try
            {
               fileStream = File.Open(files[i], FileMode.Open, FileAccess.ReadWrite, FileShare.None);

               //CLM an exception should fire before this occurs
               filename = files[i];
               return fileStream;
            }
            catch (Exception e)
            {
               
            }
         }
         return fileStream;
      }
      public static void pollManagerForAvailableJobs()
      {
         string filename ="";
         Stream fileStream = findAvailableJob(ref filename);
         if (fileStream == null)
            return;


         Console.WriteLine("Acquired pending job at " + DateTime.Now.ToString());
         Console.WriteLine("Job " + Path.GetFileName(filename));


         //update perforce
         syncPerforce();
         
         //we have a job. 
         pendingJobXML pendingJob = null;
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(pendingJobXML), new Type[] { });
            pendingJob = (pendingJobXML)s.Deserialize(fileStream);
         }
         catch(Exception e)
         {
            //this job file is invalid. Delete it so that no one else tries to grab it
            fileStream.Position = 0;
            fileStream.WriteByte(128);
            fileStream.WriteByte(128);
            fileStream.WriteByte(128);
            fileStream.Close();
            try { File.Delete(filename); }catch (Exception et) { };
         }
         

         bool processOK = false;
         if(pendingJob!=null)
            processOK = processJob(fileStream,filename,pendingJob);


         Console.WriteLine("Entering polling mode");
         Console.WriteLine("================================================");

      }



      static string computeGameDir()
      {
         string gameDir = AppDomain.CurrentDomain.BaseDirectory;// mBaseDirectory;

         if (gameDir.Contains("\\work\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\work\\")) + "\\work";
         }
         else if (gameDir.Contains("\\production\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\production\\")) + "\\production\\work";
         }
         else if (gameDir.Contains("\\xbox\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\xbox\\")) + "\\xbox\\work";
         }
         else if (gameDir.Contains("\\x\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\x\\")) + "\\x\\work";
         }
         else if (gameDir.Contains("\\X\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\X\\")) + "\\X\\work";
         }
         else
         {
            return "";
         }

         return gameDir;
      }

      public static void Main(string[] args)
      {
         Xceed.Zip.Licenser.LicenseKey = "ZIN23-BUSZZ-NND31-7WBA";

         Console.ForegroundColor = ConsoleColor.Yellow;
         Console.WriteLine("======Networked AO Gen client V" + cVersion + "=========");
         Console.ForegroundColor = ConsoleColor.White;

        

         if(!networkAOInterface.networkAccessAvailable())
         {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("ERROR : access to esfile\\phoenix not available");
            Console.ForegroundColor = ConsoleColor.White;
            return;
         }


         TerrainGlobals.mGameDir = computeGameDir();


         //create D3D as a static function
         PictureBox dumControl = new PictureBox();
         if (!BRenderDevice.createDevice(/*this*/ dumControl, 512, 512, false))
            return;

         TerrainGlobals.getLODVB().init();


         Console.WriteLine("Polling for work");
         
         //CLM General TRY-CATCH block here to grab anything else I've missed
         try
         {
            while (mStayAlive)
            {
               pollManagerForAvailableJobs();
               Thread.Sleep(1000);
            }
         }catch(Exception e)
         {
           //we've died somewhere. Write a crashlog with the exception and time.
            if (!Directory.Exists(AppDomain.CurrentDomain.BaseDirectory + "crashlogs"))
               Directory.CreateDirectory(AppDomain.CurrentDomain.BaseDirectory + "crashlogs");
            
            string errorLogName = AppDomain.CurrentDomain.BaseDirectory + "crashlogs\\" + Guid.NewGuid().ToString() +".txt";
            StreamWriter w = new StreamWriter(errorLogName, true);
            w.WriteLine(DateTime.Now.ToString());
            w.WriteLine(e.ToString());
            w.Close();

            //send an e-mail to COLT MCANLIS
            sendCrashEmail(errorLogName);

            //now, respawn our app
            Process proc = new Process();
            proc.StartInfo.FileName = AppDomain.CurrentDomain.BaseDirectory + "DistributedLightingClient.exe";
            proc.Start();

           // proc.WaitForExit(60000);
         }

         //another try block here, incase we can't shutdown..
         try
         {
            TerrainGlobals.getLODVB().destroy();
            BRenderDevice.destroyDevice();
         }catch(Exception e)
         {

         }
      }

      static void sendCrashEmail(string lognameToAttach)
      {
         try
         {
            MailMessage oMsg = new MailMessage();
            // TODO: Replace with sender e-mail address.
            oMsg.From = "cmcanlis@ensemblestudios.com";
            // TODO: Replace with recipient e-mail address.
            oMsg.To = "cmcanlis@ensemblestudios.com";
            oMsg.Subject = "!! Distributed Lighting Client Crash !!";

            // SEND IN HTML FORMAT (comment this line to send plain text).
            oMsg.BodyFormat = MailFormat.Html;
            oMsg.Body = "A distributed lighting client has encountered an unhandled exception.<br>";
            oMsg.Body += "Machine: " + System.Environment.MachineName + "<br>";
            oMsg.Body += "User: " + System.Environment.UserName + "<br>";
            oMsg.Body += "OS: " + System.Environment.OSVersion + "<br>";
            oMsg.Body += "CLR: " + System.Environment.Version + "<br>";

            oMsg.Priority = MailPriority.High;
               

            // ADD AN ATTACHMENT.
            // TODO: Replace with path to attachment.
            MailAttachment oAttch = null;
            if(lognameToAttach!=null)
            {
               String sFile = lognameToAttach;
               oAttch = new MailAttachment(sFile, MailEncoding.Base64);
               oMsg.Attachments.Add(oAttch);
            }
            

            // TODO: Replace with the name of your remote SMTP server.
            SmtpMail.SmtpServer = "ensexch.ENS.Ensemble-Studios.com";
            SmtpMail.Send(oMsg);

            oMsg = null;
            oAttch = null;
         }
         catch (Exception e)
         {
            Console.WriteLine("{0} Exception caught.", e);
         }
      }



      private class networkAOInterface
      {
         #region directories
         static public string cDistrubtedWorkRootDir = @"\\esfile\phoenix\Tools\DistributedLighting\";
         static private string cJobsFileDir = @"jobFiles\";
         static private string cSourceFileDir = @"sourceFiles\";
         static private string cResultFileDir = @"resultFiles\";   //subdirs by issuing system name

         static public string JobsDir
         {
            get { return cDistrubtedWorkRootDir + cJobsFileDir; }
         }
         static public string SourceDir
         {
            get { return cDistrubtedWorkRootDir + cSourceFileDir; }
         }
         static public string ResultDir
         {
            get { return cDistrubtedWorkRootDir + cResultFileDir; }
         }

         #endregion

         static public bool networkAccessAvailable()
         {
            return Directory.Exists(@"\\esfile\phoenix\");
         }

         static public void ensureDirectories()
         {
            if (!Directory.Exists(cDistrubtedWorkRootDir))
               Directory.CreateDirectory(cDistrubtedWorkRootDir);

            if (!Directory.Exists(cDistrubtedWorkRootDir + cJobsFileDir))
               Directory.CreateDirectory(cDistrubtedWorkRootDir + cJobsFileDir);

            if (!Directory.Exists(cDistrubtedWorkRootDir + cSourceFileDir))
               Directory.CreateDirectory(cDistrubtedWorkRootDir + cSourceFileDir);

            if (!Directory.Exists(cDistrubtedWorkRootDir + cResultFileDir))
               Directory.CreateDirectory(cDistrubtedWorkRootDir + cResultFileDir);
         }
      };
   }

}


