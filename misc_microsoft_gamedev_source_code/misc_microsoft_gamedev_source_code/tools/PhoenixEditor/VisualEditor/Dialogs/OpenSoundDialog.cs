using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using EditorCore;



namespace VisualEditor.Dialogs
{
   public partial class OpenSoundDialog : Form
   {
      private string mFileName = "";
      private List<string> mSoundNameList = new List<string>();

      private string mFilterString = "";



      public string FileName
      {
         get { return mFileName; }
         set
         {
            mFileName = value;
            textBox1.Text = mFileName;

            // select name in treeview and listbox if non-empty
            if (!String.IsNullOrEmpty(mFileName))
            {
               // Listbox first
               //

               if ((SoundNamesListBox.SelectedItem == null) ||
                   (String.Compare(mFileName, (string)SoundNamesListBox.SelectedItem, true) != 0))
               {
                  bool found = false;
                  for (int i = 0; i < SoundNamesListBox.Items.Count; i++)
                  {
                     if (String.Compare(mFileName, (string)SoundNamesListBox.Items[i], true) == 0)
                     {
                        SoundNamesListBox.SelectedItem = SoundNamesListBox.Items[i];
                        found = true;
                        break;
                     }
                  }


                  if (!found)
                  {
                     SoundNamesListBox.SelectedIndex = -1;
                     SoundNamesListBox.SelectedItem = null;
                     SoundNamesListBox.Text = null;
                  }
               }



               // Now TreeView
               //
               if ((SoundTreeView.SelectedNode == null) ||
                   ((int)SoundTreeView.SelectedNode.Tag == -1) ||
                   (String.Compare(mSoundNameList[(int)SoundTreeView.SelectedNode.Tag], mFileName, true) != 0))
               {
                  bool found = false;
                  for (int i = 0; i < SoundTreeView.Nodes.Count; i++)
                  {
                     TreeNode foundNode = treeNodeFind(SoundTreeView.Nodes[i], mFileName);

                     if (foundNode != null)
                     {
                        SoundTreeView.SelectedNode = foundNode;
                        found = true;
                        break;
                     }
                  } 
                  
                  if (!found)
                  {
                     SoundTreeView.SelectedNode = null;
                  }
               }
            }
         }
      }


      TreeNode treeNodeFind(TreeNode treeNode, string filename)
      {
         if ((int)treeNode.Tag != -1)
         {
            if (String.Compare(mSoundNameList[(int)treeNode.Tag], filename, true) == 0)
            {
               return (treeNode);
            }
         }
         else
         {
            for (int i = 0; i < treeNode.Nodes.Count; i++)
            {
               TreeNode foundNode = treeNodeFind(treeNode.Nodes[i], filename);

               if(foundNode != null)
               {
                  return (foundNode);
               }
            }
         }

         return (null);
      }

      public OpenSoundDialog()
      {
         InitializeComponent();



         // Read sound bank
         string wwiseFilename = CoreGlobals.getWorkPaths().mGameDirectory + "\\sound\\wwise_material\\generatedSoundBanks\\Wwise_IDs.h";

         if (!File.Exists(wwiseFilename))
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Can't find wwise file: {0}", wwiseFilename));
         }

         // Create an instance of StreamReader to read from a file.
         // The using statement also closes the StreamReader.
         using (StreamReader sr = new StreamReader(wwiseFilename))
         {
            String line;

            // Skip all lines until I get to the Events section
            string eventString = "namespace EVENTS";

            while ((line = sr.ReadLine()) != null)
            {
               if(line.Contains(eventString))
               {
                  break;
               }
            }

            // Find next "{"
            while ((line = sr.ReadLine()) != null)
            {
               if (line.Contains("{"))
                  break;
            }

            // Start reading names now until closing bracket "}"
            while ((line = sr.ReadLine()) != null)
            {
               if (line.Contains("}"))
                  break; 
               
               string soundname = line;

               // Select just the event name
               string[] splitSpace = soundname.Split(' ');
               if (splitSpace.Length >= 3)
               {
                  soundname = splitSpace[splitSpace.Length - 3];
               }

               soundname = soundname.Trim();               
               soundname = soundname.ToLower();

               // Add to list
               if(!string.IsNullOrEmpty(soundname))
               {
                  mSoundNameList.Add(soundname);
               }
            }
         }

