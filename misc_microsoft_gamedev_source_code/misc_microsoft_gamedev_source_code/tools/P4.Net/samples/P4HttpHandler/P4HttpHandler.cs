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
using System.Configuration;
using System.Web;
using System.Web.Security;
using Microsoft.Win32;
using System.IO;
using P4API;
using System.Text;
using System.Collections.Generic;

namespace P4HttpHandler
{
    public class P4HttpHandler: IHttpHandler
    {
        private static Dictionary<string, string> _mimeTypeCache = new Dictionary<string, string>();

        public bool IsReusable
        {
            get { return true; }
        }

        public void ProcessRequest(HttpContext context)
        {

            // Don't allow this response to be cached by the browser.
            context.Response.Cache.SetCacheability(HttpCacheability.NoCache);
            context.Response.Cache.SetNoStore();
            context.Response.Cache.SetExpires(DateTime.MinValue);

            // Create a new p4 connection, and set the appropriate properties
            P4Connection p4 = new P4Connection();
            AppSettingsReader appSettings = new AppSettingsReader();
            p4.Port =     (string) appSettings.GetValue("P4PORT", typeof(string));
            p4.User =     (string) appSettings.GetValue("P4USER", typeof(string));
            p4.Client =   (string) appSettings.GetValue("P4CLIENT", typeof(string));
            p4.Password = (string) appSettings.GetValue("P4PASSWD", typeof(string));

            try
            {
                p4.Connect();

                //Figure out the clientPath for the file
                string clientPath = string.Format("//{0}/{1}", p4.Client, context.Request.AppRelativeCurrentExecutionFilePath.Substring(2));

                if (!clientPath.EndsWith("/"))
                {
                    // We have a path to a file

                    // find the MIME type and set it
                    string ext = Path.GetExtension(clientPath);
                    string mimeType = getMimeType(ext);
                    context.Response.ContentType = mimeType;

                    //stream the results ... will throw an exception if the path isn't found
                    try
                    {
                        p4.PrintStream(context.Response.OutputStream, clientPath);
                        context.Response.OutputStream.Flush();
                    }
                    catch (P4API.Exceptions.FileNotFound)
                    {
                        context.Response.StatusCode = 404;
                    }
                }
                else
                {

                    // we have a directory... let's look for a default "index" file and redirect

                    // My Rule for a default page is: 
                    // :: "index.htm" or "index.html" in the current directory (case insensitive)
                    //
                    // I don't rely on the Perforce server to be case insensitive, so I will run an fstat for 
                    // all files in the directory and see if there are any "index.htm*" files
                    P4RecordSet rs = p4.Run("fstat", "-Op", clientPath + "*");
                    foreach (P4Record r in rs)
                    {
                        if (r["depotFile"].ToLower().EndsWith("index.html") || r["depotFile"].ToLower().EndsWith("index.htm"))
                        {
                            // the -Op switch means client file will be //<clientname>/<clientpath>
                            clientPath = r["clientFile"];
                            break;
                        }
                    }
                    if (clientPath.EndsWith("/"))
                    {
                        // clientPath not updated, means we can't find a default page

                        // For now, just 404... in the future we could allow directory browsing 
                        // (which we be a lot bigger than a sample application ;-)
                        context.Response.StatusCode = 404;
                    }
                    else
                    {
                        // redirect to the index page
                        string redirect = "~" + clientPath.Substring(p4.Client.Length + 2);
                        context.Response.Redirect(redirect, false);
                    }                    
                }
            }
            catch (Exception e)
            {
                // unhandled exception... send a 500 to the browser
                System.Diagnostics.Trace.WriteLine(e.StackTrace);
                context.Response.StatusCode = 500;
            }
            finally
            {
                p4.Disconnect();
                context.Response.End();
            }
        }

        /// <summary>
        /// Get's the mime type from the registry.  This is a DUMB way to do it... why isn't there an API?
        /// </summary>
        /// <param name="sExtension"></param>
        /// <returns></returns>
        private static string getMimeType(string sExtension)
        {
            string extension = sExtension.ToLower();

            if (_mimeTypeCache.ContainsKey(extension))
            {
                return _mimeTypeCache[extension];
            }

            RegistryKey key = Registry.ClassesRoot.OpenSubKey(sExtension);
            if (key != null)
            {
                string mimetype = key.GetValue("Content Type") as string;
                if (mimetype != null)
                {
                    lock (_mimeTypeCache)
                    {
                        _mimeTypeCache.Add(extension, mimetype);
                    }
                    return mimetype;
                }
            }
            return "application/unknown";

        }

        #region debugHelper
        private void debugMe(HttpContext context)
        {
            string html = @"
<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 3.2 Final//EN'>
<HTML><HEAD><TITLE>Test</TITLE></HEAD>
<BODY BGCOLOR='#FFFFFF' TEXT='#000000' LINK='#FF0000' VLINK='#800000' ALINK='#FF00FF' BACKGROUND='?'>
<BR>ApplicationPath: {0}</BR>
<BR>Path: {1}</BR>
<BR>FilePath: {2}</BR>
<BR>ContentType: {3}</BR>
<BR>AppRelativeCurrentExecutionFilePath: {4}</BR>
<BR>Browser: {5}</BR>
<BR>PathInfo: {6}</BR>
<BR>QueryString: {7}</BR>
<BR>RawUrl: {8}</BR>
<BR>Url: {9}</BR>
<BR>Extension: {10}</BR>
<BR>My Mime: {11}</BR>
</BODY></HTML>
";
            string ext = Path.GetExtension(context.Request.AppRelativeCurrentExecutionFilePath);
            string mimeType = getMimeType(ext);
            string sHtml = string.Format(html,
            context.Request.ApplicationPath,
            context.Request.Path,
            context.Request.FilePath,
            context.Request.ContentType,
            context.Request.AppRelativeCurrentExecutionFilePath,
            context.Request.Browser,
            context.Request.PathInfo,
            context.Request.QueryString,
            context.Request.RawUrl,
            context.Request.Url,
            ext, mimeType
            );
            context.Response.Write(sHtml);
        }

        #endregion
    }
}
