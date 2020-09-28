using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Serialization;

using SimEditor;
using EditorCore;


namespace PhoenixEditor.ScenarioEditor
{


   public class TriggerNamespace
   {
      TriggerRoot mTriggerRoot;
      Dictionary<int, Trigger> mTriggers = new Dictionary<int,Trigger>();
      Dictionary<int, TriggerTemplateMapping> mTemplateMappings = new Dictionary<int, TriggerTemplateMapping>();
      Dictionary<int, TriggerCondition> mTriggerConditions = new Dictionary<int,TriggerCondition>();
      Dictionary<int, TriggerEffect> mTriggerEffects = new Dictionary<int, TriggerEffect>();
      Dictionary<int, TriggerEffect> mTriggerEffectsFalse = new Dictionary<int, TriggerEffect>();
      Dictionary<int, TriggerValue> mTriggerValues = new Dictionary<int, TriggerValue>();
      UniqueID mTriggerIDs = new UniqueID();
      UniqueID mConditionIDs = new UniqueID();
      UniqueID mEffectIDs = new UniqueID();
      UniqueID mValueIDs = new UniqueID();
      //UniqueID mTemplateIDs = new UniqueID();

      Dictionary<int, List<TriggerVariable>> mValueIDsToVars = new Dictionary<int, List<TriggerVariable>>();
      Dictionary<int, List<int>> mValueIDsToTriggers = new Dictionary<int, List<int>>();
      Dictionary<int, List<int>> mValueIDsToTemplates = new Dictionary<int, List<int>>();

      Dictionary<string, int> mValueNames = new Dictionary<string, int>();


      List<int> mGlobalVars = new List<int>();
      List<int> mSharedValueVars = new List<int>();
      List<int> mOrphans = new List<int>();


      //templates
      //List<TriggerTemplateMapping> mTemplates = new List<TriggerTemplateMapping>();

      XmlSerializer mTriggerRootSerializer = new XmlSerializer(typeof(TriggerRoot), new Type[] { });

      public Dictionary<int, TriggerTemplateMapping> getTemplateMappings()
      {
         return mTemplateMappings;
      }

      public TriggerTemplateMapping getTemplateMapping(int id)
      {
         if (id == -1)
            return null;
         TriggerTemplateMapping map = null;
         mTemplateMappings.TryGetValue(id, out map);
         return map;
      }

      public bool IsGlobalVar(int id)
      {
         if (mGlobalVars.Contains(id))
            return true;
         return false;
      }
      public bool IsSharedValueVar(int id)
      {
         if (mSharedValueVars.Contains(id))
            return true;
         return false;
      }

      public Dictionary<int, TriggerValue> GetValues()
      {

         return mTriggerValues;
      }
      public List<TriggerValue> GetValueList()
      {
         return mEditorData.TriggerSystem.TriggerVars.TriggerVar;
      }

      public void KillOrphanValues()
      {
         foreach(int id in mOrphans)
         {            
            if (RemoveValue(id) == false)
            {
               CoreGlobals.getErrorManager().OnSimpleWarning("Error clearing orphan var: " + id.ToString());
            }
         }
         mOrphans.Clear();
      }
      public bool RemoveValue(int id)
      {
         if(mTriggerValues.ContainsKey(id) == false)
         {
            return false;
         }

         mEditorData.TriggerSystem.TriggerVars.TriggerVar.Remove(mTriggerValues[id]);

         return mTriggerValues.Remove(id);
      }

      public List<int> QueryVars(string type, int triggerID)
      {
         List<int> valueIDs = new List<int>();
         Dictionary<int, TriggerValue>.Enumerator it = mTriggerValues.GetEnumerator();
         while(it.MoveNext())
         {
            if(type != "" && type != it.Current.Value.Type)
            {
               continue;
            }
            if(triggerID != -1 && !mValueIDsToTriggers[it.Current.Key].Contains(triggerID))
            {
               continue;
            }
            //Ignore constants where name=="" means const
            if (it.Current.Value.Name == null || it.Current.Value.Name == "")
            {
               continue;
            }
            valueIDs.Add(it.Current.Key);
         }
         return valueIDs;
      }

      //      //public void InsertTemplate()

      public void InsertTrigger(Trigger t, out int newID)
      {
         newID = mTriggerIDs.GetUniqueID();
         t.ID = newID;

         mTriggers[t.ID] = t;
         mEditorData.TriggerSystem.Trigger.Add(t);
         
      }


      public void InsertTemplateMapping(TriggerTemplateMapping mapping, out int newID)
      {
         newID = mTriggerIDs.GetUniqueID();      //mTemplateIDs.GetUniqueID();
         mapping.ID = newID;

         mEditorData.TriggerMappings.Add(mapping);

         mTemplateMappings[mapping.ID] = mapping;
      }
      public void DeleteTemplateMapping(TriggerTemplateMapping mapping)
      {
         mEditorData.TriggerMappings.Remove(mapping);
         mTemplateMappings.Remove(mapping.ID);
      }

      public void InsertCondition(int triggerID, TriggerCondition c, Dictionary<int, TriggerValue> tempValues, out int newID)
      {
         InsertCondition(triggerID, c, tempValues, out newID, null);
      }

      public void InsertCondition(int triggerID, TriggerCondition c, Dictionary<int, TriggerValue> tempValues, out int newID, TriggerCondition after)
      {
         newID = -1;
         if(mTriggers[triggerID].TriggerConditions.Child == null)
         {
            mTriggers[triggerID].TriggerConditions.Child = new TriggersTriggerAnd();
         }
         //TriggerCondition topCond = mTriggers[triggerID].TriggerConditions.Child as TriggerCondition;
         //if(topCond )

         TriggersTriggerAnd mainAnd = mTriggers[triggerID].TriggerConditions.Child as TriggersTriggerAnd;
         if(mainAnd != null)
         {
            newID = mConditionIDs.GetUniqueID();
            c.ID = newID;

            if (after == null)
            {
               mainAnd.Children.Add(c);
            }
            else
            {
               int index = mainAnd.Children.FindIndex(delegate(object val) { return ((TriggerCondition)val).ID == after.ID; });
               mainAnd.Children.Insert(index, c);
            }

            mTriggerConditions[c.ID] = c;
            InsertVariableList(triggerID,c.Parameter, tempValues);
         }
         TriggersTriggerOR mainOr = mTriggers[triggerID].TriggerConditions.Child as TriggersTriggerOR;
         if (mainOr != null)
         {
            newID = mConditionIDs.GetUniqueID();
            c.ID = newID;

            if (after == null)
            {
               mainOr.Children.Add(c);
            }
            else
            {
               int index = mainOr.Children.FindIndex(delegate(object val) { return val == after; });
               mainOr.Children.Insert(index, c);
            }

            mTriggerConditions[c.ID] = c;
            InsertVariableList(triggerID, c.Parameter, tempValues);
         }


      }


      public void InsertEffect(int triggerID, TriggerEffect e, Dictionary<int, TriggerValue> tempValues, bool onFalse, out int newID)
      {
         InsertEffect(triggerID, e, tempValues, onFalse, out newID, null);
      }

      public void InsertEffect(int triggerID, TriggerEffect e, Dictionary<int, TriggerValue> tempValues, bool onFalse, out int newID, TriggerEffect after)
      {
         newID = mEffectIDs.GetUniqueID();
         e.ID = newID;
         if (onFalse)
         {
            if (after == null)
            {
               mTriggers[triggerID].TriggerEffectsFalse.Effects.Add(e);
            }
            else
            {
               int index = mTriggers[triggerID].TriggerEffectsFalse.Effects.FindIndex(delegate(SimEditor.TriggerEffect val) { return val.ID == after.ID; });
               mTriggers[triggerID].TriggerEffectsFalse.Effects.Insert(index, e);
            }                    

            mTriggerEffectsFalse[e.ID] = e;
         }
         else
         {
            if (after == null)
            {
               mTriggers[triggerID].TriggerEffects.Effects.Add(e);
            }
            else
            {
               int index = mTriggers[triggerID].TriggerEffects.Effects.FindIndex(delegate(SimEditor.TriggerEffect val) { return val.ID == after.ID; });
               mTriggers[triggerID].TriggerEffects.Effects.Insert(index, e);
            }        

            mTriggerEffects[e.ID] = e;
         }

         InsertVariableList(triggerID, e.Parameter, tempValues);

      }

      static public string GetDefaultVarSuffix()
      {
         return "Var";
      }


      


      public void InsertValue(TriggerValue v, out int newID)
      {
         newID = mValueIDs.GetUniqueID();
         mTriggerValues[newID] = v;
         v.ID = newID;

         int offset = 1;

         if (v.Name != "")
         {         
            string baseName = v.Name;
            int suffixIndx = baseName.IndexOf(TriggerNamespace.GetDefaultVarSuffix());
            if(suffixIndx > 0)
               baseName = baseName.Substring(0, suffixIndx);

            while (mValueNames.ContainsKey(baseName + TriggerNamespace.GetDefaultVarSuffix() + (offset).ToString()))
            {
               offset++;
            }
            
            v.Name = baseName + TriggerNamespace.GetDefaultVarSuffix() + offset.ToString();

            mValueNames[v.Name] = 1;
         }

         mEditorData.TriggerSystem.TriggerVars.TriggerVar.Add(v);

      }

      public void InsertVariableList(int triggerID, List<TriggerVariable> varlist, Dictionary<int, TriggerValue> values)
      {
         if (values == null)
            return;

         Dictionary<int, TriggerValue> tempValues = new Dictionary<int, TriggerValue>();
         foreach (TriggerVariable v in varlist)
         {          
            int newID;
            if (values.ContainsKey(v.ID) == false) 
               continue;
            
            TriggerValue copyVal = values[v.ID].GetCopy();

            InsertValue(copyVal, out newID);
            //InsertValue(values[v.ID], out newID);
            tempValues[newID] = copyVal;
            v.ID = newID;

            RegisterVar(triggerID,v);

         }
         
         //rebuild hash
         //values.Clear();

         //add to hash
         Dictionary<int, TriggerValue>.Enumerator it = tempValues.GetEnumerator();
         while (it.MoveNext())
         {
            //values.Add(it.Current.Key, it.Current.Value);
            values[it.Current.Key] = it.Current.Value;
         }         
      }

      public void SwitchVarToConstant( TriggerVariable var, ref TriggerValue val)
      {

         if(IsSharedValueVar(var.ID))
         {
            //make new value 
            val = val.GetCopy();
            int newID;
            InsertValue(val, out newID);
            val.ID = newID;
            var.ID = newID;
         }
         val.Name = "";

         ProcessVarMapping();
      }
      public void SwitchConstantToVar( TriggerVariable var, ref TriggerValue val)
      {
         if (IsSharedValueVar(var.ID))
         {
            //make new value 
            val = val.GetCopy();
            int newID;
            InsertValue(val, out newID);
            val.ID = newID;
            var.ID = newID;
         }
         //val.Name = "";

         if(val.Name == "")
         {
            val.Name = var.Name + TriggerNamespace.GetDefaultVarSuffix() + val.ID.ToString();
         }

         ProcessVarMapping();

      }

      //public void AssignValueToVar(TriggerVariable var, TriggerValue val )
      //{

      //   var.ID = val.ID;
 
      //   ProcessVarMapping();

      //}
      public TriggerValue AssignValueToVar( TriggerVariable var, int newID)
      {
         TriggerValue val = mTriggerValues[newID];
         var.ID = newID;
         ProcessVarMapping();

         //RegisterVar(triggerID, var);


         return val;
      }

