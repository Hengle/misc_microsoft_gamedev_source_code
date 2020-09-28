using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

using ConsoleUtils;
using DatabaseUtils;
using DependencyUtils;



namespace depGen
{
   class Program
   {

      static int Main(string[] args)
      {
#if !DEBUG               
         try
#endif         
         {
            long g_version_number = 2;

            bool bQuiet = false;
            bool bCivs = false;
            bool bOutputTree = false;
            bool bOutputImage = false;
            bool bFileSizes = false;
            bool bOutputUnitList = false;
            string logFileName = null;
            string logErrorFileName = null;

            List<string> m_filesToLoad = new List<string>();

            // Process command line arguments
            //
            int argc = args.GetLength(0);
            for (int i = 0; i < argc; i++)
            {
               string argument = args[i];

               if (argument.StartsWith("-"))
               {
                  switch (argument.ToLower())
                  {
                     case "-quiet":
                        bQuiet = true;
                        break;

                     case "-civs":
                        bCivs = true;
                        break;

                     case "-outputtree":
                        bOutputTree = true;
                        break;

                     case "-outputunitlist":
                        bOutputUnitList = true;
                        break;

                     case "-outputimage":
                        bOutputImage = true;
                        break;

                    case "-filesize":
                        bFileSizes = true;
                        break;                        

                     case "-logfile":
                        i++;
                        logFileName = args[i];
                        break;

                     case "-errorlogfile":
                        i++;
                        logErrorFileName = args[i];
                        break;                       

                     default:
                        ConsoleOut.Write(ConsoleOut.MsgType.Error, "Invalid parameter: {0}.\n", argument);
                        printHelp(g_version_number);
                        return 1;
                  }
               }
               else
               {
                  string filename = null;
                  if (Path.IsPathRooted(argument))
                  {
                     if (argument.StartsWith(CoreGlobals.getWorkPaths().mGameDirectory, true, System.Globalization.CultureInfo.CurrentCulture))
                     {
                        filename = argument.Substring(CoreGlobals.getWorkPaths().mGameDirectory.Length + 1);
                     }
                     else
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Error, "Invalid file \"{0}\".  File must reside in the within the Game directory: \"{1}\".\n", argument, CoreGlobals.getWorkPaths().mGameDirectory);
                     }
                  }
                  else
                  {
                     filename = argument;
                  }

                  if (!String.IsNullOrEmpty(filename))
                  {
                     m_filesToLoad.Add(filename);
                  }
               }
            }

