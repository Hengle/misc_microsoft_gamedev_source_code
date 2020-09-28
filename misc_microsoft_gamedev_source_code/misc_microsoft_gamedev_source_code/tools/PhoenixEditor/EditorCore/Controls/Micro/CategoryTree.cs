using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using System.IO;

using System.Xml.Serialization;
using System.Text.RegularExpressions;


namespace EditorCore.Controls.Micro
{
   public partial class CategoryTree : UserControl
   {
      public CategoryTree()
      {
         InitializeComponent();

         mTreeView.MouseDown += new MouseEventHandler(mTreeView_MouseDown);
         mTreeView.AfterSelect += new TreeViewEventHandler(mTreeView_AfterSelect);
         //mTreeView.

         //TreeNode n = new TreeNode("ALL");
         //mTreeView.Nodes.Add(n);

         mTreeView.Tag = mSettings;

      }

      void mTreeView_AfterSelect(object sender, TreeViewEventArgs e)
      {
         Category cat = null;
         if (mTreeView.SelectedNode != null)
         {
            cat = mTreeView.SelectedNode.Tag as Category;
         }
         if (cat != null && SelectedCategoryChanged != null)
         {
            SelectedCategoryChanged.Invoke(this, null);
         }
      }

      public void Setup(CategoryTreeNode settings)//ICollection<string> entries)
      {
         mSettings = settings;
         mTreeView.Tag = mSettings;

         mTreeView.Nodes.Clear();


         mTreeView.Nodes.AddRange(CategoryTreeLogic.BuildTree(mSettings));

         //mTreeView.Nodes.AddRange(CategoryTreeLogic.BuildTreeEditor(mSettings).Nodes);

         //load it up...

      }
      

      CategoryTreeNode mSettings = new CategoryTreeNode();

      void mTreeView_MouseDown(object sender, MouseEventArgs e)
      {
         
         //Control c = sender as Control;
         //TreeView v = c as TreeView;
         //TreeNode n = v.GetNodeAt(e.X, e.Y);
         //if (e.Button == MouseButtons.Right)
         //{
         //   ContextMenu m = CategoryTreeLogic.GetMainMenu(mTreeView, mSettings);
         //   m.Show(c, new Point(0, 0));
         //}
         if (e.Button == MouseButtons.Right)
         {
            CategoryTreeLogic.ShowMenu(mTreeView, mSettings, e);
         }
         if (e.Button == MouseButtons.Left)
         {
            //Category cat = null;
            //if (mTreeView.SelectedNode != null)
            //{
            //   cat = mTreeView.SelectedNode.Tag as Category;
            //}
            //if (cat != null && SelectedCategoryChanged != null)
            //{

            //   SelectedCategoryChanged.Invoke(this, null);
            //}
         }

      }

      public bool Locked = false;


      public Category SelectedCategory
      {
         get
         {
            Category cat = null; 
            if (mTreeView.SelectedNode != null)
            {
               cat = mTreeView.SelectedNode.Tag as Category;
            }
            return cat;
         }
      }
      public event EventHandler SelectedCategoryChanged = null;

      
   }

   //always alpha?
   public class CategoryTreeLogic
   {
      static public void ShowMenu(TreeView tv, CategoryTreeNode settings, MouseEventArgs e)
      {
         TreeNode node = tv.GetNodeAt(e.X, e.Y);
         ContextMenu m = null;
         if (e.Button == MouseButtons.Right)
         {
            if (node != null)
            {
               m = CategoryTreeLogic.GetMenu(tv, node);
            }
            else
            {
               m = CategoryTreeLogic.GetMainMenu(tv, settings);
            }
         }
         if(m!=null)
            m.Show(tv, new Point(e.X, e.Y));

      }


      public static TreeNode[] BuildTree(CategoryTreeNode settings)
      {
         //ICollection<string> names = settings.mAllEntries;

         //TreeNode root = new TreeNode();
         //root.Text = "Root";
         List<TreeNode> nodes = new List<TreeNode>();
         foreach (Category c in settings.Categories)
         {
            TreeNode cat = LoadCategory(c);
            //root.Nodes.Add(cat);
            nodes.Add(cat);
         }

         //return root;
         return nodes.ToArray();
      }
      private static TreeNode LoadCategory(Category c)
      {
         TreeNode category = new TreeNode();
         category.Text = c.Name;
         category.Tag = c;
         foreach (Category subCat in c.Categories)
         {
            TreeNode sub = LoadCategory(subCat);
            category.Nodes.Add(sub);
         }
         return category;
      }

