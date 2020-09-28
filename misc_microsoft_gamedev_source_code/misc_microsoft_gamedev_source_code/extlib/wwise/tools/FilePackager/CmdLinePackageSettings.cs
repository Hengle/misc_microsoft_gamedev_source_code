using System;
using System.Collections.Generic;
using System.Text;

namespace AkFilePackager
{
    /// <summary>
    /// Command line package settings.
    /// Contains the package settings, and defines setters that are aware of if a setting
    /// was changed explicitly.
    /// </summary>
    class CmdLinePackageSettings
    {
        public uint DefaultBlockSize
        {
            set { m_uDefaultBlockSize = value; m_bDefaultBlockSizeSpecified = true; }
        }
        public string FilePackage
        {
            set { m_szFilePackagePath = value; m_bFilePackagePathSpecified = true; }
        }
        public bool FilePackagePathSpecified
        {
            get { return m_bFilePackagePathSpecified; }
        }

        /// <summary>
        /// Get setting values stored herein.
        /// </summary>
        /// <returns>Package settings</returns>
        public PackageSettings GetSettings()
        {
            PackageSettings settings;
            settings.uDefaultBlockSize = m_uDefaultBlockSize;
            settings.szFilePackageFilename = m_szFilePackagePath;
            return settings;
        }

        /// <summary>
        /// Overrides the package settings with values stored herein, only
        /// when these values have been set explicitly by the user.
        /// </summary>
        /// <param name="io_settings">Package settings. Settings that were set explicitly are replaced.</param>
        public void OverrideSettingsWithCmdLine(ref PackageSettings io_settings)
        {
            if (m_bDefaultBlockSizeSpecified)
                io_settings.uDefaultBlockSize = m_uDefaultBlockSize;
            if (m_bFilePackagePathSpecified)
                io_settings.szFilePackageFilename = m_szFilePackagePath;
        }

        private uint m_uDefaultBlockSize = 1;   // Default DefaultBlockSize value is 1.
        private bool m_bDefaultBlockSizeSpecified = false;

        private string m_szFilePackagePath = "";
        private bool m_bFilePackagePathSpecified = false;
    };
}
