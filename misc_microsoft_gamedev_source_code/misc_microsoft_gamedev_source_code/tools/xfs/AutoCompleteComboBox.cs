using System;
using System.Windows.Forms;
using System.ComponentModel;

namespace tabComplete
{
    public class AutoCompleteComboBox : System.Windows.Forms.ComboBox
    {
        public event System.ComponentModel.CancelEventHandler NotInList;

        private bool _limitToList = true;
        private bool _inEditMode = false;

        private bool bFireEvent = true;

        public AutoCompleteComboBox()
            : base()
        {
        }

        public bool LimitToList
        {
            get { return _limitToList; }
            set { _limitToList = value; }
        }

        protected virtual void OnNotInList(System.ComponentModel.CancelEventArgs e)
        {
            if (NotInList != null)
            {
                NotInList(this, e);
            }
        }

        protected override void OnTextChanged(System.EventArgs e)
        {
            if (bFireEvent)
            {
                if (_inEditMode)
                {
                    string input = Text;
                    int index = FindString(input);

                    if (index >= 0)
                    {
                        _inEditMode = false;
                        SelectedIndex = index;
                        _inEditMode = true;
                        Select(input.Length, Text.Length);
                    }
                }
            }
            else
            {
                bFireEvent = true;
            }

            base.OnTextChanged(e);
        }

        /*
                protected override void OnValidating(System.ComponentModel.CancelEventArgs e)
                {
                    if (this.LimitToList)
                    {
                        int pos = this.FindStringExact(this.Text);
        
                        if (pos == -1)
                        {
                            OnNotInList(e);
                        }
                        else
                        {
                            this.SelectedIndex = pos;
                        }
                    }

                    base.OnValidating(e);
                }
        */
        protected override void OnKeyDown(System.Windows.Forms.KeyEventArgs e)
        {
            _inEditMode = (e.KeyCode != Keys.Back && e.KeyCode != Keys.Delete);

            if (e.KeyCode == Keys.Down)
            {
                String _base = this.Text.Substring(0, this.SelectionStart);
                foreach (String entry in this.Items)
                {
                    if (entry.StartsWith(_base))
                    {
                        bFireEvent = false;

                        this.Text = entry;
                        this.SelectionStart = _base.Length;
                        this.SelectionLength = this.Text.Length - _base.Length;

                        break;
                    }
                }
            }
            //base.OnKeyDown(e);
        }

        protected override void OnKeyUp(KeyEventArgs e)
        {
            bFireEvent = false;
            base.OnKeyUp(e);
        }
    }
}
