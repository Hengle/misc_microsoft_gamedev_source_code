using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Serialization;

using SimEditor;
using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{

   public partial class TriggerEditor : UserControl, IGraphicsWindowOwner
   {
      public TriggerEditor()
      {
         try
         {
            InitializeComponent();
            triggerHostArea1.ViewChanged += new EventHandler(triggerHostArea1_ViewChanged);

            TriggerSystemMain.Init();


            previewNavWindow1.BorderStyle = BorderStyle.FixedSingle;

            mMinPanelSize = panel1.Size;
            mMaxPanelSize = new Size(mMinPanelSize.Width + 800, mMaxPanelSize.Height + 600);
            this.Resize += new EventHandler(TriggerEditor_Resize);

         }
         catch(System.Exception ex)
         {

         }
         triggerHostArea1.ScanComplete += new EventHandler(triggerHostArea1_ScanComplete);
         ErrorPanel.Click += new EventHandler(ErrorPanel_Click);
      }

      void ErrorPanel_Click(object sender, EventArgs e)
      {
         triggerHostArea1.Scan(false);
      }

      void triggerHostArea1_ScanComplete(object sender, EventArgs e)
      {
         if (triggerHostArea1.ErrorCount > 0)
         {
            ErrorPanel.BackColor = Color.Red;
         }
         else if (triggerHostArea1.WarningCount > 0)
         {
            ErrorPanel.BackColor = Color.Yellow;
         }
         else
         {
            ErrorPanel.BackColor = Color.Green;
         }
      }




      void TriggerEditor_Resize(object sender, EventArgs e)
      {
         panel1.Left = this.Parent.Width - mMaxPanelSize.Width;

         //throw new Exception("The method or operation is not implemented.");
      }
      Size mMinPanelSize;
      Size mMaxPanelSize;

      public BaseClientPage mBaseClientParent = null;

      void triggerHostArea1_ViewChanged(object sender, EventArgs e)
      {
         this.previewNavWindow1.MasterControl = triggerHostArea1.ActiveNodeHostControl;
      }
      private TriggerArmiesRoot mTrigArmies = new TriggerArmiesRoot();
      UniqueID mArmyIDs = new UniqueID();

      TriggerSystemMain mMainTriggerSystem = new TriggerSystemMain();
   
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public TriggerRoot TriggerData
      {
         set
         {
            if (value == null)
            {
               mMainTriggerSystem = new TriggerSystemMain();
               mMainTriggerSystem.TriggerData = new TriggerRoot();

            }
            else
            {
               mMainTriggerSystem.TriggerData = value;

               if (mMainTriggerSystem == null)
               {
                  mMainTriggerSystem = new TriggerSystemMain();
                  mMainTriggerSystem.TriggerData = new TriggerRoot();
               }
               if (triggerHostArea1 != null)// && triggerHostArea1.CurrentTriggerNamespace == null)
               {
                  //load ui
                  triggerHostArea1.CurrentTriggerNamespace = mMainTriggerSystem.MainNamespace;
               }
            }

            //this.previewNavWindow1.MasterControl = triggerHostArea1.ActiveNodeHostControl;

         }
         get
         {
            if (mMainTriggerSystem == null)
            {
               mMainTriggerSystem = new TriggerSystemMain();
               mMainTriggerSystem.TriggerData = new TriggerRoot();

            }

            triggerHostArea1.Scan(true);

            return mMainTriggerSystem.TriggerData;
         }
      }

      public Control GetGraphicsWindow()
      {
         return this.panel1;
         //if (mMaximizedPanel == true)
         //{

         //   return this.panel1;
         //}
         //else
         //{
         //   return null;
         //}
      }
      public bool GraphicsEnabled()
      {

         return mMaximizedPanel;
      }

      bool mMaximizedPanel = false;

      void MaximizePanel()
      {
         if(mMaximizedPanel == false)
         {
            mMaximizedPanel = true;

            panel1.Size = mMaxPanelSize;

            panel1.Left = this.Parent.Width - mMaxPanelSize.Width;
            //mMaxPanelSize.Width

            if (mBaseClientParent != null)
            {
               mBaseClientParent.Activate();
            }
         }
      }
      void MinimizePanel()
      {
         if (lockedOn == true)
            return;
         if (mMaximizedPanel == true)
         {
            mMaximizedPanel = false;

            panel1.Size = mMinPanelSize;

            if (mBaseClientParent != null)
            {
               //mBaseClientParent.Activate();
            }
         }
      }


      private void panel1_MouseEnter(object sender, EventArgs e)
      {
         MaximizePanel();
      }

      private void panel1_MouseLeave(object sender, EventArgs e)
      {
         MinimizePanel();
      }


      bool lockedOn = false;
      private void MaximizeCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         if(MaximizeCheckBox.Checked == true)
         {
            lockedOn = true;
            MaximizePanel();

         }
         else
         {
            lockedOn = false;
            MinimizePanel();

         }
      }

      private void ImportButton_Click(object sender, EventArgs e)
      {
         if(MessageBox.Show("This will erase the existing triggers.","Warning!", MessageBoxButtons.OKCancel) != DialogResult.OK)
         {
            return;
         }

         try
         {
            OpenFileDialog d = new OpenFileDialog();
            d.Filter = "Script (*" + CoreGlobals.getWorkPaths().mScriptExtention + ")|*" + CoreGlobals.getWorkPaths().mScriptExtention;
            d.InitialDirectory = CoreGlobals.getWorkPaths().mScriptTriggerDirectory; 
            if (d.ShowDialog() == DialogResult.OK)
            {
               XmlSerializer s = new XmlSerializer(typeof(TriggerRoot), new Type[] { });
               Stream st = File.OpenRead(d.FileName);
               TriggerData = (TriggerRoot)s.Deserialize(st);
               st.Close();
               
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      private void ExportButton_Click(object sender, EventArgs e)
      {
         try
         {

            SaveFileDialog d = new SaveFileDialog();
            d.Filter = "Script (*" + CoreGlobals.getWorkPaths().mScriptExtention + ")|*" + CoreGlobals.getWorkPaths().mScriptExtention;
            d.InitialDirectory = CoreGlobals.getWorkPaths().mScriptTriggerDirectory; 
            if (d.ShowDialog() == DialogResult.OK)
            {
               XmlSerializer s = new XmlSerializer(typeof(TriggerRoot), new Type[] { });
               Stream st = File.Open(d.FileName, FileMode.Create);
               s.Serialize(st, TriggerData);
               st.Close();

               //game does not need this xmb at this point in time

            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }


   }





}
