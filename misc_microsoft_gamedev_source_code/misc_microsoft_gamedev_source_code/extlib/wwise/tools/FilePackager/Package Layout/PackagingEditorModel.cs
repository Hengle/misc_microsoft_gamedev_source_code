using System;
using System.Collections.Generic;
using System.Text;

using System.Xml;
using System.Xml.Schema;

using AK.Wwise.FilePackager.InfoFile;
using AK.Wwise.FilePackager.PackageLayout;

namespace AkFilePackager
{
    /// <summary>
    /// Packaging editor model. 
    /// Manages the layout, serializing and deserializing.
    /// Handles the business logic of the package editor.
    /// </summary>
    public class PackagingEditorModel
    {
        readonly string LAYOUT_SCHEMA_VERSION = "1";

        /// <summary>
        /// Custom event arguments for the PackagingModelUpdate event.
        /// </summary>
        public class PackagingModelUpdateEventArgs : EventArgs
        {
            public PackagingModelUpdateEventArgs(bool in_bSkipList)
                : this(0, 0, in_bSkipList)
            {
            }
            public PackagingModelUpdateEventArgs(int in_iFirstSelectedIndex, int in_iNumSelectedIndices, bool in_bSkipList)
            {
                m_iFirstSelectedIndex = in_iFirstSelectedIndex;
                m_iNumSelectedIndices = in_iNumSelectedIndices;
                m_bSkipList = in_bSkipList;
            }
            public int FirstSelectedIndex
            {
                get { return m_iFirstSelectedIndex; }
            }
            public int NumSelectedIndices
            {
                get { return m_iNumSelectedIndices; }
            }
            public bool ListChanged
            {
                get { return !m_bSkipList; }
            }            
            private int m_iFirstSelectedIndex;
            private int m_iNumSelectedIndices;
            private bool m_bSkipList;
        }

        /// <summary>
        /// PackagingModelUpdate event. Dispatched whenever the model changes.
        /// </summary>
        public event EventHandler<PackagingModelUpdateEventArgs> PackagingModelUpdate;


        public PackagingEditorModel()
        {
            ClearSettings();
        }

        /// <summary>
        /// Load a layout file.
        /// </summary>
        /// <param name="in_szLayoutFileName">Layout file name</param>
        public void LoadLayout(string in_szLayoutFileName)
        {
            FilePackagerLayout layout = null;

            XmlReaderSettings readerSettings = new XmlReaderSettings();
            readerSettings.ValidationType = ValidationType.Schema;
            string szSchemaPath = System.IO.Path.GetDirectoryName(System.Windows.Forms.Application.ExecutablePath) + "/Package Layout/FilePackagerLayout.xsd";
            string fpl = AkFilePackager.Properties.Resources.FilePackagerLayout;
            System.Xml.XmlTextReader stream = new System.Xml.XmlTextReader(new System.IO.StringReader(fpl));
            readerSettings.Schemas.Add(null, stream);

            using (XmlReader reader = XmlReader.Create(in_szLayoutFileName, readerSettings))
            {
                // Verify the schema version.
                reader.MoveToContent();
                if (reader.GetAttribute("SchemaVersion") != LAYOUT_SCHEMA_VERSION)
                {
                    throw new Exception("Wrong Layout file schema version.");
                }

                // Deserialize.
                System.Xml.Serialization.XmlSerializer serializer = new System.Xml.Serialization.XmlSerializer(typeof(FilePackagerLayout));
                layout = (FilePackagerLayout)serializer.Deserialize(reader);
            }

            ClearLayout();
            System.Diagnostics.Debug.Assert(layout != null);
            m_layout = layout;
            
            // Update model.
            m_szCurrentPersistFile = in_szLayoutFileName;
            m_settings.uDefaultBlockSize = m_layout.DefaultBlockSize;
            m_settings.szFilePackageFilename = m_layout.OutputFileName;
            ReorderFiles();

            OnModelUpdate();
        }

