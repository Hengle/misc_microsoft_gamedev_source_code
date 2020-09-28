using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace EditorCore
{
   public partial class FileBrowseControl : UserControl
   {
      private string mFileName = "";
      private string mFilterName = "Particle Effect Files";
      private string mFilterExtension = ".pfx";
      private string mReferenceFolder = "art";
      private string mStartFolder = "art\\effects";
     
      public delegate void ValueChangedDelegate(object sender, EventArgs e);
      public event ValueChangedDelegate ValueChanged;


      // Remembers the last directory where a file was open so that next time you go
      // to the filebrowse dialog, it will return to that location.
      private static string sLastBrowseDirectory = null;

      public string FileName
      {
         get { return mFileName; }
         set
         {
            mFileName = value;
            textBox1.Text = mFileName;

            // Check if file exist and change the text color accordingly
            if (!String.IsNullOrEmpty(mFileName))
            {
               string filenameAbsolute = CoreGlobals.getSaveLoadPaths().mGameDirectory + "\\" + mReferenceFolder + "\\" + mFileName + mFilterExtension;
               if (File.Exists(filenameAbsolute))
               {
                  textBox1.ForeColor = Color.Black;
               }
               else
               {
                  textBox1.ForeColor = Color.Red;
               }
            }

         }
      }

      public string FilterName
      {
         get { return mFilterName; }
         set { mFilterName = value; }
      }

      public string FilterExtension
      {
         get { return mFilterExtension; }
         set { mFilterExtension = value; }
      }

      public string ReferenceFolder
      {
         get { return mReferenceFolder; }
         set { mReferenceFolder = value; }
      }

      public string StartFolder
      {
         get { return mStartFolder; }
         set { mStartFolder = value; }
      }

      public FileBrowseControl()
      {
         InitializeComponent();
      }

      private void buttonBrowse_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();

         string filter;
         filter = mFilterName + " (*" + mFilterExtension + ")|*" + mFilterExtension;
         d.Filter = filter;
         d.FilterIndex = 0;

         d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameDirectory;

         if((mFileName != null) && (mFileName.Length > 0))
         {
            if (mReferenceFolder.Length > 0)
            {
               d.InitialDirectory += "\\" + mReferenceFolder;
            }

            d.InitialDirectory += "\\" + mFileName;

            if (!Path.HasExtension(d.InitialDirectory))
            {
               // Add extension
               d.InitialDirectory += mFilterExtension;
            }
         }
         else
         {
            // Use last browse folder if it matches the startup folder
            if ((sLastBrowseDirectory != null) && (sLastBrowseDirectory.Length > 0))
            {
               if (mStartFolder.Length > 0)
               {
                  if (sLastBrowseDirectory.IndexOf(mStartFolder) != -1)
                  {
                     d.InitialDirectory += "\\" + sLastBrowseDirectory;
                  }
                  else
                  {
                     d.InitialDirectory += "\\" + mStartFolder;
                  }
               }
               else
               {
                  d.InitialDirectory += "\\" + sLastBrowseDirectory;
               }
            }
            else
            {
               if (mStartFolder.Length > 0)
               {
                  d.InitialDirectory += "\\" + mStartFolder;
               }
            }
         }

         if (d.ShowDialog() == DialogResult.OK)
         {
            ResourcePathInfo relativePathName = new ResourcePathInfo(d.FileName);

            if(relativePathName.IsRelativePath)
            {
               string relativeFileName = relativePathName.RelativePath;

               // Remove ReferenceFolder if non-empty
               if(mStartFolder.Length > 0)
                  relativeFileName = relativeFileName.Substring(mReferenceFolder.Length + 1);

               // Remove extension
               relativeFileName = relativeFileName.Remove(relativeFileName.Length - 4);

               FileName = relativeFileName;

               // Remember last directory we were in.
               sLastBrowseDirectory = relativePathName.RelativePath;

               // call event
               EventArgs args = new EventArgs();
               if (ValueChanged != null)
                  ValueChanged(this, args);
            }
            else
            {
               if (MessageBox.Show("File is not under the correct folder.  Nothing changed.", "Warning", MessageBoxButtons.OK) != DialogResult.OK)
               {
                  return;
               }
            }
         }
      }

      private void buttonClear_Click(object sender, EventArgs e)
      {
         FileName = "";

         // call event
         EventArgs args = new EventArgs();
         if (ValueChanged != null)
            ValueChanged(this, args);
      }
   }
}
