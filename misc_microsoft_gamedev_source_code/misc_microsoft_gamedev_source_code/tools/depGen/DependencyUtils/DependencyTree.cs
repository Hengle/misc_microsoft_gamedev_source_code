using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;

using ConsoleUtils;

namespace DependencyUtils
{
   class GraphNode
   {
      public string name;
      public string label;
      public bool bFileExists;
   };

   class GraphEdge
   {
      public GraphNode parentNode;
      public GraphNode childNode;
      public bool      bFileExists;
   };

   class DependencyNode
   {
      public FileInfo               mFileInfo;
      private List<DependencyNode>  mChildNodeList = null;

      public List<string>          mUnitList = new List<string>();


      public string filename
      {
         get { return this.mFileInfo.mFilename; }
      }

      public DependencyNode(FileInfo file)
      {
         mFileInfo = file;
         mChildNodeList = null;
      }

      public int getNumChildren()
      {
         if (mChildNodeList == null)
            return 0;
         else
            return (mChildNodeList.Count);
      }
      
      public DependencyNode getChild(int i)
      {
         if (i < 0 || i > mChildNodeList.Count)
            return null;

         return (mChildNodeList[i]);
      }
    
      public void addChild(DependencyNode node)
      {
         if (mChildNodeList == null)
            mChildNodeList = new List<DependencyNode>();

         mChildNodeList.Add(node);
      }
   }





   public class DependencyTree
   {
      DependencyNode mRootNode = null;


      public DependencyTree(List<string> fileNames)
      {
         // Create root node
         mRootNode = new DependencyNode(new FileInfo("root", false));

         // Add first level dependencies
         foreach (string fileName in fileNames)
         {
            // Check file existance
            bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + fileName);

            DependencyNode node = new DependencyNode(new FileInfo(fileName, fileExists));

            mRootNode.addChild(node);
         }
      }


      public void process()
      {
         int numChildren = mRootNode.getNumChildren();
         for (int i = 0; i < numChildren; i++)
         {
            processRecurse(mRootNode.getChild(i));
         }
      }

      private void processRecurse(DependencyNode node)
      {
         if (!node.mFileInfo.bExists)
            return;

         // Create list of files that this node depends on
         List<FileInfo> dependencies = new List<FileInfo>();
         DependencyManager.getDependencyList(node.filename, dependencies, node.mUnitList);


         // Add a child node to given node for each dependency file in the list
         foreach (FileInfo dependentFileInfo in dependencies)
         {
            DependencyNode childNode = new DependencyNode(dependentFileInfo);
            node.addChild(childNode);
         }

         // Now recurse into the child nodes
         int numChildren = node.getNumChildren();
         for (int i = 0; i < numChildren; i++)
         {
            processRecurse(node.getChild(i));
         }
      }


      public void getAssetList(List<string> assetList)
      {
         int numChildren = mRootNode.getNumChildren();
         for (int i = 0; i < numChildren; i++)
         {
            getAssetListRecurse(mRootNode.getChild(i), assetList);
         }
      }

      private void getAssetListRecurse(DependencyNode node, List<string> assetList)
      {
         if (!node.mFileInfo.bExists)
            return;

         // disregard DEP files
         if(String.Compare(Path.GetExtension(node.filename), ".dep", true) != 0)
         {
            // Add file to list
            if (!assetList.Contains(node.filename))
               assetList.Add(node.filename);
         }

         // Now recurse into the child nodes
         int numChildren = node.getNumChildren();
         for (int i = 0; i < numChildren; i++)
         {
            getAssetListRecurse(node.getChild(i), assetList);
         }
      }



      public void getUnitList(List<string> unitList)
      {
         foreach (string unitName in mRootNode.mUnitList)
         {
            if (!unitList.Contains(unitName))
               unitList.Add(unitName);
         }

         int numChildren = mRootNode.getNumChildren();
         for (int i = 0; i < numChildren; i++)
         {
            getUnitListRecurse(mRootNode.getChild(i), unitList);
         }
      }

      private void getUnitListRecurse(DependencyNode node, List<string> unitList)
      {
         foreach (string unitName in node.mUnitList)
         {
            if(!unitList.Contains(unitName))
               unitList.Add(unitName);
         }

         // Now recurse into the child nodes
         int numChildren = node.getNumChildren();
         for (int i = 0; i < numChildren; i++)
         {
            getUnitListRecurse(node.getChild(i), unitList);
         }
      }



