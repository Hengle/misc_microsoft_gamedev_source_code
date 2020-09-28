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
   public partial class FunctorEditor : UserControl, ITriggerUIUpdate
   {
      public FunctorEditor()
      {
         InitializeComponent();
         this.Margin = new Padding(0);
         flowLayoutPanel1.Margin = new Padding(0);


         FunctionNameLabel.Margin = new Padding(0);
         FunctionNameLabel.Padding = new Padding(0);

         //FunctionNameLabel.Click += new EventHandler(FunctionNameLabel_Click);
         FunctionNameLabel.MouseClick += new MouseEventHandler(FunctionNameLabel_MouseClick);
         FunctionNameLabel.MouseEnter += new EventHandler(FunctionNameLabel_MouseEnter);
         FunctionNameLabel.MouseHover += new EventHandler(FunctionNameLabel_MouseHover);

         FunctionNameLabel.MouseLeave += new EventHandler(FunctionNameLabel_MouseLeave);
         mHoverTimer = new Timer();
         mHoverTimer.Interval = 500;
         mHoverTimer.Tick += new EventHandler(mHoverTimer_Tick);

      }

      void mHoverTimer_Tick(object sender, EventArgs e)
      {
         if (mHover && FunctionNameHover != null)
         {
            FunctionNameHover.Invoke(this, e);
         }
      }
      bool mHover = false;
      Timer mHoverTimer;
      void FunctionNameLabel_MouseLeave(object sender, EventArgs e)
      {
         mHover = false;
         mHoverTimer.Stop();
      }
      void FunctionNameLabel_MouseEnter(object sender, EventArgs e)
      {
         mHover = true;
         mHoverTimer.Start();

      }

      void FunctionNameLabel_MouseClick(object sender, MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Right)
         {
            if (FunctionNameRightClick != null)
            {
               FunctionNameRightClick.Invoke(this, null);
            }
         }
         else
         {
            if (FunctionNameClicked != null)
            {
               FunctionNameClicked.Invoke(this, null);
            }

         }
      }
      public event EventHandler FunctionNameRightClick;

      public event EventHandler FunctionNameClicked;
      public event EventHandler FunctionNameHover;


      void FunctionNameLabel_MouseHover(object sender, EventArgs e)
      {
         //if(FunctionNameHover != null)
         //{
         //   FunctionNameHover.Invoke(this, e);
         //}
      }
      //void FunctionNameLabel_Click(object sender, EventArgs e)
      //{
      //   if (FunctionNameClicked != null)
      //   {
      //      FunctionNameClicked.Invoke(this, null);
      //   }
      //}

      public string FunctionName
      {
         get
         {
            return this.FunctionNameLabel.Text;
         }
         set
         {
            FunctionNameLabel.Text = value;
         }
      }

      List<Control> mInputs = new List<Control>();
      List<Control> mOutputs = new List<Control>();

      private class NullHost { }

      object mLogicalHost = new NullHost();
      public object LogicalHost
      {
         get
         {
            return mLogicalHost;
         }
         set
         {
            mLogicalHost = value;
         }

      }

      public enum eLayoutStyle
      {
         VerticleList,
         Flow
      }
      eLayoutStyle mLayoutStyle = eLayoutStyle.Flow;
      public eLayoutStyle LayoutStyle
      {
         get
         {
            return mLayoutStyle;
         }
         set
         {
            mLayoutStyle = value;
         }
      }

      public void SetWarningText()
      {
         this.ForeColor = Color.Goldenrod;
      }
      public void SetErrorText()
      {
         this.ForeColor = Color.Red;
      }
      public void SetUpdatedText()
      {
         this.ForeColor = Color.DarkGreen;
      }

      public ICollection<Control> GetComponents()
      {
         List<Control> all = new List<Control>();
         if(mInputs !=null)
            all.AddRange(mInputs);
         if(mOutputs != null)
            all.AddRange(mOutputs);
         return all;

      }


      public void SetupParameters(List<Control> inputs, List<Control> outputs)
      {
         mInputs = inputs;
         mOutputs = outputs;

         if(mLayoutStyle == eLayoutStyle.VerticleList)
         {
            //flowLayoutPanel1.Margin = new Padding(0);

            flowLayoutPanel1.Padding = new Padding(0);
         }

         flowLayoutPanel1.Controls.Clear();
         flowLayoutPanel1.Controls.Add(FunctionNameLabel);

      
         foreach(Control i in mInputs)
         {
            //i.Refresh();

            if(mLayoutStyle == eLayoutStyle.VerticleList)
            {
               i.Margin = new Padding(0);
               i.Padding = new Padding(5, 0, 0, 0);
               i.Width = 150;
            }
            flowLayoutPanel1.Controls.Add(i);

         }
         if (mOutputs.Count > 0)
         {

            //Label l = new Label();
            //l.Text = "->";
            //l.ForeColor = Color.Red;
            //l.AutoSize = true;
            //l.Margin = new Padding(0);
            //flowLayoutPanel1.Controls.Add(l);
            //l.Refresh();
            foreach(Control o in mOutputs)
            {
               //o.Refresh();
               Label l = new Label();
               l.Text = "->";
               l.ForeColor = Color.Red;
               l.AutoSize = true;
               l.Margin = new Padding(0);
               flowLayoutPanel1.Controls.Add(l);

               if (mLayoutStyle == eLayoutStyle.VerticleList)
               {
                  o.Margin = new Padding(0);
                  o.Padding = new Padding(0, 0, 0, 0);
                  o.Width = 100;
               }
               flowLayoutPanel1.Controls.Add(o);
            }

         }
         //this.Refresh();
         //Application.DoEvents();

         if(flowLayoutPanel1.Controls.Count > 0)
         {
            Control c = flowLayoutPanel1.Controls[flowLayoutPanel1.Controls.Count - 1];

            int maxY = c.Height + c.Top;
            this.Height = maxY;
         }
      }

      public int RecalculateSize()
      {
         int maxY = 10;
         if (flowLayoutPanel1.Controls.Count > 0)
         {
            Control c = flowLayoutPanel1.Controls[flowLayoutPanel1.Controls.Count - 1];

            maxY = c.Height + c.Top;

            //if (c.Right > Parent.Width)
            //{
            //   maxY += 19;
            //}


         }
         //int caryover = 0;
         //int lastTop = 0;
         //int rowNumber = 0;
         //int rowwidth = 0;
         int extraheight = 0;
         FlowLayoutPanel fpanel = flowLayoutPanel1;
         //foreach(Control c in fpanel.Controls)
         //{
         //   //c.PreferredSize
         //   rowwidth += c.Width;
         //   int maxX = caryover + rowwidth;// c.Right;
         //   if(maxX > fpanel.Width)
         //   {
         //      extraheight += c.Height;
         //      rowNumber++;
         //      rowwidth = 0;
         //      //caryover = maxX - fpanel.Width;
         //   }


         //}
         maxY += extraheight;

         this.Height = maxY;


         return maxY;
      }

      //public List<Control> Inputs
      //{
      //   get
      //   {
      //      return mInputs;
      //      //mInputs = value;
      //   }
      //   set
      //   {
      //      mInputs
      //   }
      //}
      

      #region ITriggerUIUpdate Members
      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity)
      {
         List<Control> notused = new List<Control>();
         UIUpdate(data, arguments, visiblity, ref notused);
      }
      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity, ref List<Control> owners)
      {
         foreach (Control c in flowLayoutPanel1.Controls)
         {
            ITriggerUIUpdate ui = c as ITriggerUIUpdate;
            if (ui != null)
            {
               ui.UIUpdate(data, arguments, visiblity, ref owners);
            }

         }

         BasicArgument ba = arguments as BasicArgument;
         if (data == null)
         {
         }
         else if (data is TriggerComponent && (((TriggerComponent)data).ID == ((TriggerComponent)this.Tag).ID) && (ba != null))
         {
            if (ba.mArgument == BasicArgument.eArgumentType.Search)
            {
               owners.Add(this);
            }
         }
      
      }

      #endregion
   }
}
