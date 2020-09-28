using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.Design;
using System.Globalization;
using System.ComponentModel.Design.Serialization;


//Todo: templatize this
namespace DataDriven
{
   public partial class SliderPicker : UserControl
   {
      public SliderPicker()
      {
         InitializeComponent();
      }

      private void tableLayoutPanel1_Paint(object sender, PaintEventArgs e)
      {

      }
      NumericValue mValue;

      public NumericValue NumericValue
      {
         get
         {
            return mValue;
         }
         set
         {
            mValue = value;
         }
      }

      private IWindowsFormsEditorService edSvc;
      public IWindowsFormsEditorService EditorService
      {
         get
         {
            return edSvc;
         }
         set
         {
            this.edSvc = value;
         }
      }

   }

   [Editor(typeof(NumericUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
   [TypeConverter(typeof(NumericConverter))]
   public class NumericValue
   {
      private int intValue;

      public NumericValue(int intValue)
      {
         this.intValue = intValue;
      }

      public int Value
      {
         get
         {
            return intValue;
         }
      }


      
   }


   
   //public class NumericValue
   //{
   //   private float intValue;

   //   public NumericValue(float intValue)
   //   {
   //      this.intValue = intValue;
   //   }

   //   public float Value
   //   {
   //      get
   //      {
   //         return intValue;
   //      }
   //   }



   //}

   internal class NumericConverter : TypeConverter
   {

      public override bool CanConvertFrom(ITypeDescriptorContext context, Type destType)
      {
         if (destType == typeof(string)) return true;
         return base.CanConvertFrom(context, destType);
      }

      public override bool CanConvertTo(ITypeDescriptorContext context, Type destType)
      {
         if (destType == typeof(InstanceDescriptor) || destType == typeof(string))
         {
            return true;
         }
         return base.CanConvertTo(context, destType);
      }

      public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
      {
         if (value is string)
         {
            return new NumericValue(Int32.Parse((string)value));
         }
         return base.ConvertFrom(context, culture, value);
      }

      public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destType)
      {
         if (destType == typeof(InstanceDescriptor))
         {
            return new InstanceDescriptor(typeof(NumericValue).GetConstructor(new Type[] { typeof(int) }), new object[] { ((NumericValue)value).Value });
         }
         else if (destType == typeof(string))
         {
            if (value is NumericValue)
            {

               return ((NumericValue)value).Value.ToString();
            }
            else
            {
               return value.ToString();
            }
         }
         else if(destType == typeof(NumericValue))
         {
            if (value is int)
            {

               return new NumericValue((int)value);
            }
         }
         return base.ConvertTo(context, culture, value, destType);
      }

   }

   //internal class NumericConverter : ExpandableObjectConverter
   //{

   //   public override bool CanConvertFrom(
   //         ITypeDescriptorContext context, Type t)
   //   {

   //      if (t == typeof(string))
   //      {
   //         return true;
   //      }
   //      return base.CanConvertFrom(context, t);
   //   }

   //   public override object ConvertFrom(
   //         ITypeDescriptorContext context,
   //         CultureInfo info,
   //          object value) {

   //   if (value is string) {
   //      try {
   //      string s = (string) value;
   //      // parse the format "Last, First (Age)"
   //      //
   //      int comma = s.IndexOf(',');
   //      if (comma != -1) {
   //         // now that we have the comma, get 
   //         // the last name.
   //         string last = s.Substring(0, comma);
   //          int paren = s.LastIndexOf('(');
   //         if (paren != -1 && 
   //               s.LastIndexOf(')') == s.Length - 1) {
   //            // pick up the first name
   //            string first = 
   //                  s.Substring(comma + 1, 
   //                     paren - comma - 1);
   //            // get the age
   //            int age = Int32.Parse(
   //                  s.Substring(paren + 1, 
   //                  s.Length - paren - 2));
   //               Person p = new Person();
   //               p.Age = age;
   //               p.LastName = last.Trim();
   //               .FirstName = first.Trim();
   //               return p;
   //         }
   //      }
   //   }
   //   catch {}
   //   // if we got this far, complain that we
   //   // couldn't parse the string
   //   //
   //   throw new ArgumentException(
   //      "Can not convert '" + (string)value + 
   //               "' to type Person");
         
   //   }
   //   return base.ConvertFrom(context, info, value);
   //}

   //   public override object ConvertTo(
   //            ITypeDescriptorContext context,
   //            CultureInfo culture,
   //            object value,
   //            Type destType)
   //   {
   //      if (destType == typeof(string) && value is Person)
   //      {
   //         Person p = (Person)value;
   //         // simply build the string as "Last, First (Age)"
   //         return p.LastName + ", " +
   //             p.FirstName +
   //             " (" + p.Age.ToString() + ")";
   //      }
   //      return base.ConvertTo(context, culture, value, destType);
   //   }
   //}









   /// <summary>
   /// Our implementation of UITypeEditor that allows us to modify
   /// the editor for Point on each of our Control vertices.
   /// </summary>
   public class NumericUIEditor : System.Drawing.Design.UITypeEditor
   {
      NumericValue target;
      SliderPicker ui;

      public NumericUIEditor()
      { 
      }

      public NumericUIEditor(NumericValue target)
      {
         this.target = target;
      }

      public override object EditValue(ITypeDescriptorContext context, IServiceProvider sp, object value)
      {
         // get the editor service.
         IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)sp.GetService(typeof(IWindowsFormsEditorService));

         if (edSvc == null)
         {
            // uh oh.
            return value;
         }

         // create our UI
         if (ui == null)
         {
            ui = new SliderPicker();
         }

         ui.EditorService = edSvc;
         ui.NumericValue = (NumericValue)value;

         // instruct the editor service to display the control as a dropdown.
         edSvc.DropDownControl(ui);

         // return the updated value;
         return ui.NumericValue;
      }

      public override System.Drawing.Design.UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
      {
         return System.Drawing.Design.UITypeEditorEditStyle.DropDown;
      }

      public override bool GetPaintValueSupported(ITypeDescriptorContext context)
      {
         return false;
      }



   }

}
