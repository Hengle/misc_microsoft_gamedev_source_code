/*
 * P4.Net *
Copyright (c) 2007 Shawn Hladky

Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without 
restriction, including without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */


using System;
using NUnit.Framework;
using P4API;
using System.IO;
using System.Threading;

namespace P4.Net.TestDriver
{

    /// <summary>Some simple Tests.</summary>
    /// 
    [TestFixture]
    public class StandardTest
    {

        private P4Connection p4 = null;
        private string p4ClientRootDirectory = null;
        private string p4ClientRootDirectory2 = null;

        public static ManualResetEvent mre = new ManualResetEvent(false);
        public static ManualResetEvent mre2 = new ManualResetEvent(false);

        private int onResolveCount;

        private string _testPort = "5791";

        [Test]
        public void Test001_SetupConnection()
        {
            p4 = new P4Connection();
            p4.Port = _testPort;
            p4.CallingProgram = "P4.Net Test Harness";
            p4.CallingVersion = "1.0";
            p4.Connect();

            p4.Client = "TestClient1";
            p4.User = "TestUser1";

            P4Record s = p4.Run("info")[0];
            Assert.AreEqual(s["userName"], "*unknown*", "User name should be *unknown*.  This test requires an empty repository");
            Assert.AreEqual(s["clientName"], "*unknown*", "Client spec should be *unknown*.  This test requires an empty repository");

        }

        [Test]
        public void Test002_CreateClient()
        {
            p4ClientRootDirectory = Path.Combine(Path.GetTempPath(), "p4temproot");
            if (Directory.Exists(p4ClientRootDirectory))
            {
                // what to do???
            }
            else
            {
                Directory.CreateDirectory(p4ClientRootDirectory);
            }
            P4Form client = p4.Fetch_Form("client");

            //test the indexer
            client["Root"] = p4ClientRootDirectory;

            //test the Fields collection
            client.Fields["Description"] = "Created from unit test";

            //Test setting an array
            string[] view = { @"//depot/... //TestClient1/..." };
            client.ArrayFields["View"] = view;

            P4UnParsedRecordSet result = p4.Save_Form(client);

            Assert.AreEqual(result.Messages.Length, 1, "While saving client spec, unexpected number of messages");
            Assert.AreEqual(result[0], "Client TestClient1 saved.", "Unable to save client spec");

            //now lets pull the form again and verify values
            client = p4.Fetch_Form("client");
            Assert.AreEqual(client["Root"], p4ClientRootDirectory, "Unexpected client root.");
            Assert.AreEqual(client.Fields["Description"], "Created from unit test\n", "Unexpected client description.");
            Assert.AreEqual(client.ArrayFields["View"].Length, 1, "Unexpected client view.");

            // lets save this with a new name
            client["Client"] = "TestClient2";

            // now, lets test the view as a multi-valued array
            // yes, I know this is a dumb view
            string[] view2 = {  @"//depot/shawn/... //TestClient2/shawn2/...", 
                                @"//depot/shawn/a/... //TestClient2/shawn/a/...",
                                @"//depot/shawn/b/... //TestClient2/shawn/b/...", 
                                @"//depot/shawn/c/... //TestClient2/shawn/c/...",
                                @"//depot/shawn/d/... //TestClient2/shawn/d/...",
                                @"//depot/shawn/e/... //TestClient2/shawn/e/...",
                                @"//depot/shawn/f/... //TestClient2/shawn/f/...",   
                                @"//depot/shawn/g/... //TestClient2/shawn/g/...",
                                @"//depot/shawn/h/... //TestClient2/shawn/h/...",
                                @"//depot/shawn/i/... //TestClient2/shawn/i/...",
                                @"//depot/shawn/j/... //TestClient2/shawn/j/...",
                                @"//depot/shawn/k/... //TestClient2/shawn/k/...",
                                @"//depot/shawn/l/... //TestClient2/shawn/l/...",
                                @"//depot/shawn/m/... //TestClient2/shawn/m/..."};

            client.ArrayFields["View"] = view2;

            result = p4.Save_Form(client);

            Assert.AreEqual(result.Messages.Length, 1, "While saving client spec, unexpected number of messages");
            Assert.AreEqual(result[0], "Client TestClient2 saved.", "Unable to save client spec");

            //now lets pull the form again and verify values
            client = p4.Fetch_Form("client", "TestClient2");
            Assert.AreEqual(client.ArrayFields["View"].Length, 14, "Unexpected client view.");

        }

        [Test]
        public void Test003_AddFiles()
        {
            string testPath = Path.Combine(p4ClientRootDirectory, "test003");
            //Create some test files
            Directory.CreateDirectory(testPath);

            P4PendingChangelist cl = p4.CreatePendingChangelist("Test003");
            string[] args = new string[12];
            args[0] = "-c";
            args[1] = cl.Number.ToString();

            for (int i = 0; i < 10; i++)
            {
                string fn = string.Format("File{0}", i);
                StreamWriter sw = File.CreateText(Path.Combine(testPath, fn));
                sw.WriteLine("this is file: {0}", fn);
                sw.Close();
                args[i + 2] = Path.Combine(testPath, fn);
            }
            P4UnParsedRecordSet r = p4.RunUnParsed("add", args);

            Assert.AreEqual(r.Messages.Length, 10, "Invalid response from p4 add");
            for (int i = 0; i < 10; i++)
            {
                Assert.AreEqual(string.Format("//depot/test003/File{0}#1 - opened for add", i), r.Messages[i], "Invalid response from p4 add");
            }

            r = cl.Submit();
            Assert.AreEqual("Submitting change 1.", r.Messages[0], "Unexpected output from p4 submit");

        }

