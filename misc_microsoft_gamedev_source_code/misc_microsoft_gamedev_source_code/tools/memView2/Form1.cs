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
using System.Diagnostics;

namespace memView2
{
   public partial class Form1 : Form
   {
      memStatsXML fsXML = new memStatsXML();

      //==================================================
      public Form1()
      {
         InitializeComponent();
         tabControl1.SelectTab(1);
      }
      //==================================================
      private void exitToolStripMenuItem_Click(object sender, EventArgs e)
      {
         this.Close();

      }
      //==================================================
      private void openXMLToolStripMenuItem_Click(object sender, EventArgs e)
      {
         OpenFileDialog ofd = new OpenFileDialog();
         ofd.Filter = "XML Memory Files (*.xml) | *.xml";
         ofd.InitialDirectory = AppDomain.CurrentDomain.BaseDirectory;
         if(ofd.ShowDialog() == DialogResult.OK)
         {
            string filename = ofd.FileName;
            parseOpenXML(filename);
            Text = filename;
            //populateTreeView(fsXML);
         }
      }
      //==================================================
      bool parseOpenXML(string filename)
      {
         

         try
         {
            XmlSerializer s = new XmlSerializer(typeof(memStatsXML), new Type[] { });
            Stream st = File.OpenRead(filename);
            fsXML = (memStatsXML)s.Deserialize(st);
            st.Close();
         }
         catch(Exception e)
         {
            MessageBox.Show(e.InnerException.ToString());
            return false;
         }


         //treeview
         //{
         //   populateTreeView(fsXML);
         //   fsXML = null;
         //}
         
         //searchview
         {

         }

         return true;
      }

      #region TREE VIEW

      //==================================================
      private static int SortBySamples(allocTreeSampleData x, allocTreeSampleData y)
      {
         if (x.sampleIndex < y.sampleIndex)
            return(-1);
         else if (x.sampleIndex > y.sampleIndex)
            return(1);
         else
            return(0);
      }

      //==================================================
      void populateTreeView(memStatsXML fsXML)
      {
         allocTreeNode tnRootNode = new allocTreeNode();
         tnRootNode.function = "ALL";
         tnRootNode.file = "";
         tnRootNode.line = "";
         tnRootNode.allocSize = 0;


         for(int allocIndex = 0; allocIndex < fsXML.allocations.Count; allocIndex++)
         {
            allocXML alloc = fsXML.allocations[allocIndex];
            int allocSize = int.Parse(alloc.size);

            int stackIndex = 0;

            allocTreeNode lastNode = tnRootNode;
            bool added = false;
            while (stackIndex < alloc.stack.Count)
            {

               bool found = false;
               for (int i = 0; i < lastNode.Nodes.Count; i++)
               {
                  allocTreeNode node = lastNode.Nodes[i] as allocTreeNode;
                  if (node == null)
                     continue;

                  if (node.function == alloc.stack[stackIndex].function)
                  {
                     lastNode = node;
                     stackIndex++;
                     found = true;
                     break;
                  }
               }

               //we didn't find ourselves at this node, add us.
               if(!found)
               {
                  added = true;
                  allocTreeNode atn = new allocTreeNode();
                  atn.function = alloc.stack[stackIndex].function;
                  atn.file = alloc.stack[stackIndex].file;
                  atn.line = alloc.stack[stackIndex].line;
                  atn.allocSize = allocSize;
                  atn.samples = new List<allocTreeSampleData>();
                  foreach (sizeSampleXML sample in alloc.sizeSamples)
                  {
                     allocTreeSampleData sampleData = new allocTreeSampleData();
                     sampleData.size = Int32.Parse(sample.size);
                     sampleData.sampleIndex = Int32.Parse(sample.sample);
                     atn.samples.Add(sampleData);
                  }
                  atn.samples.Sort(SortBySamples);
                  
                  lastNode.Nodes.Add(atn);
                  stackIndex++;
                  lastNode = (allocTreeNode)lastNode.Nodes[lastNode.Nodes.Count-1];
               }
            }


            //if we never added, then this is a duplicate path, so add our memory to the leaf node
            if(!added)
            {
               lastNode.allocSize += allocSize;
            }

         }

         updateTreeNodes(tnRootNode);
         removeFilteredNodes(tnRootNode);
         sortTreeNodes(tnRootNode);
         treeView1.Nodes.Clear();
         treeView1.Nodes.Add(tnRootNode);

      }

