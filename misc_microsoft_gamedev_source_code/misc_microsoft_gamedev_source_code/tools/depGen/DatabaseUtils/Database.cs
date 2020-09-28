using System;
using System.Collections.Generic;
using System.Text;

using System.IO;
using System.Xml;
using System.Xml.Serialization;

using ConsoleUtils;


namespace DatabaseUtils
{

   public class ProtoUnit
   {
      public string mName = null;
      public string mVisFile = null;
      public long mAbilityCommand = -1;
      public string mTriggerScriptFileName = null;
      public string mTacticsFileName = null;
      public string mImpactDecalRootName = null;

      public long mDeathSpawnSquad = -1;
      public long mBeamHeadObject = -1;
      public long mBeamTailObject = -1;
      public long mShieldTypeObject = -1;
      public long mAutoParkingLotObject = -1;
      public long mDeathReplacementObject = -1;
      public long mTargetBeamObject = -1;
      public long mKillBeamObject = -1;
      public long mLevelUpEffectObject = -1;
      public List<long> mCommandResearchTechIDs = new List<long>();
      public List<long> mCommandTrainsSquadIDs = new List<long>();
      public List<long> mCommandBuildOtherUnitIDs = new List<long>();
      public List<long> mChildUnitIDs = new List<long>();

      public long mPower = -1;

      public string mPhysicsInfoFileName = null;
   }

   public class ProtoSquad
   {
      public string mName = null;
      public List<long> mUnitIDs = new List<long>();
      public long mRecoveringEffectID = -1;
   }

   public class Civ
   {
      public string mName = null;
      public long mTech = -1;
      public long mCommandAckUnit = -1;
      public long mRallyPointUnit = -1;
      public long mLocalRallyPointUnit = -1;
      public long mTransportUnit = -1;
   }

   public class Leader
   {
      public string mName = null;
      public long mCiv = -1;
      public long mTech = -1;
      public long mStartingUnit = -1;
      public long mStartingSquad = -1;
   }

   public class Power
   {
      public string mName = null;
      public string mTriggerScriptFile = null;
      public List<long> mDataLevelUnitIDs = new List<long>();
      public List<long> mDataLevelSquadIDs = new List<long>();
      public List<long> mDataLevelTechIDs = new List<long>();
   }

   public class Ability
   {
      public string mName = null;
      public string mTriggerScriptFile = null;
   }

   public class GameMode
   {
      public string mName = null;
      public string mWorldTriggerScriptFile = null;
      public string mPlayerTriggerScriptFile = null;
   }


   static public class Database
   {
      static public Dictionary<string, long> m_objectTypes = new Dictionary<string, long>();       // Unit name -> unit ID
      static public Dictionary<string, long> m_protoSquadTable = new Dictionary<string, long>();   // Squad name -> squad ID
      static public Dictionary<string, long> m_tableTech = new Dictionary<string, long>();         // Tech name -> tech ID
      static public Dictionary<string, long> m_tablePowers = new Dictionary<string, long>();       // Power name -> power ID
      static public Dictionary<string, long> m_tableCivs = new Dictionary<string, long>();         // Civ name -> civ ID
      static public Dictionary<string, long> m_tableLeaders = new Dictionary<string, long>();      // Leader name -> leader ID
      static public Dictionary<string, long> m_tableAbilities = new Dictionary<string, long>();    // Ability name -> ability ID
      static public Dictionary<string, long> m_tableGameModes = new Dictionary<string, long>();    // Ability name -> ability ID
      static public Dictionary<string, long> m_infectionMap = new Dictionary<string, long>();        // Unit name -> Infected Unit ID

      static public Dictionary<string, string> m_impactEffectToTfxName = new Dictionary<string, string>();     // impact effect name -> vis file name

      static public List<ProtoUnit>    m_protoUnits = new List<ProtoUnit>();
      static public List<ProtoSquad>   m_protoSquads = new List<ProtoSquad>();
      static public List<ProtoTech>    m_protoTechs = new List<ProtoTech>();
      static public List<Civ>          m_civs = new List<Civ>();
      static public List<Leader>       m_leaders = new List<Leader>();
      static public List<Power>        m_powers = new List<Power>();
      static public List<Ability>      m_abilities = new List<Ability>();
      static public List<GameMode>     m_gameModes = new List<GameMode>();

      static private bool bInitialized = false;


      //==============================================================================
      // init
      //==============================================================================
      static public void init()
      {
         if (bInitialized)
            return;


         // read database
         readDatabaseFiles();

         bInitialized = true;
      }


      //==============================================================================
      // isInitialized
      //==============================================================================
      static public bool isInitialized()
      {
         return (bInitialized);
      }