        [Test]
        public void Test004_IntegFiles()
        {


            P4PendingChangelist cl = p4.CreatePendingChangelist("Test004");

            P4UnParsedRecordSet r = p4.RunUnParsed("integ", "-c", cl.Number.ToString(), "//depot/test003/...", "//depot/test004/...");

            Assert.AreEqual(r.Messages.Length, 10, "Invalid response from p4 add");
            for (int i = 0; i < 10; i++)
            {
                Assert.AreEqual(string.Format("//depot/test004/File{0}#1 - branch/sync from //depot/test003/File{0}#1", i), r.Messages[i], "Invalid response from p4 add");
            }

            r = cl.Submit();
            Assert.AreEqual("Submitting change 2.", r.Messages[0], "Unexpected output from p4 submit");

        }

        [Test]
        public void Test005_EditFiles()
        {
            string testPath = Path.Combine(p4ClientRootDirectory, "test003");

            P4PendingChangelist cl = p4.CreatePendingChangelist("Test005");

            // Add lines to a file
            P4UnParsedRecordSet r = p4.RunUnParsed("edit", "-c", cl.Number.ToString(), "//depot/test003/File0");
            string fn = Path.Combine(testPath, "File0");
            StreamWriter sw = File.AppendText(fn);
            sw.WriteLine("Line 2", fn);
            sw.WriteLine("Line 3", fn);
            sw.WriteLine("Line 4", fn);
            sw.Close();

            //Modify a line
            r = p4.RunUnParsed("edit", "-c", cl.Number.ToString(), "//depot/test003/File1");
            fn = Path.Combine(testPath, "File1");
            sw = File.CreateText(fn);
            sw.WriteLine("Modified File1", fn);
            sw.Close();

            //Delete a file
            r = p4.RunUnParsed("delete", "-c", cl.Number.ToString(), "//depot/test003/File2");
            
            r = cl.Submit();
            Assert.AreEqual("Submitting change 3.", r.Messages[0], "Unexpected output from p4 submit");

        }

        [Test]
        public void Test006_Password()
        {
            string testPath = Path.Combine(p4ClientRootDirectory, "test003");

            p4.OnPrompt += new OnPromptEventHandler(OnPrompt);
            try
            {
                //change the password
                P4UnParsedRecordSet r = p4.RunUnParsed("passwd");
                //now login
                p4.Login("password");

            }
            catch (Exception e)
            {
                Assert.Fail(e.Message);
            }

        }

        [Test]
        [ExpectedException(typeof(P4API.Exceptions.RunException))]
        public void Test007_WarningLevels()
        {
            p4.ExceptionLevel = P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;
            p4.Run("sync");
        }

        [Test]
        public void Test008_WarningLevels()
        {
            p4.ExceptionLevel = P4ExceptionLevels.NoExceptionOnWarnings;
            P4RecordSet r = p4.Run("sync");
            Assert.AreEqual(r.Warnings[0], "File(s) up-to-date.");

            P4UnParsedRecordSet r2 = p4.RunUnParsed("sync");
            Assert.AreEqual(r2.Warnings[0], "File(s) up-to-date.");
        }

        [Test]
        [ExpectedException(typeof(P4API.Exceptions.RunException))]
        public void Test009_WarningLevels()
        {
            p4.ExceptionLevel = P4ExceptionLevels.NoExceptionOnWarnings;
            P4RecordSet r = p4.Run("bogus");
        }

        [Test]
        public void Test010_WarningLevels()
        {
            p4.ExceptionLevel = P4ExceptionLevels.NoExceptionOnErrors;
            P4UnParsedRecordSet r = p4.RunUnParsed("bogus");
            Assert.AreEqual(r.Errors[0], "Unknown command.  Try 'p4 help' for info.");

            P4RecordSet r2 = p4.Run("bogus");
            Assert.AreEqual(r2.Errors[0], "Unknown command.  Try 'p4 help' for info.");
        }

        [Test]
        public void Test011_DateConversions()
        {
            DateTime res = p4.ConvertDate("1159361459");
            DateTime ans = new DateTime(2006, 9, 27, 6, 50, 59);
            Assert.AreEqual(res, ans);
            int tics = p4.ConvertDate(ans);
            Assert.AreEqual(tics, 1159361459);
        }

        ////[Test]
        //public void Test012_DroppedConnection()
        //{
        //    System.Threading.Thread t = new System.Threading.Thread(new System.Threading.ThreadStart(RunOnNewThread));
        //    t.Start();
        //    mre.WaitOne();

        //    Thread.Sleep(100);
        //    Console.WriteLine("ready for some football");
        //    //find the PID and kill the new connection
        //    P4RecordSet monitor = p4.Run("monitor", "show", "-ale");
        //    foreach (P4Record r in monitor)
        //    {
        //        if (r["command"] == "login")
        //        {
        //            P4UnParsedRecordSet s = p4.RunUnParsed("monitor", "terminate", r["id"]);
        //            Console.WriteLine(s.Messages[0]);
        //        }
        //    }
        //    mre2.WaitOne();
        //}

        [Test]
        public void Test013_RecordsetReporting()
        {
            string s = "";
            P4RecordSet fstat = p4.Run("filelog", "//...");
            foreach (P4Record r in fstat)
            {
                foreach (string k in r.Fields.Keys)
                {
                    s = string.Format("{0} : {1}", k, r[k]);
                    //Console.WriteLine(s);
                }
                foreach (string k in r.ArrayFields.Keys)
                {
                    for (int i = 0; i < r.ArrayFields[k].Length; i++)
                    {
                        s = string.Format("... {0}.{1} : {2}", k, i, r.ArrayFields[k][i]);
                        //Console.WriteLine(s);
                    }
                }
            }
        }