      public void writeGraphPNG(string outputFilenamePNG)
      {
         // Create .dot file for input to the GraphViz graph generator.  This file is only
         // temporary and will be deleted after the png image has been computed.
         //
         string outputFilenameDOT = Path.ChangeExtension(outputFilenamePNG, ".dot");
         writeGraphDOT(outputFilenameDOT);


         // Run GraphViz dot.exe graph image generator on the .dot file created
         //
         try
         {
            if (File.Exists(CoreGlobals.getWorkPaths().mDOTToolPath) == false)
            {
               return;
            }

            string arguments = "";
            arguments = arguments + " -Tpng";      // set format
            arguments = arguments + " -o\"" + outputFilenamePNG + "\"";
            arguments = arguments + " \"" + outputFilenameDOT + "\"";

            System.Diagnostics.Process xmbUtility;
            xmbUtility = new System.Diagnostics.Process();
            xmbUtility = System.Diagnostics.Process.Start(CoreGlobals.getWorkPaths().mDOTToolPath, arguments);
            xmbUtility.WaitForExit();
            xmbUtility.Close();
         }
         catch (System.Exception ex)
         {
            throw (ex);
         }

         // Now delete .dot file
         //
         try
         {
            File.Delete(outputFilenameDOT);
         }
         catch (System.Exception ex)
         {
            throw (ex);
         }
      }

      private void writeGraphDOT(string outputFilenameDOT)
      {
         Dictionary<string, GraphNode> graphNodeTable = new Dictionary<string, GraphNode>();
         List<GraphEdge> graphEdgeList = new List<GraphEdge>();

         int numChildren = mRootNode.getNumChildren();
         for (int i = 0; i < numChildren; i++)
         {
            DependencyNode childNode = mRootNode.getChild(i);

            // Recurse child
            createDOTRecurse(childNode, null, ref graphNodeTable, ref graphEdgeList);
         }


         // Write out file
         using (StreamWriter sw = File.CreateText(outputFilenameDOT))
         {
            sw.WriteLine("digraph g {");
            sw.WriteLine("\trankdir = LR;");
            sw.WriteLine("\tranksep = \"8.0\"");
            sw.WriteLine("\tnodesep = \"0.05\"");

            // write nodes list and their labels

            // all texture nodes in the same rank
            sw.WriteLine("\t{");
            sw.WriteLine("\t\trank = same;");

            foreach (KeyValuePair<string, GraphNode> nodePair in graphNodeTable)
            {
               switch (Path.GetExtension(nodePair.Value.label))
               {
                  case ".ddx":
                     {
                        string labelDoubleSlash = nodePair.Value.label.Replace("\\", "\\\\");

                        string shape = ",shape=box";
                        string color = nodePair.Value.bFileExists ? "" : ",color=red";
                        string style = nodePair.Value.bFileExists ? "" : ",style=filled";
                        sw.WriteLine("\t\t{0} [label=\"{1}\"{2}{3}{4}];", nodePair.Value.name, labelDoubleSlash, shape, color, style);
                     }
                     break;
               }
            }
            sw.WriteLine("\t}");


            foreach (KeyValuePair<string, GraphNode> nodePair in graphNodeTable)
            {
               string shape = "";

               switch (Path.GetExtension(nodePair.Value.label))
               {
                  case ".ddx":
                     continue;
                  case ".scn":
                  case ".sc2":
                  case ".sc3":
                     {
                        shape = ",shape=doublecircle";
                     }
                     break;
                  default:
                     {
                     }
                     break;
               }

               string labelDoubleSlash = nodePair.Value.label.Replace("\\", "\\\\");

               string color = nodePair.Value.bFileExists ? "" : ",color=red";
               string style = nodePair.Value.bFileExists ? "" : ",style=filled";

               sw.WriteLine("\t{0} [label=\"{1}\"{2}{3}{4}];", nodePair.Value.name, labelDoubleSlash, color, style, shape);

            }

            // write edges
            foreach (GraphEdge edge in graphEdgeList)
            {
               if (edge.parentNode == null)
                  continue;

               string style = edge.bFileExists ? "style=solid" : "style=dashed";
               string color = edge.bFileExists ? "" : ",color=red";

               sw.WriteLine("\t{0} -> {1} [{2}{3}];", edge.parentNode.name, edge.childNode.name, style, color);
            }

            sw.WriteLine("}");
            sw.Close();
         }
      }

