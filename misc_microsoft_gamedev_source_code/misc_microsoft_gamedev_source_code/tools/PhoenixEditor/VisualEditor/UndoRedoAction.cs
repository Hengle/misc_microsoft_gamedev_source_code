using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

using ModelSystem;



namespace VisualEditor
{
   public abstract class UndoRedoAction
   {      
      public abstract void undo();
      public abstract void redo();

      public virtual bool affectsSameNode(UndoRedoAction action)
      {
         return false;
      }

      public virtual void updateAction(UndoRedoAction action)
      {
      }


      private TreeView m_treeView = null;

      public TreeView treeView
      {
         get { return this.m_treeView; }
         set { this.m_treeView = value; }
      }

      public TreeNode findTreeNode(XMLVisualNode vnode)
      {
         foreach (TreeNode curTNode in m_treeView.Nodes)
         {
            TreeNode foundTNode = findTreeNodeRecurse(curTNode, vnode);

            if (foundTNode != null)
               return (foundTNode);
         }

         return (null);
      }

      public TreeNode findTreeNodeRecurse(TreeNode tnode, XMLVisualNode vnode)
      {
         if(tnode.Tag == vnode)
         {
            return (tnode);
         }

         foreach(TreeNode curTNode in tnode.Nodes)
         {
            TreeNode foundTNode = findTreeNodeRecurse(curTNode, vnode);

            if(foundTNode != null)
               return(foundTNode);
         }

         return (null);
      }

   }



   // This undo / redo action handles undoing a change in the data of a node.
   // The constructor accepts the node that was changed (after the changes are
   // applied) and a copy of the node with the data before the changes.
   public class UndoRedoChangeDataAction : UndoRedoAction
   {
      private XMLVisualNode mNode;
      private XMLVisualNode mNodeBeforeChanges;
      private XMLVisualNode mNodeAfterChanges;


      public UndoRedoChangeDataAction(XMLVisualNode node, XMLVisualNode copyAfterChanges)
      {
         mNode = node;
         mNodeAfterChanges = copyAfterChanges;
         if (mNode != null)
            mNodeBeforeChanges = mNode.clone(false);
      }

      public override void undo()
      {
         if ((mNode != null) && (mNodeBeforeChanges != null))
         {
            mNode.copy(mNodeBeforeChanges);

            TreeNode curTreeNode = findTreeNode(mNode);
            mNode.updateTreeNodeText(curTreeNode);
         }
      }

      public override void redo()
      {
         if ((mNode != null) && (mNodeAfterChanges != null))
         {
            mNode.copy(mNodeAfterChanges);

            TreeNode curTreeNode = findTreeNode(mNode);
            mNode.updateTreeNodeText(curTreeNode);
         }
      }

      public override bool affectsSameNode(UndoRedoAction action)
      {
         UndoRedoChangeDataAction curChangeDataAction = action as UndoRedoChangeDataAction;

         if(curChangeDataAction != null)
         {
            if(curChangeDataAction.mNode == mNode)
               return true;
         }
         return false;
      }

      
      public override void updateAction(UndoRedoAction action)
      {
         UndoRedoChangeDataAction curChangeDataAction = action as UndoRedoChangeDataAction;

         if (curChangeDataAction != null)
         {
            mNodeAfterChanges = curChangeDataAction.mNodeAfterChanges;
         }
      }
   };


   // This undo / redo action handles undoing adding of a node under another node.
   public class UndoRedoAddNodeAction : UndoRedoAction
   {
      // Member variables
      //
      private XMLVisualNode mParentNode;
      private XMLVisualNode mAddedNode;

      // Member functions
      //
      public UndoRedoAddNodeAction(XMLVisualNode parentNode, XMLVisualNode addedNode)
      {
         mParentNode = parentNode;
         mAddedNode = addedNode;
      }

      public override void undo()
      {
         // Remove the child from the parent
         if ((mParentNode != null) && (mAddedNode != null))
         {
            mParentNode.removeChild(mAddedNode);

            TreeNode parentTreeNode = findTreeNode(mParentNode);
            foreach (TreeNode curTreeNode in parentTreeNode.Nodes)
            {
               if(curTreeNode.Tag == mAddedNode)
               {
                  parentTreeNode.Nodes.Remove(curTreeNode);
                  break;
               }
            }

            // Update parent tree node text
            mParentNode.updateTreeNodeText(parentTreeNode);
         }
      }

