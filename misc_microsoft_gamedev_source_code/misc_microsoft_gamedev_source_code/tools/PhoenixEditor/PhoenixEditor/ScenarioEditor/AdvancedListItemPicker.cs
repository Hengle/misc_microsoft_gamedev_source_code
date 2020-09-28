using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class AdvancedListItemPicker : UserControl
   {
      public AdvancedListItemPicker()
      {
         InitializeComponent();
      }

      string mSelectedItem = "";
      public string SelectedItem
      {
         get
         {
            return mSelectedItem;
         }
      }

      public event EventHandler ListItemSelected = null;

      List<string> mChoices = new List<string>();

      public void SetAllChoices(ICollection<string> choices)
      {
         mChoices.Clear();
         mChoices.AddRange( choices);

         TabPage p = new TabPage();
         p.Text = "All";
         tabControl1.Controls.Add(p);
         ListView v = new ListView();
         v.Sorting = SortOrder.Ascending;
         foreach(string s in choices)
         {
            v.Items.Add(s);
         }
         p.Controls.Add(v);
         v.Dock = DockStyle.Fill;
         v.View = View.List;
         v.ItemSelectionChanged += new ListViewItemSelectionChangedEventHandler(v_ItemSelectionChanged);

      }

      void v_ItemSelectionChanged(object sender, ListViewItemSelectionChangedEventArgs e)
      {
         mSelectedItem = e.Item.Text;
         if (ListItemSelected != null)
            ListItemSelected.Invoke(this, null);
      }

      public void AddTab(string tabName, Dictionary<string, List<string>> listGroups)
      {
         TabPage p = new TabPage();

         tabControl1.Controls.Add(p);
         p.Text = tabName;
         FlowLayoutPanel fp = new FlowLayoutPanel();
         
         fp.FlowDirection = FlowDirection.TopDown;
         //fp.Height = 3000;
         //fp.Width = 600;

         // = true;
         fp.AutoScroll = true;
         p.Controls.Add(fp);
         fp.Dock = DockStyle.Fill;
         List<string> sortedGroups = new List<string>();
  
         sortedGroups.AddRange(listGroups.Keys);
         sortedGroups.Sort();
           
         foreach (string groupName in sortedGroups)
         {
            List<string> group = listGroups[groupName];

            int h = group.Count * 13 + 50;
            h = Math.Max(50, h);
            h = Math.Min(300, h);

            GroupBox box = new GroupBox();

            box.Height = h;

            box.Text = groupName;
            ListBox lb = new ListBox();

            foreach (string s in group)
            {
               lb.Items.Add(s);
            }
            box.Controls.Add(lb);
            lb.Dock = DockStyle.Fill;
            fp.Controls.Add(box);
            lb.SelectedIndexChanged += new EventHandler(lb_SelectedIndexChanged);
         }


      }

      void lb_SelectedIndexChanged(object sender, EventArgs e)
      {
         ListBox lb = sender as ListBox;
         if (lb != null && lb.SelectedItem != null)
         {
            mSelectedItem = lb.SelectedItem.ToString();
            if (ListItemSelected != null)
               ListItemSelected.Invoke(this, null);
         }
      }


   }
}
