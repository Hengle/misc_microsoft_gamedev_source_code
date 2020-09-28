using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using EditorCore;
using System.IO;
using System.Xml;
using SimEditor;


namespace PhoenixEditor
{
   public partial class ProtoObjectQuickEditor : Form
   {
      public ProtoObjectQuickEditor()
      {
         InitializeComponent();

         loadXML();

      }
      XmlDocument doc = null;
      public void loadXML()
      {
         string fileName = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "xobjects.xml");

         doc = new XmlDocument();
         doc.Load(fileName);

         XmlNodeList units = doc.GetElementsByTagName("Unit");

         // Retrieve a reference to the first product.
         //XmlNode product = doc.GetElementsByTagName("products")[0];
         // Find the price under this product.
         //XmlNode price = ((XmlElement)product).GetElementsByTagName("productPrice")[0];

        
         foreach (XmlNode unit in units)
         {
            //this.NodeComboBox.Items.Add(unit.)

            XmlAttribute nameAttr = (XmlAttribute)unit.Attributes.GetNamedItem("name");

            XmlAttribute idAttr = (XmlAttribute)unit.Attributes.GetNamedItem("id");

            int id = System.Convert.ToInt32((string)idAttr.Value) ;
            if(id > mHightestID)
            {
               mHightestID = id;
            }

            XmlNode animFile = ((XmlElement)unit).GetElementsByTagName("Visual")[0];
            if (animFile != null)
            {
               string animFileString = animFile.InnerText;
               this.NodeComboBox.Items.Add(new UnitHolder((string)nameAttr.Value, unit));
            }
            //XmlNode price = ((XmlElement)product).GetElementsByTagName("name")

            // Get the inner text of each matching element.
            //otalPrice += Decimal.Parse(price.ChildNodes[0].Value);
         }

         unitcountlabel.Text = mHightestID.ToString() + " units in xobjects.xml";


      }
      public int mHightestID = 0;
      class UnitHolder
      {
         public XmlNode mNode = null;
         public string mName = "";
         public UnitHolder(string name, XmlNode node)
         {
            mNode = node;
            mName = name;
         }
         public override string ToString()
         {
            return mName;
         }
      }


      private void AddViaAnimButton_Click(object sender, EventArgs e)
      {
         try
         {
            string filename = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "xobjects.xml");

            if(File.GetAttributes(filename) == FileAttributes.ReadOnly)
            {
               MessageBox.Show(filename + " is readonly.  please check it out");
            }

            if (NodeComboBox.SelectedItem == null)
               return;
            UnitHolder copyholder = NodeComboBox.SelectedItem as UnitHolder;
            if (copyholder == null)
               return;


            OpenFileDialog d = new OpenFileDialog();
            d.Filter = "AnimXML (*.xml)|*.xml";
            d.FilterIndex = 0;
            d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameArtDirectory;

            if (d.ShowDialog() == DialogResult.OK)
            {
               //Validate xml
               List<string> models = SimUnitXML.getGrannyFileNames(d.FileName);
               if (models.Count == 0)
               {
                  if(MessageBox.Show(String.Format("No models found in {0} add anyway?",d.FileName), "", MessageBoxButtons.YesNo) == DialogResult.Yes)
                  {

                  }
                  else
                  {
                     return;
                  }

               }


               string name = Path.GetFileNameWithoutExtension(d.FileName);

               //check for duplicates
               foreach(UnitHolder holder in this.NodeComboBox.Items)
               {
                  if(holder.mName == name)
                  {
                     MessageBox.Show(name + " has already been added to xobjects");
                     return;
                  }
               }



               mHightestID++;
               int id = mHightestID;

               string artpath = CoreGlobals.getWorkPaths().mGameArtDirectory + "\\";
               string animFileName = d.FileName.Substring(artpath.Length);
               
               //BuildNode
               XmlNode unit = copyholder.mNode.Clone();
               XmlAttribute nameAttr = (XmlAttribute)unit.Attributes.GetNamedItem("name");
               nameAttr.Value = name;
               XmlAttribute idAttr = (XmlAttribute)unit.Attributes.GetNamedItem("id");
               idAttr.Value = id.ToString();
               XmlNode animFile = ((XmlElement)unit).GetElementsByTagName("Visual")[0];
               animFile.InnerText = animFileName;

               //Add to File
               XmlNode protoNode = doc.GetElementsByTagName("Proto")[0];

               protoNode.AppendChild(unit);
               doc.Save(filename);
               CoreGlobals.getSaveLoadPaths().mGameArtDirectory = Path.GetDirectoryName(d.FileName);

               listBox1.Items.Add(String.Format("Added: {0}" , name));

               this.NodeComboBox.Items.Add(new UnitHolder(name, unit));

               foreach (string modelname in models)
               {

                  listBox1.Items.Add(String.Format("    Model: {0}", modelname));
               }
               
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }


      
   }
}