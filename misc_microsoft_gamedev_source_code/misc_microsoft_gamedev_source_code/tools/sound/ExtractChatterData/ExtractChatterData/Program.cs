using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;
using System.Xml;

namespace ExtractChatterData
{
   class Program
   {
      static void Main(string[] args)
      {
         if (args.Length < 2)
         {
            Console.WriteLine("This function reads the events from a wise generated content file and puts the event ids into an an xml data file for use in the game");
            Console.WriteLine("Use:  extractchatterdata <input txt file> [<input txt file2>] [...] <output xml file>");
            Console.WriteLine("    example:");
            Console.WriteLine("    extractchatterdata C:\\HaloWars\\work\\sound\\wwise_material\\GeneratedSoundBanks\\XBox360\\English(US)\\chatter.txt C:\\HaloWars\\work\\data\\chattertable.xml");
            return;
         }

         //string inputStr = args[0];
         //string outputStr = args[1];
         //string inputStr = "C:\\HaloWars\\work\\sound\\wwise_material\\GeneratedSoundBanks\\XBox360\\English(US)\\chatter.txt";
         //string outputStr = "C:\\HaloWars\\work\\data\\chattertable.xml";


         //Console.WriteLine(inputStr);
         //Console.WriteLine(outputStr);

         List<UInt32> mChatterTable = new List<UInt32>();

         for (int i = 0; i < args.Length - 1; i++)
         {
            string inputStr = args[i];
            Console.WriteLine(inputStr);

            //-- Open the header file
            string[] headerFile = File.ReadAllLines(inputStr);
            bool foundEvents = false;
            foreach (string str in headerFile)
            {
               if (str.StartsWith("Event"))
               {
                  foundEvents = true;
                  continue;  //data starts on the next line
               }

               if (foundEvents == false)
                  continue;

               if (str.StartsWith("\t"))
               {
                  //-- Just get the last 3 pieces
                  string[] splitSpace = str.Split('\t');
                  if (splitSpace.Length >= 2)
                  {
                     UInt32 index = UInt32.Parse(splitSpace[1]);
                     mChatterTable.Add(index);
                  }
               }
               else
                  break;
            }
         }

         string outputStr = args[args.Length-1];
         Console.WriteLine(outputStr);
         XmlTextWriter textWriter = new XmlTextWriter(outputStr, null);

         textWriter.WriteStartDocument();
         textWriter.Formatting = Formatting.Indented;

         textWriter.WriteStartElement("ChatterIDs");

         foreach (UInt32 id in mChatterTable)
         {
            textWriter.WriteStartElement("ID");
            textWriter.WriteString(id.ToString());
            textWriter.WriteEndElement();
         }

         textWriter.WriteEndElement();
         textWriter.WriteEndDocument();
         textWriter.Close();
      }
   }
}
