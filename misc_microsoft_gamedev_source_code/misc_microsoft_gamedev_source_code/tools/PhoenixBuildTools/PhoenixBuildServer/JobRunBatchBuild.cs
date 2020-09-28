using System;
using System.Collections.Generic;
using System.Text;

using System.IO;

namespace PhoenixBuildServer
{
   class JobRunBatchBuild : Job
   {
      private string mBatchCommand = null;


      public JobRunBatchBuild(string command)
      {
         mBatchCommand = command;
      }

      override public QUITCODE run()
      {
         QUITCODE retValue = QUITCODE.NONE;

         retValue = base.run();
         if (retValue != QUITCODE.NONE)
            return (retValue);

         // Run batch command 
         Task buildBatchCommand = runBatch(mBatchCommand);
         retValue = buildBatchCommand.run();

         // If error we must write to the db.
         if (retValue == QUITCODE.ERROR)
         {
            setRunStatus(QUITCODE.ERROR);
         }

         if (mJobLog != null)
            mJobLog.Close();

         return (retValue);
      }
   }
}
