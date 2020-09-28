using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using System.Diagnostics;
using System.IO;
using System.Xml;

namespace AssetProfiler
{
    public partial class AssetProfiler : Form
    {
        public AssetProfiler()
        {
            InitializeComponent();
        }

        private void AssetProfiler_Load(object sender, EventArgs e)
        {
            String fullPath = Application.StartupPath.Substring(0, Application.StartupPath.Length - 20);

            TreeNode rootNode = new TreeNode(fullPath);

            treeView1.Nodes.Add(rootNode);
            rootNode.Nodes.Add("");
            rootNode.Expand();            
        }

        private void treeView1_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
            String validPath = e.Node.FullPath.Replace("\\", "/");
            validPath = validPath.Replace("//", "/");
            DirectoryInfo childDI = new DirectoryInfo(validPath);
            AddDirectories(childDI, e.Node);
            AddFiles(childDI, e.Node);
        }

        private void AddDirectories(DirectoryInfo parent, TreeNode parentNode)
        {
            parentNode.Nodes.Remove(parentNode.Nodes[0]);

            DirectoryInfo[] children = parent.GetDirectories();
            foreach (DirectoryInfo child in children)
            {
                TreeNode childNode = parentNode.Nodes.Add(child.Name);
                if( (new DirectoryInfo(childNode.FullPath).GetDirectories().Length > 0) || (new DirectoryInfo(childNode.FullPath).GetFiles().Length > 0) )
                {
                    childNode.Nodes.Add("");
                }
            }            
        }
        
        private void AddFiles(DirectoryInfo parent, TreeNode parentNode)
        {
            foreach (FileInfo file in parent.GetFiles() )
            {
                switch (file.Extension.ToLower())
                {
                    case ".cin":
                    case ".dmg":
                    case ".pfx":
                    case ".scn":
                    case ".tfx":
                    case ".vis":
                        TreeNode childnode = parentNode.Nodes.Add(file.Name);
                        break;
                }
            }        
        }


        private void buttonProcess_Click(object sender, EventArgs e)
        {
            String depFileName = treeView1.SelectedNode.FullPath;
            String[] row = new String[3];
            
            XmlDataDocument xdd = new XmlDataDocument();
            xdd.DataSet.ReadXml(runDepGen(depFileName));
            dataGridView1.DataSource = xdd.DataSet;
            dataGridView1.DataMember = "node";
            
            dataGridView1.Columns[0].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;
            
        }
        
        
        private String runDepGen(String filename)
        {
            Process depgen = new Process();
            depgen.StartInfo.FileName = "../depgen/depgen.exe";
            depgen.StartInfo.Arguments = String.Concat("-filesize ", "-outputtree ", filename);
            depgen.Start();
            depgen.WaitForExit();

            String outFileName = filename.Substring(filename.LastIndexOf("\\") + 1);
            outFileName = filename.Replace( filename.Substring(filename.Length - 3, 3), "xml" );
            
            FileInfo outFile = new FileInfo(outFileName);
            
            return (outFile.Name);
        }
    }
}