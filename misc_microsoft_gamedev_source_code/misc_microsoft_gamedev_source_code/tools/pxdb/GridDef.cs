using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Serialization;

namespace pxdb
{
   //==============================================================================
   // GridDef
   //==============================================================================
   [XmlRoot("GridDef")]
   public class GridDef
   {
      [XmlElement("Data", typeof(GridDefData))]
      public List<GridDefData> mDatas = new List<GridDefData>();
   }

   //==============================================================================
   // GridDefData
   //==============================================================================
   public class GridDefData
   {
      [XmlAttribute("Type")]
      public string mType = @"";

      [XmlAttribute("DisableAdd")]
      public bool mDisableAdd = false;

      [XmlElement("Group", typeof(GridDefGroup))]
      public List<GridDefGroup> mGroups = new List<GridDefGroup>();
   };

   //==============================================================================
   // GridDefGroup
   //==============================================================================
   public class GridDefGroup
   {
      [XmlAttribute("GroupName")]
      public string mGroupName = @"";

      [XmlElement("Column", typeof(GridDefColumn))]
      public List<GridDefColumn> mColumns = new List<GridDefColumn>();

      [XmlElement("Detail", typeof(GridDefDetail))]
      public List<GridDefDetail> mDetails = new List<GridDefDetail>();
   };

   //==============================================================================
   // GridDefColumn
   //==============================================================================
   public class GridDefColumn
   {
      [XmlAttribute("DataName")]
      public string mDataName=@"";

      [XmlAttribute("HeaderText")]
      public string mHeaderText = @"";

      [XmlAttribute("ValueType")]
      public string mValueType = @"";

      [XmlAttribute("ColumnType")]
      public string mColumnType = @"";

      [XmlAttribute("ReadOnly")]
      public bool mReadOnly=false;

      [XmlAttribute("Frozen")]
      public bool mFrozen = false;

      [XmlAttribute("DataSource")]
      public string mDataSource = @"";

      [XmlAttribute("ValueMember")]
      public string mValueMember = @"";

      [XmlAttribute("DisplayMember")]
      public string mDisplayMember = @"";

      [XmlElement("Item", typeof(string))]
      public List<string> mItems = new List<string>();
   };

   //==============================================================================
   // GridDefDetail
   //==============================================================================
   public class GridDefDetail
   {
      [XmlAttribute("DataName")]
      public string mDataName = @"";

      [XmlElement("Column", typeof(GridDefColumn))]
      public List<GridDefColumn> mColumns = new List<GridDefColumn>();
   };
}
