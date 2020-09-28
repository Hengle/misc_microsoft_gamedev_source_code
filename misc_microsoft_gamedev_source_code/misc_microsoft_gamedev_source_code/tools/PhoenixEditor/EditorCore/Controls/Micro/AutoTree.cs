using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using System.Text.RegularExpressions;


namespace EditorCore.Controls.Micro
{
   public partial class AutoTree : UserControl
   {
      public AutoTree()
      {
         InitializeComponent();
         //mTreeView.NodeMouseClick += new TreeNodeMouseClickEventHandler(mTreeView_NodeMouseClick);
         mTreeView.AfterSelect += new TreeViewEventHandler(mTreeView_AfterSelect);
      }

      void mTreeView_AfterSelect(object sender, TreeViewEventArgs e)
      {
         string tagString = e.Node.Tag as string;  //only content nodes have the tag set
         if (tagString != null)
         {
            mTreeView.SelectedNode = e.Node;
            SelectedItemChanged.Invoke(this, null);
         }
      }

      public void Clear()
      {
         mTreeView.Nodes.Clear();
      }

      //void mTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
      //{
      //   mTreeView.SelectedNode = e.Node;
      //   SelectedItemChanged.Invoke(this, null);
      //}

      public void SetContent(ICollection<string> content)
      {
         TreeNode root = SmartStringTree.BuildTree(content, new string[] { "KBSF" });

         mTreeView.Nodes.Add(root);
         mTreeView.ExpandAll();

      }

      public event EventHandler SelectedItemChanged = null;

      public string SelectedItem
      {
         get
         {
            string item = "";
            if (mTreeView.SelectedNode != null)
            {
               item = mTreeView.SelectedNode.Text;
            }
            return item;
         }
      }

   }

   public class RegexFactory
   {
      static Dictionary<string, Regex> mRegexMap = new Dictionary<string, Regex>();
      //doesn't provide for regex options yet... to avoid caching problems
      public static Regex GetRegex(string desc)
      {
         Regex r;
         if (mRegexMap.TryGetValue(desc, out r) == false)
         {
            r = new Regex(desc);
         }
         return r;
      }

   }


   public class StringSets
   {
      public static Dictionary<string, List<string>> BuildMap(ICollection<string> rawInput, ICollection<string> toIgnore, int minMatches)
      {
         Dictionary<string, List<string>> map = new Dictionary<string, List<string>>();
         foreach (string s in rawInput)
         {
            string[] camelSplit = Regex.Split(s, @"(?<!^)(?=[A-Z])");
            s.GetHashCode();
            string buffer = "";
            foreach (string w in camelSplit)
            {
               if (w.Length == 1)
               {
                  buffer += w;
                  continue;
               }
               else
               {
                  if (buffer.Length > 0)
                  {
                     if (map.ContainsKey(buffer) == false)
                     {
                        map[buffer] = new List<string>();
                     }
                     if (buffer.Contains("(") || buffer.Contains(")") || toIgnore.Contains(buffer))
                     {
                        //skip it...
                     }
                     else
                     {
                        map[buffer].Add(s);
                     }
                  }
                  buffer = "";
               }
               if (map.ContainsKey(w) == false)
               {
                  map[w] = new List<string>();
               }
               if (w.Contains("(") || w.Contains(")") || toIgnore.Contains(w))
                  continue;
               map[w].Add(s);
            }
         }

         List<string> toRemove = new List<string>();

         Dictionary<string, List<string>>.Enumerator it = map.GetEnumerator();
         while (it.MoveNext())
         {
            if (it.Current.Value.Count < minMatches)
            {
               toRemove.Add(it.Current.Key);
            }
         }
         foreach (string s in toRemove)
         {
            map.Remove(s);
         }
         return map;
      }


      public static ICollection<string> ApplyRule(StringRule rule, ICollection<string> input)
      {
         List<string> output = new List<string>();
         if (rule.Type == StringRule.RuleType.RegexInclude)
         {
            Regex r = RegexFactory.GetRegex(rule.Value);

            foreach (string s in input)
            {
               if (r.IsMatch(s))
               {
                  output.Add(s);
               }
            }
         }
         return output;
      }

   }



   public class SmartStringTree
   {
      static int compareIntString(Pair<int, string> a, Pair<int, string> b)
      {
         return b.Key - a.Key;
      }

      static public TreeNode BuildTree(ICollection<string> rawInput, ICollection<string> toIgnore)
      {
         Dictionary<string, List<string>> groupsMap = new Dictionary<string, List<string>>();
         groupsMap = StringSets.BuildMap(rawInput, toIgnore, 3);
         return BuildTree(groupsMap, rawInput);
      }

      static public TreeNode BuildTree(Dictionary<string, List<string>> groupsMap, ICollection<string> originalList)
      {
         int minCount = 3;

         TreeNode root = new TreeNode();
         root.Text = "ALL";

         List<string> topLevel = new List<string>();
         List<string> usedGroups = new List<string>();
         List<string> usedEntries = new List<string>();

         List<Pair<int, string>> sizes = new List<Pair<int, string>>();
         Dictionary<string, List<string>>.Enumerator it2 = groupsMap.GetEnumerator();
         while (it2.MoveNext())
         {
            sizes.Add(new Pair<int, string>(it2.Current.Value.Count, it2.Current.Key));
         }
         sizes.Sort(compareIntString);

         List<string> toIgnore = new List<string>();
         foreach (Pair<int, string> pair in sizes)
         {
            string thisGroup = pair.Value;
            if (usedGroups.Contains(thisGroup))
               continue;

            //TreeNode n = new TreeNode();
            //n.Text = thisGroup;
            //root.Nodes.Add(n);

            toIgnore.Clear();
            toIgnore.Add(thisGroup);
            List<string> values = groupsMap[thisGroup];

            Dictionary<string, List<string>> moreSubStrings = StringSets.BuildMap(values, toIgnore, 3);
            //add substrings ... 
            Dictionary<string, List<string>>.Enumerator it3 = moreSubStrings.GetEnumerator();
            while (it3.MoveNext())
            {
               string subGroup = it3.Current.Key;
               if (usedGroups.Contains(subGroup))
                  continue;

               if (!groupsMap.ContainsKey(subGroup) || Set.IsSubset<string>(groupsMap[subGroup], it3.Current.Value) == false)
               {
                  continue;
               }

               //TreeNode n2 = new TreeNode();
               //n2.Text = subGroup;
               //n.Nodes.Add(n2);

               usedGroups.Add(subGroup);
            }

            usedGroups.Add(thisGroup);
            topLevel.Add(thisGroup);
         }

         //sizes.Sort( new delegate Comparison<Pair<int,string>>(Pair<int,string> a, Pair<int,string> b){ return    } );

         topLevel.Sort();

         foreach (string s in topLevel)
         {
            TreeNode n = new TreeNode();
            n.Text = s;
            foreach (string subEntry in groupsMap[s])
            {
               TreeNode n2 = new TreeNode();
               n2.Text = subEntry;
               n2.Tag = subEntry;
               n.Nodes.Add(n2);

               usedEntries.Add(subEntry);
            }
            root.Nodes.Add(n);
         }

         TreeNode unsorted = new TreeNode();
         unsorted.Text = "UNSORTED";
         foreach (string s in originalList)
         {
            if (usedEntries.Contains(s) == false)
            {
               TreeNode n2 = new TreeNode();
               n2.Text = s;
               n2.Tag = s;
               unsorted.Nodes.Add(n2);
            }
         }
         root.Nodes.Add(unsorted);

         return root;
      }


   }


}
