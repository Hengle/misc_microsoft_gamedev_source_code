using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;

namespace graphapp
{
    public class OutputGenerationParams
    {
        int mWidth=256;
        public int Width
        {
            get{return mWidth;}
            set{mWidth = value;}
        }

        int mHeight = 256;
        public int Height
        {
            get { return mHeight; }
            set { mHeight = value; }
        }

    }
}
