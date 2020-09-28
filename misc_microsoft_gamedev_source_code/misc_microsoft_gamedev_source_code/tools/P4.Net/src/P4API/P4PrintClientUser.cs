/*
 * P4.Net *
Copyright (c) 2007 Shawn Hladky

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
using System.Collections;
using System.IO;
using System.Text;

namespace P4API
{
    internal class P4PrintClientUser : p4dn.ClientUser
    {
        private Stream _stream = null;
        private P4Connection _p4 = null;
        private OnPrintStreamEventHandler _printEvent = null;
        private OnPrintEndEventHandler _printEndEvent = null;
        //private bool _isPrinting;
        private P4PrintStreamEventArgs _args;

        internal P4PrintClientUser(P4Connection p4connection, OnPrintStreamEventHandler OnPrintEvent, OnPrintEndEventHandler OnPrintEnd)
        {
            _p4 = p4connection;
            _printEvent = OnPrintEvent;
            _printEndEvent = OnPrintEnd;
        }

        public override void Finished()
        {
            if (_stream != null)
            {
                _stream = null;
            }
        }
        
        public override void Message(p4dn.Error err)
        {
            // if we get any message, it's an error??
        }
        
        // we always use OutputBinary (easier to deal with encoding in managed code).
        public override void OutputBinary(byte[] b)
        {
            if (b.Length > 0)
            {
                if (_stream != null)
                {
                    if (_stream.CanWrite)
                    {
                        byte[] e = encodeIfText(b);
                        _stream.Write(e, 0, e.Length);
                    }
                }
            }
            else
            {
                RaiseEndEvent();
            }
            
        }
        
        public override void OutputStat(Hashtable varList)
        {

            OnPrintStreamEventHandler handler = _printEvent;

            // Event will be null if there are no subscribers
            if (handler != null)
            {
                string depotFile = (string)varList[(object)"depotFile"];
                string action = (string)varList[(object)"action"];
                string fileType = (string)varList[(object)"type"];
                DateTime changeDate = _p4.ConvertDate((string)varList[(object)"time"]);
                int change = int.Parse((string)varList[(object)"change"]);

                // get the information about the file
                _args = new P4PrintStreamEventArgs(depotFile, action, fileType, changeDate, change);
                handler(_args, out _stream);
            }
        }

        // this is a dumb little callback to force OutputText to redirect to OutputBinary
        public override bool PrintBinary()
        {
            return true;
        }

        private byte[] encodeIfText(byte[] b)
        {
            if (_args != null)
            {
                if (_args.FileType.IndexOf("text") != -1)
                {
                    if (_args.TextEncoding != Encoding.GetEncoding(1252))
                    {
                        return Encoding.Convert(Encoding.GetEncoding(1252), _args.TextEncoding, b);
                    }
                }
                else if (_args.FileType.IndexOf("unicode") != -1)
                {
                    if (_args.UnicodeEncoding != Encoding.UTF8)
                    {
                        return Encoding.Convert(Encoding.UTF8, _args.UnicodeEncoding, b);
                    }
                }
            }
            return b ;
        }

        private void RaiseEndEvent()
        {
            OnPrintEndEventHandler handler = _printEndEvent;
            // Event will be null if there are no subscribers
            if (handler != null)
            {
                handler(_args, _stream);
            }
        }
        
    }
}
