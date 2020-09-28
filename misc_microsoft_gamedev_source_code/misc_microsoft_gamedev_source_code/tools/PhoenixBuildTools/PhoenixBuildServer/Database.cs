using System;
using System.Collections.Generic;
using System.Text;

using System.Data.SqlClient;
using System.Data;

namespace PhoenixBuildServer
{
   class Database
   {
      static private SqlConnection mDBConnection = new SqlConnection();
      static private int mStatusStringMaxSize = 100;

      static public int StatusStringMaxSize
      {
         get
         {
            return mStatusStringMaxSize;
         }
      }

      static public SqlConnection DBConnection
      {
         get
         {
            return mDBConnection;
         }
      }

      #region Public Functions

      static public bool init()
      {
         bool success;

         // Init DB
         //
         success = initDBConnection();

         if (!success)
         {
            return (success);
         }


         // Init Status field size
         //
         success = initDBStatusSize();

         if (!success)
         {
            return (success);
         }

         return (true);
      }

      //-- Checks for the db value being null. Do this check before you try to index into
      //-- the SqlDataReader
      static private bool isDBNULL(string columnName, SqlDataReader rs)
      {
         int column = rs.GetOrdinal(columnName);
         return (rs.IsDBNull(column));
      }

      static public SqlDataReader exSQL(string query, bool nonquery)
      {
         mDBConnection.Close();

         SqlCommand cmd = new SqlCommand();
         cmd.CommandText = query;
         cmd.Connection = mDBConnection;
         mDBConnection.Open();
         if (nonquery == true)
         {
            cmd.ExecuteNonQuery();
            mDBConnection.Close();
            return (null);
         }

         return (cmd.ExecuteReader());
      }


      static public int getFirstJobFromQueueAndSetStatus(int serverID)
      {
         // Does two things at the same time.  It queries to see if there is a job available to process, and if so it will set it's state to running.  This
         // needs to be done in the same DB connection to prevent multiple BuildServers to start working on the same job.
         //
         int queuenum = -1;

         try
         {
            mDBConnection.Close();

            mDBConnection.Open();

#if true
            // Check if there are any available jobs to start working on.
            //
            {
               string query = "SELECT * FROM " + Globals.Config.getString(Globals.Config.cDBQueueTableName) + " WHERE running = '0'" + " order by queuenum asc";

               switch (serverID)
               {
                  case 1:
                     query = "SELECT * FROM " + Globals.Config.getString(Globals.Config.cDBQueueTableName) + " WHERE running = '0'" + " order by queuenum asc";
                     break;
                  case 2:
                     query = "SELECT * FROM " + Globals.Config.getString(Globals.Config.cDBQueueTableName) + " WHERE running = '0' AND (buildconfig <> 0) AND (buildconfig <> 18) AND (buildconfig <> 27) AND (buildconfig <> 28)" + " order by queuenum asc";
                     break;
               }

               SqlCommand cmd = new SqlCommand();
               cmd.CommandText = query;
               cmd.Connection = mDBConnection;


               SqlDataReader rs = cmd.ExecuteReader();
               if (rs.Read() == true)
               {
                  queuenum = (int)rs["queuenum"];
               }
               rs.Close();
            }

            // If we found a job set the status to running so no other buildserver will start working on it.
            //
            if (queuenum != -1)
            {
               string query = "UPDATE " + Globals.Config.getString(Globals.Config.cDBQueueTableName) + 
                              " SET running = '1', status = 'running on " + Environment.MachineName + "', timestart = '" +
                              DateTime.Now.ToString() + "' WHERE queuenum = " + queuenum;

               SqlCommand cmd = new SqlCommand();
               cmd.CommandText = query;
               cmd.Connection = mDBConnection;

               cmd.ExecuteNonQuery();
            }

#else
            string statusString = "running on " + Environment.MachineName;
            string whereSection = " WHERE running = '0'" + " order by queuenum asc";

            switch (serverID)
            {
               case 1:
                  whereSection = " WHERE running = '0'" + " order by queuenum asc";
                  break;
               case 2:
                  whereSection = " WHERE running = '0' AND (buildconfig <> 0) AND (buildconfig <> 18) AND (buildconfig <> 19)" + " order by queuenum asc";
                  break;
            }

            // First try to update
            string query = "UPDATE " + Globals.Config.getString(Globals.Config.cDBQueueTableName) +
                           " SET running = '1', status = '" + statusString + "', timestart = '" +
                           DateTime.Now.ToString() + "'" + whereSection;

            SqlCommand cmd = new SqlCommand();
            cmd.CommandText = query;
            cmd.Connection = mDBConnection;

            int affectedRows = cmd.ExecuteNonQuery();

            if(affectedRows > 1)
               ....

            // Need to fix this so it only updates one entry and no more.
#endif


            mDBConnection.Close();
         }
         catch
         {
         }
         return (queuenum);
      }