        [Test]
        public void Test014_AddInternationalFiles()
        {
            string testPath = Path.Combine(p4ClientRootDirectory, "español");
            //Create some test files
            Directory.CreateDirectory(testPath);

            P4PendingChangelist cl = p4.CreatePendingChangelist("español");
            string[] args = new string[13];
            args[0] = "-c";
            args[1] = cl.Number.ToString();
            args[2] = "-tunicode";

            for (int i = 0; i < 10; i++)
            {
                string fn = string.Format("LatinoaméricaFile{0}", i);
                StreamWriter sw = File.CreateText(Path.Combine(testPath, fn));
                sw.WriteLine("this is file: {0}\nEn español.\nΚαλημέρα κόσμε\nこんにちは 世界", fn);
                sw.Close();
                args[i + 3] = Path.Combine(testPath, fn);
            }
            P4UnParsedRecordSet r = p4.RunUnParsed("add", args);

            Assert.AreEqual(r.Messages.Length, 10, "Invalid response from p4 add");
            for (int i = 0; i < 10; i++)
            {
                Assert.AreEqual(string.Format("//depot/español/LatinoaméricaFile{0}#1 - opened for add", i), r.Messages[i], "Invalid response from p4 add");
            }

            r = cl.Submit();
            Assert.AreEqual("Submitting change 4.", r.Messages[0], "Unexpected output from p4 submit");

        }

        [Test]
        public void Test015_PrintStreams()
        {
            //PrintHelper ph = new PrintHelper(p4);
            //string s = ph.PrintTextFile("//depot/español/LatinoaméricaFile1");
            string s = p4.PrintText("//depot/test003/File1");
            Assert.AreEqual("Modified File1\n", s, "Print output failed.");

            s = p4.PrintText("//depot/español/LatinoaméricaFile1");
            Assert.AreEqual("this is file: LatinoaméricaFile1\nEn español.\nΚαλημέρα κόσμε\nこんにちは 世界\n", s, "International print output failed.");
        }


        [Test]
        [ExpectedException(typeof(ArgumentNullException))]
        public void Test016_BugMemoryExceptionOnNullArgumentToRun()
        {
            //someone's trying to hack p4....
            p4.Run("changes", null, "-m1");
        }

        [Test]
        [ExpectedException(typeof(P4API.Exceptions.FileNotFound))]
        public void Test017_PrintNonExistantFile()
        {
            
            string s = p4.PrintText("//depot/bogus.file");
            Assert.AreEqual("", s, "Print of non-existance file.");
        }

        [Test]
        public void Test018_FormatSpec()
        {
            P4Form job = p4.Fetch_Form("job");
            
            job["Description"] = "Quality is Job Number 1.\n";
            string s = job.FormatSpec();

            P4Form job2 = p4.Parse_Form("job", s);

            Assert.AreEqual(job["Description"], job2["Description"]);

            job["Description"] = "Quality is Job Number 2.\n";
            s = job.FormatSpec();
            P4Form job3 = p4.Parse_Form("job", s);
            Assert.AreEqual(job["Description"], job3["Description"]);

        }

        [Test]
        [ExpectedException(typeof(P4API.Exceptions.FormParseException))]
        public void Test019_BadSpec()
        {
            P4Form job = p4.Fetch_Form("job");
            job["Description"] = "Quality is Job Number 1.";
            job["Status"] = "bogus";
            string s = job.FormatSpec();

            // should throw an exception
            P4Form job2 = p4.Parse_Form("job", s);

        }

        // Bug fix.  Can't clobber writable files is not reporting in the errors collection
        [Test]
        public void Test020_BugCantClobberReportError()
        {
            p4.ExceptionLevel = P4ExceptionLevels.NoExceptionOnErrors;
            string testPath = Path.Combine(p4ClientRootDirectory, "test003");
            string fn = Path.Combine(testPath, "File0");
            
            //clear the read-only flag
            File.SetAttributes(fn, FileAttributes.Normal);

            // run a flush so it will try to sync
            p4.Run("flush", string.Format("{0}#1", fn));

            //now run the sync
            P4RecordSet rs = p4.Run("sync", fn);

            string[] errors = { string.Format("Can't clobber writable file {0}", fn) };
            Assert.AreEqual(errors, rs.Errors);
            
        }

        // Bug, attempting to merge a file (OnPrompt from a resolve command, and return "m"
        // resulted in an infinite loop.  For now, we throw an exception
        [Test]
        [ExpectedException(typeof(P4API.Exceptions.MergeNotImplemented))]
        public void Test021_BugMergeFile()
        {

            string testPath = Path.Combine(p4ClientRootDirectory, "test003");
            string fn = Path.Combine(testPath, "File0");

            P4PendingChangelist cl = p4.CreatePendingChangelist("resolve test");

            // This file was synced to a previous rev in another test
            P4RecordSet rs = p4.Run("edit", "-c", cl.Number.ToString(), fn);
            //foreach (string w in rs.Warnings) Console.WriteLine("W: " + w);
            //foreach (string m in rs.Messages) Console.WriteLine("M: " + m);
            //foreach (string e in rs.Errors) Console.WriteLine("E: " + e);

            StreamWriter sw = File.AppendText(fn);
            sw.WriteLine("Resolve Me 2", fn);
            sw.WriteLine("Resolve Me 3", fn);
            sw.WriteLine("Resolve Me 4", fn);
            sw.Close();

            //now run the sync
            rs = p4.Run("sync", fn);
            //foreach (string w in rs.Warnings) Console.WriteLine("W: " + w);
            //foreach (string m in rs.Messages) Console.WriteLine("M: " + m);
            //foreach (string e in rs.Errors) Console.WriteLine("E: " + e);

            //TODO -- not woking yet, so this is commented !

            if (p4.ServerLevel < 21)
            {
                Assert.Fail("\nServers earlier than 2006.1 always fail this test!");
            }

            onResolveCount = 0;
            OnPromptEventHandler eh = new OnPromptEventHandler(OnResolvePromptMerge);
            p4.OnPrompt += eh;

            // now run a resolve... what happens?
            P4UnParsedRecordSet urs = p4.RunUnParsed("resolve", fn);
            //foreach (string w in urs.Warnings) Console.WriteLine("W: " + w);
            //foreach (string m in urs.Messages) Console.WriteLine("M: " + m);
            //foreach (string e in urs.Errors) Console.WriteLine("E: " + e);

            p4.OnPrompt -= eh;

            Console.WriteLine(onResolveCount);
            
        }

