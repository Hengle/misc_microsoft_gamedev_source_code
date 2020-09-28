using System;
using System.Collections.Generic;
using System.Text;

using System.Collections;
using System.Windows.Forms;

using P4API;
using p4dn;

namespace DepGenReporter
{
    class Error
    {
        private String originatingFile;
        private String referencedFile;
        
        private ArrayList originatingChanges;
        private ArrayList referencedChanges;
        
        private String description;
        
        public ArrayList user;
        
        public Error(String line)
        {
            user = new ArrayList();
            
            originatingChanges = new ArrayList();
            referencedChanges = new ArrayList();
                
            switch ( line[9] )
            {
            case 'W' : //warning
               DeconstructWarning(line);
               GetChangeHistory(originatingFile);
               GetChangeHistory(referencedFile);               
            	break;
            case 'E' : //warning
               DeconstructError(line);
               GetChangeHistory(originatingFile);
               GetChangeHistory(referencedFile);
            	break;
            default:
                break;          	
            	
            }

            foreach (Object chg in referencedChanges)
            {
               user.Add( ((Change)chg).User );
            }
        }

        public String OriginatingFile
        {
            get { return originatingFile; }
        }
        
        public String ReferencedFile
        {
            get { return referencedFile; }
        }

       public ArrayList ReferencedChanges
        {
            get { return referencedChanges; }
        }

       public ArrayList OriginatingChanges
       {
          get { return originatingChanges; }
       }
       
       public String Description
       {
            get { return description; }
       }
       
        private void DeconstructWarning(string warning)
        {
            String[] tmp = warning.Split(' ');
            
            if (tmp[5] == "depends")
            {
               originatingFile = tmp[4].Replace("\"", "");
               referencedFile = tmp[8].Replace("\"", "");            
            }
            else if (tmp[5] == "Visual")
            {
               originatingFile = tmp[3].Replace("\"", "");
               referencedFile = tmp[7].Replace("\"", "");
            }
            else if (tmp[5] == "is")
            {
               if (tmp[8] == "pfx")
               {
                  originatingFile = tmp[4].Replace("\"", "");
                  referencedFile = tmp[10].Replace("\"", "");                   
               }
               else
               {
                  originatingFile = tmp[4].Replace("\"", "");
                  referencedFile = tmp[9].Replace("\"", "");               
               }
            }
            else if (tmp[5] == "in")
            {
               originatingFile = tmp[3].Replace("\"", "");
               originatingFile = tmp[11].Replace("\"", "");
            }
        }
          
        private void DeconstructError(String error)
        {
            String[] tmp = error.Split(' ');
            originatingFile = tmp[4].Replace("\"", "");
            referencedFile = tmp[9].Replace("\"", "");            
        }
        
        private void GetChangeHistory( String file )
        {
            String fi = String.Concat("c:\\phoenix\\xbox\\work\\", file);

            System.Diagnostics.Process proc = new System.Diagnostics.Process();
            proc.StartInfo.UseShellExecute = false;
            proc.StartInfo.RedirectStandardOutput = true;
            proc.StartInfo.RedirectStandardInput = true;
            proc.StartInfo.RedirectStandardError = true;
            proc.StartInfo.CreateNoWindow = true;
            proc.StartInfo.FileName = @"p4.exe";
            proc.StartInfo.Arguments = String.Concat( "filelog ", fi.Replace("\\", "/") );
            proc.Start();

            proc.StandardInput.Write("\r\n");
            proc.StandardInput.Close();
            string output = proc.StandardOutput.ReadToEnd();
            string errortext = proc.StandardError.ReadToEnd();
            
            String[] history = output.Split('\n');
            
            foreach (String change in history)
            {
               {
                  if ( change.StartsWith("... #") )
                  {
                     if ( file == referencedFile )
                     {
                        referencedChanges.Add(new Change( change.Substring(4), history[0] ));
                     }
                     else if ( file == originatingFile)
                     {
                        referencedChanges.Add(new Change(change.Substring(4), history[0]));
                     }
                  }               
               }
            }
        }
    }
}
