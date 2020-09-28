using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AK.Wwise.FilePackager.InfoFile;


namespace AkFilePackager
{
    /// <summary>
    /// PackageSettings: Generation settings for file package generator.
    /// </summary>
    public struct PackageSettings
    {
        public uint uDefaultBlockSize;
        public string szFilePackageFilename;
    };

    /// <summary>
    /// ProgressNotificationsDispatcher interface. The FilePackageGenerator 
    /// implements this interface to allow other objects to dispatch notifications 
    /// to progress listeners.
    /// </summary>
    interface IProgressNotificationsDispatcher
    {
        /// <summary>
        /// Send a notification in order to signal that a generation substep 
        /// has been completed.
        /// </summary>
        void NotifySubstep();
        
        /// <summary>
        /// Send a message about the generation progress.
        /// </summary>
        /// <param name="in_szMsg"></param>
        void NotifyLogMsg(string in_szMsg);
    }

    /// <summary>
    /// FilePackageGenerator: Generates a file package based on an InfoFile and some
    /// settings (PackageSettings).
    /// 
    /// Packaged file header format: 
    /// IMPORTANT: KEEP IN SYNC WITH /SDKs/Samples/SoundEngine/Common/PackagedFileDefs.h
    /// 
    /// 'AKPK' (4 bytes)
    /// Header size, excluding the header chunk definition (-8 bytes) (number of bytes) (4 bytes)
    /// Version (4 bytes)
    /// Language names map size (4 bytes) 
    /// SoundBank titles map size (4 bytes) 
    /// SoundBank files LUT size (4 bytes) 
    /// Streamed audio files LUT size (4 bytes) 
    /// Language names map (variable) (sorted by strings)
    /// SoundBank titles map (variable) (sorted by strings)
    /// SoundBank files LUT (variable) (sorted by ID)
    /// Streamed files LUT (variable) (sorted by ID)
    /// 
    /// String maps:
    /// Number of strings (4 bytes)
    /// Array of StringEntry (NSTRINGS * sizeof(StringEntry))
    /// Concatenated strings (Sum((NumChars(i)+Null) * sizeof(WCHAR)))
    /// 
    /// File LUT
    /// Number of files (4 bytes)
    /// Array of FileEntry (NFILES * sizeof(FileEntry))
    /// </summary>
    internal class FilePackageGenerator : IProgressNotificationsDispatcher
    {
        readonly static uint AK_INVALID_LANGUAGE_ID = 0;

        // File package generator events.
        /// <summary>
        /// Register to this event to be notified when the generation step changes.
        /// </summary>
        public event EventHandler<StepChangeEventArgs> StepChange;
        /// <summary>
        /// Register to this event to be notified when the generation substep is incremented.
        /// </summary>
        public event EventHandler<EventArgs> SubStep;
        /// <summary>
        /// Register to this event to be notified of messages about the generation process.
        /// </summary>
        public event EventHandler<LogMsgEventArgs> LogMsg;


