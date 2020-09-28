using System;
using System.Collections.Generic;
using System.Text;

namespace DepGenReporter
{
   class Change
   {
      private String filename;
      private int changeNumber;
      private int changeListNumber;
      private String user;
      private String description;
      private DateTime submitDate;
      
      public Change( String change, String file )
      {
         filename = file.TrimEnd('\r');
         changeNumber = int.Parse( change[1].ToString() );
         
         int start = change.IndexOf("change ") + 7;
         int len = 0;
         if ( change.Contains("edit") )
         {
            len = change.IndexOf(" edit") - (change.IndexOf("change ") + 7);
         }
         else if ( change.Contains("add") )
         {
            len = change.IndexOf(" add") - (change.IndexOf("change ") + 7);
         }
         else if (change.Contains(" delete") )
         {
            len = change.IndexOf(" delete") - (change.IndexOf("change ") + 7);
         }
         else if (change.Contains(" branch") )
         {
            len = change.IndexOf(" branch") - (change.IndexOf("change ") + 7);
         }
         else
            len = 0;
         changeListNumber = int.Parse( change.Substring( start, len) );
         
         start = change.IndexOf("by ") + 3;
         len = change.IndexOf("@") - (change.IndexOf("by ") + 3);
         user = change.Substring(start, len);

         start = change.IndexOf(") '") + 3;
         len = (change.Length - 3) - start;
         description = change.Substring(start, len);


         start = change.IndexOf("on ") + 3;
         len = change.IndexOf(" by") - start;
         String[] date = change.Substring(start, len).Split('/');
         submitDate = new DateTime( int.Parse(date[0]), int.Parse(date[1]), int.Parse(date[2]) );
                  
      }
      
      public String Filename
      {
         get { return filename; }
      }
      
      public int ChangeNumber
      {
         get { return changeNumber; }
      }
      
      public int ChangeListNumber
      {
         get { return changeListNumber; }
      }
      
      public String User
      {
         get{ return user; }
      }
      
      public String Description
      {
         get { return description; }
      }
      
      public DateTime SubmitDate
      {
         get { return submitDate; }
      }
      
      public String NextUser()
      {
         return null;
      }
   }
}
