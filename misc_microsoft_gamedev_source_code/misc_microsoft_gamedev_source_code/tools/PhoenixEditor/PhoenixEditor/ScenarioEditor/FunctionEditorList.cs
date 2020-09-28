using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class FunctionEditorList : PhoenixEditor.ScenarioEditor.SuperList
   {
      public FunctionEditorList()
      {
         InitializeComponent();
      }
      override public int RecaluculateSize()
      {
         int totalsize = 0;
         int row = 0;
         foreach (FunctorEditor f in mControls)
         {
            int fingSize = f.RecalculateSize();
            GetTableLayout().RowStyles[row].Height = fingSize;// f.Height;

            totalsize += fingSize;// f.Height;

            totalsize++;  // one pixel more per entry

            row++;
         }

         this.Height = totalsize + 20;
         return totalsize + 10;
      }

      override public DragDropEffects ValidateDragTarget(Control toMove, Control target, DragEventArgs e)
      {
         FunctorEditor ctrlMove = toMove.Tag as FunctorEditor;
         FunctorEditor ctrlTarget = target.Tag as FunctorEditor;

         SuperList targetList = target as SuperList;

         //to empty list
         if (targetList != null)
         {
            SuperListDragButton button = toMove as SuperListDragButton;
            if (targetList.GetType() == button.GetParentList().GetType())
            {
               if ((e.KeyState & 8) == 8 && (e.AllowedEffect & DragDropEffects.Copy) == DragDropEffects.Copy)
               {
                  return DragDropEffects.Copy;
               }
               else
               {
                  return DragDropEffects.Move;
               }
            }
         }
         if (ctrlMove == null || ctrlTarget == null)
         {

            return DragDropEffects.None;
         }

         if ((e.KeyState & 8) == 8 && (e.AllowedEffect & DragDropEffects.Copy) == DragDropEffects.Copy)
         {

            return DragDropEffects.Copy;
         }
         else if ((e.AllowedEffect & DragDropEffects.Move) == DragDropEffects.Move)
         {
            if (ctrlMove.LogicalHost == ctrlTarget.LogicalHost)
            {
               return DragDropEffects.Move;
            }
            else if (ctrlMove.LogicalHost.GetType() == ctrlTarget.LogicalHost.GetType())
            {
               return DragDropEffects.Move;
            }
         }

         return DragDropEffects.None;

      }


      override public void HandleDrop(DragEventArgs e, SuperListDragButton otherButton, SuperListDragButton thisButton)
      {
         FunctorEditor ctrlMove = otherButton.Tag as FunctorEditor;
         FunctorEditor ctrlTarget = thisButton.Tag as FunctorEditor;

         if (e.Effect == DragDropEffects.Move)
         {
            if (ctrlMove.LogicalHost == ctrlTarget.LogicalHost  && ctrlMove.Parent == ctrlTarget.Parent)
            {
               this.OnReOrderRequest(otherButton, thisButton);
            }
            else if (ctrlMove.LogicalHost.GetType() == ctrlTarget.LogicalHost.GetType())
            {
               SuperList senderParent = otherButton.GetParentList();
               senderParent.OnTransferRequest(this, otherButton);
               //return DragDropEffects.Move;
            }
            else
            {
               MessageBox.Show("asdf");
            }

         }
         else if (e.Effect == DragDropEffects.Copy)
         {
            if (ctrlMove.LogicalHost.GetType() == ctrlTarget.LogicalHost.GetType())
            {
               SuperList senderParent = otherButton.GetParentList();
               senderParent.OnCopyRequest(this, otherButton);
               //return DragDropEffects.Move;

               
            }

         }

      }

   
   }

   public class ToolTipGroup
   {
      public int mKey = -1;
      List<ToolTip> mToolTips = new List<ToolTip>();

      public ToolTip GetToolTip(int key)
      {
         if(key != mKey)
         {
            foreach(ToolTip t in mToolTips)
            {
               if (t.Active)
               {
                  t.Active = false;
                  t.Dispose();
               }
            }
            mToolTips.Clear();
         }
         mKey = key;
         ToolTip newtip = new ToolTip();
         mToolTips.Add(newtip);
         return newtip;
      }

   }


   public class TriggerComponentList : FunctionEditorList, ITriggerUIUpdate
   {
      //static protected ToolTip mComponentHelp = new ToolTip();

      static protected ToolTipGroup mToolTipGroup = new ToolTipGroup();

      protected string UpdateHelpText(FunctorEditor f, TriggerComponentDefinition def)
      {
         string newHelpText;
         try
         {
            //List<object> paramsString = new List<object>();
            //paramsString.Add("REPORT_THIS_ERROR_PLZ!");  //don't use index 0            
            //foreach (Control c in f.GetComponents())
            //{
            //   TriggerParameterControl param = c as TriggerParameterControl;
            //   if (param != null)
            //   {
            //      paramsString.Add(param.TextValue);
            //   }
            //}
            List<object> paramsString = new List<object>();
            Dictionary<int, string> paramNames = new Dictionary<int, string>();

            foreach (Control c in f.GetComponents())
            {
               TriggerParameterControl param = c as TriggerParameterControl;
               if (param != null)
               {

                  paramNames[param.GetVariable().SigID] = param.TextValue;

               }
            }

            for (int i = 0; i < TriggerSystemDefinitions.cMaxParameters; i++)
            {
               if (paramNames.ContainsKey(i) == false)
               {
                  paramsString.Add("##ERRORPARAM=" + i.ToString());
               }
               else
               {
                  paramsString.Add(paramNames[i]);
               }
            }



            newHelpText = string.Format(def.HelpText, paramsString.ToArray());
         }
         catch (System.Exception ex)
         {
            newHelpText = "Please Report Help text error";
            ex.ToString();
         }
         return newHelpText;
      }




      #region ITriggerUIUpdate Members
      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity)
      {
         List<Control> notused = new List<Control>();
         UIUpdate(data, arguments, visiblity, ref notused);
      }

      public void UIUpdate(object data, object arguments, eUpdateVisibilty visiblity, ref List<Control> owners)
      {
         foreach (Control c in mControls)//this.Controls)
         {
            ITriggerUIUpdate ui = c as ITriggerUIUpdate;
            if (ui != null)
            {
               ui.UIUpdate(data, arguments, visiblity, ref owners);
            }




         }
      }

      #endregion


   }


}

