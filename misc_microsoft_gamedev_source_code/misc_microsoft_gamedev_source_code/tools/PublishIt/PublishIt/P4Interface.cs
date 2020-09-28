using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

using System.Windows.Forms;
using P4API;

namespace PublishIt
{
   class P4Interface
   {
		public String clientRoot;
      public String clientPath;
      private String depotPath;
      private bool validConnection = false;
      private P4API.P4Connection p4;


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

      private void BuildClientDepotPath()
      {
         P4Form myClient = p4.Fetch_Form("client");

         String[] newView = new String[myClient.ArrayFields["View"].Length + 1];
         myClient.ArrayFields["View"].CopyTo(newView, 0);

         String client = myClient["Client"];
         clientRoot = myClient["Root"];

			if (newView[0].Contains("playtest") )
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

		public P4RecordSet Run(String command, String[] args)
		{
		   try
		   {
            P4RecordSet results = p4.Run(command, args);
            return (results);			   
		   }
		   catch (System.Exception e)
		   {
		   	MessageBox.Show(e.Message, "Perforce Error!");
		   	return null;
		   }
		}

       public P4RecordSet Run(String command)
       {
           try
           {
               P4RecordSet results = p4.Run(command);
               return (results);
           }
           catch (System.Exception e)
           {
               MessageBox.Show(e.Message, "Perforce Error!");
               return null;
           }
       }

      public P4RecordSet GetSyncFiles(String[] path)
		{
			String[] args = new String[path.Length + 1];
			args[0] = "-n";
			for (int i = 1; i < args.Length; i++)
				args[i] = path[i - 1];

			p4.Connect();
			P4RecordSet syncFiles = Run("sync", args);
			p4.Disconnect();

			return (syncFiles);
		}

       public int Integrate(ArrayList files)
       {
           P4RecordSet record;
           P4RecordSet resolve;


           P4PendingChangelist integrateChangelist = p4.CreatePendingChangelist("PublishIt! changelsit");

           String tmp = "";
           String[] intArgs = { "-c", integrateChangelist.Number.ToString(), "-d", "-i", "-t", "-v", "-b", "phx_published", "" };
           String[] resArgs = { "-at", "" };

           foreach (String file in files)
           {
               tmp = file.Replace("\\depot\\phoenix\\xbox\\work", "//depot/phoenix/xbox/published");
               intArgs[8] = tmp.Replace("\\", "/");
               record = Run("integrate", intArgs);

               resArgs[1] = tmp.Replace("\\", "/");
               resolve = Run("resolve", resArgs);
           }

           String[] subArgs = { "-c", "" };
           subArgs[1] = integrateChangelist.Number.ToString();
           Run("submit", subArgs);
           
           return integrateChangelist.Number;
       }      
       
      public ArrayList GetUsers()
      {
            ArrayList userList = new ArrayList();
            
            P4RecordSet users = Run("users");
            
            foreach (P4Record user in users)
            {
                userList.Add( user.Fields["User"] );
            }     
            return userList;
      }
   }
}
