using System;
using System.Data;
using System.Configuration;
using System.Collections;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using System.Web.UI.HtmlControls;

using System.Collections.Specialized;


public partial class Status : System.Web.UI.Page
{
   private static BuildDB s_phxBuildDB = new BuildDB();


   protected void Page_Load(object sender, EventArgs e)
   {         
      phxbuild.BuildStatusDataTable buildStatusTable = s_phxBuildDB.GetBuildStatus();

      if (buildStatusTable.Count > 0)
      {
         if (!buildStatusTable[0].IsserverNull())
            serverStatusLabel1.Text = buildStatusTable[0].server;
         else
            serverStatusLabel1.Text = "";

         if (!buildStatusTable[0].IsjobNull())
            jobStatusLabel1.Text = buildStatusTable[0].job.Replace("\n", "<br>");
         else
            jobStatusLabel1.Text = "";
      }

      if (buildStatusTable.Count > 1)
      {
         if (!buildStatusTable[1].IsserverNull())
            serverStatusLabel2.Text = buildStatusTable[1].server;
         else
            serverStatusLabel2.Text = "";

         if (!buildStatusTable[1].IsjobNull())
            jobStatusLabel2.Text = buildStatusTable[1].job.Replace("\n", "<br>");
         else
            jobStatusLabel2.Text = "";
      }
   }

   protected void Timer1_Tick(object sender, EventArgs e)
   {
   }
}