      static public bool getJobFromQueue(int jobNumber, ref BuildQueueItem jobInfo)
      {
         string query = "SELECT * FROM " + Globals.Config.getString(Globals.Config.cDBQueueTableName) + " WHERE queuenum = " + jobNumber;

         try
         {
            SqlDataReader rs = exSQL(query, false);
            if (rs.Read() == true)
            {
               if (isDBNULL("queuenum", rs) == false)
                  jobInfo.queuenum = (int)rs["queuenum"];
               if (isDBNULL("buildnum", rs) == false)
                  jobInfo.buildnum = ((string)(rs["buildnum"])).Trim();
               if (isDBNULL("builduser", rs) == false)
                  jobInfo.builduser = ((string)(rs["builduser"])).Trim();
               if (isDBNULL("timestart", rs) == false)
                  jobInfo.timestart = ((string)(rs["timestart"])).Trim();
               if (isDBNULL("buildconfig", rs) == false)
                  jobInfo.buildconfig = (int)rs["buildconfig"];
               if (isDBNULL("buildparams", rs) == false)
                  jobInfo.buildparams = ((string)(rs["buildparams"])).Trim();
               if (isDBNULL("comment", rs) == false)
                  jobInfo.comment = ((string)(rs["comment"])).Trim();

               rs.Close();
               mDBConnection.Close();
               return (true);
            }
            else
            {
               rs.Close();
               mDBConnection.Close();
               return (false);
            }
         }
         catch
         {
         }
         return (false);
      }

      // Remove Job from queue
      static public void removeJobFromQueue(int jobNumber)
      {
         string query = "DELETE FROM " + Globals.Config.getString(Globals.Config.cDBQueueTableName) + " WHERE queuenum = " + jobNumber;

         try
         {
            exSQL(query, true);
         }
         catch
         {
            throw;
         }
      }


      static public void setRunStatus(int jobNumber)
      {
         try
         {
            //logServer("Setting run status");

            string query = "UPDATE " + Globals.Config.getString(Globals.Config.cDBQueueTableName) +
               " SET running = '1', status = 'running on " + Environment.MachineName + "', timestart = '" + DateTime.Now.ToString() + "' WHERE queuenum = " + jobNumber;
            exSQL(query, true);
         }
         catch
         {
            throw;
         }
      }


      static public bool getBuildTypeFromTypes(int buildConfig, ref BuildTypeItem typeItem)
      {
         string query = "SELECT * FROM " + Globals.Config.getString(Globals.Config.cDBBuildTypes) + " WHERE Id = " + buildConfig;

         try
         {
            SqlDataReader rs = exSQL(query, false);
            if (rs.Read() == true)
            {
               if (isDBNULL("Perms", rs) == false)
                  typeItem.perms = (int)rs["Perms"];
               if (isDBNULL("Name", rs) == false)
                  typeItem.name = ((string)(rs["Name"])).Trim();
               if (isDBNULL("Description", rs) == false)
                  typeItem.description = ((string)(rs["Description"])).Trim();
               if (isDBNULL("Type", rs) == false)
                  typeItem.type = ((string)(rs["Type"])).Trim();
               if (isDBNULL("Extra", rs) == false)
                  typeItem.extra = ((string)(rs["Extra"])).Trim();

               rs.Close();
               mDBConnection.Close();
               return (true);
            }
            else
            {
               rs.Close();
               mDBConnection.Close();
               return (false);
            }
         }
         catch
         {
         }
         return (false);
      }



