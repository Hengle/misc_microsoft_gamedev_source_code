using System;
using System.Collections.Generic;
using System.Text;
using System.ComponentModel;
using System.Xml.Serialization;

namespace pxdb
{
   //==============================================================================
   // GameData
   //==============================================================================
   [XmlRoot("GameData")]
   public class GameData
   {
      private GameDataLinkList mLinks = new GameDataLinkList();
      private GameDataResources mResources = new GameDataResources();
      private GameDataPops mPops = new GameDataPops();
      private float mGarrisonDamageMultiplier;
      private float mConstructionDamageMultiplier;
      private float mSquadLeashLength;
      private float mCloakingDelay;
      private string mShieldBarColor;
      private float mShieldRegenDelay;
      private float mShieldRegenTime;
      private float mProjectileGravity;

      [XmlElement("Link", typeof(GameDataLink))]
      public GameDataLinkList Links
      {
         get { return mLinks; }
         set { mLinks = value; }
      }

      [XmlElement("Resources", typeof(GameDataResources))]
      public GameDataResources ResourceData
      {
         get { return mResources; }
         set { mResources = value; }
      }

      [XmlIgnore]
      public DataStringList Resources
      {
         get { return mResources.List; }
         set { mResources.List = value; }
      }

      [XmlElement("Pops", typeof(GameDataPops))]
      public GameDataPops PopData
      {
         get { return mPops; }
         set { mPops = value; }
      }

      [XmlIgnore]
      public DataStringList Pops
      {
         get { return mPops.List; }
         set { mPops.List = value; }
      }

      [XmlElement("GarrisonDamageMultiplier")]
      public float GarrisonDamageMultiplier
      {
         get { return mGarrisonDamageMultiplier; }
         set { mGarrisonDamageMultiplier = value; }
      }

      [XmlElement("ConstructionDamageMultiplier")]
      public float ConstructionDamageMultiplier
      {
         get { return mConstructionDamageMultiplier; }
         set { mConstructionDamageMultiplier = value; }
      }

      [XmlElement("SquadLeashLength")]
      public float SquadLeashLength
      {
         get { return mSquadLeashLength; }
         set { mSquadLeashLength = value; }
      }

      [XmlElement("CloakingDelay")]
      public float CloakingDelay
      {
         get { return mCloakingDelay; }
         set { mCloakingDelay = value; }
      }

      [XmlElement("ShieldBarColor")]
      public string ShieldBarColor
      {
         get { return mShieldBarColor; }
         set { mShieldBarColor = value; }
      }

      [XmlElement("ShieldRegenDelay")]
      public float ShieldRegenDelay
      {
         get { return mShieldRegenDelay; }
         set { mShieldRegenDelay = value; }
      }

      [XmlElement("ShieldRegenTime")]
      public float ShieldRegenTime
      {
         get { return mShieldRegenTime; }
         set { mShieldRegenTime = value; }
      }

      [XmlElement("ProjectileGravity")]
      public float ProjectileGravity
      {
         get { return mProjectileGravity; }
         set { mProjectileGravity = value; }
      }
   }

   //==============================================================================
   // GameDataResources
   //==============================================================================
   public class GameDataResources
   {
      private DataStringList mList = new DataStringList();

      [XmlElement("Resource", typeof(DataString))]
      public DataStringList List
      {
         get { return mList; }
         set { mList = value; }
      }
   }

   //==============================================================================
   // GameDataPops
   //==============================================================================
   public class GameDataPops
   {
      private DataStringList mList = new DataStringList();

      [XmlElement("Pop", typeof(DataString))]
      public DataStringList List
      {
         get { return mList; }
         set { mList = value; }
      }
   }

   //==============================================================================
   // GameDataLink
   //==============================================================================
   public class GameDataLink
   {
      private int mID = -1;
      private string mName = @"";

      [XmlAttribute("id")]
      public int ID
      {
         get { return mID; }
         set { mID = value; }
      }

      [XmlText]
      public string Name
      {
         get { return mName; }
         set { mName = value; }
      }
   }

   //==============================================================================
   // GameDataLinkList
   //==============================================================================
   public class GameDataLinkList : List<GameDataLink>
   {
   }
}
