using System;

using System.Windows.Forms;
using System.IO;

using System.Collections.Generic;
using System.Text;
using System.Drawing;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using System.Xml;
using System.Xml.Serialization;

using EditorCore;
using Rendering;

namespace SimEditor
{
   public class SimTerrainType
   {
      static public string mTextureSetExtention = ".ttp";   //terrain Texture Set
      static public string mTextureDefFileName = "terrainTextureDefinitions.xml";
      static public string mDefaultSetFileName = "defaultSet";
      static public string mBlankTex = "blank";
      static public string mTerrainTileTypeFileName = "terrainTileTypes.xml";

      static public List<TerrainTextureDef> mTextureList = new List<TerrainTextureDef>();
      static public List<int> mActiveWorkingSet = new List<int>();
   
      static public TerrainTileTypes mTerrainTileTypes;

      static public List<string> getTerrainTextures()
      {

         List<string> tex = new List<string>();

         string[] texThemes = Directory.GetDirectories(CoreGlobals.getWorkPaths().mTerrainTexturesPath);
         for(int themes=0;themes<texThemes.Length;themes++)
         {
            string mainFolder = texThemes[themes].Substring(texThemes[themes].LastIndexOf("\\") + 1);
            string[] textureNames = Directory.GetFiles(CoreGlobals.getWorkPaths().mTerrainTexturesPath + @"\" + mainFolder, "*.ddx", SearchOption.TopDirectoryOnly);

            for (int i = 0; i < textureNames.Length; i++)
            {
               //get our path relative to the theme and name
               string dir = Path.GetDirectoryName(textureNames[i]);
               string Theme = dir.Substring(dir.LastIndexOf("\\") + 1);
           //    if (Theme == "terrain")
           //       continue;

               string fname = Theme + @"\" + Path.GetFileNameWithoutExtension(textureNames[i]);
               if (fname.Contains("_df") && !fname.Contains("_dcl_"))
                  tex.Add(getpureFileNameNoExt(fname));
            }
         }

         

         

         
         
     

         return tex;
      }
      static public string getpureFileNameNoExt(string fname)
      {
         string[] stripList = new string[5]{"_df","_nm","_sp","_em","_rm"};

         for(int i=0;i<5;i++)
         {
            int k = fname.IndexOf(stripList[i]);
            if(k!=-1)

            return fname.Substring(0,fname.LastIndexOf("_"));
         }
         

         return fname;
      }

      static public void addDefaultBlankType()
      {
         TerrainTextureDef def = new TerrainTextureDef();
         def.toDefaultValues();
         def.TypeName = mBlankTex;
         mTextureList.Add(def);
      }
      
      static public void loadTerrainTileTypes()
      {
         if (mTerrainTileTypes != null)
            return;

         //load the XML file
         string fileName = CoreGlobals.getWorkPaths().mGameDataDirectory + @"\" + mTerrainTileTypeFileName;
         if (!File.Exists(fileName))
         {
            CoreGlobals.getErrorManager().OnSimpleWarning("Error loading terrain tile types.  Can't find " + fileName);
            return;
         }

         XmlSerializer s = new XmlSerializer(typeof(TerrainTileTypes), new Type[] { typeof(TerrainTileType) });
         Stream st = File.OpenRead(fileName);
         mTerrainTileTypes = (TerrainTileTypes)s.Deserialize(st);

         if (mTerrainTileTypes == null)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning("No terrain tile types loaded check " + fileName);
         }
      }

