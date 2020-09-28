using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Reflection;
using System.Windows.Forms;

namespace UIHelper
{
   public enum eControlType
   {
      cDefault,
      cCheck,
      cCombo
   }    
   /// <summary>
   /// command metadata
   /// 
   /// use this tag to mark up a function to be linked to the ui
   /// </summary>
   [AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
   public class ToolCommandAttribute : System.Attribute
   {  

      public string Name;        
      public string Icon;
      public string GroupName;
      public eControlType ButtonType;


      public ToolCommandAttribute()
      {
         
      }
   }

   /// <summary>
   ///  field/property metadata
   /// </summary>
   [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
   public class ToolPropertyAttribute : System.Attribute
   {
      public string Name;
      //public string GroupName;
      //public eType ButtonType;


      public ToolPropertyAttribute()
      {

      }
   }
   
  
   public class UiGenerator
   {
      public class SomeCommand
      {
         public SomeCommand(MethodInfo m, ToolCommandAttribute t)
         {
            mMethod = m;
            mSettings = t;
            
         }
         public string GetName()
         {
            if (mSettings.Name != "")
               return mSettings.Name;
            else
               return mMethod.Name;
         }
         public void BindClick(ToolStripButton b, object bindingContext)
         {
            mBindingContext = bindingContext;
            b.Click += new EventHandler(b_Click);
            mInvoke = Delegate.CreateDelegate(typeof(BasicSig), bindingContext, mMethod);            
         }
         void b_Click(object sender, EventArgs e)
         {
            mInvoke.DynamicInvoke(new object[0]);  //Dynamic invoke = teh fancy 
            //mMethod.Invoke(mBindingContext, new object[0]);
         }
         public MethodInfo mMethod;
         public ToolCommandAttribute mSettings;
         object mBindingContext;

         public delegate void BasicSig();
         Delegate mInvoke;

      }

      public class SomeField
      {
         public SomeField(MemberInfo member, ToolPropertyAttribute settings)
         {
            mMember = member;
            mSettings = settings;
         }
         object Data
         {
            set 
            {

            }
            get 
            {
               //FieldInfo f;
               //f.
               return null;
            }
         }

         public MemberInfo mMember;
         public ToolPropertyAttribute mSettings;
      }

      public ArrayList GetCommandsByType(Type classType)
      {
         ArrayList methods = new ArrayList();
         MethodInfo[] mi = classType.GetMethods();
         for (int x = 0; x < mi.Length; x++)
         {
            Object[] MethodAttrs = mi[x].GetCustomAttributes(typeof(ToolCommandAttribute), false);
            for (int n = 0; n < MethodAttrs.Length; n++)
            {
               if (MethodAttrs[n] is ToolCommandAttribute)
               {
                  methods.Add(new SomeCommand(mi[x],(ToolCommandAttribute) MethodAttrs[n]));
               }
            }
         }
         return methods;
      }


      //With a little refactoring this could generate a wider range of ui stuff
      public ArrayList GetToolStripButtons(object bindingContext)
      {
         Type classType = bindingContext.GetType();
         ArrayList buttons = new ArrayList();
         ArrayList commands = GetCommandsByType(classType);
         foreach (SomeCommand c in commands)
         {
            ToolStripButton b = new ToolStripButton();
            if (c.mSettings.Icon != null)
            {
               if (File.Exists(c.mSettings.Icon + ".bmp"))
               {
                  b.ImageScaling = ToolStripItemImageScaling.None;
                  b.Image = new System.Drawing.Bitmap(c.mSettings.Icon + ".bmp");
                  b.ToolTipText = c.mMethod.Name;
               }
            }
            else
            {
               b.Text = c.mMethod.Name;
            }
            if (c.mSettings.ButtonType == eControlType.cCheck)
            {
               b.CheckOnClick = true;
            }            
            b.Width += (int)(b.Width * 1.5);
            c.BindClick(b, bindingContext);
            buttons.Add(b);
         }
         return buttons;
      }



   }



}
