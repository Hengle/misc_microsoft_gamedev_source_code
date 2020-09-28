using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml.Serialization;
using System.Threading;
using System.ComponentModel;

using P4API;

namespace GenerateScenarioOpenFileLists
{
   class GenearateOpenFileList
   {
      static bool mIsWorking = false;
      static string gameDirectory = null;
      static Thread mWorkerThread = null;

      static string calculateGameDir()
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

      public enum eListFilter
      {
         eShowAll =0,
         ePlaytestOnly,
      }
      public static List<string> generateScenarioList(eListFilter filter)
      {
         if (gameDirectory == null)
            gameDirectory = calculateGameDir();

         List<string> scenarioList = new List<string>();
         string mDescriptionFilename = gameDirectory + @"\data\scenariodescriptions.xml";
         ScenarioDescriptionsXml mScenarioDescriptions= null;
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(ScenarioDescriptionsXml), new Type[] { });
            Stream st = File.OpenRead(mDescriptionFilename);
            mScenarioDescriptions = (ScenarioDescriptionsXml)s.Deserialize(st);
            st.Close();


            for (int i = 0; i < mScenarioDescriptions.ScenarioInfoList.Count; i++)
            {
               bool add = true;

               if (filter == eListFilter.ePlaytestOnly)
               {
                  add = mScenarioDescriptions.ScenarioInfoList[i].Type == "Campaign" ||
                        mScenarioDescriptions.ScenarioInfoList[i].Type == "Playtest";
               }

               if(add)
                  scenarioList.Add(mScenarioDescriptions.ScenarioInfoList[i].File);
            }
            

