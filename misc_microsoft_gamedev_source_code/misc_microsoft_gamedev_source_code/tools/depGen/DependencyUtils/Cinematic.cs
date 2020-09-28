using System.Xml.Serialization;


namespace DependencyUtils
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

      private cinematicModel[] headField;

      private cinematicShot[] bodyField;

      private string scenariofileField;

      private string soundfileField;

      /// <remarks/>
      [System.Xml.Serialization.XmlArrayItemAttribute("model", IsNullable = false)]
      public cinematicModel[] head
      {
         get
         {
            return this.headField;
         }
         set
         {
            this.headField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlArrayItemAttribute("shot", IsNullable = false)]
      public cinematicShot[] body
      {
         get
         {
            return this.bodyField;
         }
         set
         {
            this.bodyField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string scenariofile
      {
         get
         {
            return this.scenariofileField;
         }
         set
         {
            this.scenariofileField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string soundfile
      {
         get
         {
            return this.soundfileField;
         }
         set
         {
            this.soundfileField = value;
         }
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

      private string nameField;

      private string typeField;

      private string modelfileField;

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string name
      {
         get
         {
            return this.nameField;
         }
         set
         {
            this.nameField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string type
      {
         get
         {
            return this.typeField;
         }
         set
         {
            this.typeField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string modelfile
      {
         get
         {
            return this.modelfileField;
         }
         set
         {
            this.modelfileField = value;
         }
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

      private cinematicShotAnimatedmodel[] animatedmodelField;

      private cinematicShotTag[] tagField;

      private string nameField;

      private string cameraField;

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("animatedmodel")]
      public cinematicShotAnimatedmodel[] animatedmodel
      {
         get
         {
            return this.animatedmodelField;
         }
         set
         {
            this.animatedmodelField = value;
         }
      }


      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("tag")]
      public cinematicShotTag[] tag
      {
         get
         {
            return this.tagField;
         }
         set
         {
            this.tagField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string name
      {
         get
         {
            return this.nameField;
         }
         set
         {
            this.nameField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string camera
      {
         get
         {
            return this.cameraField;
         }
         set
         {
            this.cameraField = value;
         }
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
         get
         {
            return this.modelField;
         }
         set
         {
            this.modelField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string animationfile
      {
         get
         {
            return this.animationfileField;
         }
         set
         {
            this.animationfileField = value;
         }
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

      private string typeField;

      private string nameField;

      private string tomodelField;

      private string toboneField;

      private bool queueField;

      private bool queueFieldSpecified;

      private bool durationFieldSpecified;

      private ushort stringidField;

      private bool stringidFieldSpecified;

      private string soundField;

      private string talkingheadField;

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string type
      {
         get
         {
            return this.typeField;
         }
         set
         {
            this.typeField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string name
      {
         get
         {
            return this.nameField;
         }
         set
         {
            this.nameField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string tomodel
      {
         get
         {
            return this.tomodelField;
         }
         set
         {
            this.tomodelField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string tobone
      {
         get
         {
            return this.toboneField;
         }
         set
         {
            this.toboneField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool queue
      {
         get
         {
            return this.queueField;
         }
         set
         {
            this.queueField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlIgnoreAttribute()]
      public bool queueSpecified
      {
         get
         {
            return this.queueFieldSpecified;
         }
         set
         {
            this.queueFieldSpecified = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public ushort stringid
      {
         get
         {
            return this.stringidField;
         }
         set
         {
            this.stringidField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlIgnoreAttribute()]
      public bool stringidSpecified
      {
         get
         {
            return this.stringidFieldSpecified;
         }
         set
         {
            this.stringidFieldSpecified = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string sound
      {
         get
         {
            return this.soundField;
         }
         set
         {
            this.soundField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string talkinghead
      {
         get
         {
            return this.talkingheadField;
         }
         set
         {
            this.talkingheadField = value;
         }
      }
   }
}