        /// <summary>
        /// Clear the current layout.
        /// </summary>
        public void ClearLayout()
        {
            m_layout = null;
            m_bIsLayoutDirty = false;
            m_szCurrentPersistFile = "";
            ClearSettings();
            ResetFileOrderFromInfoData();

            ReorderFiles();

            OnModelUpdate();
        }

        /// <summary>
        /// Saves the current layout to disk.
        /// The name of the persistence file is stored in this class.
        /// </summary>
        public void SaveLayout()
        {
            System.Diagnostics.Debug.Assert(HasPersistFile);

            // Update layout.
            if (m_layout == null)
                m_layout = new FilePackagerLayout();
            m_layout.SchemaVersion = uint.Parse(LAYOUT_SCHEMA_VERSION);
            m_layout.DefaultBlockSize = m_settings.uDefaultBlockSize;
            m_layout.OutputFileName = m_settings.szFilePackageFilename;
            m_layout.FileList.Clear();
            foreach (OrderedFile file in m_arFileOrder)
	        {
                AK.Wwise.FilePackager.PackageLayout.File laidOutFile  = new AK.Wwise.FilePackager.PackageLayout.File();
                laidOutFile.Id = file.Id;
                laidOutFile.Language = file.Language;
                laidOutFile.Type = file.Type;
                // Mark model as laid out if not missing.
                if (file.Status == OrderedFile.StatusType.FileStatusNew)
                    file.Status = OrderedFile.StatusType.FileStatusLaidOut;
                m_layout.FileList.Add(laidOutFile);
                
	        }


            // Serialize.
            System.Xml.Serialization.XmlSerializer serializer = new System.Xml.Serialization.XmlSerializer(typeof(FilePackagerLayout));
            System.IO.FileStream fileStream = new System.IO.FileStream(m_szCurrentPersistFile, System.IO.FileMode.Create);
            serializer.Serialize(fileStream, m_layout);
            fileStream.Close();

            m_bIsLayoutDirty = false;
            OnModelUpdate();
        }

        /// <summary>
        /// Load an info file.
        /// </summary>
        /// <param name="in_szInfoFilePath">Info file name.</param>
        public void ReadInfoFile(string in_szInfoFilePath)
        {
            m_infoData = InfoFileHelpers.LoadInfoFile(in_szInfoFilePath, "");
            m_szInfoFileName = in_szInfoFilePath;

            System.Diagnostics.Debug.Assert(m_infoData != null);
            
            // Create a default output file path if it was not already set.
            if (m_settings.szFilePackageFilename.Length == 0)
                m_settings.szFilePackageFilename = System.IO.Path.GetDirectoryName(in_szInfoFilePath) + System.IO.Path.DirectorySeparatorChar + "default.pck";

            ResetFileOrderFromInfoData();

            ReorderFiles();

            OnModelUpdate();
        }

        /// <summary>
        /// Remove all files of the layout whose status is "missing".
        /// </summary>
        public void RemoveAllMissingFiles()
        {
            int i = m_arFileOrder.Count - 1;
            while (i >= 0)
            {
                if (m_arFileOrder[i].Status == OrderedFile.StatusType.FileStatusMissing)
                {
                    m_arFileOrder.RemoveAt(i);
                    m_bIsLayoutDirty = true;
                }
                --i;
            }

            OnModelUpdate();
        }

        /// <summary>
        /// Handler for when the laid out files list is reordered.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void listView_Reorder(object sender, ReorderingListView.ReorderEventArgs e)
        {
            System.Diagnostics.Debug.Assert(e.DropIndex <= m_arFileOrder.Count);

            List<OrderedFile> shuffledItems = new List<OrderedFile>();
            for (int i = 0; i < e.SelIndices.Count; i++)
            {
                OrderedFile fileToMove = m_arFileOrder[e.SelIndices[i]];
                System.Diagnostics.Debug.Assert(fileToMove != null);
                shuffledItems.Add(fileToMove);
            }

            int iOriginalDropIndex = e.DropIndex;
            for (int i = e.SelIndices.Count - 1; i >= 0; i--)
            {
                if (e.SelIndices[i] < iOriginalDropIndex)
                    --e.DropIndex;
                m_arFileOrder.RemoveAt(e.SelIndices[i]);
            }
            m_arFileOrder.InsertRange(e.DropIndex, shuffledItems);

            m_bIsLayoutDirty = true;

            // Update model, specifying which indices should be selected (and visible).
            OnModelUpdate(e.DropIndex, shuffledItems.Count);
        }

