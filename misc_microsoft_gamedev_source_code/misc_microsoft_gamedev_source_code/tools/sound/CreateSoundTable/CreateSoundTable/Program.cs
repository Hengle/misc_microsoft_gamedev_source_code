using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;
using System.Xml;

namespace CreateSoundTable
{
	class TableItem
	{
		public UInt32 cueIndex;
		public string cueName;
	}

	class Program
	{
		static void Main(string[] args)
		{
			if(args.Length != 2)
			 return;

   		string inputStr = args[0];
			string outputStr = args[1];

         /*string inputStr = "C:\\x\\work\\sound\\wwise_material\\GeneratedSoundBanks\\wwise_ids.h";
         string outputStr = "C:\\x\\work\\data\\soundtable.xml";*/



			Console.WriteLine(inputStr);
			Console.WriteLine(outputStr);

			List<TableItem> mSoundTable = new List<TableItem>();        
			//-- Open the header file
			long bracketIndex = -1;
			bool foundEvents = false;

			string[] headerFile = File.ReadAllLines(inputStr);
			foreach(string str in headerFile)
			{
				if(str.Contains("EVENTS"))
					foundEvents = true;

				if (foundEvents == false)
					continue;

				if(str.Contains("{"))
				{
					if(bracketIndex == -1)
						bracketIndex = 1;
					else
						bracketIndex++;

					continue;
				}

				if(str.Contains("}"))
				{					
					bracketIndex--;
				}

				if (bracketIndex > 0)
				{
               //-- Just get the last 3 pieces
               string[] splitSpace = str.Split(' ');
               if (splitSpace.Length >= 3)
               {
                  int idIdx = splitSpace.Length-1;
                  int nameIdx = splitSpace.Length - 3;

                  //-- Remove the U;
                  splitSpace[idIdx] = splitSpace[idIdx].Replace("U;", "");

                  TableItem item = new TableItem();
                  item.cueName = splitSpace[nameIdx];
                  item.cueIndex = UInt32.Parse(splitSpace[idIdx]);
                  mSoundTable.Add(item);

               }              
				}
			
				if(bracketIndex == 0)
					break;
			}

			XmlTextWriter textWriter = new XmlTextWriter(outputStr, null);

			textWriter.WriteStartDocument();
			textWriter.Formatting = Formatting.Indented;

			textWriter.WriteStartElement("Table");

			foreach(TableItem item in mSoundTable)
			{
				textWriter.WriteStartElement("Sound");

				textWriter.WriteStartElement("CueName");
				textWriter.WriteString(item.cueName.ToLower());
				textWriter.WriteEndElement();

				textWriter.WriteStartElement("CueIndex");				
				textWriter.WriteString(item.cueIndex.ToString());
				textWriter.WriteEndElement();				

				textWriter.WriteEndElement();
			}


			textWriter.WriteEndElement();
			textWriter.WriteEndDocument();
			textWriter.Close();  

		}
	}
}