      //==================================================
      bool checkFilter(allocTreeNode node)
      {
         if (mFilterLarge.Checked)
         {
            if (node.allocSize < 1024 * 10) // 10kb
               return(false);
         }

         if ((mFilterIncreasing.Checked) || mFilterIncreasingNonTrivial.Checked)
         {
            int filterVal = 0;
            if (mFilterIncreasingNonTrivial.Checked)
               filterVal = 1024;
            int minVal = 0;
            bool growth = false;
            if (node.samples != null)
            {
               foreach (allocTreeSampleData asd in node.accumulatedSamples)
               {
                  if (minVal == 0)
                     minVal = asd.size;
                  else if (asd.size > (minVal + filterVal))
                  {
                     growth = true;
                     break;
                  }
               }
               if (!growth)
                  return(false);
            }
         }

         if (mFilterAlwaysIncreasing.Checked)
         {
            int filterVal = 0;
            int lastVal = 0;
            bool growth = false;
            if (node.samples != null)
            {
               foreach (allocTreeSampleData asd in node.accumulatedSamples)
               {
                  if (lastVal == 0)
                     lastVal = asd.size;
                  else if (asd.size > (lastVal + filterVal))
                  {
                     lastVal = asd.size;
                  }
                  else
                     return(false);
               }
               return (true);
            }

         }

         return(true);
      }

      //==================================================
      void removeFilteredNodes(allocTreeNode node)
      {
         if (mFilterColorOnly.Checked)
            return;

         if (node == null)
            return;

         List<allocTreeNode> removeList = new List<allocTreeNode>();
         foreach (allocTreeNode n in node.Nodes)
         {
            removeFilteredNodes(n);
            if (!n.mFiltered)
               removeList.Add(n);
         }
         foreach (allocTreeNode r in removeList)
         {
            r.Remove();
         }
      }

      //==================================================
      void updateTreeNodes(allocTreeNode node)
      {
         for (int i = 0; i < node.Nodes.Count; i++)
         {
            allocTreeNode knode = node.Nodes[i] as allocTreeNode;
            if (knode == null)
               continue;

            updateTreeNodes(knode);
         }



         node.accumulatedSamples.Clear();
         node.accumulateSamples(node.accumulatedSamples);

         node.Text = node.getFullText();

         node.mFiltered = checkFilter(node);

         if (mFilterColorOnly.Checked)
         {
            if (node.mFiltered)
               node.ForeColor = Color.Blue;
            else
               node.ForeColor = Color.LightGray;
         }
      }

      //==================================================
      void sortTreeNodes(allocTreeNode node)
      {
         //stupid bubble sort
         for (int i = 0; i < node.Nodes.Count; i++)
         {
            allocTreeNode knode = node.Nodes[i] as allocTreeNode;
            if (knode == null)
               continue;

            int targetAmt = knode.getInclusiveMemory();

            for (int j = i; j < node.Nodes.Count; j++)
            {
               allocTreeNode tnode = node.Nodes[j] as allocTreeNode;
               if (knode == null)
                  continue;

               int nextAmt = tnode.getInclusiveMemory();

               if (nextAmt > targetAmt)
               {
                  TreeNode tn = node.Nodes[i];
                  node.Nodes[i] = node.Nodes[j];
                  node.Nodes[j] = tn;

                  targetAmt = nextAmt;
               }


            }
         }

         for (int i = 0; i < node.Nodes.Count; i++)
         {
            allocTreeNode knode = node.Nodes[i] as allocTreeNode;
            if (knode == null)
               continue;

            sortTreeNodes(knode);
         }
      }

      //==================================================
      public class allocTreeSampleData
      {
         public int sampleIndex;
         public int size;
      }

      //==================================================
      public class allocTreeNode : TreeNode
      {
         public string file;
         public string line;
         public string function;
         public int allocSize;

         public bool mFiltered = false;

         public List<allocTreeSampleData> accumulatedSamples = new List<allocTreeSampleData>();
         public List<allocTreeSampleData> samples;

