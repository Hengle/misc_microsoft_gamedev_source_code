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

using System.Security.Principal;

public partial class Manage : System.Web.UI.Page
{
   private static BuildDB s_phxBuildDB = new BuildDB();
   string mUserName;

   string lockdownButtonLockStr = "Lockdown Builds";
   string lockdownButtonUnlockStr = "Unlock Builds";


   string lockdownLabelLockStr = "Build System is under Lockdown";
   string lockdownLabelUnlockStr = "Build System is unlocked";

   protected void Page_Load(object sender, EventArgs e)
   {
      // Get current user
      WindowsIdentity currentIdentity = WindowsIdentity.GetCurrent();
      int lastSlash = currentIdentity.Name.LastIndexOf('\\') + 1;
      mUserName = currentIdentity.Name.Substring(lastSlash, currentIdentity.Name.Length - lastSlash).ToLower();

      // Get user access level
      int accessLevel = s_phxBuildDB.GetAccessByUserID(mUserName);

      // Get builds lockdown
      bool bLockdown = s_phxBuildDB.GetStatusLockdown();
      setLockdownControlText(bLockdown);

      if (accessLevel >= 6)
      {
         managePanel.Visible = true;
         manageDisabledPanel.Visible = false;
      }
      else
      {
         managePanel.Visible = false;
         manageDisabledPanel.Visible = true;
      }
   }


   protected void setLockdownControlText(bool bLockdown)
   {
      if (bLockdown == true)
      {
         lockdownStateLabel.Text = lockdownLabelLockStr;
         lockdownButton.Text = lockdownButtonUnlockStr;
      }
      else
      {
         lockdownStateLabel.Text = lockdownLabelUnlockStr;
         lockdownButton.Text = lockdownButtonLockStr;
      }
   }


   protected void lockdownButton_Click(object sender, EventArgs e)
   {
      bool bLockdown = s_phxBuildDB.GetStatusLockdown();

      if (bLockdown == true)
      {
         // Unlock builds
         s_phxBuildDB.SetStatusLockdown(false);
      }
      else
      {
         // Lock builds
         s_phxBuildDB.SetStatusLockdown(true);
      }

      setLockdownControlText(!bLockdown);
   }
}
