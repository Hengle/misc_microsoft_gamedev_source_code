using System;
using System.Data;
using System.Configuration;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using System.Web.UI.HtmlControls;

using System.Data.SqlClient;
using phxbuildTableAdapters;

/// <summary>
/// Summary description for BuildDB
/// </summary>
public class BuildDB
{
   string         mConnectionString = null;
   SqlConnection  mConnection = null;

   public BuildDB()
   {
      //
      // TODO: Add constructor logic here
      //
      


      // Get the connection string from the .config file.
      mConnectionString = ConfigurationManager.ConnectionStrings["phxbuildConnectionString"].ConnectionString;
      mConnection = new SqlConnection(mConnectionString);
   }



   // ------------------------------------------------------------------------------------
   // BuildQueue Table
   // ------------------------------------------------------------------------------------
   public phxbuild.BuildQueueDataTable GetBuildQueue()
   {
      BuildQueueTableAdapter buildQueueEntriesAdapter = new BuildQueueTableAdapter();
      return (buildQueueEntriesAdapter.GetBuildQueue());
   }

   public void insertBuildToBuildQueue(string buildnum, string builduser, string timestart, string status, string running,
                                       int buildconfig, string buildparams, string comment)
   {
      BuildQueueTableAdapter buildQueueEntriesAdapter = new BuildQueueTableAdapter();
      buildQueueEntriesAdapter.Insert(buildnum, builduser, timestart, status, running, buildconfig, buildparams, comment);
   }

   public bool isBuildTypePending(int buildconfig)
   {
      BuildQueueTableAdapter buildQueueEntriesAdapter = new BuildQueueTableAdapter();
      phxbuild.BuildQueueDataTable queueTable = buildQueueEntriesAdapter.GetBuildQueueByConfig(buildconfig);

      bool isBuildPending = false;
      if (queueTable.Count > 0)
      {
         isBuildPending = true;
      }

      return (isBuildPending);
   }

   public void updatePendingBuildUserAndComment(int buildconfig, string user, string addedComment)
   {
      BuildQueueTableAdapter buildQueueEntriesAdapter = new BuildQueueTableAdapter();
      phxbuild.BuildQueueDataTable queueTable = buildQueueEntriesAdapter.GetBuildQueueByConfig(buildconfig);

      if (queueTable.Count > 0)
      {
         string comment = null;
         string originalComment = queueTable[0].comment;

         if (addedComment != null)
            comment = addedComment.Trim();

         if(originalComment != null)
            comment += "\r\n\r\n" + "From: " + queueTable[0].builduser.Trim() + "\r\n" + originalComment.Trim() + "\r\n";

         buildQueueEntriesAdapter.SetUserAndCommentByQueueNum(user, comment, queueTable[0].queuenum);
      }
   }


   // ------------------------------------------------------------------------------------
   // BuildHistory Table
   // ------------------------------------------------------------------------------------
   public phxbuild.BuildHistoryDataTable GetBuildHistory()
   {
      BuildHistoryTableAdapter buildHistoryEntriesAdapter = new BuildHistoryTableAdapter();
      return (buildHistoryEntriesAdapter.GetBuildHistory());
   }


   // ------------------------------------------------------------------------------------
   // BuildTypes Table
   // ------------------------------------------------------------------------------------
   public phxbuild.BuildTypesDataTable GetBuildTypes()
   {
      BuildTypesTableAdapter buildTypesAdapter = new BuildTypesTableAdapter();
      return (buildTypesAdapter.GetBuildTypes());
   }

   public phxbuild.BuildTypesDataTable GetBuildTypesSorted()
   {
      BuildTypesTableAdapter buildTypesAdapter = new BuildTypesTableAdapter();
      return (buildTypesAdapter.GetBuildTypesSorted());
   }

   public string GetBuildTypeNameByID(int id)
   {
      BuildTypesTableAdapter buildTypesAdapter = new BuildTypesTableAdapter();
      phxbuild.BuildTypesDataTable buildTypeTable = buildTypesAdapter.GetBuildTypeByID(id);

      string buildTypeName = "";
      if (buildTypeTable.Count > 0)
      {
         buildTypeName = buildTypeTable[0].Name;

         // Strip out "Build" from the end
         if (buildTypeName.LastIndexOf("Build") != -1)
         {
            buildTypeName = buildTypeName.Substring(0, buildTypeName.LastIndexOf("Build"));
         }
         buildTypeName = buildTypeName.Trim();
      }
      else
      {
         buildTypeName = "unknow (" + id + ")";
      }

      return (buildTypeName);
   }

   // ------------------------------------------------------------------------------------
   // AccessRights Table
   // ------------------------------------------------------------------------------------
   public int GetAccessByUserID(string username)
   {
      AccessRightsTableAdapter accessRightsAdapter = new AccessRightsTableAdapter();
      phxbuild.AccessRightsDataTable accessTable = accessRightsAdapter.GetAccessByUserID(username);

      int accessLevel = 0;
      if (accessTable.Count > 0)
      {
         accessLevel = accessTable[0].accesslevel;
      }

      return (accessLevel);
   }


   // ------------------------------------------------------------------------------------
   // BuildStatus Table
   // ------------------------------------------------------------------------------------
   public bool GetStatusLockdown()
   {
      BuildStatusTableAdapter buildStatusAdapter = new BuildStatusTableAdapter();
      phxbuild.BuildStatusDataTable statusTable = buildStatusAdapter.GetBuildStatus();

      bool bLockdown = false;
      if (statusTable.Count > 0)
      {
         bLockdown = (statusTable[0].lockdown != 0) ? true : false;
      }

      return (bLockdown);
   }

   public void SetStatusLockdown(bool status)
   {
      BuildStatusTableAdapter buildStatusAdapter = new BuildStatusTableAdapter();
      buildStatusAdapter.UpdateBuildStatusLockdown((status == false) ? 0 : 1);
   }

   public phxbuild.BuildStatusDataTable GetBuildStatus()
   {
      BuildStatusTableAdapter buildStatusAdapter = new BuildStatusTableAdapter();
      return (buildStatusAdapter.GetBuildStatus());
   }
}