        // Diff... not yet implemented
        [Test]
        [ExpectedException(typeof(P4API.Exceptions.DiffNotImplemented))]
        public void Test022_DiffNotImplemented()
        {

            string testPath = Path.Combine(p4ClientRootDirectory, "test003");
            string fn = Path.Combine(testPath, "File0");

            P4RecordSet rs = p4.Run("diff", fn);
            //foreach (string w in rs.Warnings) Console.WriteLine("W: " + w);
            //foreach (string m in rs.Messages) Console.WriteLine("M: " + m);
            //foreach (string e in rs.Errors) Console.WriteLine("E: " + e);

            //string[] e ={ "Diff not yet implemented in P4.Net." };
            //Assert.AreEqual(e, rs.Errors);
        }

        [Test]
        [ExpectedException(typeof(P4API.Exceptions.FormFetchException))]
        public void Test023_BadFormCommand()
        {
            P4Form bs = p4.Fetch_Form("info");
        }

        [Test]
        [ExpectedException(typeof(P4API.Exceptions.FormFetchException))]
        public void Test024_BadFormCommand()
        {
            P4Form bs = p4.Fetch_Form("bogus");
        }

        [Test]
        [ExpectedException(typeof(ArgumentNullException))]
        public void Test025_BadArguments()
        {
            P4Form bs = p4.Fetch_Form("change", null);
        }

        [Test]
        public void Test026_BadArguments()
        {
            P4Form bs = p4.Fetch_Form("change", "");
            
            //P4 API just ignores the empty string

            //What's it look like?
            //Console.WriteLine(bs.FormatSpec());
        }

        [Test]
        [ExpectedException(typeof(ArgumentNullException))]
        public void Test027_BadArguments()
        {
            P4RecordSet bs = p4.Run("sync", null);
        }

        [Test]
        public void Test028_BadArguments()
        {
            P4RecordSet bs = p4.Run("sync", "", "-f", "//...");
            
            //empty string causes a perforce error
            Assert.AreEqual(2, bs.Errors.Length);
        }

        [Test]
        [ExpectedException(typeof(ArgumentNullException))]
        public void Test029_BadArguments()
        {
            P4PendingChangelist bs = p4.CreatePendingChangelist(null);
        }

        [Test]
        [ExpectedException(typeof(ArgumentException))]
        public void Test030_BadArguments()
        {
            P4PendingChangelist bs = p4.CreatePendingChangelist("");
        }

        [Test]
        [ExpectedException(typeof(ArgumentNullException))]
        public void Test031_BadArguments()
        {
            // should do nothing
            p4.Login("");

            // this one raises the exception
            p4.Login(null);
        }

        [Test]
        public void Test032_RevertPropertiesToEnvironment()
        {
            string oldClient = p4.Client;
            p4.Client = "";
            //Client will revert to the environment... don't know an easy way to automate that test
            //Console.WriteLine(p4.Client);

            // set it back
            p4.Client = oldClient;
        }

        [Test]
        public void Test033_BadArguments()
        {
            // what happens when I do this?
            p4.CWD = @"o:\bogus";
            
            //doesn't matter to Perforce if the path doesn't exist, so I won't throw an exception either.
        }

        [Test]
        [ExpectedException(typeof(ArgumentNullException))]
        public void Test034_BadArguments()
        {
            p4.PrintStream(null, "//depot/whocares.txt");
        }

        [Test]
        [ExpectedException(typeof(ArgumentNullException))]
        public void Test035_BadArguments()
        {
            string localPath  = Path.GetTempFileName();
            FileStream s = new FileStream(localPath, FileMode.Create);
            p4.PrintStream(s, null);
            s.Close();
        }

        [Test]
        [ExpectedException(typeof(ArgumentException))]
        public void Test036_BadArguments()
        {
            string localPath = Path.GetTempFileName();
            FileStream s = new FileStream(localPath, FileMode.Create);
            p4.PrintStream(s, "");
            s.Close();
        }

        // Bug, diff2 returns tagged results that end in an integer (i.e. depotFile and depotFile2)
        // P4Record should show this as two keys in the Fields dictionary, but it's not.
        // The logic that was testing it to be an array, correctly identified it 
        // as not being an array, but did not put them in the fields collection either
        [Test]
        public void Test037_BugDiff2FieldsLost()
        {
            P4RecordSet diff2 = p4.Run("diff2", "//depot/test003/File1#1", "//depot/test003/File1#2");

            // should only be one record
            Assert.AreEqual(1, diff2.Records.Length);

            P4Record d2 = diff2[0];

            Assert.AreEqual("content", d2["status"]);
            Assert.AreEqual("text", d2["type"]);
            Assert.AreEqual("1", d2["rev"]);
            Assert.AreEqual("2", d2["rev2"]);
            Assert.AreEqual("//depot/test003/File1", d2["depotFile"]);
            Assert.AreEqual("//depot/test003/File1", d2["depotFile2"]);

            // make sure nothing is trapped as an arrayfield
            Assert.AreEqual(0, d2.ArrayFields.Count);

            //foreach(P4Record d2 in diff2)
            //{
            //    foreach(string s in d2.Fields.Keys)
            //    {
            //        Console.WriteLine(string.Format("{0} : {1}", s, d2[s]));
            //    }
            //}
            

        }
        
        
        [Test]
        public void Test038_ParseBranchSpec()
        {
            string branchSpec = @"
Branch: bogus_branch

Owner:  shawnh

Description:
        Created by P4.Net

Options:        unlocked

View:
        //depot/test003/a/... //depot/test038/a/...
        //depot/test003/b/... //depot/test038/b/...
        
";
            P4Form branch = p4.Parse_Form("branch", branchSpec);
            Assert.AreEqual("bogus_branch", branch["Branch"]);

            string[] view = { "//depot/test003/a/... //depot/test038/a/...", "//depot/test003/b/... //depot/test038/b/..." };
            Assert.AreEqual(view, branch.ArrayFields["View"]);

            //foreach (string s in branch.Fields.Keys)
            //{
            //    Console.WriteLine(string.Format("{0} : {1}", s, branch[s]));
            //}
            

        }