      public void DeleteTrigger(Trigger t)
      {
         mEditorData.TriggerSystem.Trigger.Remove(t);
         mTriggers.Remove(t.ID);

         List<TriggerCondition> conditions = WalkConditions(t.TriggerConditions.Child);
         foreach (TriggerCondition cond in conditions)
         {
            mTriggerConditions.Remove(cond.ID);
         }
         foreach (TriggerEffect effect in t.TriggerEffects.Effects)
         {
            mTriggerEffects.Remove(effect.ID);
         }
         foreach (TriggerEffect effect in t.TriggerEffectsFalse.Effects)
         {
            mTriggerEffectsFalse.Remove(effect.ID);
         }
    
         ProcessVarMapping();
      }
      public bool DeleteCondition(Trigger t, TriggerCondition condition)
      {
         return DeleteCondition(t, condition, false);
      }

      public bool DeleteCondition(Trigger t, TriggerCondition condition, bool basic)
      {
         bool result = false;

         if(!basic)
            mTriggerConditions.Remove(condition.ID);

         if (t.TriggerConditions.Child == condition)
         {
            t.TriggerConditions.Child = null;
            result = true;

         }
         else
         {
            object parent = GetConditionNodeParent(t.TriggerConditions.Child, condition);
            if (parent == null)
            {
               CoreGlobals.getErrorManager().OnSimpleWarning("Error removing condition " + condition.ID.ToString() + " from trigger " + t.Name + " : " + t.ID.ToString() + " No parent found!");
               result = false;
            }

            if (parent is TriggersTriggerAnd)
            {
               TriggersTriggerAnd andCond = (TriggersTriggerAnd)parent;
               result = andCond.Children.Remove(condition);

            }
            if (parent is TriggersTriggerOR)
            {
               TriggersTriggerOR orCond = (TriggersTriggerOR)parent;
               result = orCond.Children.Remove(condition);

            }
         }
         if (!basic)
            ProcessVarMapping();
         return result;
      }

      public bool DeleteEffect(Trigger t, TriggerEffect effect, bool onFalse)
      {
         return DeleteEffect(t, effect, onFalse, false);
      }

      public bool DeleteEffect(Trigger t, TriggerEffect effect, bool onFalse, bool basic)
      {
         bool result = false;
         if (onFalse)
         {
            mTriggerEffectsFalse.Remove(effect.ID);
         }
         else
         {
            mTriggerEffects.Remove(effect.ID);
         }
         if (!basic)
            ProcessVarMapping();

         if (onFalse)
         {
            result = t.TriggerEffectsFalse.Effects.Remove(effect);
         }
         else
         {
            result = t.TriggerEffects.Effects.Remove(effect);
         }
         return result;
      }


      public bool ReOrderConditions(Trigger t, List<TriggerCondition> conditions)
      {
         if (conditions.Count == 0)
            t.TriggerConditions.Child = null;
         if (conditions.Count == 1)
         {
            //t.TriggerConditions.Child = conditions[0];
            TriggersTriggerAnd andlist = new TriggersTriggerAnd();
            t.TriggerConditions.Child = andlist;
            andlist.Children.Add(conditions[0]);
         }
         else
         {
            TriggersTriggerOR orlist = t.TriggerConditions.Child as TriggersTriggerOR;
            TriggersTriggerAnd andlist = t.TriggerConditions.Child as TriggersTriggerAnd;

            if (orlist != null)
            {
               orlist.Children.Clear();
               orlist.Children.AddRange(conditions.ToArray());
            }
            if (andlist != null)
            {
               andlist.Children.Clear();
               andlist.Children.AddRange(conditions.ToArray());

            }
         }
         return true;
      }
      public bool ReOrderEffects(Trigger t, List<TriggerEffect> effects, bool onFalse)
      {
         List<TriggerEffect> dontTouch = new List<TriggerEffect>();

         List<TriggerEffect> current;
         if (onFalse == false)
            current = t.TriggerEffects.Effects;
         else
            current = t.TriggerEffectsFalse.Effects;
         foreach (TriggerEffect e in current)
         {
            if((e.Type ==  TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName())
            || (e.Type ==  TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName()))
            {
               dontTouch.Add(e);
            }
         }

         if(onFalse == false)
         {
            t.TriggerEffects.Effects.Clear();
            t.TriggerEffects.Effects.AddRange( effects.ToArray() );
            t.TriggerEffects.Effects.AddRange(dontTouch);
         }
         else
         {
            t.TriggerEffectsFalse.Effects.Clear();
            t.TriggerEffectsFalse.Effects.AddRange(effects.ToArray());
            t.TriggerEffectsFalse.Effects.AddRange(dontTouch);

         }
         return true;
      }



      //public void clear unused values...

      //InsertRoot .. merge an entire system into this one.
               //entry and exit points must match up good


      //TriggerRoot mEditorRoot = null;
      TriggerEditorData mEditorData = null;

      public TriggerRoot TriggerData
      {
         set
         {
            ClearData();
            mTriggerRoot = value;
            if (mTriggerRoot.TriggerEditorData != null && mTriggerRoot.TriggerEditorData.TriggerSystem != null)
            {
               mEditorData = TriggerEditorData;
               ProcessEditorData(mEditorData);
            }
            else if (mTriggerRoot.Trigger.Count > 0  ) //oldpowers
            {
               mEditorData.TriggerSystem = mTriggerRoot;
               mTriggerRoot = new TriggerRoot();
               mTriggerRoot.TriggerEditorData = new TriggerEditorData();
               mTriggerRoot.TriggerEditorData.TriggerSystem = mEditorData.TriggerSystem;
               ProcessEditorData(mEditorData);


            }
            else //empty
            {
               mEditorData = new TriggerEditorData();
               mEditorData.TriggerSystem = new TriggerRoot();
               mTriggerRoot.TriggerEditorData = mEditorData;
            }
         }
         get
         {

            mEditorData.TriggerSystem.NextTriggerID = mTriggerIDs.NextID();
            mEditorData.TriggerSystem.NextConditionID = mConditionIDs.NextID();
            mEditorData.TriggerSystem.NextEffectID = mEffectIDs.NextID();
            mEditorData.TriggerSystem.NextTriggerVarID = mValueIDs.NextID();
            //mEditorData.TriggerSystem.NextTemplateID = mValueIDs.NextID();
            RenderOutputData();

            WriteGroupData(mTriggerRoot);

            CleanBadLinks(mTriggerRoot);
            CleanBadLinks(mTriggerRoot.TriggerEditorData.TriggerSystem);
            CleanBadValues(mTriggerRoot);

            //FinalBake(mTriggerRoot);


            return mTriggerRoot;
         }
      }
     
      public TriggerEditorData TriggerEditorData
      {
         get
         {
            return mTriggerRoot.TriggerEditorData;
         }

      }

      protected void WriteGroupData(TriggerRoot root)
      {
         Dictionary<int, List<int>> triggersbyGroup = new Dictionary<int, List<int>>();
         foreach(Trigger t in root.Trigger)
         {
            if (t.GroupID == -1)
               continue;
            if(triggersbyGroup.ContainsKey(t.GroupID) == false)
            {
               triggersbyGroup[t.GroupID] = new List<int>();
            }
            triggersbyGroup[t.GroupID].Add(t.ID);           
         }

         Dictionary<int, GroupUI> groups = new Dictionary<int, GroupUI>();
         foreach (GroupUI groupUI in root.TriggerEditorData.UIData.mGroups)
         {
            groups[groupUI.InternalGroupID] = groupUI;
         }



         foreach (GroupUI groupUI in root.TriggerEditorData.UIData.mGroups)
         {
            GameGroupInfo info = new GameGroupInfo();
            info.ID = groupUI.InternalGroupID;
            info.Name = groupUI.Title;
            root.GroupInfo.Add(info);
            List<int> children = new List<int>();

            if (triggersbyGroup.ContainsKey(info.ID) == false)
               continue; // no triggers in this group

            children.AddRange(triggersbyGroup[info.ID]);

            foreach (GroupUI otherGroupUI in root.TriggerEditorData.UIData.mGroups)
            {
               int child = otherGroupUI.InternalGroupID;
               int id = child;
               int parent = groupUI.InternalGroupID;
               int lastid = id;
               if (id == -1)
                  continue; 

               while (id != -1)
               {
                  id = groups[id].GroupID;
                  if (id == parent)
                  {
                     if (triggersbyGroup.ContainsKey(child))
                     {
                        children.AddRange(triggersbyGroup[child]);
                     }
                     continue;
                  }
                  if (lastid == id)
                     continue;
                  lastid = id;
               }
            

            }

            foreach (int i in children)
            {
               if (info.Children != "")
                  info.Children += ",";
               info.Children += i.ToString();
            }

         }


      }

      //protected void 

      protected void CleanBadValues(TriggerRoot root)
      {
         Dictionary<int, TriggerValue> tempTriggerValueMap = new Dictionary<int, TriggerValue>();
         List<TriggerValue> duplicates = new List<TriggerValue>();
         foreach (TriggerValue v in root.TriggerVars.TriggerVar)
         {
            if (tempTriggerValueMap.ContainsKey(v.ID))
            {
               duplicates.Add(v);
            }
            tempTriggerValueMap[v.ID] = v;
         }
         foreach (TriggerValue v in duplicates)
         {
            root.TriggerVars.TriggerVar.Remove(v);
         }         
      }

      protected void CleanBadLinks(TriggerRoot root)
      {
         //root.
         //this.WalkConditions

         List<TriggerEffect> links = new List<TriggerEffect>();
         Dictionary<int, Trigger> tempTriggerMap = new Dictionary<int, Trigger>();
         Dictionary<int, TriggerValue> tempTriggerValueMap = new Dictionary<int, TriggerValue>();
         foreach (Trigger t in root.Trigger)
         {
            tempTriggerMap[t.ID] = t;
         }         
         foreach(TriggerValue v in root.TriggerVars.TriggerVar)
         {
            tempTriggerValueMap[v.ID] = v;
         }
         List<TriggerEffect> toremove = new List<TriggerEffect>();
         foreach(Trigger t in root.Trigger)
         {
            if(t.TriggerEffects != null)
            {
               toremove.Clear();
               foreach(TriggerEffect e in t.TriggerEffects.Effects)
               {
                  if( TriggerSystemMain.mTriggerDefinitions.IsLink(e) )
                  {

                     if (tempTriggerValueMap[e.Parameter[0].ID].Value == null)
                     {
                        toremove.Add(e);
                        continue; 
                        //In this situation the template will probably need to be swapped out anyhow, but this will keep it from crashing untill then

                     }
                     
                     int targetid = int.Parse(tempTriggerValueMap[e.Parameter[0].ID].Value.ToString());
                     if(tempTriggerMap.ContainsKey(targetid) == false)
                     {
                        toremove.Add(e);
                     }
                  }
               }
               foreach(TriggerEffect e in toremove )
               {
                  t.TriggerEffects.Effects.Remove(e);
               }
            }
            if(t.TriggerEffectsFalse != null)
            {
               toremove.Clear();
               foreach (TriggerEffect e in t.TriggerEffectsFalse.Effects)
               {
                  if (TriggerSystemMain.mTriggerDefinitions.IsLink(e))
                  {
                     int targetid = int.Parse(tempTriggerValueMap[e.Parameter[0].ID].Value.ToString());
                     if (tempTriggerMap.ContainsKey(targetid) == false)
                     {
                        toremove.Add(e);
                     }
                  }
               }
               foreach (TriggerEffect e in toremove)
               {
                  t.TriggerEffectsFalse.Effects.Remove(e);
               }
            }

         }
      

      }

      public void RenderOutputData()
      {

         MemoryStream s = new MemoryStream();

         TriggerRoot backupCopy;
         //xml copy....TriggerEditorData.TriggerSystem
         mTriggerRootSerializer.Serialize(s, mTriggerRoot);  
         
         //no need to xmb this !!

         s.Seek(0, SeekOrigin.Begin);
         backupCopy = (TriggerRoot)mTriggerRootSerializer.Deserialize(s);

         TriggerNamespace renderNamespace = new TriggerNamespace();
         renderNamespace.TriggerData = backupCopy;// mTriggerRoot;

         try
         {

            renderNamespace.InternalDataBake();
         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);

         }

