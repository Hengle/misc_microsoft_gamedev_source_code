using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using System.Xml.Serialization;

namespace EditorCore.Controls.GameSpecific
{
   public partial class EnumGrabber : UserControl
   {
      public EnumGrabber()
      {
         InitializeComponent();
      }
   }

   public class EnumHelper
   {
      //Grab Enums


   }

   //Settings for all stuff..
   //Regex


   //	<Enums>
	//<Enum Name="">0</Enum>

   [XmlRoot("Enumeration")]
   public class EnumerationXml
   {

      [XmlArrayItem(ElementName = "Enum", Type = typeof(EnumXml))]
      [XmlArray("Enums")]
      public List<EnumXml> Categories = new List<EnumXml>();

   }

   [XmlRoot("Enum")]
   public class EnumXml
   {
      [XmlAttribute]
      public string Name = "";
      [XmlText]
      public string Value = "";

   }



}