      public override void redo()
      {
         // Add the child back under the parent node
         if ((mParentNode != null) && (mAddedNode != null))
         {
            TreeNode parentTreeNode = findTreeNode(mParentNode);
            TreeNode addedTreeNode = mAddedNode.createTreeNode();

            mParentNode.addChild(mAddedNode);
            parentTreeNode.Nodes.Add(addedTreeNode);

            // Update parent tree node text
            mParentNode.updateTreeNodeText(parentTreeNode);

            // Expand parent node, and expand all child node
            parentTreeNode.Expand();
            addedTreeNode.ExpandAll();
         }
      }
   };


   // This undo / redo action handles undoing the deletion of a node.
   public class UndoRedoDeleteNodeAction : UndoRedoAction
   {
      // Member variables
      //
      private XMLVisualNode mParentNode;
      private XMLVisualNode mDeletedNode;

      private int mDeletedNodeIndex = -1;
      private int mDeletedTreeNodeIndex = -1;


      // Member functions
      //
      public UndoRedoDeleteNodeAction(XMLVisualNode parentNode, XMLVisualNode deletedNode)
      {
         mParentNode = parentNode;
         mDeletedNode = deletedNode;
      }


      public override void undo()
      {
         // Add the child back under the parent node
         if ((mParentNode != null) && (mDeletedNode != null))
         {
            TreeNode parentTreeNode = findTreeNode(mParentNode);
            TreeNode deletedTreeNode = mDeletedNode.createTreeNode();

            mParentNode.insertChild(mDeletedNodeIndex, mDeletedNode);
            parentTreeNode.Nodes.Insert(mDeletedTreeNodeIndex, deletedTreeNode);

            // Update parent tree node text
            mParentNode.updateTreeNodeText(parentTreeNode);

            // Expand parent node, and expand all child node
            parentTreeNode.Expand();
            deletedTreeNode.ExpandAll();
         }
      }
      public override void redo()
      {
         // Remove the child from the parent
         if ((mParentNode != null) && (mDeletedNode != null))
         {
            mDeletedNodeIndex = mParentNode.indexOf(mDeletedNode);
            mParentNode.removeChild(mDeletedNode);

            TreeNode parentTreeNode = findTreeNode(mParentNode);
            for (int i = 0; i < parentTreeNode.Nodes.Count; i++)
            {
               TreeNode curTreeNode = parentTreeNode.Nodes[i];

               if (curTreeNode.Tag == mDeletedNode)
               {
                  mDeletedTreeNodeIndex = i;
                  parentTreeNode.Nodes.Remove(curTreeNode);
                  break;
               }
            }

            // Update parent tree node text
            mParentNode.updateTreeNodeText(parentTreeNode);
         }
      }
   };




   // Undo / redo manager.  This class performs the undo and redo actions
   // and keeps track of what actions can be undone and redone.
   public class UndoRedoManager
   {
      private const int          UNDO_REDO_STACK_SIZE = 200;
      private const float        MAX_ELAPSED_TIME = 2.0f;          // in seconds


      private UndoRedoAction[]   mUndoRedoStack = new UndoRedoAction[UNDO_REDO_STACK_SIZE];     // Stack of actions to undo / redo.  (Really a circular array.)
      private long               mNextUndoRedoIndex = 0;                                        // Index of next position where an action can be added to the stack
      VisualEditorPage           mVisualPage = null;
      private DateTime           mLastActionTime;

      public UndoRedoManager(VisualEditorPage visualPage)
      {
         mVisualPage = visualPage;

         DateTime mLastActionTime = DateTime.Now;
      }

      // Initializes the undo / redo manager.
      public void init()
      {
      }

      // Cleans up after the undo / redo manager.
      public void cleanup()
      {
      }