      //our main texture definitions
      static public void loadTerrainTypes()
      {
         //grab a list of all the texture names that exist
         List<string> currentTextures = SimTerrainType.getTerrainTextures();

         mTextureList.Clear();

         TerrainTextureDefXML mTerrainTypesXML;

         //load the XML file
         string fileName = CoreGlobals.getWorkPaths().mGameDataDirectory + @"\" + mTextureDefFileName;
         if (File.Exists(fileName))
         {
            XmlSerializer s = new XmlSerializer(typeof(TerrainTextureDefXML), new Type[] { typeof(TerrainTextureDef) });
            Stream st = File.OpenRead(fileName);
            mTerrainTypesXML = (TerrainTextureDefXML)s.Deserialize(st);

            for (int j = 0; j < currentTextures.Count; j++)
            {
               string themeName;
               string fName;
               chopName(currentTextures[j],out themeName,out fName);

               bool found=false;
               int k = 0;
               for(k=0;k<mTerrainTypesXML.mTypes.Count;k++)
               {
                  if (mTerrainTypesXML.mTypes[k].Theme == themeName &&
                     mTerrainTypesXML.mTypes[k].TextureName == fName)
                  {
                     found = true;
                     break;
                  }
               }
               if(found)
               {
                  mTextureList.Add(mTerrainTypesXML.mTypes[k]);
               }
               else
               {
                  TerrainTextureDef def = new TerrainTextureDef();
                  def.toDefaultValues();
                  def.Theme = themeName;
                  def.TextureName = getpureFileNameNoExt(fName);
                  def.TypeName = def.Theme + "_" + def.TextureName;
                  mTextureList.Add(def);
               }
               
            }
              
            st.Close();
         }
         
         //if the file doesn't exist, create it real quick..
         if (!File.Exists(fileName))
         {
            SimTerrainType.writeTerrainTypes();
         }

         //load a default working set...
         if (mActiveWorkingSet.Count == 0)
         {
            for (int i = 0; i < mTextureList.Count; i++)
            {
               if (!mTextureList[i].TextureName.Contains("blank"))
               {
                  mActiveWorkingSet.Add(i);
                  break;
               }
            }
         }

      }
      static public void writeTerrainTypes()
      {
         string fileName = CoreGlobals.getWorkPaths().mGameDataDirectory + @"\" + mTextureDefFileName;
         if (!P4CanEdit(fileName, false))
            {
               if(MessageBox.Show("data\\terrainTextureDefinitions.xml is not checked out from perforce.\n This file must be checked out in order to make changes.\nWould you like to check it out?", "ALERT!", MessageBoxButtons.YesNo) == DialogResult.Yes)
               {
                  P4AddFileToChangeList(CoreGlobals.getWorkPaths().mGameDataDirectory + @"\" + mTextureDefFileName);
               }
               else
               {
                  MessageBox.Show("You must check out hise file before you can save it.");
                  return;
               }
               
            }

         
         
         if (File.Exists(fileName))
            File.Delete(fileName);

         {
            TerrainTextureDef defaultDef = new TerrainTextureDef();
            defaultDef.toDefaultValues();

            TerrainTextureDefXML kDef = new TerrainTextureDefXML();
            for (int i = 0; i < mTextureList.Count; i++)
            {
               string tName = mTextureList[i].Theme + @"\" + mTextureList[i].TextureName;

               if (File.Exists(CoreGlobals.getWorkPaths().mTerrainTexturesPath + @"\" + tName + "_df.ddx"))
               {
                  //check to ensure we actually have sim data that's not defaulted...
                  if ((mTextureList[i].ObstructLand != defaultDef.ObstructLand) || (mTextureList[i].TileType != defaultDef.TileType))
                  {
                     kDef.mTypes.Add(mTextureList[i]);
                  }
               }
            }

            XmlSerializer s = new XmlSerializer(typeof(TerrainTextureDefXML), new Type[] { typeof(TerrainTextureDef) });
            Stream st = File.OpenWrite(fileName);
            s.Serialize(st, kDef);

            st.Close();

            XMBProcessor.CreateXMB(fileName, false);

         }
      }
      static public List<TerrainTextureDef> getFilteredDefs(string themeFilter)
      {
         List<TerrainTextureDef> tList = new List<TerrainTextureDef>();

         foreach (TerrainTextureDef obj in mTextureList)
         {
            if (themeFilter != null)
               if (themeFilter != obj.Theme)
                  continue;

            tList.Add(obj);
         }

         return tList;
      }
      static public void mergeFilteredDefs(List<TerrainTextureDef> extList)
      {
         for (int i = 0; i < extList.Count;i++ )
         {
            for (int k = 0; k < mTextureList.Count; k++)
            {
               if (extList[i].TypeName == mTextureList[k].TypeName)
               {
                  mTextureList[k].ObstructLand = extList[i].ObstructLand;
                  mTextureList[k].TileType = extList[i].TileType;
               }
            }
         }
      }
      static public TerrainTextureDef getDefFromDat(string typeName)
      {
         for (int k = 0; k < mTextureList.Count; k++)
         {
            if (mTextureList[k].TypeName == typeName)
               return mTextureList[k];
         }
         return null;
      }
      static public void chopName(string filename, out string themeName, out string fName)
      {
         fName = getpureFileNameNoExt(Path.GetFileNameWithoutExtension(filename));

         string dir = Path.GetDirectoryName(filename);
         themeName = dir.Substring(dir.LastIndexOf("\\") + 1);  
      }
      //working sets
      static public List<string> loadTerrainPalettes()
      {
         List<string> texSetList = new List<string>();
         //load the rest of the sets
         string[] setNames = Directory.GetFiles(CoreGlobals.getWorkPaths().mTerrainTexturesPath, "*" + mTextureSetExtention, SearchOption.AllDirectories);
         for (int i = 0; i < setNames.Length; i++)
         {
            string dir = setNames[i].Remove(0,CoreGlobals.getWorkPaths().mTerrainTexturesPath.Length + 1);
            string fname = dir.Substring(0,dir.LastIndexOf("."));
               texSetList.Add(fname);
         }

         return texSetList;
      }
      static public List<TerrainSetTexture> loadTerrainPalette(string fileName)
      {
         TerrainTextureSetXML terrainSetXML;

         //load the XML file
         if (File.Exists(fileName))
         {
            XmlSerializer s = new XmlSerializer(typeof(TerrainTextureSetXML), new Type[] { typeof(TerrainSetTexture) });
            Stream st = File.OpenRead(fileName);
            terrainSetXML = (TerrainTextureSetXML)s.Deserialize(st);

            st.Close();

            return terrainSetXML.mTextures;
         }
         return null;
      }
      static public List<TerrainSetTexture> loadActiveSetFromPalette(string filename)
      {

         List<TerrainSetTexture> tex = loadTerrainPalette(filename);
         if(tex==null) return null;

         //mActiveWorkingSet.Clear();
         int c = tex.Count;
         if (c > SimTerrainType.mActiveWorkingSet.Count) c = SimTerrainType.mActiveWorkingSet.Count;
         for (int i = 0; i < c; i++)
            mActiveWorkingSet[i] = (getIndexFromDef(getFromTypeName(tex[i].mTypeName)));

         return tex;
      }
      static public void writeTerrainPalette(string fileName, List<string> texSet)
      {
         if (File.Exists(fileName) )
         {
            if (P4CanEdit(fileName, true))
               File.Delete(fileName);
            else
               return;
         }
         else if (Path.GetFileNameWithoutExtension(fileName) != mDefaultSetFileName)
         {
            if (MessageBox.Show(fileName + " does not exist in perforce.\n Would you like to add it?", "", MessageBoxButtons.YesNo) == DialogResult.Yes)
               P4AddFileToChangeList(fileName);
         }

         {
            TerrainTextureSetXML kDef = new TerrainTextureSetXML();
            kDef.mTextures = new List<TerrainSetTexture>(texSet.Count);

            for (int i = 0; i < texSet.Count; i++)
            {
               kDef.mTextures.Add(new TerrainSetTexture());
               kDef.mTextures[kDef.mTextures.Count - 1].mTypeName = texSet[i];
            }


            XmlSerializer s = new XmlSerializer(typeof(TerrainTextureSetXML), new Type[] { typeof(TerrainSetTexture) });
            Stream st = File.OpenWrite(fileName);
            s.Serialize(st, kDef);

            st.Close();

            XMBProcessor.CreateXMB(fileName, false);

         }
      }

      //queary from the outside world!
      static public List<TerrainTextureDef> getFirstXTextures(int X)
      {
         List<TerrainTextureDef> list = new List<TerrainTextureDef>();
         int c = mTextureList.Count;
         if (c > X) c = X;
         for (int k = 0; k < c; k++)
            list.Add(mTextureList[k]);
         

         return list;
      }
      static public TerrainTextureDef getFromNumber(int k)
      {
         if (k < 0 || k >= mTextureList.Count) 
            return null;

         return mTextureList[k];
      }
      static public TerrainTextureDef getFromTypeName(string name)
      {
         for (int i = 0; i < mTextureList.Count; i++)
         {
            if (mTextureList[i].TypeName == name)
               return mTextureList[i];
         }

         return null;
      }
      static public TerrainTextureDef getFromTextureName(string fullTexturePath)
      {
         string dir = Path.GetDirectoryName(fullTexturePath);
         dir = dir.Substring(dir.LastIndexOf("\\") + 1);
         string fname = Path.GetFileNameWithoutExtension(fullTexturePath);

         for (int i = 0; i < mTextureList.Count; i++)
         {
            if (mTextureList[i].TextureName == fname && mTextureList[i].Theme == dir)
               return mTextureList[i];
         }

         return null;
      }
      static public string getWorkingTexName(TerrainTextureDef def)
      {
         return CoreGlobals.getWorkPaths().mTerrainTexturesPath + @"\" + def.Theme + @"\" + def.TextureName + "_df.tga";
      }
      static public string convToTextureLocalName(string fullFileName)
      {
         string dir = Path.GetDirectoryName(fullFileName);
         return dir.Substring(dir.LastIndexOf("\\") + 1) + @"\" + Path.GetFileNameWithoutExtension(fullFileName);
      }
      static public int getIndexFromDef(TerrainTextureDef def)
      {
         for (int i = 0; i < mTextureList.Count;i++ )
            if(mTextureList[i].TypeName == def.TypeName)
            return i;
         return -1;
      }

      static public List<string> getTextureNamesOfActiveSet()
      {
         List<string> texList = new List<string>();
         for (int i = 0; i < SimTerrainType.mActiveWorkingSet.Count; i++)
         {
            texList.Add(SimTerrainType.getFromNumber(SimTerrainType.mActiveWorkingSet[i]).TextureName);
         }
         return texList;
      }
      static public List<string> getTypeNamesOfActiveSet()
      {
         List<string> texList = new List<string>();
         for (int i = 0; i < SimTerrainType.mActiveWorkingSet.Count; i++)
         {
            texList.Add(SimTerrainType.getFromNumber(SimTerrainType.mActiveWorkingSet[i]).TypeName);
         }
         return texList;
      }
      static public List<TerrainTextureDef> getDefsOfActiveSet()
      {
         List<TerrainTextureDef> texList = new List<TerrainTextureDef>();
         for (int i = 0; i < SimTerrainType.mActiveWorkingSet.Count; i++)
         {
            texList.Add(SimTerrainType.getFromNumber(SimTerrainType.mActiveWorkingSet[i]));
         }
         return texList;
      }
      static public void swapActiveSetDef(int index,TerrainTextureDef def)
      {
         if (index < 0 || index >= SimTerrainType.mActiveWorkingSet.Count)
            return;

         SimTerrainType.mActiveWorkingSet[index] = SimTerrainType.getIndexFromDef(def);
      }
      static public void addActiveSetDef(TerrainTextureDef def)
      {
         if (def == null)
         {
            MessageBox.Show("WARNING: Terrain type " + def.TextureName + " not found! Replacing with a default type");
            def = SimTerrainType.getFromTypeName("terrain_blank");
         }
         int ind = SimTerrainType.getIndexFromDef(def);
         for (int i = 0; i < SimTerrainType.mActiveWorkingSet.Count; i++)
            if (SimTerrainType.mActiveWorkingSet[i] == ind)
               return;

         SimTerrainType.mActiveWorkingSet.Add(ind);
      }
      static public int getActiveSetIndex(TerrainTextureDef def)
      {
         if (def == null)
            return -1;

         int ind = SimTerrainType.getIndexFromDef(def);
         for (int i = 0; i < SimTerrainType.mActiveWorkingSet.Count; i++)
            if (SimTerrainType.mActiveWorkingSet[i] == ind)
               return i ;

         return -1;
      }
      static public bool isDefInActiveSet(TerrainTextureDef def)
      {
         if (def == null)
            return false;

         int ind = SimTerrainType.getIndexFromDef(def);
         for (int i = 0; i < SimTerrainType.mActiveWorkingSet.Count; i++)
            if (SimTerrainType.mActiveWorkingSet[i] == ind)
               return true;

         return false;
      }
      static public void removeActiveSetDef(int i)
      {
         SimTerrainType.mActiveWorkingSet.RemoveAt(i);
      }
      static public void removeActiveSetDef(TerrainTextureDef def)
      {
         int index = getActiveSetIndex(def);
         if(index ==-1)
            return;

         removeActiveSetDef(index);
      }
      static public void swapActiveSetDef(int orig, int next)
      {
         if(orig > next)
         {
            int k = orig;
            orig = next;
            next = k;
         }
         SimTerrainType.mActiveWorkingSet.Insert(orig, SimTerrainType.mActiveWorkingSet[next]);
         SimTerrainType.mActiveWorkingSet.Insert(next+1, SimTerrainType.mActiveWorkingSet[orig + 1]);
         SimTerrainType.mActiveWorkingSet.RemoveAt(orig + 1);
         SimTerrainType.mActiveWorkingSet.RemoveAt(next + 1);
      }
      static public void replaceActiveSetDef(int orig, int next)
      {
         swapActiveSetDef(orig, next);
         removeActiveSetDef(next);
         /*
         SimTerrainType.mActiveWorkingSet.Insert(orig, SimTerrainType.mActiveWorkingSet[next]);
         SimTerrainType.mActiveWorkingSet.RemoveAt(orig + 1);
         SimTerrainType.mActiveWorkingSet.RemoveAt(next);
          */ 
      }
      static public void clearActiveSet()
      {
         if (mActiveWorkingSet.Count > 0)
            mActiveWorkingSet.RemoveRange(1, mActiveWorkingSet.Count-1);
      }

      static public int getNumTileTypes()
      {
         return mTerrainTileTypes.mTypes.Count;
      }
      static public TerrainTileType getTileTypeByIndex(int idx)
      {
         if (idx < 0 || idx >= mTerrainTileTypes.mTypes.Count)
            idx = 0;
         return mTerrainTileTypes.mTypes[idx];
      }

      static public TerrainTileType getTileTypeByName(string name)
      {
         for (int i = 0; i < mTerrainTileTypes.mTypes.Count; i++)
         {
            if(mTerrainTileTypes.mTypes[i].Name == name)
            {
               return mTerrainTileTypes.mTypes[i];
            }
         } 
         return mTerrainTileTypes.mTypes[0];
      }
      static public int getTileTypeIndexByName(string name)
      {
         for (int i = 0; i < mTerrainTileTypes.mTypes.Count; i++)
         {
            if (mTerrainTileTypes.mTypes[i].Name == name)
            {
               return i;
            }
         }
         return 0;
      }

      //perforce interaction
      static private string mChangelistDesc = "Editor Terrain Type Automated Checkout";
      static private int mChangelistID = -1;
      static private void P4AddFileToChangeList(string filename)
      {
         PerforceChangeList list = null;
         if (mChangelistID == -1)
         {
            list = CoreGlobals.getPerforce().GetNewChangeList(mChangelistDesc);
            mChangelistID = list.ID;
         }
         else
         {
            list = CoreGlobals.getPerforce().GetExistingChangeList(mChangelistID);
         }


         list.AddOrEdit(filename,true);
      }

      static private bool P4CanEdit(string filename, bool showAlerts)
      {
         SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(filename);
         if (status.InPerforce == false)              return true;
         if (status.CheckedOutOtherUser == true)       return false;
         

         if (status.CheckedOut==false)
         {
            if(showAlerts)
            {
               if (MessageBox.Show(Path.GetFileName(filename) + "is not checked out. Would you like to check it out and add it to a new perforce changelist?", "", MessageBoxButtons.OKCancel) == DialogResult.OK)
               {
                  P4AddFileToChangeList(filename);
                  return true;
               }
               else
               {
                  return false;
               }
            }
            return false;
         }

         return true;
      }
      static private bool P4IsCheckedOut(string fileName,bool showAlert)
      {
         SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(fileName);
            if (status.CheckedOutOtherUser == true)
            {
               if (showAlert)
               {
                  if (MessageBox.Show("This scenario is checked out by " + status.ActionOwner + " cannot check out", "", MessageBoxButtons.OKCancel) == DialogResult.OK)
                  {
                     return true;
                  }
                  else
                  {
                     return false;
                  }
               }
               return true;
            }
            return false;
      }


      
   }
}


[XmlRoot("TerrainTileTypes")]
public class TerrainTileTypes
{
   [XmlAttribute("version")]
   public string version;

   [XmlElement("TerrainTileType", typeof(TerrainTileType))]
   public List<TerrainTileType> mTypes = new List<TerrainTileType>();
}

[XmlRoot("TerrainTileType")]
public class TerrainTileType
{
   string mTerrainTileType = "UNDEFINED";

   [XmlAttribute("name")]
   public string Name
   {
      get
      {
         return this.mTerrainTileType;
      }
      set
      {
         this.mTerrainTileType = value;
      }
   }

   string mTerrainTileTypeEditorColor = "0x00000000";

   [XmlAttribute("EditorColor")]
   public string EditorColor
   {
      get
      {
         return mTerrainTileTypeEditorColor;// Convert.ToInt32(this.mTerrainTileTypeEditorColor, 16);
      }
      set
      {
         mTerrainTileTypeEditorColor = value;// this.mTerrainTileTypeEditorColor = value.ToString("x");
      }
   }
}


[XmlRoot("TerrainTextureDef")]
public class TerrainTextureDefXML
{
   [XmlAttribute("version")]
   public string version;

   [XmlElement("TextureDef", typeof(TerrainTextureDef))]
   public List<TerrainTextureDef> mTypes = new List<TerrainTextureDef>();
}

[XmlRoot("TextureDef")]
public class TerrainTextureDef
{
   string mTypeName = "Type";
   string mTextureName = "blank.tga";
   string mTheme = "wasteland";
   bool mObstructLand = false;
   string mTileType = "UNDEFINED";

   string mDumbImgHolder;

   [XmlText]
   public string TypeName
   {
      get
      {
         return this.mTypeName;
      }
      set
      {
         this.mTypeName = value;
      }
   }


   [XmlAttribute("TextureName")]
   public string TextureName
   {
      get
      {
         return this.mTextureName;
      }
      set
      {
         this.mTextureName = value;
      }
   }


   [XmlAttribute("Theme")]
   public string Theme
   {
      get
      {
         return this.mTheme;
      }
      set
      {
         this.mTheme = value;
      }
   }

   [XmlAttribute("ObstructLand")]
   public bool ObstructLand
   {
      get
      {
         return /*System.Convert.ToString*/(this.mObstructLand);
      }
      set
      {
         this.mObstructLand = /*System.Convert.ToBoolean*/(value);
      }
   }

   [XmlAttribute("TileType")]
   public string TileType
   {
      get
      {
         return this.mTileType;
      }
      set
      {
         this.mTileType = value;
      }
   }

   [XmlIgnore]
   public string Preview
   {
      get
      {
         return this.mDumbImgHolder;
      }
      set
      {
         this.mDumbImgHolder = value;
      }
   }

   public void toDefaultValues()
   {
      mTypeName = "Type";
      mTextureName = "blank";
      mTheme = "terrain";
      mObstructLand = false;
      mTileType = "UNDEFINED";
   }
}

[XmlRoot("TerrainTextureSet")]
public class TerrainTextureSetXML
{
   [XmlAttribute("version")]
   public string version;

   [XmlElement("TextureDef", typeof(TerrainSetTexture))]
   public List<TerrainSetTexture> mTextures = new List<TerrainSetTexture>();
}

[XmlRoot("SetTexture")]
public class TerrainSetTexture
{
   [XmlText]
   public string mTypeName;
}
