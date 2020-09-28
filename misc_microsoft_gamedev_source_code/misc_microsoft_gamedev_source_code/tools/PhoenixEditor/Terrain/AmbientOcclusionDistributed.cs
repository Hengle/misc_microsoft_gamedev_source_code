
using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Windows.Forms;
using System.Threading;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Xml;
using System.Xml.Serialization;
using System.Net;

using Xceed.FileSystem;
using Xceed.Compression;
using Xceed.Zip;

using EditorCore;
using Rendering;
using SimEditor;
using Export360;

//----------------------------------

namespace Terrain
{
   /// <summary>
   /// This class uses a networking system to distribute the AO generation process
   /// </summary>
   public class DistributedAOGen
   {

      const int cNumSections = 20;
      private BackgroundWorker mWorkerThread = null;
      Controls.AOGenDialog mControllingDialog = null;

      private int mNumSectionsToComplete = 0;
      private string mCurrJobGUID = "";
      private bool mIncludeObjects = false;

      #region XML for file reading
      [XmlRoot("pendingJob")]
      public class pendingJobXML
      {
         [XmlAttribute]
         public DateTime lastModified;   //datetime

         [XmlAttribute]
         public string issuingSystemName;
         [XmlAttribute]
         public string issuingSourceFile;
         [XmlAttribute]
         public string workingJobGUID;

         [XmlAttribute]
         public AmbientOcclusion.eAOQuality quality;

         [XmlAttribute]
         public int totalNumberOfSections;
         [XmlAttribute]
         public int targetSectionIndex;


      };
      #endregion


    

      public void issueAOGenLocal(AmbientOcclusion.eAOQuality quality,bool includeObjects, Controls.AOGenDialog primDlg)
      {
         mControllingDialog = primDlg;
         if (mControllingDialog != null)
            mControllingDialog.setNumWorkUnits(1);

         TerrainGlobals.getEditor().computeAmbientOcclusion(quality, includeObjects);
         if (mControllingDialog != null)
            mControllingDialog.increaseWorkUnitCount();
        
      }

      public bool issueAOGenJob(AmbientOcclusion.eAOQuality quality,bool includeObjects)
      {
         return issueAOGenJob(quality, includeObjects,null);
      }
      public bool issueAOGenJob(AmbientOcclusion.eAOQuality quality, bool includeObjects,Controls.AOGenDialog primDlg)
      {
         networkAOInterface.ensureDirectories();

         deleteExistingInputFile();
         deleteExistingResults();
         deleteExistingJobs();

         if (doIHavePendingJobs())
         {
            MessageBox.Show("There are still jobs still being processed from this client. \n Please try again in a few moments.");
            return false;
         }


         string mHostName = Dns.GetHostName();
         mCurrJobGUID = System.Guid.NewGuid().ToString();
         mIncludeObjects = includeObjects;

         //Now, start up a thread to watch for file IO
         mControllingDialog = primDlg;
         if (mControllingDialog != null)
            mControllingDialog.setNumWorkUnits(cNumSections);

         mNumSectionsToComplete = cNumSections;


         mWorkerThread = new BackgroundWorker();
         mWorkerThread.WorkerReportsProgress = true;
         mWorkerThread.WorkerSupportsCancellation = true;
         mWorkerThread.DoWork += bw_DoWork;
         mWorkerThread.ProgressChanged += bw_ProgressChanged;
         mWorkerThread.RunWorkerCompleted += bw_RunWorkerCompleted;

         mWorkerThread.RunWorkerAsync(quality);


         {
            //clear current AO Values to zero
            float[] AOVals = TerrainGlobals.getEditor().getAmbientOcclusionValues();
            for (int i = 0; i < AOVals.Length; i++)
               AOVals[i] = 0.0f;

            if (TerrainGlobals.getTerrain().getQuadNodeRoot() != null)
               TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
         }




         return true;
      }
      public void cancelAOGenJob()
      {
         //issue the cancel command to the worker thread
         //let it clean up everything Async style.
         if (mWorkerThread != null)
         {
            if (mWorkerThread.IsBusy)
               mWorkerThread.CancelAsync();
         }
      }


