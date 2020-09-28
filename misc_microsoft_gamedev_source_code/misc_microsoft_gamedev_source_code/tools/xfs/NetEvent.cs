using System;
using System.Collections.Generic;
using System.Text;

namespace xfs
{
   public class NetEvent
   {
      public enum EventType
      {
         ClientConnect = 1,
         ClientDisconnect,
         ConnectionOpen,
         ConnectionClose,
         ConnectionReset,
         FileOpen,
         FileClose,
         General,
         ResetClient,
         Exception,
      };

      public EventType  mType;
      public string     mText;

      override public string ToString()
      {
         string s="";
         switch (mType)
         {
            case EventType.ClientConnect: s = "Client Connect "; break;
            case EventType.ClientDisconnect: s = "Client Disconnect "; break;
            case EventType.ConnectionOpen: s = "Connection Open "; break;
            case EventType.ConnectionClose: s = "Connection Close "; break;
            case EventType.ConnectionReset: s = "Connection Reset "; break;
            case EventType.FileOpen: s = "Open "; break;
            case EventType.FileClose: s = "Close "; break;
            case EventType.ResetClient: s = "ResetClient"; break;
            case EventType.Exception: s = "Exception"; break;
         }
         s = s + mText;
         return s;
      }
   }
}
