using System;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Serialization;

using SimEditor;
using EditorCore;
using EditorCore.Controls.Micro;

namespace PhoenixEditor.ScenarioEditor
{

   public class TriggerSystemDefinitions
   {

      Dictionary<string, TriggerEffect> mInternalEffectsMap = new Dictionary<string, TriggerEffect>();

      Dictionary<string, TriggerCondition> mConditionsMap = new Dictionary<string, TriggerCondition>();
      Dictionary<string, TriggerEffect> mEffectsMap = new Dictionary<string, TriggerEffect>();
      Dictionary<int, TriggerValue> mTriggerDefaultValues = new Dictionary<int, TriggerValue>();
      int mVarIdCount = 0;

      Dictionary<int, int> mLatestVersion = new Dictionary<int, int>();
      Dictionary<int, TriggerComponentDefinition> mDefinitionsByDBID = new Dictionary<int, TriggerComponentDefinition>();
      Dictionary<int, Dictionary<int, TriggerComponentDefinition>> mAllDefinitionsByDBID = new Dictionary<int, Dictionary<int,TriggerComponentDefinition>>();
      Dictionary<string, int> mStringToDBID = new Dictionary<string, int>();
      bool bPreload = true;
      Dictionary<string, TriggerTemplateDefinition> mTemplateDefinitions = new Dictionary<string, TriggerTemplateDefinition>();
      Dictionary<string, TriggerTemplateDefinition> mObsoleteTemplateDefinitions = new Dictionary<string, TriggerTemplateDefinition>();


      public TriggerSystemDefinitions()
      {
         LoadTriggerDescription(); 
         LoadTemplates();
         LoadTypeNames();
         LoadUserClassDefinitions();
      }

      public void ReloadData()
      {
         mVarIdCount = 0;
         mConditionsMap.Clear();
         mEffectsMap.Clear();
         mTriggerDefaultValues.Clear();
         mLatestVersion.Clear();
         mDefinitionsByDBID.Clear();
         mStringToDBID.Clear();
         mTemplateDefinitions.Clear();

         LoadTriggerDescription();
         LoadTemplates();
         LoadUserClassDefinitions();
      }

      XmlSerializer mTriggerDefinitionSerializer = new XmlSerializer(typeof(TriggerDefinition), new Type[] { });
      XmlSerializer mTemplateDefinitionSerializer = new XmlSerializer(typeof(TriggerTemplateDefinition), new Type[] { });
      XmlSerializer mTemplateMappingSerializer = new XmlSerializer(typeof(TriggerTemplateMapping), new Type[] { });
      XmlSerializer mTemplateVersionSerializer = new XmlSerializer(typeof(TemplateVersionFile), new Type[] { });

      TriggerDefinition mDefinition = new TriggerDefinition();

      public enum UpgradeStatus
      {
         UpToDate,
         //NoChanges,  
         TrivialReordering,
         NewParams,
         NewOptionalParams,
         DeletedParams,
         DeletedNullParams,
         //Todo: deleted null optional parameters.
         NewAndRemovedParams
      };

      public UpgradeStatus CalculateUpgrade(TriggerComponent comp)
      {
         TriggerComponentDefinition latestDef = mDefinitionsByDBID[comp.DBID];
         TriggerComponentDefinition thisDef = mAllDefinitionsByDBID[comp.DBID][comp.Version];
         int latestVersion = mLatestVersion[comp.DBID];
            
         if (comp.Version == latestVersion)
         {
            return UpgradeStatus.UpToDate;
         }

         //analyze the component
         //comp


         //analyze the definitions
         List<int> idsInA = new List<int>();
         List<int> idsInB = new List<int>();
         List<int> idsOnlyInA = new List<int>();
         List<int> idsOnlyInB = new List<int>();
         List<int> idsInBoth = new List<int>();

         List<int> optionalA = new List<int>();
         List<int> optionalB = new List<int>();

         foreach (InParameterDefintion param in thisDef.InParameterDefinitions)
         {
            idsInA.Add(param.SigID);
            idsInBoth.Add(param.SigID);
            if(param.Optional == true)
            {
               optionalA.Add(param.SigID);
            }
         }
         foreach (OutParameterDefition param in thisDef.OutParameterDefinitions)
         {
            idsInA.Add(param.SigID);
            idsInBoth.Add(param.SigID);
            if (param.Optional == true)
            {
               optionalA.Add(param.SigID);
            }
         }
         foreach (InParameterDefintion param in latestDef.InParameterDefinitions)
         {
            idsInB.Add(param.SigID);
            idsInBoth.Add(param.SigID);
            if (param.Optional == true)
            {
               optionalB.Add(param.SigID);
            }
         }
         foreach (OutParameterDefition param in latestDef.OutParameterDefinitions)
         {
            idsInB.Add(param.SigID);
            idsInBoth.Add(param.SigID);
            if (param.Optional == true)
            {
               optionalB.Add(param.SigID);
            }
         }

         idsOnlyInA = Set.Difference(idsInBoth, idsInB);
         idsOnlyInB = Set.Difference(idsInBoth, idsInA);

         if(idsOnlyInA.Count == 0 && idsOnlyInB.Count == 0)
         {
            return UpgradeStatus.TrivialReordering;
         }
         else if(idsOnlyInA.Count == 0 && idsOnlyInB.Count > 0)
         {
            //if (Difference(idsOnlyInB, optionalB).Count == idsOnlyInB.Count)
            //if (optionalB.Count == idsOnlyInB.Count)
            if (Set.Difference(optionalB, optionalA).Count == idsOnlyInB.Count)
            {
               return UpgradeStatus.NewOptionalParams;
            }
            else
            {
               return UpgradeStatus.NewParams;
            }
         }
         else if(idsOnlyInA.Count > 0 && idsOnlyInB.Count == 0)
         {
            return UpgradeStatus.DeletedParams;
         }
         else if (idsOnlyInA.Count > 0 && idsOnlyInB.Count > 0)
         {
            return UpgradeStatus.NewAndRemovedParams;
         }
         
         //for (int i = 0; i < comp.Parameter.Count; i++)
         //{
         //   comp.Parameter[i].SigID = templist[i].SigID;
         //}


         return UpgradeStatus.UpToDate;
      }

      //todo make this generic
      //public List<int> Difference(List<int> a, List<int> b)
      //{
      //   List<int> output = new List<int>();
      //   foreach (int i in a)
      //   {
      //      if((a.Contains(i) == true) && (b.Contains(i) == false))
      //      {
      //         output.Add(i);
      //      }
      //   }
      //   return output;
      //}


      public TriggerComponent TryUpgrade(TriggerNamespace n, Trigger t, TriggerComponent comp, UpgradeStatus status, out Dictionary<int, TriggerValue> values)
      {
         // Halwes - 3/11/2008 - Added or condition to allow for auto update of PlayChat trigger effect
         //if (true
         //   && (UpgradeStatus.NewOptionalParams == status))
         //// &&  (UpgradeStatus.TrivialReordering == status)
         //// && (UpgradeStatus.DeletedNullParams == status))
         if((UpgradeStatus.NewOptionalParams == status) || (UpgradeStatus.NewAndRemovedParams == status))
         {
            //Automatic upgrade
            return AutomaticUpgrade(n, t, comp, status, out values);
         }
         values = new Dictionary<int, TriggerValue>();
         return null;
      }

      TriggerComponent AutomaticUpgrade(TriggerNamespace n, Trigger t, TriggerComponent comp, UpgradeStatus status, out Dictionary<int, TriggerValue> values)
      {


         TriggerComponent newComp = null;
         values = null;
         bool onFalse = true;
         TriggerCondition newCond = null;
         TriggerEffect newEff = null;
         if (comp is TriggerEffect)
         {
            this.GetTriggerEffect(comp.Type, out newEff, out values);
            newComp = (TriggerComponent)newEff;

            if (t.TriggerEffects.Effects.Contains((TriggerEffect)comp))
            {
               onFalse = false;
            }            
         }
         else if (comp is TriggerCondition)
         {
            this.GetTriggerCondition(comp.Type, out newCond, out values);
            newComp = (TriggerComponent)newCond;

            newCond.Invert = ((TriggerCondition)comp).Invert;
         }
            int newID;

         //Insert new     
         newComp.JustUpgraded = true;
         bool res = true;
         if (newEff != null)
         {
            n.InsertEffect(t.ID, newEff, values, onFalse, out newID, (TriggerEffect)comp);
            //res = n.DeleteEffect(t, (TriggerEffect)comp, onFalse);
         }
         else if(newCond != null)
         {
            n.InsertCondition(t.ID, newCond, values, out newID, (TriggerCondition)comp);
            //res = n.DeleteCondition(t, (TriggerCondition)comp);
         }

       
         //Update variables
         Dictionary<int, int> oldIDsBySigID = new Dictionary<int, int>();
         for (int i = 0; i < comp.Parameter.Count; i++)
         {
            oldIDsBySigID[comp.Parameter[i].SigID] = comp.Parameter[i].ID;
         }
         List<TriggerVariable> oldParams = comp.Parameter;
         for (int i = 0; i < newComp.Parameter.Count; i++)
         {
            int oldVarID;
            if (oldIDsBySigID.TryGetValue(newComp.Parameter[i].SigID, out oldVarID))
            {
               newComp.Parameter[i].ID = oldVarID;
            }
         }

         //Delete old
         if (newEff != null)
         {
            res = n.DeleteEffect(t, (TriggerEffect)comp, onFalse);
         }
         else if (newCond != null)
         {
            res = n.DeleteCondition(t, (TriggerCondition)comp);
         }


         return newComp;
      }