        /// <summary>
        /// True when the model has the required information to call the file package generator.
        /// </summary>
        public bool CanPerformPackaging
        {
            get { return (m_infoData != null && m_settings.uDefaultBlockSize > 0 && m_settings.szFilePackageFilename.Length > 0 && m_arFileOrder.Count > 0); }
        }

        /// <summary>
        /// True when the layout has been modified.
        /// </summary>
        public bool LayoutDirty
        {
            get { return m_bIsLayoutDirty; }
        }

        /// <summary>
        /// True when the layout has been modified, or a layout was explicitly 
        /// loaded (this test is used to enable the Close Layout command).
        /// </summary>
        public bool HasLayoutEdited
        {
            get { return (m_bIsLayoutDirty || m_layout != null); }
        }

        /// <summary>
        /// Get the file name of the Info file currently loaded.
        /// </summary>
        public string InfoFileName
        {
            get { return m_szInfoFileName; }
        }

        /// <summary>
        /// Get the Info data.
        /// </summary>
        public SoundBanksInfo InfoData
        {
            get { return m_infoData; }
        }
        
        /// <summary>
        /// Get the list of ordered files.
        /// </summary>
        public List<OrderedFile> OrderedFiles
        {
            get { return m_arFileOrder; }
        }

        /// <summary>
        /// Get the package settings defined by the layout.
        /// </summary>
        public PackageSettings Settings
        {
            get { return m_settings; }
        }

        /// <summary>
        /// Setter for the DefaultBlockSize setting.
        /// </summary>
        public uint DefaultBlockSize
        {
            set
            {
                if (m_settings.uDefaultBlockSize != value)
                {
                    m_bIsLayoutDirty = true;
                    m_settings.uDefaultBlockSize = value;
                    OnModelUpdate(true);    // Skip list update.
                }
            }
        }

        /// <summary>
        /// Setter for the OutputFilePath setting.
        /// </summary>
        public string OutputFilePath
        {
            set
            {
                if (m_settings.szFilePackageFilename != value)
                {
                    m_bIsLayoutDirty = true;
                    m_settings.szFilePackageFilename = value;
                    OnModelUpdate(true);    // Skip list update.
                }
            }
        }

        /// <summary>
        /// Helper: assign settings their default values.
        /// </summary>
        private void ClearSettings()
        {
            m_settings.uDefaultBlockSize = 1;
            m_settings.szFilePackageFilename = "";
        }

        /// <summary>
        /// Helper: Reset the file order list, fill with files defined in the Info
        /// data if an Info file was already loaded.
        /// </summary>
        private void ResetFileOrderFromInfoData()
        {
            m_arFileOrder.Clear();
            if (m_infoData != null)
            {
                foreach (FileDescriptorType soundbank in m_infoData.SoundBanks.SoundBankCollection)
                {
                    m_arFileOrder.Add(new OrderedFile(soundbank.Id, soundbank.Language, soundbank.ShortName, AK.Wwise.FilePackager.PackageLayout.Type.SoundBank, OrderedFile.StatusType.FileStatusNew));
                }
                foreach (FileDescriptorType stream in m_infoData.StreamedFiles.FileCollection)
                {
                    m_arFileOrder.Add(new OrderedFile(stream.Id, stream.Language, stream.ShortName, AK.Wwise.FilePackager.PackageLayout.Type.StreamedAudio, OrderedFile.StatusType.FileStatusNew));
                }
            }
        }

