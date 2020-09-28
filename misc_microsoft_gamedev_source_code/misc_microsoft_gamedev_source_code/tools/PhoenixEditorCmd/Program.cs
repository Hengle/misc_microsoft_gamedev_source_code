using System;
using System.Collections.Generic;
using System.Text;

namespace PhoenixEditorCmd
{
   class Program
   {
      static void Main(string[] args)
      {         
         // Merge all arguments into one string
         //
         string allArgs = null;
         int argId = 0;

         foreach (string argument in args)
         {
            if (argId++ > 0)
               allArgs += " ";

            allArgs += argument;
         }

         // Run PhoenixEditor
         //
         try
         {
            System.Diagnostics.Process editorUtility = new System.Diagnostics.Process();
            editorUtility = System.Diagnostics.Process.Start("PhoenixEditor.exe", allArgs);
            editorUtility.WaitForExit();
            editorUtility.Close();
         }
         catch
         {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.Out.Write("Error: Unable to run PhoenixEditor.exe.\n\n");
            Console.ResetColor();
         }
      }
   }
}