      //==============================================================================
      // readDatabaseFiles
      //==============================================================================
      static private void readDatabaseFiles()
      {
         ConsoleOut.Write(ConsoleOut.MsgType.Info, "Reading database files...\n");

         XmlDocument objectsDoc = null;
         XmlDocument squadsDoc = null;
         XmlDocument techsDoc = null;
         XmlDocument civsDoc = null;
         XmlDocument leadersDoc = null;
         XmlDocument powersDoc = null;
         XmlDocument abilitiesDoc = null;
         XmlDocument gameModesDoc = null;
         XmlDocument gameDataDoc = null;
         XmlDocument impactEffectsDoc = null;


         // Read database files
         //
         objectsDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(objectsDoc, CoreGlobals.getWorkPaths().mGameDataDirectory + @"\objects.xml");

         squadsDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(squadsDoc, CoreGlobals.getWorkPaths().mGameDataDirectory + @"\squads.xml");

         techsDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(techsDoc, CoreGlobals.getWorkPaths().mGameDataDirectory + @"\techs.xml");

         civsDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(civsDoc, CoreGlobals.getWorkPaths().mGameDataDirectory + @"\civs.xml");

         leadersDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(leadersDoc, CoreGlobals.getWorkPaths().mGameDataDirectory + @"\leaders.xml");

         powersDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(powersDoc, CoreGlobals.getWorkPaths().mGameDataDirectory + @"\powers.xml");

         abilitiesDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(abilitiesDoc, CoreGlobals.getWorkPaths().mGameDataDirectory + @"\abilities.xml");

         gameModesDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(gameModesDoc, CoreGlobals.getWorkPaths().mGameDataDirectory + @"\gamemodes.xml");

         gameDataDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(gameDataDoc, CoreGlobals.getWorkPaths().mGameDataDirectory + @"\gamedata.xml");
         
         impactEffectsDoc = new XmlDocument();
         CoreGlobals.XmlDocumentLoadHelper(impactEffectsDoc, CoreGlobals.getWorkPaths().mGameDataDirectory + @"\impacteffects.xml");


         // Build look up tables
         //
         buildNameToIdTables(objectsDoc, squadsDoc, techsDoc, civsDoc, leadersDoc, powersDoc, abilitiesDoc, gameModesDoc, impactEffectsDoc);

         // Build look up tables
         //
         readDatabase(objectsDoc, squadsDoc, techsDoc, civsDoc, leadersDoc, powersDoc, abilitiesDoc, gameModesDoc, gameDataDoc);
      }


