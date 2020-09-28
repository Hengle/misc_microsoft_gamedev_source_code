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

public partial class Build : System.Web.UI.Page
{
   private static BuildDB s_phxBuildDB = new BuildDB();
   int mAccessLevel = 0;
   bool mBuildsLockedDown = false;

   string mUserName;


   string lockdownLabelLockStr = "[Build System is under Lockdown]";
   string lockdownLabelUnlockStr = "";

   protected void refreshHighLight()
   {
      // Highlight running builds

      // Early out if queue is empty
      if (buildQueueGridView.Rows.Count == 0)
         return;

      phxbuild.BuildQueueDataTable queueTable = s_phxBuildDB.GetBuildQueue();

      if (buildQueueGridView.Rows.Count != queueTable.Count)
         return;


      for (int i = 0; i < queueTable.Count; i++)
      {
         if ((queueTable[i].IsrunningNull() == false) && (queueTable[i].running.Trim() != "0"))
         {
            buildQueueGridView.Rows[i].BackColor = System.Drawing.Color.Chocolate;
         }
         else
         {
            buildQueueGridView.Rows[i].BackColor = System.Drawing.Color.Empty;
         }
      }
   }


   protected void Page_Load(object sender, EventArgs e)
   {
      // Get current user
      WindowsIdentity currentIdentity = WindowsIdentity.GetCurrent();
      int lastSlash = currentIdentity.Name.LastIndexOf('\\') + 1;
      mUserName = currentIdentity.Name.Substring(lastSlash, currentIdentity.Name.Length - lastSlash).ToLower();

      // Get user access level
      mAccessLevel = s_phxBuildDB.GetAccessByUserID(mUserName);

      // Get builds lockdown
      mBuildsLockedDown = s_phxBuildDB.GetStatusLockdown();


      if (!Page.IsPostBack)
      {         
         // Populate controls
         //

         // BuildTypes radio buttons
         phxbuild.BuildTypesDataTable buildTypesTable = s_phxBuildDB.GetBuildTypesSorted();
         buildTypeRadioButtonList.Items.Clear();

         for (int i = 0; i < buildTypesTable.Count; i++)
         {
            bool bIsItemEnabled = false;
            if (mAccessLevel >= buildTypesTable[i].Perms)
            {
               bIsItemEnabled = true;
            }

            string typeName = null;
            if (!buildTypesTable[i].IsNameNull())
               typeName = buildTypesTable[i].Name;
            else
               typeName = "Unkown";

            string typeDescription = null;
            if(!buildTypesTable[i].IsDescriptionNull())
               typeDescription = buildTypesTable[i].Description;

            string name;
            if ((typeName != null) && (typeDescription != null))
               name = typeName.Trim() + ":  " + typeDescription.Trim();
            else
               name = typeName.Trim();

            string value = buildTypesTable[i].Id.ToString();

            buildTypeRadioButtonList.Items.Add(new ListItem(name, value, bIsItemEnabled));

            if ((buildTypeRadioButtonList.SelectedIndex == -1) && bIsItemEnabled)
            {
               buildTypeRadioButtonList.SelectedIndex = i;
            }
         }
      }
      else
      {
         buildQueueGridView.DataBind();
      }


      if (mBuildsLockedDown == true)
      {
         lockdownStateLabel.Text = lockdownLabelLockStr;
      }
      else
      {
         lockdownStateLabel.Text = lockdownLabelUnlockStr;
      }

      if (mBuildsLockedDown && (mAccessLevel < 6))
      {
         typeLabel.Enabled = false;
         buildTypeRadioButtonList.Enabled = false;
         commentsLabel.Enabled = false;
         buildCommentTextBox.Enabled = false;
         buildButton.Enabled = false;
      }
      else
      {
         typeLabel.Enabled = true;
         buildTypeRadioButtonList.Enabled = true;
         commentsLabel.Enabled = true;
         buildCommentTextBox.Enabled = true;
         buildButton.Enabled = true;
      }

      // Refresh grid view colors
      refreshHighLight();
   }



   protected void buildButton_Click(object sender, EventArgs e)
   {
      // get the build type
      int buildType = Convert.ToInt32(buildTypeRadioButtonList.SelectedItem.Value);

      // verify the type
      if(buildType < 0)
      {
         return;
      }


      // get the build comment
      string buildCommentStr = buildCommentTextBox.Text;

      // verify the buid Comment
      if (buildCommentStr.Length <= 0)
      {
         return;
      }


      if (s_phxBuildDB.isBuildTypePending(buildType) == true)
      {
         // Add comment to pending build
         s_phxBuildDB.updatePendingBuildUserAndComment(buildType, mUserName, buildCommentStr);
      }
      else
      {
         // Insert new build in queue
         s_phxBuildDB.insertBuildToBuildQueue("10", mUserName, null, "waiting", "0",
                                               buildType, null, buildCommentStr);

      }

      buildQueueGridView.DataBind();

      // Reset comment
      buildCommentTextBox.Text = "";
   }
   protected void Label1_DataBinding(object sender, EventArgs e)
   {
      Label label = (Label)sender;

      if (String.IsNullOrEmpty(label.Text))
         return;

      label.Text = label.Text.Replace("\n", "<br>");
   }
   protected void Label2_DataBinding(object sender, EventArgs e)
   {
      Label label = (Label)sender;

      if (String.IsNullOrEmpty(label.Text))
         return;

      label.Text = s_phxBuildDB.GetBuildTypeNameByID(Convert.ToInt32(label.Text));
   }
   protected void buildQueueGridView_RowDeleting(object sender, GridViewDeleteEventArgs e)
   {
      // Early out - always allow deletion for manager
      mAccessLevel = s_phxBuildDB.GetAccessByUserID(mUserName);
      if (mAccessLevel >= 6)
      {
         return;
      }

      
      // Only allow deletions if not deleting running build
      object value = e.Values["status"];
      if (value == null)
         return;

      string statusStr = value.ToString().Trim();

      if (String.Compare(statusStr, "waiting", true) != 0)
      {
         e.Cancel = true;
      }
   }


   protected void buildQueueGridView_RowCreated(object sender, GridViewRowEventArgs e)
   {
      // Refresh grid view colors
      refreshHighLight();
   }

   protected void buildQueueGridView_RowDeleted(object sender, GridViewDeletedEventArgs e)
   {
      // Refresh grid view colors
      refreshHighLight();
   }
}