        /// <summary>
        /// Helper: Arrange file order based on the files referenced in the Info file and
        /// files referenced in the Layout.
        /// Sets LayoutDirty flag if it encounters "new" files (files that are in the 
        /// Info file but not in the layout).
        /// </summary>
        private void ReorderFiles()
        {
            // Extract all file references from the layout:
            // Those for which a match is found in the file list are added in the temporary
            // list listReorderedFiles (laidout), the others are added in the temporary
            // list listMissingFiles.
            List<OrderedFile> listReorderedFiles = new List<OrderedFile>();
            List<OrderedFile> listMissingFiles = new List<OrderedFile>();
            if (m_layout != null)
            {
                foreach (AK.Wwise.FilePackager.PackageLayout.File laidOutFile in m_layout.FileList.FileCollection)
                {
                    // Search corresponding entry in the list of files.
                    OrderedFile orderedFile = m_arFileOrder.Find(delegate(OrderedFile in_searchedFile) { return in_searchedFile.Id == laidOutFile.Id && in_searchedFile.Language == laidOutFile.Language && in_searchedFile.Type == laidOutFile.Type; });
                    if (orderedFile != null)
                    {
                        // It is there: this file is laid out. Remove and put in temp list to be reinsterted at the beginning.
                        listReorderedFiles.Add(orderedFile);
                        orderedFile.Status = OrderedFile.StatusType.FileStatusLaidOut;
                        m_arFileOrder.Remove(orderedFile);
                    }
                    else
                    {
                        // The file specified in the layout not exist.
                        listMissingFiles.Add(new OrderedFile(laidOutFile.Id, laidOutFile.Language, laidOutFile.Id.ToString(), laidOutFile.Type, OrderedFile.StatusType.FileStatusMissing));
                    }
                }
            }

            // All remaining files are "new". 
            foreach (OrderedFile file in m_arFileOrder)
            {
                file.Status = OrderedFile.StatusType.FileStatusNew;
                m_bIsLayoutDirty = true;
            }

            // Prepend with ordered, laid out files.
            m_arFileOrder.InsertRange(0, listReorderedFiles);

            // Append with missing files.
            m_arFileOrder.AddRange(listMissingFiles);
        }

        /// <summary>
        /// Helper: Dispatch a ModelUpdate event, with default arguments.
        /// </summary>
        private void OnModelUpdate()
        {
            if (PackagingModelUpdate != null)
                PackagingModelUpdate(this, new PackagingModelUpdateEventArgs(false));
        }
        
        /// <summary>
        /// Helper: Dispatch a ModelUpdate event, with default arguments.
        /// </summary>
        private void OnModelUpdate(bool in_bSkipList)
        {
            if (PackagingModelUpdate != null)
                PackagingModelUpdate(this, new PackagingModelUpdateEventArgs(in_bSkipList));
        }

        /// <summary>
        /// Helper: Dispatch a ModelUpdate event, with specific arguments.
        /// </summary>
        /// <param name="in_iFirstSelectedIndex">The first of the selected indices in the file order list.</param>
        /// <param name="in_iNumSelectedIndices">The number of selected indices.</param>
        private void OnModelUpdate(int in_iFirstSelectedIndex, int in_iNumSelectedIndices)
        {
            if (PackagingModelUpdate != null)
                PackagingModelUpdate(this, new PackagingModelUpdateEventArgs(in_iFirstSelectedIndex, in_iNumSelectedIndices, false));
        }

        private PackageSettings m_settings;
        private List<OrderedFile> m_arFileOrder = new List<OrderedFile>();
        private FilePackagerLayout m_layout = null;
        private SoundBanksInfo m_infoData = null;
        private string m_szInfoFileName = "";
        private bool m_bIsLayoutDirty = false;
        
        // Layout persistence file.
        /// <summary>
        /// Get/set layout persistence file name.
        /// </summary>
        public string CurrentPersistFile
        {
            get { return m_szCurrentPersistFile; }
            set
            {
                System.Diagnostics.Debug.Assert(value.Length > 0);
                m_szCurrentPersistFile = value;
            }
        }
        /// <summary>
        /// True when a layout persistence file is specified.
        /// </summary>
        public bool HasPersistFile
        {
            get { return m_szCurrentPersistFile.Length > 0; }
        }
        private string m_szCurrentPersistFile = "";

    }
}
