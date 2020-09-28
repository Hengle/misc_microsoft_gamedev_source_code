
//---------------------------------------
namespace Terrain
{
    #region TerrainMetrics
    public class BTerrainMetrics
    {

        static public void addCPUMem(int val) { mCPUMemCount += val; }
        static public void addPSTex(int mem) { mNumPSTextures++; mGPUMemCount += mem; mPSTexMem += mem; }
        static public void addVSTex(int mem) { mNumVSTextures++; mGPUMemCount += mem; mVSTexMem += mem; }
        static public void addQuadNode() { mNumQN++; }

        static public void clearCounters()
        {
            mGPUMemCount = 0;
            mCPUMemCount = 0;
            mNumPSTextures = 0;
            mNumVSTextures = 0;
        }
        static public void access()
        {
            int memPerTex = mGPUMemCount / (mNumPSTextures + mNumVSTextures);
        }

        static public int mGPUMemCount = 0;
        static public int mCPUMemCount = 0;
        static public int mNumPSTextures = 0;
        static public int mPSTexMem = 0;
        static public int mNumVSTextures = 0;
        static public int mVSTexMem = 0;
        static public int mNumQN = 0;

        static public void addGPUMem(int val) { mGPUMemCount += val; }
    };

    #endregion

}