using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Xml.Serialization;

namespace ParticleEditor
{
   public class ParticleEffectData
   {
      public ParticleEffectData()
      {
      }


      public void loadXML(string fileName)
      {
      }


   }


   [XmlRoot("ParticleEffectDefinition")]
   public class ParticleEffectDefinition
   {
      [XmlElement("ParticleEmitterDefinition")]
      public class ParticleEmitterDefinition
      {
         [XmlElement("EmitterProperties")]
         public class EmitterProperties
         {
            [XmlElement("TiedToEmitter")]
            public bool mbTiedToEmitter;
            [XmlIgnore]
            public bool TiedToEmitter
            {
               get
               {
                  return mbTiedToEmitter;
               }
               set
               {
                  mbTiedToEmitter = value;
               }
            }
         }
      }
   }
}
