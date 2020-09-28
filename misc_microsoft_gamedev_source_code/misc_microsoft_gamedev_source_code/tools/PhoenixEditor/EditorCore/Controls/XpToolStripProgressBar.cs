using System;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;
namespace EditorCore
{
   // Summary:
   //     Represents a Windows progress bar control contained in a System.Windows.Forms.StatusStrip.
   [DefaultProperty("Value")]
   public class XpToolStripProgressBar : ToolStripControlHost
   {

      XpProgressBar mProgressBar;
      public XpToolStripProgressBar():
         base(new XpProgressBar())
      {
         mProgressBar = Control as XpProgressBar;
      }

      public XpToolStripProgressBar(string name)
         : base(new XpProgressBar())
      {
         mProgressBar = Control as XpProgressBar;
         mProgressBar.Name = name;
      }


      [DefaultValue(100)]
      public int Maximum 
      {
         get
         {
            return mProgressBar.PositionMax;
         }
         set
         {
            mProgressBar.PositionMax = value;
         }
      }
     
      [DefaultValue(0)]
      public int Minimum
      {
         get
         {
            return mProgressBar.PositionMin;
         }
         set
         {
            mProgressBar.PositionMin = value;
         }
      }

      //[DefaultValue(Color.Green)]
      public Color ForeColor
      {
         get
         {
            return mProgressBar.ColorBarCenter;
         }
         set
         {
            mProgressBar.ColorBarCenter = value;
         }
      }
     
      [DesignerSerializationVisibility(0)]
      [Browsable(false)]
      public XpProgressBar ProgressBar 
      {
         get { return mProgressBar; }
      }
     
      [DefaultValue(10)]
      int mStepAmt = 1;
      public int Step 
      {
         get
         {
            return mStepAmt;
         }
         set
         {
            mStepAmt = value;
         }
      }

      [DefaultValue(true)]
      public bool Solid
      {
         get
         {
            return mProgressBar.StepDistance==0;
         }
         set
         {
            if (value)
               mProgressBar.StepDistance = 0;
            else
               mProgressBar.StepDistance = 3;
         }
      }

      [Browsable(false)]
      [DesignerSerializationVisibility(0)]
      public override string Text 
      { 
         get 
         { 
            return mProgressBar.Text; 
         } 
         set
         {
            mProgressBar.Text = value; 
         }
      }
     
      [DefaultValue(0)]
      [Bindable(true)]
      public int Value 
      {
         get 
         {
            return mProgressBar.Position; 
         } 
         set
         {
            mProgressBar.Position = value; 
         } 
      }


      public void Increment(int value)
      {
         mProgressBar.Position += value;
      }
      public void PerformStep()
      {
         mProgressBar.Position += Step;
      }
   
      //protected virtual void OnRightToLeftLayoutChanged(EventArgs e)
      //{

      //}
      //protected override void OnSubscribeControlEvents(Control control){}
      //protected override void OnUnsubscribeControlEvents(Control control){}


   }
}