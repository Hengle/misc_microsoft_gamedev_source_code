using System;
using System.Collections.Generic;
using System.Text;

namespace AkFilePackager
{
    /// <summary>
    /// Item of the ordered file list of a layout.
    /// </summary>
    public class OrderedFile
    {
        /// <summary>
        /// A file has 3 possible states:
        /// "Laid out" if it is defined in the Info file, and it is referred to in the layout;
        /// "New" if it is defined in the Info file, but it is not referred to in the layout;
        /// "Missing" if it is referred to in the layout, but it is not defined in the Info file.
        /// </summary>
        public enum StatusType
        {
            FileStatusLaidOut,
            FileStatusNew,
            FileStatusMissing,
        };

        public OrderedFile(uint in_id, string in_szLanguage, string in_szShortName, AK.Wwise.FilePackager.PackageLayout.Type in_eType, StatusType in_eStatus)
        {
            m_id = in_id;
            m_szLanguage = in_szLanguage;
            m_szShortName = in_szShortName;
            m_eType = in_eType;
            m_eStatus = in_eStatus;
        }

        /// <summary>
        /// File ID.
        /// </summary>
        public uint Id
        {
            get { return m_id; }
            set { m_id = value; }
        }

        /// <summary>
        /// Language (string).
        /// </summary>
        public string Language
        {
            get { return m_szLanguage; }
            set { m_szLanguage = value; }
        }

        /// <summary>
        /// Short name for display in UI.
        /// </summary>
        public string ShortName
        {
            get { return m_szShortName; }
            set { m_szShortName = value; }
        }

        /// <summary>
        /// File type.
        /// </summary>
        public AK.Wwise.FilePackager.PackageLayout.Type Type
        {
            get { return m_eType; }
            set { m_eType = value; }
        }

        /// <summary>
        /// Status of the file in the layout.
        /// </summary>
        public StatusType Status
        {
            get { return m_eStatus; }
            set { m_eStatus = value; }
        }

        private uint m_id = 0;
        private string m_szLanguage = "";
        private string m_szShortName = "";
        private AK.Wwise.FilePackager.PackageLayout.Type m_eType = AK.Wwise.FilePackager.PackageLayout.Type.SoundBank;
        private StatusType m_eStatus = StatusType.FileStatusNew;
    }
}
