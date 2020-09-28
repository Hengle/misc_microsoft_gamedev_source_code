using System;
using System.Collections.Generic;
using System.Text;

namespace RemoteMemory
{
   public class ThreadSafeCallbackList
   {
      public delegate void MessagePacketCallback(string messageID, object objData);

      public class MessagePacket
      {
         public MessagePacket()
         {
            mProducerThreadID = System.Threading.Thread.CurrentThread.ManagedThreadId;
         }
         private int mProducerThreadID = -1;  //useful for debugging

         public string mMessageID = "";
         public MessagePacketCallback mCallback = null;
         public object mDataObject = null;
      };

      ~ThreadSafeCallbackList()
      {
         mMessageQueue = null;
      }

      protected System.Collections.Queue mMessageQueue = System.Collections.Queue.Synchronized(new System.Collections.Queue());

      public void enqueueMessage(MessagePacket packet)
      {
         mMessageQueue.Enqueue(packet);
      }

      public MessagePacket dequeueMessage()
      {
         return (MessagePacket)mMessageQueue.Dequeue();
      }

      public void dequeueProcessMessage()
      {
         MessagePacket mp = dequeueMessage();
         if (mp != null && mp.mCallback != null)
         {
            mp.mCallback(mp.mMessageID, mp.mDataObject);
         }
      }
      public int Count()
      {
         return mMessageQueue.Count;
      }
   };
}
