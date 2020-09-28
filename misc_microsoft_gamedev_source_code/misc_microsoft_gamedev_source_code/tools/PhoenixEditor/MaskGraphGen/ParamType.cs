using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;

namespace graphapp
{
    public class ParamType
    {
        public ParamType(Type T, string description)
        {
            mType = T;
            mDescriptor = description;
        }

        string mDescriptor;
        public string Descriptor
        {
            get { return mDescriptor; }
        }

        Type mType;
        public Type Type
        {
            get { return mType; }
        }
        

    }
}