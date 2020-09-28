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

namespace P4.Net.TestDriver
{
    [TestFixture]
    public class UnicodeTest
    {
        private P4Connection p4 = null;
        private string p4ClientRootDirectory = null;
        private const string _port = "5791";

        [Test]
        public void Test001_SetupConnection()
        {
            p4 = new P4Connection();
            p4.Port = _port;
            p4.CallingProgram = "P4.Net Test Harness";
            p4.CallingVersion = "1.0";
            p4.Connect();

            p4.Client = "TestClient1";
            p4.User = "TestUser1";

            P4Record s = p4.Run("info")[0];
            Assert.AreEqual(s["userName"], "TestUser1", "User name should be known.  This test requires StandardTest to be executed first");
            Assert.AreEqual(s["clientName"], "TestClient1", "Client spec should be known.  This test requires StandardTest to be executed first");

        }

        [Test]
        public void Test002_CreateClient()
        {
            p4.Client = "UnicodeTest";

            p4ClientRootDirectory = Path.Combine(Path.GetTempPath(), "p4temprootU");
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

            P4UnParsedRecordSet result = p4.Save_Form(client);

            Assert.AreEqual(result.Messages.Length, 1, "While saving client spec, unexpected number of messages");
            Assert.AreEqual(result[0], "Client UnicodeTest saved.", "Unable to save client spec");

        }

        [Test]
        public void Test003_AddFiles()
        {
            p4.Disconnect();
            p4.Charset = "utf16le-bom";
            p4.Connect();

            string testPath = Path.Combine(p4ClientRootDirectory, "TestUnicode");
            //Create some test files
            Directory.CreateDirectory(testPath);

            P4PendingChangelist cl = p4.CreatePendingChangelist("TestUnicode");
            string[] args = new string[13];
            args[0] = "-c";
            args[1] = cl.Number.ToString();
            args[2] = "-tunicode";

            for (int i = 0; i < 10; i++)
            {
                string fn = string.Format("File{0}", i);
                string contents = string.Format("this is file: {0}\nEn español.\nΚαλημέρα κόσμε\nこんにちは 世界", i);
                StreamWriter sw = new StreamWriter(Path.Combine(testPath, fn), false, System.Text.Encoding.Unicode);
                sw.Write(contents);
                sw.Close();

                args[i + 3] = Path.Combine(testPath, fn);
            }
            P4UnParsedRecordSet r = p4.RunUnParsed("add", args);

            Assert.AreEqual(r.Messages.Length, 10, "Invalid response from p4 add");
            for (int i = 0; i < 10; i++)
            {
                Assert.AreEqual(string.Format("//depot/TestUnicode/File{0}#1 - opened for add", i), r.Messages[i], "Invalid response from p4 add");
            }

            r = cl.Submit();
            Assert.AreEqual("Submitting change 8.", r.Messages[0], "Unexpected output from p4 submit");

        }

        [Test]
        public void Test004_PrintTest()
        {
            //PrintHelper ph = new PrintHelper(p4);
            //string s = ph.PrintTextFile("//depot/español/LatinoaméricaFile1");
            string s = p4.PrintText("//depot/TestUnicode/File1");
            Assert.AreEqual("this is file: 1\nEn español.\nΚαλημέρα κόσμε\nこんにちは 世界", s, "Print output failed.");

            s = p4.PrintText("//depot/español/LatinoaméricaFile1");
            Assert.AreEqual("this is file: LatinoaméricaFile1\nEn español.\nΚαλημέρα κόσμε\nこんにちは 世界\n", s, "International print output failed.");
        }

        [Test]
        public void Test005_PrintTest()
        {
            p4.Disconnect();
            p4.Charset = "eucjp";
            p4.Connect();
            //PrintHelper ph = new PrintHelper(p4);
            //string s = ph.PrintTextFile("//depot/español/LatinoaméricaFile1");
            string s = p4.PrintText("//depot/TestUnicode/File1");
            Assert.AreEqual("this is file: 1\nEn español.\nΚαλημέρα κόσμε\nこんにちは 世界", s, "Print output failed.");

            s = p4.PrintText("//depot/español/LatinoaméricaFile1");
            Assert.AreEqual("this is file: LatinoaméricaFile1\nEn español.\nΚαλημέρα κόσμε\nこんにちは 世界\n", s, "International print output failed.");

            //// print will always use UTF-8 so the following fail.
            //byte[] b = p4.PrintBinary("//depot/TestUnicode/File1");

            //P4RecordSet rs = p4.Run("sync", "-f", "//depot/TestUnicode/File1");
            //string localPath = rs[0]["clientFile"];
            //byte[] c = File.ReadAllBytes(localPath);
            ////Console.WriteLine(ToHexString(b));
            //Assert.AreEqual(c, b);
        }

        static char[] hexDigits = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        public static string ToHexString(byte[] bytes)
        {
            char[] chars = new char[bytes.Length * 2];
            
            for (int i = 0; i < bytes.Length; i++)
            {
                int b = bytes[i];
                chars[i * 2] = hexDigits[b >> 4];
                chars[i * 2 + 1] = hexDigits[b & 0xF];
            }
            return new string(chars);
        }

        [Test]
        public void Test888_CleanUpFileSystem()
        {
            // Remove all Read-Only attitude 
            RemoveReadOnly(p4ClientRootDirectory);
            Directory.Delete(p4ClientRootDirectory, true);

        }

        [Test]
        public void Test999_Disconnect()
        {
            //duh
            p4.Disconnect();
        }

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
}