        [Test]
        public void Test039_ServerCaseSensitivity()
        {
            // since we fired this test off from a NT machine,
            // we know the server is case insinsitive.
            // the other test case needs to be run manually against a unix server

            Assert.AreEqual(false, p4.IsServerCaseSensitive());

        }

        // this test will fail if the user can not connect to the Perforce public depot
        [Test]
        public void Test039_ServerCaseSensitivityPublicDepot()
        {
            P4Connection pub = new P4Connection();
            pub.Port = "public.perforce.com:1666";
            pub.Connect();

            Assert.AreEqual(true, pub.IsServerCaseSensitive());
            pub.Disconnect();

        }
        
        [Test]
        public void Test040_PermittedFields()
        {
            P4Form job = p4.Fetch_Form("job");
            string[] result = { "Job", "Status", "User", "Date", "Description" };
            Assert.AreEqual(true, job.PermittedFields.Contains("Job"));
            Assert.AreEqual(true, job.PermittedFields.Contains("Status"));
            Assert.AreEqual(true, job.PermittedFields.Contains("User"));
            Assert.AreEqual(true, job.PermittedFields.Contains("Date"));
            Assert.AreEqual(true, job.PermittedFields.Contains("Description"));
            Assert.AreEqual(false, job.PermittedFields.Contains("job"));
            Assert.AreEqual(false, job.PermittedFields.Contains("bogus"));
        }

        [Test]
        [ExpectedException(typeof(P4API.Exceptions.FormCommandException))]
        public void Test041_FormCommandException()
        {
            // run a form command w/o -o or -i
            p4.Run("client");
        }

        [Test]
        [ExpectedException(typeof(P4API.Exceptions.FormCommandException))]
        public void Test042_FormCommandException()
        {
            // run a form command w/o -o or -i
            p4.RunUnParsed("client");
        }

        [Test]
        [ExpectedException(typeof(ArgumentException))]
        public void Test043_BadArguments()
        {
            p4.PrintBinary("");
        }

        [Test]
        [ExpectedException(typeof(ArgumentNullException))]
        public void Test044_BadArguments()
        {
            p4.PrintText(null);
        }

        [Test]
        public void Test045_OtherAction()
        {
            // need to create a file with multiple checkouts

            // first create another client, sync a file, and open for edit
            p4.Client = "TestClient3";
            p4ClientRootDirectory2 = Path.Combine(Path.GetTempPath(), "p4temproot2");
            if (Directory.Exists(p4ClientRootDirectory2))
            {
                // what to do???
            }
            else
            {
                Directory.CreateDirectory(p4ClientRootDirectory2);
            }
            P4Form client = p4.Fetch_Form("client");

            //test the indexer
            client["Root"] = p4ClientRootDirectory2;
            P4UnParsedRecordSet rs = p4.Save_Form(client);
            //Console.WriteLine(rs.ErrorMessage);

            p4.Run("sync", "//depot/test003/File1");
            p4.Run("edit", "//depot/test003/File1");

            // change back to TestClient1
            p4.Client = "TestClient1";
            p4.Run("sync", "//depot/test003/File1");
            p4.Run("edit", "//depot/test003/File1");

            P4RecordSet fstat = p4.Run("fstat", "//depot/test003/File1");

            // should be an otherOpen in the fields AND array fields
            Assert.AreEqual("1", fstat[0]["otherOpen"]);

            Assert.AreEqual("TestUser1@TestClient3", fstat[0].ArrayFields["otherOpen"][0]);
            
            //foreach (P4Record r in fstat)
            //{
            //    foreach (string k in r.Fields.Keys)
            //    {
            //        s = string.Format("{0} : {1}", k, r[k]);
            //        Console.WriteLine(s);
            //    }
            //    foreach (string k in r.ArrayFields.Keys)
            //    {
            //        for (int i = 0; i < r.ArrayFields[k].Length; i++)
            //        {
            //            s = string.Format("... {0}.{1} : {2}", k, i, r.ArrayFields[k][i]);
            //            Console.WriteLine(s);
            //        }
            //    }
            //}
            ////now 
            
        }

        //depot/test003/File2
        [Test]
        public void Test046_FileLogFilesize()
        {
            P4RecordSet rs = p4.Run("filelog", "//depot/test003/File2");

            string[] result = { "", "20" };
            Assert.AreEqual(result, rs[0].ArrayFields["fileSize"]);

        }

        // these aren't easy to test, but at least ensure they don't raise an exception
        [Test]
        public void Test047_MaxScanRows()
        {
            P4Connection p4Con = new P4Connection();
            p4Con.Port = _testPort;
            p4Con.CallingProgram = "P4.Net Test Harness";
            p4Con.CallingVersion = "1.0";
            p4Con.Client = "TestClient1";
            p4Con.User = "TestUser1";
            p4Con.MaxResults = 10;

            p4Con.Connect();
            P4RecordSet rs = p4Con.Run("filelog", "//...");
            //Console.WriteLine(rs.ErrorMessage);
            p4Con.Disconnect();

        }

