using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using ConsoleUtils;



namespace DatabaseUtils
{

   class TechEffectTarget
   {
      public enum TargetType
      {
         cProtoUnit,
         cProtoSquad
      };


      public TargetType mTargetType = TargetType.cProtoUnit;
      public long mTargetID = -1;
   }



   public class TechEffect
   {
      public enum EffectType
      {
         cEffectData,
         cEffectTransformUnit,
         cEffectTransformProtoUnit,
         cEffectTransformProtoSquad,
         cEffectAge,
         cEffectPower,
         cEffectTech,
         cEffectAbility,
      };

      public EffectType effectType;
      public long effectValue;
      public long objectType;
      List<TechEffectTarget> mTargets = new List<TechEffectTarget>();


      //==============================================================================
      // load
      //==============================================================================
      public bool load(XmlNode techEffect, string techName)
      {
         XmlNode typeNode = techEffect.Attributes.GetNamedItem("type");
         if (typeNode == null)
            return false;

         switch (typeNode.FirstChild.Value)
         {
            case "Data":
               {
                  effectType = EffectType.cEffectData;

                  XmlNode subTypeNode = techEffect.Attributes.GetNamedItem("subtype");
                  if (subTypeNode == null)
                     return false;

                  // Only care about 'Enable' subtype else disregard
                  if (String.Compare(subTypeNode.FirstChild.Value, "Enable", true) != 0)
                     return false;



                  // read target
                  XmlNodeList targetNodes = techEffect.SelectNodes("./Target");
                  foreach (XmlNode targetNode in targetNodes)
                  {
                     TechEffectTarget target = new TechEffectTarget();

                     XmlNode targetTypeNode = targetNode.Attributes.GetNamedItem("type");
                     if (targetTypeNode == null)
                        continue;

                     switch (targetTypeNode.FirstChild.Value)
                     {
                        case "ProtoUnit":
                           target.mTargetType = TechEffectTarget.TargetType.cProtoUnit;

                           string unitName = targetNode.FirstChild.Value;
                           long unitID;

                           if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out unitID))
                           {
                              target.mTargetID = unitID;
                           }
                           else
                           {
                              ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" has an effect that is referring to Unit \"{1}\" which does not exist.\n", techName, unitName);
                              return (false);
                           }
                           break;

                        case "ProtoSquad":
                           target.mTargetType = TechEffectTarget.TargetType.cProtoSquad;

                           string squadName = targetNode.FirstChild.Value;
                           long squadID;

                           if (Database.m_protoSquadTable.TryGetValue(squadName.ToLower(), out squadID))
                           {
                              target.mTargetID = squadID;
                           }
                           else
                           {
                              ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" has an effect that is referring to Squad \"{1}\" which does not exist.\n", techName, squadName);
                              return (false);
                           }
                           break;
                     }

                     mTargets.Add(target);
                  }
               }
               break;

            case "TransformUnit":
               {
                  effectType = EffectType.cEffectTransformUnit;

                  string unitName = techEffect.FirstChild.Value;
                  long unitID;
                  if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out unitID))
                  {
                     effectValue = unitID;
                  }
                  else
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" has an effect that is referring to Unit \"{1}\" which does not exist.\n", techName, unitName);
                     return (false);
                  }
               }
               break;

            case "TransformProtoUnit":
               {
                  effectType = EffectType.cEffectTransformProtoUnit;

                  XmlNode fromTypeNode = techEffect.Attributes.GetNamedItem("FromType");
                  if (fromTypeNode == null)
                     return false;

                  XmlNode toTypeNode = techEffect.Attributes.GetNamedItem("ToType");
                  if (toTypeNode == null)
                     return false;


                  long unitID;
                  string unitName = fromTypeNode.FirstChild.Value;

                  if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out unitID))
                  {
                     objectType = unitID;
                  }
                  else
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" has an effect that is referring to Unit \"{1}\" which does not exist.\n", techName, unitName);
                     return (false);
                  }


                  unitName = toTypeNode.FirstChild.Value;

                  if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out unitID))
                  {
                     effectValue = unitID;
                  }
                  else
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" has an effect that is referring to Unit \"{1}\" which does not exist.\n", techName, unitName);
                     return (false);
                  }
               }
               break;

            case "TransformProtoSquad":
               {
                  effectType = EffectType.cEffectTransformProtoSquad;

                  XmlNode fromTypeNode = techEffect.Attributes.GetNamedItem("FromType");
                  if (fromTypeNode == null)
                     return false;

                  XmlNode toTypeNode = techEffect.Attributes.GetNamedItem("ToType");
                  if (toTypeNode == null)
                     return false;


                  long squadID;
                  string squadName = fromTypeNode.FirstChild.Value;

                  if (Database.m_protoSquadTable.TryGetValue(squadName.ToLower(), out squadID))
                  {
                     objectType = squadID;
                  }
                  else
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" has an effect that is referring to Squad \"{1}\" which does not exist.\n", techName, squadName);
                     return (false);
                  }

                  squadName = toTypeNode.FirstChild.Value;

                  if (Database.m_protoSquadTable.TryGetValue(squadName.ToLower(), out squadID))
                  {
                     effectValue = squadID;
                  }
                  else
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" has an effect that is referring to Squad \"{1}\" which does not exist.\n", techName, squadName);
                     return (false);
                  }
               }
               break;

            case "SetAge":
               effectType = EffectType.cEffectAge;
               return false;

            case "GodPower":
               effectType = EffectType.cEffectPower;

               string powerName = techEffect.FirstChild.Value;
               long powerID;
               if (Database.m_tablePowers.TryGetValue(powerName.ToLower(), out powerID))
               {
                  effectValue = powerID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" has an effect that is referring to Power \"{1}\" which does not exist.\n", techName, powerName);
                  return (false);
               }
               break;

            case "TechStatus":
               effectType = EffectType.cEffectTech;
               return false;

            case "Ability":
               effectType = EffectType.cEffectAbility;
               return false;
         }

         return true;
      }


      //==============================================================================
      // apply
      //==============================================================================
      public void apply(TechTree techTree)
      {
         switch (effectType)
         {
            case EffectType.cEffectData:
               {
                  long count = mTargets.Count;
                  for (long i = 0; i < count; i++)
                  {
                     TechEffectTarget target = mTargets[(int)i];
                     switch (target.mTargetType)
                     {
                        case TechEffectTarget.TargetType.cProtoUnit: //FIXME - SHOULD BE cTargetObjectType
                           {
                              techTree.applyProtoObjectEffect(target.mTargetID);
                              break;
                           }

                        case TechEffectTarget.TargetType.cProtoSquad:
                           {
                              techTree.applyProtoSquadEffect(target.mTargetID);
                              break;
                           }
                     }
                  }
                  break;
               }

            case EffectType.cEffectTransformUnit:

               techTree.buildUnit(effectValue);
               //techTree.applyProtoObjectEffect(effectValue);
               break;

            case EffectType.cEffectTransformProtoUnit:

               techTree.buildUnit(effectValue);
               //techTree.applyProtoObjectEffect(effectValue);
               break;

            case EffectType.cEffectTransformProtoSquad:

               techTree.buildSquad(effectValue); 
               //techTree.applyProtoSquadEffect(effectValue);
               break;

            case EffectType.cEffectAge:
               break;

            case EffectType.cEffectPower:

               techTree.applyEffectGodPower(effectValue);
               break;

            case EffectType.cEffectTech:

               techTree.makeObtainable(effectValue);
               break;

            case EffectType.cEffectAbility:
               break;
         }
      }
   }
}
