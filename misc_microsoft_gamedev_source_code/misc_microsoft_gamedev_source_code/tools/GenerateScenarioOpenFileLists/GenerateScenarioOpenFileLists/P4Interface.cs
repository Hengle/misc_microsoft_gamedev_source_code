using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Text;

using P4API;

public class P4Interface
{
   public String clientRoot;
   public String clientPath;
   private String depotPath;
   private P4API.P4Connection p4=null;

   //---------------------------------
   public P4Interface()
   {
      p4 = new P4Connection();
      p4.ExceptionLevel = P4ExceptionLevels.NoExceptionOnWarnings;
			//p4.ExceptionLevel = P4ExceptionLevels.NoExceptionOnErrors;
			//p4.ExceptionLevel = P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;
      bool test = Connect();

      if (IsValidConnection())
         BuildClientDepotPath();
   }

   //---------------------------------
   public bool Connect()
   {
		try
		{
			p4.Connect();
		}
		catch (Exception ex)
		{
			return false;
		}
		if ( p4.IsValidConnection(true, true) )
			return true;
		else
			return false;
   }

	public bool Disconnect()
	{
		p4.Disconnect();
		return true;
	}

	public bool IsValidConnection()
	{
		return p4.IsValidConnection(true, true);
	}
   //---------------------------------
   private void BuildClientDepotPath()
   {
      P4Form myClient = p4.Fetch_Form("client");

      String[] newView = new String[myClient.ArrayFields["View"].Length + 1];
      myClient.ArrayFields["View"].CopyTo(newView, 0);

      String client = myClient["Client"];
      clientRoot = myClient["Root"];

      if (newView[0].Contains("playtest"))
      {
         newView[0] = newView[1];
      }
      String[] pathConstructor = newView[0].Split('/');
      for (int i = 0; i < pathConstructor.Length - 2; i++)
      {
         pathConstructor[i] = pathConstructor[i + 2];
      }
      pathConstructor[pathConstructor.Length - 1] = pathConstructor[pathConstructor.Length - 2] = null;

      String[] pathArr = newView[0].Split(' ');
      String path = pathArr[1].Replace(String.Concat("//", client), clientRoot);
      clientPath = path.Replace("\\", "/");
      clientPath = clientPath.Replace("//", "/");
      clientPath = clientPath.Replace("/...", "");
      depotPath = pathArr[1];

   }
   //---------------------------------
   private P4RecordSet RunCmd(String command, String[] args)
   {
      try
      {
         if (IsValidConnection())
            return (p4.Run(command, args));
      }
      catch (P4API.Exceptions.RunException e)
      {
         System.Windows.Forms.MessageBox.Show(e.Message);
      }
      return null;
   }
   //---------------------------------
   public P4RecordSet GetSyncFiles(String[] path)
   {
      String[] args = new String[path.Length + 1];
      args[0] = "-n";
      for (int i = 1; i < args.Length; i++)
         args[i] = path[i - 1];

      p4.Connect();
      P4RecordSet syncFiles = RunCmd("sync", args);
      p4.Disconnect();

      return (syncFiles);
   }
   
   public P4RecordSet SyncFiles(String[] path)
   {
      p4.Connect();
      P4RecordSet syncFiles = RunCmd("sync", path);
      p4.Disconnect();

      return (syncFiles);
   }

   //---------------------------------
   public P4PendingChangelist createChangeList(string description)
   {
      if (p4 == null)
         return null;

      return p4.CreatePendingChangelist(description);
   }
   public void submitChangelist(P4PendingChangelist list)
   {
      try
      {
         list.Submit();
      }catch (Exception e)
      {

      }
   }
   //---------------------------------

   public bool checkoutFileToChangelist(string filepath,P4PendingChangelist list )
   {
      string[] args = new string[] { "-c", list.Number.ToString(), filepath };
      P4RecordSet setRes = RunCmd("edit", args); // "-c", cl.Number.ToString(), "//depot/path/foo.cs", "//depot/path/bar.cs");
      return true;
   }

   public void addFileToChangelist(string filepath, P4PendingChangelist list)
   {
      string[] args = new string[] { "-c", list.Number.ToString(), filepath };
      RunCmd("add", args); // "-c", cl.Number.ToString(), "//depot/path/foo.cs", "//depot/path/bar.cs");   
   }

   public bool revertFileInChangelist(string filepath, P4PendingChangelist list)
   {
      string[] args = new string[] { "-c", list.Number.ToString(), filepath };
      P4RecordSet setRes = RunCmd("revert", args); // "-c", cl.Number.ToString(), "//depot/path/foo.cs", "//depot/path/bar.cs");
      return true;
   }

   //---------------------------------
   public P4FileStatus getFileStatus(string filepath)
   {
      string[] args = new string[] { "-m1",filepath };
      P4RecordSet setRes = RunCmd("fstat", args);
      P4FileStatus fs = new P4FileStatus();
      fs.parseFromRecordset(setRes);
      return fs;
   }

   };

   public class P4FileStatus
   {

      bool mIsFileInPerforce = false;
      public bool IsFileInPerforce
      {
         get
         {
            return mIsFileInPerforce;

         }
      }

      bool mIsFileLatestRevision = false;
      public bool IsFileLatestRevision
      {
         get
         {
            return mIsFileLatestRevision;

         }
      }

      bool mIsFileCheckedOutByMe = false;
      public bool IsFileCheckedOutByMe
      {
         get
         {
            return mIsFileCheckedOutByMe;

         }
      }

      bool mIsFileCheckedOutByOther = false;
      public bool IsFileCheckedOutByOther
      {
         get
         {
            return mIsFileCheckedOutByOther;

         }
      }

      public bool IsFileCheckedOut
      {
         get
         {
            return IsFileCheckedOutByMe || IsFileCheckedOutByOther;
         }
      }

      public void parseFromRecordset(P4RecordSet rs)
      {
         if(rs.HasErrors())
         {

         }

         if(rs.HasWarnings())
         {
            for(int i=0;i<rs.Warnings.Length;i++)
            {
               string warning= rs.Warnings[i];
               if (warning.Contains("no such file(s)"))
                  mIsFileInPerforce = false;
            }
         }

         
         if(rs.Records.Length !=0)
         {
             P4Record stat = rs.Records[0];

            //set the bools
            mIsFileInPerforce = stat["headAction"] != "delete";
            mIsFileLatestRevision = stat["headRev"] == stat["haveRev"];
            mIsFileCheckedOutByMe = stat["action"] == "actionOwner";
            mIsFileCheckedOutByOther = stat["otherOpen0"] != null;
         }
      }
}