        [Test]
        public void Test048_MaxResults()
        {
            P4Connection p4Con = new P4Connection();
            p4Con.Port = _testPort;
            p4Con.CallingProgram = "P4.Net Test Harness";
            p4Con.CallingVersion = "1.0";
            p4Con.Client = "TestClient1";
            p4Con.User = "TestUser1";
            p4Con.MaxScanRows = 10;

            p4Con.Connect();
            P4RecordSet rs = p4Con.Run("filelog", "//...");
            //Console.WriteLine(rs.ErrorMessage);
            p4Con.Disconnect();
        }

        [Test]
        public void Test049_MaxLockTime()
        {
            P4Connection p4Con = new P4Connection();
            p4Con.Port = _testPort;
            p4Con.CallingProgram = "P4.Net Test Harness";
            p4Con.CallingVersion = "1.0";
            p4Con.Client = "TestClient1";
            p4Con.User = "TestUser1";
            p4Con.MaxLockTime = 10;

            p4Con.Connect();
            P4RecordSet rs = p4Con.Run("filelog", "//...");
            //Console.WriteLine(rs.ErrorMessage);
            p4Con.Disconnect();

        }

        [Test]
        public void Test050_PrintBinary()
        {
            byte[] x = { 0x12, 0x55, 0xA4, 0xF9, 0x3A, 0x10, 0x00, 0xC4 };
            string filePath = Path.Combine(this.p4ClientRootDirectory, "File1.bin");
            System.IO.FileStream fs = new FileStream(filePath, FileMode.OpenOrCreate);
            fs.Write(x, 0, x.Length);
            fs.Close();
            
            P4PendingChangelist cl = p4.CreatePendingChangelist("Test050");
            P4RecordSet rs = p4.Run("add", "-c", cl.Number.ToString(), "-tbinary", filePath);

            cl.Submit();

            byte[] pX = p4.PrintBinary(filePath);
            Assert.AreEqual(x, pX);
        }

        [Test]
        public void Test051_UnparsedForms()
        {
            P4UnParsedRecordSet urs = p4.RunUnParsed("client", "-o");
            Assert.AreEqual(1,urs.Messages.Length);
        }

        [Test]
        public void Test052_BigFilePrint()
        {
            string filePath = Path.Combine(this.p4ClientRootDirectory, "File1.big");
            System.IO.StreamWriter sw = new StreamWriter(filePath);
            int i = 0;
            for (i = 0; i < 1000; i++)
            {
                sw.WriteLine("shawn was here");
            }
            sw.Close();

            P4PendingChangelist cl = p4.CreatePendingChangelist("Test052");
            P4RecordSet rs = p4.Run("add", "-c", cl.Number.ToString(), filePath);
            cl.Submit();

            P4UnParsedRecordSet urs = p4.RunUnParsed("print", "-q", filePath);
            // messages are chunked
            Assert.AreEqual(5, urs.Messages.Length);

            string s = p4.PrintText(filePath);
            StringReader sr = new StringReader(s);
            Assert.AreEqual(1001, sr.ReadToEnd().Split('\n').Length);
            sr.Close();

        }

        // Bug, diff2 looses the context diff in unparsed mode
        [Test]
        public void Test053_BugDiff2DiffsLost()
        {
            P4UnParsedRecordSet diff2 = p4.RunUnParsed("diff2", "-dbc", "//depot/test003/File1#1", "//depot/test003/File1#2");

            string[] DesiredResult = {@"==== //depot/test003/File1#1 (text) - //depot/test003/File1#2 (text) ==== content",
                                       "***************\n*** 1,1 ****\n! this is file: File1\n--- 1,1 ----\n! Modified File1\n",
                                       ""};

            Assert.AreEqual(DesiredResult, diff2.Messages);

        }

        [Test]
        public void Test054_ApiVersions()
        {
            // set to a lower API level
            p4.Disconnect();
            p4.Api = 55;
            p4.Connect();

            P4RecordSet rs1 = p4.Run("info");
            Assert.AreEqual("User name: TestUser1", rs1.Messages[0]);

            // reset to normal API levels
            p4.Disconnect();
            p4.Api = 58;
            p4.Connect();
            P4RecordSet rs2 = p4.Run("info");
            //Console.WriteLine(rs2.Records.Length);
            //Console.WriteLine(rs2[0].Fields.Count);
            Assert.AreEqual("TestUser1", rs2[0]["userName"]);

        }

        [Test]
        public void Test055_ChangePortAfterConnected()
        {
            p4.Disconnect();
            p4.Port = _testPort;
            p4.Connect();

            P4RecordSet rs2 = p4.Run("info");
            Assert.AreEqual("TestUser1", rs2[0]["userName"]);

        }
        [Test]
        [ExpectedException(typeof(P4API.Exceptions.ServerAlreadyConnected))]
        public void Test056_ChangePortWithoutDisconnect()
        {
            p4.Port = _testPort;

            P4RecordSet rs2 = p4.Run("info");
            Assert.AreEqual("TestUser1", rs2[0]["userName"]);
        }

        // Run an external merge tool 
        //[Test]
        //public void Test057_MergeFileRunMergeTool()
        //{

        //    string testPath = Path.Combine(p4ClientRootDirectory, "test003");
        //    string fn = Path.Combine(testPath, "File0");
        //    P4RecordSet rs = p4.Run("revert", fn);

        //    // run a flush so it will try to sync
        //    p4.Run("flush", string.Format("{0}#1", fn));

        //    P4PendingChangelist cl = p4.CreatePendingChangelist("resolve test");

        //    p4.RunMergeTool = true;

        //    // This file was synced to a previous rev in another test
        //    rs = p4.Run("edit", "-c", cl.Number.ToString(), fn);
            
        //    StreamWriter sw = File.AppendText(fn);
        //    sw.WriteLine("Resolve Me 2", fn);
        //    sw.WriteLine("Resolve Me 3", fn);
        //    sw.WriteLine("Resolve Me 4", fn);
        //    sw.Close();