      // Adds an action to the undo list.  The manager will be responsible for deleting
      // the action when necessary.
      public void addUndoRedoActionAndExecute(UndoRedoAction action)
      {
         float elapseTime = (float)((TimeSpan)(System.DateTime.Now - mLastActionTime)).TotalSeconds;
         mLastActionTime = System.DateTime.Now;

         // Check if we can update the last action.
         // (If we are changing the same object and a short period of time has passed since
         // last edit, then update the last UndoRedoAction instead of adding a new one.
         // This happens often with UndoRedoChangeDataAction when sliders or edit boxes
         // are changed).
         //

         if (canUndo() && (elapseTime < MAX_ELAPSED_TIME))
         {
            long prevUndoRedoIndex = mNextUndoRedoIndex - 1;
            if (prevUndoRedoIndex < 0)
               prevUndoRedoIndex = UNDO_REDO_STACK_SIZE - 1;

            if (mUndoRedoStack[prevUndoRedoIndex].affectsSameNode(action))
            {
               mUndoRedoStack[prevUndoRedoIndex].updateAction(action);

               // Execute the action
               mUndoRedoStack[prevUndoRedoIndex].redo();
               markDocDirty();

               return;
            }
         }


         // Add new action
         //

         action.treeView = mVisualPage.getVisualTreeView();

         // Delete the action in the next position if necessary
         if (mUndoRedoStack[mNextUndoRedoIndex] != null)
         {
            mUndoRedoStack[mNextUndoRedoIndex] = null;
         }

         mUndoRedoStack[mNextUndoRedoIndex] = action;

         // Move the index to the next insertion point
         mNextUndoRedoIndex++;
         if (mNextUndoRedoIndex >= UNDO_REDO_STACK_SIZE)
            mNextUndoRedoIndex = 0;

         // Zero out the next action now so it can be used as a delimiter in the
         // circular array.
         if (mUndoRedoStack[mNextUndoRedoIndex] != null)
         {
            mUndoRedoStack[mNextUndoRedoIndex] = null;
         }

         // Execute the action
         action.redo();
         markDocDirty();
      }

      // Deletes all actions from the undo / redo manager.
      public void removeAllUndoRedoActions()
      {
         int nIndx;

         // Delete all the actions in the stack
         if (mUndoRedoStack != null)
         {
            for (nIndx = 0; nIndx < UNDO_REDO_STACK_SIZE; nIndx++)
            {
               if (mUndoRedoStack[nIndx] != null)
               {
                  mUndoRedoStack[nIndx] = null;
               }
            }

            mNextUndoRedoIndex = 0;
         }
      }

      // Returns true if there is an action to be undone or redone and false if there
      // is not.
      public bool canUndo()
      {
         // Can undo if there is an action in the previous position (the one before the
         // position where new actions are inserted).
         long prevUndoRedoIndex = mNextUndoRedoIndex - 1;
         if (prevUndoRedoIndex < 0)
            prevUndoRedoIndex = UNDO_REDO_STACK_SIZE - 1;

         if (mUndoRedoStack[prevUndoRedoIndex] != null)
            return true;
         else
            return false;
      }

      public bool canRedo()
      {
         // Can redo if there is an action at the point where the next undo action will
         // be inserted.
         if (mUndoRedoStack[mNextUndoRedoIndex] != null)
            return true;
         else
            return false;
      }


      // Undo or redo the action in the undo / redo stacks.
      public void undo()
      {
         if (canUndo())
         {
            // Decrement the index to the position of the action to undo
            mNextUndoRedoIndex--;
            if (mNextUndoRedoIndex < 0)
               mNextUndoRedoIndex = UNDO_REDO_STACK_SIZE - 1;

            // Undo the action
            mUndoRedoStack[mNextUndoRedoIndex].undo();

            // Update property page
            mVisualPage.ShowPropertyPageForSelectedNode();

            markDocDirty();
         }
      }

      public void redo()
      {
         // Redo the action at the current insertion point
         if (canRedo())
         {
            // Redo the action
            mUndoRedoStack[mNextUndoRedoIndex].redo();

            // Update property page
            mVisualPage.ShowPropertyPageForSelectedNode();

            // Increment the index to the next position
            mNextUndoRedoIndex++;
            if (mNextUndoRedoIndex >= UNDO_REDO_STACK_SIZE)
               mNextUndoRedoIndex = 0;

            markDocDirty();
         }
      }

      private void markDocDirty()
      {
         mVisualPage.isDocumentModified = true;
      }
   };
}