      public static ICollection<string> GetContent(CategoryTreeNode settings , Category cat)//, ICollection<string> allNames)
      {
         //Category cat = node.Tag as Category;
         ICollection<string> allNames = settings.mAllEntries;

         ICollection<string> collection = new List<string>();

         if (cat.Rules.Count > 0)
         {
            ((List<string>)collection).AddRange(allNames);

            foreach (StringRule sr in cat.Rules)
            {
               collection = StringSets.ApplyRule(sr, collection);
            }
         }
         
         foreach (string e in cat.Entries)
         {
            collection.Add(e);
         }

         return collection;

      }



      //Build a tree with built in edit caps
      public static TreeNode BuildTreeEditor(CategoryTreeNode settings)
      {
         TreeNode root = new TreeNode();
         root.Text = "Root";
         foreach (Category c in settings.Categories)
         {
            TreeNode cat = LoadCategoryEditor(c);
            root.Nodes.Add(cat);
         }

         return root;
      }
      private static TreeNode LoadCategoryEditor(Category c)
      {
         TreeNode category = new TreeNode();
         category.Text = c.Name;
         category.Tag = c;
         foreach (Category subCat in c.Categories)
         {
            TreeNode sub = LoadCategoryEditor(subCat);
            category.Nodes.Add(sub);
         }
         foreach (StringRule stringRule in c.Rules)
         {
            TreeNode rule = new TreeNode();
            rule.Text = stringRule.GetDescription();
            rule.Tag = stringRule;
            category.Nodes.Add(rule);
         }

         return category;
      }



      protected static ContextMenu GetMainMenu(TreeView tv, CategoryTreeNode settings)
      {
         ContextMenu menu = new ContextMenu();

         MenuItem addTopCategory = new MenuItem("Add Category");
         addTopCategory.Click += new EventHandler(addTopCategory_Click);
         addTopCategory.Tag = tv;
         menu.MenuItems.Add(addTopCategory);

         MenuItem tempSave = new MenuItem("tempSave");
         tempSave.Click += new EventHandler(tempSave_Click);
         tempSave.Tag = tv;
         menu.MenuItems.Add(tempSave);

         return menu;

      }

      static void tempSave_Click(object sender, EventArgs e)
      {
         TreeView tv = ((MenuItem)sender).Tag as TreeView;
         CategoryTreeNode settings = tv.Tag as CategoryTreeNode;
          
         BaseLoader<CategoryTreeNode>.Save(settings.mFileName, settings);
      }

      static void addTopCategory_Click(object sender, EventArgs e)
      {
         TreeView tv = ((MenuItem)sender).Tag as TreeView;
         CategoryTreeNode settings = tv.Tag as CategoryTreeNode;

         Category cat = new Category();
         cat.Name = "New Category";
         settings.Categories.Add(cat);

         TreeNode category = new TreeNode();
         category.Text = cat.Name;
         category.Tag = cat;
         tv.Nodes.Add(category);
      }

      
      protected static ContextMenu GetMenu(TreeView tv, TreeNode node)
      {
         ContextMenu menu = new ContextMenu();

         Category cat = node.Tag as Category;
         if (cat != null)
         {
            MenuItem editCategory = new MenuItem("Edit");
            editCategory.Click += new EventHandler(editCategory_Click);
            editCategory.Tag = node;
            menu.MenuItems.Add(editCategory);

            //MenuItem rename = new MenuItem("Rename");
            //menu.MenuItems.Add(rename);

            //MenuItem addRule = new MenuItem("Add Rule");
            //addRule.Click += new EventHandler(addRule_Click);
            //addRule.Tag = node;
            //menu.MenuItems.Add(addRule);

            MenuItem addCategory = new MenuItem("Add Category");
            addCategory.Click += new EventHandler(addCategory_Click);
            addCategory.Tag = node;
            menu.MenuItems.Add(addCategory);


            MenuItem deleteCategory = new MenuItem("Delete");
            deleteCategory.Click += new EventHandler(deleteCategory_Click);
            deleteCategory.Tag = node;
            menu.MenuItems.Add(deleteCategory);

            //move up/down

         }
         StringRule rule = node.Tag as StringRule;
         if (rule != null)
         {
            MenuItem editRule = new MenuItem("Edit");
            editRule.Click += new EventHandler(editRule_Click);
            editRule.Tag = node;
            menu.MenuItems.Add(editRule);

            MenuItem deleteRule = new MenuItem("Delete");
            deleteRule.Click += new EventHandler(deleteRule_Click);
            deleteRule.Tag = node;
            menu.MenuItems.Add(deleteRule);
            //move up/down
         }
         //sibligs  ..move up / down

         return menu;
      }

      static void addRule_Click(object sender, EventArgs e)
      {
         throw new Exception("The method or operation is not implemented.");
      }