         this.mDebugInfo.Clear();
         this.mDebugInfo.AddRange(renderNamespace.mDebugInfo);

         mTriggerRoot = backupCopy.TriggerEditorData.TriggerSystem;
         mTriggerRoot.TriggerEditorData = mEditorData;



         //foreach() 

      }



      //creates and inserts trigger system one mapping at a time.
      public void InternalDataBake()
      {
         //nuke commented out triggers.
         List<Trigger> HitList = new List<Trigger>();
         foreach (Trigger t in this.mEditorData.TriggerSystem.Trigger)
         {
            if (t.CommentOut == true)
            {
               HitList.Add(t);
               continue;
            }
            //Commented Condtions?
            List<TriggerCondition> CondHitList = new List<TriggerCondition>();
            foreach (TriggerCondition c in WalkConditions(t.TriggerConditions.Child))
               if (c.CommentOut == true)
                  CondHitList.Add(c);
            //GetConditionNodeParent
            foreach (TriggerCondition c in CondHitList)
               DeleteCondition(t, c, true);
            //commented effects?
            List<TriggerEffect> EffHitList = new List<TriggerEffect>();
            foreach(TriggerEffect e in t.TriggerEffects.Effects)
               if(e.CommentOut == true)
                  EffHitList.Add(e);
            foreach (TriggerEffect e in EffHitList)
               t.TriggerEffects.Effects.Remove(e);
            //Commented effects on false?
            EffHitList.Clear();
            foreach (TriggerEffect e in t.TriggerEffectsFalse.Effects)
               if (e.CommentOut == true)
                  EffHitList.Add(e);
            foreach (TriggerEffect e in EffHitList)
               t.TriggerEffectsFalse.Effects.Remove(e);
         }
         foreach (Trigger t in HitList)
         {
            this.mEditorData.TriggerSystem.Trigger.Remove(t);
         }
        
         foreach (TriggerValue v in mEditorData.TriggerSystem.TriggerVars.TriggerVar)
         {
            this.InternalBakeValue(v);
         }         
         if(mEditorData.TriggerMappings != null)
         {
            Dictionary<TriggerTemplateMapping,Dictionary<int, int>> triggerTranslationByMapping = new Dictionary<TriggerTemplateMapping,Dictionary<int,int>>();
            Dictionary<TriggersTemplateActionBinder, TriggerTemplateMapping> binderToMapping = new Dictionary<TriggersTemplateActionBinder, TriggerTemplateMapping>();
            //Merge in the trigger data
            List<TriggerTemplateMapping> commentedOutTemplates = new List<TriggerTemplateMapping>();
            foreach (TriggerTemplateMapping m in mEditorData.TriggerMappings)
            {
               if (m.CommentOut == true)
               {
                  commentedOutTemplates.Add(m);
                  continue;
               }

               TriggerTemplateDefinition d = TriggerSystemMain.mTriggerDefinitions.GetTemplateDefinition(m.Name);

               //if(d.TriggerTemplateMapping.DoNotUse == true)
               //{
               //   mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.DoNotUse, null, "Do not use ", d));

               //}
               //if(d.TriggerTemplateMapping.Obsolete == true)
               //{
               //   mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.ObsoleteVersion, null, "Obsolete ", d));

               //}


               Dictionary<int, int> valueTranslation = new Dictionary<int, int>();
               Dictionary<int, int> triggerTranslation = new Dictionary<int, int>();

               Dictionary<int, int> replacementMappings = new Dictionary<int,int>();

               triggerTranslationByMapping[m] = triggerTranslation;

               foreach (TriggersTemplateInputVariable v in m.InputMappings)
               { 
                  replacementMappings[v.BindID] = v.ID;
               }
               foreach (TriggersTemplateOutputVariable v in m.OutputMappings)
               { 
                  replacementMappings[v.BindID] = v.ID;
               }
               if (d != null)
               {
                  Dictionary<int, TriggerValue> values = new Dictionary<int,TriggerValue>();
                  foreach (TriggerValue v in d.TriggerSystemRoot.TriggerVars.TriggerVar)
                  {
                     //we have a replacement...
                     if (replacementMappings.ContainsKey(v.ID))
                        continue;

                     int newID = 0;
                     int oldID = v.ID;
                     TriggerValue newVal = new TriggerValue();
                     v.CopyTo(newVal);

                     InternalBakeValue(newVal);

                     InsertValue(newVal,out newID);
                     valueTranslation[oldID] = newID;
                     values[newID] = newVal;
                    
                  }

                  foreach(Trigger t1 in d.TriggerSystemRoot.Trigger)
                  {
                     int newID = 0;
                     int oldID = t1.ID;
                     Trigger newTrig = new Trigger();                    
                     t1.DeepCopyTo(newTrig);
                     InsertTrigger(newTrig, out newID);
                     triggerTranslation[oldID] = newID;

                     newTrig.TemplateID = m.ID;

                     //process trigger now...?
                     List<TriggerCondition> conditions = WalkConditions(newTrig.TriggerConditions.Child);
                     foreach (TriggerCondition cond in conditions)
                     {
                        foreach(TriggerVariable v in cond.Parameter)
                        {
                           int key = -1;

                           if (replacementMappings.ContainsKey(v.ID))
                           {
                              key = replacementMappings[v.ID];
                           }
                           else
                           {
                              key = valueTranslation[v.ID];
                           }

                           v.ID = key;
                        }
                        newID = mConditionIDs.GetUniqueID();
                        cond.ID = newID;
                     }
                     foreach (TriggerEffect effect in newTrig.TriggerEffects.Effects)
                     {
                        foreach (TriggerVariable v in effect.Parameter)
                        {
                           int key = -1;

                           if (replacementMappings.ContainsKey(v.ID))
                           {
                              key = replacementMappings[v.ID];
                           }
                           else
                           {
                              key = valueTranslation[v.ID];
                           }
                           v.ID = key;

                        }
                        newID = mEffectIDs.GetUniqueID();
                        effect.ID = newID;
                     }
                     foreach (TriggerEffect effect in newTrig.TriggerEffectsFalse.Effects)
                     {
                        foreach (TriggerVariable v in effect.Parameter)
                        {
                           int key = -1;

                           if (replacementMappings.ContainsKey(v.ID))
                           {
                              key = replacementMappings[v.ID];
                           }
                           else
                           {
                              key = valueTranslation[v.ID];
                           }
                           v.ID = key;

                        }
                        newID = mEffectIDs.GetUniqueID();
                        effect.ID = newID;
                     }

                     foreach (TriggersTemplateInputActionBinder input in m.TriggerInputs)
                     {
                        binderToMapping[input] = m;
                     }
   
                     foreach (TriggersTemplateOutputActionBinder output in m.TriggerOutputs)
                     {
                        binderToMapping[output] = m;
                     }
                  }

                  //Translate trigger link values to the new system
                  Dictionary<int, TriggerValue>.Enumerator itNewVals = values.GetEnumerator();
                  while (itNewVals.MoveNext())
                  {
                     if (itNewVals.Current.Value.Type == TriggerSystemMain.mTriggerDefinitions.GetTriggerType())
                     {
                        int oldLink = System.Convert.ToInt32(itNewVals.Current.Value.Value);
                        
                        int newLink;
                        if (triggerTranslation.TryGetValue(oldLink, out newLink))
                        {
                           itNewVals.Current.Value.Value = newLink.ToString();
                        }
                        else
                        {
                           CoreGlobals.getErrorManager().OnSimpleWarning("Error Remapping linke to trigger " + oldLink.ToString() + " " + m.Name + " in " + this.GetGroupName(m.GroupID));
                           
                        }
                     }
                  }

               }

            }
            //Resolve links
            foreach (TriggerTemplateMapping m in mEditorData.TriggerMappings)
            {
               if (m.CommentOut == true)
                  continue;

               //target -> bindID
               foreach (TriggersTemplateInputActionBinder inputBinder in m.TriggerInputs)
               {
                  foreach (TriggerBindInfo target in inputBinder.TargetIDs)
                  {
                     try
                     {                     
                        int triggerA = inputBinder.BindID.ID;
                        TriggerTemplateMapping mapping = binderToMapping[inputBinder];
                        triggerA = triggerTranslationByMapping[mapping][triggerA];
                        int triggerB = target.ID;
                        bool onfalse = (target.LinkName == "Effect.False") ? true : false;

                        if (mTriggers.ContainsKey(triggerB) == true)
                        {
                           NewLink(inputBinder.BindID.LinkName, triggerB, triggerA, onfalse);
                        }
                     }
                     catch (System.Exception ex)
                     {
                        try
                        {
                           string debugInfo = "Template " + m.Name;
                           GroupUI owningGroup;
                           if (this.GetGroups().TryGetValue(m.GroupID, out owningGroup))
                           {
                              debugInfo = debugInfo + " in group: " + owningGroup.Title;
                           }
                           debugInfo = debugInfo + " has an error being linked to by trigger " + target.ID.ToString();

                           string ext = ex.ToString();

                           CoreGlobals.getErrorManager().OnSimpleWarning(debugInfo);

                        }
                        catch (System.Exception ex2)
                        {
                           CoreGlobals.getErrorManager().OnSimpleWarning(ex2.ToString());
                        }
                     }
                  }

               }

               //bindID -> target
               foreach (TriggersTemplateOutputActionBinder outputBinding in m.TriggerOutputs)
               {
                  if (m.CommentOut == true)
                     continue;

                  foreach (TriggerBindInfo b in outputBinding.TargetIDs)
                  {
                     try
                     {
                        int triggerA = outputBinding.BindID.ID;
                        Dictionary<int, int> mapping = triggerTranslationByMapping[m];
                        triggerA = mapping[triggerA];

                        //since we could be pointing to a template and not a trigger we need to search for a valid target
                        TriggerBindInfo finalLink = SearchForTriggerBinding(b, triggerTranslationByMapping, binderToMapping);
                        int triggerB = finalLink.ID;
                        string finalLinkName = finalLink.LinkName;

                        bool onfalse = (outputBinding.BindID.LinkName == "Effect.False") ? true : false ;
                        if (mTriggers.ContainsKey(triggerA) == true)   
                        {
                           NewLink(finalLinkName, triggerA, triggerB, onfalse);      
                        }
                     }
                     catch (System.Exception ex)
                     {
                        try
                        {
                           string debugInfo = "Template " + m.Name;
                           GroupUI owningGroup;
                           if (this.GetGroups().TryGetValue(m.GroupID, out owningGroup))
                           {
                              debugInfo = debugInfo + " in group: " + owningGroup.Title;
                           }
                           debugInfo = debugInfo + " has an error being linked from by trigger " + b.ID.ToString();

                           string ext = ex.ToString();

                           CoreGlobals.getErrorManager().OnSimpleWarning(debugInfo);
                        }
                        catch (System.Exception ex2)
                        {
                           CoreGlobals.getErrorManager().OnSimpleWarning(ex2.ToString());
                        }
                     }
                  }
               }
            }
         }
      }

     
      private void InternalBakeValue(TriggerValue val)
      {
         StringDecorator d;
         if (val.Type == "VectorList" && val.Value != null)
         {
            string list = "";
            string[] ids = val.Value.Split('|');
            foreach (string locationString in ids)
            {
               object value;
               if (list != "")
                  list += "|";
               object o;

               if (StringDecorator.TryParse(locationString, out d) == true)
               {
                  if (SimGlobals.getSimMain().TryParseObjectPropertyRef(d.mDecorator, out o) == true)
                  {
                     string loc = o.ToString();
                     list += o.ToString();
                  }
               }
               else
               {
                  list += locationString;
               }
            }
            val.Value = list;
         }
         else if(StringDecorator.TryParse(val.Value, out d) == true)
         {
            object o;

            //validate squad
            if (val.Type == "SquadList")
            {
               string list = "";
               string[] ids = d.mDecorator.Split(',');
               foreach (string idstring in ids)
               {
                  object value;
                  int id;
                  string propname;

                  if (list != "")
                     list += ",";

                  if (int.TryParse(idstring, out id))
                  {
                     EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
                     //SimGlobals.getSimMain().addSelectedObject(obj);
                     SimObjectData data = obj.GetProperties() as SimObjectData;
                     if (data != null && data.IsSquad == true)
                     {
                        list += data.ID;
                     }
                     else
                     {
                        //nothing
                     }
                  }
               }
            }
            //validate unitlist
            else if (val.Type == "UnitList")
            {
               string list = "";               
               string[] ids = d.mDecorator.Split(',');
               foreach (string idstring in ids)
               {
                  object value;
                  int id;
                  string propname;

                  if (list != "")
                     list += ",";

                  if (int.TryParse(idstring, out id))
                  {
                     EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
                     //SimGlobals.getSimMain().addSelectedObject(obj);
                     SimObjectData data = obj.GetProperties() as SimObjectData;
                     if (data != null && data.IsSquad == false)
                     {
                        list += data.ID;
                     }
                     else
                     {
                        //nothing
                     }
                  }
               }
            }
            //else if (val.Type == "Squad")
            //{
            //   int id;
            //   string sID = d.mDecorator;
            //   if (int.TryParse(sID, out id))
            //   {
            //      EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
            //      if (obj != null)
            //      {
            //         SimObjectData data = obj.GetProperties() as SimObjectData;
            //         if (data != null && data.IsSquad == true)
            //         {
            //            val.Value = d.mDecorator;
            //         }
            //      }
            //   }
            //}
            else if (SimGlobals.getSimMain().TryParseObjectPropertyRef(d.mDecorator, out o) == true)
            {
               val.Value = o.ToString();
            }
            else
            {

               val.Value = d.mDecorator;

            }

         }




      }