      private void createDOTRecurse(DependencyNode node, GraphNode grNodeParent, ref Dictionary <string, GraphNode> graphNodeTable, ref List <GraphEdge> graphEdgeList)
      {
         GraphNode grNode;
         bool nodeFound = graphNodeTable.TryGetValue(node.filename, out grNode);


         if (nodeFound)
         {
            // Since the node is found just add an edge
            //

            // Create edge
            GraphEdge grEdge = new GraphEdge();
            grEdge.parentNode = grNodeParent;
            grEdge.childNode = grNode;
            grEdge.bFileExists = grNode.bFileExists;
            graphEdgeList.Add(grEdge);
         }
         else
         {
            // The node is not found, so we must create a new node,
            // add the edge and recurse into the node.
            //


            // Skip dep nodes since they add confusion.
            if (String.Compare(Path.GetExtension(node.mFileInfo.mFilename), ".dep", true) == 0)
            {
               // Recurse with children
               int numChildren = node.getNumChildren();
               for (int i = 0; i < numChildren; i++)
               {
                  DependencyNode childNode = node.getChild(i);
                  createDOTRecurse(childNode, grNodeParent, ref graphNodeTable, ref graphEdgeList);
               }
            }
            else
            {
               // Create new GraphNode
               grNode = new GraphNode();
               grNode.label = node.filename;
               grNode.name = "node" + graphNodeTable.Count;
               grNode.bFileExists = node.mFileInfo.bExists;

               graphNodeTable.Add(node.filename, grNode);


               // Create edge
               GraphEdge grEdge = new GraphEdge();
               grEdge.parentNode = grNodeParent;
               grEdge.childNode = grNode;
               grEdge.bFileExists = node.mFileInfo.bExists;
               graphEdgeList.Add(grEdge);


               // Recurse with children
               int numChildren = node.getNumChildren();
               for (int i = 0; i < numChildren; i++)
               {
                  DependencyNode childNode = node.getChild(i);
                  createDOTRecurse(childNode, grNode, ref graphNodeTable, ref graphEdgeList);
               }
            }
         }
      }


      //==============================================================================
      // writeFileXML
      //==============================================================================
      public void writeFileXML(string outputFilename, bool bIncludeFileSizes)
      {
         XmlDocument treeDoc = new XmlDocument();

         XmlElement rootElement = treeDoc.CreateElement("root");
         treeDoc.AppendChild(rootElement);

         int numChildren = mRootNode.getNumChildren();
         for (int i = 0; i < numChildren; i++)
         {
            XmlElement childElement = treeDoc.CreateElement("node");
            createXMLDocRecurse(mRootNode.getChild(i), treeDoc, childElement, bIncludeFileSizes);
            rootElement.AppendChild(childElement);
         }
         treeDoc.Save(outputFilename);
      }
      
      

      private void createXMLDocRecurse(DependencyNode node, XmlDocument doc, XmlElement element, bool includeFileSizes)
      {
         XmlAttribute attrib = doc.CreateAttribute("file");

         // Make filename lowercase and add correct extension
         string name = node.filename.ToLower();
         string extension = Path.GetExtension(name);

         switch (extension)
         {
            case ".vis":
            case ".pfx":
            case ".scn":
            case ".sc2":
            case ".sc3":
            case ".gls":
            case ".dmg":
            case ".cin":
               name = name + ".xmb";
               break;
         }

         attrib.Value = name;

         element.Attributes.Append(attrib);
         
         if (includeFileSizes)
         {
             XmlAttribute sizeAttr = doc.CreateAttribute("size");
             sizeAttr.Value = node.mFileInfo.lFilesize.ToString();

             element.Attributes.Append(sizeAttr);         
         }

         // Now recurse into the child nodes
         int numChildren = node.getNumChildren();
         for (int i = 0; i < numChildren; i++)
         {
            XmlElement childElement = doc.CreateElement("node");
            createXMLDocRecurse(node.getChild(i), doc, childElement, includeFileSizes);
            element.AppendChild(childElement);
         }
      }

      //==============================================================================
      // writeTxtFile
      //==============================================================================
      public void writeFileTxt(string outputFilename)
      {
         List<string> dependencies = new List<string>();
         getAssetList(dependencies);

         using (StreamWriter sw = File.CreateText(outputFilename))
         {
            foreach (string file in dependencies)
            {
               string name = file.ToLower();
               string extension = Path.GetExtension(name);

               switch (extension)
               {
                  case ".vis":
                  case ".pfx":
                  case ".scn":
                  case ".sc2":
                  case ".sc3":
                  case ".gls":
                  case ".dmg":
                  case ".cin":
                     name = name + ".xmb";
                     break;
               }


               sw.WriteLine(name);
            }
            sw.Close();
         }
      }


      //==============================================================================
      // writeUnitListTxt
      //==============================================================================
      public void writeUnitListTxt(string outputFilename)
      {
         List<string> dependencies = new List<string>();
         getUnitList(dependencies);

         dependencies.Sort();

         using (StreamWriter sw = File.CreateText(outputFilename))
         {
            foreach (string unitName in dependencies)
            {
               sw.WriteLine(unitName.ToLower());
            }
            sw.Close();
         }
      }
   }
}
