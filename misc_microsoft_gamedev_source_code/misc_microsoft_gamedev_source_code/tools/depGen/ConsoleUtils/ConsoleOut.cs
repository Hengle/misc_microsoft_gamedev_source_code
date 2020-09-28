using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace ConsoleUtils
{
   static public class ConsoleOut
   {

      public enum MsgType
      {
         Info,
         Warn,
         Error
      };

      static private bool mQuietFlag = false;
      static private string mLogFileName = null;
      static private string mErrorLogFileName = null;
      static private StreamWriter mLogFileWriter = null;
      static private StreamWriter mErrorLogFileWriter = null;

      static private string mDepGenTag = "[DEPGEN] ";


      static public bool init(bool quiet, string logFile, string errorLogFile)
      {
         mQuietFlag = quiet;
         mLogFileName = logFile;
         mErrorLogFileName = errorLogFile;

         if (!openLogFile())
            return false;

         if (!openErrorLogFile())
            return false;

         return true;
      }

      static public void deinit()
      {
         closeLogFile();
         closeErrorLogFile();
      }

      static private bool openLogFile()
      {
         closeLogFile();

         if (!String.IsNullOrEmpty(mLogFileName))
         {
            try
            {
               mLogFileWriter = File.CreateText(mLogFileName);
               if (mLogFileWriter == null)
               {
                  Write(MsgType.Error, "Unable to create log file \"{0}\"\n", mLogFileName);
                  return false;
               }
            }
            catch
            {
            }
         }

         return true;
      }

      static private void closeLogFile()
      {
         if (mLogFileWriter != null)
            mLogFileWriter.Close();
         mLogFileWriter = null;
      }


      static private bool openErrorLogFile()
      {
         closeErrorLogFile();

         if (!String.IsNullOrEmpty(mErrorLogFileName))
         {
            try
            {
               mErrorLogFileWriter = File.CreateText(mErrorLogFileName);
               if (mErrorLogFileWriter == null)
               {
                  Write(MsgType.Error, "Unable to create error log file \"{0}\"\n", mErrorLogFileName);
                  return false;
               }
            }
            catch
            {
            }
         }

         return true;
      }

      static private void closeErrorLogFile()
      {
         if (mErrorLogFileWriter != null)
            mErrorLogFileWriter.Close();
         mErrorLogFileWriter = null;
      }


      static public void Write(string s, params object[] args)
      {
         if (!mQuietFlag)
         {
            Console.Out.Write(mDepGenTag);
            Console.Out.Write(s, args);
         }

         if (mLogFileWriter != null)
         {
            mLogFileWriter.Write(mDepGenTag);
            mLogFileWriter.WriteLine(s, args);
         }
      }

      static public void Write(MsgType msgType, string s, params object[] args)
      {
         // Set start string
         string start = null;
         switch (msgType)
         {
            case MsgType.Info:
               start = "";
               break;

            case MsgType.Warn:
               start = "Warning: ";
               break;

            case MsgType.Error:
               start = "Error: ";
               break;
         }

         if (!mQuietFlag)
         {
            // Set console color
            switch (msgType)
            {
               case MsgType.Info:
                  Console.ResetColor();
                  break;

               case MsgType.Warn:
                  Console.ForegroundColor = ConsoleColor.Yellow;
                  break;

               case MsgType.Error:
                  Console.ForegroundColor = ConsoleColor.Red;
                  break;
            }

            Console.Out.Write(mDepGenTag);
            Console.Out.Write(start);
            Console.Out.Write(s, args);

            Console.ResetColor();
         }

         if (mLogFileWriter != null)
         {
            mLogFileWriter.Write(mDepGenTag);
            mLogFileWriter.Write(start);
            mLogFileWriter.WriteLine(s, args);
         }

         if ((mErrorLogFileWriter != null) && (msgType != MsgType.Info))
         {
            mErrorLogFileWriter.Write(mDepGenTag);
            mErrorLogFileWriter.Write(start);
            mErrorLogFileWriter.WriteLine(s, args);
         }
      }


   }
}
