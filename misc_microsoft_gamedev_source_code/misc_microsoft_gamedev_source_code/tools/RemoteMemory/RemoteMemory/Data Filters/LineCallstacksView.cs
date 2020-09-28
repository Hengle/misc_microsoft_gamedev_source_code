using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace RemoteMemory
{
   public partial class LineCallstacksView : Form
   {
      //=========================================
      // LineCallstacksView
      //=========================================
      public LineCallstacksView()
      {
         InitializeComponent();
      }
      //=========================================
      // load
      //=========================================
      public bool load(uint lineNum, Hashtable BlockAllocs)
      {
         this.Text = " line :" + lineNum;

         //
         IDictionaryEnumerator _enumerator = BlockAllocs.GetEnumerator();
         while (_enumerator.MoveNext())
         {
            BlockAlloc block = (BlockAlloc)_enumerator.Value;


            SymbolInfo.LookupInfo li = new SymbolInfo.LookupInfo();

            string callstack = "";
            string spacing = "";
            for (int i = block.mContext.mBackTraceSize-1; i >=0; i--)
            {
               HaloWarsMem.getSymbolInfo().lookupImmediate(block.mContext.mBackTrace[i], ref li);

               string shortName = Path.GetFileName(li.mFilename);
               if (HaloWarsMem.getSymbolInfo().isIgnoreSymbol(shortName))
                  continue;

               string displayString = "";
               if(shortName == "?")
               {
                  displayString = "unkown (..)";
               }
               else
               {
                   displayString = shortName + "::" + li.mSymbol + " (line " + li.mLine + ") \n";
               }
               listBox1.Items.Add(spacing + displayString);

               //if (lastNode != null)
               //   lastNode = lastNode.Nodes.Add(displayString);
               //else
               //   lastNode = tv.Nodes.Add(displayString);

               spacing += " ";
            }

            listBox1.Items.Add(" (" + MemoryNumber.convert(block.mBlockSize) + ")");

            
            listBox1.Items.Add("==========================================================");
            listBox1.Items.Add("==========================================================");

         }

         return true;
      }
   }
}