      //==============================================================================
      // buildNameToIdTables
      //==============================================================================
      static private void buildNameToIdTables(XmlDocument objectsDoc, XmlDocument squadsDoc, XmlDocument techsDoc, 
                                                XmlDocument civsDoc, XmlDocument leadersDoc, XmlDocument powersDoc,
                                                XmlDocument abilitiesDoc, XmlDocument gameModesDoc, 
                                                XmlDocument impactEffectsDoc)
      {
         // Build unit name table
         //
         XmlNodeList objectNodes = objectsDoc.SelectNodes("/Objects/Object");

         int i = 0;
         foreach (XmlNode objectNode in objectNodes)
         {
            // read name
            XmlNode nameNode = objectNode.Attributes.GetNamedItem("name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string unitName = nameNode.FirstChild.Value;

            // Add to map table
            m_objectTypes.Add(unitName.ToLower(), i++);
         }


         // Build squad name table
         //
         XmlNodeList squadNodes = squadsDoc.SelectNodes("/Squads/Squad");

         i = 0;
         foreach (XmlNode squadNode in squadNodes)
         {
            // read name
            XmlNode nameNode = squadNode.Attributes.GetNamedItem("name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string squadName = nameNode.FirstChild.Value;

            // Add to map table
            m_protoSquadTable.Add(squadName.ToLower(), i++);
         }


         // Build tech names table
         //
         XmlNodeList techNodes = techsDoc.SelectNodes("//Tech");

         i = 0;
         foreach(XmlNode techNode in techNodes)
         {
            // read name
            XmlNode nameNode = techNode.Attributes.GetNamedItem("name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string techName = nameNode.FirstChild.Value;

            // Add to map table
            m_tableTech.Add(techName.ToLower(), i++);
         }


         // Build godpower names table
         //
         XmlNodeList powerNodes = powersDoc.SelectNodes("/Powers/Power");

         i = 0;
         foreach (XmlNode powerNode in powerNodes)
         {
            // read name
            XmlNode nameNode = powerNode.Attributes.GetNamedItem("name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string powerName = nameNode.FirstChild.Value;

            // Add to map table
            try
            {
               m_tablePowers.Add(powerName.ToLower(), i++);
            }
            catch(System.ArgumentException)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Power {0} is defined more than once!\n", powerName);
            }
         }


         // Build civ names table
         //
         XmlNodeList civNodes = civsDoc.SelectNodes("/Civs/Civ");

         i = 0;
         foreach (XmlNode civNode in civNodes)
         {
            // read name
            XmlNode nameNode = civNode.SelectSingleNode("Name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string civName = nameNode.FirstChild.Value;

            // Add to map table
            m_tableCivs.Add(civName.ToLower(), i++);
         }


         // Build leader names table
         //
         XmlNodeList leaderNodes = leadersDoc.SelectNodes("/Leaders/Leader");

         i = 0;
         foreach (XmlNode leaderNode in leaderNodes)
         {
            // read name
            XmlNode nameNode = leaderNode.Attributes.GetNamedItem("Name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string leaderName = nameNode.FirstChild.Value;

            // Add to map table
            m_tableLeaders.Add(leaderName.ToLower(), i++);
         }



         // Build ability names table
         //
         XmlNodeList abilityNodes = abilitiesDoc.SelectNodes("/Abilities/Ability");

         i = 0;
         foreach (XmlNode abilityNode in abilityNodes)
         {
            // read name
            XmlNode nameNode = abilityNode.Attributes.GetNamedItem("Name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string abilityName = nameNode.FirstChild.Value;

            // Add to map table
            m_tableAbilities.Add(abilityName.ToLower(), i++);
         }


         // Build gamemode names table
         //
         XmlNodeList gameModeNodes = gameModesDoc.SelectNodes("/GameModes/GameMode");

         i = 0;
         foreach (XmlNode gameModeNode in gameModeNodes)
         {
            // read name
            XmlNode nameNode = gameModeNode.SelectSingleNode("Name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string gameModeName = nameNode.FirstChild.Value;

            // Add to map table
            m_tableGameModes.Add(gameModeName.ToLower(), i++);
         }


         // Impact Effects
         XmlNodeList impactEffectNodes = impactEffectsDoc.SelectNodes("/ImpactEffects/TerrainEffect");
         foreach (XmlNode impactEffectNode in impactEffectNodes)
         {
            // read name
            XmlNode nameNode = impactEffectNode.Attributes.GetNamedItem("name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string effectName = nameNode.FirstChild.Value;
            string tfxFileName = null;

            // read file name
            if (impactEffectNode.FirstChild != null)
            {
               tfxFileName = impactEffectNode.FirstChild.Value;

               // Validate that the vis file exists
               if (tfxFileName != null)
               {
                  tfxFileName = String.Concat("art\\", tfxFileName, ".tfx");
                  string tfxFileNameAbsolute = CoreGlobals.getWorkPaths().mGameDirectory + "\\" + tfxFileName;

                  // check if file exists
                  if (!File.Exists(tfxFileNameAbsolute))
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "ImpactEffect \"{0}\" in impacteffects.xml is referring to terraineffect file \"{1}\" which does not exist.\n", effectName, tfxFileName);
                  }

                  m_impactEffectToTfxName.Add(effectName.ToLower(), tfxFileName);
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "ImpactEffect found in impacteffects.xml which does not have a name.\n");
               }
            }
            else
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "ImpactEffect \"{0}\" in impacteffects.xml does not have a terraineffect file name\n", effectName);
            }
         }
      }


      //==============================================================================
      // buildNameToIdTables
      //==============================================================================
      static private void readDatabase(XmlDocument objectsDoc, XmlDocument squadsDoc, XmlDocument techsDoc,
                                       XmlDocument civsDoc, XmlDocument leadersDoc, XmlDocument powersDoc,
                                       XmlDocument abilitiesDoc, XmlDocument gameModesDoc, XmlDocument gameDataDoc)
      {

         // Read objects
         //
         String delim = "\\";
         XmlNodeList objectNodes = objectsDoc.SelectNodes("/Objects/Object");
         foreach (XmlNode objectNode in objectNodes)
         {
            // read name
            XmlNode nameNode = objectNode.Attributes.GetNamedItem("name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string unitName = nameNode.FirstChild.Value;
            string visFileName = null;
            string abilityTriggerFileName = null;
            string abilityCommandName = null;
            string tacticsName = null;
            string impactDecalName = null;
            string deathSpawnSquadName = null;
            string beamHeadName = null;
            string beamTailName = null;
            string shieldTypeName = null;
            string autoParkingLotName = null;
            string deathReplacementName = null;
            string targetBeamName = null;
            string killBeamName = null;
            string levelUpEffectName = null;
            string powerName = null;
            string physicsInfoFileName = null;


            // read vis file (if exists)
            XmlNode visFileNode = objectNode.SelectSingleNode("./Visual");
            if ((visFileNode != null) && (visFileNode.FirstChild != null))
               visFileName = visFileNode.FirstChild.Value.TrimStart(delim.ToCharArray());

            // Add extension if missing
            if (!String.IsNullOrEmpty(visFileName) && String.IsNullOrEmpty(Path.GetExtension(visFileName)))
               visFileName = String.Concat(visFileName, ".vis");

            // read ability (if exists)
            XmlNode abilityNode = objectNode.SelectSingleNode("./Ability");
            if ((abilityNode != null) && (abilityNode.FirstChild != null))
               abilityTriggerFileName = abilityNode.FirstChild.Value;

            // read ability command (if exists)
            XmlNode abilityCommandNode = objectNode.SelectSingleNode("./AbilityCommand");
            if ((abilityCommandNode != null) && (abilityCommandNode.FirstChild != null))
               abilityCommandName = abilityCommandNode.FirstChild.Value;

            // read tactics (if exists)
            XmlNode tacticsNode = objectNode.SelectSingleNode("./Tactics");
            if ((tacticsNode != null) && (tacticsNode.FirstChild != null))
               tacticsName = tacticsNode.FirstChild.Value;

            // read impact decal (if exists)
            XmlNode impactDecalNode = objectNode.SelectSingleNode("./ImpactDecal");
            if ((impactDecalNode != null) && (impactDecalNode.FirstChild != null))
               impactDecalName = impactDecalNode.FirstChild.Value;

            // read beam head (if exists)
            XmlNode deathSpawnSquadNode = objectNode.SelectSingleNode("./DeathSpawnSquad");
            if ((deathSpawnSquadNode != null) && (deathSpawnSquadNode.FirstChild != null))
               deathSpawnSquadName = deathSpawnSquadNode.FirstChild.Value;

            // read beam head (if exists)
            XmlNode beamHeadNode = objectNode.SelectSingleNode("./BeamHead");
            if ((beamHeadNode != null) && (beamHeadNode.FirstChild != null))
               beamHeadName = beamHeadNode.FirstChild.Value;

            // read beam tail (if exists)
            XmlNode beamTailNode = objectNode.SelectSingleNode("./BeamTail");
            if ((beamTailNode != null) && (beamTailNode.FirstChild != null))
               beamTailName = beamTailNode.FirstChild.Value;

            // read shield type (if exists)
            XmlNode shieldTypeNode = objectNode.SelectSingleNode("./ShieldType");
            if ((shieldTypeNode != null) && (shieldTypeNode.FirstChild != null))
               shieldTypeName = shieldTypeNode.FirstChild.Value;

            // read autoParkingLot (if exists)
            XmlNode autoParkingLotNode = objectNode.SelectSingleNode("./AutoParkingLot");
            if ((autoParkingLotNode != null) && (autoParkingLotNode.FirstChild != null))
               autoParkingLotName = autoParkingLotNode.FirstChild.Value;

            // read DeathReplacement (if exists)
            XmlNode deathReplacementNode = objectNode.SelectSingleNode("./DeathReplacement");
            if ((deathReplacementNode != null) && (deathReplacementNode.FirstChild != null))
               deathReplacementName = deathReplacementNode.FirstChild.Value;

            // read TargetBeam (if exists)
            XmlNode targetBeamNode = objectNode.SelectSingleNode("./TargetBeam");
            if ((targetBeamNode != null) && (targetBeamNode.FirstChild != null))
               targetBeamName = targetBeamNode.FirstChild.Value;

            // read KillBeam (if exists)
            XmlNode killBeamNode = objectNode.SelectSingleNode("./KillBeam");
            if ((killBeamNode != null) && (killBeamNode.FirstChild != null))
               killBeamName = killBeamNode.FirstChild.Value;

            // read LevelUpEffect (if exists)
            XmlNode levelUpEffectNode = objectNode.SelectSingleNode("./LevelUpEffect");
            if ((levelUpEffectNode != null) && (levelUpEffectNode.FirstChild != null))
               levelUpEffectName = levelUpEffectNode.FirstChild.Value;

            // read power name (if exists)
            XmlNode powerNode = objectNode.SelectSingleNode("./Power");
            if ((powerNode != null) && (powerNode.FirstChild != null))
               powerName = powerNode.FirstChild.Value;

            // read physicsinfo file (if exists)
            XmlNode physicsInfoNode = objectNode.SelectSingleNode("./PhysicsInfo");
            if ((physicsInfoNode != null) && (physicsInfoNode.FirstChild != null))
               physicsInfoFileName = physicsInfoNode.FirstChild.Value;


            // Add to list
            ProtoUnit unit = new ProtoUnit();


            unit.mName = unitName;
            unit.mVisFile = (visFileName != null) ? "art\\" + visFileName : null;
            unit.mTriggerScriptFileName = abilityTriggerFileName;
            unit.mPhysicsInfoFileName = (physicsInfoFileName != null) ? "physics\\" + physicsInfoFileName + ".physics" : null;


            // Validate that the vis file exists
            if (unit.mVisFile != null)
            {
               string visFileNameAbsolute = CoreGlobals.getWorkPaths().mGameDirectory + "\\" + unit.mVisFile;

               // check if file exists
               if (!File.Exists(visFileNameAbsolute))
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" uses Visual file \"{1}\" which does not exist.\n", unit.mName, unit.mVisFile);
               }
            }

            if (abilityCommandName != null)
            {
               long abilityCommandID;
               if (m_tableAbilities.TryGetValue(abilityCommandName.ToLower(), out abilityCommandID))
               {
                  unit.mAbilityCommand = abilityCommandID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to AbilityCommand \"{1}\" which does not exist in abilities.xml.\n", unit.mName, abilityCommandName);
               }
            }

            if (tacticsName != null)
            {
               string tacticsFileNameAbsolute = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\tactics\\" + tacticsName;

               // check if file exists
               if (File.Exists(tacticsFileNameAbsolute))
               {
                  unit.mTacticsFileName = tacticsName;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to tactics file \"{1}\" which does not exist.\n", unit.mName, tacticsFileNameAbsolute);
               }
            }

            if (impactDecalName != null)
            {
               unit.mImpactDecalRootName = "art\\" + impactDecalName;

               {
                  string[] impactDecalFileName = new string[3];

                  impactDecalFileName[0] = String.Concat(unit.mImpactDecalRootName, "_df.ddx");
                  impactDecalFileName[1] = String.Concat(unit.mImpactDecalRootName, "_nm.ddx");
                  impactDecalFileName[2] = String.Concat(unit.mImpactDecalRootName, "_op.ddx");

                  long textureChannelFoundCount = 0;

                  for (long channel = 0; channel < 3; channel++)
                  {
                     // Check file existance
                     bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + impactDecalFileName[channel]);

                     if (fileExists)
                     {
                        textureChannelFoundCount += 1;
                     }
                  }

                  if (textureChannelFoundCount == 0)
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to impactdecal \"{1}\" which does not exist.\n", unit.mName, unit.mImpactDecalRootName);
                  }
               }
            }


