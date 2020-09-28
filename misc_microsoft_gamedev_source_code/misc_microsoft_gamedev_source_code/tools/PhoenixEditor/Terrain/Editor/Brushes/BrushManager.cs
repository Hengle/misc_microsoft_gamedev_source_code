
namespace Terrain
{

   public class BTerrainBrush
   {
      protected bool m_bApplyOnSelection = false;


      public BTerrainBrush() { }
      ~BTerrainBrush() { }

      public bool isApplyOnSelectionEnabled()
                  { return (m_bApplyOnSelection); }
   };
   //-----------------------------------
   //-----------------------------------
   //-----------------------------------
   class BrushManager
   {
      //Vertex Brushes
      static public BTerrainVertexBrush getHeightBrush()
      {
         if (mHeightBrush == null) mHeightBrush = new BTerrainHeightBrush();
         return mHeightBrush;
      }
      static public BTerrainVertexBrush getPushBrush()
      {
         if(mPushBrush == null) mPushBrush = new BTerrainPushBrush();
         return mPushBrush;
      }
      static public BTerrainVertexBrush getAvgHeightBrush()
      {
         if(mAvgHeightBrush == null) mAvgHeightBrush = new BTerrainAvgHeightBrush();
         return mAvgHeightBrush;
      }
      static public BTerrainVertexBrush getStdBrush()
      {
         if (mStdBrush == null) mStdBrush = new BTerrainStdBrush();
         return mStdBrush;
      }
      static public BTerrainVertexBrush getLayerBrush()
      {
         if(mLayerBrush == null) mLayerBrush = new BTerrainLayerBrush();
         return mLayerBrush;
      }
      static public BTerrainVertexBrush getInflateBrush()
      {
         if(mInflateBrush == null) mInflateBrush = new BTerrainInflateBrush();
         return mInflateBrush;
      }
      static public BTerrainVertexBrush getSmoothBrush()
      {
         if(mSmoothBrush == null) mSmoothBrush = new BTerrainSmoothBrush();
         return mSmoothBrush;
      }
      static public BTerrainVertexBrush getPinchBrush()
      {
         if(mPinchBrush == null) mPinchBrush = new BTerrainPinchBrush();
         return mPinchBrush;
      }
      static public BTerrainVertexBrush getSetHeightBrush()
      {
         if (mSetHeightBrush == null) mSetHeightBrush = new BTerrainSetHeightBrush();
         return mSetHeightBrush;
      }
      static public BTerrainVertexBrush getUniformBrush()
      {
         if (mUniformBrush == null) mUniformBrush = new BTerrainUniformBrush();
         return mUniformBrush;
      }
      static public BTerrainVertexBrush getScalarBrush()
      {
         if (mScalarBrush == null) mScalarBrush = new BTerrainScalarBrush();
         return mScalarBrush;
         
      }
      static public BTerrainAlphaBrush getAlphaBrush()
      {
         if (mAlphaBrush == null) mAlphaBrush = new BTerrainAlphaBrush();
         return mAlphaBrush;
      }
      static public BTerrainVertexBrush getSkirtHeightBrush()
      {
         if (mSkirtHeightBrush == null) mSkirtHeightBrush = new BTerrainSkirtHeightBrush();
         return mSkirtHeightBrush;
	  }

      static public BTerrainTesselationBrush getTesselationBrush()
      {
         if (mTesselationOverrideBrush == null) mTesselationOverrideBrush = new BTerrainTesselationBrush();
         return mTesselationOverrideBrush;
      }

      //Texturing Brushes
      static public BTerrainPaintBrush getSplatBrush()
      {
         if (mSplatBrush == null) mSplatBrush = new BTerrainSplatBrush();
         return mSplatBrush;
      }
      static public BTerrainPaintBrush getDecalBrush()
      {
         if (mDecalBrush == null) mDecalBrush = new BTerrainDecalBrush();
         return mDecalBrush;
      }

      //Sim Brushes
      static public BSimPassibilityBrush getSimPassibilityBrush()
      {
         if (mSimPassibilityBrush == null) mSimPassibilityBrush = new BSimPassibilityBrush();
         return mSimPassibilityBrush;
      }
      static public BSimBuildabilityBrush getSimBuildabilityBrush()
      {
         if (mSimBuildabilityBrush == null) mSimBuildabilityBrush = new BSimBuildabilityBrush();
         return mSimBuildabilityBrush;
      }
      static public BSimFloodPassabilityBrush getSimFloodPassibilityBrush()
      {
         if (mSimFloodPassabilityBrush == null) mSimFloodPassabilityBrush = new BSimFloodPassabilityBrush();
         return mSimFloodPassabilityBrush;
      }
      static public BSimScarabPassabilityBrush getSimScarabPassibilityBrush()
      {
         if (mSimScarabPassabilityBrush == null) mSimScarabPassabilityBrush = new BSimScarabPassabilityBrush();
         return mSimScarabPassabilityBrush;
      }
      static public BSimTileTypeBrush getSimTileTypeBrush()
      {
         if (mSimTileTypeBrush == null) mSimTileTypeBrush = new BSimTileTypeBrush();
         return mSimTileTypeBrush;
      }

