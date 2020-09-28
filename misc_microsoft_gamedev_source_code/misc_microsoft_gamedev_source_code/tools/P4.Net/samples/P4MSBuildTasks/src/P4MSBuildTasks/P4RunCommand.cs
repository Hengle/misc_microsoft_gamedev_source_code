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
    public class P4RunCommand : P4BaseTask
    {

        private List<string> _errors = new List<string>();
        private List<string> _warnings = new List<string>();
        private List<string> _messages = new List<string>();

        private string _command = null;
        private string[] _args = new string[0];

        private List<TaskItem> _parsedOutput = new List<TaskItem>();

        [Required]
        public string Command
        {
            get { return _command; }
            set { _command = value; }
        }
        [Required]
        public string Arguments
        {
            get
            {
                return string.Join(";", _args);
            }
            set
            {
                List<string> args = new List<string>();
                foreach (string s in value.Split(';'))
                {
                    if (s.Trim() != string.Empty) args.Add(s);
                }
                _args = args.ToArray();
            }
        }
        [Output]
        public ITaskItem[] ParsedOutput
        {
            get { return _parsedOutput.ToArray();}

        }
        [Output]
        public string[] Errors
        {
            get { return _errors.ToArray(); }

        }
        [Output]
        public string[] Warnings
        {
            get { return _warnings.ToArray(); }

        }
        [Output]
        public string[] Messages
        {
            get { return _messages.ToArray(); }

        }

        public override void P4Execute()
        {
            try
            {

                P4RecordSet rs = _p4.Run(_command, _args);
                
                _errors.AddRange(rs.Errors);
                _warnings.AddRange(rs.Warnings);
                _messages.AddRange(rs.Messages);

                foreach (string m in _errors)
                {
                    Log.LogError(m);
                }

                int i = 0;
                foreach (P4Record r in rs)
                {
                    string itemspec = string.Format("P4Output-{0}-{1:D8}", _command, i);
                    TaskItem item = new TaskItem(itemspec);
                    LogMessage(MessageImportance.High,"{0}",itemspec);
                    foreach (string key in r.Fields.Keys)
                    {
                        LogMessage(MessageImportance.Normal, "  {0,-10} : {1}", key, r[key]);
                        item.SetMetadata(key, r[key]);
                    }
                    
                    //  array fields will be populated as:
                    //  key  : arr-<keyname>
                    //  value: ';' delimited list
                    foreach (string key in r.ArrayFields.Keys)
                    {
                        string newKey = string.Format("arr-{0}", key);
                        string newVal = string.Empty;
                        foreach(string val in r.ArrayFields[key])
                        {
                            if (newVal != string.Empty) newVal += ";";
                            newVal += val;
                        }
                        LogMessage(MessageImportance.Normal, "  {0,-10} : {1}", newKey, newVal);
                        item.SetMetadata(newKey, newVal);
                    }
                    _parsedOutput.Add(item);
                    i++;
                }

            }
            catch (Exception e)
            {
                Log.LogError(e.Message); 
            }
        }

    }
}
