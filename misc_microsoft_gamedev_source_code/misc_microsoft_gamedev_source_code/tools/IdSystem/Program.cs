using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Xml.XPath;
using System.IO;

namespace IdSystem
{
    class Program
    {
        static void Main(string[] args)
        {
            Dictionary<int, XmlNode> table = new Dictionary<int, XmlNode>();

            SourceControl scm = new SourceControl(false);
            string controlFile = null;
            try
            {
                if (args.Length < 3)
                {
                    Console.WriteLine("usage: IdSystem <input file> <Root Path> <Node for DBID Path> [output file]");
                    Console.WriteLine("example: IdSystem squads.xml /Squads /Squads/Squad");
                    Console.WriteLine("");
                    Console.WriteLine("Note: The Node names are case sensitive!");
                    Console.WriteLine("Note: If the output file is omitted, the data will be written");
                    Console.WriteLine("      back to the input file.");
                    
                    return;
                }

                string inputFile = args[0];
                string rootPath = args[1];
                string dbidPath = args[2];
                string outputFile = inputFile;
                if (args.Length > 3)
                    outputFile = args[3];

                if (!File.Exists(inputFile))
                {
                    Console.WriteLine("The input file cannot be found.");
                    Console.WriteLine("Input File: " +inputFile);
                    throw new Exception("Error");
                }

                FileAttributes fa = File.GetAttributes(inputFile);
                if ( (fa & FileAttributes.ReadOnly) == FileAttributes.ReadOnly )
                {
                    Console.WriteLine("The input file is Read Only.");
                    Console.WriteLine("Input File: " +inputFile);
                    throw new Exception("No changes were made.");
                }


                // Sync to the head
                if (!scm.Sync("dbid.control"))
                {
                    Console.WriteLine("Could not sync the control file.");
                    Console.WriteLine("Control file: dbid.control");
                    throw new Exception("Error");
                }

                // Check out the control file
                if (!scm.Checkout("dbid.control"))
                {
                    Console.WriteLine("Could not check out the control file.");
                    Console.WriteLine("Control file: dbid.control");
                    throw new Exception("Error");
                }

                // now the exception handler knows to revert the file if we get an error.
                controlFile = "dbid.control";

                if (!File.Exists("dbid.control"))
                {
                    Console.WriteLine("The DBID control file cannot be found.");
                    Console.WriteLine("Control file: dbid.control");
                    throw new Exception("Error");
                }

                // lock the control file
                if (!scm.Lock(controlFile))
                {
                    Console.WriteLine("Could not lock the control file.");
                    Console.WriteLine("Control file: " + controlFile);
                    throw new Exception("Error");
                }

                // read the control file.
                XmlDocument controlDoc = new XmlDocument();
                try
                {
                    controlDoc.Load(controlFile);
                }
                catch(Exception)
                {
                    Console.WriteLine("The control file could not be loaded.");
                    Console.WriteLine("Control file: " +inputFile);
                    throw new Exception("Error");
                }

                XmlDocument doc = new XmlDocument();
                doc.PreserveWhitespace = true;
                try
                {
                    doc.Load(inputFile);
                }
                catch(Exception)
                {
                    Console.WriteLine("The input file could not be loaded.");
                    Console.WriteLine("Input File: " +inputFile);
                    throw new Exception("Error");
                }

                // query for the root node of the CONTROL file that has the next DBID on it.
                XmlNode root = controlDoc.SelectSingleNode(rootPath);
                if (root == null)
                {
                    Console.WriteLine("The root node could not be found.");
                    Console.WriteLine("Root Node: " +rootPath);
                    throw new Exception("Error");
                }
                bool isDirty = false;
                XmlAttribute rootAttr = root.Attributes["nextDBID"];
                if (rootAttr == null)
                {
                    Console.WriteLine("File does not contain the DBID system. Adding...");
                    rootAttr = doc.CreateAttribute("nextDBID");
                    rootAttr.Value = "1";
                    root.Attributes.Append(rootAttr);
                    isDirty = true;
                }

                int nextDBID = 0;
                try
                {
                    nextDBID = int.Parse(rootAttr.Value);
                }
                catch(Exception e)
                {
                    System.Console.WriteLine(e.Message);
                    System.Console.WriteLine(e.StackTrace);
                    throw new Exception("Error");
                }

                int numAdded = 0;

                // Iterate over each node in the list that needs the DBID attribute
                XmlNodeList list = doc.SelectNodes(dbidPath);
                foreach (XmlNode node in list)
                {
                    // see if there is DB ID on the node
                    XmlAttribute attr = node.Attributes["dbid"];
                    if (attr == null)
                    {
                        attr = doc.CreateAttribute("dbid");
                        attr.Value = nextDBID.ToString();
                        node.Attributes.Append(attr);
                        nextDBID++;
                        numAdded++;
                        isDirty = true;
                    }

                    int dbid = 0;
                    try
                    {
                        dbid = int.Parse(attr.Value);
                    }
                    catch(Exception)
                    {
                        attr.Value = nextDBID.ToString();
                        dbid = nextDBID;
                        node.Attributes.Append(attr);
                        nextDBID++;
                        numAdded++;
                        isDirty = true;
                    }


                    // preserve this ID so we can check for duplicates
                    if (table.ContainsKey(dbid) )
                    {
                        Console.WriteLine(Environment.NewLine + "Duplicate DBID found: " + dbid + Environment.NewLine + Environment.NewLine + node.OuterXml);
                        Console.WriteLine(Environment.NewLine +"Please remove and re-run the tool.");
                        throw new Exception("Error");
                    }

                    // add this
                    table.Add(dbid, node);
                }

                if (isDirty)
                {
                    // we have modified the DBID 
                    // update the root next DBID
                    rootAttr.Value = nextDBID.ToString();

                    // save our control file
                    controlDoc.Save(controlFile);

                    // submit our control file.
                    scm.Checkin(controlFile, "Next DBID is " + nextDBID);

                    // save our document
                    doc.Save(outputFile);

                    Console.WriteLine("Number of new DBIDs added: " + numAdded);
                    Console.WriteLine("The next DBID is         : " + nextDBID);
                    Console.WriteLine("DBID changes are saved to: " + outputFile);

                    Console.WriteLine("Please make sure you check in the file");
                }
                else
                {
                    Console.WriteLine("No DBIDs were added.");
                    scm.Revert(controlFile);
                }
            }
            catch (Exception e)
            {
                System.Console.WriteLine(e.Message);
                // System.Console.WriteLine(e.StackTrace);
                if (controlFile != null)
                    scm.Revert(controlFile);
            }
        }
    }
}