      static public BSimHeightBrush getSimHeightOverrideBrush()
      {
         if (mSimHeightBrush == null) mSimHeightBrush = new BSimHeightBrush();
         return mSimHeightBrush;
      }
      static public BSimSetHeightBrush getSimSetHeightOverrideBrush()
      {
         if (mSimSetHeightBrush == null) mSimSetHeightBrush = new BSimSetHeightBrush();
         return mSimSetHeightBrush;
      }
      static public BSimSmoothBrush getSimSmoothOverrideBrush()
      {
         if (mSimSmoothBrush == null) mSimSmoothBrush = new BSimSmoothBrush();
         return mSimSmoothBrush;
      }
      public static BSimHeightEraseBrush getSimEraseHeightOverrideBrush()
      {
         if (mSimHeightEraseBrush == null) mSimHeightEraseBrush = new BSimHeightEraseBrush();
         return mSimHeightEraseBrush;

      }

      public static BTerrainFoliageBrush getFoliageBrush()
      {
         if (mFoliageBrush == null) mFoliageBrush = new BTerrainFoliageBrush();
         return mFoliageBrush;

      }

      public static BTerrainCameraHeightBrush getCameraHeightBrush()
      {
         if (mCameraHeightBrush == null) mCameraHeightBrush = new BTerrainCameraHeightBrush();
         return mCameraHeightBrush;

      }
      public static BTerrainCameraSetHeightBrush getCameraSetHeightBrush()
      {
         if (mCameraSetHeightBrush == null) mCameraSetHeightBrush = new BTerrainCameraSetHeightBrush();
         return mCameraSetHeightBrush;

      }
      public static BTerrainCameraSmoothBrush getCameraSmoothBrush()
      {
         if (mCameraSmoothBrush == null) mCameraSmoothBrush = new BTerrainCameraSmoothBrush();
         return mCameraSmoothBrush;
      }
      public static BTerrainCameraEraseOverrideBrush getCameraEraseBrush()
      {
         if (mCameraEraseOverrideBrush == null) mCameraEraseOverrideBrush = new BTerrainCameraEraseOverrideBrush();
         return mCameraEraseOverrideBrush;
      }


      public static BTerrainFlightHeightBrush getFlightHeightBrush()
      {
         if (mFlightHeightBrush == null) mFlightHeightBrush = new BTerrainFlightHeightBrush();
         return mFlightHeightBrush;

      }
      public static BTerrainFlightSetHeightBrush getFlightSetHeightBrush()
      {
         if (mFlightSetHeightBrush == null) mFlightSetHeightBrush = new BTerrainFlightSetHeightBrush();
         return mFlightSetHeightBrush;

      }
      public static BTerrainFlightSmoothBrush getFlightSmoothBrush()
      {
         if (mFlightSmoothBrush == null) mFlightSmoothBrush = new BTerrainFlightSmoothBrush();
         return mFlightSmoothBrush;
      }
      public static BTerrainFlightEraseOverrideBrush getFlightEraseBrush()
      {
         if (mFlightEraseOverrideBrush == null) mFlightEraseOverrideBrush = new BTerrainFlightEraseOverrideBrush();
         return mFlightEraseOverrideBrush;
      }

      static private BTerrainHeightBrush     mHeightBrush;
      static private BTerrainPushBrush       mPushBrush;
      static private BTerrainAvgHeightBrush  mAvgHeightBrush;
      static private BTerrainStdBrush        mStdBrush; 
      static private BTerrainLayerBrush      mLayerBrush;
      static private BTerrainInflateBrush    mInflateBrush;
      static private BTerrainSmoothBrush     mSmoothBrush;
      static private BTerrainPinchBrush      mPinchBrush;
      static private BTerrainSetHeightBrush  mSetHeightBrush;
      static private BTerrainUniformBrush    mUniformBrush;
      static private BTerrainScalarBrush     mScalarBrush;
      static private BTerrainAlphaBrush      mAlphaBrush;
      static private BTerrainSkirtHeightBrush mSkirtHeightBrush;

      static private BTerrainTesselationBrush mTesselationOverrideBrush;


      static private BSimTileTypeBrush       mSimTileTypeBrush;
      static private BSimFloodPassabilityBrush mSimFloodPassabilityBrush;
      static private BSimScarabPassabilityBrush mSimScarabPassabilityBrush;
      static private BSimBuildabilityBrush mSimBuildabilityBrush;
      static private BSimPassibilityBrush mSimPassibilityBrush;
      static private BSimHeightBrush         mSimHeightBrush;
      static private BSimSetHeightBrush      mSimSetHeightBrush;
      static private BSimSmoothBrush         mSimSmoothBrush;
      static private BSimHeightEraseBrush    mSimHeightEraseBrush;

      static private BTerrainSplatBrush      mSplatBrush;
      static private BTerrainDecalBrush      mDecalBrush;

      static private BTerrainFoliageBrush    mFoliageBrush;

      static private BTerrainCameraHeightBrush     mCameraHeightBrush;
      static private BTerrainCameraSetHeightBrush     mCameraSetHeightBrush;
      static private BTerrainCameraSmoothBrush     mCameraSmoothBrush;
      static private BTerrainCameraEraseOverrideBrush mCameraEraseOverrideBrush;


      static private BTerrainFlightHeightBrush mFlightHeightBrush;
      static private BTerrainFlightSetHeightBrush mFlightSetHeightBrush;
      static private BTerrainFlightSmoothBrush mFlightSmoothBrush;
      static private BTerrainFlightEraseOverrideBrush mFlightEraseOverrideBrush;
      
   }
}