        //    //now run the sync
        //    rs = p4.Run("sync", fn);
            
        //    onResolveCount = 0;
        //    OnPromptEventHandler eh = new OnPromptEventHandler(OnResolveRunMergeTool);
        //    p4.OnPrompt += eh;

        //    // now run a resolve... what happens?
        //    P4UnParsedRecordSet urs = p4.RunUnParsed("resolve", fn);

        //    // will execute P4MERGE command now;

        //    p4.OnPrompt -= eh;

        //    Console.WriteLine(onResolveCount);

        //}


        // Test a different ticket file 
        [Test]
        public void Test058_SetTicketFile()
        {
            string ticketFile = Path.Combine(Path.GetTempPath(), "tmpticket.txt");
            p4.TicketFile = ticketFile;
            p4.Run("logout");

            // running a login should create the new ticket file
            p4.Login("password");
            if (!File.Exists(ticketFile))
            {
                Assert.Fail("Ticket file not generated!");
            }
            else
            {
                // clear the read-only flag and delete the file
                File.SetAttributes(ticketFile, FileAttributes.Normal);
                File.Delete(ticketFile);
            }
            p4.TicketFile = "";

        }

        // Test the silent reconect "feature"
        [Test]
        public void Test059_SilentReconnect()
        {
            p4.Disconnect();

            // ConvertDate was reported to be a problem after a Disconnect in 0.9
            // this shouldn't be true in 1.0 since ConvertDate no longer makes a server call
            // and it shouldn't be a problem for any command
            DateTime res = p4.ConvertDate("1159361459");
            DateTime ans = new DateTime(2006, 9, 27, 6, 50, 59);
            Assert.AreEqual(res, ans);

            // this one should silently reconnect.
            P4RecordSet rs = p4.Run("users");
            Assert.AreEqual(2, rs.Records.Length);

        }

        // Parse a form without making a server connection.
        [Test]
        public void Test060_ParseRawForm()
        {
            // pull a bogus form to find the specdef
            P4Form b1 = p4.Fetch_Form("branch", "bogus-branch");
            string specdef = b1.SpecDef;
            string branchSpec = @"
Branch: bogus_branch

Owner:  shawnh

Description:
        Created by P4.Net

Options:        unlocked

View:
        //depot/test003/a/... //depot/test038/a/...
        //depot/test003/b/... //depot/test038/b/...
        
";
            P4Form branch = P4Form.LoadFromSpec("branch", specdef, branchSpec, System.Text.Encoding.GetEncoding(1253));
            Assert.AreEqual("bogus_branch", branch["Branch"]);

            string[] view = { "//depot/test003/a/... //depot/test038/a/...", "//depot/test003/b/... //depot/test038/b/..." };
            Assert.AreEqual(view, branch.ArrayFields["View"]);

        }

        [Test]
        public void Test100_LoadTesting_LookForMemoryLeaks()
        {
            bool run = false;

            //uncomment to actually do this... it slows the test down a bit
            //run = true;

            if (run)
            {
                // fetch the latest changelist
                 P4RecordSet rs = p4.Run("changes", "-ssubmitted", "-m1");
                int beginFolder = int.Parse(rs[0]["change"]) + 1;
                
                //branch 1000 files, 1 changelist/p4connection at a time
                for (int i = beginFolder; i < 1000 + beginFolder; i++)
                {
                    integTest(i);
                }
            }

        }

        private void integTest(int folderNumber)
        {
            P4Connection p4Con = new P4Connection();
            p4Con.Port = _testPort;
            p4Con.CallingProgram = "P4.Net Test Harness";
            p4Con.CallingVersion = "1.0";
            p4Con.Client = "TestClient1";
            p4Con.User = "TestUser1";

            p4Con.Connect();

            string sourcePath = "//depot/test003/File0";
            string destPath = string.Format("//depot/test100/{0}/File0", folderNumber);

            P4PendingChangelist cl = p4Con.CreatePendingChangelist(string.Format("integ {0}", folderNumber));

            P4RecordSet IntegResult = p4Con.Run("integ", "-c", cl.Number.ToString(), sourcePath, destPath);

            P4UnParsedRecordSet CLResult = cl.Submit();

            string[] DesiredResult = { string.Format("Submitting change {0}.", folderNumber),
                                       "Locking 1 files ..."            ,
                                       string.Format("branch //depot/test100/{0}/File0#1", folderNumber),
                                       string.Format("Change {0} submitted.", folderNumber)};
            
            Assert.AreEqual(DesiredResult, CLResult.Messages);
            
            p4Con.Disconnect();        
        }

        [Test]
        public void Test200_ConnectionProperties()
        {
            p4.Host = "host";
            //p4.CallingProgram = "P4.NetTestHarness";
            Assert.AreEqual(p4.IsValidConnection(true, true), true);
            p4.Client = "client";
            string dir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().CodeBase);
            p4.CWD = dir;

            Assert.AreEqual(p4.Host, "host");
            Assert.AreEqual(p4.Client, "client");
            Assert.AreEqual(p4.CWD, dir);
            Assert.AreEqual(p4.Port, _testPort);
            Assert.AreEqual(p4.CallingProgram, "P4.Net Test Harness");
            Assert.AreEqual(p4.CallingVersion, "1.0");

            Assert.AreEqual(p4.IsValidConnection(true, true), false);
        }

        [Test]
        [ExpectedException(typeof(P4API.Exceptions.PerforceInitializationError))]
        public void Test201_BogusConnection()
        {
            P4Connection bogus = new P4Connection();
            bogus.Port = "1234";
            bogus.Connect();
        }

        [Test]
        public void Test888_CleanUpFileSystem()
        {
            // Remove all Read-Only attitude 
            RemoveReadOnly(p4ClientRootDirectory);
            Directory.Delete(p4ClientRootDirectory, true);

            RemoveReadOnly(p4ClientRootDirectory2);
            Directory.Delete(p4ClientRootDirectory2, true);
        }

