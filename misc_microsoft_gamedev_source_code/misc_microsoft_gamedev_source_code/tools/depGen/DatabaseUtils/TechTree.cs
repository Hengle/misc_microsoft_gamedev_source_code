using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.IO;

using ConsoleUtils;


namespace DatabaseUtils
{
   public enum TechStatus
   {
      cStatusUnobtainable,
      cStatusObtainable,
      cStatusAvailable,
      cStatusResearching,
      cStatusActive,
      cStatusDisabled
   };


   public class TechNode
   {
      public TechStatus    mStatus = TechStatus.cStatusUnobtainable;
      public bool          mFlagForbid = false;


      public bool getFlagForbid()
      {
         return (mFlagForbid);
      }

      public void setFlagForbid(bool v)
      {
         mFlagForbid = v;
      }
   };



   public class TechTree
   {
      private List<TechNode> m_techNodes = new List<TechNode>();
      private List<bool> m_powerActive = new List<bool>();

      private List<bool> m_unitAvailabitily = new List<bool>();
      private List<bool> m_squadAvailabitily = new List<bool>();

      private List<bool> m_unitBuilt = new List<bool>();
      private List<bool> m_squadBuilt = new List<bool>();

      private List<bool> m_unitForbid = new List<bool>();
      private List<bool> m_squadForbid = new List<bool>();

      private List<bool> m_unitExclude = new List<bool>();

      private List<string> m_freeFileList = new List<string>();

      


      public TechTree(List<ProtoTech> protoTechs)
      {

         int techCount = protoTechs.Count;
         for (int i = 0; i < techCount; i++)
         {
            TechNode techNode = new TechNode();

            if(protoTechs[i].bFlagUnobtainable)
               techNode.mStatus = TechStatus.cStatusUnobtainable;
            else
            {
               techNode.mStatus = TechStatus.cStatusObtainable;
               // The game does something different here.  If the tech is unique here it will
               // call checkTech on it.  Not sure if this is needed.
            }

            m_techNodes.Add(techNode);
         }



         int unitCount = Database.m_protoUnits.Count;
         for (int i = 0; i < unitCount; i++)
         {
            m_unitAvailabitily.Add(false);
         }

         for (int i = 0; i < unitCount; i++)
         {
            m_unitBuilt.Add(false);
         }

         for (int i = 0; i < unitCount; i++)
         {
            m_unitForbid.Add(false);
         }

         for (int i = 0; i < unitCount; i++)
         {
            m_unitExclude.Add(false);
         }

         int squadCount = Database.m_protoSquads.Count;
         for (int i = 0; i < squadCount; i++)
         {
            m_squadAvailabitily.Add(false);
         }

         for (int i = 0; i < squadCount; i++)
         {
            m_squadBuilt.Add(false);
         }

         for (int i = 0; i < squadCount; i++)
         {
            m_squadForbid.Add(false);
         }

         int powerCount = Database.m_powers.Count;
         for (int i = 0; i < powerCount; i++)
         {
            m_powerActive.Add(false);
         }


         process();
      }



      //==============================================================================
      // process
      //==============================================================================
      public void process()
      {

         bool bSomethingChanged = true;

         while (bSomethingChanged)
         {
            bSomethingChanged = false;
            for (int i = 0; i < m_techNodes.Count; i++)
            {
               if (m_techNodes[i].getFlagForbid())
                  continue;

               if (m_techNodes[i].mStatus != TechStatus.cStatusObtainable)
                  continue;

               // Check prereqs
               if (checkPrereqs(i))
               {
                  ProtoTech protoTech = Database.m_protoTechs[i];

                  if (protoTech.bFlagShadow)
                  {
                     activateTech(i);

                     bSomethingChanged = true;
                  }
                  else
                  {
                     m_techNodes[i].mStatus = TechStatus.cStatusAvailable;
                  }
               }
            }
         }
      }


