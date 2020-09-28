using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using AK.Wwise.FilePackager.InfoFile;
using AK.Wwise.FilePackager.PackageLayout;

namespace AkFilePackager
{
    /// <summary>
    /// Edit mode form.
    /// </summary>
    public partial class EditModeForm : Form
    {
        readonly static uint MAX_DEFAULT_BLOCK_SIZE = 1 * 1024 * 1024;   // 1 Mb.
        
        public EditModeForm()
        {
            InitializeComponent();

            listViewFiles.Reorder += new EventHandler<ReorderingListView.ReorderEventArgs>(m_model.listView_Reorder);
            listViewFiles.GridLines = true;

            m_model.PackagingModelUpdate += new EventHandler<PackagingEditorModel.PackagingModelUpdateEventArgs>(ModelUpdated);
            UpdateView(0, 0, true);
        }

        private void textBoxDefaultBlockSize_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!char.IsDigit( e.KeyChar )
                && e.KeyChar != 8)  // backspace.
            {
                e.Handled = true;
            }
        }

        private void textBoxDefaultBlockSize_Validating(object sender, CancelEventArgs e)
        {
            if (sender != textBoxDefaultBlockSize)
                return;
            try
            {
                uint uDefaultBlockSize = uint.Parse(textBoxDefaultBlockSize.Text);
                if (uDefaultBlockSize > 0 && uDefaultBlockSize <= MAX_DEFAULT_BLOCK_SIZE)
                    m_model.DefaultBlockSize = uDefaultBlockSize;
                else
                    throw new Exception();
            }
            catch
            {
                e.Cancel = true;
            }
        }

        private void textBoxOutputFilePath_Validating(object sender, CancelEventArgs e)
        {
            if (sender != textBoxOutputFilePath)
                return;
            m_model.OutputFilePath = textBoxOutputFilePath.Text;
        }

        private void menuStrip1_MenuActivate(object sender, EventArgs e)
        {
            // Ensure that the model is consistent with the view.
            this.ValidateChildren();
        }

        private void menuFileOpen_Click(object sender, EventArgs e)
        {
            if (!EnsureLayoutSaved())
                return;

            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Filter = "XML files (*.xml)|*.xml|All files (*.*)|*.*";
            dialog.FilterIndex = 1;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    m_model.LoadLayout(dialog.FileName);
                }
                catch (Exception ex)
                {
                    MessageBox.Show("FAILED loading Layout: " + ex.Message, Program.APP_NAME);
                }
            }
        }

        private void menuFileSave_Click(object sender, EventArgs e)
        {
            try
            {
                SaveLayout();
            }
            catch (Exception ex)
            {
                MessageBox.Show("FAILED saving Layout: " + ex.Message, Program.APP_NAME);
            }
        }

        private void menuFileSaveAs_Click(object sender, EventArgs e)
        {
            try
            {
                SaveLayoutAs();
            }
            catch (Exception ex)
            {
                MessageBox.Show("FAILED saving Layout: " + ex.Message, Program.APP_NAME);
            }
        }

        private void closeLayoutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!EnsureLayoutSaved())
                return;

            m_model.ClearLayout();
        }

        private void SaveLayout()
        {
            if (m_model.HasPersistFile)
                m_model.SaveLayout();
            else
                SaveLayoutAs();
        }

        private void SaveLayoutAs()
        {
            SaveFileDialog dialog = new SaveFileDialog();
            dialog.Filter = "XML files (*.xml)|*.xml|All files (*.*)|*.*";
            dialog.FilterIndex = 1;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                m_model.CurrentPersistFile = dialog.FileName;
                m_model.SaveLayout();
            }
        }

        private void menuFileLoadInfoFile_Click(object sender, EventArgs e)
        {
            if (!EnsureLayoutSaved())
                return;

            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Filter = "XML files (*.xml)|*.xml|All files (*.*)|*.*";
            dialog.FilterIndex = 1;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                PopulateFromInfoFile(dialog.FileName);
            }
        }

        private void PopulateFromInfoFile(string in_szInfoFileName)
        {
            try
            {
                m_model.ReadInfoFile(in_szInfoFileName);
            }
            catch (Exception ex)
            {
                MessageBox.Show("FAILED loading Info file: " + ex.Message, Program.APP_NAME);
            }
        }

        private void removeAllMissingFilesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            m_model.RemoveAllMissingFiles();
        }

        /// <summary>
        /// Handler of Generate File Package menu item.
        /// The progress bar dialog is created, displayed, and glued to the notifications of the 
        /// file package generator.  
        /// The file package generator is launched with the settings stored in the 
        /// packaging editor model.
        /// </summary>
        private void generateFilePackageToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (m_model.CanPerformPackaging)
            {
                Progress progressDlg = new Progress();
                
                FilePackageGenerator generator = new FilePackageGenerator();
                generator.StepChange += progressDlg.ProgressStepHandler;
                generator.SubStep += progressDlg.StepHandler;
                generator.LogMsg += progressDlg.LogMsgHandler;

                progressDlg.StopRequested += generator.StopRequestedHandler;
                progressDlg.Show();

                try
                {
                    if (!generator.Generate(m_model.InfoData, m_model.Settings, m_model.OrderedFiles))
                    {
                        progressDlg.LogMsg("WARNING: some files referenced in " + m_model.InfoFileName + " could not be found.");
                    }
                    progressDlg.LogMsg("SUCCESSFUL file package generation: " + m_model.Settings.szFilePackageFilename);
                }
                catch (Exception ex)
                {
                    progressDlg.LogMsg("File packaging FAILED! " + ex.Message);
                    MessageBox.Show("File packaging FAILED!\n\n" + ex.Message, Program.APP_NAME);
                }
                
                progressDlg.Finished();
            }
        }

        private void quitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            // EnsureLayoutSaved() performed in Closing handler.
            base.Close();
        }

        private void EditMode_FormClosing(object sender, FormClosingEventArgs e)
        {
            e.Cancel = !EnsureLayoutSaved();
        }

        /// <summary>
        /// Checks the LayoutDirty flags, asks the user if the layout should be
        /// saved and save it if applicable.
        /// </summary>
        /// <returns>False if operation should be cancelled.</returns>
        private bool EnsureLayoutSaved()
        {
            if (m_model.LayoutDirty)
            {
                DialogResult eResult = MessageBox.Show("Do you wish to save the current layout?", Program.APP_NAME, MessageBoxButtons.YesNoCancel);
                switch (eResult)
                {
                    case DialogResult.Yes:
                        {
                            try
                            {
                                SaveLayout();
                            }
                            catch (Exception ex)
                            {
                                MessageBox.Show("FAILED saving Layout: " + ex.Message, Program.APP_NAME);
                                return false;
                            }
                        }
                        break;
                    case DialogResult.No:
                        break;
                    default:
                        return false;
                }
            }
            return true;
        }

        /// <summary>
        /// ModelUpdated event handler.
        /// </summary>
        private void ModelUpdated(object sender, PackagingEditorModel.PackagingModelUpdateEventArgs e)
        {
            if (sender != m_model)
                return;

            UpdateView(e.FirstSelectedIndex, e.NumSelectedIndices, e.ListChanged);
        }

        /// <summary>
        /// Update all view controls with the model.
        /// </summary>
        /// <param name="in_iFirstSelectedIndex">List item index to select/display</param>
        /// <param name="in_iNumSelectedIndices">Number of selected items (always contiguous)</param>
        /// <param name="in_bListChanged">True when list has changed (avoid refreshing it for nothing)</param>
        private void UpdateView(int in_iFirstSelectedIndex, int in_iNumSelectedIndices, bool in_bListChanged)
        {
            textBoxInfoFile.Text = (m_model.InfoFileName.Length > 0) ? m_model.InfoFileName : "<none>";

            PackageSettings settings = m_model.Settings;
            textBoxDefaultBlockSize.Text = settings.uDefaultBlockSize.ToString();
            textBoxOutputFilePath.Text = settings.szFilePackageFilename;

            if (in_bListChanged)
            {
                listViewFiles.BeginUpdate();
                listViewFiles.Items.Clear();
                foreach (OrderedFile file in m_model.OrderedFiles)
                {
                    ListHelpers.AddOrderedFile(listViewFiles, file);
                }
                if (in_iNumSelectedIndices > 0)
                {
                    listViewFiles.Items[in_iFirstSelectedIndex + in_iNumSelectedIndices - 1].EnsureVisible();
                    for (int i = 0; i < in_iNumSelectedIndices; i++)
                        listViewFiles.Items[in_iFirstSelectedIndex + i].Selected = true;
                    listViewFiles.Items[in_iFirstSelectedIndex].EnsureVisible();
                }
                listViewFiles.EndUpdate();
            }

            // Window title.
            if (m_model.HasPersistFile)
            {
                this.Text = Program.APP_NAME + " - " + System.IO.Path.GetFileName(m_model.CurrentPersistFile);
                if (m_model.LayoutDirty)
                    this.Text += " *";
            }

            closeLayoutToolStripMenuItem.Enabled = m_model.HasLayoutEdited;
            generateFilePackageToolStripMenuItem.Enabled = m_model.CanPerformPackaging;
        }

        private PackagingEditorModel m_model = new PackagingEditorModel();
    }

    /// <summary>
    /// Helpers for ListView displaying of ordered files.
    /// </summary>
    public class ListHelpers
    {
        public static void AddOrderedFile(ListView in_list, OrderedFile in_file)
        {
            ListViewItem item = new ListViewItem(in_file.ShortName);

            item.SubItems.Add(FileTypeToString(in_file.Type));
            item.SubItems.Add(in_file.Language);

            item.UseItemStyleForSubItems = false;
            Color statusColor;
            string szStatus = StatusTypeToString(in_file.Status, out statusColor);
            item.SubItems.Add(szStatus, statusColor, in_list.BackColor, in_list.Font);
            in_list.Items.Add(item);
        }

        internal static string FileTypeToString(AK.Wwise.FilePackager.PackageLayout.Type in_type)
        {
            string szString;
            switch (in_type)
            {
                case AK.Wwise.FilePackager.PackageLayout.Type.StreamedAudio:
                    szString = "Streamed audio";
                    break;
                case AK.Wwise.FilePackager.PackageLayout.Type.SoundBank:
                    szString = "Sound bank";
                    break;
                default:
                    szString = "Invalid type";
                    break;
            }
            return szString;
        }

        internal static string StatusTypeToString(OrderedFile.StatusType in_status, out Color out_color)
        {
            string szString;
            switch (in_status)
            {
                case OrderedFile.StatusType.FileStatusLaidOut:
                    out_color = Color.FromKnownColor(KnownColor.Black);
                    szString = "Laid out";
                    break;
                case OrderedFile.StatusType.FileStatusNew:
                    out_color = Color.FromKnownColor(KnownColor.Blue);
                    szString = "[New]";
                    break;
                case OrderedFile.StatusType.FileStatusMissing:
                    out_color = Color.FromKnownColor(KnownColor.Red);
                    szString = "[Missing]";
                    break;
                default:
                    System.Diagnostics.Debug.Assert(false);
                    out_color = Color.FromKnownColor(KnownColor.Black);
                    szString = "";
                    break;
            }
            return szString;
        }
    }
}