         // populate tree
         PopulateTreeView();

         // populate list
         PopulateListBox();
      }


      public void PopulateTreeView()
      {
         for (int soundId = 0; soundId < mSoundNameList.Count; soundId++)
         {
            string name = mSoundNameList[soundId];
            string[] namePaths = name.Split('_');


            TreeNode parentNode = null;

            int bonus = 0;
            /*
            // Do not separate the last _(number) into it's own folder
            int extension;
            if (int.TryParse(namePaths[namePaths.Length - 1], out extension))
            {
               bonus++;
            }
            */

            string shortName = namePaths[namePaths.Length - (1 + bonus)];
            for (int i = bonus - 1; i >= 0; i--)
            {
               shortName += "_" + namePaths[namePaths.Length - (1 + i)];
            }

            TreeNode n = new TreeNode();
            n.Text = shortName;
            n.Tag = soundId;


            for (int i = 0; i < namePaths.Length - (1 + bonus); i++)
            {
               string folderName = namePaths[i];

               if (parentNode != null)
               {
                  int index = parentNode.Nodes.IndexOfKey(folderName);
                  if (index >= 0)
                  {
                     parentNode = parentNode.Nodes[index];
                  }
                  else
                  {
                     TreeNode oldparent = parentNode;
                     parentNode = new TreeNode(folderName);
                     parentNode.Name = folderName;
                     parentNode.Tag = -1;
                     oldparent.Nodes.Add(parentNode);
                  }
               }
               else
               {
                  int index = SoundTreeView.Nodes.IndexOfKey(folderName);
                  if (index >= 0)
                  {
                     parentNode = SoundTreeView.Nodes[index];
                  }
                  else
                  {
                     parentNode = new TreeNode(folderName);
                     parentNode.Name = folderName;
                     parentNode.Tag = -1;
                     SoundTreeView.Nodes.Add(parentNode);
                  }
               }

            }


            if(parentNode != null)
               parentNode.Nodes.Add(n);
            else
               SoundTreeView.Nodes.Add(n);
         }


         if (SoundTreeView.Nodes.Count > 0)
         {
            SoundTreeView.Nodes[0].Expand();
            SoundTreeView.SelectedNode = SoundTreeView.Nodes[0];
         }

      }

      public void PopulateListBox()
      {
         // Clear list
         SoundNamesListBox.Items.Clear();

         if (string.IsNullOrEmpty(mFilterString))
         {
            SoundNamesListBox.Items.AddRange(mSoundNameList.ToArray());
         }
         else
         {
            foreach (string name in mSoundNameList)
            {
               if (name.Contains(mFilterString))
               {
                  SoundNamesListBox.Items.Add(name);
               }

            }
         }
      }


      private void OpenButton_Click(object sender, EventArgs e)
      {
         this.Close();
      }
      
      private void CancelLoadButton_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      private void SoundTreeView_AfterSelect(object sender, TreeViewEventArgs e)
      {
         int soundIdx = (int) (SoundTreeView.SelectedNode.Tag);
         if (soundIdx != -1)
         {
            FileName = mSoundNameList[soundIdx];
         }

      }

      private void FilterTextBox_TextChanged(object sender, EventArgs e)
      {
         mFilterString = FilterTextBox.Text;

         // Repopulate the list
         PopulateListBox();
      }


      private void SoundNamesListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (SoundNamesListBox.SelectedItems.Count > 0)
         {
            FileName = (string) SoundNamesListBox.SelectedItems[0];
         }
      }

      private void SoundNamesListBox_MouseDoubleClick(object sender, MouseEventArgs e)
      {
         // Play sound
         CoreGlobals.getSoundManager().playSound(FileName);
      }

      private void playButton_Click(object sender, EventArgs e)
      {
         // Play sound
         CoreGlobals.getSoundManager().playSound(FileName);
      }

      private void stopButton_Click(object sender, EventArgs e)
      {
         CoreGlobals.getSoundManager().stopAllSounds();
      }
   }
}