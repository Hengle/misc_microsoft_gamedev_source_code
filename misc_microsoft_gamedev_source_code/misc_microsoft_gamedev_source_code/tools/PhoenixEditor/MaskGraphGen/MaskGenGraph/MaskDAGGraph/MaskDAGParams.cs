using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;

using EditorCore;
using NoiseGeneration;
namespace graphapp
{
    public class FloatParam : ParamType
    {
        public FloatParam()
            : base(typeof(float), "Scalar")
        {
            mValue = new ClampedFloat(float.MinValue, float.MaxValue,0);
        }

        public FloatParam(float defval)
            : base(typeof(float), "Scalar")
        {
            mValue = new ClampedFloat(float.MinValue, float.MaxValue,defval);
        }

        public FloatParam(float defval, float min, float max)
            : base(typeof(float), "Scalar")
        {
            mValue = new ClampedFloat(min, max,defval);
        }

        ClampedFloat mValue;
        public float Value
        {
            get { return mValue.Value; }
            set { mValue.Value = value; }
        }
    };

    public class IntParam : ParamType
    {
        public IntParam()
            : base(typeof(int), "Integer")
        {
            mValue = new ClampedInt(int.MinValue, int.MaxValue,0);
            mValue.Value = 0;
        }

        public IntParam(int defval)
            : base(typeof(int), "Integer")
        {
           mValue = new ClampedInt(int.MinValue, int.MaxValue, defval);
        }

        public IntParam(int defval, int min, int max)
            : base(typeof(int), "Integer")
        {
           mValue = new ClampedInt(min, max, defval);
        }

        ClampedInt mValue;
        public int Value
        {
            get { return mValue.Value; }
            set { mValue.Value = value; }
        }
    };

    public class BoolParam : ParamType
    {
        public BoolParam()
            : base(typeof(bool),"Boolean")
        {
        }
        public BoolParam(bool defVal)
            : base(typeof(bool), "Boolean")
        {
            mValue = defVal;
        }

        bool mValue;
        public bool Value
        {
            get { return mValue; }
            set { mValue = value; }
        }
    };


    public class MaskParam : ParamType
    {
        public MaskParam()
            : base(typeof(DAGMask),"Mask")
        {
        }
       DAGMask mValue;
       public DAGMask Value
        {
            get { return mValue; }
            set { mValue = value; }
        }
    };






}