         public bool isMemoryChildrenInclusive()
         {
            return Nodes.Count >= 2;
         }
         //==================================================
         public int getInclusiveMemory()
         {
            if(Nodes.Count >=1)
            {
               int totalAmt =0;
               for (int i = 0; i < Nodes.Count; i++)
                  totalAmt += ((allocTreeNode)Nodes[i]).getInclusiveMemory();

               return totalAmt;
            }

            return allocSize;
         }
         //==================================================
         public string getFileLineFunctionText()
         {
            return function + "(" + file + ", line: " + line + ")";
         }
         //==================================================
         public string getFullText() 
         {
            string result = "";

            if (file == "")
            {
               result = "Total : " + MemoryNumber.convert(getInclusiveMemory());
            }
            else
            {
               string incExl = isMemoryChildrenInclusive()? " inclusive" : "";

               string sampleText = getSampleString();
               if (String.IsNullOrEmpty(sampleText))
                  result = getFileLineFunctionText() + ":" + "    [" + MemoryNumber.convert(getInclusiveMemory()) + incExl + "]";
               else
                  result = getFileLineFunctionText() + ":" + "    [" + MemoryNumber.convert(getInclusiveMemory()) + incExl + "] (" + getSampleString() + ")";

            }

            return result;
         }

         //==================================================
         public void addAcccumulatedSamples(List<allocTreeSampleData> accumulator, bool useOriginalSamples)
         {
            // [12/3/2008 Xemu] add our contribution to the accumulation 
            List<allocTreeSampleData> useSamples = null;
            if (useOriginalSamples)
               useSamples = samples;
            else
               useSamples = accumulatedSamples;

            if (useSamples == null)
               return;

            foreach (allocTreeSampleData s in useSamples)
            {
               int matchSample = s.sampleIndex;

               // [12/3/2008 Xemu] find the matching accumulator entry 
               bool found = false;

               foreach (allocTreeSampleData sampleData in accumulator)
               {
                  if (sampleData.sampleIndex == matchSample)
                  {
                     if (s.size < 0)
                        Debugger.Break();
                     sampleData.size += s.size;
                     found = true;
                     break;
                  }
               }

               // [12/3/2008 Xemu] add it into the accumulator if no match found
               if (!found)
               {
                  allocTreeSampleData asd = new allocTreeSampleData();
                  asd.size = s.size;
                  asd.sampleIndex = s.sampleIndex;
                  accumulator.Add(asd);
               }
            }
         }

         //==================================================
         public void accumulateSamples(List<allocTreeSampleData> accumulator)
         {
            if (Nodes.Count >= 1)
            {
               for (int i = 0; i < Nodes.Count; i++)
               {
                  // [12/3/2008 Xemu] this will recursively add each of our children 
                  ((allocTreeNode)Nodes[i]).addAcccumulatedSamples(accumulator, false);
               }
            }
            else
            {
               // [12/3/2008 Xemu] just put in our samples directly
               addAcccumulatedSamples(accumulator, true);
            }
         }

         //==================================================
         public string getSampleString()
         {
            string retval = "";
            bool first = true;
            foreach (allocTreeSampleData sample in accumulatedSamples)
            {
               if (!first)
                  retval = retval + ", ";
               retval = retval + MemoryNumber.convert(sample.size);
               first = false;
            }
            return retval;
         }

