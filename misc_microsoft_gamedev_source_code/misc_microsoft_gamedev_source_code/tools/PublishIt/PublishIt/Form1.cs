using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using System.IO;
using System.Reflection;
using P4API;
using System.Net.Mail;

namespace PublishIt
{
    public partial class Form1 : Form
    {
        private P4Interface p4I;
        private Settings appSettings;
        
        private String artPath;
        private ArrayList checkedNodes;
        private String p4User;

        private P4Connection p4;
        private String localPath;
        private String localWorkPath;
        
        private bool scan = false;

        public Form1()
        {

            InitializeComponent();
            
            
            appSettings = new Settings("data.dat");

            p4 = new P4Connection();
            p4.ExceptionLevel = P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;
            p4.Connect();

            if (!p4.IsValidConnection(true, true))
            {
                MessageBox.Show("No valid p4 connection.");
            }
            
            p4I = new P4Interface();
            p4I.Connect();

            if (!p4I.IsValidConnection())
            {
                MessageBox.Show("No valid p4I connection.");
            }
            
            checkedNodes = new ArrayList();
        
            foreach (String user in p4I.GetUsers())
            {
                comboBoxUsers.Items.Add(user.ToLower());
            }
            comboBoxUsers.SelectedItem = p4.User.ToLower().ToString();
            
            BuildLocalPaths();
            MarkUnpublishedFolders(getUnpublishedFiles());
            ExpandNode(treeView1.Nodes[0]);
    
            p4.Disconnect();
        }
        
        private void BuildLocalPaths()
        {
            String appPath = Application.StartupPath;
            String[] tmp = appPath.Split('\\');
            String workPath = "";
            
            for (int i = 0; i < tmp.Length - 2; i++)
            {
                workPath = String.Concat(workPath, tmp[i], "\\");
            }
            localPath = workPath.TrimEnd('\\');

            String myRoot = localPath;
            String[] fullPath = myRoot.Split('/');

            TreeNode rootNode = new TreeNode(myRoot);
            treeView1.Nodes.Add(rootNode);
            rootNode.Expand();
            TreeNode artNode = rootNode.Nodes.Add("art");
            TreeNode scenarioNode = rootNode.Nodes.Add("scenario");

            DirectoryInfo artDir = new DirectoryInfo(String.Concat(myRoot, "\\art"));
            DirectoryInfo scenarioDir = new DirectoryInfo(String.Concat(myRoot, "\\scenario"));

            LogLine( String.Concat("myRoot: ", myRoot, "\nappPath:", appPath, "\n") );
            
                     
        }

        private void AddDirectories(DirectoryInfo parent, TreeNode parentNode)
        {
            //if (scan == false)
            {
                DirectoryInfo[] children = parent.GetDirectories();
                
                foreach (DirectoryInfo child in children)
                {
                    TreeNode childNode = parentNode.Nodes.Add(child.Name);
                    if ( child.GetDirectories().Length > 0 )
                    {
                        childNode.Nodes.Add("");
                        //AddDirectories(new DirectoryInfo(childNode.FullPath), childNode);
                        //childNode.Expand();
                    }
                }
            }
            treeView1.Refresh();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            foreach (TreeNode tn in checkedNodes)
            {
                try
                {
                    //p4I.Integrate(tn.FullPath);
                }
                catch (System.Exception ex)
                {
                    MessageBox.Show(ex.Message, "Error!");
                }
            }
            MessageBox.Show("Publish complete.", "Done!");
        }

        private void treeView1_AfterCheck(object sender, TreeViewEventArgs e)
        {
            if (e.Node.Checked)
            {
                if (checkedNodes.Contains(e.Node) == false)
                {
                    checkedNodes.Add(e.Node);
                }
            }
            else
            {
                if (checkedNodes.Contains(e.Node))
                {
                    checkedNodes.Remove(e.Node);
                }
            }
        }
        
        private void LogLine(String line)
        {
            String buffer = richTextBox1.Text;
            richTextBox1.Clear();
            richTextBox1.Text = String.Concat( line, "\n", buffer);
            richTextBox1.Refresh();        
        }
        
