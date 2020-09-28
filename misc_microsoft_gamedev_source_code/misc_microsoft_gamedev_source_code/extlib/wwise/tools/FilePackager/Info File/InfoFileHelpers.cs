using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Xml.Schema;
using AK.Wwise.FilePackager.InfoFile;
using System.IO;

namespace AK.Wwise.FilePackager.InfoFile
{
    class InfoFileHelpers
    {
        const string INFO_SCHEMA_VERSION = "1";

        /// <summary>
        /// Load an soundbanks info file. Its data is returned in a SoundBanksInfo class.
        /// </summary>
        /// <param name="file">INFO file to be loaded</param>
        static internal SoundBanksInfo LoadInfoFile(string in_file, string in_specifiedLanguage)
        {
            XmlReaderSettings readerSettings = new XmlReaderSettings();
            readerSettings.ValidationType = ValidationType.Schema;
            string szSchemaFolder = System.IO.Path.GetDirectoryName(System.Windows.Forms.Application.ExecutablePath);
            string szSchemaFile = System.IO.Path.Combine( szSchemaFolder, @"..\Schemas\SoundbanksInfo.xsd" );
            readerSettings.Schemas.Add("", szSchemaFile);

            SoundBanksInfo data = null;

            using (XmlReader reader = XmlReader.Create(in_file, readerSettings))
            {
               XmlReader newReader = reader;
               //-- Remove 
               if (in_specifiedLanguage != "")
               {
                  newReader = RemoveOtherLanguages(reader, in_specifiedLanguage);
               }

                // Verify the schema version.
               newReader.MoveToContent();
               if (newReader.GetAttribute("SchemaVersion") != INFO_SCHEMA_VERSION)
                {
                    throw new Exception("Wrong Info file schema version.");
                }

                System.Xml.Serialization.XmlSerializer serializer = new System.Xml.Serialization.XmlSerializer(typeof(AK.Wwise.FilePackager.InfoFile.SoundBanksInfo));
                data = (SoundBanksInfo)serializer.Deserialize(newReader);
            }
            
            return data;
        }

        static internal XmlReader RemoveOtherLanguages(XmlReader reader, string in_specifiedLanguage)
        {
            XmlDocument xDoc = new XmlDocument();
            xDoc.Load(reader);            

            XmlNodeList list = xDoc.ChildNodes;
            for (int i = 0; i < list.Count; i++)
            {
               if (RemoveNodes(list[i], in_specifiedLanguage))
                  i--;
            }

            return XmlReader.Create(new StringReader(xDoc.OuterXml));
         }

         static internal bool RemoveNodes(XmlNode node, string in_specifiedLanguage)
         {
            bool removeNode = false;
            if (node.Attributes != null)
            {
               for (int i = 0; i < node.Attributes.Count; i++)
               {
                  if (node.Attributes[i].Name == "Language" && node.Attributes[i].Value.ToString() != in_specifiedLanguage && node.Attributes[i].Value.ToString() != "SFX")
                     removeNode = true;
               }
            }

            if (removeNode)
            {
               node.ParentNode.RemoveChild(node);
               return true;
            }
            else if (node.ChildNodes.Count > 0)
            {
               for (int i = 0; i < node.ChildNodes.Count; i++)
               {
                  if (RemoveNodes(node.ChildNodes[i], in_specifiedLanguage))
                     i--;
               }
            }
            return false;
         }  
    }
}