      //public TriggerComponent GetAutomaticUpgrade()
      //{
      //   int DBID = mStringToDBID[comp.Type];
      //   TriggerComponentDefinition def = mDefinitionsByDBID[DBID];
      //   comp.DBID = DBID;
      //   comp.Version = def.Version;

      //   List<ParameterDefintion> templist = new List<ParameterDefintion>();
      //   templist.AddRange(def.InParameterDefinitions.ToArray());
      //   templist.AddRange(def.OutParameterDefinitions.ToArray());

      //   if (comp.Parameter.Count != templist.Count)
      //   {
      //      //TriggerSystemError("Parameter count does not match " + comp.Type);
      //      debugInfoList.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.InvalidParameters, null, "Parameter count does not match ", comp));
      //      comp.DebugInfo.Clear();
      //      comp.DebugInfo.AddRange(debugInfoList);
      //      return debugInfoList;
      //   }
      //   for (int i = 0; i < comp.Parameter.Count; i++)
      //   {
      //      comp.Parameter[i].SigID = templist[i].SigID;
      //   }

      //   return null;
      //}

      public List<TriggerSystemDebugInfo> ProcessTriggerComponent(TriggerComponent comp)
      {
         //UpgradeStatus status = CalculateUpgrade(comp);
         //if (status != UpgradeStatus.UpToDate)
         //{
         //   CoreGlobals.getErrorManager().SendToErrorWarningViewer(comp.Type + " status: " + status.ToString());
         //   //GetUpgradeReplacement(comp, status);
         //}


         List<TriggerSystemDebugInfo> debugInfoList = new List<TriggerSystemDebugInfo>();
         if (comp.DBID == -1)
         {
            if (mStringToDBID.ContainsKey(comp.Type) == false)
            {
               //TriggerSystemError("Missing from definition " + comp.Type);                             
               debugInfoList.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.MissingDefinition, null, "Missing from definition ", comp));
               comp.DebugInfo.Clear();
               comp.DebugInfo.AddRange(debugInfoList);
               return debugInfoList;
            }
            int DBID = mStringToDBID[comp.Type];
            TriggerComponentDefinition def = mDefinitionsByDBID[DBID];
            comp.DBID = DBID;
            comp.Version = def.Version;

            List<ParameterDefintion> templist = new List<ParameterDefintion>();
            templist.AddRange(def.InParameterDefinitions.ToArray());
            templist.AddRange(def.OutParameterDefinitions.ToArray());