      static void deleteCategory_Click(object sender, EventArgs e)
      {
         TreeNode node = ((MenuItem)sender).Tag as TreeNode;
         Category cat = node.Tag as Category;

         TreeView tv = node.TreeView;
         CategoryTreeNode settings = tv.Tag as CategoryTreeNode;


         TreeNode parentNode = node.Parent;
         //has  parent
         if (parentNode != null)
         {
            Category parentCat = parentNode.Tag as Category;
            if (parentCat != null)
            {
               parentNode.Nodes.Remove(node);
               parentCat.Categories.Remove(cat);
               return; 
            }
            else
            {
               settings.Categories.Remove(cat);
               parentNode.Nodes.Remove(node);
               return;
            }
         }
         else  //is root
         {
            tv.Nodes.Remove(node);
            settings.Categories.Remove(cat);
            return;
         }

         //parent.Categories.Add(cat);


         //node.Nodes.Add(category);
      }

      static void deleteRule_Click(object sender, EventArgs e)
      {
         throw new Exception("The method or operation is not implemented.");
      }

      static void editRule_Click(object sender, EventArgs e)
      {
         throw new Exception("The method or operation is not implemented.");
      }

      static void editCategory_Click(object sender, EventArgs e)
      {
         TreeNode node = ((MenuItem)sender).Tag as TreeNode;
         Category cat = node.Tag as Category;
         TreeView tv = node.TreeView;
         CategoryTreeNode settings = tv.Tag as CategoryTreeNode;


         CategoryTreeEditor cte = new CategoryTreeEditor();
         cte.Setup(settings, cat);

         //todo edit page
         //todo popup entry editor??
         PopupEditor pe = new PopupEditor();
         Form f = pe.ShowPopup(tv, cte, FormBorderStyle.Sizable, true);
         if (f.ShowDialog() == DialogResult.OK)
         {
            node.Text = cat.Name;

         }
      }

      static void addCategory_Click(object sender, EventArgs e)
      {
         TreeNode node = ((MenuItem)sender).Tag as TreeNode;
         Category parent = node.Tag as Category;

         Category cat = new Category();
         cat.Name = "New Category";
         parent.Categories.Add(cat);

         TreeNode category = new TreeNode();
         category.Text = cat.Name;
         category.Tag = cat;
         node.Nodes.Add(category);
      }

   }


   public class BaseLoader<T> where T : new() 
   {     
      static public T Load(string fileName)
      {
         T settings = default(T);
         try
         {
            if (!File.Exists(fileName))
               return settings;
            XmlSerializer s = new XmlSerializer(typeof(T), new Type[] { });
            Stream st = File.OpenRead(fileName);
            settings = ((T)s.Deserialize(st));
            st.Close();
         }
         catch (System.Exception ex)
         {
            ex.ToString();
         }
         return settings;
      }
      static public void Save(string fileName, T settings)
      {
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(T), new Type[] { });
            Stream st = File.Open(fileName, FileMode.Create);
            s.Serialize(st, settings);
            st.Close();
         }
         catch (System.Exception ex)
         {
            ex.ToString();
         }
      }
   }



   [XmlRoot("CategoryTree")]
   public class CategoryTreeNode   
   {
      [XmlArrayItem(ElementName = "Category", Type = typeof(Category))]
      [XmlArray("Categories")]
      public List<Category> Categories = new List<Category>();

      [XmlIgnore]
      public List<string> mAllEntries = new List<string>();
      [XmlIgnore]
      public string mFileName = "";
   }

   [XmlRoot("Category")]
   public class Category
   {
      [XmlAttribute]
      public string Name = "noName";

      [XmlArrayItem(ElementName = "Entry", Type = typeof(string))]
      [XmlArray("Entries")]
      public List<string> Entries = new List<string>();

      [XmlArrayItem(ElementName = "Rule", Type = typeof(StringRule))]
      [XmlArray("Rules")]
      public List<StringRule> Rules = new List<StringRule>();

      [XmlArrayItem(ElementName = "Category", Type = typeof(Category))]
      [XmlArray("Categories")]
      public List<Category> Categories = new List<Category>();

      
   }

   [XmlRoot("Rule")]
   public class StringRule
   {
      public enum RuleType
      {
         RegexInclude,
         RegexExclude
      };

      [XmlAttribute]
      public RuleType Type = RuleType.RegexInclude;

      [XmlText]
      public string Value = "";

      public string GetDescription()
      {
         return Type.ToString() + " " + Value;
      }
   }

   //[XmlRoot("Entry")]
   //public class Entry
   //{
   //   [XmlAttribute]
   //   public string Name = "";
   //}


}