        /// <summary>
        /// Generate a file package.
        /// </summary>
        /// <param name="in_soundbanksInfo">Soundbanks data model.</param>
        /// <param name="in_settings">Generation settings.</param>
        /// <param name="in_szOutputFileName">Full path of the file package to be created.</param>
        /// <returns>Returns true when no files are missing.</returns>
        public bool Generate(SoundBanksInfo in_soundbanksInfo, PackageSettings in_settings, List<OrderedFile> in_listOrderedFiles)
        {
            // Open output file.
            FileStream file = new FileStream(in_settings.szFilePackageFilename, FileMode.Create);

            // Create the writer for data.
            FilePackageWriter.Endianness eEndianness;
            switch (in_soundbanksInfo.Platform)
            {
                case Platform.Windows:
                    eEndianness = FilePackageWriter.Endianness.LittleEndian;
                    break;
                default:
                    eEndianness = FilePackageWriter.Endianness.BigEndian;
                    break;
            }
            FilePackageWriter writer = new FilePackageWriter(file, eEndianness);

            // Generate the file package.
            bool bAllFilesExist = GeneratePackage(in_soundbanksInfo, in_settings, writer, in_listOrderedFiles);
            
            writer.Close();
            file.Close();

            return bAllFilesExist;
        }

        
        /// <summary>
        /// Generate the file package.
        /// Creates the package header:
        /// - Header
        /// - Map of language strings
        /// - Map of soundbank titles
        /// - Soundbank files LUT
        /// - Streamed audio files LUT
        /// Writes the header to file.
        /// Concatenates files referenced in the LUTs.
        /// </summary>
        /// <param name="in_soundbanksInfo">Soundbank data model.</param>
        /// <param name="in_settings">Generation settings.</param>
        /// <param name="in_writer">Binary writer.</param>
        /// <returns>Returns true when no files are missing.</returns>
        internal bool GeneratePackage(SoundBanksInfo in_soundbanksInfo, PackageSettings in_settings, FilePackageWriter in_writer, List<OrderedFile> in_listOrderedFiles)
        {
            bool bNoFilesMissing = true;

            const int kNumSubStepsHeader = 6;
            const string kHeaderStepName = "Generating header";
            OnStepChange(kNumSubStepsHeader, kHeaderStepName);

            // Header chunk.
            Header header = new Header();
            OnSubStep();

            // Language names map.
            Dictionary<string, uint> mapLanguageIDs = FindAllLanguages(in_soundbanksInfo);
            LanguagesMap langMap = new LanguagesMap(mapLanguageIDs);
            OnSubStep();
            
            // Banks LUT.
            FileLUT banksLUT = new FileLUT(in_settings.uDefaultBlockSize);
            foreach (FileDescriptorType soundbank in in_soundbanksInfo.SoundBanks.SoundBankCollection)
            {
                if (!banksLUT.Add(soundbank, mapLanguageIDs))
                    bNoFilesMissing = false;
            }
            banksLUT.Sort();
            OnSubStep();
            
            // Steamed files LUT.
            FileLUT streamsLUT = new FileLUT(in_settings.uDefaultBlockSize);
            foreach (FileDescriptorType stream in in_soundbanksInfo.StreamedFiles.FileCollection)
            {
                if (!streamsLUT.Add(stream, mapLanguageIDs))
                    bNoFilesMissing = false;
            }
            streamsLUT.Sort();
            OnSubStep();

            // Find the header size.
            uint uHeaderSize =
                header.SizeOnDisk +
                langMap.MapSizeSize +
                banksLUT.LUTSizeSize +
                streamsLUT.LUTSizeSize +
                langMap.TotalSize +
                banksLUT.TotalSize +
                streamsLUT.TotalSize;
            
            // Prepare files for ordered concatenation.
            FileOrganizer organizer = new FileOrganizer();
            organizer.AddLUT(AK.Wwise.FilePackager.PackageLayout.Type.SoundBank, banksLUT);
            organizer.AddLUT(AK.Wwise.FilePackager.PackageLayout.Type.StreamedAudio, streamsLUT);
            organizer.OrganizeFiles(uHeaderSize, in_listOrderedFiles, mapLanguageIDs, this);
            OnSubStep();


            // Set header size.
            header.HeaderSize = uHeaderSize;


            // Write to output file:

            // Header.
            header.Write(in_writer);
            in_writer.Write(langMap.TotalSize);
            in_writer.Write(banksLUT.TotalSize);
            in_writer.Write(streamsLUT.TotalSize); 
            
            langMap.Write(in_writer);
            banksLUT.Write(in_writer);
            streamsLUT.Write(in_writer);
            OnSubStep();
            

            // Concatenated files.
            const string kConcatenateStepName = "Concatenating files";
            OnStepChange((int)(banksLUT.NumFiles + streamsLUT.NumFiles), kConcatenateStepName);
            organizer.ConcatenateFiles(in_writer, this);

            return bNoFilesMissing;
        }


        

