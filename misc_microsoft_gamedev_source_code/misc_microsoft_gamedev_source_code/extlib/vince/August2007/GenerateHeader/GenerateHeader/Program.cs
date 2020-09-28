using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.Xsl;

namespace GenerateHeader
{
    class Program
    {
        static void Main(string[] args)
        {

            // Check Argument and determine source file name and path
            string eventFile = null;
            if (args.GetLength(0) == 0)
            {
                eventFile = "EventSpecs.xml";
            }
            else if (args.GetLength(0) == 1)
            {
                eventFile = args[0];
            }
            else
            {
                Console.WriteLine("Error: Too many arguments specified.");
                Program.ShowUsage();
                return;
            }


            if (eventFile.Length < 5)
            {
                Console.WriteLine("Error: Invalid event file specified as input");
                ShowUsage();
                return;
            }

            // Make sure event file exists and read it into memory
            if (!File.Exists(eventFile))
            {
                Console.WriteLine("Error: Input file not found");
                ShowUsage();
                return;
            }
            string currentEvents = File.ReadAllText(eventFile);

            // Now we determine the output file path. It goes
            // in the same folder as the event specification file.
            string headerFile = "VinceEvents.h";
            int lastSlash = eventFile.LastIndexOf('\\');
            if (lastSlash >= 0)
            {
                headerFile = eventFile.Substring(0, lastSlash) + headerFile;
            }

            // See if anything has changed since previously generated version.
            // Skip this test if the header file does not exist, since we will
            // want to always generate a header in this case.
            string previousEventFile = eventFile + ".previous";
            if ( File.Exists(previousEventFile) &&
                 File.Exists(headerFile) )
            {
                string previousEvents = File.ReadAllText(previousEventFile);
                if (previousEvents == currentEvents)
                {
                    Console.WriteLine("No changes detected since previous header generation.");
                    return;
                }
            }

            // Load Style Sheet - we shouldn't need to check for errors, since the style sheet is
            // under internal control of the assembly.
            System.Reflection.Assembly myAssembly = typeof(Program).Assembly;
            Stream stream = myAssembly.GetManifestResourceStream("GenerateHeader.Resources.GenerateEventHeader.xsl");
            XmlReader reader = new XmlTextReader(stream);
            XslCompiledTransform xslt = new XslCompiledTransform();
            xslt.Load(reader, null, null);

            // Apply transformation
            try
            {
                xslt.Transform(eventFile, "VinceEvents.h");
            }
            catch
            {
                Console.WriteLine("Error: Problem encountered attempting to perform transformation. Check XML for errors.");
                return;
            }
            File.WriteAllText(previousEventFile, currentEvents);
            Console.WriteLine("VinceEvents.h successfully generated.");
        }

        static void ShowUsage()
        {
            Console.WriteLine("Usage: GenerateHeader [EventSpecXMLFilePath]");
            Console.WriteLine("       'EventSpecs.xml' assumed if argument omitted.");
            Console.WriteLine("       'VinceEvents.h' will be written to same folder as input file.");
        }
    }
}