         //==================================================
         public string toClipboardString(bool includeParents,bool includeKids)
         {
            string outString = "";
            if(includeParents)
            {

               Stack<string> parentStrings = new Stack<string>();
               //walk parents of me.
               allocTreeNode parent = (allocTreeNode)this.Parent;
               while(parent != null)
               {
                  parentStrings.Push(parent.getFileLineFunctionText() + ":" + "    [" + MemoryNumber.convert(getInclusiveMemory()) + "]");
                  parent = (allocTreeNode)parent.Parent;
               }

               parentStrings.Pop();//we don't care about the topmost entry
            
               while(parentStrings.Count!=0)
               {
                  outString += parentStrings.Pop() + "\n";
               }
            }
            

            outString += getFullText();

            
            if (includeKids)
            {
               for (int i = 0; i < Nodes.Count; i++)
               {
                  allocTreeNode knode = Nodes[i] as allocTreeNode;
                  if (knode == null)
                     continue;

                  outString += "\n" + knode.toClipboardString(false, includeKids);
               }
            }
            

            return outString;
         }
         public string toClipboardStringOneLevel()
         {
            string outString = "";
           

            Stack<string> parentStrings = new Stack<string>();
            //walk parents of me.
            allocTreeNode parent = (allocTreeNode)this.Parent;
            while (parent != null)
            {
               parentStrings.Push(parent.getFileLineFunctionText() + ":" + "    [" + MemoryNumber.convert(getInclusiveMemory()) + "]");
               parent = (allocTreeNode)parent.Parent;
            }

            parentStrings.Pop();//we don't care about the topmost entry

            while (parentStrings.Count != 0)
            {
               outString += parentStrings.Pop() + "\n";
            }
            


            outString += getFullText();


           
               for (int i = 0; i < Nodes.Count; i++)
               {
                  allocTreeNode knode = Nodes[i] as allocTreeNode;
                  if (knode == null)
                     continue;

                  outString += "\n" + knode.getFullText();
               }
            


            return outString;
         }

      };
      #endregion

      #region SEARCH View
      void printSummaryToListBox(int numAllocs, int totalAllocSize)
      {
         listBox1.Items.Add("=============================================");
         listBox1.Items.Add("=============================================");
         listBox1.Items.Add("Total Allocation Size : " + MemoryNumber.convert(totalAllocSize));
         listBox1.Items.Add("Total Allocations : " + numAllocs);
      }

      void addAllocToListBox(allocXML alloc)
      {
         listBox1.Items.Add("=============================================");
         listBox1.Items.Add("Allocation Size : " + MemoryNumber.convert(Int32.Parse(alloc.size)) + " address : 0x" + uint.Parse(alloc.address).ToString("x"));

         string tab = "";
         for (int stackIndex = 0; stackIndex < alloc.stack.Count; stackIndex++)
         {
            listBox1.Items.Add(tab + alloc.stack[stackIndex].function + "(" + alloc.stack[stackIndex].file + ", line: " + alloc.stack[stackIndex].line + ")");
            tab += " ";
         }
      }

      bool matchStrings(string containsString, string ignoreString, allocXML alloc)
      {
         if (containsString == "" && ignoreString == "")
            return true;

         for (int stackIndex = 0; stackIndex < alloc.stack.Count; stackIndex++)
         {
            bool test = false;
            if (containsString != "")
               if (alloc.stack[stackIndex].function.Contains(containsString) ||
                  alloc.stack[stackIndex].file.Contains(containsString))
                  test = true;

            if (ignoreString != "")
               if (alloc.stack[stackIndex].function.Contains(ignoreString) ||
                  alloc.stack[stackIndex].file.Contains(ignoreString))
                  test = false;

            if (test)
               return true;
         }
         return false;
      }

      bool matchMemory(int minMem, int maxMem, allocXML alloc)
      {
         int allocSize = int.Parse(alloc.size);
         return allocSize >= minMem && allocSize <= maxMem;
      }


      void doSearch(string stringMatch, string stringIgnore, 
                     int minMem, int maxMem,
                     bool doSort, bool printSummary)
      {
         listBox1.Items.Clear();
         List<Point> passedAllocs = new List<Point>();

         
         for (int allocIndex = 0; allocIndex < fsXML.allocations.Count; allocIndex++)
         {
            allocXML alloc = fsXML.allocations[allocIndex];
            bool test = false;
            test = matchStrings(stringMatch, stringIgnore, alloc);

            test &= matchMemory(minMem, maxMem, alloc);

            if (test)
            {
               Point p = new Point(allocIndex, int.Parse(alloc.size));
               passedAllocs.Add(p);
            }
         }


         if(doSort)
            passedAllocs.Sort(ComparePassedAllocSizes);

         int totalMem = 0;
         for (int i = 0; i < passedAllocs.Count;i++ )
         {
            allocXML alloc = fsXML.allocations[passedAllocs[i].X];
            totalMem += int.Parse(alloc.size);
            addAllocToListBox(alloc);
         }

         if(printSummary)
            printSummaryToListBox(passedAllocs.Count, totalMem);
         

            
      }
      private static int ComparePassedAllocSizes(Point A, Point B)
      {
         if (A.Y < B.Y)
            return 1;

         if (A.Y > B.Y)
            return -1;

         return 0;
      }


