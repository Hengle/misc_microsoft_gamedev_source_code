using System;
using System.IO;
using System.Collections.Generic;
using System.Windows.Forms;


using EditorCore;
using Rendering;
using Sim;


namespace LightingClient
{
   partial class LightingClientMain
   {

      #region command line

      int mAoSectionIndex = -1;
      int mAoNumSections = -1;
      
      string outputLog = null;
      string outputDir = null;
      string targetFile = null;

      int argStringContains(string[] args, String searchString)
      {
         for (int i = 0; i < args.Length; i++)
         {
            if (args[i] == searchString)
               return i;
         }
         return -1;
      }

      #endregion


      public enum eReturnCodes
      {
         cRC_OK = 0,
         cRC_TDL_NOT_FOUND,
         cRC_TDL_LOAD_ERROR,
         cRC_D3D_CREATE_ERROR,
         cRC_OUTPUT_WRITE_ERROR,
         eRC_UNKOWN,
      }
      public eReturnCodes generateLighting(string targetFile, string destDir, string issuingName, AmbientOcclusion.eAOQuality quality, int totalNumberOfSections, int targetSectionIndex)
      {
         mAoNumSections = totalNumberOfSections;
         mAoSectionIndex = targetSectionIndex;
         outputDir = destDir;

         //load our TLD file
         if (targetFile != "" && File.Exists(targetFile))
         {
            if (!loadTempDataFile(targetFile))
               return eReturnCodes.cRC_TDL_LOAD_ERROR; 
         }
         else
         {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("TDL File not specified, or not available.");
            Console.ForegroundColor = ConsoleColor.White;
            return eReturnCodes.cRC_TDL_NOT_FOUND; 
         }



         ////////////////////////////////////////////////////////////////////////////////
         
         TerrainGlobals.getTerrainExportRender().init();
         ModelManager.init();

        // AmbientOcclusion.eAOQuality quality = AmbientOcclusion.eAOQuality.cAO_Worst;
         int numSamples = (int) quality;
         int samplesPerSection = (int)(numSamples / mAoNumSections);
         int startSampleCount = samplesPerSection * mAoSectionIndex;
         int endSampleCount = startSampleCount + samplesPerSection;

         AmbientOcclusion ao = new AmbientOcclusion();
         float totalTime = 0;
         float peelTime = 0;
         float gatherTime = 0;
         ao.calcualteAO(quality, true, ref totalTime, ref peelTime, ref gatherTime, startSampleCount, endSampleCount);
         ao.destroy();
         ao = null;


         
         TerrainGlobals.getTerrainExportRender().destroy();
         ModelManager.destroy();


         //////////////////////////////////////////////////////////////////////////
         bool writingOK = true;
         {
            const int cMajik = 0xA001;

            string ouputDumpFilename = outputDir + @"\" + issuingName + ".AO" + targetSectionIndex;

            if (!Directory.Exists(outputDir))
            {
               Directory.CreateDirectory(outputDir);
            }
            FileStream s = null;
            try
            {
               s = File.Open(ouputDumpFilename, FileMode.OpenOrCreate, FileAccess.Write, FileShare.None);
            }
            catch (Exception e)
            {
               Console.ForegroundColor = ConsoleColor.Red;
               Console.WriteLine("Error writing result to" + ouputDumpFilename);
               Console.ForegroundColor = ConsoleColor.White;
               writingOK= false;
            }

            if(writingOK)
            {
               BinaryWriter f = new BinaryWriter(s);
               int width = (int)TerrainGlobals.getTerrain().mNumXVerts;
               int height = (int)TerrainGlobals.getTerrain().mNumZVerts;

               //write our header.
               f.Write(cMajik);
               f.Write(mAoNumSections);
               f.Write(mAoSectionIndex);
               f.Write(numSamples);
               f.Write(startSampleCount);
               f.Write(endSampleCount);

               f.Write(width);
               f.Write(height);
               for (int x = 0; x < width; x++)
               {
                  for (int y = 0; y < height; y++)
                  {
                     float AOVal = TerrainGlobals.getTerrain().getAmbientOcclusion(x, y);
                     f.Write(AOVal);
                  }
               }

               f.Close();
               s.Close();
            }
         }
         if (!writingOK)
            return eReturnCodes.cRC_OUTPUT_WRITE_ERROR;
         

         return eReturnCodes.cRC_OK;
      }

      
   };



}