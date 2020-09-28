using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using ConsoleUtils;


namespace DatabaseUtils
{
   public enum OperatorType
   {
      Equal = 0,
      GreaterThan,
      LessThan
   }

   public class UnitPrereq
   {
      public long unitID = -1;
      public long count = 0;
      public OperatorType op = OperatorType.Equal;
   }



   public class ProtoTech
   {   
      /*
      enum
      {
         cFlagOwnStaticData,
         cFlagUnobtainable,
         cFlagUnique,
         cFlagShadow,
         cFlagOrPrereqs,
         cFlagPerpetual,
         cFlagForbid,
      };
      */

      public bool bFlagUnobtainable = false;
      public bool bFlagUniqueProtoUnitInstance = false;
      public bool bFlagShadow = false;
      public bool bFlagOrPrereqs = false;
      public bool bFlagPerpetual = false;

      //UTBitVector<8>          mFlags;
      //BCost                   mCost;
      //float                   mResearchPoints;

      //long                             mID;
      //long                             mDBID;
      //long                             mDisplayNameID;
      //long                             mRolloverTextID;
      public string              mName;
      //string                           mIcon;
      //long                             mAnimType;
      public List<long> mTechPrereqs = new List<long>();
      public List<UnitPrereq>    mUnitPrereqs = new List<UnitPrereq>();
      public List<TechEffect>    mTechEffects = new List<TechEffect>();
      //public List<long>          mDependentTechs = new List<long>();
      //long                             mProtoVisualIndex;
      //int                              mCircleMenuIconID;


      public ProtoTech()
      {
      }

      //==============================================================================
      // load
      //==============================================================================
      public bool load(XmlNode techNode)
      {
         // read name
         XmlNode nameNode = techNode.Attributes.GetNamedItem("name");
         if ((nameNode == null) || (nameNode.FirstChild == null))
            return false;
            
         mName = nameNode.FirstChild.Value;

         // read flags
         XmlNodeList flagNodes = techNode.SelectNodes("./Flag");
         foreach (XmlNode flagNode in flagNodes)
         {
            switch (flagNode.FirstChild.Value.ToLower())
            {
               case "unobtainable":
                  bFlagUnobtainable = true;
                  break;
               case "uniqueprotounitinstance":
                  bFlagUniqueProtoUnitInstance = true;
                  break;
               case "shadow":
                  bFlagShadow = true;
                  break;
               case "orprereqs":
                  bFlagOrPrereqs = true;
                  break;
               case "perpetual":
                  bFlagPerpetual = true;
                  break;
            }
         }
         
         // read status
         XmlNodeList statusNodes = techNode.SelectNodes("./Status");
         foreach (XmlNode statusNode in statusNodes)
         {
            switch (statusNode.FirstChild.Value.ToLower())
            {
               case "unobtainable":
                  bFlagUnobtainable = true;
                  break;
            }
         }
         
         // read prereq
         XmlNodeList prereqNodes = techNode.SelectNodes("./Prereqs/*");
         foreach (XmlNode prereqNode in prereqNodes)
         {
            switch(prereqNode.Name)
            {
               case "TechStatus":
                  {
                     long techID;
                     if (Database.m_tableTech.TryGetValue(prereqNode.FirstChild.Value.ToLower(), out techID))
                     {
                        mTechPrereqs.Add(techID);
                     }
                     else
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" has an effect that is referring to Tech \"{1}\" which does not exist.\n", mName, prereqNode.FirstChild.Value);
                     }
                  }

                  break;

               case "TypeCount":
                  {
                     XmlNode unitName = prereqNode.Attributes.GetNamedItem("unit");
                     if (unitName == null)
                        continue;

                     XmlNode operatorType = prereqNode.Attributes.GetNamedItem("operator");
                     if (operatorType == null)
                        continue;

                     XmlNode countNode = prereqNode.Attributes.GetNamedItem("count");
                     if (countNode == null)
                        continue;

                     long unitID;
                     if (Database.m_objectTypes.TryGetValue(unitName.FirstChild.Value.ToLower(), out unitID))
                     {
                        UnitPrereq preseq = new UnitPrereq();

                        preseq.unitID = unitID;
                        preseq.count = (long) System.Convert.ToSingle(countNode.FirstChild.Value);

                        switch(operatorType.FirstChild.Value.ToLower())
                        {
                           case "e":
                              preseq.op = OperatorType.Equal;
                              break;
                           case "gt":
                              preseq.op = OperatorType.GreaterThan;
                              break;
                           case "lt":
                              preseq.op = OperatorType.LessThan;
                              break;
                        }

                        mUnitPrereqs.Add(preseq);
                     }
                     else
                     {
                        ConsoleOut.Write(ConsoleOut.MsgType.Warn, "Tech \"{0}\" has a prereq to Unit \"{1}\" which does not exist.\n", mName, unitName.FirstChild.Value);
                     }
                  }

                  break;
            }
         }

         // read effects
         XmlNodeList effectNodes = techNode.SelectNodes("./Effects/Effect");
         foreach (XmlNode effectNode in effectNodes)
         {
            TechEffect effect = new TechEffect();

            bool success = effect.load(effectNode, mName);
            if (!success)
               continue;

            mTechEffects.Add(effect);
         }

         return true;
      }

      //==============================================================================
      // applyEffects
      //==============================================================================
      public void applyEffects(TechTree techTree)
      {
         long count = mTechEffects.Count;
         for(int i=0; i<count; i++)
         {
            TechEffect techEffect = mTechEffects[i];
            techEffect.apply(techTree);
         }
      }
   }

}
