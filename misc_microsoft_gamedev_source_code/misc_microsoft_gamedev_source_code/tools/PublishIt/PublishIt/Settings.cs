using System;
using System.Collections.Generic;
using System.Text;

using System.IO;
using System.Xml;
using System.Windows.Forms;

namespace PublishIt
{
    class Settings
	{
		private XmlDocument xmlDocument = new XmlDocument();
		private String documentPath;

		public Settings(String docPath)
		{
			documentPath = docPath;
			try
			{
				xmlDocument.Load(documentPath);
			}	
			catch (FileNotFoundException ex)
			{
				MessageBox.Show("Data file missing!", "Missing File");
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message);
			}
		}

		public String getSetting(String setting)
		{
			XmlNode xmlNode = xmlDocument.SelectSingleNode("settings/" + setting);
			if (xmlNode != null)
			{
				return (xmlNode.InnerText);
			}
			else
				return "";
		}

		public bool PutSetting(string setting, string value)
		{
			String path = String.Concat("settings/", setting);
			XmlNode xmlNode = xmlDocument.SelectSingleNode(path);
			xmlNode.InnerText = value;
			try
			{
				xmlDocument.Save(documentPath);
				return true;
			}
			catch
			{
				return false;
			}
		}
		public bool PutAttribute(String node, String attr, String attrValue)
		{
			XmlNode xmlNode = xmlDocument.SelectSingleNode("profiles");
			try
			{
				if (xmlNode.Attributes.Count == 0)
				{
					XmlAttribute attribute = xmlDocument.CreateAttribute(attr);
					attribute.Value = attrValue;
					xmlNode.Attributes.SetNamedItem(attribute);
				}
				else
				{
					XmlAttribute attribute = xmlNode.Attributes[attr];
					attribute.Value = attrValue;
				}
				xmlDocument.Save(documentPath);
				return true;
			}
			catch (System.Exception e)
			{
				return false;
			}
		}
	}
}
