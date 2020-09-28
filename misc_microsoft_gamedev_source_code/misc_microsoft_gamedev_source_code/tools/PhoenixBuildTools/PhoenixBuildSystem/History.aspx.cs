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

public partial class History : System.Web.UI.Page
{
   private static BuildDB s_phxBuildDB = new BuildDB();

   protected void Page_Load(object sender, EventArgs e)
   {

   }

   protected void Label2_DataBinding(object sender, EventArgs e)
   {
      Label label = (Label)sender;

      if (String.IsNullOrEmpty(label.Text))
         return;

      label.Text = label.Text.Replace("\n", "<br>");
   }
   protected void Label1_DataBinding(object sender, EventArgs e)
   {
      Label label = (Label)sender;

      if (String.IsNullOrEmpty(label.Text))
         return;

      label.Text = label.Text.Replace("\n", "<br>");
   }
   protected void Label3_DataBinding(object sender, EventArgs e)
   {
      Label label = (Label)sender;

      if (String.IsNullOrEmpty(label.Text))
         return;

      label.Text = s_phxBuildDB.GetBuildTypeNameByID(Convert.ToInt32(label.Text));
   }
}