        private ArrayList getUnpublishedFiles()
        {
            this.Cursor = Cursors.WaitCursor;
            Scanning scan = new Scanning();
            scan.StartPosition = FormStartPosition.CenterParent;
            scan.Show();
            scan.Refresh();

            p4 = new P4Connection();
            p4.Connect();
            p4I = new P4Interface();         
            
            ArrayList unpublished = new ArrayList();
            String[] args = {"-n", "-d", "-i", "-t", "-v", "-b", "phx_published", "//depot/phoenix/xbox/published/..."};
            
            // logging
            String tmp = "";
            foreach (String t in args)
            {
                tmp += t;
                tmp += " ";
            }
            LogLine( String.Concat("Running: Integrate ", tmp) );
            
            P4RecordSet record = p4I.Run("integrate", args);
            LogLine(String.Concat("Total Records: ", record.Records.Length ));
            foreach (P4Record entry in record)
            {
                
                if (checkBoxFilter.Checked)
                {
                    String file = entry.Fields["depotFile"];
                    String delim = "/";
                    int trimIndex = -1;
                    if ( file.Split(delim.ToCharArray())[6] == "art")
                    {
                        trimIndex = file.IndexOf("art");
                    }
                    else if ( file.Split(delim.ToCharArray())[6] == "scenario" )
                    {
                        trimIndex = file.IndexOf("scenario");
                    }
                    
                    file = file.Substring(trimIndex, file.Length - trimIndex);
                    file = "//depot/phoenix/xbox/published/" + file.Replace("\\","/");
                
                    String[] args2 = { file.Replace("//depot/phoenix/xbox/published", localPath).Replace("\\","/") };
                    P4RecordSet changes = p4I.Run("changes", args2);
                    
                    if (changes.Records.Length > 0 && changes[0].Fields["user"] == comboBoxUsers.SelectedItem.ToString())
                    {
                        LogLine(String.Concat("Record: ", entry.Fields["depotFile"]));
                        unpublished.Add(entry.Fields["fromFile"]);
                    }
                }
                else
                    unpublished.Add(entry.Fields["fromFile"]);
            }
            scan.Close();
            this.Cursor = Cursors.Default;
            return unpublished;
            
            p4.Disconnect();
        }

        private ArrayList getUnpublishedFiles(String path)
        {
            this.Cursor = Cursors.WaitCursor;

            if (scan == false)
            {
                int trimIndex = path.IndexOf("art");
                path = path.Substring(trimIndex, path.Length - trimIndex);
                path = "//depot/phoenix/xbox/published/" + path.Replace("\\","/");
            }

            p4 = new P4Connection();
            p4.Connect();
            p4I = new P4Interface(); 

            ArrayList unpublished = new ArrayList();
            String[] args = { "-n", "-d", "-i", "-t", "-v", "-b", "phx_published", path };
            P4RecordSet record = p4I.Run("integrate", args);
            foreach (P4Record entry in record)
            {
                unpublished.Add(entry.Fields["fromFile"]);
            }
            this.Cursor = Cursors.Default;
            return unpublished;
            
            p4.Disconnect();
        }
        
        private void MarkUnpublishedFolders(ArrayList files)
        {
            ArrayList unPublishedFolders = new ArrayList();
            
            foreach (String file in files)
            {
                String[] dir = file.Split('/');
                //find the index of "work"
                int index;
                for (index = 0; index < dir.Length; index++)
                {
                    if (dir[index] == "work")
                    {
                        break;
                    }
                }
                //rebuild the local path after work
                String path = "";
                
                // Just add and return all the files.
                
                unPublishedFolders.Add(file);
            }
            BuildUnPubTree(unPublishedFolders);
        }
        
