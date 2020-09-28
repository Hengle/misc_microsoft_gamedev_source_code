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

   

   //what to do?

   // need scheme to update other propery values...
   // empty defalut?

   /// <summary>
   /// ////oooo,   flow send event , list handle/send data change event... = magic
   /// </summary>
   /// 
   //list scroll?   replace

   //public class VariableIDProperty : EnumeratedProperty
   //{
   //   public VariableIDProperty(INamedTypedProperty prop)
   //      : base(prop)
   //   {

   //      if (GetMetadata().ContainsKey("TriggerNamespace"))
   //      {
   //         mTriggerNamespace = (TriggerNamespace)GetMetadata()["TriggerNamespace"];


   //      }
   //      if (mTriggerNamespace != null)
   //      {
   //         foreach (TriggerValue v in mTriggerNamespace.GetValueList())
   //         {
   //            if(v.Name != null && v.Name != "")
   //               AddEnum(v.ID.ToString(),v.Name );
   //         }
   //         mDefaultValue = "";

   //      }
   //      //AddEnum("NotEqualTo", "!=");
   //      //AddEnum("LessThan", "<");
   //      //AddEnum("LessThanOrEqualTo", "<=");
   //      //AddEnum("EqualTo", "=");
   //      //AddEnum("GreaterThanOrEqualTo", ">=");
   //      //AddEnum("GreaterThan", ">");

   //      //mDefaultValue = "<";
   //      //PresentationValue = mDefaultValue;
   //   }
   //   TriggerNamespace mTriggerNamespace = null;


   //}
   public class VariableIDProperty : HighLevelProperty
   {
      public VariableIDProperty(INamedTypedProperty prop)
         : base(prop)
      {

         if (GetMetadata().ContainsKey("TriggerNamespace"))
         {
            mTriggerNamespace = (TriggerNamespace)GetMetadata()["TriggerNamespace"];


         }
         //if (mTriggerNamespace != null)
         //{
         //   foreach (TriggerValue v in mTriggerNamespace.GetValueList())
         //   {
         //      if (v.Name != null && v.Name != "")
         //         AddEnum(v.ID.ToString(), v.Name);
         //   }
         //   mDefaultValue = "";

         //}
      
      }
      TriggerNamespace mTriggerNamespace = null;

      string mUnitializedText = "DragVariableHere";

      public override Control GetEditor(out string bindPropName)
      {
         string text = "";
         if (PresentationValue != null)
         {
            if ((int)PresentationValue == -1)
            {
               text = mUnitializedText;

            }
            else
            {
               text = PresentationValue.ToString();
            }
         }
         else
            text = "empty";

         Label l = new Label();

         int idval;
         if(System.Int32.TryParse(text, out idval))
         {         
            TriggerValue val;
            if(mTriggerNamespace.GetValues().TryGetValue(idval, out val))
            {
               text = val.Name;
            }
         }
         l.Text = text;

         l.ForeColor = Color.Blue;
         if (text == mUnitializedText)
            l.ForeColor = Color.Red;
         bindPropName = "Text";
         l.AutoSize = true;
         l.Margin = new Padding(0);

         l.DragEnter += new DragEventHandler(l_DragEnter);
         l.DragDrop += new DragEventHandler(l_DragDrop);
         l.AllowDrop = true;
         return l;


         //return base.GetEditor(out bindPropName);
      }

      void l_DragDrop(object sender, DragEventArgs e)
      {
         Label l = sender as Label;
         object data = e.Data.GetData(typeof(TriggerParameterControl));
         TriggerParameterControl otherButton = data as TriggerParameterControl;
         if (otherButton != null)
         {
            TriggerVariable ctrlMove = otherButton.GetVariable();// as TriggerVariable;

            if (e.Effect == DragDropEffects.Move)
            {
               if (otherButton.ParameterMode == TriggerParameterControl.eParameterMode.Const)
               {
                  PresentationValue = ctrlMove.ID;
                  l.Text = mTriggerNamespace.GetValues()[ctrlMove.ID].Value;
       
               }
               else
               {
                  PresentationValue = ctrlMove.ID;
                  l.Text = ctrlMove.Name;
                  l.Refresh();
               }
               //UpdateControl();
            }
         }
      }

      void l_DragEnter(object sender, DragEventArgs e)
      {
         e.Effect = DragDropEffects.None;
         if (e.Data.GetDataPresent(typeof(TriggerParameterControl)))
         {
            object data = e.Data.GetData(typeof(TriggerParameterControl));
            TriggerParameterControl otherButton = data as TriggerParameterControl;
            if (otherButton != null)
            {
               //TriggerValue ctrlMove = otherButton.mValue as TriggerValue;
               //TriggerValue ctrlTarget = this.mValue as TriggerValue;

               //if (ctrlMove.Type == ctrlTarget.Type)
                  e.Effect = DragDropEffects.Move;


            }

         }
      }

   }

   ////////////////


   public class TriggerBindIDProperty : HighLevelProperty
   {
      public TriggerBindIDProperty(INamedTypedProperty prop)
         : base(prop)
      {

         if (GetMetadata().ContainsKey("TriggerNamespace"))
         {
            mTriggerNamespace = (TriggerNamespace)GetMetadata()["TriggerNamespace"];
         }
  

      }
      TriggerNamespace mTriggerNamespace = null;

      string mUninitializedValue = "-1,DragLinkHere";
      public override Control GetEditor(out string bindPropName)
      {
         string text = "";
         if (PresentationValue != null)
         {
            if((string)PresentationValue == "")
            {
               PresentationValue = mUninitializedValue;
            }
            text = PresentationValue.ToString();
         }
         else
            text = mUninitializedValue;

         Label l = new Label();
         l.Text = text;
         l.ForeColor = Color.Blue;
         if (text == mUninitializedValue)
            l.ForeColor = Color.Red;
         bindPropName = "Text";
         l.AutoSize = true;
         l.Margin = new Padding(0);

         l.DragEnter += new DragEventHandler(l_DragEnter);
         l.DragDrop += new DragEventHandler(l_DragDrop);
         l.AllowDrop = true;
         return l;


         //return base.GetEditor(out bindPropName);
      }

      void l_DragDrop(object sender, DragEventArgs e)
      {
         Label l = sender as Label;
         //object data = e.Data.GetData(typeof(TriggerParameterControl));
         //TriggerParameterControl otherButton = data as TriggerParameterControl;
         //if (otherButton != null)
         //{
         //   TriggerVariable ctrlMove = otherButton.GetVariable();// as TriggerVariable;

         //   if (e.Effect == DragDropEffects.Move)
         //   {
         //      if (otherButton.ParameterMode == TriggerParameterControl.eParameterMode.Const)
         //      {
         //         PresentationValue = ctrlMove.ID;
         //         l.Text = mTriggerNamespace.GetValues()[ctrlMove.ID].Value;

         //      }
         //      else
         //      {
         //         PresentationValue = ctrlMove.ID;
         //         l.Text = ctrlMove.Name;
         //         l.Refresh();
         //      }
         //      //UpdateControl();
         //   }
         //}

         string[] formats = e.Data.GetFormats();
         if (formats.Length == 0)
            return;
         IControlPoint otherControlPoint = e.Data.GetData(e.Data.GetFormats()[0]) as IControlPoint;

         if (otherControlPoint != null)
         {
            if (e.Effect == DragDropEffects.Link)
            {
               string key = otherControlPoint.ToString();
               string id = otherControlPoint.TagObject.ToString();
               string link = string.Format("{0},{1}", id, key);
               this.PresentationValue = link;
               l.Text = link;
               l.ForeColor = Color.Blue;
               l.Refresh();

               //if (otherControlPoint.CanConnect(this))
               //{
               //   otherControlPoint.ConnectControlPoint(this);
               //}
               //else if (this.CanConnect(otherControlPoint))
               //{
               //   ConnectControlPoint(otherControlPoint);
               //}
               //mHost.SetDirty();
            }
         }
      }

      void l_DragEnter(object sender, DragEventArgs e)
      {
         string[] formats = e.Data.GetFormats();
         if (formats.Length == 0)
            return;
         IControlPoint otherControlPoint = e.Data.GetData(e.Data.GetFormats()[0]) as IControlPoint;

         e.Effect = DragDropEffects.None;

         if (otherControlPoint == this)
            return;

         if (otherControlPoint != null)
         {
            //if (otherControlPoint.CanConnect(this) || this.CanConnect(otherControlPoint))
            {
               e.Effect = DragDropEffects.Link;

            }
         } 

         //e.Effect = DragDropEffects.None;
         //if (e.Data.GetDataPresent(typeof(TriggerParameterControl)))
         //{
         //   object data = e.Data.GetData(typeof(TriggerParameterControl));
         //   TriggerParameterControl otherButton = data as TriggerParameterControl;
         //   if (otherButton != null)
         //   {
         //      //TriggerValue ctrlMove = otherButton.mValue as TriggerValue;
         //      //TriggerValue ctrlTarget = this.mValue as TriggerValue;

         //      //if (ctrlMove.Type == ctrlTarget.Type)
         //      e.Effect = DragDropEffects.Move;


         //   }

         //}
      }

   }

}
