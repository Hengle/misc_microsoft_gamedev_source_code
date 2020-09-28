/*===================================
  Globals.cs
 * Origional code : Hoppe / Lebreve (MS Research)
 * Ported code : Colt "MainRoach" McAnlis (MGS Ensemble studios) [12.01.06]
===================================*/
using System;
using System.Collections.Generic;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Diagnostics;
using System.Text;
using System.Windows.Forms;
using System.IO;
using Rendering;
using EditorCore;

namespace TextureSynthesis
{
   class Globals
   {

      //CLM - this class is just a grouping of defines from the main system
      //The defines needed for the HLSL files have been copied off into the bin\shaders folder

      public static int NUM_RECOLORED_PCA_COMPONENTS = 8;
      public static int NUM_RUNTIME_PCA_COMPONENTS = 8;

      public static int NEIGHBORHOOD_RADIUS_SEARCH = 1; // 3x3 (size is 2*R+1)

      public static int QUANTIZE_NUM_BITS = 8;
      public static float QUANTIZE_PERCENT_INSIDE = 95.0f;

      public static bool FORCE_GAUSS = false;

      // ------------------------------------------------------
      // change *does* require to recompute analyse files

      public static int COARSE_LEVELS_SKIP_ANALYSE = 3; // ( >= 1 ; 1 -> no level skipped)
      public static int COARSE_LEVELS_SKIP_SYNTHESIS = 1; // ( >= 1 ; 1 -> no level skipped)
      public static int NEIGHBORHOOD_RADIUS_RECOLOR = 2; // 1 - 3x3   2 - 5x5  3 - 7x7  (size is 2*R+1)
      
      public static int K_NEAREST = 2; // 2,4,8

      public static bool USE_STACK_EXEMPLAR = true;// UPDATE correction.fx !!    //// WARNING :::> get rid of pyramid version - code path probably broken

      // misc
      public static int NEIGHBORHOOD_RADIUS_RT = 2;
      public static int NEIGHBORHOOD_RADIUS_MAX = (Math.Max(NEIGHBORHOOD_RADIUS_RT, Globals.NEIGHBORHOOD_RADIUS_SEARCH));
      public static float INIT_CONSTRAINT_THRESHOLD = 128.0f / 256.0f; // unit = normalized distance

      //#define FIRST_LEVEL_WITH_BORDER    3;
      public static int NUM_LEVELS_WITHOUT_BORDER = 4; // (3 last levels for 64^2 and 4 last for 128^3 are considered non toroidal)
      // This is arbitrary and experimentally chosen. Most likely related to feature size.

      public static bool TEST_PADDING = false; // RELEASE set to false // 'clear' before all renderings?


      public enum eSynthMethod
      {
         cSynth_Isometric = 0,
         cSynth_Ansiometric = 1
      };
      public static eSynthMethod gSynthMethod = eSynthMethod.cSynth_Isometric;//eSynthMethod.cSynth_Ansiometric;


      // ----------------------
      //NEIGHBORHOODS
      // configuration of non-square neighborhoods
      public static int NeighborhoodSynth_NUM_COMP = NUM_RECOLORED_PCA_COMPONENTS;
      public static int NeighborhoodSynth_RADIUS = 2;
      public static int NeighborhoodSynth_SIZE = (NeighborhoodSynth_RADIUS * 2 + 1);

      public static int NeighborhoodSynth_NUM_NEIGHBORS = 4;
      public static char[] NeighborhoodSynth_SHAPE = new char[]{' ',' ',' ',' ',' ', 
                                                           ' ','x',' ','x',' ', 
                                                           ' ',' ',' ',' ',' ', 
                                                           ' ','x',' ','x',' ', 
                                                           ' ',' ',' ',' ',' '};


      //SYNTHESIZER
      public static int SYNTH_PREFETCH_BORDER = 0;
      public static int INDIRECTION_BORDER_SIZE = 2; //NeighborhoodSynth_RADIUS



      public static int SYNTH_ORDER_NUM_PASSES = 2;

      public static int cNumCorrectionPasses = 2;
      public const int cNumLevels = 8;

      // 4 subpasses

      public static int[] SYNTH_SUB1234_ORDER = new int[] { 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0 }; // Sig 05
      public static int[] SYNTH_SUB1234_SUBPASS_SHRINK = {  2, 2 ,  1, 1 ,  1, 2 ,  1, 1 ,  2, 1 ,  1, 1 ,  2, 1 ,  1, 1 ,  0, 0  };

      public static int SYNTH_SUB1234_BORDER_L =(11); // NOTE: this is for the above ordering *only*
      public static int SYNTH_SUB1234_BORDER_R =(11); 
      public static int SYNTH_SUB1234_BORDER_T =(10);
      public static int SYNTH_SUB1234_BORDER_B =(10);

      // {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}} // zero shrink for debug

      public static bool TAKE_SCREEN_CAPTURES = false;
      public static bool PRINT_DEBUG_MSG = false;



      //STUFF
      static public byte FLOAT_2_UCHAR(float F)
      {
         return (byte)(Math.Min(Math.Max((F), 0.0f), 255.0f));
      }
      


      #region DEFINE SYSTEM
      //DEFINITIONS
      static Dictionary<string, string> m_Parameters = new Dictionary<string, string>();
      public static void clearAllDefs()
      {
         m_Parameters.Clear();
      }

      // Add a parameter. If an environment variable is defined, the
      // paramter will be initialized with its value.
      public static void addDef(String def)
      {
         if (!m_Parameters.ContainsKey(def))
            m_Parameters.Add(def,"");
      }

      // Returns the value of a parameter. NULL is returned if empty parameter.
      public static string getValue(String def)
      {
         String val = null;
         m_Parameters.TryGetValue(def, out val);
         return val;
      }

        // Set the value of a parameter
  // if 'add' is true, add the parameter if it does not exist
  // return true if success, false otherwise
      public static bool setValue(string def, string val, bool add)
      {
         if(!m_Parameters.ContainsKey(def))
         {
            if (add)
               addDef(def);
            else
               return false;
         }
         m_Parameters[def]=val;
         return true;
      }

      // Test if a paramter is defined (define means present + non empty value)
      public static bool isDefined(String def)
      {
         if (!m_Parameters.ContainsKey(def))
            return false;
         String val = "";
         m_Parameters.TryGetValue(def, out val);
         return val.Length > 0;
      }
      #endregion


      #region ASSERTS
      public static void Assert(bool value)
      {
         if (!value)
         {
            //    MessageBox.Show("Assert Failed!");
            Debug.Assert(value);
         }

      }
      public static void Assert(bool value, String result)
      {
         if (!value)
         {
            //   MessageBox.Show(result);
            Debug.Assert(value);
         }

      }
      #endregion
   }

}