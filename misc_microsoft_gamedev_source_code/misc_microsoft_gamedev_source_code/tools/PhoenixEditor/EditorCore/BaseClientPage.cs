using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore
{
   public partial class BaseClientPage : UserControl
   {
      public BaseClientPage()
      {
         InitializeComponent();
      }

      //this function called when a tab becomes 'active' (tab selected)
      public virtual void Activate() { }

      //this function called when a tab becomes 'deactive' (tab unselected)
      public virtual void Deactivate() { }

      //called when this page is closed from the main tab
      public virtual void destroyPage() { }

      //these two functions are called when the tab is created, and deinitalized respectivly
      public virtual void init() { }
      public virtual void deinit() { }

      //CLM [04.26.06] these functions called for all data that's not in the MANAGED pool for d3d.
      //on a device resize, or reset, these functions are called for you.
      public virtual void initDeviceData() { }
      public virtual void deinitDeviceData() { }

      //override these functions to ensure your app gets the proper processing.
      public virtual void alwaysProcess() { }  //does not get paused, so make this run fast.
      public virtual void input() { }
      public virtual void update() { }
      public virtual void render() { }

      //these functions will be called from the file menu. It's the panel's job to call the proper dialog
      public virtual void save_file() { }
      public virtual void save_file_as() { }

      //this function is called when we've already chosen a file
      public virtual void open_file(string filename) { }
      //this function is called when we haven't chosen a filename yet.
      public virtual void open_file() { }

      //For specific menus and toolbars:
      //public virtual DynamicMenus getDynamicMenus() { return null; }
      protected DynamicMenus mDynamicMenus = new DynamicMenus();
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public DynamicMenus DynamicMenus
      {
         get
         {
            return mDynamicMenus;
         }
      }
      public virtual Control GetUITarget()
      {
         return null;
      }

      protected bool mbPreventDeviceResize = false;
      public bool PreventDeviceResize
      {
         get
         {
            return mbPreventDeviceResize;
         }
         set
         {
            mbPreventDeviceResize = value;
         }
      }
   }

   public class DynamicMenus
   {
      public List<ToolStripMenuItem> mMenus = new List<ToolStripMenuItem>();
      public List<ToolStrip> mToolStrips = new List<ToolStrip>();
      public void BuildDynamicMenus(ToolStripContainer toolStripContainer, MenuStrip menuStrip)
      {
         if (menuStrip != null)
         {

            foreach (ToolStripMenuItem menu in menuStrip.Items)
            {
               this.mMenus.Add(menu);
               List<ToolStripItem> subItems = new List<ToolStripItem>();
               foreach (ToolStripItem subItem in menu.DropDownItems)
               {
                  subItems.Add(subItem);
               }
               menu.Tag = subItems;
            }
         }
         if (toolStripContainer != null)
         {

            foreach (ToolStrip toolStrip in toolStripContainer.TopToolStripPanel.Controls)
            {
               this.mToolStrips.Add(toolStrip);
            }
         }
         if (menuStrip != null)
         {
            menuStrip.Items.Clear();
         }
         if (toolStripContainer != null)
         {
            toolStripContainer.TopToolStripPanel.Controls.Clear();
         }
      }

   }

   public interface IGraphicsWindowOwner
   {
      Control GetGraphicsWindow();
      bool GraphicsEnabled();
   }
}