      //Take binding to a template and resolve the information to find the actual trigger that should be bound to
      private TriggerBindInfo SearchForTriggerBinding(TriggerBindInfo start, Dictionary<TriggerTemplateMapping, Dictionary<int, int>> triggerTranslationByMapping, Dictionary<TriggersTemplateActionBinder, TriggerTemplateMapping> binderToMapping)
      {
         TriggerBindInfo found = new TriggerBindInfo();
         if (start.IsLinkToTemplate() == false)
            return start;

         TriggerTemplateMapping targetMapping = mTemplateMappings[start.ID];

         TriggersTemplateInputActionBinder ib = targetMapping.TriggerInputs.Find(delegate(TriggersTemplateInputActionBinder s) { return (bool)(s.Name == start.LinkName); });
         if (ib != null)
         {
            string linkName = ib.BindID.LinkName;
            int translatedID = triggerTranslationByMapping[targetMapping][ib.BindID.ID];
            found.SetTarget(translatedID, linkName);

         }

         //do we ever need to look out outputs?   Nope templates resolve in one direction only
         //TriggersTemplateOutputActionBinder ob = targetMapping.TriggerInputs.Find(delegate(TriggersTemplateInputActionBinder s) { return (bool)(s.Name == start.LinkName); });

         //binderToMapping[start]
         //start.ID
         return found;
      }
      
      public void NewLink(string name, int triggerA, int triggerB, bool onfalse)
      {
         //string activate = TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName();

         TriggerEffect effect;
         Dictionary<int, TriggerValue> values;

         if (name == "Activate")
         {
            TriggerSystemMain.mTriggerDefinitions.GetTriggerEffect(TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName(), out effect, out values);
         }
         else if (name == "Deactivate")
         {
            TriggerSystemMain.mTriggerDefinitions.GetTriggerEffect(TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName(), out effect, out values);
         }
         else
         {

            return;
         }

         int newID;     

         this.InsertEffect(triggerA, effect, values, onfalse, out newID);

         mTriggerValues[effect.Parameter[0].ID].Value = triggerB.ToString();

         //create effect in trigger a,  link it up to trigger b
         //how to update controls?
      }
      public void DeleteLink(string name, int triggerA, int triggerB)
      {
         //should link be an object that each cp points to?
      }

      public void ClearData()
      {
         mTriggerIDs = new UniqueID();
         mConditionIDs = new UniqueID();
         mEffectIDs = new UniqueID();
         mValueIDs = new UniqueID();
         //mTemplateIDs = new UniqueID();
         mTriggers.Clear();
         mTriggerConditions.Clear();
         mTriggerEffects.Clear();
         mTriggerEffectsFalse.Clear();
         mTriggerValues.Clear();

         mValueIDsToVars.Clear();
         mValueIDsToTriggers.Clear();
         mValueNames.Clear();


         mGlobalVars.Clear();
         mSharedValueVars.Clear();
         mOrphans.Clear();
      }

      public List<TriggerSystemDebugInfo> mDebugInfo = new List<TriggerSystemDebugInfo>();

      public Dictionary<int, bool> mActivatedTriggers = new Dictionary<int, bool>();
      public Dictionary<int, bool> mInitializedValues = new Dictionary<int, bool>();


      List<int> mListOfBoundObjects = new List<int>();

