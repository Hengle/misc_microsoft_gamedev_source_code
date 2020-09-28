using System;
using System.Collections.Generic;
using System.Text;

namespace RemoteMemory
{
   class GlobalErrors
   {
      static Queue<String> mErrors = new Queue<string>();

      //============================================================================
      // addError
      //============================================================================
      static public void addError(string err)
      {
         mErrors.Enqueue(err);
      }


      //============================================================================
      // addError
      //============================================================================
      static public string popError()
      {
         if (mErrors.Count == 0)
            return "";

         return mErrors.Dequeue();
      }

      //============================================================================
      // clearErrors
      //============================================================================
      static public void clearErrors()
      {
         mErrors.Clear();
      }


   }
}
