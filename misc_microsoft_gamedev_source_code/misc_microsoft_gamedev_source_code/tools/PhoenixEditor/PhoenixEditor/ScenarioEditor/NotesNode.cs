using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using SimEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class NotesNode : PhoenixEditor.ScenarioEditor.ClientNodeControl, ISelectable, IDeletable
   {
      public NotesNode()
      {
         InitializeComponent();

         AddDragSurface(this);
         AddDragSurface(this.NoteTextBox);
         AddDragSurface(this.TitleTextBox);

         AddResizeSurface(this.ResizePanel);

         AddMinMaxButton(this.MinMaxPanel);

         this.NoteTextBox.TextChanged += new EventHandler(stateChanged);
         this.TitleTextBox.TextChanged += new EventHandler(stateChanged);
         this.Move += new EventHandler(stateChanged);
         this.Resize +=new EventHandler(stateChanged);


         //this.DoubleBuffered = true;
         SetStyle(ControlStyles.OptimizedDoubleBuffer, true);

      }


      bool mbPaused = false;
      void stateChanged(object sender, EventArgs e)
      {
         if (mbPaused) return;
         GetData();//does update
      }

      NoteNodeXml mNoteData = new NoteNodeXml();

      public NoteNodeXml GetData()
      {
         mNoteData.X = this.Location.X - mHost.AutoScrollPosition.X;
         mNoteData.Y = this.Location.Y - mHost.AutoScrollPosition.Y;
         mNoteData.Width = this.Width;
         mNoteData.Height = this.Height;
         mNoteData.GroupID = this.GetGroupID();
         mNoteData.Title = this.TitleTextBox.Text;
         mNoteData.Description = this.NoteTextBox.Text;
         return mNoteData;
      }
      public void LoadFromData(NoteNodeXml noteData)
      {
         mNoteData = noteData;
         mbPaused = true;
         this.Left = mNoteData.X;
         this.Top = mNoteData.Y;
         this.Width = mNoteData.Width;
         this.Height = mNoteData.Height;
         this.SetGroupID(mNoteData.GroupID);
         this.TitleTextBox.Text = mNoteData.Title;
         this.NoteTextBox.Multiline = true;
         mNoteData.Description = mNoteData.Description.Replace("\n", Environment.NewLine);
         mNoteData.Description = mNoteData.Description.Replace("\r\r", "\r");
         this.NoteTextBox.Text = mNoteData.Description;
         mbPaused = false;
      }

      TriggerNamespace mParentTriggerNamespace = null;
      public TriggerNamespace ParentTriggerNamespace
      {
         set
         {
            mParentTriggerNamespace = value;
         }
         get
         {
            return mParentTriggerNamespace;
         }
      }

      #region ISelectable Members

      void ISelectable.SelectControl()
      {
         this.BorderStyle = BorderStyle.FixedSingle;
         mbSelected = true;
      }

      void ISelectable.DeSelectControl()
      {
         this.BorderStyle = BorderStyle.None;
         mbSelected = false;
      }
      Rectangle ISelectable.GetBounds()
      {
         return Bounds;
      }

      bool mbSelected = false;
      public bool IsSelected()
      {
         return mbSelected;
      }

      #endregion

      #region IDeletable Members

      public void Delete()
      {
         mParentTriggerNamespace.TriggerEditorData.UIData.mNoteNodes.Remove(mNoteData);
         this.Parent.Controls.Remove(this);
      }

      #endregion
   }
}
