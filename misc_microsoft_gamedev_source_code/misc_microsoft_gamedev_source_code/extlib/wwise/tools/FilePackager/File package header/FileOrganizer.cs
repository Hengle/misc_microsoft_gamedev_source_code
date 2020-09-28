using System;
using System.Collections.Generic;
using System.Text;

namespace AkFilePackager
{
    /// <summary>
    /// File organizer: Once the LUTs have been created, one list of files to
    /// concatenate is created through the FileOrganizer. The files of all LUTs can
    /// be placed in any order. When this is done, the (original) entries of all
    /// LUTs are updated with the uStartingBlock. Then the actual files can be concatenated
    /// to the file package.
    /// Usage: Once LUTs are created and the offset of the data section is known,
    /// AddLUT() each LUT and call OrganizeFiles(). This will fill up the organizer's
    /// internal list and update all entries' uStartingBlock.
    /// When the header is written to file, call ConcatenateFiles() to copy the content
    /// of each file referenced by the file entries to the file package.
    /// </summary>
    internal class FileOrganizer
    {
        public FileOrganizer()
        {
        }

        public void AddLUT(AK.Wwise.FilePackager.PackageLayout.Type in_eType, FileLUT fileLUTs)
        {
            m_arFileLUTs[in_eType] = fileLUTs;
        }

        public void OrganizeFiles(ulong in_uDataOffset, List<OrderedFile> in_listOrderedFiles, Dictionary<string, uint> in_mapLanguages, IProgressNotificationsDispatcher in_notifDispatcher)
        {
            // Build list of files based on specified file order. 
            if (in_listOrderedFiles != null)
            {
                foreach (OrderedFile orderedFile in in_listOrderedFiles)
                {
                    // Select proper LUT.
                    FileLUT lut = m_arFileLUTs[orderedFile.Type];
                    
                    // Find file (binary search: files in LUTs are sorted in ID/LanguageID.
                    uint uLanguageID;
                    if (in_mapLanguages.TryGetValue(orderedFile.Language, out uLanguageID))
                    {
                        int iFileIndex = lut.FindEntry(orderedFile.Id, uLanguageID);
                        if (iFileIndex >= 0)
                        {
                            FileLUT.IncludedFile file = lut.GetAt(iFileIndex);

                            // File was laid out. 
                            // Set starting block, mark as ready.
                            AddOrganizedFile(file, ref in_uDataOffset);
                        }
                        else
                        {
                            // File specified in layout is not in the LUT.
                            in_notifDispatcher.NotifyLogMsg("WARNING: File " + orderedFile.ShortName + " specified in the layout is missing.");
                        }
                    }
                    else
                    {
                        // Invalid language.
                        in_notifDispatcher.NotifyLogMsg("WARNING: File " + orderedFile.ShortName + " specified in the layout has invalid language + " + orderedFile.Language + ".");
                    }
                }
            }

            // Default logic: add files of all LUTs in order.

            // Set each file's starting block.
            foreach (FileLUT lut in m_arFileLUTs.Values)
            {
                foreach (FileLUT.IncludedFile file in lut)
                {
                    if (!file.Included)
                    {
                        AddOrganizedFile(file, ref in_uDataOffset);
                    }
                }
            }
        }

        public void ConcatenateFiles(FilePackageWriter in_writer, IProgressNotificationsDispatcher in_notifDispatcher)
        {
            foreach (FileLUT.IncludedFile file in m_arOrganizedFileEntries)
            {
                // Add padding so that next file falls on a block boundary.
                PadToBlock(in_writer, file.uBlockSize, in_writer.Position);

                // At this point we know the file exists. 
                System.Diagnostics.Debug.Assert(file.szPath.Length > 0 && System.IO.File.Exists(file.szPath));

                in_notifDispatcher.NotifyLogMsg("Copying file " + file.szPath);

                // Copy file.
                in_writer.Write(System.IO.File.ReadAllBytes(file.szPath));

                in_notifDispatcher.NotifySubstep();
            }
        }

        private void AddOrganizedFile(FileLUT.IncludedFile in_file, ref ulong io_uDataOffset)
        {
            System.Diagnostics.Debug.Assert(!in_file.Included);
            io_uDataOffset += ComputePaddingSize(in_file.uBlockSize, io_uDataOffset);
            // Starting block is expressed in terms of this file's own block size.
            System.Diagnostics.Debug.Assert(io_uDataOffset % in_file.uBlockSize == 0);
            in_file.uStartingBlock = (uint)(io_uDataOffset / in_file.uBlockSize);
            m_arOrganizedFileEntries.Add(in_file);
            in_file.Included = true;
            io_uDataOffset += in_file.uFileSize;
        }

        /// <summary>
        /// File helper: Compute padding size based on the required alignment and current offset.
        /// </summary>
        /// <param name="in_uBlockSize">Required alignment.</param>
        /// <param name="in_uOffset">Current offset.</param>
        /// <returns>Number of bytes to be written to file to meet the required alignment.</returns>
        internal static uint ComputePaddingSize(uint in_uBlockSize, ulong in_uOffset)
        {
            return (uint)((in_uBlockSize * (uint)((in_uOffset + in_uBlockSize - 1) / in_uBlockSize)) - in_uOffset);
        }

        /// <summary>
        /// File helper: Write a certain amount of padding zeros to file.
        /// </summary>
        /// <param name="in_writer">Binary writer.</param>
        /// <param name="in_uPadSize">Number of zeroed bytes to be written.</param>
        internal static void Pad(FilePackageWriter in_writer, uint in_uPadSize)
        {
            if (in_uPadSize > 0)
            {
                byte[] padding = new byte[in_uPadSize];
                in_writer.Write(padding);
            }
        }

        /// <summary>
        /// File helper: Write padding zeros to file to meet the required alignment.
        /// </summary>
        /// <param name="in_writer">Binary writer.</param>
        /// <param name="in_uBlockSize">Required alignment.</param>
        /// <param name="in_uOffset">Current offset.</param>
        internal static void PadToBlock(FilePackageWriter in_writer, uint in_uBlockSize, ulong in_uOffset)
        {
            Pad(in_writer, ComputePaddingSize(in_uBlockSize, in_uOffset));
        }

        private Dictionary<AK.Wwise.FilePackager.PackageLayout.Type, FileLUT> m_arFileLUTs = new Dictionary<AK.Wwise.FilePackager.PackageLayout.Type, FileLUT>();
        private List<FileLUT.IncludedFile> m_arOrganizedFileEntries = new List<FileLUT.IncludedFile>();
    };
}