        TreeNode LocateNode(String path, TreeNodeCollection treeCol)
        {
            String[] PTms = path.Split(new Char[] {'\\'});
            
            for (int k = 0; k < treeCol.Count; k++)
            {
                if (treeCol[k].Text == PTms[0])
                {
                    treeCol[k].Expand();
                    if (treeCol[k].Nodes.Count == 1 || PTms.Length == 2)
                    {
                        treeCol[k].Expand();
                        return treeCol[k];
                    }
                    return LocateNode(path.Remove(0, PTms[0].Length + 1), treeCol[k].Nodes);
                }
            }
            return null;
        }

        private void BuildUnPubTree(ArrayList files)
        {
            if (treeView1.Nodes.Count > 0)
            {
                treeView1.Nodes.RemoveAt(0);
                treeView1.Refresh();
            }

            TreeNode root = new TreeNode("Depot");
            treeView1.Nodes.Add(root);

            foreach (String file in files)
            {
                String tmp = file.Replace("//depot", "");
                String[] path = (tmp.TrimStart('/').Split('/'));

                TreeNode parenNode = root;

                int bonus = 0;
                int extension;
                if (int.TryParse(path[path.Length - 1], out extension))
                {
                    bonus++;
                }

                string shortName = path[path.Length - (1 + bonus)];
                for (int i = bonus - 1; i >= 0; i--)
                {
                    shortName += "\\" + path[path.Length - (1 + i)];
                }

                TreeNode n = new TreeNode();
                n.Text = shortName;

                for (int i = 0; i < path.Length - (1 + bonus); i++)
                {
                    String folderName = path[i];

                    int index = parenNode.Nodes.IndexOfKey(folderName);
                    if (index >= 0)
                    {
                        parenNode = parenNode.Nodes[index];
                    }
                    else
                    {
                        TreeNode oldParent = parenNode;
                        parenNode = new TreeNode(folderName);
                        parenNode.Name = folderName;
                        oldParent.Nodes.Add(parenNode);
                    }
                }

                parenNode.Nodes.Add(n);
            }

            treeView1.Sort();
            //treeView1.ExpandAll();
        }
        
        private void ExpandNode(TreeNode parent)
        {
            for (int i = 0; i < parent.Nodes.Count; i++)
            {
                if (parent.Nodes[i].Text.Contains(".") == false)
                {
                    parent.Nodes[i].Expand();
                    ExpandNode(parent.Nodes[i]);
                }
                else
                    parent.Collapse();
            }
            treeView1.Nodes[0].Expand();
        }
               
        private void Form1_Load(object sender, EventArgs e)
        {
        }

        private void treeView1_AfterCheck_1(object sender, TreeViewEventArgs e)
        {
            if (e.Node.Checked)
            {
                if (e.Node.Nodes.Count == 0)
                {
                    if (checkedNodes.Contains(e.Node) == false)
                    {
                        checkedNodes.Add(e.Node);
                    }
                }
                foreach (TreeNode node in e.Node.Nodes)
                {
                    node.Checked = true;
                    if (node.Nodes.Count == 0)
                    {
                        if (checkedNodes.Contains(node) == false)
                        {
                            checkedNodes.Add(node);
                        }
                    }
                }            
            }
            else
            {
                checkedNodes.Remove(e.Node);
                foreach (TreeNode node in e.Node.Nodes)
                {
                    node.Checked = false;
                    if (checkedNodes.Contains(node))
                    {
                        checkedNodes.Remove(node);
                    }
                }
            }
        }