      bool doIHavePendingJobs()
      {
         //Delete any pending work that hasn't been picked up yet.
         string[] files = Directory.GetFiles(networkAOInterface.JobsDir, Dns.GetHostName() + "*.aojob");
         return files.Length != 0;
      }
      bool areJobsBeingProcessed()
      {
         //UG this bangs on the pending jobs more than i'd like...

         // if we have access to ALL of the files, then they have not been processed yet..
         string[] files = Directory.GetFiles(networkAOInterface.JobsDir, Dns.GetHostName() + "*.aojob");

         for (int i = 0; i < files.Length; i++)
         {
            try
            {
               Stream fs = File.Open(files[i], FileMode.Open, FileAccess.Read, FileShare.None);
               fs.Close();
            }
            catch (Exception e)
            {
               return true;
            }
         }
         return false;
      }
      void deleteExistingJobs()
      {
         //Delete any pending work that hasn't been picked up yet.
         string[] files = Directory.GetFiles(networkAOInterface.JobsDir, Dns.GetHostName() + "*.aojob");

         for (int i = 0; i < files.Length; i++)
         {
            try
            {
               File.Delete(files[i]);
            }
            catch (Exception e)
            {

            }
         }
      }
      void deleteExistingResults()
      {
         string targetDir = networkAOInterface.ResultDir + Dns.GetHostName();
         if (!Directory.Exists(targetDir))
            return;

         string[] files = Directory.GetFiles(targetDir);

         for (int i = 0; i < files.Length; i++)
         {
            bool OK = false;
            while(!OK)
            {
               try
               {
                  File.Delete(files[i]);
                  OK = true;
               }
               catch (Exception e)
               {

               }
            }
         }

         Directory.Delete(targetDir);
      }
      void deleteExistingInputFile()
      {
         //delete our source input file
         string sourceInputFile = networkAOInterface.SourceDir + "\\" + Dns.GetHostName() + "\\data.TDL";
         if (!File.Exists(sourceInputFile))
            return;

         bool OK = false;
         while (!OK)
         {
            try
            {
               File.Delete(sourceInputFile);
               OK = true;
            }
            catch(Exception e)
            {

            }
         }
      }

