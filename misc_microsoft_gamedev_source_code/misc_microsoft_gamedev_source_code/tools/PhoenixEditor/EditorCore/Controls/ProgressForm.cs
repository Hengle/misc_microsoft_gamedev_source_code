using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace EditorCore
{
   public partial class ProgressForm : Form
   {
      public ProgressForm()
      {
         InitializeComponent();

         this.StartPosition = FormStartPosition.CenterScreen;

         this.Shown += new EventHandler(ProgressForm_Shown);
      }
      bool mShown = false;
      void ProgressForm_Shown(object sender, EventArgs e)
      {
         mShown = true;
         ////throw new Exception("The method or operation is not implemented.");
         foreach (TaskStatus t in mTasks)
         {
            listBox1.Items.Add(t);
            //listView1.Items.Add(
         }
         listBox1.Refresh();
      }

      private void AddSerialStatusInternal(TaskStatus status)
      {

         if (mTasks.Count > 1)
            listBox1.Items.Add(mTasks[mTasks.Count - 2].ToString());

         //listBox1.Items.Add(status);
         textBox1.Text = (status.ToString());
         listBox1.Refresh();
         listBox1.Update();
      }

      delegate void StringPassDel(TaskStatus status);
      //StringPassDel myDelegate;
      //myDelegate = new StringPassDel(UpdateInternal);

      public void AddSerialStatus(string status)
      {
        
         foreach (TaskStatus t in mTasks)
            t.Complete();
         TaskStatus newTask = new TaskStatus(status);
         mTasks.Add(newTask);
//this.Handle != null)// &&
         if (mShown == true)// && this.Handle != null)
         {
            this.Invoke(new StringPassDel(AddSerialStatusInternal), newTask);
         }
         else
         {
            status = this.ToString();
         }

         //AddSerialStatusInternal(newTask);

      }

      List<TaskStatus> mTasks = new List<TaskStatus>();
      //public void AddStatus(string status)
      //{
      //}
      //Dictionary<string, TaskStatus> mTaskStatus = new Dictionary<string, TaskStatus>();

      private class TaskStatus
      {
         DateTime mStartTime;
         double mElapsedtime = -1;
         string mName;
         public TaskStatus(string name)
         {
            mStartTime = System.DateTime.Now;
            mName = name;
         }
         public void Complete()
         {
            mElapsedtime = ((TimeSpan)(System.DateTime.Now - mStartTime)).TotalMilliseconds;
         }
         public override string ToString()
         {
            if (mElapsedtime != -1)
            {
               return mName + " " + mElapsedtime.ToString();
            }
            return mName;  
         }


      }


   }

   public class ProgressHelper
   {
      static ProgressForm mForm = null;
      static object mSyncRoot = new object();
      static string mFirstMessage = "";
      static public void StatusMessage(string status)
      {

         //if (mForm == null || mForm.IsDisposed)
         //{
         //   mFirstMessage = status;
         //   System.Threading.Thread start = new System.Threading.Thread(new System.Threading.ThreadStart(StartForm));
         //   start.Name = "arrrg";
         //   start.Start();
         //   System.Threading.Thread.Sleep(1000);
         //   //StartForm();
         //}
         ////else
         //{
         //   //lock (mSyncRoot)
         //   {

         //      mForm.AddSerialStatus(status);
         //   }
         //}
      }
      static void StartForm()
      {
         ////lock (mSyncRoot)
         //{
         //   mForm = new ProgressForm();
            
         //   //mForm.Show();
         //   Application.Run(mForm);
         //}
         ////mForm.AddSerialStatus(mFirstMessage);

         ////while (mForm != null && mForm.IsDisposed == false)
         ////{
         ////   System.Threading.Thread.Sleep(1000);
         ////}
      }
      static public void Finished(bool stayOpen)
      {
         ////lock (mSyncRoot)
         //{
         //   mForm.AddSerialStatus("Finished");
         //   if (stayOpen == false)
         //      mForm.Close();
         //}
         ////mForm = null;
      }


   }


   //todo bring task status up a level..
   //build profiler system using the 'using' statement
   //hook up to realtime profiler
   //profile macro task
}