        private void buttonPublishChecked_Click(object sender, EventArgs e)
        {
            if (checkedNodes.Count > 0)
            {
                ArrayList uncheck = new ArrayList();
                ArrayList files = new ArrayList();

                if (appSettings.getSetting("locked") == "true")
                {
                    MessageBox.Show("The archive build is currently locked.\nPlease get with a lead to publish files.","Build Locked!", MessageBoxButtons.OK, MessageBoxIcon.Stop);
                    return;
                }
                foreach (TreeNode node in checkedNodes)
                {
                    bool allChildrenChecked = true;
                    foreach (TreeNode child in node.Nodes)
                    {
                        if (child.Checked == false)
                        {
                            allChildrenChecked = false;
                        }
                    }
                    if (allChildrenChecked)
                    {
                       String tmp = node.FullPath;//.Replace("Depot", "\\\\depot\\phoenix\\xbox\\work");
                        String[] split = tmp.Split('\\');
                        int index;
                        for (index = 0; index < split.Length; index++)
                        {
                            if (split[index] == "work")
                            {
                                break;
                            }
                        }
                        String depotPath = "\\depot\\phoenix\\xbox";
                        for (int i = index; i < split.Length; i++)
                        {
                            depotPath = String.Concat(depotPath, "\\", split[i]);
                        }
                        //depotPath += "\\...";
                        
                        
                        if (files.Contains(depotPath) == false)
                        {
                            String buffer = richTextBox1.Text;
                            richTextBox1.Clear();
                            richTextBox1.Text = String.Concat("Publishing ", depotPath, "\n", buffer);
                            richTextBox1.Refresh();
                            files.Add(depotPath);
                        }
                    }
                }
                
                Form3 verify = new Form3(files);
                DialogResult rslt = verify.ShowDialog();
                if ( rslt == DialogResult.OK)
                {
                    int change = p4I.Integrate(files);
                    sendNotifyEmail(files, change);                                    
                }
                    
                checkedNodes.Clear();
                treeView1.Nodes.Clear();
                MarkUnpublishedFolders(getUnpublishedFiles());
                ExpandNode(treeView1.Nodes[0]);
                treeView1.SelectedNode = treeView1.Nodes[0];
            }            
        }

        private void Form1_Load_1(object sender, EventArgs e)
        {

        }

        private void buttonScan_Click(object sender, EventArgs e)
        {
            checkedNodes.Clear();
                treeView1.Nodes.Clear();
                MarkUnpublishedFolders(getUnpublishedFiles());
                treeView1.SelectedNode = treeView1.Nodes[0];
        }

        private void treeView1_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
        }

        private void treeView1_AfterExpand(object sender, TreeViewEventArgs e)
        {
            if (e.Node.Checked)
            {
                foreach (TreeNode node in e.Node.Nodes)
                {
                    node.Checked = true;
                }
            }
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            try
            {
                p4.Disconnect();
            }
            catch (System.Exception ex)
            {
            	MessageBox.Show(ex.Message.ToString());
            }
            
        }
        
        private void CollapseThisNodeLevel(TreeNode node)
        {
            foreach (TreeNode sibling in node.Parent.Nodes)
            {
                sibling.Toggle();
            }
        }

        private void button1_Click_1(object sender, EventArgs e)
        {
            TreeNode sel = treeView1.SelectedNode;
            CollapseThisNodeLevel(treeView1.SelectedNode);
        }

        static void sendNotifyEmail(ArrayList files, int changelistNumber)
        {
            try
            {
                MailMessage oMsg = new MailMessage();
                oMsg.From = new MailAddress("PublishIt@ensemblestudios.com", System.Environment.UserName);
                oMsg.To.Add("cvandoren@ensemblestudios.com");
                oMsg.To.Add("swinsett@ensemblestudios.com");
                oMsg.To.Add("crippy@ensemblestudios.com");
                oMsg.To.Add("dpottinger@ensemblestudios.com");
                oMsg.To.Add("alaudon@ensemblestudios.com");
                oMsg.To.Add("bjackson@ensemblestudios.com");

                oMsg.Subject = String.Concat("ChangeList ", changelistNumber.ToString(), " has been published.");
                oMsg.IsBodyHtml = true;

                oMsg.Body = "A Publish event has occured.<br>";

                oMsg.Body += "<b>Machine:</b> " + System.Environment.MachineName + "<br>";
                oMsg.Body += "<b>User:</b> " + System.Environment.UserName + "<br>";
                oMsg.Body += "<b>Changelist:</b> " + changelistNumber.ToString() + "<br>";
                oMsg.Body += "<br><br><b>Files:</b><br>";

                foreach (String file in files)
                {
                    oMsg.Body += file + "<br>";
                }

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