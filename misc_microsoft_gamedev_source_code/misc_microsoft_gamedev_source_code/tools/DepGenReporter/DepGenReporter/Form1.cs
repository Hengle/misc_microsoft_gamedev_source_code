using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using System.IO;
using System.Collections;
using System.Net.Mail;

namespace DepGenReporter
{
    public partial class Form1 : Form
    {
        private ArrayList errorColl;
    
        public Form1()
        {
            InitializeComponent();
            
            errorColl = new ArrayList();
/*
            try
            {
               using (StreamReader sr = new StreamReader("c:\\errors.txt"))
               {
                  String line;
                  while ((line = sr.ReadLine()) != null)
                  {
                     Error error = new Error(line);
                     errorColl.Add(error);
                  }
               }
            }
            catch (FileLoadException ex)
            {
               MessageBox.Show(ex.Message, "The file could not be opened");
            }
          
            foreach ( Error error in errorColl)
            {
               if ( error.ReferencedChanges.Count > 0 )
               {
                  sendNotifyEmail(error);               
               }
            }
 */
        }

        private void buttonBrowse_Click(object sender, EventArgs e)
        {
            OpenFileDialog dlg = new OpenFileDialog();
            dlg.Title = "DepGen Error File";
            dlg.Filter = "All Files(*.*)|*.*";
            dlg.RestoreDirectory = true;
            if (dlg.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    using (StreamReader sr = new StreamReader(dlg.FileName))
                    {
                        String line;
                        log( String.Concat("Parsing ", dlg.FileName) );
                        while ((line = sr.ReadLine()) != null)
                        {
                            Error error = new Error(line);
                            log(".");
                            errorColl.Add(error);
                        }
                        log("done");
                    }
                }
                catch (FileLoadException ex)
                {
                   MessageBox.Show(ex.Message, "The file could not be opened");
                }
                foreach (Error error in errorColl)
                {
                   if (error.ReferencedChanges.Count > 0)
                   {
                      sendNotifyEmail(error);
                   }
                }
            }
        }
        
       private void log(String logLine)
       {
            String lines = richTextBox1.Text;
            lines = String.Concat(logLine, lines);
            richTextBox1.Text = lines;
            richTextBox1.Update();
       }

       private void sendNotifyEmail( Error err )
       {
          try
          {
             MailMessage oMsg = new MailMessage();
             oMsg.From = new MailAddress("cvandoren@ensemblestudios.com", "cvandoren");
             //oMsg.To.Add("cvandoren@ensemblestudios.com");
             
             Change tmp = (Change)err.ReferencedChanges[0];
             
             log(String.Concat(" | ", err.OriginatingFile, "\n"));
             oMsg.To.Add("jirabugs@ensemblestudios.com");
             log("Emailing ");
            
             oMsg.Subject = err.OriginatingFile + " refers to " + err.ReferencedFile + " which does not exist.";
             oMsg.IsBodyHtml = true;

             oMsg.Body = "The build server has found an error.<br>This is an auto-generated email.<br>";

             oMsg.Body += "<b>Perforce change number:</b> " + tmp.ChangeListNumber + "<br>";
             oMsg.Body += "<b>Last checked in by:</b> " + tmp.User + "<br><br><br>";
             oMsg.Body += "<b>" + err.OriginatingFile + "</b><br>refers to<br><b>" + err.ReferencedFile + "</b> which does not exist<br><br><br><br>";
             oMsg.Body += "Please check all relevent assets (Max files, etc.) to either remove uneeded references or add the missing file to Perforce!";

             oMsg.Priority = MailPriority.Normal;



             // TODO: Replace with the name of your remote SMTP server.
             SmtpClient client = new SmtpClient("ensexch.ENS.Ensemble-Studios.com");
             client.UseDefaultCredentials = true;
             
             
             client.Send(oMsg);
            
             oMsg = null;
          }
          catch (Exception e)
          {
             MessageBox.Show(String.Concat(e, " Exception caught."));
          }
       }        
    }
}