        [Test]
        public void Test999_Disconnect()
        {
            //duh
            p4.Disconnect();
        }

        private void OnPrompt(object sender, P4PromptEventArgs e)
        {
            switch (e.Message)
            {

                case ("Enter new password: "):
                    e.Response = "password";
                    break;
                case ("Re-enter new password: "):
                    e.Response = "password";
                    break;
            }
        }

        private void OnResolvePromptMerge(object sender, P4PromptEventArgs e)
        {
           
            // return m, to try and manually resolve it
            onResolveCount++;
            if (onResolveCount < 10)
            {
                e.Response = "m";
            }
            else
            {
                e.Response = "s";
            }
        }

        private int onResolveRunMergeToolCount;
        private void OnResolveRunMergeTool(object sender, P4PromptEventArgs e)
        {
            Console.WriteLine("OnResolve");
            Console.WriteLine(e.Message);

            // first promt we run the merge tool.  After that we 
            // accept the merged version
            switch (onResolveRunMergeToolCount)
            {
                case 0:
                    e.Response = "m";
                    break;
                case 1:
                    e.Response = "a";
                    break;
                default:
                    e.Response = "s";
                    break;
            }
            onResolveRunMergeToolCount++;
        }

        private static void OnPromptPause(object sender, P4PromptEventArgs e)
        {
            Console.WriteLine("On Prompt...");
            System.Threading.Thread.Sleep(11000);
            mre.Set();
            System.Threading.Thread.Sleep(500);
            e.Response = "password";
            Console.WriteLine("Exiting On Prompt...");

        }

        //private static void RunOnNewThread()
        //{
        //    P4Connection p4 = new P4Connection();
        //    p4.Port = this._testPort ;
        //    p4.User = "TestUser1";
        //    p4.Connect();
        //    p4.OnPrompt += new OnPromptEventHandler(OnPromptPause);
        //    try
        //    {
        //        Console.WriteLine("Running login");
        //        p4.RunUnParsed("login");

        //        Console.WriteLine("after login");
        //        // should have been killed in the other connection

        //        Console.WriteLine("testing connection");
        //        P4UnParsedRecordSet r = p4.RunUnParsed("info");
        //        Console.WriteLine(r.Messages[0]);
        //        /*if (!p4.IsValidConnection(true, false))
        //        {
        //            Console.WriteLine("connection failed");
        //            Assert.Fail("Did not re-establish dropped connection.");
        //        } 
        //        */

        //    }
        //    catch (Exception e)
        //    {
        //        // not to worry here...  we didn't supply a valid password
        //        //Console.WriteLine(e.Result.ErrorMessage);
        //        Console.WriteLine(e.StackTrace);
        //    }
        //    finally
        //    {
        //        Console.WriteLine("dude");
        //        p4.Disconnect();
        //    }
        //    mre2.Set();
        //}

        private void RemoveReadOnly(string path)
        {
            try
            {
                DirectoryInfo current = new DirectoryInfo(path);
                current.Attributes = FileAttributes.Normal;


                foreach (FileSystemInfo file in current.GetFileSystemInfos())
                    file.Attributes = FileAttributes.Normal;


                foreach (DirectoryInfo folder in current.GetDirectories())
                    RemoveReadOnly(folder.FullName);
            }
            catch (System.Exception excpt)
            {
                Console.WriteLine(excpt.Message);
            }
        }
    }
    class PasswordSetter
    {
        private P4Connection _p4 = null;
        private string _oldPassword = "";
        private string _newPassword = "";
        PasswordSetter(P4Connection p4)
        {
            _p4 = p4;
        }

        public void SetPassword(string OldPassword, string NewPassword)
        {
            OnPromptEventHandler eh = new OnPromptEventHandler(OnPrompt);
            _p4.OnPrompt += eh;

            _oldPassword = OldPassword;
            _newPassword = NewPassword;

            //run passwd
            P4UnParsedRecordSet r = _p4.RunUnParsed("passwd");

            //Clear the event just in case
            _p4.OnPrompt -= eh;

            //Clear the passwords from memory
            _newPassword = "";
            _oldPassword = "";
        }

        private void OnPrompt(object sender, P4PromptEventArgs e)
        {
            switch (e.Message)
            {
                case ("Enter old password: "):
                    e.Response = _oldPassword;
                    break;
                case ("Enter new password: "):
                    e.Response = _newPassword;
                    break;
                case ("Re-enter new password: "):
                    e.Response = _newPassword;
                    break;
            }
        }
    }

    public class PrintHelper
    {
        private P4Connection _p4 = null;
        private MemoryStream _ms = null;
        private StreamReader _sr = null;
        private string _s = null;
        public PrintHelper(P4Connection p4)
        {
            _p4 = p4;
        }

        public string PrintTextFile(string depotpath)
        {
            OnPrintStreamEventHandler eh = new OnPrintStreamEventHandler(OnPrint);
            OnPrintEndEventHandler eh2 = new OnPrintEndEventHandler(OnPrintEnd);
            _p4.OnPrintStream += eh;
            _p4.OnPrintEndFile += eh2;

            _p4.PrintStreamEvents(depotpath);

            _p4.OnPrintStream -= eh;
            _p4.OnPrintEndFile -= eh2;

            return _s;
        }

        private void OnPrintEnd(P4PrintStreamEventArgs e, Stream s)
        {
            _ms.Position = 0;
            _s = _sr.ReadToEnd();

            _sr.Close();

            s.Close();
            _ms = null;
            _sr = null;
        }

        private void OnPrint(P4PrintStreamEventArgs e, out Stream s)
        {
            _ms = new MemoryStream();
            _sr = new StreamReader(_ms);
            s = _ms;
        }
    }

}