        /// <summary>
        /// Data model helper: Search the model for all languages, create an ID dynamically for each language.
        /// </summary>
        /// <param name="in_infoDOM">Data model.</param>
        /// <returns>A hash [LanguageNameString, GeneratedLanguageID]</returns>
        static internal Dictionary<string, uint> FindAllLanguages(SoundBanksInfo in_infoDOM)
        {
            Dictionary<string, uint> mapLanguages = new Dictionary<string, uint>();
            uint uID = AK_INVALID_LANGUAGE_ID;
            mapLanguages.Add("SFX", uID);

            // Search languages in streamed files.
            foreach (AK.Wwise.FilePackager.InfoFile.File streamedFile in in_infoDOM.StreamedFiles.FileCollection)
            {
                string szLanguage = streamedFile.Language;
                if (!mapLanguages.ContainsKey(szLanguage))
                {
                    ++uID;
                    mapLanguages.Add(szLanguage, uID);
                }
            }

            // Search languages in soundbanks.
            foreach (SoundBank soundBank in in_infoDOM.SoundBanks.SoundBankCollection)
            {
                string szLanguage = soundBank.Language;
                if (!mapLanguages.ContainsKey(szLanguage))
                {
                    ++uID;
                    mapLanguages.Add(szLanguage, uID);
                }
            }

            return mapLanguages;
        }

        // ProgressNotificationsDispatcher implementation.
        /// <summary>
        /// ProgressNotificationsDispatcher implementation.
        /// Dispatches the SubStep event.
        /// </summary>
        public void NotifySubstep()
        {
            OnSubStep();
        }

        /// <summary>
        /// ProgressNotificationsDispatcher implementation.
        /// Dispatches the LogMsg event.
        /// </summary>
        /// <param name="in_szMsg">Message</param>
        public void NotifyLogMsg(string in_szMsg)
        {
            OnLogMsg(in_szMsg);
        }

        /// <summary>
        /// Progress.StopRequestedEventHandler.
        /// Handles "abort generation" events.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void StopRequestedHandler(object sender, EventArgs e)
        {
            m_bStopRequested = true;
        }

        /// <summary>
        /// Returns true if someone requested that the generation be stopped.
        /// </summary>
        public bool StopRequested
        {
            get { return m_bStopRequested; }
        }
        
        /// <summary>
        /// Dispatch the StepChange event.
        /// </summary>
        /// <param name="in_iNumSubSteps">Number of substeps contained in this generation step</param>
        /// <param name="in_szStepName">Name of the generation step</param>
        internal void OnStepChange(int in_iNumSubSteps, string in_szStepName)
        {
            if (StepChange != null)
                StepChange(this, new StepChangeEventArgs(in_iNumSubSteps, in_szStepName));
        }

        /// <summary>
        /// Dispatch the SubStep event.
        /// Note: External stop requests are processed here. An exception is
        /// thrown in order to abort the generation.
        /// </summary>
        internal void OnSubStep()
        {
            if (SubStep != null)
                SubStep(this, new EventArgs());

            // Check for stop request now.
            if (StopRequested)
            {
                throw new Exception("Packaging stopped by user request.");
            }
        }

        /// <summary>
        /// Dispatch the LogMsg event.
        /// </summary>
        /// <param name="in_szMsg">Message</param>
        internal void OnLogMsg(string in_szMsg)
        {
            if (LogMsg != null)
                LogMsg(this, new LogMsgEventArgs(in_szMsg));
        }

        private bool m_bStopRequested = false;
    }

    /// <summary>
    /// Custom event definition for StepChange event.
    /// Provides the number of substeps and name of the step.
    /// </summary>
    public class StepChangeEventArgs : EventArgs
    {
        public StepChangeEventArgs(int in_iNumSubSteps, string in_szStepName)
            : base()
        {
            m_iNumSubSteps = in_iNumSubSteps;
            m_szStepName = in_szStepName;
        }
        public int NumSubSteps
        {
            get { return m_iNumSubSteps; }
            set { m_iNumSubSteps = value; }
        }
        public string StepName
        {
            get { return m_szStepName; }
            set { m_szStepName = value; }
        }
        private int m_iNumSubSteps;
        private string m_szStepName;
    }

    /// <summary>
    /// Custom event definition for LogMsg event.
    /// Provides the message.
    /// </summary>
    public class LogMsgEventArgs : EventArgs
    {
        public LogMsgEventArgs(string in_szMsg)
            : base()
        {
            m_szMsg = in_szMsg;
        }
        public string Msg
        {
            get { return m_szMsg; }
            set { m_szMsg = value; }
        }
        private string m_szMsg;
    }
}