      void bw_DoWork(object sender, DoWorkEventArgs e)
      {
         string mHostName = Dns.GetHostName();
         AmbientOcclusion.eAOQuality quality = (AmbientOcclusion.eAOQuality)e.Argument;
         //Copy our data & jobs on a worker thread
         //write our temporary data
         Directory.CreateDirectory(networkAOInterface.SourceDir + "\\" + mHostName);
         string sourceInputFile = networkAOInterface.SourceDir + "\\" + mHostName + "\\data.ZIP";
         writeTempData(sourceInputFile);

         if (mWorkerThread.CancellationPending)
         {
            e.Cancel = true;
            return;
         }

         //Issue our job files to the network
         XmlSerializer s = new XmlSerializer(typeof(pendingJobXML), new Type[] { });
         for (int i = 0; i < cNumSections; i++)
         {
            pendingJobXML pending = new pendingJobXML();
            pending.workingJobGUID = mCurrJobGUID;
            pending.issuingSystemName = mHostName;
            pending.quality = quality;
            pending.targetSectionIndex = i;
            pending.totalNumberOfSections = cNumSections;
            pending.issuingSourceFile = sourceInputFile;
            pending.lastModified = DateTime.Now;

            string jobFilename = networkAOInterface.JobsDir + mHostName + "_" + i + "_" + mCurrJobGUID + ".aojob";
            Stream st = File.Open(jobFilename, FileMode.CreateNew, FileAccess.Write, FileShare.None);
            s.Serialize(st, pending);
            st.Close();
         }


         string targetDir = networkAOInterface.ResultDir + Dns.GetHostName();
         List<string> filesToCheck = new List<string>();
         for (int i = 0; i < mNumSectionsToComplete; i++)
            filesToCheck.Add(targetDir + "\\" + Dns.GetHostName() + ".AO" + i);

         int notProcessedTimer = 10;//in seconds (due to the sleep @ the bottom of this thread
         while (filesToCheck.Count != 0)
         {
            //did we cancel?
            if (mWorkerThread.CancellationPending)
            {
               e.Cancel = true;
               return;
            }

            //if NONE of our work has been picked up then either the clients are all down, or they are all busy
            //ask the user if they want to keep waiting.
            if (!areJobsBeingProcessed())
            {
               if (notProcessedTimer <= 0)
               {
                  DialogResult res = MessageBox.Show(
                                                      "The issued jobs have not been acquired by the working clients yet. \n "+
                                                      "This may mean that the clients are down, or otherwise busy with other work. \n"+
                                                      "Would you like to continue to wait? \n\n "+
                                                      "Click YES to wait in line for the network. \n\n Click NO to cancel. ",
                                                      "Think about it..", 
                                                      MessageBoxButtons.YesNo, 
                                                      MessageBoxIcon.Exclamation);
                  if (res == DialogResult.Yes)
                     notProcessedTimer = 10;//in seconds (due to the sleep @ the bottom of this thread
                  else if (res == DialogResult.No)
                  {
                     e.Cancel = true;
                     return;
                  }
               }
               else
                  notProcessedTimer--;
            }
            else
               notProcessedTimer = 60;  //reset the timer
 

            for (int i = 0; i < filesToCheck.Count; i++)
            {
               if (File.Exists(filesToCheck[i]))
               {
                  mWorkerThread.ReportProgress(mNumSectionsToComplete-1-filesToCheck[i].Length, filesToCheck[i]);
                  filesToCheck.RemoveAt(i);
                  i--;
               }
            }


            Thread.Sleep(1000);
         }

         e.Result = mNumSectionsToComplete;// This gets passed to RunWorkerCopmleted

      }

      void bw_ProgressChanged(object sender, ProgressChangedEventArgs e)
      {
         addAOFileSegment((string)e.UserState);
         if (mControllingDialog != null)
            mControllingDialog.increaseWorkUnitCount();
      }
      void addAOFileSegment(string inputFilename)
      {
         int cMajik = 0xA001;

         //The source may not be done writing the file out just yet. Spinloop here until we're sure we have access
         FileStream sr = null;
         while (sr == null)
         {
            try
            {
               sr = File.Open(inputFilename, FileMode.Open, FileAccess.Read);
            }
            catch (Exception e)
            {

            }
         }
            
            
         BinaryReader fr = new BinaryReader(sr);

         int majik = fr.ReadInt32();
         if (majik != cMajik)
         {
            sr.Close();
            sr = null;
            fr.Close();
            fr = null;

            return;
         }

         int numSectionsT = fr.ReadInt32();
         Debug.Assert(numSectionsT == mNumSectionsToComplete);

         int mySecetion = fr.ReadInt32();
         int numSamples = fr.ReadInt32();
         int startSampleCount = fr.ReadInt32();
         int endSampleCount = fr.ReadInt32();
         int width = fr.ReadInt32();
         int height = fr.ReadInt32();


         float[] AOVals = TerrainGlobals.getEditor().getAmbientOcclusionValues();
         for (int x = 0; x < width * height; x++)
         {
            AOVals[x] += fr.ReadSingle();
            AOVals[x] = BMathLib.Clamp(AOVals[x], 0, 1);
            //Debug.Assert(AOVals[x] <= 1);
         }

         sr.Close();
         sr = null;
         fr.Close();
         fr = null;


         if (TerrainGlobals.getTerrain().getQuadNodeRoot() != null)
            TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
      }
      void buildAOFiles(string inputFilename)
      {
         for (int i = 0; i < mNumSectionsToComplete; i++)
         {
            //work\scenario\development\coltTest\coltTest.AO0, .AO1, .AO2...., .AON
            string outFileName = Path.ChangeExtension(inputFilename, ".AO" + i);

            addAOFileSegment(outFileName);
         }
      }

