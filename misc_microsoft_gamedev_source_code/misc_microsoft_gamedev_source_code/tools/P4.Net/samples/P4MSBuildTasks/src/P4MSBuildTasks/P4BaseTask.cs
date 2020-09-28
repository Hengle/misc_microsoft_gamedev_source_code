/*
 * P4.Net *
Copyright (c) 2006 Shawn Hladky

Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without 
restriction, including without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */


using System;
using System.Collections.Generic;
using System.Xml;
using System.Text;
using System.Text.RegularExpressions;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using Microsoft.Build.BuildEngine;
using System.IO;
using P4API;

namespace P4.Net.P4MSBuildTasks
{
    public abstract class P4BaseTask : Task
    {
        private string _p4Client = null;
        private string _p4CWD = null;
        private string _p4Host = null;
        private string _p4Port = null;
        private string _p4User = null;
        private string _p4Passwd = null;
        private bool _verbose = false;
        private bool _useLogin = false;

        protected P4Connection _p4 = null;

        public string Client
        {
            get { return _p4Client; }
            set { _p4Client = value; }
        }
        public string CWD
        {
            get { return _p4CWD; }
            set { _p4CWD = value; }
        }
        public string Host
        {
            get { return _p4Host; }
            set { _p4Host = value; }
        }
        public string Port
        {
            get { return _p4Port; }
            set { _p4Port = value; }
        }
        public string User
        {
            get { return _p4User; }
            set { _p4User = value; }
        }
        public string Password
        {
            get { return _p4Passwd; }
            set { _p4Passwd = value; }
        }
        public bool Verbose
        {
            get { return _verbose; }
            set { _verbose = value; }
        }
        public bool UseLogin
        {
            get { return _useLogin; }
            set { _useLogin = value; }

        }



        public override sealed bool Execute()
        {
            try
            {
                InitP4();
                P4Execute();
            }
            finally
            {
                DisconnectP4();
            }
            return !Log.HasLoggedErrors;
        }

        public abstract void P4Execute();

        protected void InitP4()
        {
            _p4 = new P4Connection();
            if (_p4Port != null) _p4.Port = _p4Port;
            if (_p4Client != null) _p4.Client = _p4Client;
            if (_p4CWD != null) _p4.CWD = _p4CWD;
            if (_p4Host != null) _p4.Host = _p4Host;
            if (_p4User != null) _p4.User = _p4User;
            if (_p4Passwd != null) _p4.Password = _p4Passwd;

            _p4.ExceptionLevel = P4ExceptionLevels.NoExceptionOnErrors;
            _p4.CallingProgram = "P4MSBuildTasks";
            _p4.CallingVersion = "1.0";

            _p4.Connect();
            if (_useLogin && _p4Passwd != null)
            {
                _p4.Login(_p4Passwd);
            }
        }

        protected void DisconnectP4()
        {
            _p4.Disconnect();
        }

        protected void LogMessage(MessageImportance importance, string FormatString, params object[] args)
        {
            if(_verbose) Log.LogMessage(importance, FormatString, args);
        }



    }
}