            if ((!bCivs) && (m_filesToLoad.Count == 0))
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Error, "No files specified to process!\n");
               printHelp(g_version_number);
               return 1;
            }

            ConsoleOut.init(bQuiet, logFileName, logErrorFileName);


            // process civs
            //
            if (bCivs)
            {
               // Read database        
               Database.init();

               
               ConsoleOut.Write(ConsoleOut.MsgType.Info, "Archiving Civs...\n");

               int civCount = Database.getCivCount();
               for (int i = 0; i < civCount; i++)
               {
                  string civName = Database.getCivName(i);

                  // Get the list of vis files needed for this civ
                  //
                  List<string> dependenciesList = new List<string>();
                  Database.processCiv(i, dependenciesList, bOutputUnitList);


                  if (!bOutputUnitList)
                  {
                     // Now compute all dependencies for each vis file
                     DependencyTree dependencyTree = new DependencyTree(dependenciesList);
                     dependencyTree.process();

                     string outputFilename;

                     if (bOutputTree)
                     {
                        // Write .xml file
                        outputFilename = civName.ToLower() + ".xml";
                        dependencyTree.writeFileXML(outputFilename, false);
                     }
                     else
                     {
                        // Write .txt file
                        outputFilename = civName.ToLower() + ".txt";
                        dependencyTree.writeFileTxt(outputFilename);
                     }

                     ConsoleOut.Write(ConsoleOut.MsgType.Info, "Wrote File \"{0}\".\n", outputFilename);

                     if (bOutputImage)
                     {
                        // Write tree hierarchy image
                        string outputTreeFilename = civName.ToLower() + ".png";
                        dependencyTree.writeGraphPNG(outputTreeFilename);

                        ConsoleOut.Write(ConsoleOut.MsgType.Info, "Wrote Tree Image File \"{0}\".\n", outputTreeFilename);
                     }
                  }
                  else
                  {
                     dependenciesList.Sort();

                     string outputFilename = civName.ToLower() + ".txt";
                     using (StreamWriter sw = File.CreateText(outputFilename))
                     {
                        foreach (string name in dependenciesList)
                        {
                           sw.WriteLine(name);
                        }
                        sw.Close();
                     }
                  }
               }
            }


            // process all files
            //
            foreach (string filename in m_filesToLoad)
            {
               // must initialize database for scenario or cin files
               if (!Database.isInitialized() )
               {
                  if((String.Compare(Path.GetExtension(filename), ".scn", true) == 0) || 
                     (String.Compare(Path.GetExtension(filename), ".cin", true) == 0))
                  {
                     Database.init();
                  }
               }


               List<string> fileList = new List<string>();
               fileList.Add(filename);

               DependencyTree dependencyTree = new DependencyTree(fileList);
               dependencyTree.process();

               string outputFilename;

               if (bOutputTree)
               {
                  // Write .xml file               
                  outputFilename = Path.ChangeExtension(Path.GetFileName(filename), ".xml");
                  dependencyTree.writeFileXML(outputFilename, bFileSizes);
               }
               else
               {
                  // Write .txt file               
                  outputFilename = Path.ChangeExtension(Path.GetFileName(filename), ".txt");
                  dependencyTree.writeFileTxt(outputFilename);
               }

               ConsoleOut.Write(ConsoleOut.MsgType.Info, "\n");
               ConsoleOut.Write(ConsoleOut.MsgType.Info, "Writing File \"{0}\".\n", outputFilename);

               if (bOutputImage)
               {
                  // Write tree hierarchy image
                  string outputTreeFilename = Path.ChangeExtension(Path.GetFileName(filename), ".png");
                  ConsoleOut.Write(ConsoleOut.MsgType.Info, "Writing Tree Image File \"{0}\".\n", outputTreeFilename);

                  dependencyTree.writeGraphPNG(outputTreeFilename);
               }
               
               if (bOutputUnitList)
               {
                  // Write unit list 
                  string outputUnitListFilename = Path.ChangeExtension(Path.GetFileName(filename), ".unitlist");
                  ConsoleOut.Write(ConsoleOut.MsgType.Info, "Writing Unit List File \"{0}\".\n", outputUnitListFilename);

                  dependencyTree.writeUnitListTxt(outputUnitListFilename);
               }

               ConsoleOut.Write(ConsoleOut.MsgType.Info, "\n");
            }


            ConsoleOut.Write(ConsoleOut.MsgType.Info, "Done.\n");

            ConsoleOut.deinit();
         }
#if !DEBUG         
         catch(System.Exception ex)
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "Unhandled exception!\n");
            
            if (System.Diagnostics.Debugger.IsAttached)
               throw ex;

            return 1;
         }
#endif         

         return 0;
      }


      //==============================================================================
      // printHelp
      //==============================================================================
      static void printHelp(long version)
      {
         ConsoleOut.Write("[DEPGEN version: {0}]\n", version);
         ConsoleOut.Write("Description: depGen.exe creates a dependency lists for each file passed in.\n");
         ConsoleOut.Write("             (the file lists returned will not have duplicates).\n");
         ConsoleOut.Write("\n");
         ConsoleOut.Write("Usage: depGen <options> <filenames>\n");
         ConsoleOut.Write("Options:\n\n");
         ConsoleOut.Write(" -quiet                    Disable all console output.\n");
         ConsoleOut.Write(" -civs                     Create per civilization dependency files.  DepGen will look at civs.xml and create a dependency list for each civilization found.\n");
         ConsoleOut.Write(" -outputtree               Writes out an xml file containing the dependency tree (.xml).\n");
         ConsoleOut.Write(" -outputimage              Writes out a dependency tree image graph (.png).\n");
         ConsoleOut.Write(" -logfile filename         Outputs log file.\n");
         ConsoleOut.Write(" -errorlogfile filename    Outputs error log file.\n");
      }
   }


}