      void bw_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
      {
         if (e.Cancelled)
         {
            Thread.Sleep(500);   //Sleep a little bit to ensure everyone is properly finished
            deleteExistingJobs();
            deleteExistingInputFile();

            TerrainGlobals.getEditor().clearAmbientOcclusion();
            if (TerrainGlobals.getTerrain().getQuadNodeRoot() != null)
               TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
         }
         else if (e.Error != null)
         {
            //Console.WriteLine("Worker exception: " + e.Error.ToString());
         }
         else //completed OK
         {
            Thread.Sleep(500);   //Sleep a little bit to ensure everyone is properly finished
            //clean up the directory
            deleteExistingResults();

            deleteExistingInputFile();
         }
      }



      public class networkAOInterface
      {
         #region directories
         static public string cDistrubtedWorkRootDir = @"\\esfile\phoenix\Tools\DistributedLighting\";
         static private string cJobsFileDir = @"jobFiles\";
         static private string cSourceFileDir = @"sourceFiles\";
         static private string cResultFileDir = @"resultFiles\";   //subdirs by issuing system name

         static public string JobsDir
         {
            get { return cDistrubtedWorkRootDir + cJobsFileDir; }
         }
         static public string SourceDir
         {
            get { return cDistrubtedWorkRootDir + cSourceFileDir; }
         }
         static public string ResultDir
         {
            get { return cDistrubtedWorkRootDir + cResultFileDir; }
         }

         #endregion

         static public bool doesWorkExist()
         {
            string[] files = Directory.GetFiles(JobsDir, "*.aojob");
            return files.Length != 0;
         }
         static public bool networkAccessAvailable()
         {
            return Directory.Exists(@"\\esfile\phoenix\");
         }

         static public void ensureDirectories()
         {
            if (!Directory.Exists(cDistrubtedWorkRootDir))
               Directory.CreateDirectory(cDistrubtedWorkRootDir);

            if (!Directory.Exists(cDistrubtedWorkRootDir + cJobsFileDir))
               Directory.CreateDirectory(cDistrubtedWorkRootDir + cJobsFileDir);

            if (!Directory.Exists(cDistrubtedWorkRootDir + cSourceFileDir))
               Directory.CreateDirectory(cDistrubtedWorkRootDir + cSourceFileDir);

            if (!Directory.Exists(cDistrubtedWorkRootDir + cResultFileDir))
               Directory.CreateDirectory(cDistrubtedWorkRootDir + cResultFileDir);
         }
      };


      

      
      void writeTempData(string filename)
      {
         //if we've already got the source file written, delete it
         if (File.Exists(Path.ChangeExtension(filename, ".ZIP")))
            File.Delete(Path.ChangeExtension(filename, ".ZIP"));


         DiskFile zipFile = new DiskFile(Path.ChangeExtension(filename,".ZIP"));
         if (!zipFile.Exists)
            zipFile.Create();

         ZipArchive zip = new ZipArchive(zipFile);
         zip.DefaultCompressionMethod = CompressionMethod.Deflated;
         zip.AllowSpanning = true;

         writeTempTerrainToZip(zip);
         writeTempModelsToZip(zip);
         
      }
      public class ObjectInstanceXML
      {
         [XmlIgnore]
         private Matrix mOrientation;
         public void setOrientation(Matrix m)
         {
            mOrientation = m;
         }
         [XmlAttribute]
         public string Orientation
         {
            get
            {
               string outV =  mOrientation.M11 + "," + mOrientation.M12 + "," + mOrientation.M13 + "," + mOrientation.M14 + "," +
                              mOrientation.M21 + "," + mOrientation.M22 + "," + mOrientation.M23 + "," + mOrientation.M24 + "," +
                              mOrientation.M31 + "," + mOrientation.M32 + "," + mOrientation.M33 + "," + mOrientation.M34 + "," +
                              mOrientation.M41 + "," + mOrientation.M42 + "," + mOrientation.M43 + "," + mOrientation.M44;

               return outV;
            }
            set
            {
               char[] sep = new char[]{','};
               string[] vs = value.Split(sep);

               mOrientation.M11 = System.Convert.ToSingle(vs[0]); mOrientation.M12 = System.Convert.ToSingle(vs[1]); mOrientation.M13 = System.Convert.ToSingle(vs[2]); mOrientation.M14 = System.Convert.ToSingle(vs[3]);
               mOrientation.M21 = System.Convert.ToSingle(vs[4]); mOrientation.M22 = System.Convert.ToSingle(vs[5]); mOrientation.M23 = System.Convert.ToSingle(vs[6]); mOrientation.M24 = System.Convert.ToSingle(vs[7]);
               mOrientation.M31 = System.Convert.ToSingle(vs[8]); mOrientation.M32 = System.Convert.ToSingle(vs[9]); mOrientation.M33 = System.Convert.ToSingle(vs[10]); mOrientation.M34 = System.Convert.ToSingle(vs[11]);
               mOrientation.M41 = System.Convert.ToSingle(vs[12]); mOrientation.M42 = System.Convert.ToSingle(vs[13]); mOrientation.M43 = System.Convert.ToSingle(vs[14]); mOrientation.M44 = System.Convert.ToSingle(vs[15]);

            }
         }
         [XmlAttribute]
         public string GR2Filename;
      };