            return scenarioList;
         }
         catch (System.Exception ex)
         {
            
         }
         return null;
      }

      public static void stopWork()
      {
         mIsWorking = false;
      }
      public static bool IsWorking()
      {
         return mIsWorking;
      }

      public static void generateFileLists(List<string> scenariosToProcess)
      {
         mIsWorking = true;

         //spawn worker thread..
         mWorkerThread = new System.Threading.Thread(new System.Threading.ParameterizedThreadStart(generateFileListsinternal));
         mWorkerThread.Start(scenariosToProcess);
      }
      private static void generateFileListsinternal(object objin)
      {
         List<string> scenariosToProcess = objin as List<string>;

         //if we had some problem launching XFS, die out
         if (!XFSInterface.launchApp())
            return;


         //perforce connections
         P4Interface perforce = new P4Interface();
         P4PendingChangelist cl = perforce.createChangeList("openFileLists for archive reordering");


         //wait a bit after game launch to ensure that the game is loaded
         XFSInterface.launchGame();
         System.Threading.Thread.Sleep(10000);



         for(int i=0;i<scenariosToProcess.Count;i++)
         {
            //Has the user pressed 'cancel' ?
            if (mIsWorking == false)
            {
               sendStatusMsg("User stopped work");
               break;
            }

            string launchString = scenariosToProcess[i].Substring(0, scenariosToProcess[i].LastIndexOf(@"."));
            string scenariodir = gameDirectory + @"\scenario\" + scenariosToProcess[i].Substring(0, scenariosToProcess[i].LastIndexOf(@"\"));
            string fileOpenLogStr = scenariodir + @"\FileopenLog.txt";
            bool addFileToPerforce = true;

            sendStatusMsg("Processing " + scenariosToProcess[i]);

            //there's a bit of logic here that we have to do.
            //considering there's like 800 states that a file can be in wrt perforce...
            if (File.Exists(fileOpenLogStr))
            {
               P4FileStatus fstat = perforce.getFileStatus(fileOpenLogStr);

               //is the file checked into perforce?
               if (!fstat.IsFileInPerforce)
               {
                  addFileToPerforce = true;
               }
               else
               {
                  addFileToPerforce = false;

                  if(!fstat.IsFileCheckedOutByMe)
                  {
                     //checkout the file
                     //NOTE .txt isn't exclusive, so we don't care if someone else has it checked out.
                     perforce.checkoutFileToChangelist(fileOpenLogStr, cl);
                     sendStatusMsg("..Checking out " + fileOpenLogStr);
                  }
               }

               //this is a local delete so we can use the MOVE command;
               try
               {
                  File.Delete(fileOpenLogStr);
               }
               catch (System.Exception e)
               {

                  sendStatusMsg("..Error Deleting " + fileOpenLogStr);
                  sendStatusMsg("..Skipping File " + fileOpenLogStr);
                  continue;
               }

            }


            //do this a couple times to esnure it's actualy being done!
            for (int k = 0; k < 3; k++)
            {
               XFSInterface.clearOpenFileList();
               System.Threading.Thread.Sleep(100);
            }

            const int cNumSecondsToWait = 45;
            sendStatusMsg("..Launching " + launchString);
            XFSInterface.launchScenario(launchString);
            System.Threading.Thread.Sleep(cNumSecondsToWait * 1000);

            XFSInterface.safeOpenFileList();
            System.Threading.Thread.Sleep(10000); 


            //move the file from work to the directory
            do
            {
               try
               {
                  File.Move(gameDirectory + @"\FileopenLog.txt", fileOpenLogStr);
                  break;
               }
               catch (System.Exception e)
               {

               }
            } while (true);


            //check file back into perforce here.
            if(addFileToPerforce)
            {
               perforce.addFileToChangelist(fileOpenLogStr, cl);
               sendStatusMsg("..Adding " + fileOpenLogStr);
            }

            //update our status message on the main form
            sendStatusMsg("..Finished " + scenariosToProcess[i]);

            System.Threading.Thread.Sleep(5000); 
         }

         perforce.submitChangelist(cl);
         sendStatusMsg("..Changelist " + cl.Number + " submitted");
         perforce.Disconnect();

         mIsWorking = false;
         sendStatusMsg("!DONE");
      }
      private static void sendStatusMsg(string msg)
      {
         ThreadSafeMessageList.MessagePacket mp = new ThreadSafeMessageList.MessagePacket();
         mp.mCallback = Program.addStatusStringToForm;
         mp.mMessageID = msg;
         mp.mDataObject = null;
         Program.mMessageList.enqueueMessage(mp);

      }

      private string convertPathToPerforceDepotPath(string filepath,P4Interface p4)
      {
         string res = filepath;
         if(filepath.Contains(p4.clientPath))
         {
            res = filepath.Remove(0, gameDirectory.Length);
         }

         return res;
      }
   }

   [XmlRoot("ScenarioDescriptions")]
   public class ScenarioDescriptionsXml
   {
      List<ScenarioInfoXml> mScenarioInfo = new List<ScenarioInfoXml>();

      //[XmlArrayItem(ElementName = "ScenarioInfo", Type = typeof(PlayerPositionXML))]
      //[XmlArray("ScenarioInfo")]
      [XmlElement("ScenarioInfo", typeof(ScenarioInfoXml))]
      public List<ScenarioInfoXml> ScenarioInfoList
      {
         get
         {
            return mScenarioInfo;
         }
         set
         {
            mScenarioInfo = value;
         }
      }
   }

   [XmlRoot("ScenarioInfo")]
   public class ScenarioInfoXml
   {
      string mMapNameKeyFrame = "placeholder";
      string mMaxPlayers = "2";
      int mNameStringID = 0;
      int mInfoStringID = 0;
      string mType = "Test";
      string mScenario = "";

      [XmlAttribute]
      public string MapName
      {
         get
         {
            return mMapNameKeyFrame;
         }
         set
         {
            mMapNameKeyFrame = value;
         }
      }

      [XmlAttribute]
      public string MaxPlayers
      {
         get
         {
            return mMaxPlayers;
         }
         set
         {
            mMaxPlayers = value;
         }
      }


      [XmlAttribute]
      public int NameStringID
      {
         get
         {
            return mNameStringID;
         }
         set
         {
            mNameStringID = value;
         }
      }

      [XmlAttribute]
      public int InfoStringID
      {
         get
         {
            return mInfoStringID;
         }
         set
         {
            mInfoStringID = value;
         }
      }

      [XmlAttribute]
      public string Type
      {
         get
         {
            return mType;
         }
         set
         {
            mType = value;
         }
      }

      [XmlAttribute]
      public string File
      {
         get
         {
            return mScenario;
         }
         set
         {
            mScenario = value;
         }
      }
   }
}