      //==============================================================================
      // getDependencyList
      //==============================================================================
      public void getDependencyList(List<string> dependenyList, bool bOutputUnitNamesInsteadOfVisFileNames)
      {
         // Convert array of units names into new array of *.vis files.  One .vis file
         // per unit.
         //
         int unitCount = m_unitBuilt.Count;
         for (int k = 0; k < unitCount; k++)
         {
            if (m_unitBuilt[k] == true)
            {
               if (bOutputUnitNamesInsteadOfVisFileNames)
               {
                  if (!dependenyList.Contains(Database.m_protoUnits[k].mName))
                     dependenyList.Add(Database.m_protoUnits[k].mName);
               }
               else
               {
                  if (Database.m_protoUnits[k].mVisFile != null)
                  {
                     if (!dependenyList.Contains(Database.m_protoUnits[k].mVisFile))
                        if (File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + Database.m_protoUnits[k].mVisFile))
                           dependenyList.Add(Database.m_protoUnits[k].mVisFile);
                  }
                  if (Database.m_protoUnits[k].mImpactDecalRootName != null)
                  {
                     string[] impactDecalFileName = new string[3];

                     impactDecalFileName[0] = String.Concat(Database.m_protoUnits[k].mImpactDecalRootName, "_df.ddx");
                     impactDecalFileName[1] = String.Concat(Database.m_protoUnits[k].mImpactDecalRootName, "_nm.ddx");
                     impactDecalFileName[2] = String.Concat(Database.m_protoUnits[k].mImpactDecalRootName, "_op.ddx");

                     for (long channel = 0; channel < 3; channel++)
                     {
                        if (!dependenyList.Contains(impactDecalFileName[channel]))
                        {
                           if(File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + impactDecalFileName[channel]))
                              dependenyList.Add(impactDecalFileName[channel]);
                        }
                     }
                  }
                  if (Database.m_protoUnits[k].mPhysicsInfoFileName != null)
                  {
                     if (!dependenyList.Contains(Database.m_protoUnits[k].mPhysicsInfoFileName))
                        if (File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + Database.m_protoUnits[k].mPhysicsInfoFileName))
                           dependenyList.Add(Database.m_protoUnits[k].mPhysicsInfoFileName);
                  }
               }
            }
         }

         if (!bOutputUnitNamesInsteadOfVisFileNames)
         {
            // Add free files (impact effects usually)
            int visFileCount = m_freeFileList.Count;
            for (int k = 0; k < visFileCount; k++)
            {
               if (!dependenyList.Contains(m_freeFileList[k]))
                  dependenyList.Add(m_freeFileList[k]);
            }
         }
      }



      //==============================================================================
      // checkPrereqs
      //==============================================================================
      public bool checkPrereqs(long techID)
      {
         ProtoTech protoTech = Database.m_protoTechs[(int)techID];

         bool orPrereqs=protoTech.bFlagOrPrereqs;
         bool anyMet=false;

         // First check tech prereqs
         long prereqCount=protoTech.mTechPrereqs.Count;
         for(long k=0; k<prereqCount; k++)
         {
            if(getTechStatus(protoTech.mTechPrereqs[(int)k]) != TechStatus.cStatusActive)
            {
               if(!orPrereqs)
                  return false;
            }
            else
            {
               if(orPrereqs)
               {
                  anyMet=true;
                  break;
               }
            }
         }

         if(!orPrereqs || !anyMet)
         {
            // Now check unit prereqs
            prereqCount=protoTech.mUnitPrereqs.Count;
            for(long k=0; k<prereqCount; k++)
            {
               //DWORD unitPrereq=protoTech.mUnitPrereqs[k];
               UnitPrereq unitPrereq = protoTech.mUnitPrereqs[(int)k];

               OperatorType opType = unitPrereq.op;
               long unitCount = unitPrereq.count;
               long unitID = unitPrereq.unitID;

               long curUnitCount = m_unitBuilt[(int)unitID] ? 1000000 : 0;

               bool met=false;

               switch(opType)
               {
                  case OperatorType.GreaterThan:
                     if (curUnitCount > unitCount)
                        met = true; 
                     break;
                  case OperatorType.LessThan:
                     if (curUnitCount < unitCount)
                        met = true;
                     break;
                  case OperatorType.Equal:
                     if (curUnitCount == unitCount)
                        met = true;
                     break;
               }

               if(!met)
               {
                  if(!orPrereqs)
                     return false;
               }
               else
               {
                  if(orPrereqs)
                  {
                     anyMet=true;
                     break;
                  }
               }
            }
         }

         if(orPrereqs && !anyMet)
            return false;


         return true;
      }



      //==============================================================================
      // activateTech
      //==============================================================================
      public void activateTech(long techID)
      {
         if ((techID < 0) || (techID >= m_techNodes.Count))
            return;

         TechNode techNode = m_techNodes[(int)techID];

         if (techNode.mStatus == TechStatus.cStatusActive)
            return;


         techNode.mStatus = TechStatus.cStatusActive;

         ProtoTech protoTech = Database.m_protoTechs[(int)techID];
         protoTech.applyEffects(this);

         return;
      }

      //==============================================================================
      // getTechStatus/setTechStatus
      //==============================================================================
      public TechStatus getTechStatus(long techID)
      {
         TechNode techNode = m_techNodes[(int)techID];
         if (techNode != null)
            return techNode.mStatus;

         return TechStatus.cStatusUnobtainable;
      }

      public void setTechStatus(long techID, TechStatus status)
      {
         TechNode techNode = m_techNodes[(int)techID];
         if (techNode != null)
            techNode.mStatus = status;
      }

      public bool getForgidTech(long techID)
      {
         TechNode techNode = m_techNodes[(int)techID];
         if (techNode != null)
            return techNode.getFlagForbid();

         return false;
      }

      public void setForbidTech(long techID, bool forbid)
      {
         TechNode techNode = m_techNodes[(int)techID];
         if (techNode != null)
            techNode.setFlagForbid(forbid);
      }


      public bool getForgidUnit(long unitID)
      {
         if ((unitID < 0) || (unitID >= m_unitForbid.Count))
            return false;

         return m_unitForbid[(int)unitID];
      }

      public void setForbidUnit(long unitID, bool forbid)
      {
         if ((unitID < 0) || (unitID >= m_unitForbid.Count))
            return;

         m_unitForbid[(int)unitID] = forbid;
      }


      public bool getForgidSquad(long squadID)
      {
         if ((squadID < 0) || (squadID >= m_squadForbid.Count))
            return false;

         return m_squadForbid[(int)squadID];
      }

      public void setForbidSquad(long squadID, bool forbid)
      {
         if ((squadID < 0) || (squadID >= m_squadForbid.Count))
            return;

         m_squadForbid[(int)squadID] = forbid;
      }


      public bool getExcludeUnit(long unitID)
      {
         if ((unitID < 0) || (unitID >= m_unitExclude.Count))
            return false;

         return m_unitExclude[(int)unitID];
      }

      public void setExcludeUnit(long unitID, bool exclude)
      {
         if ((unitID < 0) || (unitID >= m_unitExclude.Count))
            return;

         m_unitExclude[(int)unitID] = exclude;
      }

      //==============================================================================
      // applyProtoObjectEffect
      //==============================================================================
      public void applyProtoObjectEffect(long unitID)
      {
         if ((unitID < 0) || (unitID >= m_unitAvailabitily.Count))
            return;

         if (m_unitAvailabitily[(int)unitID] == true)
            return;

         if (m_unitForbid[(int)unitID] == true)
            return;

         // Make the unit available
         m_unitAvailabitily[(int)unitID] = true;
      }

      //==============================================================================
      // applyProtoSquadEffect
      //==============================================================================
      public void applyProtoSquadEffect(long squadID)
      {
         if ((squadID < 0) || (squadID >= m_squadAvailabitily.Count))
            return;

         if (m_squadAvailabitily[(int)squadID] == true)
            return;

         if (m_squadForbid[(int)squadID] == true)
            return;

         // Make the squad available
         m_squadAvailabitily[(int)squadID] = true;
      }


      //==============================================================================
      // makeObtainable
      //==============================================================================
      public void makeObtainable(long techID)
      {
         if ((techID < 0) || (techID >= m_techNodes.Count))
            return;

         TechNode techNode = m_techNodes[(int)techID];
         techNode.mStatus = TechStatus.cStatusObtainable;
      }


      //==============================================================================
      // buildUnit
      //==============================================================================
      public void buildUnit(long unitID)
      {
         if ((unitID < 0) || (unitID >= m_unitBuilt.Count))
            return;

         if (m_unitBuilt[(int)unitID] == true)
            return;

         // Don't think I need this here.  Forbids should only affect what can be trained
         //if (m_unitForbid[(int)unitID] == true)
         //   return;

         if (m_unitExclude[(int)unitID] == true)
            return;

         m_unitBuilt[(int)unitID] = true;
         

         // Also make available all techs that this unit can research
         //
         ProtoUnit unit = Database.m_protoUnits[(int)unitID];
         foreach (long techID in unit.mCommandResearchTechIDs)
         {
            activateTech(techID);
         }


         // Look at unit ability
         //
         if (!String.IsNullOrEmpty(unit.mTriggerScriptFileName))
         {
            string triggerScriptFileName = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\triggerscripts\\" + unit.mTriggerScriptFileName;

            // check if file exists
            if (!File.Exists(triggerScriptFileName))
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" specifies an Ability whose file \"{1}\" does not exist.\n", unit.mName, triggerScriptFileName);
            }
            else
            {
               // Read ability file
               XmlDocument abilityTriggerScripDoc = new XmlDocument();
               CoreGlobals.XmlDocumentLoadHelper(abilityTriggerScripDoc, triggerScriptFileName);

               XmlNodeList triggerSystemNodes = abilityTriggerScripDoc.SelectNodes("/TriggerSystem");
               foreach (XmlNode triggerSystemNode in triggerSystemNodes)
               {
                  activateTriggerSystemNode(triggerSystemNode, triggerScriptFileName);
               }
            }
         }

         // Look at unit ability command
         //
         if (unit.mAbilityCommand != -1)
         {
            Ability ability = Database.m_abilities[(int)unit.mAbilityCommand];

            if (ability.mTriggerScriptFile != null)
            {
               string triggerScriptFileName = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\triggerscripts\\" + ability.mTriggerScriptFile;

               // check if file exists
               if (!File.Exists(triggerScriptFileName))
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" specifies an AbilityCommand \"{1}\" whose TriggerScript file \"{2}\" does not exist.\n", unit.mName, ability.mName, triggerScriptFileName);
               }
               else
               {
                  // Read ability file
                  XmlDocument abilityTriggerScripDoc = new XmlDocument();
                  CoreGlobals.XmlDocumentLoadHelper(abilityTriggerScripDoc, triggerScriptFileName);

                  XmlNodeList triggerSystemNodes = abilityTriggerScripDoc.SelectNodes("/TriggerSystem");
                  foreach (XmlNode triggerSystemNode in triggerSystemNodes)
                  {
                     activateTriggerSystemNode(triggerSystemNode, triggerScriptFileName);
                  }
               }
            }
         }

         // Now process tactics file
         if (!String.IsNullOrEmpty(unit.mTacticsFileName))
         {
            string tacticsFileNameAbsolute = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\tactics\\" + unit.mTacticsFileName;

            // check if file exists
            if (File.Exists(tacticsFileNameAbsolute))
            {
               // Process tactics file
               XmlDocument tacticsDoc = new XmlDocument();
               CoreGlobals.XmlDocumentLoadHelper(tacticsDoc, tacticsFileNameAbsolute);

               XmlNodeList weaponNodes = tacticsDoc.SelectNodes("/TacticData/Weapon");
               foreach (XmlNode weaponNode in weaponNodes)
               {
                  // read projectile (if exists)
                  XmlNode projectileNode = weaponNode.SelectSingleNode("./Projectile");
                  if ((projectileNode != null) && (projectileNode.FirstChild != null))
                  {
                     string unitName = projectileNode.FirstChild.Value;
                     long projectileUnitID;

                     if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out projectileUnitID))
                     {
                        buildUnit(projectileUnitID);
                        //applyProtoObjectEffect(projectileUnitID);
                     }
                     else
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tactic file \"{0}\" is referring to Projectile unit \"{1}\" which is not in objects.xml.\n", unit.mTacticsFileName, unitName);
                     }
                  }

                  // read impact effect (if exists)
                  XmlNode impactEffectNode = weaponNode.SelectSingleNode("./ImpactEffect");
                  if ((impactEffectNode != null) && (impactEffectNode.FirstChild != null))
                  {
                     string impactEffectName = impactEffectNode.FirstChild.Value;
                     string tfxFileName;

                     if (Database.m_impactEffectToTfxName.TryGetValue(impactEffectName.ToLower(), out tfxFileName))
                     {
                        if (!m_freeFileList.Contains(tfxFileName))
                           m_freeFileList.Add(tfxFileName);
                     }
                     else
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tactic file \"{0}\" is referring to ImpactEffect \"{1}\" which is not in impacteffects.xml.\n", unit.mTacticsFileName, impactEffectName);
                     }
                  }

                  // read trigger script
                  XmlNode triggerScriptNode = weaponNode.SelectSingleNode("./TriggerScript");
                  if ((triggerScriptNode != null) && (triggerScriptNode.FirstChild != null))
                  {
                     string triggerScriptFile = triggerScriptNode.FirstChild.Value;
                     string triggerScriptFileName = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\triggerscripts\\" + triggerScriptFile;

                     // check if file exists
                     if (!File.Exists(triggerScriptFileName))
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tactic file \"{0}\" is referring to TriggerScript file \"{1}\" which is not exist.\n", unit.mTacticsFileName, triggerScriptFileName);
                     }
                     else
                     {
                        // Read trigger script
                        XmlDocument externalTriggerScripDoc = new XmlDocument();
                        CoreGlobals.XmlDocumentLoadHelper(externalTriggerScripDoc, triggerScriptFileName);

                        XmlNodeList triggerSystemNodes = externalTriggerScripDoc.SelectNodes("/TriggerSystem");
                        foreach (XmlNode triggerSystemNode in triggerSystemNodes)
                        {
                           activateTriggerSystemNode(triggerSystemNode, triggerScriptFileName);
                        }
                     }
                  }

                  // read physics explosion
                  XmlNode physicsExplosionEffectNode = weaponNode.SelectSingleNode("./CausePhysicsExplosion");
                  if (physicsExplosionEffectNode != null)
                  {
                     // read name
                     XmlNode nameNode = physicsExplosionEffectNode.Attributes.GetNamedItem("particle");

                     if ((nameNode != null) && (nameNode.FirstChild != null))
                     {
                        string pfxFileName = "art\\" + nameNode.FirstChild.Value + ".pfx";
                        string pfxFileNameAbsolute = CoreGlobals.getWorkPaths().mGameDirectory + "\\" + pfxFileName;

                        if (!File.Exists(pfxFileNameAbsolute))
                        {
                           ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tactic file \"{0}\" is referring to CausePhysicsExplosion particle effect file \"{1}\" which is not exist.\n", unit.mTacticsFileName, pfxFileName);
                        }
                        else
                        {
                           if (!m_freeFileList.Contains(pfxFileName))
                              m_freeFileList.Add(pfxFileName);
                        }
                     }
                  }

                  // read DOT effect
                  XmlNode dotEffectEffectNode = weaponNode.SelectSingleNode("./DOTEffect");
                  if (dotEffectEffectNode != null)
                  {
                     // read small
                     XmlNode smallNode = dotEffectEffectNode.Attributes.GetNamedItem("small");

                     if ((smallNode != null) && (smallNode.FirstChild != null))
                     {
                        string unitName = smallNode.FirstChild.Value;
                        long effectUnitID;

                        if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out effectUnitID))
                        {
                           buildUnit(effectUnitID);
                        }
                        else
                        {
                           ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tactic file \"{0}\" is referring to DOTEffect unit \"{1}\" which is not in objects.xml.\n", unit.mTacticsFileName, unitName);
                        }
                     }

                     // read medium
                     XmlNode mediumNode = dotEffectEffectNode.Attributes.GetNamedItem("medium");

                     if ((mediumNode != null) && (mediumNode.FirstChild != null))
                     {
                        string unitName = mediumNode.FirstChild.Value;
                        long effectUnitID;

                        if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out effectUnitID))
                        {
                           buildUnit(effectUnitID);
                        }
                        else
                        {
                           ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tactic file \"{0}\" is referring to DOTEffect unit \"{1}\" which is not in objects.xml.\n", unit.mTacticsFileName, unitName);
                        }
                     }

                     // read large
                     XmlNode largeNode = dotEffectEffectNode.Attributes.GetNamedItem("large");

                     if ((largeNode != null) && (largeNode.FirstChild != null))
                     {
                        string unitName = largeNode.FirstChild.Value;
                        long effectUnitID;

                        if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out effectUnitID))
                        {
                           buildUnit(effectUnitID);
                        }
                        else
                        {
                           ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tactic file \"{0}\" is referring to DOTEffect unit \"{1}\" which is not in objects.xml.\n", unit.mTacticsFileName, unitName);
                        }
                     }

                     // base 
                     if(dotEffectEffectNode.FirstChild != null)
                     {                        
                        string unitName = dotEffectEffectNode.FirstChild.Value;
                        long effectUnitID;

                        if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out effectUnitID))
                        {
                           buildUnit(effectUnitID);
                        }
                        else
                        {
                           ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tactic file \"{0}\" is referring to DOTEffect unit \"{1}\" which is not in objects.xml.\n", unit.mTacticsFileName, unitName);
                        }
                     }
                  }
               }

               XmlNodeList protoObjectNodes = tacticsDoc.SelectNodes("/TacticData/Action/ProtoObject");
               foreach (XmlNode protoObjectTypeNode in protoObjectNodes)
               {
                  if (protoObjectTypeNode.FirstChild != null)
                  {
                     // read name
                     string unitName = protoObjectTypeNode.FirstChild.Value;
                     long protoUnitID;

                     if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out protoUnitID))
                     {
                        buildUnit(protoUnitID);
                     }
                     else
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tactic file \"{0}\" is referring to ProtoObject unit \"{1}\" which is not in objects.xml.\n", unit.mTacticsFileName, unitName);
                     }
                  }
               }

               XmlNodeList squadTypeNodes = tacticsDoc.SelectNodes("/TacticData/Action/SquadType");
               foreach (XmlNode squadTypeTypeNode in squadTypeNodes)
               {
                  if (squadTypeTypeNode.FirstChild != null)
                  {
                     // read name
                     string squadName = squadTypeTypeNode.FirstChild.Value;
                     long protoSquadID;

                     if (Database.m_protoSquadTable.TryGetValue(squadName.ToLower(), out protoSquadID))
                     {
                        buildSquad(protoSquadID);
                     }
                     else
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tactic file \"{0}\" is referring to SquadType unit \"{1}\" which is not in squads.xml.\n", unit.mTacticsFileName, squadName);
                     }
                  }
               }
            }
         }

         // Add depedent deathSpawnSquad
         if (unit.mDeathSpawnSquad != -1)
         {
            buildSquad(unit.mDeathSpawnSquad);
         }

         // Add depedent units for BeamHead and BeamTail
         if (unit.mBeamHeadObject != -1)
         {
            buildUnit(unit.mBeamHeadObject);
         }

         if (unit.mBeamTailObject != -1)
         {
            buildUnit(unit.mBeamTailObject);
         }

         if (unit.mShieldTypeObject != -1)
         {
            buildUnit(unit.mShieldTypeObject);
         }

         if (unit.mAutoParkingLotObject != -1)
         {
            buildUnit(unit.mAutoParkingLotObject);
         }

         if (unit.mDeathReplacementObject != -1)
         {
            buildUnit(unit.mDeathReplacementObject);
         }

         if (unit.mTargetBeamObject != -1)
         {
            buildUnit(unit.mTargetBeamObject);
         }

         if (unit.mKillBeamObject != -1)
         {
            buildUnit(unit.mKillBeamObject);
         }

         if (unit.mLevelUpEffectObject != -1)
         {
            buildUnit(unit.mLevelUpEffectObject);
         }

         // Add children object
         foreach (long childUnitID in unit.mChildUnitIDs)
         {
            buildUnit(childUnitID);
         }


         // Activate train squads if they are available
         foreach (long trainSquadID in unit.mCommandTrainsSquadIDs)
         {
            if (m_squadAvailabitily[(int)trainSquadID] == false)
               continue;

            buildSquad(trainSquadID);
         }
         
         // Activate BuildOther if they are available
         foreach (long buildOtherUnitID in unit.mCommandBuildOtherUnitIDs)
         {
            if (m_unitAvailabitily[(int)buildOtherUnitID] == false)
               continue;

            buildUnit(buildOtherUnitID);
         }

         if (unit.mPower != -1)
         {
            applyEffectGodPower(unit.mPower);
         }
      }


      //==============================================================================
      // buildSquad
      //==============================================================================
      public void buildSquad(long squadID)
      {
         if ((squadID < 0) || (squadID >= m_squadBuilt.Count))
            return;

         if (m_squadBuilt[(int)squadID] == true)
            return;

         // Don't think I need this here.  Forbids should only affect what can be trained
         //if (m_squadForbid[(int)squadID] == true)
         //   return;

         m_squadBuilt[(int)squadID] = true;


         // Also make available all the units the squad depends on.
         //
         ProtoSquad squad = Database.m_protoSquads[(int)squadID];
         foreach (long unitID in squad.mUnitIDs)
         {
            buildUnit(unitID);
            //applyProtoObjectEffect(unitID);
         }

         if (squad.mRecoveringEffectID != -1)
         {
            buildUnit(squad.mRecoveringEffectID);
            //applyProtoObjectEffect(squad.mRecoveringEffectID);
         }
      }


      //==============================================================================
      // buildInfectedUnits
      //
      // This function gets called at the end, once all "based" units have been unlocked.  It goes
      // through every unit and builds it corresponing infected unit, if it has one.
      //==============================================================================
      public void buildInfectedUnits()
      {
         int unitCount = m_unitBuilt.Count;
         for (int k = 0; k < unitCount; k++)
         {
            if (m_unitBuilt[k] == true)
            {
               // Check to see if this unit has an infected counterpart
               long infectedUnitID;
               if (Database.m_infectionMap.TryGetValue(Database.m_protoUnits[k].mName.ToLower(), out infectedUnitID))
               {
                  buildUnit(infectedUnitID);
               }
            }
         }
      }

      //==============================================================================
      // buildCollectorsEditionUnits
      //
      // Check to see if the warthog or wraith are enabled, if so the also include
      // their corresponding collectors edition versions.
      //==============================================================================
      public void buildCollectorsEditionUnits()
      {
         long unitID = -1;
         long lceUnitID = -1;

         if(Database.m_objectTypes.TryGetValue("unsc_veh_warthog_01", out unitID) &&
            Database.m_objectTypes.TryGetValue("unsc_veh_warthoglce_01", out lceUnitID))
         {
            if (m_unitBuilt[(int)unitID] == true)
            {
               buildUnit(lceUnitID);
            }
         }

         if (Database.m_objectTypes.TryGetValue("cov_veh_wraith_01", out unitID) &&
            Database.m_objectTypes.TryGetValue("cov_veh_wraithlce_01", out lceUnitID))
         {
            if (m_unitBuilt[(int)unitID] == true)
            {
               buildUnit(lceUnitID);
            }
         }
      }

      //==============================================================================
      // buildGruntPartySkullUnits
      //
      // Check to see if grunts are enabled, if so the also include their corresponding 
      // skull assets.
      //==============================================================================
      public void buildGruntPartySkullUnits()
      {
         long unitID = -1;
         long lceUnitID = -1;

         if (Database.m_objectTypes.TryGetValue("cov_inf_grunt_01", out unitID) &&
            Database.m_objectTypes.TryGetValue("fx_obj_gruntconfetti", out lceUnitID))
         {
            if (m_unitBuilt[(int)unitID] == true)
            {
               buildUnit(lceUnitID);
            }
         }
      }

      //==============================================================================
      // buildWuvWooSkullUnits
      //
      // Check to see if the scarabs are enabled, if so the also include
      // their corresponding skull assets.
      //==============================================================================
      public void buildWuvWooSkullUnits()
      {
         long unitID1 = -1;
         long unitID2 = -1;
         long wuvwooUnitID = -1;

         if (Database.m_objectTypes.TryGetValue("cov_veh_scarab_01", out unitID1) &&
             Database.m_objectTypes.TryGetValue("cpgn_scn13_scarab_01", out unitID2) &&
             Database.m_objectTypes.TryGetValue("fx_proj_wuvwoobeam_01", out wuvwooUnitID))
         {
            if ((m_unitBuilt[(int)unitID1] == true) ||
                (m_unitBuilt[(int)unitID2] == true))
            {
               buildUnit(wuvwooUnitID);
            }
         }
      }

      //==============================================================================
      // buildMissingProjectileHack
      //
      // Resolve missing projectile changes from wraith and cobra.  This is a hardcoded it
      // change since it's too big of a change to do the real fix at this point.
      //==============================================================================
      public void buildMissingProjectileHack()
      {
         // ***************************************************************************
         // HACK BEGINS
         long hackUnitID = -1;
         long projectileUnitID1 = -1;
         long projectileUnitID2 = -1;

         if (Database.m_objectTypes.TryGetValue("cov_veh_wraith_01", out hackUnitID) &&
            Database.m_objectTypes.TryGetValue("fx_proj_plasmamortar_02", out projectileUnitID1))
         {
            if (m_unitBuilt[(int)hackUnitID] == true)
            {
               buildUnit(projectileUnitID1);
            }
         }

         if (Database.m_objectTypes.TryGetValue("unsc_veh_cobra_01", out hackUnitID) &&
            Database.m_objectTypes.TryGetValue("fx_proj_lightrailgun_02", out projectileUnitID1) &&
            Database.m_objectTypes.TryGetValue("fx_proj_railgun_02", out projectileUnitID2))
         {
            if (m_unitBuilt[(int)hackUnitID] == true)
            {
               buildUnit(projectileUnitID1);
               buildUnit(projectileUnitID2);
            }
         }
         // HACK ENDS
         // ***************************************************************************
      }

      //==============================================================================
      // buildExtraGameDataUnits
      //==============================================================================
      public void buildExtraGameDataUnits()
      {
         long unitID = -1;

         if (Database.m_objectTypes.TryGetValue("fx_stuneffect", out unitID))
            buildUnit(unitID);
         if (Database.m_objectTypes.TryGetValue("fx_empeffect", out unitID))
            buildUnit(unitID);
      }      


      //==============================================================================
      // buildForerunnerMonitorShieldUnits
      //==============================================================================
      public void buildForerunnerMonitorShieldUnits()
      {
         long squadID = -1;

         if (Database.m_protoSquadTable.TryGetValue("for_air_monitor_04", out squadID))
         {
            if (m_squadBuilt[(int)squadID] == true)
            {
               long shieldSquadID = -1;

               if (Database.m_protoSquadTable.TryGetValue("sys_bubbleshield_small_01", out shieldSquadID))
                  buildSquad(shieldSquadID);
               if (Database.m_protoSquadTable.TryGetValue("sys_bubbleshield_med_01", out shieldSquadID))
                  buildSquad(shieldSquadID);
               if (Database.m_protoSquadTable.TryGetValue("sys_bubbleshield_large_01", out shieldSquadID))
                  buildSquad(shieldSquadID);
            }
         }
      }

      //==============================================================================
      // applyEffectGodPower
      //==============================================================================
      public void applyEffectGodPower(long powerID)
      {
         if ((powerID < 0) || (powerID >= m_powerActive.Count))
            return;

         if (m_powerActive[(int)powerID] == true)
            return;

         m_powerActive[(int)powerID] = true;


         Power power = Database.m_powers[(int)powerID];

         if (power.mTriggerScriptFile != null)
         {
            string triggerScriptFileName = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\triggerscripts\\" + power.mTriggerScriptFile;

            // check if file exists
            if (!File.Exists(triggerScriptFileName))
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Trigger script file \"{0}\" specified in \"{1}\" GodPower does not exist.\n", triggerScriptFileName, power.mName);
               return;
            }

            // Read god power file
            XmlDocument powerTriggerScripDoc = new XmlDocument();
            CoreGlobals.XmlDocumentLoadHelper(powerTriggerScripDoc, triggerScriptFileName);

            XmlNodeList triggerSystemNodes = powerTriggerScripDoc.SelectNodes("/TriggerSystem");
            foreach (XmlNode triggerSystemNode in triggerSystemNodes)
            {
               activateTriggerSystemNode(triggerSystemNode, triggerScriptFileName);
            }
         }

         // Add datalevel object
         foreach (long dataLevelUnitID in power.mDataLevelUnitIDs)
         {
            buildUnit(dataLevelUnitID);
         }

         // Add datalevel squads
         foreach (long dataLevelSquadID in power.mDataLevelSquadIDs)
         {
            buildSquad(dataLevelSquadID);
         }

         // Add datalevel techs
         foreach (long dataLevelTechID in power.mDataLevelTechIDs)
         {
            activateTech(dataLevelTechID);
         }
      }


      //==============================================================================
      // activateTriggerSystemNode
      //==============================================================================
      public void activateTriggerSystemNode(XmlNode triggerSystemNode, string filename)
      {
         XmlNodeList nodes;


         // Look for triggervars with proto objects 'ProtoObject'
         nodes = triggerSystemNode.SelectNodes("./TriggerVars/TriggerVar[@Type='ProtoObject']");

         foreach (XmlNode node in nodes)
         {
            // disregard empty variables
            if (node.FirstChild == null)
               continue;

            string unitName = node.FirstChild.Value;
            long unitID;

            if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out unitID))
            {
               buildUnit(unitID);
               //applyProtoObjectEffect(unitID);
            }
            else
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" referred in for TriggerVar (type = \"ProtoObject\") in file \"{1}\" does not exist.\n", unitName, filename);
               continue;
            }
         }


         // Look for triggervars with proto object lists 'ProtoObjectList'
         nodes = triggerSystemNode.SelectNodes("./TriggerVars/TriggerVar[@Type='ProtoObjectList']");

         foreach (XmlNode node in nodes)
         {
            // disregard empty variables
            if (node.FirstChild == null)
               continue;

            string unitNameList = node.FirstChild.Value;

            while (!string.IsNullOrEmpty(unitNameList))
            {
               string unitName;

               int posOfComma = unitNameList.IndexOf(',');
               if (posOfComma == -1)
               {
                  unitName = unitNameList.TrimEnd(null);
                  unitNameList = null;
               }
               else
               {
                  unitName = unitNameList.Remove(posOfComma).TrimEnd(null);
                  unitNameList = unitNameList.Remove(0, posOfComma);
                  unitNameList = unitNameList.TrimStart(',');
               }

               long unitID;
               if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out unitID))
               {
                  buildUnit(unitID);
                  //applyProtoObjectEffect(unitID);
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" referred in for TriggerVar (type = \"ProtoObjectList\") in file \"{1}\" does not exist.\n", unitName, filename);
                  continue;
               }
            }
         }


         // Look for triggervars with squads 'ProtoSquad'
         nodes = triggerSystemNode.SelectNodes("./TriggerVars/TriggerVar[@Type='ProtoSquad']");


         List<string> squadNames = new List<string>();
         foreach (XmlNode node in nodes)
         {
            // disregard empty variables
            if (node.FirstChild == null)
               continue;

            string squadName = node.FirstChild.Value;
            long squadID;

            if (Database.m_protoSquadTable.TryGetValue(squadName.ToLower(), out squadID))
            {
               buildSquad(squadID);
               //applyProtoSquadEffect(squadID);
            }
            else
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Squad \"{0}\" referred in for TriggerVar (type = \"ProtoSquad\") in file \"{1}\" does not exist.\n", squadName, filename);
               continue;
            }
         }


         // Look for triggervars with squad lists 'ProtoSquadList'
         nodes = triggerSystemNode.SelectNodes("./TriggerVars/TriggerVar[@Type='ProtoSquadList']");

         foreach (XmlNode node in nodes)
         {
            // disregard empty variables
            if (node.FirstChild == null)
               continue;

            string squadNameList = node.FirstChild.Value;

            while (!string.IsNullOrEmpty(squadNameList))
            {
               string squadName;

               int posOfComma = squadNameList.IndexOf(',');
               if (posOfComma == -1)
               {
                  squadName = squadNameList.TrimEnd(null);
                  squadNameList = null;
               }
               else
               {
                  squadName = squadNameList.Remove(posOfComma).TrimEnd(null);
                  squadNameList = squadNameList.Remove(0, posOfComma);
                  squadNameList = squadNameList.TrimStart(',');
               }

               long squadID;

               if (Database.m_protoSquadTable.TryGetValue(squadName.ToLower(), out squadID))
               {
                  buildSquad(squadID);
                  //applyProtoSquadEffect(squadID);
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Squad \"{0}\" referred in for TriggerVar (type = \"ProtoSquad\") in file \"{1}\" does not exist.\n", squadName, filename);
                  continue;
               }
            }
         }


         // Look for triggervars with strings that have extenstions .triggerscript
         nodes = triggerSystemNode.SelectNodes("./TriggerVars/TriggerVar[@Type='String']");

         foreach (XmlNode node in nodes)
         {
            // disregard empty variables
            if (node.FirstChild == null)
               continue;

            string stringName = node.FirstChild.Value;

            // Check if the string that the .triggerscript extension
            if (!stringName.EndsWith(".triggerscript", true, System.Globalization.CultureInfo.CurrentCulture))
               continue;

            string exposedScriptFile = stringName;

            if (exposedScriptFile != null)
            {
               string exposedScriptFileName = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\triggerscripts\\" + exposedScriptFile;

               // check if file exists
               if (!File.Exists(exposedScriptFileName))
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "ExposedScript trigger script file \"{0}\" specified in \"{1}\" script does not exist.\n", exposedScriptFileName, filename);
                  continue;
               }

               // Read god power file
               XmlDocument triggerScripDoc = new XmlDocument();
               CoreGlobals.XmlDocumentLoadHelper(triggerScripDoc, exposedScriptFileName);

               XmlNodeList triggerSystemNodes = triggerScripDoc.SelectNodes("/TriggerSystem");
               foreach (XmlNode exposedTriggerSystemNode in triggerSystemNodes)
               {
                  activateTriggerSystemNode(exposedTriggerSystemNode, exposedScriptFileName);
               }
            }
         }






         // Look for triggervars with tech
         nodes = triggerSystemNode.SelectNodes("./TriggerVars/TriggerVar[@Type='Tech']");

         foreach (XmlNode node in nodes)
         {
            // disregard empty variables
            if (node.FirstChild == null)
               continue;

            string techName = node.FirstChild.Value;
            long techID;

            if (Database.m_tableTech.TryGetValue(techName.ToLower(), out techID))
            {
               activateTech(techID);
            }
            else
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" referred in for TriggerVar (type = \"Tech\") in file \"{1}\" does not exist.\n", techName, filename);
               continue;
            }
         }


         // Look for triggervars with Power
         nodes = triggerSystemNode.SelectNodes("./TriggerVars/TriggerVar[@Type='Power']");

         foreach (XmlNode node in nodes)
         {
            // disregard empty variables
            if (node.FirstChild == null)
               continue;

            string powerName = node.FirstChild.Value;
            long powerID;

            if (Database.m_tablePowers.TryGetValue(powerName.ToLower(), out powerID))
            {
               applyEffectGodPower(powerID);
            }
            else
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Power \"{0}\" referred in for TriggerVar (type = \"Power\") in file \"{1}\" does not exist.\n", powerName, filename);
               continue;
            }
         }
      }
   }
}