            if (deathSpawnSquadName != null)
            {
               long squadID;
               if (m_protoSquadTable.TryGetValue(deathSpawnSquadName.ToLower(), out squadID))
               {
                  unit.mDeathSpawnSquad = squadID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to DeathSpawnSquad squad \"{1}\" which does not exist in squads.xml.\n", unit.mName, deathSpawnSquadName);
               }
            }

            if (beamHeadName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(beamHeadName.ToLower(), out unitID))
               {
                  unit.mBeamHeadObject = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to BeamHead unit \"{1}\" which does not exist in objects.xml.\n", unit.mName, beamHeadName);
               }
            }

            if (beamTailName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(beamTailName.ToLower(), out unitID))
               {
                  unit.mBeamTailObject = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to BeamTail unit \"{1}\" which does not exist in objects.xml.\n", unit.mName, beamTailName);
               }
            }

            if (shieldTypeName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(shieldTypeName.ToLower(), out unitID))
               {
                  unit.mShieldTypeObject = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to ShieldType unit \"{1}\" which does not exist in objects.xml.\n", unit.mName, shieldTypeName);
               }
            }

            if (autoParkingLotName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(autoParkingLotName.ToLower(), out unitID))
               {
                  unit.mAutoParkingLotObject = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to AutoParkingLot unit \"{1}\" which does not exist in objects.xml.\n", unit.mName, autoParkingLotName);
               }
            }

            if (deathReplacementName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(deathReplacementName.ToLower(), out unitID))
               {
                  unit.mDeathReplacementObject = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to DeathReplacement unit \"{1}\" which does not exist in objects.xml.\n", unit.mName, deathReplacementName);
               }
            }

            if (targetBeamName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(targetBeamName.ToLower(), out unitID))
               {
                  unit.mTargetBeamObject = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to TargetBeam unit \"{1}\" which does not exist in objects.xml.\n", unit.mName, targetBeamName);
               }
            }

            if (killBeamName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(killBeamName.ToLower(), out unitID))
               {
                  unit.mKillBeamObject = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to KillBeam unit \"{1}\" which does not exist in objects.xml.\n", unit.mName, killBeamName);
               }
            }

            if (levelUpEffectName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(levelUpEffectName.ToLower(), out unitID))
               {
                  unit.mLevelUpEffectObject = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to LevelUpEffect unit \"{1}\" which does not exist in objects.xml.\n", unit.mName, levelUpEffectName);
               }
            }


            // look for research techs
            XmlNodeList researchTechNodes = objectNode.SelectNodes("./Command[@Type='Research']");
            foreach (XmlNode researchTechNode in researchTechNodes)
            {
               if (researchTechNode.FirstChild == null)
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Command for type 'Research' in unit \"{0}\" is empty.\n", unitName);
                  continue;
               }

               long techID;
               if (m_tableTech.TryGetValue(researchTechNode.FirstChild.Value.ToLower(), out techID))
               {
                  unit.mCommandResearchTechIDs.Add(techID);
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" has a 'Research' command that refers to Tech \"{1}\" which does not exist in techs.xml.\n", unitName, researchTechNode.FirstChild.Value);
               }
            }

            // look for BuildOther units
            XmlNodeList buildOtherNodes = objectNode.SelectNodes("./Command[@Type='BuildOther']");
            foreach (XmlNode buildOtherNode in buildOtherNodes)
            {
               if (buildOtherNode.FirstChild == null)
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Command for type 'BuildOther' in unit \"{0}\" is empty.\n", unitName);
                  continue;
               }

               long unitID;
               if (m_objectTypes.TryGetValue(buildOtherNode.FirstChild.Value.ToLower(), out unitID))
               {
                  unit.mCommandBuildOtherUnitIDs.Add(unitID);
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" has a 'BuildOther' command that refers to unit \"{1}\" which does not exist in objects.xml.\n", unitName, buildOtherNode.FirstChild.Value);
               }
            }

            // look for TrainSquad squads
            XmlNodeList trainSquadNodes = objectNode.SelectNodes("./Command[@Type='TrainSquad']");
            foreach (XmlNode trainSquadNode in trainSquadNodes)
            {
               if (trainSquadNode.FirstChild == null)
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Command for type 'TrainSquad' in unit \"{0}\" is empty.\n", unitName);
                  continue;
               }

               long squadID;
               if (m_protoSquadTable.TryGetValue(trainSquadNode.FirstChild.Value.ToLower(), out squadID))
               {
                  unit.mCommandTrainsSquadIDs.Add(squadID);
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" has a 'TrainSquad' command that refers to squad \"{1}\" which does not exist in squad.xml.\n", unitName, trainSquadNode.FirstChild.Value);
               }
            }


            // look for child objects
            XmlNode childObjectListNode = objectNode.SelectSingleNode("./ChildObjects");
            if (childObjectListNode != null)
            {
               XmlNodeList childObjectNodes = childObjectListNode.SelectNodes("./Object");
               foreach (XmlNode childObjectNode in childObjectNodes)
               {
                  if (childObjectNode.FirstChild == null)
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "ChildObjects node in unit \"{0}\" is empty.\n", unitName);
                     continue;
                  }

                  long unitID;
                  if (m_objectTypes.TryGetValue(childObjectNode.FirstChild.Value.ToLower(), out unitID))
                  {
                     unit.mChildUnitIDs.Add(unitID);
                  }
                  else
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" has an 'Object' node with a object \"{1}\" which does not exist in objects.xml.\n", unitName, childObjectNode.FirstChild.Value);
                  }
               }
            }

            if (powerName != null)
            {
               long powerID;
               if (m_tablePowers.TryGetValue(powerName.ToLower(), out powerID))
               {
                  unit.mPower = powerID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to Power \"{1}\" which does not exist in powers.xml.\n", unit.mName, powerName);
               }
            }

            if (unit.mPhysicsInfoFileName != null)
            {
               string physicsInfoFileNameAbsolute = CoreGlobals.getWorkPaths().mGameDirectory  + "\\" + unit.mPhysicsInfoFileName;

               // check if file exists
               if (!File.Exists(physicsInfoFileNameAbsolute))
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" uses PhysicsInfo file \"{1}\" which does not exist.\n", unit.mName, unit.mPhysicsInfoFileName);
               }
            }


            m_protoUnits.Add(unit);
         }


         // Read squads
         //
         XmlNodeList squadNodes = squadsDoc.SelectNodes("/Squads/Squad");
         foreach (XmlNode squadNode in squadNodes)
         {
            string recoveringEffectName = null;

            // read name
            XmlNode nameNode = squadNode.Attributes.GetNamedItem("name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            // read unit names
            XmlNodeList unitNodes = squadNode.SelectNodes("./Units/Unit");
            if (unitNodes.Count == 0)
            {
               // Apparently it's valid to have squads with 0 units now.
               //ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Squad \"{0}\" does not have any units.\n", nameNode.FirstChild.Value);
            }

            // read squad recovering effect (if exists)
            XmlNode recoveringEffectNode = squadNode.SelectSingleNode("./RecoveringEffect");
            if ((recoveringEffectNode != null) && (recoveringEffectNode.FirstChild != null))
               recoveringEffectName = recoveringEffectNode.FirstChild.Value;

            string squadName = nameNode.FirstChild.Value;


            // Add to list
            ProtoSquad squad = new ProtoSquad();

            squad.mName = squadName;

            foreach (XmlNode unitNode in unitNodes)
            {
               if (unitNode.FirstChild == null)
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Empty unit element in Squad \"{1}\".\n", squadName);
                  continue;
               }

               long unitID;
               if (m_objectTypes.TryGetValue(unitNode.FirstChild.Value.ToLower(), out unitID))
               {
                  squad.mUnitIDs.Add(unitID);
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" referenced in Squad \"{1}\" does not exist in objects.xml.\n", unitNode.FirstChild.Value, squadName);
               }
            }

            if (recoveringEffectName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(recoveringEffectName.ToLower(), out unitID))
               {
                  squad.mRecoveringEffectID = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Unit \"{0}\" is referring to RecoveringEffect for Squad \"{1}\" does not exist in objects.xml.\n", recoveringEffectName, squadName);
               }
            }

            m_protoSquads.Add(squad);
         }


         // Read techs
         //
         XmlNodeList techNodes = techsDoc.SelectNodes("//Tech");
         foreach (XmlNode techNode in techNodes)
         {
            ProtoTech protoTech = new ProtoTech();

            bool success = protoTech.load(techNode);

            if (success)
            {
               m_protoTechs.Add(protoTech);
            }
         }


         // Read Powers
         //
         XmlNodeList powerNodes = powersDoc.SelectNodes("/Powers/Power");
         foreach (XmlNode powerNode in powerNodes)
         {
            // read name
            XmlNode nameNode = powerNode.Attributes.GetNamedItem("name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string powerName = nameNode.FirstChild.Value;
            string triggerScriptFileName = null;

            // read vis file (if exists)
            XmlNode triggerScriptNode = powerNode.SelectSingleNode("./TriggerScript");
            if ((triggerScriptNode != null) && (triggerScriptNode.FirstChild != null))
               triggerScriptFileName = triggerScriptNode.FirstChild.Value;



            // Add to list
            Power power = new Power();
            power.mName = powerName;
            power.mTriggerScriptFile = triggerScriptFileName;


            // look for datalevel entries
            XmlNodeList dataLevelNodes = powerNode.SelectNodes("./Attributes/DataLevel | ./Attributes/BaseDataLevel");
            foreach (XmlNode dataLevelNode in dataLevelNodes)
            {
               // Objects
               XmlNodeList protoObjectNodes = dataLevelNode.SelectNodes("./Data[@type='protoobject']");
               foreach (XmlNode protoObjectNode in protoObjectNodes)
               {
                  if (protoObjectNode.FirstChild == null)
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "DataLevel of type 'protoobject' in power \"{0}\" is empty.\n", powerName);
                     continue;
                  }

                  long unitID;
                  if (m_objectTypes.TryGetValue(protoObjectNode.FirstChild.Value.ToLower(), out unitID))
                  {
                     if(!power.mDataLevelUnitIDs.Contains(unitID))
                     {
                        power.mDataLevelUnitIDs.Add(unitID);
                     }
                  }
                  else
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Power \"{0}\" has a DataLevel 'protoobject' entry with object \"{1}\" which does not exist in objects.xml.\n", powerName, protoObjectNode.FirstChild.Value);
                  }
               }


               // Squads
               XmlNodeList protoSquadNodes = dataLevelNode.SelectNodes("./Data[@type='protosquad']");
               foreach (XmlNode protoSquadNode in protoSquadNodes)
               {
                  if (protoSquadNode.FirstChild == null)
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "DataLevel of type 'protosquad' in power \"{0}\" is empty.\n", powerName);
                     continue;
                  }

                  long squadID;
                  if (m_protoSquadTable.TryGetValue(protoSquadNode.FirstChild.Value.ToLower(), out squadID))
                  {
                     if (!power.mDataLevelSquadIDs.Contains(squadID))
                     {
                        power.mDataLevelSquadIDs.Add(squadID);
                     }
                  }
                  else
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Power \"{0}\" has a DataLevel 'protosquad' entry with squad \"{1}\" which does not exist in squads.xml.\n", powerName, protoSquadNode.FirstChild.Value);
                  }
               }

               // Techs
               XmlNodeList dataTechNodes = dataLevelNode.SelectNodes("./Data[@type='tech']");
               foreach (XmlNode dataTechNode in dataTechNodes)
               {
                  if (dataTechNode.FirstChild == null)
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "DataLevel of type 'tech' in power \"{0}\" is empty.\n", powerName);
                     continue;
                  }

                  long techID;
                  if (m_tableTech.TryGetValue(dataTechNode.FirstChild.Value.ToLower(), out techID))
                  {
                     if (!power.mDataLevelTechIDs.Contains(techID))
                     {
                        power.mDataLevelTechIDs.Add(techID);
                     }
                  }
                  else
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Power \"{0}\" has a DataLevel 'tech' entry with tech \"{1}\" which does not exist in techs.xml.\n", powerName, dataTechNode.FirstChild.Value);
                  }
               }

            }


            m_powers.Add(power);
         }


         // Read Abilities
         //
         XmlNodeList abilityNodes = abilitiesDoc.SelectNodes("/Abilities/Ability");
         foreach (XmlNode abilityNode in abilityNodes)
         {
            // read name
            XmlNode nameNode = abilityNode.Attributes.GetNamedItem("Name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string abilityName = nameNode.FirstChild.Value;
            string triggerScriptFileName = null;

            // read vis file (if exists)
            XmlNode triggerScriptNode = abilityNode.SelectSingleNode("./TriggerScript");
            if ((triggerScriptNode != null) && (triggerScriptNode.FirstChild != null))
               triggerScriptFileName = triggerScriptNode.FirstChild.Value;



            // Add to list
            Ability ability = new Ability();
            ability.mName = abilityName;
            ability.mTriggerScriptFile = triggerScriptFileName;

            m_abilities.Add(ability);
         }


         // Read GameModes
         //
         XmlNodeList gameModesNodes = gameModesDoc.SelectNodes("/GameModes/GameMode");
         foreach (XmlNode gameModeNode in gameModesNodes)
         {
            // read name
            XmlNode nameNode = gameModeNode.SelectSingleNode("Name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;


            string gameModeName = nameNode.FirstChild.Value;
            string worldTriggerScriptFileName = null;
            string playerTriggerScriptFileName = null;

            XmlNode triggerScriptNode = gameModeNode.SelectSingleNode("./WorldScript");
            if ((triggerScriptNode != null) && (triggerScriptNode.FirstChild != null))
               worldTriggerScriptFileName = triggerScriptNode.FirstChild.Value;

            triggerScriptNode = gameModeNode.SelectSingleNode("./PlayerScript");
            if ((triggerScriptNode != null) && (triggerScriptNode.FirstChild != null))
               playerTriggerScriptFileName = triggerScriptNode.FirstChild.Value;


            // Add to list
            GameMode gameMode = new GameMode();
            gameMode.mName = gameModeName;
            gameMode.mWorldTriggerScriptFile = worldTriggerScriptFileName;
            gameMode.mPlayerTriggerScriptFile = playerTriggerScriptFileName;

            m_gameModes.Add(gameMode);
         }


         // Read leaders
         //
         XmlNodeList leaderNodes = leadersDoc.SelectNodes("/Leaders/Leader");
         foreach (XmlNode leaderNode in leaderNodes)
         {
            // read name
            XmlNode nameNode = leaderNode.Attributes.GetNamedItem("Name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string leaderName = nameNode.FirstChild.Value;
            string civName = null;
            string techName = null;
            string startingUnitName = null;
            string startingSquadName = null;

            // read civ
            XmlNode civNameNode = leaderNode.SelectSingleNode("./Civ");
            if ((civNameNode != null) && (civNameNode.FirstChild != null))
               civName = civNameNode.FirstChild.Value;

            // read tech
            XmlNode techNameNode = leaderNode.SelectSingleNode("./Tech");
            if ((techNameNode != null) && (techNameNode.FirstChild != null))
               techName = techNameNode.FirstChild.Value;

            // read starting unit
            XmlNode startingUnitNode = leaderNode.SelectSingleNode("./StartingUnit");
            if ((startingUnitNode != null) && (startingUnitNode.FirstChild != null))
               startingUnitName = startingUnitNode.FirstChild.Value;

            // read starting squad
            XmlNode startingSquadNode = leaderNode.SelectSingleNode("./StartingSquad");
            if ((startingSquadNode != null) && (startingSquadNode.FirstChild != null))
               startingSquadName = startingSquadNode.FirstChild.Value;


            // Add to list
            Leader leader = new Leader();
            leader.mName = leaderName;

            if (civName != null)
            {
               long civID;
               if (m_tableCivs.TryGetValue(civName.ToLower(), out civID))
               {
                  leader.mCiv = civID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Leader \"{0}\" is referring to Civ \"{1}\" which does not exist in civs.xml.\n", leaderName, civName);
               }
            }

            if (techName != null)
            {
               long techID;
               if (m_tableTech.TryGetValue(techName.ToLower(), out techID))
               {
                  leader.mTech = techID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Leader \"{0}\" is referring to Tech \"{1}\" which does not exist in techs.xml.\n", leaderName, techName);
               }
            }


            if (startingUnitName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(startingUnitName.ToLower(), out unitID))
               {
                  leader.mStartingUnit = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Leader \"{0}\" is referring to Starting Unit \"{1}\" which does not exist in objects.xml.\n", leaderName, startingUnitName);
               }
            }


            if (startingSquadName != null)
            {
               long squadID;
               if (m_protoSquadTable.TryGetValue(startingSquadName.ToLower(), out squadID))
               {
                  leader.mStartingSquad = squadID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Leader \"{0}\" is referring to Starting Squad \"{1}\" which does not exist in squads.xml.\n", leaderName, startingSquadName);
               }
            }

            m_leaders.Add(leader);
         }


         // Read Civs
         //
         XmlNodeList civNodes = civsDoc.SelectNodes("/Civs/Civ");
         foreach (XmlNode civNode in civNodes)
         {
            // read name
            XmlNode nameNode = civNode.SelectSingleNode("Name");
            if ((nameNode == null) || (nameNode.FirstChild == null))
               continue;

            string civName = nameNode.FirstChild.Value;
            string techName = null;
            string commandAckName = null;
            string rallyPointName = null;
            string localRallyPointName = null;
            string transportName = null;

            // read tech
            XmlNode techNameNode = civNode.SelectSingleNode("./CivTech");
            if ((techNameNode != null) && (techNameNode.FirstChild != null))
               techName = techNameNode.FirstChild.Value;

            // read command acknowledgment object
            XmlNode commandAckNameNode = civNode.SelectSingleNode("./CommandAckObject");
            if ((commandAckNameNode != null) && (commandAckNameNode.FirstChild != null))
               commandAckName = commandAckNameNode.FirstChild.Value;

            // read command acknowledgment object
            XmlNode rallyPointNameNode = civNode.SelectSingleNode("./RallyPointObject");
            if ((rallyPointNameNode != null) && (rallyPointNameNode.FirstChild != null))
               rallyPointName = rallyPointNameNode.FirstChild.Value;

            // read command acknowledgment object
            XmlNode localRallyPointNameNode = civNode.SelectSingleNode("./LocalRallyPointObject");
            if ((localRallyPointNameNode != null) && (localRallyPointNameNode.FirstChild != null))
               localRallyPointName = localRallyPointNameNode.FirstChild.Value;

            // read transport unit
            XmlNode transportNameNode = civNode.SelectSingleNode("./Transport");
            if ((transportNameNode != null) && (transportNameNode.FirstChild != null))
               transportName = transportNameNode.FirstChild.Value;


            // Add to list
            Civ civ = new Civ();
            civ.mName = civName;

            if (techName != null)
            {
               long techID;
               if (m_tableTech.TryGetValue(techName.ToLower(), out techID))
               {
                  civ.mTech = techID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Civ \"{0}\" is referring to Tech \"{1}\" which does not exist in techs.xml.\n", civName, techName);
               }
            }

            if (commandAckName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(commandAckName.ToLower(), out unitID))
               {
                  civ.mCommandAckUnit = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Civ \"{0}\" is referring to CommandAck unit \"{1}\" which does not exist in objects.xml.\n", civName, commandAckName);
               }
            }

            if (rallyPointName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(rallyPointName.ToLower(), out unitID))
               {
                  civ.mRallyPointUnit = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Civ \"{0}\" is referring to RallyPoint unit \"{1}\" which does not exist in objects.xml.\n", civName, rallyPointName);
               }
            }

            if (localRallyPointName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(localRallyPointName.ToLower(), out unitID))
               {
                  civ.mLocalRallyPointUnit = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Civ \"{0}\" is referring to LocalRallyPoint unit \"{1}\" which does not exist in objects.xml.\n", civName, localRallyPointName);
               }
            }

            if (transportName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(transportName.ToLower(), out unitID))
               {
                  civ.mTransportUnit = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Civ \"{0}\" is referring to Transform unit \"{1}\" which does not exist in objects.xml.\n", civName, transportName);
               }
            }

            m_civs.Add(civ);
         }


         // Build unit infection map
         //
         XmlNodeList infectionMapNodes = gameDataDoc.SelectNodes("/GameData/InfectionMap/InfectionMapEntry");

         foreach (XmlNode infectionMapNode in infectionMapNodes)
         {
            long baseUnitID = -1;
            long infectedUnitID = -1;

            // read base unit name
            XmlNode baseNode = infectionMapNode.Attributes.GetNamedItem("base");
            if ((baseNode == null) || (baseNode.FirstChild == null))
               continue;

            string baseUnitName = baseNode.FirstChild.Value;
            if (baseUnitName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(baseUnitName.ToLower(), out unitID))
               {
                  baseUnitID = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "InfectionMapEntry in gamedata.xml refers to base Unit \"{0}\" which does not exist in objects.xml.\n", baseUnitName);
               }
            }

            // read infected unit name
            XmlNode infectedNode = infectionMapNode.Attributes.GetNamedItem("infected");
            if ((infectedNode == null) || (infectedNode.FirstChild == null))
               continue;

            string infectedUnitName = infectedNode.FirstChild.Value;
            if (infectedUnitName != null)
            {
               long unitID;
               if (m_objectTypes.TryGetValue(infectedUnitName.ToLower(), out unitID))
               {
                  infectedUnitID = unitID;
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "InfectionMapEntry in gamedata.xml refers to infected Unit \"{0}\" which does not exist in objects.xml.\n", infectedUnitName);
               }
            }

            if ((baseUnitID != -1) && (infectedUnitID != -1))
            {
               // Add to infection map
               m_infectionMap.Add(baseUnitName.ToLower(), infectedUnitID);
            }
         }

      }


      static public int getCivCount()
      {
         if (!bInitialized)
            return -1;

         return (m_civs.Count);
      }

      static public string getCivName(int i)
      {
         return (m_civs[i].mName);
      }

      //==============================================================================
      // archiveCiv
      //==============================================================================
      static public void processCiv(int i, List<string> dependenyList, bool bOutputUnitNamesOnly)
      {
         if (!bInitialized)
            return;


         Civ civ = m_civs[i];
         TechTree tree = new TechTree(m_protoTechs);


         // Enable civ tech
         if (civ.mTech != -1)
         {
            //tree.setTechStatus(civ.mTech, TechStatus.cStatusObtainable);
            tree.activateTech(civ.mTech);
         }

         // Enable all leaders techs and staring units
         int leaderCount = m_leaders.Count;
         for (int j = 0; j < leaderCount; j++)
         {
            Leader leader = m_leaders[j];

            // When same civ
            if (leader.mCiv == i)
            {
               if (leader.mTech != -1)
               {
                  //tree.setTechStatus(leader.mTech, TechStatus.cStatusObtainable);
                  tree.activateTech(leader.mTech);
               }

               // Enable command starting unit
               if (leader.mStartingUnit != -1)
               {
                  tree.buildUnit(leader.mStartingUnit);
               }

               // Enable command starting squad
               if (leader.mStartingSquad != -1)
               {
                  tree.buildSquad(leader.mStartingSquad);
               }
            }
         }

         // Enable command acknowledgment unit
         if (civ.mCommandAckUnit != -1)
         {
            tree.buildUnit(civ.mCommandAckUnit);
            //tree.applyProtoObjectEffect(civ.mCommandAckUnit);
         }

         // Enable rally point unit
         if (civ.mRallyPointUnit != -1)
         {
            tree.buildUnit(civ.mRallyPointUnit);
            //tree.applyProtoObjectEffect(civ.mRallyPointUnit);
         }

         // Enable local rally point unit
         if (civ.mLocalRallyPointUnit != -1)
         {
            tree.buildUnit(civ.mLocalRallyPointUnit);
            //tree.applyProtoObjectEffect(civ.mLocalRallyPointUnit);
         }

         // Enable transport unit
         if (civ.mTransportUnit != -1)
         {
            tree.buildUnit(civ.mTransportUnit);
            //tree.applyProtoObjectEffect(civ.mTransportUnit);
         }

         // Process
         //
         //tree.process();

         tree.getDependencyList(dependenyList, bOutputUnitNamesOnly);
      }
   }
}
