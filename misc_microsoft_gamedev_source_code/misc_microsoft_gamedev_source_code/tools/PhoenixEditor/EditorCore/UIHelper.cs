using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Reflection;
using System.Windows.Forms;

using EditorCore;

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
      public string TopicName = "";
      public eControlType ButtonType;


      public ToolCommandAttribute()
      {
         
      }
   }

   [AttributeUsage(AttributeTargets.Class, AllowMultiple = false)]
   public class TopicTypeAttribute : System.Attribute
   {

      public string Name;

      public TopicTypeAttribute()
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
         public SomeCommand(MethodInfo m, ToolCommandAttribute t, IToolBarCommandHandler commandHandler)
         {
            mMethod = m;
            mSettings = t;
            mCommandHandler = commandHandler;
         }
         public string GetName()
         {
            if (mSettings.Name != null && mSettings.Name != "")
               return mSettings.Name;
            else
               return mMethod.Name;
         }
         public void BindClick(ToolStripItem b, object bindingContext)
         {
            mBindingContext = bindingContext;
            b.Click += new EventHandler(b_Click);
            mInvoke = Delegate.CreateDelegate(typeof(BasicSig), bindingContext, mMethod);            
         }
         void b_Click(object sender, EventArgs e)
         {
            if (CheckCommandPermission() == false)
            {
               return;
            }

            mInvoke.DynamicInvoke(new object[0]);  //Dynamic invoke = teh fancy 
            //mMethod.Invoke(mBindingContext, new object[0]);
            if(mCommandHandler != null)
            {
               mCommandHandler.ToolbarButtonClicked(this);
            }
         }

         public bool CheckCommandPermission()
         {
            //Todo:  make this not scenario specific.
            return CoreGlobals.getEditorMain().mPhoenixScenarioEditor.CheckTopicPermission(mSettings.TopicName);
         }


         public MethodInfo mMethod;
         public ToolCommandAttribute mSettings;
         object mBindingContext;

         IToolBarCommandHandler mCommandHandler = null;
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
      public ArrayList GetCommandsFromObject(object obj, IToolBarCommandHandler commandHandler)
      {
         Type classType = obj.GetType();
         return GetCommandsByType(classType, commandHandler);
      }
      public ArrayList GetCommandsByType(Type classType, IToolBarCommandHandler commandHandler)
      {
         string Topic = "";
         object[] topictype = classType.GetCustomAttributes(typeof(TopicTypeAttribute), false);
         foreach (object o in topictype)
         {
            if (o is TopicTypeAttribute)
            {
               Topic = ((TopicTypeAttribute)o).Name;
            }
         }


         ArrayList methods = new ArrayList();
         MethodInfo[] mi = classType.GetMethods();
         for (int x = 0; x < mi.Length; x++)
         {
            Object[] MethodAttrs = mi[x].GetCustomAttributes(typeof(ToolCommandAttribute), false);
            for (int n = 0; n < MethodAttrs.Length; n++)
            {
               if (MethodAttrs[n] is ToolCommandAttribute)
               {
                  if (((ToolCommandAttribute)MethodAttrs[n]).TopicName == "")
                  {
                     ((ToolCommandAttribute)MethodAttrs[n]).TopicName = Topic;
                  }
                  methods.Add(new SomeCommand(mi[x], (ToolCommandAttribute)MethodAttrs[n], commandHandler));
               }
            }
         }
         return methods;
      }


      //With a little refactoring this could generate a wider range of ui stuff
      public ArrayList GetToolStripButtons(ArrayList commands, object bindingContext)
      {
         Type classType = bindingContext.GetType();
         ArrayList buttons = new ArrayList();
         //ArrayList commands = GetCommandsByType(classType);
         foreach (SomeCommand c in commands)
         {
            ToolStripButton b = new ToolStripButton();
            if (c.mSettings.Icon != null)
            {
               string iconFileName = Path.Combine(CoreGlobals.getWorkPaths().mAppIconDir, c.mSettings.Icon + ".bmp");
               if (File.Exists(iconFileName))
               {
                  //b.ImageScaling = ToolStripItemImageScaling.None;
                  b.Image = new System.Drawing.Bitmap(iconFileName);
               }
            }
            else
            {
               b.Text = c.GetName();
            }
            if (c.mSettings.ButtonType == eControlType.cCheck)
            {
               b.CheckOnClick = true;
            }
            b.ToolTipText = c.GetName();
            b.Width += (int)(b.Width * 1.5);
            c.BindClick(b, bindingContext);
            buttons.Add(b);
         }
         return buttons;
      }

      public ArrayList GetMenuButtons(ArrayList commands, object bindingContext)
      {
         Type classType = bindingContext.GetType();
         ArrayList buttons = new ArrayList();
         //ArrayList commands = GetCommandsByType(classType);
         foreach (SomeCommand c in commands)
         {
            ToolStripMenuItem b = new ToolStripMenuItem();
            if (c.mSettings.Icon != null)
            {
               string iconFileName = Path.Combine(CoreGlobals.getWorkPaths().mAppIconDir, c.mSettings.Icon + ".bmp");
               if (File.Exists(iconFileName))
               {
                  //b.ImageScaling = ToolStripItemImageScaling.None;
                  b.Image = new System.Drawing.Bitmap(iconFileName);
                  b.ToolTipText = c.GetName();
                  b.Text = c.GetName();
                  
               }
            }
            else
            {
               b.Text = c.GetName();
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
      public interface IToolBarCommandHandler
      {
         void ToolbarButtonClicked(UiGenerator.SomeCommand command);
      }
   }



}