      static public bool logDB(string logServerTxt, string logJobTxt, int serverID)
      {
         try
         {
            if ((logJobTxt != null) && (logJobTxt.Length > (mStatusStringMaxSize - 1)))
            {
               logJobTxt = logJobTxt.Substring((logJobTxt.Length - (mStatusStringMaxSize)), mStatusStringMaxSize);
            }

            string query = "UPDATE " + Globals.Config.getString(Globals.Config.cDBStatusTableName) + " SET server " +
                   " = '" + logServerTxt + "', job = '" + logJobTxt + "' WHERE id = '" + serverID.ToString() + "'";

            exSQL(query, true);
            return (true);
         }
         catch
         {
         }
         return (false);
      }

      // Inserts various fields from the current running job into the history table
      static public bool insertHistory(BuildQueueItem jobInfo, string comments, string results, string date)
      {
         try
         {
            // Need to replace ' with '' for SQL insert
            comments = comments.Replace("'", "`");
            results = results.Replace("'", "`");

            int maxInsertSize = Globals.Config.getInt(Globals.Config.cMaxInsertSize);
            if (comments.Length > maxInsertSize)
            {
               comments = comments.Substring(0, maxInsertSize);
            }
            if (results.Length > maxInsertSize)
            {
               results = results.Substring(0, maxInsertSize);
            }

            string query = "INSERT INTO " + Globals.Config.getString(Globals.Config.cDBHistoryTableName) +
               " (buildnum, builduser, buildstart, buildend, results, comments, buildconfig) VALUES ('" +
               jobInfo.buildnum + "', '" + jobInfo.builduser + "', '" + jobInfo.timestart + "', '" + date +
               "', '" + results + "', '" + comments +
               "', '" + jobInfo.buildconfig + "')";

            exSQL(query, true);
            return (true);
         }
         catch
         {
         }
         return (false);
      }


      #endregion


      #region Private Functions


      static private bool initDBConnection()
      {
         //-- Close for good measure.
         mDBConnection.Close();

         //-- remake the connection string.
         mDBConnection.ConnectionString = "packet size=4096" +
                                          ";data source=" + Globals.Config.getString(Globals.Config.cDBServerName) +
                                          ";initial catalog=" + Globals.Config.getString(Globals.Config.cDBName) +
                                          ";user id=" + Globals.Config.getString(Globals.Config.cDBUserID) +
                                          ";password=" + Globals.Config.getString(Globals.Config.cDBPassword) +
                                          ";persist security info=True";

         // Need to test out connection string here.
         try
         {
            mDBConnection.Open();
            mDBConnection.Close();
         }
         catch
         {
            return (false);
         }

         return (true);
      }


      static private bool initDBStatusSize()
      {
         try
         {
            string query = "SELECT * FROM " + Globals.Config.getString(Globals.Config.cDBStatusTableName);
            SqlDataReader rs = exSQL(query, false);
            rs.Read();

            // make sure we don't exceed the maximun log size for the DB
            DataTable table = rs.GetSchemaTable();
            foreach (DataRow row in table.Rows)
            {
               if (((string)row["ColumnName"]) == "job")
               {
                  mStatusStringMaxSize = (int)row["ColumnSize"];
                  break;
               }
            }

            rs.Close();
         }
         catch
         {
            return (false);
         }

         return (true);
      }

      #endregion
   }
}