      private void button1_Click(object sender, EventArgs e)
      {
         doSearch(textBox1.Text, textBox2.Text, 
                  (int)numericUpDown1.Value,(int)numericUpDown2.Value,
                  checkBox1.Checked, checkBox2.Checked);
      }

      #endregion
      //==================================================
      //[XmlRoot("stack")]
      public class stackXML
      {
         [XmlAttribute]
         public string file;

         [XmlAttribute]
         public string line;

         [XmlText]
         public string function;
      }
      //==================================================
      //[XmlRoot("sizeSample")]
      public class sizeSampleXML
      {
         [XmlAttribute]
         public string sample;

         [XmlText]
         public string size;
      }
      //==================================================
      //[XmlRoot("alloc")]
      public class allocXML
      {
         [XmlAttribute]
         public string size;

         [XmlAttribute]
         public string address;

         [XmlElement("stack", typeof(stackXML))]
         public List<stackXML> stack = new List<stackXML>();

         [XmlElement("sizeSample", typeof(sizeSampleXML))]
         public List<sizeSampleXML> sizeSamples  = new List<sizeSampleXML>();
      }
      //==================================================
      [XmlRoot("MemStats")]
      public class memStatsXML
      {
         [XmlElement("alloc", typeof(allocXML))]
         public List<allocXML> allocations = new List<allocXML>();
      };
      //==================================================
      private void copyThisStackToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TreeNode selNode = treeView1.SelectedNode;
         if (selNode == null)
            return;

         allocTreeNode atn = selNode as allocTreeNode;
         if (atn == null)
            return;


         Clipboard.SetDataObject(atn.toClipboardString(true, true), true);
      }
      //==================================================
      private void copyThisNodesChildrenOnlyToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TreeNode selNode = treeView1.SelectedNode;
         if (selNode == null)
            return;

         allocTreeNode atn = selNode as allocTreeNode;
         if (atn == null)
            return;


         Clipboard.SetDataObject(atn.toClipboardStringOneLevel(), true);
         
      }

      private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
      {

      }

      private void groupBox1_Enter(object sender, EventArgs e)
      {

      }

      private void copyToClipboardToolStripMenuItem_Click(object sender, EventArgs e)
      {
         string selData = "";
         for (int i = 0; i < listBox1.SelectedItems.Count; i++)
            selData += listBox1.SelectedItems[i].ToString() + "\n";

            if (selData.Length != 0)
               Clipboard.SetDataObject(selData);
      }

      private void selectAllToolStripMenuItem_Click(object sender, EventArgs e)
      {
         for (int i = 0; i < listBox1.SelectedItems.Count; i++)
            listBox1.SetSelected(i, true);
      }

      private void button2_Click(object sender, EventArgs e)
      {
         populateTreeView(fsXML);
      }

      private void safeToDiskToolStripMenuItem_Click(object sender, EventArgs e)
      {
         SaveFileDialog ofd = new SaveFileDialog();
         ofd.Filter = "txt file (*.txt)|*.txt";
         if(ofd.ShowDialog() == DialogResult.OK)
         {
            StreamWriter re = new StreamWriter(ofd.FileName);
            for (int i = 0; i < listBox1.Items.Count; i++)
               re.WriteLine(listBox1.Items[i].ToString());
            re.Close();
         }
      }

      private void mFilterLarge_Click(object sender, EventArgs e)
      {
      }


   }

   class MemoryNumber
   {
      public static string convert(int mNumBytes)
      {
         int kb = 1024;
         int mb = kb * kb;


         //megabytes
         if (mNumBytes > mb)
         {
            float mbBytes = ((int)(mNumBytes / (float)mb * 1000) / 1000.0f);
            return mbBytes.ToString() + "mb";
         }

         //kilobytes
         if (mNumBytes > kb)
         {
            float kbBytes = (((int)(mNumBytes / (float)kb) * 1000) / 1000.0f);
            return kbBytes.ToString() + "kb";
         }

         return mNumBytes.ToString();
      }
   }


}