      [XmlRoot("SceneObjects")]
      public class SceneObjectsXML
      {
         [XmlAttribute]
         public string aabbmax;

         [XmlAttribute]
         public string aabbmin;

         [XmlArrayItem(ElementName = "gr2names", Type = typeof(string))]
         [XmlArray("gr2names")]
         public List<string> objectGR2Names = new List<string>();

         [XmlArrayItem(ElementName = "objectInstances", Type = typeof(ObjectInstanceXML))]
         [XmlArray("objectInstances")]
         public List<ObjectInstanceXML> objectinstances = new List<ObjectInstanceXML>();

      };

      void writeTempModelsToZip(ZipArchive zip)
      {

         SceneObjectsXML sceneObjects = new SceneObjectsXML();

         BBoundingBox objectAABB = new BBoundingBox();
         objectAABB.empty();

         string baseDir = CoreGlobals.getWorkPaths().mGameDirectory;

         if (mIncludeObjects)
         {

            //searalize an XML file to memorystream holding position and model names

            List<EditorObject> editObjs = SimGlobals.getSimMain().getEditorObjects(false, SimMain.eFilterTypes.cFilterAll, -1, false);
            for (int objIdx = 0; objIdx < editObjs.Count; objIdx++)
            {
               if (editObjs[objIdx] == null)
                  continue;

               if (editObjs[objIdx].GetType() == typeof(SimObject))
               {
                  SimObject obj = editObjs[objIdx] as SimObject;

                  if (obj.IgnoreToAO)
                     continue;
                        

                  if (obj != null && obj.ProtoObject != null)
                  {
                     string grannyName = obj.ProtoObject.getGrannyFileName();
                     if (grannyName == "")
                        continue;
                      
                     if (grannyName.Contains(baseDir))
                        grannyName = grannyName.Remove(0, baseDir.Length);

                     //if this GR2 isn't already listed, then list it.
                     if (!sceneObjects.objectGR2Names.Contains(grannyName))
                     {
                        sceneObjects.objectGR2Names.Add(grannyName);
                     }


                     //add our instance
                     ObjectInstanceXML inst = new ObjectInstanceXML();
                     inst.GR2Filename = grannyName;
                     inst.setOrientation(obj.getMatrix());
                     sceneObjects.objectinstances.Add(inst);


                     //add our transformed BB to the global BB list
                     if (obj != null && obj.mVisual != null)
                     {
                        if (!obj.IgnoreToAO)
                        {
                           objectAABB.addPoint(obj.mAABB.max + obj.getPosition());
                           objectAABB.addPoint(obj.mAABB.min + obj.getPosition());
                        }
                     }
                  }
               }
            }

            sceneObjects.aabbmin = TextVectorHelper.ToString(objectAABB.min);
            sceneObjects.aabbmax = TextVectorHelper.ToString(objectAABB.max);
         }

         //write it to an XML stream
         AbstractFile md = zip.CreateFile("modelPositions.xml", true);
         Stream stream = md.OpenWrite(true);
         XmlSerializer s = new XmlSerializer(typeof(SceneObjectsXML), new Type[] { });
         s.Serialize(stream, sceneObjects);
         stream.Close();
         
         //Create a folder and copy our GR2s into it
         //AbstractFolder fold = zip.CreateFolder("models");

         //if (mIncludeObjects)
         //{
         //   for (int modelIdx = 0; modelIdx < sceneObjects.objectGR2Names.Count; modelIdx++)
         //   {
         //      if (mWorkerThread.CancellationPending)
         //         return;


         //      try
         //      {
         //         if (fullGR2Names[modelIdx] == "")
         //            continue;
         //         DiskFile modelFile = new DiskFile(fullGR2Names[modelIdx]);
         //         modelFile.CopyTo(fold, true);
         //      }
         //      catch (Exception e)
         //      {
         //         continue;
         //      }
         //   }
         //}
      }
      void writeTempTerrainToZip(ZipArchive zip)
      {
         int version = 0;

         //Write our terrain information
         AbstractFile md = zip.CreateFile("terrain.tdl", true);    

         //Write the size of our data..
         Stream stream = md.OpenWrite(true);//File.Open(filename, FileMode.OpenOrCreate, FileAccess.Write);
         BinaryWriter bw = new BinaryWriter(stream);

         bw.Write(version);
         bw.Write(TerrainGlobals.getTerrain().getNumXVerts());
         bw.Write(TerrainGlobals.getTerrain().getNumZVerts());
         bw.Write(TerrainGlobals.getTerrain().getTileScale());

         //min
         Vector3 min = TerrainGlobals.getTerrain().getQuadNodeRoot().getDesc().m_min;
         bw.Write(min.X); bw.Write(min.Y); bw.Write(min.Z);

         //max
         Vector3 max = TerrainGlobals.getTerrain().getQuadNodeRoot().getDesc().m_max;
         bw.Write(max.X); bw.Write(max.Y); bw.Write(max.Z);

         //write terrain positions
         for (int x = 0; x < TerrainGlobals.getTerrain().getNumXVerts(); x++)
         {
            for (int z = 0; z < TerrainGlobals.getTerrain().getNumZVerts(); z++)
            {
               Vector3 pos = TerrainGlobals.getTerrain().getRelPos(x, z);
               bw.Write(pos.X); bw.Write(pos.Y); bw.Write(pos.Z);
            }
         }

         for (int x = 0; x < TerrainGlobals.getTerrain().getNumXVerts(); x++)
         {
            for (int z = 0; z < TerrainGlobals.getTerrain().getNumZVerts(); z++)
            {
               Vector3 pos = TerrainGlobals.getTerrain().getNormal(x, z);
               bw.Write(pos.X); bw.Write(pos.Y); bw.Write(pos.Z);
            }
         }



         //write quadnodes
         BTerrainQuadNode[] mNodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
         bw.Write(mNodes.Length);
         for (int i = 0; i < mNodes.Length; i++)
         {
            BTerrainQuadNodeDesc desc = mNodes[i].getDesc();
            bw.Write(desc.mMinXVert);
            bw.Write(desc.mMinZVert);
         }

         bw.Close();
         stream.Close();

      }

   };
}