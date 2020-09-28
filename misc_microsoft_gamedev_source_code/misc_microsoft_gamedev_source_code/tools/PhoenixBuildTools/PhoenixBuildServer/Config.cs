using System;
using System.Collections.Generic;
using System.Text;

namespace PhoenixBuildServer
{
   public class Config : ConfigData
   {
      #region Enums
      public int cLogDest = 0;
      public int cBuildRoot = 0;
      public int cLogRoot = 0;
      public int cHistoryDebug = 0;
      public int cHistoryPlaytest = 0;
      public int cHistoryFinal = 0;
      public int cMaxInsertSize = 0;
      public int cMailTemplateFile = 0;
      public int cMailTemplateTagStart = 0;
      public int cMailTemplateTagEnd = 0;
      public int cMailTemplateReport = 0;
      public int cRoboCopyExe = 0;
      public int cSendMailCmd = 0;
      public int cBuildAlias = 0;
      public int cFailedBuildAlias = 0;
      public int cCleanupBatchFile = 0;
      public int cDBServerName = 0;
      public int cDBName = 0;
      public int cDBUserID = 0;
      public int cDBPassword = 0;
      public int cDBQueueTableName = 0;
      public int cDBHistoryTableName = 0;
      public int cDBStatusTableName = 0;
      public int cDBVersionTableName = 0;
      public int cDBBuildTypes = 0;
      public int cMailServer = 0;
      public int cPerforceLoginBatchFile = 0;
      public int cPerforceUser = 0;
      public int cPerforceClient = 0;
      #endregion

      public bool init(string path)
      {
         addData("LogDest", "");
         addData("BuildRoot", "");
         addData("LogRoot", "");
         addData("HistoryDebug", "");
         addData("HistoryPlaytest", "");
         addData("HistoryFinal", "");
         addData("MaxInsertSize", "");
         addData("MailTemplateFile", "");
         addData("MailTemplateTagStart", "");
         addData("MailTemplateTagEnd", "");
         addData("MailTemplateReport", "");
         addData("RoboCopyExe", "");
         addData("SendMailCmd", "");
         addData("BuildAlias", "");
         addData("FailedBuildAlias", "");
         addData("CleanupBatchFile", "");
         addData("DBServerName", "");
         addData("DBName", "");
         addData("DBUserID", "");
         addData("DBPassword", "");
         addData("DBQueueTableName", "");
         addData("DBHistoryTableName", "");
         addData("DBStatusTableName", "");
         addData("DBVersionTableName", "");
         addData("DBBuildTypes", "");
         addData("MailServer", "");
         addData("PerforceLoginBatchFile", "");
         addData("PerforceUser", "");
         addData("PerforceClient", "");


         cLogDest = getIndex("LogDest");
         cBuildRoot = getIndex("BuildRoot");
         cLogRoot = getIndex("LogRoot");
         cHistoryDebug = getIndex("HistoryDebug");
         cHistoryPlaytest = getIndex("HistoryPlaytest");
         cHistoryFinal = getIndex("HistoryFinal");
         cMaxInsertSize = getIndex("MaxInsertSize");
         cMailTemplateFile = getIndex("MailTemplateFile");
         cMailTemplateTagStart = getIndex("MailTemplateTagStart");
         cMailTemplateTagEnd = getIndex("MailTemplateTagEnd");
         cMailTemplateReport = getIndex("MailTemplateReport");
         cRoboCopyExe = getIndex("RoboCopyExe");
         cSendMailCmd = getIndex("SendMailCmd");
         cBuildAlias = getIndex("BuildAlias");
         cFailedBuildAlias = getIndex("FailedBuildAlias");
         cCleanupBatchFile = getIndex("CleanupBatchFile");
         cDBServerName = getIndex("DBServerName");
         cDBName = getIndex("DBName");
         cDBUserID = getIndex("DBUserID");
         cDBPassword = getIndex("DBPassword");
         cDBQueueTableName = getIndex("DBQueueTableName");
         cDBHistoryTableName = getIndex("DBHistoryTableName");
         cDBStatusTableName = getIndex("DBStatusTableName");
         cDBVersionTableName = getIndex("DBVersionTableName");
         cDBBuildTypes = getIndex("DBBuildTypes");
         cMailServer = getIndex("MailServer");
         cPerforceLoginBatchFile = getIndex("PerforceLoginBatchFile");
         cPerforceUser = getIndex("PerforceUser");
         cPerforceClient = getIndex("PerforceClient");


         bool ret = initFromFile(path);

         return (ret);
      }
   }

}
