using System.Xml.Serialization;
using System.Collections.Generic;


namespace PhoenixEditor.CinematicEditor
{

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [System.Xml.Serialization.XmlRootAttribute(Namespace = "", IsNullable = false)]
   public partial class cinematic
   {

      private List<cinematicModel> headField = new List<cinematicModel>();

      private List<cinematicShot> bodyField = new List<cinematicShot>();

      private string scenariofileField;

      private string soundfileField;

      /// <remarks/>
      [System.Xml.Serialization.XmlArrayItemAttribute("model", IsNullable = false)]
      public List<cinematicModel> head
      {
         get { return this.headField; }
         set { this.headField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlArrayItemAttribute("shot", IsNullable = false)]
      public List<cinematicShot> body
      {
         get { return this.bodyField; }
         set { this.bodyField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string scenariofile
      {
         get { return this.scenariofileField; }
         set { this.scenariofileField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string soundfile
      {
         get { return this.soundfileField; }
         set { this.soundfileField = value; }
      }
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class cinematicModel
   {
      public enum ModelType
      {
         proto = 0,
         gr2
      }

      private string nameField;

      private ModelType typeField;

      private string modelfileField;

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string name
      {
         get { return this.nameField; }
         set { this.nameField = value; }
      }

      [System.Xml.Serialization.XmlAttributeAttribute()]
      public ModelType type
      {
         get { return this.typeField; }
         set { this.typeField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string modelfile
      {
         get { return this.modelfileField; }
         set { this.modelfileField = value; }
      }
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class cinematicShot
   {
      private List<cinematicShotAnimatedmodel> animatedmodelField = new List<cinematicShotAnimatedmodel>();

      private List<cinematicShotTag> tagField = new List<cinematicShotTag>();

      private string nameField;

      private string cameraField;

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("animatedmodel")]
      public List<cinematicShotAnimatedmodel> animatedmodel
      {
         get { return this.animatedmodelField; }
         set { this.animatedmodelField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("tag")]
      public List<cinematicShotTag> tag
      {
         get { return this.tagField; }
         set { this.tagField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string name
      {
         get { return this.nameField; }
         set { this.nameField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string camera
      {
         get { return this.cameraField; }
         set { this.cameraField = value; }
      }
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class cinematicShotAnimatedmodel
   {
      private string modelField;

      private string animationfileField;

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string model
      {
         get { return this.modelField; }
         set { this.modelField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string animationfile
      {
         get { return this.animationfileField; }
         set { this.animationfileField = value; }
      }
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class cinematicShotTag
   {
      public enum TagType
      {
         Particle = 0,
         Sound,
         Trigger
      }

      private TagType typeField;

      private decimal timeField = 0.0M;

      private string nameField;

      private string toboneField;

      private decimal lifespanField = 0.25M;

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public TagType type
      {
         get { return this.typeField; }
         set { this.typeField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public decimal time
      {
         get { return this.timeField; }
         set { this.timeField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string name
      {
         get { return this.nameField; }
         set { this.nameField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string tobone
      {
         get { return this.toboneField; }
         set { this.toboneField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public decimal lifespan
      {
         get { return this.lifespanField; }
         set { this.lifespanField = value; }
      }
   }
}