      public List<TriggerSystemDebugInfo> ProcessValue(TriggerValue value, bool scenarioDataAvailable)
      {
         List<TriggerSystemDebugInfo> debugInfo = new List<TriggerSystemDebugInfo>();

         /////////Values that need a scenario to check:
         //////////////////////////////////////////////
         if (scenarioDataAvailable && value.Type == "Squad" )
         {
            if (value.Value != null && value.Value != "")
            {
               string sID = value.Value;
               int id;
               if (int.TryParse(sID, out id))
               {
                  ProcessValueSquad(value, debugInfo, id, mListOfBoundObjects);
               }
            }
         }
         else if (scenarioDataAvailable && value.Type == "SquadList")
         {
            if (value.Value != null && value.Value != "")
            {
               string[] ids = value.Value.Split(',');
               foreach(string sID in ids)
               {
                  int id;
                  if (int.TryParse(sID, out id))
                  {
                     ProcessValueSquad(value, debugInfo, id, mListOfBoundObjects);
                  }
               }
            }
         }
         else if (scenarioDataAvailable && value.Type == "Unit")
         {
            if (value.Value != null && value.Value != "")
            {
               string sID = value.Value;
               int id;
               if (int.TryParse(sID, out id))
               {
                  ProcessValueUnit(value, debugInfo, id, mListOfBoundObjects);
               }
            }
         }
         else if (scenarioDataAvailable && value.Type == "UnitList")
         {
            if (value.Value != null && value.Value != "")
            {
               string[] ids = value.Value.Split(',');
               foreach (string sID in ids)
               {
                  int id;
                  if (int.TryParse(sID, out id))
                  {
                     ProcessValueUnit(value, debugInfo, id, mListOfBoundObjects);
                  }
               }
            }
         }
         else if (scenarioDataAvailable && value.Type == "Object")
         {
            if (value.Value != null && value.Value != "")
            {
               string sID = value.Value;
               int id;
               if (int.TryParse(sID, out id))
               {
                  ProcessValueObject(value, debugInfo, id, mListOfBoundObjects);
               }
            }
         }
         else if (scenarioDataAvailable && value.Type == "ObjectList")
         {
            if (value.Value != null && value.Value != "")
            {
               string[] ids = value.Value.Split(',');
               foreach (string sID in ids)
               {
                  int id;
                  if (int.TryParse(sID, out id))
                  {
                     ProcessValueObject(value, debugInfo, id, mListOfBoundObjects);
                  }
               }
            }
         }

         //Values that do not need the scenario/////
         //////////////////////////////////////////
         if (value.Type == "ObjectType")
         {
            ProcessValueEnumeratedGameType(value, debugInfo, TriggerSystemMain.mSimResources.mObjectTypeData.mObjectTypeList);
         }
         else if (value.Type == "ProtoSquad")
         {
            ProcessValueEnumeratedGameType(value, debugInfo, TriggerSystemMain.mSimResources.mProtoSquadData.mProtoSquadList);
         }
         else if (value.Type == "ProtoObject")
         {
            ProcessValueEnumeratedGameType(value, debugInfo, TriggerSystemMain.mSimResources.mProtoObjectData.mProtoObjectList);
         }
         else if (value.Type == "Tech")
         {
            ProcessValueEnumeratedGameType(value, debugInfo, TriggerSystemMain.mSimResources.mTechData.mTechList);
         }
         

         //Lists of values //////////////////////////
         ////////////////////////////////////////////
         if (value.Type == "ProtoObjectList")
         {
            ProcessValueEnumeratedGameTypeList(value, debugInfo, TriggerSystemMain.mSimResources.mProtoObjectData.mProtoObjectList);
         }
         else if (value.Type == "ProtoObjectCollection")
         {
            ProcessValueEnumeratedGameTypeList(value, debugInfo, TriggerSystemMain.mSimResources.mProtoObjectData.mProtoObjectList);
         }
         else if (value.Type == "ObjectTypeList")
         {
            ProcessValueEnumeratedGameTypeList(value, debugInfo, TriggerSystemMain.mSimResources.mObjectTypeData.mObjectTypeList);
         }
         else if (value.Type == "ProtoSquadList")
         {
            ProcessValueEnumeratedGameTypeList(value, debugInfo, TriggerSystemMain.mSimResources.mProtoSquadData.mProtoSquadList);
         }
         else if (value.Type == "ProtoSquadCollection")
         {
            ProcessValueEnumeratedGameTypeList(value, debugInfo, TriggerSystemMain.mSimResources.mProtoSquadData.mProtoSquadList);
         }
         else if (value.Type == "TechList")
         {
            ProcessValueEnumeratedGameTypeList(value, debugInfo, TriggerSystemMain.mSimResources.mTechData.mTechList);
         }


         return debugInfo;
      }
      void ProcessValueEnumeratedGameTypeList(TriggerValue value, List<TriggerSystemDebugInfo> debugInfo, ICollection<string> values)
      {
         if (value.Value != null && value.Value != "")
         {
            string[] items = value.Value.Split(',');
            foreach (string item in items)
            {
               if((item != "") && (values.Contains(item) == false))
               {
                  debugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.ValueError, null, value.Type + " has a value that is not in the game: " + item, value));
               }               
            }
         }
      }

      void ProcessValueEnumeratedGameType(TriggerValue value, List<TriggerSystemDebugInfo> debugInfo, ICollection<string> values)
      {
         if (value.Value != null && value.Value != "")
         {
            if (values.Contains(value.Value) == false)
            {
               debugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.ValueError, null, value.Type + " has a value that is not in the game: " + value.Value, value));
            }
         }
      }

      private static void ProcessValueUnit(TriggerValue value, List<TriggerSystemDebugInfo> debugInfo, int id, List<int> boundItems)
      {
         EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
         if (obj != null)
         {
            SimObjectData data = obj.GetProperties() as SimObjectData;
            if (data != null && data.IsSquad == false)
            {
               boundItems.Add(id);
               obj.SetBound(true);

            }
            else
            {
               debugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.ValueError, null, "invalid unit", value));
            }
         }
         else
         {
            debugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.ValueError, null, " invalid unit: unit does not exist", value));
         }
      }
      private static void ProcessValueObject(TriggerValue value, List<TriggerSystemDebugInfo> debugInfo, int id, List<int> boundItems)
      {
         EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
         if (obj != null)
         {
            SimObjectData data = obj.GetProperties() as SimObjectData;
            if (data != null && data.IsSquad == false)
            {
               boundItems.Add(id);
               obj.SetBound(true);

            }
            else
            {
               debugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.ValueError, null, "invalid object", value));
            }
         }
         else
         {
            debugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.ValueError, null, " invalid object: object does not exist", value));
         }
      }
      private static void ProcessValueSquad(TriggerValue value, List<TriggerSystemDebugInfo> debugInfo, int id, List<int> boundItems)
      {
         EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
         if (obj != null)
         {
            SimObjectData data = obj.GetProperties() as SimObjectData;
            if (data != null)//&& data.IsSquad == true)
            {
               boundItems.Add(id);
               obj.SetBound(true);
            }
            else
            {
               debugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.ValueError, null, "invalid squad", value));
            }
         }
         else
         {
            debugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.ValueError, null, " invalid squad: squad does not exist", value));
         }
      }

      //public void PreProcessObjectGraph(TriggerRoot triggers)
      //{
      //   PreProcessObjectGraph(triggers, true, null, null);
      //}
      public void PreProcessObjectGraph(TriggerRoot triggers, bool scenarioDataAvailable)
      {
         PreProcessObjectGraph(triggers, scenarioDataAvailable, null, null);
      }
      public void PreProcessObjectGraph(TriggerRoot triggers, bool scenarioDataAvailable, Dictionary<int, bool> externalTriggerActivation, Dictionary<int, bool> externalVarInitialization)
      {
         try
         {

            mActivatedTriggers.Clear();
            mInitializedValues.Clear();

            ScanTemplateData(triggers.TriggerEditorData);

            if (externalTriggerActivation != null)
            {
               Dictionary<int, bool>.Enumerator it = externalTriggerActivation.GetEnumerator();
               while (it.MoveNext())
               {
                  mActivatedTriggers[it.Current.Key] = it.Current.Value;
               }
            }
            if (externalVarInitialization != null)
            {
               Dictionary<int, bool>.Enumerator it = externalVarInitialization.GetEnumerator();
               while (it.MoveNext())
               {
                  mInitializedValues[it.Current.Key] = it.Current.Value;
               }
            }

            //mDebugInfo.Clear();

            mTriggerIDs.RegisterID(triggers.NextTriggerID);
            mConditionIDs.RegisterID(triggers.NextConditionID);
            mEffectIDs.RegisterID(triggers.NextEffectID);
            mValueIDs.RegisterID(triggers.NextTriggerVarID);


            Dictionary<string, TriggerValue> usedVariableNames = new Dictionary<string, TriggerValue>();
            List<TriggerValue> duplicateVariables = new List<TriggerValue>();
            foreach (TriggerValue v in triggers.TriggerVars.TriggerVar)
            {
               mValueIDs.RegisterID(v.ID);
               mTriggerValues[v.ID] = v;

               if (v.Value != "" && v.Value != null)
               {
                  mInitializedValues[v.ID] = true;

                  //mDebugInfo.AddRange(ProcessValue(v, scenarioDataAvailable));

               }

               // mrh - Add a check to see if this is a duplicately named variable.
               if (!String.IsNullOrEmpty(v.Name))
               {
                  TriggerValue previousVarWithName;
                  if (usedVariableNames.TryGetValue(v.Name, out previousVarWithName))
                  {
                     if (duplicateVariables.Contains(previousVarWithName) == false)
                        duplicateVariables.Add(previousVarWithName);
                     if (duplicateVariables.Contains(v) == false)
                        duplicateVariables.Add(v);
                     usedVariableNames.Remove(v.Name);
                  }
                  usedVariableNames.Add(v.Name, v);
               }
            }

            // mrh - Throw the dupliate named variable error.
            //    foreach (TriggerValue v in duplicateVariables)
            //    {
            //     mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.DuplicateVariableName, null, "Duplicate Variable Name", v));
            //   }

            ////find commented out values
            //Dictionary<int, bool> commentedValues = new Dictionary<int, bool>();
            //foreach(Trigger t in triggers.TriggerEditorData.TriggerSystem.Trigger)
            //{
            //   List<TriggerCondition> conditions = WalkConditions(t.TriggerConditions.Child);
            //   foreach (TriggerCondition cond in conditions)
            //      foreach (TriggerVariable v in cond.Parameter)
            //         if(t.CommentOut || cond.CommentOut)
            //            commentedValues[v.ID] = true;
            //   foreach (TriggerEffect effect in t.TriggerEffects.Effects)
            //      foreach (TriggerVariable v in effect.Parameter)
            //         if (t.CommentOut || effect.CommentOut)
            //            commentedValues[v.ID] = true;
            //   foreach (TriggerEffect effect in t.TriggerEffectsFalse.Effects)
            //      foreach (TriggerVariable v in effect.Parameter)
            //         if (t.CommentOut || effect.CommentOut)
            //            commentedValues[v.ID] = true;      
            //}

            Dictionary<int, bool> usedValues = new Dictionary<int, bool>();

            //APF:  these functions contain a dev bug :  the bool clear existing is treated as and output
            //Update.. actually it we will just ignore clear existing
            List<string> devComponentBugsToIgnore = new List<string>();
            //devComponentBugsToIgnore.Add("SquadListAdd");
            //devComponentBugsToIgnore.Add("SquadListRemove");
            //devComponentBugsToIgnore.Add("UnitListAdd");
            //devComponentBugsToIgnore.Add("UnitListRemove");


            //lists / markings for objects that are bound to triggers!
            SimGlobals.getSimMain().ClearBoundObjects();
            mListOfBoundObjects.Clear();

            foreach (Trigger t in triggers.Trigger)
            {
               //if(t.StayActiveOnFire == true)
               //{
               //   mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.GeneralError, null, "Stay active is obsolete", t));
               //}

               mTriggerIDs.RegisterID(t.ID);
               mTriggers[t.ID] = t;
               List<TriggerCondition> conditions = WalkConditions(t.TriggerConditions.Child);
               foreach (TriggerCondition cond in conditions)
               {
                  mDebugInfo.AddRange(TriggerSystemMain.mTriggerDefinitions.ProcessTriggerComponent(cond));

                  mConditionIDs.RegisterID(cond.ID);
                  mTriggerConditions[cond.ID] = cond;
                  foreach (TriggerVariable v in cond.Parameter)
                  {
                     RegisterVar(t.ID, v);

                     if (mTriggerValues.ContainsKey(v.ID) == false)
                     {
                        mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.MissingValue, null, "Missing Value (in trigger " + t.ID.ToString() + ") ", v));
                     }
                     else
                     {
                        TriggerValue value = null;
                        if (mTriggerValues.TryGetValue(v.ID, out value) == true)
                        {
                           if (value.IsNull == false)
                           {
                              mDebugInfo.AddRange(ProcessValue(value, scenarioDataAvailable));
                           }
                        }
                     }
                     if (v is TriggersOutputVariable)
                     {
                        mInitializedValues[v.ID] = true;

                        if (devComponentBugsToIgnore.Contains(cond.Type))
                           continue;
                        TriggerValue value = null;
                        if (mTriggerValues.TryGetValue(v.ID, out value) == false)
                        {

                        }
                        else if (value.IsNull == false && (value.Name == "" || value.Name == null))
                        {
                           if (v.Name.Contains("ClearExisting"))  //DEV SIDE TRIGGER BUG
                              continue;
                           mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.WriteToConstant, null, "Can't Write to Constant", v));
                        }

                     }
                     usedValues[v.ID] = true;
                  }

               }
               foreach (TriggerEffect effect in t.TriggerEffects.Effects)
               {
                  mDebugInfo.AddRange(TriggerSystemMain.mTriggerDefinitions.ProcessTriggerComponent(effect));

                  mEffectIDs.RegisterID(effect.ID);
                  mTriggerEffects[effect.ID] = effect;
                  foreach (TriggerVariable v in effect.Parameter)
                  {
                     RegisterVar(t.ID, v);

                     if (mTriggerValues.ContainsKey(v.ID) == false)
                     {
                        mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.MissingValue, null, "Missing Value (in trigger " + t.ID.ToString() + ") ", v));
                     }
                     else
                     {
                        TriggerValue value = null;
                        if (mTriggerValues.TryGetValue(v.ID, out value) == true)
                        {
                           if (value.IsNull == false)
                           {
                              mDebugInfo.AddRange(ProcessValue(value, scenarioDataAvailable));
                           }
                        }
                     }

                     if (v is TriggersOutputVariable)
                     {

                        mInitializedValues[v.ID] = true;


                        if (devComponentBugsToIgnore.Contains(effect.Type))
                           continue;

                        TriggerValue value = null;
                        if (mTriggerValues.TryGetValue(v.ID, out value) == false)
                        {

                        }
                        else if (value.IsNull == false && (value.Name == "" || value.Name == null))
                        {
                           if (v.Name.Contains("ClearExisting")) //DEV SIDE TRIGGER BUG
                              continue;
                           mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.WriteToConstant, null, "Can't Write to Constant", v));
                        }


                     }
                     usedValues[v.ID] = true;
                  }


                  //look for trigger activation
                  if (effect.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName())
                  {
                     int id = effect.Parameter[0].ID;
                     if (mTriggerValues.ContainsKey(id))
                     {
                        mActivatedTriggers[System.Convert.ToInt32(mTriggerValues[id].Value)] = true;
                     }
                  }
               }
               foreach (TriggerEffect effect in t.TriggerEffectsFalse.Effects)
               {
                  mDebugInfo.AddRange(TriggerSystemMain.mTriggerDefinitions.ProcessTriggerComponent(effect));

                  mEffectIDs.RegisterID(effect.ID);
                  mTriggerEffectsFalse[effect.ID] = effect;
                  foreach (TriggerVariable v in effect.Parameter)
                  {
                     RegisterVar(t.ID, v);

                     if (mTriggerValues.ContainsKey(v.ID) == false)
                     {
                        mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.MissingValue, null, "Missing Value (in trigger " + t.ID.ToString() + ") ", v));
                     }
                     else
                     {
                        TriggerValue value = null;
                        if (mTriggerValues.TryGetValue(v.ID, out value) == true)
                        {
                           if (value.IsNull == false)
                           {
                              mDebugInfo.AddRange(ProcessValue(value, scenarioDataAvailable));
                           }
                        }
                     }
                     if (v is TriggersOutputVariable)
                     {
                        mInitializedValues[v.ID] = true;

                        if (devComponentBugsToIgnore.Contains(effect.Type))
                           continue;
                        TriggerValue value = null;
                        if (mTriggerValues.TryGetValue(v.ID, out value) == false)
                        {

                        }
                        else if (value.IsNull == false && (value.Name == "" || value.Name == null))
                        {
                           if (v.Name.Contains("ClearExisting"))  //DEV SIDE TRIGGER BUG
                              continue;
                           mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.WriteToConstant, null, "Can't Write to Constant", v));
                        }
                     }
                     usedValues[v.ID] = true;

                  }

                  //look for trigger activation
                  if (effect.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName())
                  {
                     int id = effect.Parameter[0].ID;
                     if (mTriggerValues.ContainsKey(id))
                     {
                        mActivatedTriggers[System.Convert.ToInt32(mTriggerValues[id].Value)] = true;
                     }
                  }
               }

               if (t.Active == true)
               {
                  mActivatedTriggers[t.ID] = true;
               }
            }

            //Don't scan templates for this:

            //post scan for un assigned vars
            foreach (TriggerValue v in triggers.TriggerVars.TriggerVar)
            {
               if ((mInitializedValues.ContainsKey(v.ID) == false) && (usedValues.ContainsKey(v.ID) == true))
               {

                  if ((v.IsNull == false) && (TriggerSystemMain.mTriggerDefinitions.IsListType(v.Type) == false))
                  {
                     mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.UnassignedVariable, null, "Variable never initialized", v));
                  }
               }
            }
            foreach (Trigger t in triggers.Trigger)
            {
               if (mActivatedTriggers.ContainsKey(t.ID) == false)
               {
                  mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Warning, TriggerSystemDebugType.NeverActiveTrigger, null, "Trigger never activated", t));
               }
            }
            //NeverActiveTrigger
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      public void ScanTemplateData(TriggerEditorData data)
      {         
         foreach(TriggerTemplateMapping m in data.TriggerMappings)
         {
            TriggerTemplateDefinition d = TriggerSystemMain.mTriggerDefinitions.GetTemplateDefinition(m.Name);
            
            if (d == null)
            {
               mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.MissingDefinition, d, "Missing template ", m));
               continue;
            }
            if (d.TriggerTemplateMapping.DoNotUse == true)
            {
               mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.DoNotUse, d, "Do not use ", m));

            }
            if (d.TriggerTemplateMapping.Obsolete == true)
            {
               mDebugInfo.Add(new TriggerSystemDebugInfo(TriggerSystemDebugLevel.Error, TriggerSystemDebugType.ObsoleteVersion, d, "Obsolete ", m));
            }

         }
      }

      private void ProcessEditorData(TriggerEditorData data)
      {
         ClearData();

         foreach(TriggerTemplateMapping m in  data.TriggerMappings)
         {
            //mTemplateIDs.RegisterID(m.ID);
            mTriggerIDs.RegisterID(m.ID);

            mTemplateMappings[m.ID] = m;
         }
         mDebugInfo.Clear();
         PreProcessObjectGraph(data.TriggerSystem, false);

         ProcessVarMapping();

         KillOrphanValues();
      }

      public void ReProcessVarRegistry(TriggerEditorData data)
      {
         mValueIDsToVars.Clear();
         mValueIDsToTriggers.Clear();
         mValueIDsToTemplates.Clear();

         foreach (Trigger t in data.TriggerSystem.Trigger) 
         {
            List<TriggerCondition> conditions = WalkConditions(t.TriggerConditions.Child);
            foreach (TriggerCondition cond in conditions)
            {
               foreach (TriggerVariable v in cond.Parameter)
               {
                  RegisterVar(t.ID, v);
               }
            }
            foreach (TriggerEffect effect in t.TriggerEffects.Effects)
            {

               foreach (TriggerVariable v in effect.Parameter)
               {
                  RegisterVar(t.ID, v);
               }
            }
            foreach (TriggerEffect effect in t.TriggerEffectsFalse.Effects)
            {
               foreach (TriggerVariable v in effect.Parameter)
               {
                  RegisterVar(t.ID, v);
               }
            }

         }

         foreach (TriggerTemplateMapping m in data.TriggerMappings)
         {
            foreach (TriggersTemplateInputVariable v in m.InputMappings)
            {
               RegisterMappingVar(m.ID,v);
            }
            foreach (TriggersTemplateOutputVariable v in m.OutputMappings)
            {
               RegisterMappingVar(m.ID,v);
            }
         }


         foreach (TriggerValue v in data.TriggerSystem.TriggerVars.TriggerVar)
         {
            mValueIDs.RegisterID(v.ID);

            mValueNames[v.Name] = 1;
         }


      }

      public void ProcessVarMapping()
      {
         ReProcessVarRegistry(mEditorData);

         Dictionary<int,int> tempOneOrMoreRefs = new Dictionary<int,int>();

         mGlobalVars.Clear();
         Dictionary<int, List<int>>.Enumerator it = mValueIDsToTriggers.GetEnumerator();
         while (it.MoveNext())
         {
            if (it.Current.Value.Count > 1)
            {
               mGlobalVars.Add(it.Current.Key);
            }
         }

         mSharedValueVars.Clear();
         Dictionary<int, List<TriggerVariable>>.Enumerator it2 = mValueIDsToVars.GetEnumerator();
         while (it2.MoveNext())
         {
            if(it2.Current.Value.Count > 0 )
            {
               tempOneOrMoreRefs[it2.Current.Key] = 1;
            }

            if (it2.Current.Value.Count > 1)
            {
               mSharedValueVars.Add(it2.Current.Key);
            }
         }
         Dictionary<int, List<int>>.Enumerator it3 = mValueIDsToTemplates.GetEnumerator();
         while (it3.MoveNext())
         {
            if (it3.Current.Value.Count > 0)
            {
               tempOneOrMoreRefs[it3.Current.Key] = 1;
            }
         }

         mOrphans.Clear();

         foreach (TriggerValue v in mEditorData.TriggerSystem.TriggerVars.TriggerVar)
         {
            if(tempOneOrMoreRefs.ContainsKey(v.ID) == false)
            {
               mOrphans.Add(v.ID);
            }
         }

         foreach (TriggerValue v in mEditorData.TriggerSystem.TriggerVars.TriggerVar)
         {
            if (v.Type == "ChatSpeaker")
               v.Value = TriggerPropChatSpeaker.FixUp(v.Value);
            else if (v.Type == "TechStatus")
               v.Value = TriggerPropTechStatus.FixUp(v.Value);
            else if (v.Type == "ExposedAction")
               v.Value = TriggerPropExposedAction.FixUp(v.Value);
            else if (v.Type == "DataScalar")
               v.Value = TriggerPropDataScalar.FixUp(v.Value);
            else if (v.Type == "ActionStatus")
               v.Value = TriggerPropActionStatus.FixUp(v.Value);
            else if (v.Type == "FlareType")
               v.Value = TriggerPropFlareType.FixUp(v.Value);
            else if (v.Type == "AnimType")
               v.Value = TriggerPropAnimType.FixUp(v.Value);
         }

         //...  haha kill the poor little orphans right away
         KillOrphanValues();

      }

      private void RegisterVar(int triggerID, TriggerVariable v)
      {
         if (mValueIDsToVars.ContainsKey(v.ID) == false)
         {
            mValueIDsToVars[v.ID] = new List<TriggerVariable>();
         }
         mValueIDsToVars[v.ID].Add(v);

         if(mValueIDsToTriggers.ContainsKey(v.ID) == false)
         {
            mValueIDsToTriggers[v.ID] = new List<int>();
         }
         if (mValueIDsToTriggers[v.ID].Contains(triggerID) == false)
         {
            mValueIDsToTriggers[v.ID].Add(triggerID);
         }
      }
      private void RegisterMappingVar(int templateID, TriggersTemplateVariableBinder v)
      {
         if (mValueIDsToVars.ContainsKey(v.ID) == false)
         {
            mValueIDsToVars[v.ID] = new List<TriggerVariable>();
         }
         mValueIDsToVars[v.ID].Add(v);

         if (mValueIDsToTemplates.ContainsKey(v.ID) == false)
         {
            mValueIDsToTemplates[v.ID] = new List<int>();
         }
         if (mValueIDsToTemplates[v.ID].Contains(templateID) == false)
         {
            mValueIDsToTemplates[v.ID].Add(templateID);
         }        
      }


      public object GetConditionNodeParent(object node, object searchTarget)
      {
         object parent = null;
         List<TriggerCondition> conditions = new List<TriggerCondition>();


            if (node is TriggersTriggerAnd)
            {
               TriggersTriggerAnd andCond = (TriggersTriggerAnd)node;
               //if (andCond == searchTarget)
               //{
               //   parent = node;
               //}
               //else
               //{
               foreach (object o in andCond.Children)
               {
                  if(o == searchTarget)
                  {
                     return node;
                  }

                  parent = GetConditionNodeParent(o, searchTarget);
                  if (parent != null)
                     return parent;

               }
               //}
            }
            if (node is TriggersTriggerOR)
            {
               TriggersTriggerOR orCond = (TriggersTriggerOR)node;
               //if (orCond == searchTarget)
               //{
               //   parent = node;
               //}
               //else
               {
                  foreach (object o in orCond.Children)
                  {
                     if (o == searchTarget)
                     {
                        return node;
                     }

                     parent = GetConditionNodeParent(o, searchTarget);
                     if (parent != null)
                        return parent;
                  }
               }
            }
            //if (node is TriggerCondition)
            //{
            //   TriggerCondition condition = (TriggerCondition)node;
            //   if (condition == searchTarget)
            //      parent = node;

            //}
         
         return parent;

      }

      public List<string> GetGroupNames()
      {
         List<string> names = new List<string>();

         Dictionary<int, GroupUI> groups = new Dictionary<int, GroupUI>();
         foreach (GroupUI groupUI in TriggerEditorData.UIData.mGroups)
         {
            names.Add( groupUI.Title );
         }

         return names;
      }

      public Dictionary<int, GroupUI> GetGroups()
      {
         Dictionary<int, GroupUI> groups = new Dictionary<int, GroupUI>();
         foreach (GroupUI groupUI in TriggerEditorData.UIData.mGroups)
         {
            groups[groupUI.InternalGroupID] = groupUI;
         }

         return groups;
      }

      public string GetGroupName(int id)
      {
         if (id == -1)
            return "Top";
         GroupUI groupUI = null;
         if (GetGroups().TryGetValue(id, out groupUI) == true)
         {
            return groupUI.Title;
         }

         return "noName";

      }


      public List<TriggerVariable> WalkVariables(Trigger trigger)
      {
         List<TriggerVariable> vars = new List<TriggerVariable>();
         foreach (TriggerCondition c in WalkConditions(trigger.TriggerConditions.Child))
         {
            vars.AddRange(WalkVariables(c));
         }
         foreach(TriggerEffect e in trigger.TriggerEffects.Effects)
         {
            vars.AddRange(e.Parameter);
         }
         foreach (TriggerEffect e in trigger.TriggerEffectsFalse.Effects)
         {
            vars.AddRange(e.Parameter);
         }
         return vars;
      }


      public List<TriggerVariable> WalkVariables(TriggerComponent component)
      {
         List<TriggerVariable> vars = new List<TriggerVariable>();
         foreach(TriggerVariable v in component.Parameter)
         {
            vars.Add(v);
         }
         return vars;
      }

      static public List<TriggerVariable> WalkVariablesStatic(TriggerComponent component)
      {
         List<TriggerVariable> vars = new List<TriggerVariable>();
         foreach (TriggerVariable v in component.Parameter)
         {
            vars.Add(v);
         }
         return vars;
      }

      //public List<TriggerEffect> WalkLinks(TriggerRoot root)
      //{
      //   List<TriggerEffect> links = new List<TriggerEffect>();
      //   foreach(Trigger t in root.Trigger)
      //   {
      //      if(t.TriggerEffects != null)
      //      {
      //         foreach(TriggerEffect e in t.TriggerEffects.Effects)
      //         {
      //            if( TriggerSystemMain.mTriggerDefinitions.IsLink(e) )
      //            {
      //               links.Add(e);                     
      //            }
      //         }
      //      }

      //   }
      //}

      public List<Trigger> WalkTriggers()
      {
         return this.mTriggerRoot.Trigger;
      }

      public List<TriggerEffect> WalkEffects(Trigger t)
      {
         List<TriggerEffect> effects = new List<TriggerEffect>();

         effects.AddRange(t.TriggerEffects.Effects);
         effects.AddRange(t.TriggerEffectsFalse.Effects);

         return effects;
      }

      public List<TriggerCondition> WalkConditions(object node)
      {
         List<TriggerCondition> conditions = new List<TriggerCondition>();

         if (node is Trigger)
         {
            return WalkConditions(((Trigger)node).TriggerConditions.Child);
         }
         if (node is TriggersTriggerAnd)
         {
            TriggersTriggerAnd andCond = (TriggersTriggerAnd)node;
            foreach (object o in andCond.Children)
            {
               conditions.AddRange(WalkConditions(o));
            }
         }
         if (node is TriggersTriggerOR)
         {
            TriggersTriggerOR orCond = (TriggersTriggerOR)node;
            foreach (object o in orCond.Children)
            {
               conditions.AddRange(WalkConditions(o));
            }
         }
         if (node is TriggerCondition)
         {
            TriggerCondition condition = (TriggerCondition)node;
            conditions.Add(condition);
         }

         return conditions;
      }

      /// <summary>
      /// yes this is an afterthought
      /// </summary>
      /// <param name="node"></param>
      /// <returns></returns>
      static public List<TriggerCondition> WalkConditionsStatic(object node)
      {
         List<TriggerCondition> conditions = new List<TriggerCondition>();

         if (node is Trigger)
         {
            return WalkConditionsStatic(((Trigger)node).TriggerConditions.Child);
         }
         if (node is TriggersTriggerAnd)
         {
            TriggersTriggerAnd andCond = (TriggersTriggerAnd)node;
            foreach (object o in andCond.Children)
            {
               conditions.AddRange(WalkConditionsStatic(o));
            }
         }
         if (node is TriggersTriggerOR)
         {
            TriggersTriggerOR orCond = (TriggersTriggerOR)node;
            foreach (object o in orCond.Children)
            {
               conditions.AddRange(WalkConditionsStatic(o));
            }
         }
         if (node is TriggerCondition)
         {
            TriggerCondition condition = (TriggerCondition)node;
            conditions.Add(condition);
         }

         return conditions;
      }

   }

   //load additional trigger info from supporting files???
   //templates.


   class TriggerSystemMain
   {

      static public TriggerSystemDefinitions mTriggerDefinitions = null;

      static public SimResources mSimResources = null;

      TriggerNamespace mMainTriggers = new TriggerNamespace();

      static public void Init()
      {
         if (mTriggerDefinitions == null)
         {
            mTriggerDefinitions = new TriggerSystemDefinitions();
         }
         if(mSimResources == null)
         {

            mSimResources = new SimResources();
            mSimResources.LoadResouces();
         }
      }

      public TriggerRoot TriggerData
      {
         set
         {
            mMainTriggers.TriggerData = value;            
         }
         get
         {
            return mMainTriggers.TriggerData;
         }
      }
      public TriggerNamespace MainNamespace
      {
         get
         {
            return mMainTriggers;
         }
      }






   }

   
   //class TriggerSystemTemplate
   //{
      
   //}


   //class SubTriggerSystem
   //{


   //}


   public class TriggerValueContainer
   {
      public TriggerValueContainer(TriggerValue val)
      {
         mVal = val;
      }
      TriggerValue mVal;
      public string Name
      {
         //set
         //{
            
         //}

         get
         {
            return mVal.Name;
         }
      }
      public string DefaultValue
      {
         //set
         //{

         //}
         get
         {
            return mVal.Value;
         }
      }
      public string Type
      {
         //set
         //{

         //}
         get
         {
            return mVal.Type;
         }
      }
      public int ID
      {
         get
         {
            return mVal.ID;
         }
         //set
         //{

         //}

      }
   }


   public class TriggerClipboard
   {
      //Data
      public List<object> mData = new List<object>();
      public List<Type> mTypes = new List<Type>();
      public List<System.Drawing.Point> mPositions = new List<System.Drawing.Point>();
      public System.Drawing.Point mTopLeft = new System.Drawing.Point(int.MaxValue,int.MaxValue);
    
      public Dictionary<int, TriggerValue> mValues = new Dictionary<int, TriggerValue>();
      Dictionary<int, TriggerValue> mValueMap = new Dictionary<int, TriggerValue>();
      Dictionary<int, Trigger> mTriggerMap = new Dictionary<int, Trigger>();

      public void ClearTempData()
      {
         //mValues.Clear();
         mValueMap.Clear();
         mTriggerMap.Clear();
      }

      public void CopyValues(ICollection<TriggerValue> values)
      {
         foreach (TriggerValue v in values)
         {
            TriggerValue valCopy = v.GetCopy();
            mValues[valCopy.ID] = (valCopy);
         }
      }

      public void AddPoint(System.Drawing.Point p)
      {
         mPositions.Add(p);
         
         //foreach(System.Drawing.Point point in mPositions)
         //{
         //   x += point.X;
         //   y += point.Y;

         //}
         //x /= mPositions.Count;
         //y /= mPositions.Count;
         if (p.X < mTopLeft.X)
            mTopLeft.X = p.X;
         if (p.Y < mTopLeft.Y)
            mTopLeft.Y = p.Y;

      }



      //MergeLogic
      public TriggerValue GetValue(TriggerValue oldValue, TriggerNamespace newNamespace, out int newID)
      {
         if (mValueMap.ContainsKey(oldValue.ID) == false)
         {
            TriggerValue newVal = oldValue.GetCopy();

            newNamespace.InsertValue(newVal, out newID);
            mValueMap[oldValue.ID] = newVal;


         }
         newID = mValueMap[oldValue.ID].ID;
         return mValueMap[oldValue.ID];
      }


      public Trigger GetTrigger(Trigger oldTrigger, TriggerNamespace newNamespace, out int newID)
      {
         if (mTriggerMap.ContainsKey(oldTrigger.ID) == false)
         {
            //TriggerValue newVal = oldTrigger.GetCopy();

            //newNamespace.InsertValue(newVal, out newID);

            Trigger t = new Trigger();
            oldTrigger.DeepCopyTo(t);

            //ugh clean links since we dont paste them yet
            //List<TriggerEffect> hitlist = new List<TriggerEffect>();
            //foreach (TriggerEffect e in t.TriggerEffects.Effects)
            //{
            //   if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName() || e.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
            //      hitlist.Add(e);
            //}
            //foreach(TriggerEffect e in hitlist) 
            //{
            //   t.TriggerEffects.Effects.Remove(e);
            //}
            //hitlist.Clear();
            //foreach (TriggerEffect e in t.TriggerEffectsFalse.Effects)
            //{
            //   if (e.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName() || e.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
            //      hitlist.Add(e);
            //}
            //foreach (TriggerEffect e in hitlist)
            //{
            //   t.TriggerEffectsFalse.Effects.Remove(e);
            //}

            //Process vars..
            //Walk
            List<TriggerVariable> vars = newNamespace.WalkVariables(t);
            foreach(TriggerVariable v in vars)
            {
               int valID;
               TriggerValue oldValue = mValues[v.ID];
               TriggerValue newValue = GetValue(oldValue, newNamespace, out valID);
               v.ID = valID;
            }

            newNamespace.InsertTrigger(t, out newID);

            mTriggerMap[oldTrigger.ID] = t;
         }
         
         newID = mTriggerMap[oldTrigger.ID].ID;
         return mTriggerMap[oldTrigger.ID];


      }
      //protected TriggerComponent GetComponent(TriggerComponent oldcom, TriggerComponent newComp, TriggerNamespace newNamespace, out int newID)
      //{

            
      //      //List<TriggerVariable> vars = newNamespace.WalkVariables(t);
      //      //foreach(TriggerVariable v in vars)
      //      //{
      //      //   int valID;
      //      //   TriggerValue oldValue = mValues[v.ID];
      //      //   TriggerValue newValue = GetValue(oldValue, newNamespace, out valID);
      //      //   v.ID = valID;
      //      //}

      //}

      public TriggerEffect GetEffect(TriggerEffect oldItem, TriggerNamespace newNamespace)//, out int newID)
      {
         TriggerEffect newItem = new TriggerEffect();
         oldItem.CopyTo(newItem);

         //List<TriggerVariable> vars = newNamespace.WalkVariables(newItem);
         //foreach (TriggerVariable v in vars)
         //{
         //   int valID;
         //   TriggerValue oldValue = mValues[v.ID];
         //   TriggerValue newValue = GetValue(oldValue, newNamespace, out valID);
         //   v.ID = valID;

         //}

         

         //newNamespace.InsertEffect( )

         return newItem;

      }
      
      public TriggerCondition GetCondition(TriggerCondition oldItem, TriggerNamespace newNamespace)//, out int newID)
      {
         TriggerCondition newItem = new TriggerCondition();
         oldItem.CopyTo(newItem);

         //List<TriggerVariable> vars = newNamespace.WalkVariables(newItem);
         //foreach (TriggerVariable v in vars)
         //{
         //   int valID;
         //   TriggerValue oldValue = mValues[v.ID];
         //   TriggerValue newValue = GetValue(oldValue, newNamespace, out valID);
         //   v.ID = valID;

         //}

         //newID = 0;

         return newItem;

      }

   }

   public class TriggerFinalBake
   {

      #region FinalBake

      public enum eBuildMode
      {
         NoOptimizations,
         Debug,
         Playtest,
         Final
      }

      /// <summary>
      /// call this for scenario .scn files
      /// </summary>
      /// <param name="filename"></param>
      /// <param name="mode"></param>
      /// <returns></returns>
      static public string OptimizeScenarioScripts(string filename, eBuildMode mode)
      {
         try
         {
            throw new System.Exception("build tool not supporting in place xmb optimizations.   so this function needs to be redone");

            //load
            XmlSerializer s = new XmlSerializer(typeof(ScenarioXML), new Type[] { typeof(ScenarioSimUnitXML) });
            Stream st = File.OpenRead(filename);
            ScenarioXML scenario = (ScenarioXML)s.Deserialize(st);
            st.Close();

            //bake each root
            foreach (TriggerRoot root in scenario.mTriggers)
            {
               FinalBake(root, mode);
            }

            //save
            string saveFileName = Path.ChangeExtension(filename, ".temp.scn");
            st = File.Open(saveFileName, FileMode.Create);
            s.Serialize(st, scenario);
            st.Close();

            //xmb
            return saveFileName;
         }
         catch (System.Exception ex)
         {
            return "";
         }
      }

      /// <summary>
      /// call this for "_raw" triggerscript files
      /// the optimized file will remove the raw tag
      /// </summary>
      /// <param name="filename"></param>
      /// <param name="mode"></param>
      /// <returns></returns>
      static public string OptimizeTriggerScript(string filename, eBuildMode mode)
      {
         try
         {
            //load
            XmlSerializer s = new XmlSerializer(typeof(TriggerRoot), new Type[] { });
            Stream st = File.OpenRead(filename);
            TriggerRoot TriggerData = (TriggerRoot)s.Deserialize(st);
            st.Close();

            //bake
            FinalBake(TriggerData, mode);

            //save
            //string saveFileName = Path.ChangeExtension(filename, ".temp.triggerscript");
            string saveFileName = filename.Replace(sTriggerRaw,"");
            st = File.Open(saveFileName, FileMode.Create);
            TriggerRoot root = TriggerData;
            s.Serialize(st, root);
            st.Close();

            //xmb

            return saveFileName;
         }
         catch (System.Exception ex)
         {
            return "";
         }
      }
      static public string sTriggerRaw = "_raw_";


      static protected string FinalBake(TriggerRoot root, eBuildMode mode)
      {
         int unusedvars = 0;
         int optionalreduction = 0;
         int constReduction = 0;
         int effectsRemoved = 0;
         int unusedvars2 = 0;

         int startVarCount = root.TriggerVars.TriggerVar.Count;
         int endVarCount = 0;
         int varSaved = 0;

         ClearEditorOnlyData(root); //always
         unusedvars = CleanUnusedVariables(root); //always
         optionalreduction = OptimizeOptionalVars(root, true);
         constReduction = OptimizeConstants(root);

         if (mode == eBuildMode.Final || mode == eBuildMode.Playtest)
         {
            effectsRemoved = CleanDebugEffects(root);
            unusedvars2 = CleanUnusedVariables(root); //vars cleaned from removing effects
         }

         if (mode == eBuildMode.Final || mode == eBuildMode.Playtest)
            ReduceNameAttributes(root);

         if(mode == eBuildMode.Final || mode == eBuildMode.Playtest)
            PackVariables(root);

         endVarCount = root.TriggerVars.TriggerVar.Count;
         varSaved = startVarCount - endVarCount;

         return "";//todo write results
      }

      /// <summary>
      /// clear editor only data
      /// </summary>
      static protected void ClearEditorOnlyData(TriggerRoot root)
      {
         root.TriggerEditorData = new TriggerEditorData(); //or null
      }

      /// <summary>
      /// *make sure that all commented out data is cleard / vars
      /// </summary>
      static protected int CleanUnusedVariables(TriggerRoot root)
      {
         int numTriggerVars = root.TriggerVars.TriggerVar.Count;
         if (numTriggerVars == 0)
            return 0;
         int largestindex = root.TriggerVars.TriggerVar[numTriggerVars - 1].ID;
         int[] indexFound = new int[largestindex + 1];
         for (int i = 0; i <= largestindex; i++)
         {
            indexFound[i] = 0;
         }

         //walk all vars and mark them
         foreach (Trigger trigger in root.Trigger)
         {
            List<TriggerVariable> vars = new List<TriggerVariable>();
            foreach (TriggerCondition c in TriggerNamespace.WalkConditionsStatic(trigger.TriggerConditions.Child))
            {
               vars.AddRange(TriggerNamespace.WalkVariablesStatic(c));
            }
            foreach (TriggerEffect e in trigger.TriggerEffects.Effects)
            {
               vars.AddRange(e.Parameter);
            }
            foreach (TriggerEffect e in trigger.TriggerEffectsFalse.Effects)
            {
               vars.AddRange(e.Parameter);
            }
            foreach (TriggerVariable v in vars)
            {
               indexFound[v.ID]++;
            }
         }
         List<int> hitlist = new List<int>();
         //kill the vars
         for (int i = 0; i <= largestindex; i++)
         {
            if (indexFound[i] == 0)
            {
               hitlist.Add(i);
            }

         }
         List<TriggerValue> nonDeadVars = new List<TriggerValue>();
         foreach (TriggerValue v in root.TriggerVars.TriggerVar)
         {
            if (hitlist.Contains(v.ID))
               continue;
            nonDeadVars.Add(v);
         }
         //swap

         root.TriggerVars.TriggerVar = nonDeadVars;

         return numTriggerVars - nonDeadVars.Count;
      }

      /// <summary>
      /// PHX-15878 AI MEMORY - Reduce const-variables in bake out.
      /// </summary>
      static protected int OptimizeConstants(TriggerRoot root)
      {
         int numTriggerVars = root.TriggerVars.TriggerVar.Count;
         if (numTriggerVars == 0)
            return 0;
         int largestindex = root.TriggerVars.TriggerVar[numTriggerVars - 1].ID;
         int[] indexFound = new int[largestindex + 1];
         for (int i = 0; i <= largestindex; i++)
         {
            indexFound[i] = 0;
         }

         Dictionary<int, TriggerValue> valMap = new Dictionary<int, TriggerValue>();
         foreach (TriggerValue v in root.TriggerVars.TriggerVar)
         {
            valMap.Add(v.ID, v);
         }
         Dictionary<string, Dictionary<string, int>> optionalProtoVals = new Dictionary<string, Dictionary<string, int>>();

         List<int> hitlist = new List<int>();

         //walk all vars 
         foreach (Trigger trigger in root.Trigger)
         {
            List<TriggerVariable> vars = new List<TriggerVariable>();
            foreach (TriggerCondition c in TriggerNamespace.WalkConditionsStatic(trigger.TriggerConditions.Child))
            {
               vars.AddRange(TriggerNamespace.WalkVariablesStatic(c));
            }
            foreach (TriggerEffect e in trigger.TriggerEffects.Effects)
            {
               vars.AddRange(e.Parameter);
            }
            foreach (TriggerEffect e in trigger.TriggerEffectsFalse.Effects)
            {
               vars.AddRange(e.Parameter);
            }
            foreach (TriggerVariable v in vars)
            {
               indexFound[v.ID]++;

               TriggerValue val = valMap[v.ID];
               if (val.Name == "" && val.IsNull == false) //is it a constant
               {
                  Dictionary<string, int> mappings = null;
                  if (optionalProtoVals.TryGetValue(val.Type, out mappings) == false)
                  {
                     mappings = new Dictionary<string, int>();
                     optionalProtoVals[val.Type] = mappings;

                     mappings[val.Value] = v.ID;
                  }
                  else
                  {
                     int remapID = -1;
                     if (mappings.TryGetValue(val.Value, out remapID) == false)
                     {
                        mappings[val.Value] = v.ID;
                     }
                     else
                     {
                        hitlist.Add(val.ID); //kill value
                        v.ID = remapID; //remapvalue 
                     }

                  }

               }
            }
         }


         List<TriggerValue> nonDeadVars = new List<TriggerValue>();
         foreach (TriggerValue v in root.TriggerVars.TriggerVar)
         {
            if (hitlist.Contains(v.ID))
               continue;
            nonDeadVars.Add(v);
         }
         //swap
         root.TriggerVars.TriggerVar = nonDeadVars;
         return numTriggerVars - nonDeadVars.Count;
      }

      /// <summary>
      /// PHX-15877 AI MEMORY - Reduce optional var memory footprint
      /// </summary>
      static protected int OptimizeOptionalVars(TriggerRoot root, bool cleanupConst)
      {
         int numTriggerVars = root.TriggerVars.TriggerVar.Count;
         if (numTriggerVars == 0)
            return 0;
         int largestindex = root.TriggerVars.TriggerVar[numTriggerVars - 1].ID;
         int[] indexFound = new int[largestindex + 1];
         for (int i = 0; i <= largestindex; i++)
         {
            indexFound[i] = 0;
         }

         Dictionary<int, TriggerValue> valMap = new Dictionary<int, TriggerValue>();
         foreach (TriggerValue v in root.TriggerVars.TriggerVar)
         {
            valMap.Add(v.ID, v);
         }
         Dictionary<string, int> optionalProtoVals = new Dictionary<string, int>();

         List<int> hitlist = new List<int>();

         //walk all vars 
         foreach (Trigger trigger in root.Trigger)
         {
            List<TriggerVariable> vars = new List<TriggerVariable>();
            foreach (TriggerCondition c in TriggerNamespace.WalkConditionsStatic(trigger.TriggerConditions.Child))
            {
               vars.AddRange(TriggerNamespace.WalkVariablesStatic(c));
            }
            foreach (TriggerEffect e in trigger.TriggerEffects.Effects)
            {
               vars.AddRange(e.Parameter);
            }
            foreach (TriggerEffect e in trigger.TriggerEffectsFalse.Effects)
            {
               vars.AddRange(e.Parameter);
            }
            foreach (TriggerVariable v in vars)
            {
               indexFound[v.ID]++;
               if (v.Optional == true)
               {
                  TriggerValue val = valMap[v.ID];
                  if (val.IsNull == true)
                  {
                     int remapID = -1;
                     if (optionalProtoVals.TryGetValue(val.Type, out remapID) == false)
                     {
                        //use this one
                        optionalProtoVals[val.Type] = val.ID;
                        if (cleanupConst)
                        {
                           val.Value = null;
                        }
                     }
                     else
                     {
                        hitlist.Add(val.ID); //kill value
                        v.ID = remapID; //remapvalue 
                     }

                  }
               }
            }
         }


         List<TriggerValue> nonDeadVars = new List<TriggerValue>();
         foreach (TriggerValue v in root.TriggerVars.TriggerVar)
         {
            if (hitlist.Contains(v.ID))
               continue;
            nonDeadVars.Add(v);
         }
         //swap
         root.TriggerVars.TriggerVar = nonDeadVars;
         return numTriggerVars - nonDeadVars.Count;

      }

      /// <summary>
      /// PHX-15880 AI MEMORY - Remove debug effects in bake out (and vars only referenced by those effects.)    
      /// </summary>
      static protected int CleanDebugEffects(TriggerRoot root)
      {
         int effectsRemoved = 0;
         foreach (Trigger trigger in root.Trigger)
         {
            List<TriggerEffect> hitlist = new List<TriggerEffect>();
            foreach (TriggerEffect e in trigger.TriggerEffects.Effects)
            {
               if (isDebugEffect(e))
               {
                  hitlist.Add(e);
               }
            }
            foreach (TriggerEffect effect in hitlist)
            {
               trigger.TriggerEffects.Effects.Remove(effect);
            }

            effectsRemoved += hitlist.Count;
            hitlist.Clear();
            foreach (TriggerEffect e in trigger.TriggerEffectsFalse.Effects)
            {
               if (isDebugEffect(e))
               {
                  hitlist.Add(e);
               }
            }
            foreach (TriggerEffect effect in hitlist)
            {
               trigger.TriggerEffectsFalse.Effects.Remove(effect);
            }
            effectsRemoved += hitlist.Count;
         }

         return effectsRemoved;
      }

      static bool isDebugEffect(TriggerEffect e)
      {
         if (e.Type.StartsWith("DebugVar"))
         {
            return true;
         }
         return false;
      }


      /// <summary>
      /// if "" dont change .  if not "" set to "v"
      /// </summary>
      static protected void ReduceNameAttributes(TriggerRoot root)
      {
         foreach (TriggerValue v in root.TriggerVars.TriggerVar)
         {
            v.Name = "";
            //if (v.Name != "")
            //   v.Name = "v";
         }
         foreach (Trigger trigger in root.Trigger)
         {
            trigger.Name = "";
            List<TriggerVariable> vars = new List<TriggerVariable>();
            foreach (TriggerCondition c in TriggerNamespace.WalkConditionsStatic(trigger.TriggerConditions.Child))
            {
               c.Type = "";
               vars.AddRange(TriggerNamespace.WalkVariablesStatic(c));
            }
            foreach (TriggerEffect e in trigger.TriggerEffects.Effects)
            {
               e.Type = "";
               vars.AddRange(e.Parameter);
            }
            foreach (TriggerEffect e in trigger.TriggerEffectsFalse.Effects)
            {
               e.Type = "";
               vars.AddRange(e.Parameter);
            }
            foreach (TriggerVariable v in vars)
            {
               v.Name = "";
            }
         }
      }


      /// <summary>
      /// *pack variables
      /// </summary>
      static protected void PackVariables(TriggerRoot root)
      {
         int numTriggerVars = root.TriggerVars.TriggerVar.Count;
         if (numTriggerVars == 0)
            return;
         int largestindex = root.TriggerVars.TriggerVar[numTriggerVars - 1].ID;
         int[] idRemap = new int[largestindex + 1];
         for (int i = 0; i <= largestindex; i++)
         {
            idRemap[i] = 0;
         }
         int newIDs = 0;
         foreach (TriggerValue val in root.TriggerVars.TriggerVar)
         {
            idRemap[val.ID] = newIDs;
            val.ID = newIDs;
            newIDs++;
         }

         //walk all vars and mark them
         foreach (Trigger trigger in root.Trigger)
         {
            List<TriggerVariable> vars = new List<TriggerVariable>();
            foreach (TriggerCondition c in TriggerNamespace.WalkConditionsStatic(trigger.TriggerConditions.Child))
            {
               vars.AddRange(TriggerNamespace.WalkVariablesStatic(c));
            }
            foreach (TriggerEffect e in trigger.TriggerEffects.Effects)
            {
               vars.AddRange(e.Parameter);
            }
            foreach (TriggerEffect e in trigger.TriggerEffectsFalse.Effects)
            {
               vars.AddRange(e.Parameter);
            }
            foreach (TriggerVariable v in vars)
            {
               v.ID = idRemap[v.ID]; 
            }
         }
      }



      #endregion



   }


}