            if (comp.Parameter.Count != templist.Count)
            {
               //TriggerSystemError("Parameter count does not match " + comp.Type);
               debugInfoList.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.InvalidParameters, null, "Parameter count does not match ", comp));
               comp.DebugInfo.Clear();
               comp.DebugInfo.AddRange(debugInfoList);
               return debugInfoList;
            }
            for (int i = 0; i < comp.Parameter.Count; i++ )
            {
               comp.Parameter[i].SigID = templist[i].SigID;
            }
         }
         else if(mDefinitionsByDBID.ContainsKey(comp.DBID) == false)
         {
            debugInfoList.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.MissingDefinition, null, "Major sh*t gone wrong. Missing from definition ", comp));
            comp.DebugInfo.Clear();
            comp.DebugInfo.AddRange(debugInfoList);
            return debugInfoList;
         }
         else
         {
            //future scanning of versions
            //comp.DBID
            if(mAllDefinitionsByDBID.ContainsKey(comp.DBID) == false ||  mAllDefinitionsByDBID[comp.DBID].ContainsKey(comp.Version) == false)
            {
               debugInfoList.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.MissingDefinition, null, "Missing definition: ", comp));
               return debugInfoList;
            }
            TriggerComponentDefinition latestDef = mDefinitionsByDBID[comp.DBID];
            TriggerComponentDefinition thisDef = mAllDefinitionsByDBID[comp.DBID][comp.Version];
            int latestVersion = mLatestVersion[comp.DBID];

            if (thisDef.Obsolete == true)
            {
               //TriggerSystemError("Obsolete version: " + comp.Type);
               debugInfoList.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error,TriggerSystemDebugType.ObsoleteVersion, null, "Obsolete version: ", comp));
               comp.DebugInfo.Clear();
               comp.DebugInfo.AddRange(debugInfoList);
               return debugInfoList;
            }
            else if (comp.Version < latestVersion)
            {
               //TriggerSystemWarning("Old version: " + comp.Type);
               debugInfoList.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Warning, TriggerSystemDebugType.OldVersion, null, "Old version: ", comp));

            }
            else if (thisDef.DoNotUse)
            {
               debugInfoList.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Warning, TriggerSystemDebugType.DoNotUse, null, "Do not use: ", comp));

            }


            //upgrade the name
            if(latestDef.Type != comp.Type)
            {
               comp.Type = latestDef.Type;
            }



         }


         comp.DebugInfo.Clear();
         comp.DebugInfo.AddRange(debugInfoList);
         return debugInfoList;
      }

      public void TriggerSystemError(string text)
      {
         CoreGlobals.getErrorManager().OnSimpleWarning(text);
      }
      public void TriggerSystemWarning(string text)
      {
         CoreGlobals.getErrorManager().OnSimpleWarning(text);
      }

      public void LoadTriggerDescription()
      {

         StreamReader s = new StreamReader(CoreGlobals.getWorkPaths().mEditorSettings + "\\triggerDescription.xml");
         mDefinition = (TriggerDefinition)mTriggerDefinitionSerializer.Deserialize(s);

         //Prescan file?
         foreach (ConditionDefinition def in mDefinition.ConditionDefinitions)
         {
            if (mLatestVersion.ContainsKey(def.DBID) == false)
            {
               mLatestVersion[def.DBID] = def.Version;
            }
            if (mLatestVersion[def.DBID] <= def.Version)
            {
               mLatestVersion[def.DBID] = def.Version;
               mDefinitionsByDBID[def.DBID] = def;
               mStringToDBID[def.Type] = def.DBID;
            }
            if(mAllDefinitionsByDBID.ContainsKey(def.DBID) == false)
            {
               mAllDefinitionsByDBID[def.DBID] = new Dictionary<int, TriggerComponentDefinition>();
            }
            mAllDefinitionsByDBID[def.DBID][def.Version] = def;
         }
         foreach (EffectDefinition def in mDefinition.EffectDefinitions)
         {
            if (mLatestVersion.ContainsKey(def.DBID) == false)
            {
               mLatestVersion[def.DBID] = def.Version;
            }
            if (mLatestVersion[def.DBID] <= def.Version)
            {
               mLatestVersion[def.DBID] = def.Version;
               mDefinitionsByDBID[def.DBID] = def;
               mStringToDBID[def.Type] = def.DBID;
            }
            if (mAllDefinitionsByDBID.ContainsKey(def.DBID) == false)
            {
               mAllDefinitionsByDBID[def.DBID] = new Dictionary<int, TriggerComponentDefinition>();
            }
            mAllDefinitionsByDBID[def.DBID][def.Version] = def;
         }

         //on load warn of obsolete versions?
            //make comparison to indicate fix.

         //warning window

         foreach(ConditionDefinition cond in mDefinition.ConditionDefinitions)
         {
            //Only include the latest version?...
            if (cond.Version != mLatestVersion[cond.DBID])
               continue;
            if (cond.Obsolete == true)
               continue;


            TriggerCondition c = new TriggerCondition();
            c.ID = 0;
            c.Type = cond.Type;
            c.DBID = cond.DBID;
            c.Version = cond.Version;
            c.Async = cond.Async;
            c.AsyncParameterKey = cond.AsyncParameterKey;

            foreach(InParameterDefintion ipd in cond.InParameterDefinitions)
            {
               TriggersInputVariable invar = new TriggersInputVariable();
               invar.ID = mVarIdCount;
               invar.Name = ipd.Name;
               invar.SigID = ipd.SigID;   //Get the version
               invar.Optional = ipd.Optional;               

               TriggerValue val = new TriggerValue();
               val.Name = ipd.Name;
               val.Type = ipd.Type;
               //val.Value = "";

               if (invar.Optional == true)
               {
                  val.IsNull = true;
               }

               c.Parameter.Add(invar);
               mTriggerDefaultValues[mVarIdCount] = val;
               mVarIdCount++;
            }
            foreach (OutParameterDefition opd in cond.OutParameterDefinitions)
            {
               TriggersOutputVariable outvar = new TriggersOutputVariable();
               outvar.ID = mVarIdCount;
               outvar.Name = opd.Name;
               outvar.SigID = opd.SigID;   //Get the version
               outvar.Optional = opd.Optional;


               TriggerValue val = new TriggerValue();
               val.Name = opd.Name;
               val.Type = opd.Type;
               //val.Value = "";

               if (outvar.Optional == true)
               {
                  val.IsNull = true;
               }

               c.Parameter.Add(outvar);
               mTriggerDefaultValues[mVarIdCount] = val;
               mVarIdCount++;
            }

            if (cond.DevOnly && !CoreGlobals.IsDev)
            {

            }
            else
            {
               mConditionsMap[c.Type] = c;
            }
         }
         foreach (EffectDefinition effect in mDefinition.EffectDefinitions)
         {
            //Only include the latest version?...
            if (effect.Version != mLatestVersion[effect.DBID])
               continue;
            if (effect.Obsolete == true)
               continue;


            TriggerEffect e = new TriggerEffect();
            e.ID = 0;
            e.Type = effect.Type;
            e.DBID = effect.DBID;
            e.Version = effect.Version;


            foreach (InParameterDefintion ipd in effect.InParameterDefinitions)
            {
               TriggersInputVariable invar = new TriggersInputVariable();
               invar.ID = mVarIdCount;
               invar.Name = ipd.Name;
               invar.SigID = ipd.SigID;   //Get the version
               invar.Optional = ipd.Optional;

               TriggerValue val = new TriggerValue();
               val.Name = ipd.Name;
               val.Type = ipd.Type;
               //val.Value = "";
               
               if (invar.Optional == true)
               {
                  val.IsNull = true;
               }

               e.Parameter.Add(invar);
               mTriggerDefaultValues[mVarIdCount] = val;
               mVarIdCount++;
            }
            foreach (OutParameterDefition opd in effect.OutParameterDefinitions)
            {
               TriggersOutputVariable outvar = new TriggersOutputVariable();
               outvar.ID = mVarIdCount;
               outvar.Name = opd.Name;
               outvar.SigID = opd.SigID;   //Get the version
               outvar.Optional = opd.Optional;

               TriggerValue val = new TriggerValue();
               val.Name = opd.Name;
               val.Type = opd.Type;
               //val.Value = "";

               if (outvar.Optional == true)
               {
                  val.IsNull = true;
               }

               e.Parameter.Add(outvar);
               mTriggerDefaultValues[mVarIdCount] = val;
               mVarIdCount++;
            }

            if (effect.DevOnly && !CoreGlobals.IsDev)
            {
               mInternalEffectsMap[e.Type] = e;
            }
            else
            {
               mEffectsMap[e.Type] = e;
            }
         }
      }

      public bool TryGetDefinition(string name, bool isEffect, out TriggerComponentDefinition def)
      {
         int dbid = -1;
         def = null;
         if (isEffect == false)
         {
            TriggerCondition cond;
            if (mConditionsMap.TryGetValue(name, out cond))
            {
               dbid = cond.DBID;
            }

         }
         else
         {
            TriggerEffect eff;
            if (mEffectsMap.TryGetValue(name, out eff))
            {
               dbid = eff.DBID;
            }
         }
         if (dbid == -1)
         {
            return false;
         }

         return mDefinitionsByDBID.TryGetValue(dbid, out def);

      }

      public bool TryGetDefinition(int dbid, int ver, out TriggerComponentDefinition def)
      {
         def = null;
         if(mAllDefinitionsByDBID.ContainsKey(dbid) == false || mAllDefinitionsByDBID[dbid].ContainsKey(ver) == false)
            return false;
         def = mAllDefinitionsByDBID[dbid][ver];
         return true;
      }
      public string[] GetTemplateFileNames()
      {
         string[] res = Directory.GetFiles(CoreGlobals.getWorkPaths().mTemplateRoot, "*.xml", SearchOption.AllDirectories);
         for (int i = 0; i < res.Length; i++)
            res[i] = res[i].ToLower();
         return res;
      }
      //public string[] GetPowerFileNames()
      //{
      //   string[] res = Directory.GetFiles(CoreGlobals.getWorkPaths().mScriptPowerDirectory, "*" + CoreGlobals.getWorkPaths().mScriptPowerExtention, SearchOption.AllDirectories);
      //   for(int i = 0; i < res.Length; i++)
      //      res[i] = res[i].ToLower();
      //   return res;
      //}
      public string[] GetGeneralScriptFileNames()
      {
         string[] res = Directory.GetFiles(CoreGlobals.getWorkPaths().mScriptTriggerDirectory, "*" + CoreGlobals.getWorkPaths().mScriptExtention, SearchOption.AllDirectories);
         for (int i = 0; i < res.Length; i++)
            res[i] = res[i].ToLower();
         return res;
      }
      //public string[] GetAbilityFileNames()
      //{
      //   string[] res = Directory.GetFiles(CoreGlobals.getWorkPaths().mScriptAbilityDirectory, "*" + CoreGlobals.getWorkPaths().mScriptAbilityExtention, SearchOption.AllDirectories);
      //   for (int i = 0; i < res.Length; i++)
      //      res[i] = res[i].ToLower();
      //   return res;
      //}


      public bool SaveTemplateDescriptions()//TemplateVersionFile data)
      {
         bool success = false;
         try
         {

            StreamWriter s = new StreamWriter(CoreGlobals.getWorkPaths().mEditorSettings + "\\templateVersions.xml");
            mTemplateVersionSerializer.Serialize(s, mTemplateVersionFile);
            s.Close();
            success = true;
         }
         catch(System.Exception ex)
         {

         }
         return success;
      }

      //TemplateVersionFile mTemplateVersionFile;
      public Dictionary<int, Dictionary<int, TemplateVersionInfo>> mAllTemplateInformation = new Dictionary<int, Dictionary<int, TemplateVersionInfo>>();
      public Dictionary<int,TemplateVersionInfo> mCurrentTemplates = new Dictionary<int,TemplateVersionInfo>();
      public TemplateVersionFile mTemplateVersionFile;
      public void LoadTemplateDescriptions()
      {
         return;//not running yet

         mAllTemplateInformation.Clear();
         mCurrentTemplates.Clear();

         StreamReader s = new StreamReader(CoreGlobals.getWorkPaths().mEditorSettings + "\\templateVersions.xml");
         mTemplateVersionFile = (TemplateVersionFile)mTemplateVersionSerializer.Deserialize(s);

         foreach (TemplateVersionInfo info in mTemplateVersionFile.TemplateInfo)
         {
            if(mAllTemplateInformation.ContainsKey(info.DBID) == false)
            {
               mAllTemplateInformation[info.DBID] = new Dictionary<int, TemplateVersionInfo>();
            }
            mAllTemplateInformation[info.DBID][info.Version] = info; 

            if(info.Obsolete == false 
               && (!mCurrentTemplates.ContainsKey(info.DBID) || mCurrentTemplates[info.DBID].Version < info.Version ))
            {
               mCurrentTemplates[info.DBID] = info;
            }

         }


      }

      //not used
      public void LoadTemplates2()
      {
         LoadTemplateDescriptions();

         mTemplateDefinitions.Clear();

         string[] templates = GetTemplateFileNames();

         //foreach(string templateFile in templates)

         foreach (TemplateVersionInfo info in mCurrentTemplates.Values)
         {
            string templateFile = info.File;

            templateFile = Path.Combine(CoreGlobals.getWorkPaths().mTemplateRoot, templateFile);

            try
            {
               if (templateFile.ToLower().Contains("customswatches.xml") == true)
                  continue;
               if (File.Exists(templateFile) == false)
                  continue;

               string templateFileName = null;
               if (bPreload)
               {

                  //mTemplateSerializer
                  StreamReader s = new StreamReader(templateFile);
                  TriggerTemplateDefinition d = (TriggerTemplateDefinition)mTemplateDefinitionSerializer.Deserialize(s);

                  //do name fix here?

                  templateFileName = templateFile.Substring(CoreGlobals.getWorkPaths().mTemplateRoot.Length);
                  d.TriggerTemplateMapping.Name = templateFileName;

                  mTemplateDefinitions[templateFileName] = d;
               }
               else
               {
                  mTemplateDefinitions[templateFileName] = null;
               }
            }
            catch(System.Exception ex)
            {

            }
         }
      }
      public void LoadTemplates()
      {
         mTemplateDefinitions.Clear();

         string[] templates = GetTemplateFileNames();

         foreach(string templateFile in templates)
         {
            try
            {
               if (templateFile.ToLower().Contains("customswatches.xml") == true)
                  continue;


               string templateFileName = null;
               if (bPreload)
               {

                  //mTemplateSerializer
                  StreamReader s = new StreamReader(templateFile);
                  TriggerTemplateDefinition d = (TriggerTemplateDefinition)mTemplateDefinitionSerializer.Deserialize(s);

                  //do name fix here?
                  templateFileName = templateFile.Substring(CoreGlobals.getWorkPaths().mTemplateRoot.Length);
                  d.TriggerTemplateMapping.Name = templateFileName;

                  if (d.TriggerTemplateMapping.Obsolete || d.TriggerTemplateMapping.DoNotUse)
                  {
                     mObsoleteTemplateDefinitions[templateFileName] = d;
                  }
                  else
                  {
                     mTemplateDefinitions[templateFileName] = d;
                  }
               }
               else
               {
                  mTemplateDefinitions[templateFileName] = null;
               }
            }
            catch(System.Exception ex)
            {

            }
         }
      }
      
      public ICollection<string> GetTemplateNames()
      {
         return mTemplateDefinitions.Keys;

      }
      public TriggerTemplateMapping GetNewMapping(string name)
      {
         TriggerTemplateDefinition d = GetTemplateDefinition(name);
         //xml deep copy
         MemoryStream s = new MemoryStream();
         mTemplateMappingSerializer.Serialize(s, d.TriggerTemplateMapping);
         s.Seek(0, SeekOrigin.Begin);
         TriggerTemplateMapping newMapping = (TriggerTemplateMapping)mTemplateMappingSerializer.Deserialize(s);

         //No Need to xmb this

         return newMapping;
      }
      public TriggerTemplateDefinition GetTemplateDefinition(string name)
      {
         name = name.ToLower();

         if (mTemplateDefinitions.ContainsKey(name) == true)
         {
            return mTemplateDefinitions[name];
         }
         else if (mObsoleteTemplateDefinitions.ContainsKey(name) == true)
         {
            return mObsoleteTemplateDefinitions[name];
         }
         if (mTemplateDefinitions.ContainsKey(name) == true && mTemplateDefinitions[name] == null)
         {
            //(CoreGlobals.getWorkPaths().mTemplateRoot +templateFileName)
            //delayed load not impl
         }

         return null;
      }

      public static int cMaxParameters = 100;

