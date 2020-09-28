using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;
using System.Xml;

namespace BankChecker
{
   class TableItem
   {
		public string cueName;      
	}

   class Program
   {
      public static List<TableItem> mEvents;

      static void Main(string[] args)
      {
         XmlTextReader reader = new XmlTextReader("c:\\x\\work\\data\\soundtable.xml");

         mEvents = new List<TableItem>();        

         while(reader.Read())
         {
            if(reader.Name == "CueName")
            {           
               TableItem item = new TableItem();               
               item.cueName = reader.ReadElementContentAsString();
               if(item.cueName.Contains("xtnd"))
                  continue;
               mEvents.Add(item);
            }
         }

         //-- Go through the game data xmls and cross off whichever is used
         /*ExamineFile("c:\\x\\work\\data\\objects.xml");
         ExamineFile("c:\\x\\work\\data\\soundinfo.xml");
         ExamineFile("c:\\x\\work\\data\\squads.xml");
         ExamineFile("c:\\x\\work\\data\\impactsounds.xml");
         ExamineFile("c:\\x\\work\\data\\techs.xml");*/
         ExamineDir("c:\\x\\work\\data\\", false);

         //-- Go through all vis files and check for sound tags
         ExamineDir("c:\\x\\work\\art\\", false);

         //-- Go through all scenario files and check for sound tags
         ExamineDir("c:\\x\\work\\scenario\\", false);

         //-- Go through all code files and check for sound tags
         ExamineDir("c:\\x\\code\\xgame\\",  false);
         ExamineDir("c:\\x\\code\\xsound\\",  false);

         StreamWriter writer = new StreamWriter("c:\\x\\work\\sound\\unusedEvents.txt");
         foreach (TableItem item in mEvents)
         {
            writer.WriteLine(item.cueName);
         }
         writer.Close();

      }

      static public void ExamineDir(string strDir, bool recursive)
      {
         // Make sure we catch exceptions 
         // in looking at this directory.
         try
         {
            if (recursive == false)
            {
               ExamineFiles(strDir);
            }

            // Loop through the list of directories.
            foreach (string strDirName in Directory.GetDirectories(strDir))
            {              
               ExamineFiles(strDirName);
               ExamineDir(strDirName, true);
            }
         }
         catch { }
      }

      static public void ExamineFiles(string strDirName)
      {
         // Display the directory name.
         string[] files = Directory.GetFiles(strDirName);
         foreach (string file in files)
         {
            ExamineFile(file);
         }
      }

      static public void ExamineFile(string file)
      {
         if (file == "c:\\x\\code\\xsound\\Wwise_IDs.h")
            return;

         if (file == "c:\\x\\work\\data\\soundinfo.xml")
            return;

         if (file == "c:\\x\\work\\data\\soundtable.xml")
            return;

         //-- Is it a vis file?
         string[] split = file.Split('.');
         if (split.Length == 2 && (split[1] == "vis" || split[1] == "scn" || split[1] == "h" || split[1] == "cpp" || split[1] == "xml" || split[1] == "dmg"))
         {
            //-- Go through the object xml and cross off whichever is used
            string[] visFile = File.ReadAllLines(file);
            foreach (string str in visFile)
            {
               string toLower = str.ToLower();
               int numItems = mEvents.Count;
               for (int i = numItems - 1; i >= 0; i--)
               {
                  if (toLower.Contains(mEvents[i].cueName))
                  {
                     mEvents.RemoveAt(i);
                  }
               }
            }
         }
      }
   }
}
