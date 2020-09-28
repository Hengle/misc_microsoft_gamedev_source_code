using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using ConsoleUtils;
using DatabaseUtils;


namespace DependencyUtils
{
   class DependencySupportSCN : IDependencyInterface
   {
      private string mExtensions = ".scn";
      private static XmlSerializer mXmlSerializerSCN = new XmlSerializer(typeof(Scenario), new Type[] { });

      string IDependencyInterface.getExtensions()
      {
         return (mExtensions);
      }

      bool IDependencyInterface.getDependencyList(string filename, List<FileInfo> dependencies, List<string> dependentUnits)
      {
         List<string> dependenciesFile = new List<string>();
         List<string> unitDependencies = new List<string>();

         // check extension
         String ext = Path.GetExtension(filename).ToLower();
         if (!mExtensions.Contains(ext))
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a SCN file.  The extension must be \"{1}\".\n", filename, mExtensions);
            return false;
         }

         string fileNameAbsolute = CoreGlobals.getWorkPaths().mGameDirectory + "\\" + filename;

         // check if file exists
         if (!File.Exists(fileNameAbsolute))
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" not found.\n", filename);
            return false;
         }

         ConsoleOut.Write(ConsoleOut.MsgType.Info, "Processing File: \"{0}\"\n", filename);


         Scenario scenarioFile = null;
         Scenario scenarioFile2 = null;
         Scenario scenarioFile3 = null;


         // Load main scenario file
         Stream st = File.OpenRead(fileNameAbsolute);
         try
         {
            scenarioFile = (Scenario)mXmlSerializerSCN.Deserialize(st);
         }
         catch (System.Exception ex)
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a valid xml Scenario (.scn) file.\n", filename);
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "{0}\n", ex.ToString());
            st.Close();
            return (false);
         }
         st.Close();


         // Load possible extra extension .sc2 and .sc3 (art and sound versions)
         string fileName2Absolute = Path.ChangeExtension(fileNameAbsolute, ".sc2");
         if (File.Exists(fileName2Absolute))
         {
            Stream st2 = File.OpenRead(fileName2Absolute);
            try
            {
               scenarioFile2 = (Scenario)mXmlSerializerSCN.Deserialize(st2);
            }
            catch (System.Exception ex)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a valid xml Scenario (.sc2) file.\n", filename);
               ConsoleOut.Write(ConsoleOut.MsgType.Error, "{0}\n", ex.ToString());
            }
            st2.Close();
         }

         // Load possible extra extension .sc3
         string fileName3Absolute = Path.ChangeExtension(fileNameAbsolute, ".sc3");
         if (File.Exists(fileName3Absolute))
         {
            Stream st2 = File.OpenRead(fileName3Absolute);
            try
            {
               scenarioFile3 = (Scenario)mXmlSerializerSCN.Deserialize(st2);
            }
            catch (System.Exception ex)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Error, "File \"{0}\" is not a valid xml Scenario (.sc3) file.\n", filename);
               ConsoleOut.Write(ConsoleOut.MsgType.Error, "{0}\n", ex.ToString());
            }
            st2.Close();
         }


         // Add art and sound versions of the scenario
         dependenciesFile.Add(Path.ChangeExtension(filename, ".sc2"));
         dependenciesFile.Add(Path.ChangeExtension(filename, ".sc3"));

         string scenarioDirectory = filename.Remove(filename.LastIndexOf('\\'));

         // Add terrain files
         string mapName = scenarioDirectory + "\\" + scenarioFile.Terrain;

         dependenciesFile.Add(String.Concat(mapName, ".xsd"));
         dependenciesFile.Add(String.Concat(mapName, ".xtd"));
         dependenciesFile.Add(String.Concat(mapName, ".xtt"));
         dependenciesFile.Add(String.Concat(mapName, ".xth"));
         dependenciesFile.Add(String.Concat(mapName, ".fls"));
         dependenciesFile.Add(String.Concat(mapName, ".lrp"));

         // Add sky dome
         if (!String.IsNullOrEmpty(scenarioFile.Sky))
         {
            string skyDomeName = "art\\" + scenarioFile.Sky;
            dependenciesFile.Add(String.Concat(skyDomeName, ".vis"));
         }

         // Add terrain environment texture
         if (!String.IsNullOrEmpty(scenarioFile.TerrainEnv))
         {
            string terrainEnvName = "art\\" + scenarioFile.TerrainEnv;
            dependenciesFile.Add(terrainEnvName);
         }

         // Add light set
         if (!String.IsNullOrEmpty(scenarioFile.Lightset))
         {
            string lightSetName = scenarioDirectory + "\\" + scenarioFile.Lightset;
            dependenciesFile.Add(String.Concat(lightSetName, ".gls"));
         }

         // Add other light sets
         if (scenarioFile.Lightsets != null)
         {
            foreach (ScenarioLightset lightset in scenarioFile.Lightsets)
            {
               if (lightset.Value == null)
                  continue;

               string lightsetName = "art\\" + lightset.Value;

               dependenciesFile.Add(String.Concat(lightsetName, ".gls"));
            }
         }


         // Add pathing
         //string pathingName = scenarioDirectory + "\\" + scenarioFile.Pathing;
         //dependencies.Add(String.Concat(pathingName, ".pth"));


         // Add cinematics
         //
         if (scenarioFile.Cinematics != null)
         {
            foreach (ScenarioCinematic cin in scenarioFile.Cinematics)
            {
               if (cin.Value == null)
                  continue;

               string cinName = "art\\" + cin.Value;

               dependenciesFile.Add(String.Concat(cinName, ".cin"));
            }
         }

         // Add talking heads
         //
         if (scenarioFile.TalkingHeads != null)
         {
            foreach (ScenarioTalkingHead talkingHead in scenarioFile.TalkingHeads)
            {
               if (talkingHead.Value == null)
                  continue;

               string talkingHeadFileName = "video\\talkingheads\\" + talkingHead.Value;
               talkingHeadFileName = String.Concat(talkingHeadFileName, ".bik");

               string absoluteTalkingHeadFileName = CoreGlobals.getWorkPaths().mGameDirectory + "\\" + talkingHeadFileName;

               // check if file exists
               if (File.Exists(absoluteTalkingHeadFileName))
               {
                  dependenciesFile.Add(talkingHeadFileName);
               }
               else
               {
                  //ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario \"{0}\" refers to has Talking Head video \"{1}\" which does not exist.\n", filename, talkingHeadName);
               }
            }
         }

         // Add all terrain tile tfx so that all impact effects are loaded.
         // TODO:  This needs to be moved to the root archive to save on disk
         //        space, since otherwise these assets will be duplicated in
         //        every scenario archive.
         string terrainTileFile = "art\\effects\\terraineffects\\terraintile.tfx";
         dependenciesFile.Add(terrainTileFile);



         // Check if the scenario has flood infection.  There is no way to do this 
         // easily now, so just hardcode the names of the particular scenarios here
         //
         bool bHasInfection = false;

         string scenarioName = Path.GetFileNameWithoutExtension(filename);
         if ((String.Compare(scenarioName, "release", true) == 0) ||
            (String.Compare(scenarioName, "crevice", true) == 0) ||
            (String.Compare(scenarioName, "exile", true) == 0) ||
            (String.Compare(scenarioName, "phxscn08", true) == 0) ||
            (String.Compare(scenarioName, "phxscn09", true) == 0) ||
            (String.Compare(scenarioName, "phxscn10", true) == 0) ||
            (String.Compare(scenarioName, "phxscn11", true) == 0) ||
            (String.Compare(scenarioName, "phxscn13", true) == 0) ||
            (String.Compare(scenarioName, "phxscn14", true) == 0) ||
            (String.Compare(scenarioName, "phxscn15", true) == 0))
         {
            bHasInfection = true;
         }




         if (scenarioFile.Players == null)
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario file \"{0}\" does not have any players.  Nothing to do here then.\n", filename);
            return false;
         }

         // Loop through all players
         foreach (ScenarioPlayer player in scenarioFile.Players)
         {
            TechTree playerTechTree = new TechTree(Database.m_protoTechs);


            // Now process exclude list
            //
            int excludeObjectCount = scenarioFile.mGlobalExcludeObjects.Count;
            for (int k = 0; k < excludeObjectCount; k++)
            {
               long unitID;
               if (Database.m_objectTypes.TryGetValue(scenarioFile.mGlobalExcludeObjects[k].ToLower(), out unitID))
               {
                  // Forbid unit
                  playerTechTree.setExcludeUnit(unitID, true);
               }
               else
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario \"{0}\" has Global Exclude Unit \"{1}\" which does not exist in objects.xml.\n", filename, scenarioFile.mGlobalExcludeObjects[k]);
               }
            }

            // Now forbid all forbidden techs
            //
            int forbidTechCount = player.mForbidTechs.Count;
            for (int k = 0; k < forbidTechCount; k++)
            {
               long techID;
               if (Database.m_tableTech.TryGetValue(player.mForbidTechs[k], out techID))
               {
                  // Forbid tech
                  playerTechTree.setForbidTech(techID, true);
               }
               else
               {
                  //ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario \"{0}\" has Player \"{1}\" referring to Tech \"{2}\" which does not exist in techs.xml.\n", filename, player.Name, player.mForbidTechs[k]);
               }
            }


            // Now forbid all units
            //
            int forbidObjectCount = player.mForbidObjects.Count;
            for (int k = 0; k < forbidObjectCount; k++)
            {
               long unitID;
               if (Database.m_objectTypes.TryGetValue(player.mForbidObjects[k], out unitID))
               {
                  // Forbid unit
                  playerTechTree.setForbidUnit(unitID, true);
               }
               else
               {
                  //ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario \"{0}\" has Player \"{1}\" referring to Unit \"{2}\" which does not exist in objects.xml.\n", filename, player.Name, player.mForbidObjects[k]);
               }
            }

            
            // Now forbid all squads
            //
            int forbidSquadCount = player.mForbidSquads.Count;
            for (int k = 0; k < forbidSquadCount; k++)
            {
               long squadID;
               if (Database.m_protoSquadTable.TryGetValue(player.mForbidSquads[k], out squadID))
               {
                  // Forbid squad
                  playerTechTree.setForbidSquad(squadID, true);
               }
               else
               {
                  //ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario \"{0}\" has Player \"{1}\" referring to Squad \"{2}\" which does not exist in squads.xml.\n", filename, player.Name, player.mForbidObjects[k]);
               }
            }


            if (player.UsePlayerSettings)
            {
               // Must do a two pass approach.  First we enable all the civ and leader techs, and then
               // we start building units.  Else a unit can be built before a tech is enabled which could
               // have unlocked trained units.

               int civCount = Database.m_civs.Count;
               int leaderCount = Database.m_leaders.Count;


               // Enable all civs and all leaders techs
               //
               for (int i = 0; i < civCount; i++)
               {
                  Civ civ = Database.m_civs[i];


                  // Enable civ tech
                  if (civ.mTech != -1)
                  {
                     playerTechTree.activateTech(civ.mTech);
                  }

                  // Enable leaders techs
                  for (int j = 0; j < leaderCount; j++)
                  {
                     Leader leader = Database.m_leaders[j];

                     // When same civ
                     if (leader.mCiv == i)
                     {
                        if (leader.mTech != -1)
                        {
                           playerTechTree.activateTech(leader.mTech);
                        }
                     }
                  }
               }


               // Enable all civs and all leaders units
               //
               for(int i = 0; i < civCount; i++)
               {
                  Civ civ = Database.m_civs[i];


                  // Enable command acknowledgment unit
                  if (civ.mCommandAckUnit != -1)
                  {
                     playerTechTree.buildUnit(civ.mCommandAckUnit);
                  }

                  // Enable rally point unit
                  if (civ.mRallyPointUnit != -1)
                  {
                     playerTechTree.buildUnit(civ.mRallyPointUnit);
                  }

                  // Enable local rally point unit
                  if (civ.mLocalRallyPointUnit != -1)
                  {
                     playerTechTree.buildUnit(civ.mLocalRallyPointUnit);
                  }

                  // Enable transport unit
                  if (civ.mTransportUnit != -1)
                  {
                     playerTechTree.buildUnit(civ.mTransportUnit);
                  }

                  // Enable all leaders techs and staring units
                  for (int j = 0; j < leaderCount; j++)
                  {
                     Leader leader = Database.m_leaders[j];

                     // When same civ
                     if (leader.mCiv == i)
                     {
                        if (player.UseStartingUnits)
                        {
                           // Enable command starting unit
                           if (leader.mStartingUnit != -1)
                           {
                              playerTechTree.buildUnit(leader.mStartingUnit);
                           }

                           // Enable command starting squad
                           if (leader.mStartingSquad != -1)
                           {
                              playerTechTree.buildSquad(leader.mStartingSquad);
                           }
                        }
                     }
                  }
               }
            }
            else
            {
               // enable only this player's civ and leaders
               long civID = -1;
               if (!String.IsNullOrEmpty(player.Civ) && !Database.m_tableCivs.TryGetValue(player.Civ.ToLower(), out civID))
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario \"{0}\" has Player \"{1}\" referring to Civ \"{2}\" which does not exist in civs.xml.\n", filename, player.Name, player.Civ);
               }

               long leaderID = -1;
               if (!String.IsNullOrEmpty(player.Leader1) && !Database.m_tableLeaders.TryGetValue(player.Leader1.ToLower(), out leaderID))
               {
                  ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario \"{0}\" has Player \"{1}\" referring to Leader \"{2}\" which does not exist in civs.xml.\n", filename, player.Name, player.Leader1);
               }


               if (civID != -1 && leaderID != -1)
               {
                  Civ civ = Database.m_civs[(int)civID];
                  Leader leader = Database.m_leaders[(int)leaderID];

                  // Enable civ and leaders techs
                  //

                  if (civ.mTech != -1)
                  {
                     playerTechTree.activateTech(civ.mTech);
                  }

                  if (leader.mTech != -1)
                  {
                     playerTechTree.activateTech(leader.mTech);
                  }


                  // Enable all civs and all leaders units
                  //

                  // Enable command acknowledgment unit
                  if (civ.mCommandAckUnit != -1)
                  {
                     playerTechTree.buildUnit(civ.mCommandAckUnit);
                  }

                  // Enable rally point unit
                  if (civ.mRallyPointUnit != -1)
                  {
                     playerTechTree.buildUnit(civ.mCommandAckUnit);
                  }

                  // Enable local rally point unit
                  if (civ.mLocalRallyPointUnit != -1)
                  {
                     playerTechTree.buildUnit(civ.mLocalRallyPointUnit);
                  }

                  // Enable transport unit
                  if (civ.mTransportUnit != -1)
                  {
                     playerTechTree.buildUnit(civ.mTransportUnit);
                  }


                  if (player.UseStartingUnits)
                  {
                     // Enable command starting unit
                     if (leader.mStartingUnit != -1)
                     {
                        playerTechTree.buildUnit(leader.mStartingUnit);
                     }

                     // Enable command starting squad
                     if (leader.mStartingSquad != -1)
                     {
                        playerTechTree.buildSquad(leader.mStartingSquad);
                     }
                  }
               }
            }


            // Look for all units placed in the scenario
            //
            for (long i = 0; i < 3; i++)
            {
               Scenario scenario = null;
               switch (i)
               {
                  case 0:
                     scenario = scenarioFile;
                     break;
                  case 1:
                     scenario = scenarioFile2;
                     break;
                  case 2:
                     scenario = scenarioFile3;
                     break;
               }

               if ((scenario != null) && (scenario.Objects != null))
               {
                  foreach (ScenarioObject unit in scenario.Objects)
                  {
                     if (unit.Text == null)
                        continue;

                     string unitName = unit.Text[0];


                     if (unit.IsSquad == true)
                     {
                        long squadID;
                        if (Database.m_protoSquadTable.TryGetValue(unitName.ToLower(), out squadID))
                        {
                           playerTechTree.buildSquad(squadID);
                        }
                        else
                        {
                           ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario \"{0}\" is referring to squad \"{1}\" which does not exist in squads.xml.\n", filename, unitName);
                        }
                     }
                     else
                     {
                        long unitID;
                        if (Database.m_objectTypes.TryGetValue(unitName.ToLower(), out unitID))
                        {
                           playerTechTree.buildUnit(unitID);
                        }
                        else
                        {
                           ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario \"{0}\" is referring to unit \"{1}\" which does not exist in objects.xml.\n", filename, unitName);
                        }
                     }
                  }
               }
            }


            // Look for all units that are being referred/created by the scenario trigger scripts
            //
            if (scenarioFile.TriggerSystem != null)
            {
               XmlDocument scenarioDoc = new XmlDocument();
               CoreGlobals.XmlDocumentLoadHelper(scenarioDoc, fileNameAbsolute);

               XmlNodeList triggerSystemNodes = scenarioDoc.SelectNodes("//Scenario/TriggerSystem");
               foreach (XmlNode triggerSystemNode in triggerSystemNodes)
               {
                  playerTechTree.activateTriggerSystemNode(triggerSystemNode, filename);
               }
            }

            if (scenarioFile.ExternalTriggerScript != null)
            {
               foreach (ScenarioExternalTriggerScript externalScript in scenarioFile.ExternalTriggerScript)
               {
                  if (externalScript.FileName == null)
                     continue;

                  string externalScriptFileName = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\triggerscripts\\" + externalScript.FileName;

                  // check if file exists
                  if (!File.Exists(externalScriptFileName))
                  {
                     ConsoleOut.Write(ConsoleOut.MsgType.Warn, "ExternalTriggerScript trigger script file \"{0}\" specified in \"{1}\" scenario does not exist.\n", externalScriptFileName, filename);
                     continue;
                  }

                  // Read script file
                  XmlDocument triggerScripDoc = new XmlDocument();
                  CoreGlobals.XmlDocumentLoadHelper(triggerScripDoc, externalScriptFileName);

                  XmlNodeList triggerSystemNodes = triggerScripDoc.SelectNodes("/TriggerSystem");
                  foreach (XmlNode exposedTriggerSystemNode in triggerSystemNodes)
                  {
                     playerTechTree.activateTriggerSystemNode(exposedTriggerSystemNode, externalScriptFileName);
                  }
               }
            }

            // Enable all game modes for Skirmish Maps
            if(player.UsePlayerSettings)
            {
               foreach (GameMode mode in Database.m_gameModes)
               {
                  if (mode.mWorldTriggerScriptFile != null)
                  {
                     string externalScriptFileName = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\triggerscripts\\" + mode.mWorldTriggerScriptFile;

                     // check if file exists
                     if (!File.Exists(externalScriptFileName))
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Warn, "GameMode trigger script file \"{0}\" specified in gamemodes.xml does not exist.\n", externalScriptFileName);
                     }
                     else
                     {
                        // Read script file
                        XmlDocument triggerScripDoc = new XmlDocument();
                        CoreGlobals.XmlDocumentLoadHelper(triggerScripDoc, externalScriptFileName);

                        XmlNodeList triggerSystemNodes = triggerScripDoc.SelectNodes("/TriggerSystem");
                        foreach (XmlNode exposedTriggerSystemNode in triggerSystemNodes)
                        {
                           playerTechTree.activateTriggerSystemNode(exposedTriggerSystemNode, externalScriptFileName);
                        }
                     }
                  }


                  if (mode.mPlayerTriggerScriptFile != null)
                  {
                     string externalScriptFileName = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\triggerscripts\\" + mode.mPlayerTriggerScriptFile;

                     // check if file exists
                     if (!File.Exists(externalScriptFileName))
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Warn, "GameMode trigger script file \"{0}\" specified in gamemodes.xml does not exist.\n", externalScriptFileName);
                     }
                     else
                     {
                        // Read script file
                        XmlDocument triggerScripDoc = new XmlDocument();
                        CoreGlobals.XmlDocumentLoadHelper(triggerScripDoc, externalScriptFileName);

                        XmlNodeList triggerSystemNodes = triggerScripDoc.SelectNodes("/TriggerSystem");
                        foreach (XmlNode exposedTriggerSystemNode in triggerSystemNodes)
                        {
                           playerTechTree.activateTriggerSystemNode(exposedTriggerSystemNode, externalScriptFileName);
                        }
                     }
                  }
               }
            }


            // Add infected units if this map has flood infection
            //
            if (bHasInfection)
            {
               playerTechTree.buildInfectedUnits();
            }

            // Add collectors edition units
            playerTechTree.buildCollectorsEditionUnits();

            // Grunt Birthday skull
            playerTechTree.buildGruntPartySkullUnits();
            
            // WuvWoo skull
            playerTechTree.buildWuvWooSkullUnits();


            // Resolve missing projectile changes from wraith and cobra.  This is a hardcoded
            // change since it's too big (and risky) of a change to do the real fix at this point.

            // ***************************************************************************
            // HACK BEGINS
            playerTechTree.buildMissingProjectileHack();
            // HACK ENDS
            // ***************************************************************************


            playerTechTree.buildExtraGameDataUnits();


            // Add forerunner monitor shields
            playerTechTree.buildForerunnerMonitorShieldUnits();


            // Process
            //
            /*
            playerTechTree.process();
            */

           

            playerTechTree.getDependencyList(dependenciesFile, false);
            playerTechTree.getDependencyList(unitDependencies, true);
         }


         // ----------------------------------------
         // Hack for E3 
         // [remember to take this out]
         //
         dependenciesFile.Add("art\\effects\\unit_fx\\cov\\grunt\\methane_explosion.tfx");
         dependenciesFile.Add("art\\effects\\special\\god_bombimpact_01.tfx");
         // ---------------------------------------


         foreach(string dependentFileName in dependenciesFile)
         {
            // Check file existance
            bool fileExists = File.Exists(CoreGlobals.getWorkPaths().mGameDirectory + "\\" + dependentFileName);

            if (!fileExists)
            {
               ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Scenario File \"{0}\" is referring to file \"{1}\" which does not exist.\n", filename, dependentFileName);
            }

            dependencies.Add(new FileInfo(dependentFileName, fileExists));
         }




         // Remove duplicate
         foreach (string dependentUnit in unitDependencies)
         {
            if (!dependentUnits.Contains(dependentUnit))
            {
               dependentUnits.Add(dependentUnit);
            }
         }


         return true;
      }
   }
}