//<Groups>
//   <Group ID="5" X=23 y=45>
//            <Group ID=6

      List<string> mEffectNames = null;
      List<string> mConditionNames = null;

      public ICollection<string> GetConditionNames()
      {
         if (mConditionNames == null)
         {
            mConditionNames = new List<string>();
            mConditionNames.AddRange(mConditionsMap.Keys);
            mConditionNames.Sort();
         }
         return mConditionNames;
         //return mConditionsMap.Keys;
      }
      public ICollection<string> GetEffectNames()
      {
         if (mEffectNames == null)
         {
            mEffectNames = new List<string>();
            mEffectNames.AddRange(mEffectsMap.Keys);
            mEffectNames.Sort();
         }
         return mEffectNames;
         //return mEffectsMap.Keys;
      }

      public ICollection<string> GetInputTypesUsed(string componentName)
      {         
         int dbid;
         if (mStringToDBID.TryGetValue(componentName, out dbid))
         {
            return mDefinitionsByDBID[dbid].WalkInputTypes();
         }
         return new List<string>();
      }
      public ICollection<string> GetOutputTypesUsed(string componentName)
      {
         int dbid;
         if (mStringToDBID.TryGetValue(componentName, out dbid))
         {
            return mDefinitionsByDBID[dbid].WalkOutputTypes();
         }
         return new List<string>();
      }

      public string GetActivateEffectName()
      {
         return "TriggerActivate";
      }
      public string GetDeactivateEffectName()
      {
         return "TriggerDeactivate";
      }

      public bool IsLink(TriggerComponent comp)
      {
         if(comp.Type == GetActivateEffectName() || comp.Type == GetDeactivateEffectName())
         {
            return true;
         }
         return false;

      }

      public string GetTriggerType()
      {
         return "Trigger";
      }

      public bool IsListType(string typeName)
      {
         if (typeName == "EntityList")
         {
            return true;
         }
         else if(typeName.EndsWith("List"))
         {
            return true;
         }

         return false;
      }


      List<string> mTypeNames = new List<string>();
      public string[] GetTypeNames()
      {
         return mTypeNames.ToArray();

      }
      public void LoadTypeNames()
      {
         mTypeNames.Clear();
         mTypeNames.Add("Tech");
         mTypeNames.Add("TechStatus");
         mTypeNames.Add("Operator");
         mTypeNames.Add("ProtoObject");
         mTypeNames.Add("ObjectType");
         mTypeNames.Add("ProtoSquad");
         mTypeNames.Add("Sound");
         mTypeNames.Add("Entity");
         mTypeNames.Add("Trigger");
         mTypeNames.Add("Time");
         mTypeNames.Add("Player");
         mTypeNames.Add("Power");
         mTypeNames.Add("Army");
         mTypeNames.Add("Cost");
         mTypeNames.Add("AnimType");
         //mTypeNames.Add("ActionStatus");
         mTypeNames.Add("EntityList");
         mTypeNames.Add("Bool");
         mTypeNames.Add("UILocation");
         mTypeNames.Add("UIEntity");
         mTypeNames.Add("UIButton");
         mTypeNames.Add("Float");
         mTypeNames.Add("Iterator");
         mTypeNames.Add("Team");
         mTypeNames.Add("PlayerList");
         mTypeNames.Add("TeamList");
         mTypeNames.Add("PlayerState");
         mTypeNames.Add("Objective");
         mTypeNames.Add("Squad");
         mTypeNames.Add("Unit");
         mTypeNames.Add("SquadList");
         mTypeNames.Add("UnitList");
         mTypeNames.Add("UIUnit");
         mTypeNames.Add("UISquad");
         mTypeNames.Add("String");
         mTypeNames.Add("MessageIndex");
         mTypeNames.Add("MessageJustify");
         mTypeNames.Add("MessagePoint");
         mTypeNames.Add("Color");
         mTypeNames.Add("ProtoObjectList");
         mTypeNames.Add("ObjectTypeList");
         mTypeNames.Add("ProtoSquadList");
         mTypeNames.Add("TechList");
         mTypeNames.Add("MathOperator");
         mTypeNames.Add("ObjectDataType");
         mTypeNames.Add("ObjectDataRelative");
         mTypeNames.Add("Civ");
         mTypeNames.Add("ProtoObjectCollection");
         mTypeNames.Add("Object");
         mTypeNames.Add("ObjectList");
         mTypeNames.Add("Group");
         mTypeNames.Add("RefCountType");
         mTypeNames.Add("HUDItem");
         mTypeNames.Add("FlashableUIItem");
         mTypeNames.Add("ControlType");
         mTypeNames.Add("RumbleType");
         mTypeNames.Add("RumbleMotor");
         mTypeNames.Add("UnitFlag");
         mTypeNames.Add("SquadFlag");
         mTypeNames.Add("LOSType");
         mTypeNames.Add("EntityFilterSet");
         mTypeNames.Add("PopBucket");
         mTypeNames.Add("ListPosition");
         mTypeNames.Add("Diplomacy");
         //mTypeNames.Add("ExposedAction");
         mTypeNames.Add("SquadMode");
         //mTypeNames.Add("ExposedScript");
         mTypeNames.Add("KBUnit");
         mTypeNames.Add("KBUnitList");
         mTypeNames.Add("KBBase");
         mTypeNames.Add("KBBaseList");
         mTypeNames.Add("KBUnitQuery");
         mTypeNames.Add("DataScalar");
         mTypeNames.Add("KBBaseQuery");
         mTypeNames.Add("DesignLine");
         mTypeNames.Add("LocStringID");
         mTypeNames.Add("KBUnitFilterSet");
         mTypeNames.Add("Leader");
         mTypeNames.Add("Cinematic");
         mTypeNames.Add("TalkingHead");
         mTypeNames.Add("FlareType");
         mTypeNames.Add("CinematicTag");
         mTypeNames.Add("IconType");
         mTypeNames.Add("Integer");
         mTypeNames.Add("MissionType");
         mTypeNames.Add("MissionState");
         mTypeNames.Add("MissionTargetType");
         mTypeNames.Add("IntegerList");
         mTypeNames.Add("BidType");
         mTypeNames.Add("BidState");
         mTypeNames.Add("BuildingCommandState");
         mTypeNames.Add("Vector");
         mTypeNames.Add("VectorList");
         mTypeNames.Add("PlacementRule");
         mTypeNames.Add("UISquadList");
         mTypeNames.Add("KBSquad");
         mTypeNames.Add("KBSquadList");
         mTypeNames.Add("KBSquadQuery");
         mTypeNames.Add("AISquadAnalysis");
         mTypeNames.Add("AISquadAnalysisComponent");
         mTypeNames.Add("KBSquadFilterSet");
         mTypeNames.Add("ChatSpeaker");
         mTypeNames.Add("CommandType");
         mTypeNames.Add("SquadDataType");
         mTypeNames.Add("EventType");
         mTypeNames.Add("GameStatePredicate");
         mTypeNames.Add("Concept");
         mTypeNames.Add("ConceptList");
         mTypeNames.Add("TimeList");
         mTypeNames.Add("DesignLineList");
         mTypeNames.Add("FloatList");
         mTypeNames.Add("UILocationMinigame");
         mTypeNames.Add("Difficulty");
         //mTypeNames.Add("UserClass");
         //mTypeNames.Add("UserClassDefinition");
         //mTypeNames.Add("UserClass");
         mTypeNames.Add("UserClassType");
         //mTypeNames.Add("UserClassList");         
         mTypeNames.Add("UserClassData");

         mTypeNames.Sort();
      }
      //####NEW TYPE


      public bool CanConvertConst(string oldType, string newType)
      {
         //####NEW TYPE   optional!!! const conversion
         if (oldType == "Float" && newType == "Vector")
         {
            return true;
         }
         else if (oldType == "Vector" && newType == "Float")
         {
            return true;
         }
         else if (oldType == "Unit" && newType == "Squad")
         {
            return true;
         }
         else
         {
            return false;
         }
      }
      public object ConvertConst(object value, string oldType, string newType)
      {
         if (oldType == "Float" && newType == "Vector")
         {
            value = value.ToString().Replace("Radius", "Position");
            return value;
         }
         else if (oldType == "Vector" && newType == "Float")
         {
            value = value.ToString().Replace("Position", "Radius");
            return value;
         }
         else
         {
            return value;
         }
      }



      


      public void GetTriggerCondition(string conditionName, out TriggerCondition condition, out Dictionary<int, TriggerValue> values)
      {
         values = new Dictionary<int, TriggerValue>();
         condition = new TriggerCondition();

         TriggerCondition template = mConditionsMap[conditionName];

         condition.ID = template.ID;
         condition.Type = template.Type;
         condition.Version = template.Version;
         condition.DBID = template.DBID;
         condition.Async = template.Async;
         condition.AsyncParameterKey = template.AsyncParameterKey;

         foreach (TriggerVariable var in template.Parameter)
         {
            TriggerVariable v = null;
            if (var is TriggersInputVariable)
               v = new TriggersInputVariable();
            else
               v = new TriggersOutputVariable();
            var.CopyTo(v);
            condition.Parameter.Add(v);

            TriggerValue vl = new TriggerValue();
            mTriggerDefaultValues[v.ID].CopyTo(vl);
            values[v.ID] = vl;


         }

         //c.Parameter 


         //XmlNode n1 = mConditionsMapXML[conditionName];
         //StringReader strdr1 = new StringReader(n1.OuterXml);
         //condition = (TriggerCondition)mConditionSerializer.Deserialize(strdr1);

         //values = new Dictionary<int, TriggerValue>();
         //foreach (TriggerVariable v in condition.Parameter)
         //{
         //   XmlNode n = mTriggerDefaultValuesXML[v.ID];
         //   StringReader strdr = new StringReader(n.OuterXml);
         //   values[v.ID] = (TriggerValue)mValueSerializer.Deserialize(strdr);
         //}
      }

      public void GetTriggerEffect(string effectName, out TriggerEffect effect, out Dictionary<int, TriggerValue> values)
      {
         values = new Dictionary<int, TriggerValue>();
         effect = new TriggerEffect();

         TriggerEffect template = null;
         if (mEffectsMap.ContainsKey(effectName))
         {
            template = mEffectsMap[effectName];
         }
         else if (mInternalEffectsMap.ContainsKey(effectName))
         {
            template = mInternalEffectsMap[effectName];
         }
         else
         {
            throw new System.Exception("Missing effect definition: " + effectName);
         }

         effect.ID = template.ID;
         effect.Type = template.Type;
         effect.Version = template.Version;
         effect.DBID = template.DBID;
         string schemaType = "";
         foreach (TriggerVariable var in template.Parameter)
         {
            if (IsExpandableType(mTriggerDefaultValues[var.ID].Type))
            {
               if (schemaType == "" || schemaType == null)
               {
                  break;
               }
               if (schemaType != "")
               {

               }

               break;
            }

            else
            {
               TriggerVariable v = null;
               if (var is TriggersInputVariable)
                  v = new TriggersInputVariable();
               else
                  v = new TriggersOutputVariable();
               var.CopyTo(v);
               effect.Parameter.Add(v);

               TriggerValue vl = new TriggerValue();
               mTriggerDefaultValues[v.ID].CopyTo(vl);
               values[v.ID] = vl;


               if (IsSchemaType(mTriggerDefaultValues[var.ID].Type))
               {
                  string value = mTriggerDefaultValues[var.ID].Value;
                  schemaType = mTriggerDefaultValues[var.ID].Value;
                  effect.mLastSchema = schemaType;

               }
            }


         }

         //XmlNode n1 = mEffectsMapXML[effectName];
         //StringReader strdr1 = new StringReader(n1.OuterXml);
         //effect = (TriggerEffect)mEffectSerializer.Deserialize(strdr1);

         //values = new Dictionary<int, TriggerValue>();
         //foreach (TriggerVariable v in effect.Parameter)
         //{
         //   XmlNode n = mTriggerDefaultValuesXML[v.ID];
         //   StringReader strdr = new StringReader(n.OuterXml);
         //   values[v.ID] = (TriggerValue)mValueSerializer.Deserialize(strdr);
         //}
      }

      public bool UpdateComponentUserDataVars(TriggerComponent comp, string UserType, TriggerNamespace triggerNamespace, out Dictionary<int, TriggerValue> values)
      {
         bool changesMade = false;
         values = new Dictionary<int, TriggerValue>();
         TriggerComponentDefinition thisDef = mAllDefinitionsByDBID[comp.DBID][comp.Version];
         //CalculateUpgrade(
         //triggerNamespace.TriggerEditorData.UIData.mUserClasses[]
         TriggerVariable expandableVar = null;
         if (comp.mLastSchema != UserType)
         {
            int largestID = 0;
            List<TriggerVariable> dynamicParams = new List<TriggerVariable>();
            TriggerVariable toRemove = null;
            foreach (TriggerVariable v in comp.Parameter)
            {
               if (v.mbIsDynamicVar)
                  dynamicParams.Add(v);
               TriggerValue val = triggerNamespace.GetValues()[v.ID];
               //if (this.IsExpandableType(val.Type))
               //{
               //   expandableVar = v;
               //}
               if (largestID < v.ID)
                  largestID = v.ID;

               if (val.Type == "UserClassData")
               {
                  toRemove = v;
               }
            }
            if (toRemove != null)
               comp.Parameter.Remove(toRemove);

            //change versions
            int oldSchema = 0;
            int newSchemaID = 0;
            UserClassDefinition newSchema = null;
            if(comp.mLastSchema == null || comp.mLastSchema == "")
            {
               //Init
               //comp.mLastSchema
            }
            else
            {
               if (comp.mLastSchema != UserType)
               {
                  changesMade = true;
                  //remove dynamic entries if schemas not equal
                  foreach (TriggerVariable v in dynamicParams)
                  {
                     comp.Parameter.Remove(v);
                  }
               }


               //upgrade if schemas same name but different version
            }
            //Do we have a new schema!  well, expand that shit
            if (UserType != null || UserType != "")
            {
               if (int.TryParse(UserType, out newSchemaID))
               {
                  changesMade = true;

                  newSchema = mUserClassDefinitions.mUserClasses[newSchemaID];
                  int tempIndex = largestID + 1; 
                  bool isInput = true;

                  if (thisDef.WalkInputTypes().Contains("UserClassData") == false)
                     isInput = false;
                  //if (thisDef.WalkOutputTypes().Contains("UserClassData") == false)
                  //   isInput = true;
                  ParameterDefintion userClassParam = null;
                  int generateSigIDs = 0;
                  foreach (ParameterDefintion d in thisDef.OutParameterDefinitions)
                  {
                     if (d.Type == "UserClassData")
                     {
                        userClassParam = d;
                        generateSigIDs = d.SigID + 1;
                     }
                  }
                  //thisDef.OutParameterDefinitions.Remove(userClassParam)

                  foreach (UserClassFieldDefinition field in newSchema.Fields)
                  {

                     TriggerVariable v = null;
                     //TODO track down UserClassData and see if it is input or output
                     if (isInput)//(var is TriggersInputVariable)
                        v = new TriggersInputVariable();
                     else
                        v = new TriggersOutputVariable();

                     //var.CopyTo(v);
                     v.Name = field.Name;
                     comp.Parameter.Add(v);
                     v.ID = tempIndex;

                     v.SigID = generateSigIDs;
                     generateSigIDs++;

                     tempIndex++;
                     TriggerValue vl = new TriggerValue();
                     vl.Type = field.Type;
                     v.mbIsDynamicVar = true;
                     //mTriggerDefaultValues[v.ID].CopyTo(vl);  //??
                     values[v.ID] = vl;

                  }

                  comp.mLastSchema = newSchema.Name;
               }
            }           
         }
         return changesMade;
      }

      public UserClassDefinitions mUserClassDefinitions = new UserClassDefinitions();
      private void LoadUserClassDefinitions()
      {
         string fileName = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "userclasses.xml");
         if (File.Exists(fileName))
         {
            mUserClassDefinitions = BaseLoader<UserClassDefinitions>.Load(fileName);
         }
      }
      public UserClassDefinition GetUserClassDefinition(string name)
      {
         foreach (UserClassDefinition d in mUserClassDefinitions.mUserClasses)
         {
            if (name == d.Name)
               return d;
         }
         return null;
      }


      public HighLevelProperty GetHLProperty(INamedTypedProperty param, TriggerNamespace triggerNamespace)
      {       
         HighLevelProperty prop = null;
         if (param.GetTypeName() == "Tech")
         {
            prop = new TriggerPropTech(param, TriggerSystemMain.mSimResources.mTechData);
         }
         else if (param.GetTypeName() == "TechStatus")
         {
            prop = new TriggerPropTechStatus(param);
         }
         else if (param.GetTypeName() == "ChatSpeaker")
         {
            prop = new TriggerPropChatSpeaker(param);
         }
         else if (param.GetTypeName() == "Operator")
         {
            prop = new TriggerPropOperator(param);
         }
         else if (param.GetTypeName() == "ProtoObject")
         {
            prop = new TriggerPropProtoObject(param, TriggerSystemMain.mSimResources.mProtoObjectData);
         }
         else if (param.GetTypeName() == "ObjectType")
         {
            prop = new TriggerPropObjectType(param, TriggerSystemMain.mSimResources.mObjectTypeData);
         }
         else if (param.GetTypeName() == "ProtoSquad")
         {
            prop = new TriggerPropProtoSquad(param, TriggerSystemMain.mSimResources.mProtoSquadData);
         }
         else if (param.GetTypeName() == "Sound")
         {
            prop = new TriggerPropSound(param);
         }
         else if (param.GetTypeName() == "Entity")
         {
            prop = new TriggerPropEntity(param);
         }
         else if (param.GetTypeName() == "EntityList")
         {
            prop = new TriggerPropEntityList(param);
         }
         else if (param.GetTypeName() == "Trigger")
         {
            prop = new TriggerPropTrigger(param);
         }
         else if (param.GetTypeName() == "Time")
         {
            prop = new TriggerPropTime(param);
         }
         else if (param.GetTypeName() == "Player")
         {
            prop = new TriggerPropPlayer(param);
         }
         else if (param.GetTypeName() == "Team")
         {
            prop = new TriggerPropTeam(param);
         }
         else if (param.GetTypeName() == "Power")
         {
            prop = new TriggerPropPower(param, TriggerSystemMain.mSimResources.mPowers);
         }
         else if (param.GetTypeName() == "Army")
         {
            prop = new TriggerPropArmy(param);
         }
         else if (param.GetTypeName() == "Cost")
         {
            prop = new TriggerPropCost(param);
         }
         else if (param.GetTypeName() == "AnimType")
         {
            prop = new TriggerPropAnimType(param);
         }
         else if (param.GetTypeName() == "ActionStatus")
         {
            prop = new TriggerPropActionStatus(param);
         }
         else if (param.GetTypeName() == "Bool")
         {
            prop = new TriggerPropBool(param);
         }
         else if (param.GetTypeName() == "Float")
         {
            prop = new TriggerPropDistance(param);
         }
         else if (param.GetTypeName() == "PlayerState")
         {
            prop = new TriggerPropPlayerState(param, TriggerSystemMain.mSimResources.mGameData);
         }
         else if (param.GetTypeName() == "SquadList")
         {
            prop = new TriggerPropSquadList(param);
         }
         else if (param.GetTypeName() == "UnitList")
         {
            prop = new TriggerPropUnitList(param);
         }
         else if (param.GetTypeName() == "Squad")
         {
            prop = new TriggerPropSquad(param);
         }
         else if (param.GetTypeName() == "Unit")
         {
            prop = new TriggerPropUnit(param);
         }
         else if (param.GetTypeName() == "PlayerList")
         {
            prop = new TriggerPropPlayerList(param);
         }
         else if (param.GetTypeName() == "IntegerList")
         {
            prop = new TriggerPropIntegerList(param);
         }
         else if (param.GetTypeName() == "TeamList")
         {
            prop = new TriggerPropTeamList(param);
         }
         else if (param.GetTypeName() == "ProtoObjectList")
         {
            prop = new TriggerPropProtoObjectList(param, TriggerSystemMain.mSimResources.mProtoObjectData);
         }
         else if (param.GetTypeName() == "ProtoObjectCollection")
         {
            prop = new TriggerPropProtoObjectCollection(param, TriggerSystemMain.mSimResources.mProtoObjectData);
         }
         else if (param.GetTypeName() == "ObjectTypeList")
         {
            prop = new TriggerPropObjectTypeList(param, TriggerSystemMain.mSimResources.mObjectTypeData);
         }
         else if (param.GetTypeName() == "ProtoSquadList")
         {
            prop = new TriggerPropProtoSquadList(param, TriggerSystemMain.mSimResources.mProtoSquadData);
         }
         else if (param.GetTypeName() == "ProtoSquadCollection")
         {
            prop = new TriggerPropProtoSquadCollection(param, TriggerSystemMain.mSimResources.mProtoSquadData);
         }
         else if (param.GetTypeName() == "TechList")
         {
            prop = new TriggerPropTechList(param, TriggerSystemMain.mSimResources.mTechData);
         }
         else if (param.GetTypeName() == "Objective")
         {
            if (SimGlobals.getSimMain() != null)
            {
               prop = new TriggerPropObjective(param, SimGlobals.getSimMain().ObjectivesData);
            }
            else
            {
               prop = new HighLevelProperty(param);
            }
         }
         else if (param.GetTypeName() == "String")
         {
            prop = new HighLevelProperty(param);
         }
         else if (param.GetTypeName() == "MessageIndex")
         {
            prop = new TriggerPropMessageIndex(param);
         }
         else if (param.GetTypeName() == "MessageJustify")
         {
            prop = new TriggerPropMessageJustify(param);
         }
         else if (param.GetTypeName() == "MessagePoint")
         {
            prop = new TriggerPropMessagePoint(param);
         }
         else if (param.GetTypeName() == "Color")
         {
            prop = new TriggerPropColor(param);
         }
         else if (param.GetTypeName() == "MathOperator")
         {
            prop = new TriggerPropMathOperator(param);
         }
         else if (param.GetTypeName() == "Civ")
         {
            prop = new TriggerPropCivOperator(param);
         }
         else if (param.GetTypeName() == "ObjectDataType")
         {
            prop = new TriggerPropObjectDataType(param, TriggerSystemMain.mSimResources.mTechData);
         }
         else if (param.GetTypeName() == "ObjectDataRelative")
         {
            prop = new TriggerPropObjectDataRelative(param, TriggerSystemMain.mSimResources.mTechData);
         }
         else if (param.GetTypeName() == "Object")
         {
            prop = new TriggerPropObject(param);
         }
         else if (param.GetTypeName() == "ObjectList")
         {
            prop = new TriggerPropObjectList(param);
         }
         else if (param.GetTypeName() == "Group")
         {
            //SimGlobals.getSimMain().ObjectivesData
            //TriggerSystemMain.
            prop = new TriggerPropGroup(param, triggerNamespace);
         }
         else if (param.GetTypeName() == "RefCountType")
         {
            prop = new TriggerPropRefCountType(param, TriggerSystemMain.mSimResources.mGameData.mRefCountTypes);
         }
         else if (param.GetTypeName() == "HUDItem")
         {
            prop = new TriggerPropHUDItem(param, TriggerSystemMain.mSimResources.mGameData.mHUDItems);
         }
         else if (param.GetTypeName() == "FlashableUIItem")
         {
            prop = new TriggerPropFlashableUIItem(param, TriggerSystemMain.mSimResources.mGameData.mFlashableItems);
         }
         else if (param.GetTypeName() == "ControlType")
         {
            prop = new TriggerPropControlType(param);
         }
         else if (param.GetTypeName() == "RumbleType")
         {
            prop = new TriggerPropRumbleType(param);
         }
         else if (param.GetTypeName() == "RumbleMotor")
         {
            prop = new TriggerPropRumbleMotor(param);
         }
         else if (param.GetTypeName() == "Iterator")
         {
            prop = new TriggerPropIterator(param);
         }
         else if (param.GetTypeName() == "LOSType")
         {
            prop = new TriggerPropLOSType(param);
         }
         else if (param.GetTypeName() == "PopBucket")
         {
            prop = new TriggerPropPopBucket(param, TriggerSystemMain.mSimResources.mGameData);
         }
         else if (param.GetTypeName() == "ListPosition")
         {
            prop = new TriggerPropListPosition(param);
         }
         else if (param.GetTypeName() == "Diplomacy")
         {
            prop = new TriggerPropDiplomacy(param);
         }
         else if (param.GetTypeName() == "ExposedAction")
         {
            prop = new TriggerPropExposedAction(param);
         }
         else if (param.GetTypeName() == "SquadMode")
         {
            prop = new TriggerPropSquadMode(param);
         }
         else if (param.GetTypeName() == "ExposedScript")
         {
            prop = new TriggerPropExposedScript(param, TriggerSystemMain.mSimResources.mExposedScripts);
         }
         else if (param.GetTypeName() == "DataScalar")
         {
            prop = new TriggerPropDataScalar(param);
         }
         else if (param.GetTypeName() == "DesignLine")
         {
            prop = new TriggerPropDesignLine(param);
         }
         else if (param.GetTypeName() == "LocStringID")
         {
            prop = new TriggerPropLocStringID(param);
         }
         else if (param.GetTypeName() == "Leader")
         {
            prop = new TriggerPropLeader(param, TriggerSystemMain.mSimResources.mLeaders);
         }
         else if (param.GetTypeName() == "Cinematic")
         {
            prop = new TriggerPropCinematic(param, CoreGlobals.getGameResources().Cinematics);
         }
         else if (param.GetTypeName() == "TalkingHead")
         {
            prop = new TriggerPropTalkingHead(param, CoreGlobals.getGameResources().TalkingHeads);
         }
         else if (param.GetTypeName() == "FlareType")
         {
            prop = new TriggerPropFlareType(param);
         }
         else if (param.GetTypeName() == "CinematicTag")
         {
            prop = new TriggerPropCinematicTag(param, CoreGlobals.getGameResources().Cinematics);
         }
         else if (param.GetTypeName() == "IconType")
         {
            prop = new TriggerPropIconType(param, TriggerSystemMain.mSimResources.mIconData);
         }
         else if (param.GetTypeName() == "Integer")
         {
            prop = new TriggerPropInteger(param);
         }
         else if (param.GetTypeName() == "MissionType")
         {
            prop = new TriggerPropMissionType(param);
         }
         else if (param.GetTypeName() == "MissionState")
         {
            prop = new TriggerPropMissionState(param);
         }
         else if (param.GetTypeName() == "MissionTargetType")
         {
            prop = new TriggerPropMissionTargetType(param);
         }
         else if (param.GetTypeName() == "BidType")
         {
            prop = new TriggerPropBidType(param);
         }
         else if (param.GetTypeName() == "BidState")
         {
            prop = new TriggerPropBidState(param);
         }
         else if (param.GetTypeName() == "Vector")
         {
            prop = new TriggerPropVector(param);
         }
         else if (param.GetTypeName() == "VectorList")
         {
            prop = new TriggerPropVectorList(param);
         }
         else if (param.GetTypeName() == "PlacementRule")
         {
            prop = new TriggerPropPlacementRule(param, TriggerSystemMain.mSimResources.mPlacementRuleData);
         }
         else if (param.GetTypeName() == "UISquadList")
         {
            prop = new TriggerPropUISquadList(param);
         }
         else if (param.GetTypeName() == "AISquadAnalysisComponent")
         {
            prop = new TriggerPropAISquadAnalysisComponent(param);
         }
         else if (param.GetTypeName() == "UnitFlag")
         {
            prop = new TriggerPropUnitFlag(param, TriggerSystemMain.mSimResources.mGameData.mUnitFlags);
         }
         else if (param.GetTypeName() == "SquadFlag")
         {
            prop = new TriggerPropSquadFlag(param, TriggerSystemMain.mSimResources.mGameData.mSquadFlags);
         }
         else if (param.GetTypeName() == "UserClassType")
         {
            prop = new TriggerPropUserType(param, triggerNamespace);
         }         
         
         //else if (param.GetTypeName() == "UserClass")
         //{
         //   prop = new NoEditProperty(param);
         //}
         //else if (param.GetTypeName() == "UserClassList")
         //{
         //   prop = new NoEditProperty(param);
         //}
         else if (param.GetTypeName() == "CommandType")
         {
            prop = new TriggerPropTechDataCommandType(param, TriggerSystemMain.mSimResources.mTechData);
         }
         else if (param.GetTypeName() == "SquadDataType")
         {
            prop = new TriggerPropSquadDataType(param, TriggerSystemMain.mSimResources.mTechData);
         }
         else if (param.GetTypeName() == "EventType")
         {
            prop = new TriggerPropEventType(param);
         }
         else if (param.GetTypeName() == "GameStatePredicate")
         {
            prop = new TriggerPropGameStatePredicate(param);
         }
         else if (param.GetTypeName() == "Concept")
         {
            prop = new TriggerPropConcept(param, TriggerSystemMain.mSimResources.mHintConcepts);
         }
         else if (param.GetTypeName() == "ConceptList")
         {
            prop = new TriggerPropConceptList(param, TriggerSystemMain.mSimResources.mHintConcepts);
         }
         else if (param.GetTypeName() == "TimeList")
         {
            prop = new TriggerPropTimeList(param);
         }
         else if (param.GetTypeName() == "DesignLineList")
         {
            prop = new TriggerPropDesignLineList(param);
         }
         else if (param.GetTypeName() == "FloatList")
         {
            prop = new TriggerPropFloatList(param);
         }
         else if (param.GetTypeName() == "Difficulty")
         {
            prop = new TriggerPropDifficulty(param);
         }

         //####NEW TYPE
         else
         {
            prop = new HighLevelProperty(param);
         }
         return prop;
      }

      public bool IsDynamicComponent(string componentName)
      {
         if (componentName == "GetTableRow")
         {
            return true;
         }

         return false;

      }
      public bool IsExpandableType(string typeName)
      {
         if (typeName == "UserClassData")
         {
            return true;
         }
         return false;
      }
      public bool IsSchemaType(string typeName)
      {
         if (typeName == "UserClassType")
         {
            return true;
         }
         return false;
      }

      //public void GetNewDefinition(string componentName, )
      //{
      //   if (componentName == "UserClassGetVariables")
      //   {
      //   }
      //}

   }


   [XmlRoot("TriggerDefinition")]
   public class TriggerDefinition
   {
      List<ConditionDefinition> mConditionDefinitions = new List<ConditionDefinition>();
      List<EffectDefinition> mEffectDefinitions = new List<EffectDefinition>();

      [XmlElement("Condition", typeof(ConditionDefinition))]
      public List<ConditionDefinition> ConditionDefinitions
      {
         get
         {
            return mConditionDefinitions;
         }
         set
         {
            mConditionDefinitions = value;
         }
      }

      [XmlElement("Effect", typeof(EffectDefinition))]
      public List<EffectDefinition> EffectDefinitions
      {
         get
         {
            return mEffectDefinitions;
         }
         set
         {
            mEffectDefinitions = value;
         }
      }


      //int mMaxID = -1;
      //[XmlElement("MaxID")]
      //public int MaxID
      //{
      //   get
      //   {
      //      return mMaxID;
      //   }
      //   set
      //   {
      //      mMaxID = value;
      //   }
      //}

   }



   public class TriggerComponentDefinition
   {
      string mType = "";
      [XmlAttribute("Type")]
      public string Type
      {
         get
         {
            return mType;
         }
         set
         {
            mType = value;
         }
      }


      string mHelpText = "";
      [XmlAttribute("HelpText")]
      public string HelpText
      {
         get
         {
            return mHelpText;
         }
         set
         {
            mHelpText = value;
         }
      }

      string mDocumentation = "";
      [XmlAttribute("Documentation")]
      public string Documentation
      {
         get
         {
            return mDocumentation;
         }
         set
         {
            mDocumentation = value;
         }
      }

      bool mObsolete = false;
      [XmlAttribute("Obsolete")]
      public bool Obsolete
      {
         get
         {
            return mObsolete;
         }
         set
         {
            mObsolete = value;
         }
      }

      string mObsoleteHint = "";
      [XmlAttribute("ObsoleteHint")]
      public string ObsoleteHint
      {
         get
         {
            return mObsoleteHint;
         }
         set
         {
            mObsoleteHint = value;
         }
      }

      bool mDoNotUse = false;
      [XmlAttribute("DoNotUse")]
      public bool DoNotUse
      {
         get
         {
            return mDoNotUse;
         }
         set
         {
            mDoNotUse = value;
         }
      }

      string mDoNotUseHint = "";
      [XmlAttribute("DoNotUseHint")]
      public string DoNotUseHint
      {
         get
         {
            return mDoNotUseHint;
         }
         set
         {
            mDoNotUseHint = value;
         }
      }

      bool mDevOnly = false;
      [XmlAttribute("DevOnly")]
      public bool DevOnly
      {
         get
         {
            return mDevOnly;
         }
         set
         {
            mDevOnly = value;
         }
      }


      List<InParameterDefintion> mInParameterDefinitions = new List<InParameterDefintion>();
      List<OutParameterDefition> mOutParameterDefinitions = new List<OutParameterDefition>();

      public ICollection<string> WalkInputTypes()
      {
         
         Dictionary<string, bool> blah = new Dictionary<string,bool>();
         foreach(InParameterDefintion d  in mInParameterDefinitions)
         {
            blah[d.Type] = true;
         }
         return blah.Keys;
      }
      public ICollection<string> WalkOutputTypes()
      {
  
         Dictionary<string, bool> blah = new Dictionary<string,bool>();
         foreach(OutParameterDefition d  in mOutParameterDefinitions)
         {
            blah[d.Type] = true;
         }
         
         return blah.Keys;
      }
      


      [XmlElement("Input", typeof(InParameterDefintion))]
      public List<InParameterDefintion> InParameterDefinitions
      {
         get
         {
            return mInParameterDefinitions;
         }
         set
         {
            mInParameterDefinitions = value;
         }
      }

      [XmlElement("Output", typeof(OutParameterDefition))]
      public List<OutParameterDefition> OutParameterDefinitions
      {
         get
         {
            return mOutParameterDefinitions;
         }
         set
         {
            mOutParameterDefinitions = value;
         }
      }

      List<ParameterConversionOverride> mParameterConversionOverrides = new List<ParameterConversionOverride>();
      [XmlElement("Convert", typeof(ParameterConversionOverride))]
      public List<ParameterConversionOverride> ParameterConversionOverrides
      {
         get
         {
            return mParameterConversionOverrides;
         }
         set
         {
            mParameterConversionOverrides = value;
         }
      }


      int mVersion = 1;
      [XmlAttribute("Version")]
      public int Version
      {
         get
         {
            return mVersion;
         }
         set
         {
            mVersion = value;
         }
      }

      //int mMinVersion = 1;
      //[XmlElement("MinVer")]
      //public int MinVer
      //{
      //   get
      //   {
      //      return mMinVersion;
      //   }
      //   set
      //   {
      //      mMinVersion = value;
      //   }
      //}

      int mID = -1;
      [XmlAttribute("DBID")]
      public int DBID
      {
         get
         {
            return mID;
         }
         set
         {
            mID = value;
         }
      }

      int mMaxVarID = 0;
      [XmlAttribute("MaxVarID")]
      public int MaxVarID
      {
         get
         {
            return mMaxVarID;
         }
         set
         {
            mMaxVarID = value;
         }
      }

      public override string ToString()
      {
         return "V" + Version + "    " + Type + ((mObsolete)?"---OBSOLETE----":"");
      }

      //public void CopyTo(out TriggerComponentDefinition other)
      public virtual void CopyTo(TriggerComponentDefinition other)
      {
         //other = (TriggerComponentDefinition)this.MemberwiseClone();
         other.DBID = this.DBID;
         other.MaxVarID = this.MaxVarID;
         other.Obsolete = this.Obsolete;
         other.Type = this.Type;
         other.Version = this.Version;
         other.Documentation = this.Documentation;
         other.HelpText = this.HelpText;
         other.DoNotUse = this.DoNotUse;
         other.DevOnly = this.DevOnly;

         other.InParameterDefinitions = new List<InParameterDefintion>(); 
         foreach(InParameterDefintion p in this.InParameterDefinitions)
         {
            InParameterDefintion def = new InParameterDefintion();
            p.CopyTo(def);
            other.InParameterDefinitions.Add(def);
         }
         other.OutParameterDefinitions = new List<OutParameterDefition>();
         foreach (OutParameterDefition p in this.OutParameterDefinitions)
         {
            OutParameterDefition def = new OutParameterDefition();
            p.CopyTo(def);
            other.OutParameterDefinitions.Add(def);
         }
      }

   }

   [XmlRoot("Convert")]
   public class ParameterConversionOverride 
   {
      string mOldParameter = "";
      public string OldParameter
      {
         get
         {
            return mOldParameter;
         }
         set
         {
            mOldParameter = value;
         }
      }
      string mNewParameter = "";
      public string NewParameter
      {
         get
         {
            return mNewParameter;
         }
         set
         {
            mNewParameter = value;
         }
      }

   }


   [XmlRoot("Effect")]
   public class EffectDefinition : TriggerComponentDefinition
   {




   }

   [XmlRoot("Condition")]
   public class ConditionDefinition : TriggerComponentDefinition
   {
      bool mAsync = false;
      [XmlAttribute("Async")]
      public bool Async
      {
         get
         {
            return mAsync;
         }
         set
         {
            mAsync = value;
         }
      }
      int mAsyncParameterKey = 0;
      [XmlAttribute("AsyncParameterKey")]
      public int AsyncParameterKey
      {
         get
         {
            return mAsyncParameterKey;
         }
         set
         {
            mAsyncParameterKey = value;
         }
      }
      public override void CopyTo(TriggerComponentDefinition other)
      {
         ConditionDefinition otherCondition = other as ConditionDefinition;
         base.CopyTo(other);
         otherCondition.Async = this.Async;
         otherCondition.AsyncParameterKey = this.AsyncParameterKey;
      }
   }

   public class ParameterDefintion
   {
      int mSigID = -1;
      [XmlAttribute("SigID")]
      public int SigID
      {
         get
         {
            return mSigID;
         }
         set
         {
            mSigID = value;
         }
      }
      string mType = "";
      [XmlAttribute("Type")]
      public string Type
      {
         get
         {
            return mType;
         }
         set
         {
            mType = value;
         }
      }
      string mName = "";
      [XmlAttribute("Name")]
      public string Name
      {
         get
         {
            return mName;
         }
         set
         {
            mName = value;
         }
      }
      bool mOptional = false;
      [XmlAttribute("Optional")]
      public bool Optional
      {
          get
          {
              return mOptional;
          }
          set
          {
              mOptional = value;
          }
      }



      public void CopyTo(ParameterDefintion other)
      {
         other.Name = this.Name;
         other.Type = this.Type;
         other.SigID = this.SigID;
         other.Optional = this.Optional;
      }
   }
   [XmlRoot("Input")]
   public class InParameterDefintion : ParameterDefintion
   {

   }
   [XmlRoot("Output")]
   public class OutParameterDefition : ParameterDefintion
   {

   }

   [XmlRoot("TemplateVersionFile")]
   public class TemplateVersionFile
   {
      List<TemplateVersionInfo> mTemplateInfo = new List<TemplateVersionInfo>();

      [XmlElement("Template", typeof(TemplateVersionInfo))]
      public List<TemplateVersionInfo> TemplateInfo
      {
         get
         {
            return mTemplateInfo;
         }
         set
         {
            mTemplateInfo = value;
         }
      }

      int mMaxDBID = 0;
      [XmlAttribute("MaxDBID")]
      public int MaxDBID
      {
         get
         {
            return mMaxDBID;
         }
         set
         {
            mMaxDBID = value;
         }
      }
      public int GetNextDBID()
      {
         mMaxDBID++;
         return mMaxDBID;      
      }

   }

   public class TemplateVersionInfo
   {
      private int mDBID = -1;
      private int mVersion = -1;
      private string mName = "noname!!";
      bool mObsolete = false;
      string mFile = "";

      [XmlAttribute("DBID")]
      public int DBID
      {
         get
         {
            return mDBID;
         }
         set
         {
            mDBID = value;
         }
      }
      [XmlAttribute("Version")]
      public int Version
      {
         get
         {
            return mVersion;
         }
         set
         {
            mVersion = value;
         }
      }
      [XmlAttribute("Name")]
      public string Name
      {
         get
         {
            return mName;
         }
         set
         {
            mName = value;
         }
      }

      [XmlAttribute("Obsolete")]
      public bool Obsolete
      {
         get
         {
            return mObsolete;
         }
         set
         {
            mObsolete = value;
         }
      }

      [XmlAttribute("File")]
      public string File
      {
         get
         {
            return mFile;
         }
         set
         {
            mFile = value;
         }
      }
      public override string ToString()
      {
         return Name + " V" + this.Version + ((this.Obsolete)?" -Obsolete":"");
      }
   }
}
