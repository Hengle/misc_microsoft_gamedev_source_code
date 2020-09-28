using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using System.Drawing;
using System.Text.RegularExpressions;
using System.Reflection;

namespace EditorCore
{
   public class TextVectorHelper
   {
      static public Vector3 FromString(string s)
      {
         string[] values = s.Split(',');
         return new Vector3(System.Convert.ToSingle(values[0]), System.Convert.ToSingle(values[1]), System.Convert.ToSingle(values[2]));
      }
      static public string ToString(Vector3 v)
      {
         return String.Format("{0:F4},{1:F4},{2:F4}", v.X, v.Y, v.Z);
      }
      static public Vector2 FromStringV2(string s)
      {
         string[] values = s.Split(',');
         return new Vector2(System.Convert.ToSingle(values[0]), System.Convert.ToSingle(values[1]));
      }
      static public string ToStringV2(Vector2 v)
      {
         return String.Format("{0:F4},{1:F4}", v.X, v.Y);
      }
      static public bool IsValidVector3(string s)
      {
         Match m = sVector3Format.Match(s);
         return m.Success;
      }
      static Regex sVector3Format = new Regex(@"[-+]?([0-9]*\.[0-9]+|[0-9]+),[-+]?([0-9]*\.[0-9]+|[0-9]+),[-+]?([0-9]*\.[0-9]+|[0-9]+)");
   }

   public class TextColorHelper
   {
      static public Color FromString(string s)
      {
         // If in non-standard format straight from Color object
         if( s.Contains( "Color" ) )
         {            
            int indexStart = s.IndexOf( "R=" );
            s = s.Remove( 0, indexStart );
            indexStart = 2;
            int indexStop = s.IndexOf( "," ) - 1;
            string standard = s.Substring( indexStart, indexStop );

            indexStart = s.IndexOf( "G=" );
            s = s.Remove( 0, indexStart );
            indexStart = 2;
            indexStop = s.IndexOf( "," ) - 1;
            standard += s.Substring( indexStart, indexStop );

            indexStart = s.IndexOf( "B=" );
            s = s.Remove( 0, indexStart );
            indexStart = 2;
            indexStop = s.IndexOf( "]" ) - 2;
            standard += s.Substring( indexStart, indexStop );
            s = standard;
         }

         string[] values = s.Split(',');
         return Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
      }
      static public string ToString(Color c)
      {
         return String.Format("{0},{1},{2}", c.R, c.G, c.B);
      }
   }

   public class StringListHelper
   {
      static public ICollection<string> FromString(string s)
      {
         string[] values = s.Split(',');
         //return Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
         List<string> list = new List<string>();
         list.AddRange(values);
         return list;
      }
      static public string ToString(ICollection<string> list)
      {
         string output = "";
         if (list == null)
            return output;
         foreach(string s in list)
         {

            if (output != "")
               output += ",";
            output += s;
         }
         return output;
         //return String.Format("{0},{1},{2}", c.R, c.G, c.B);
      }

   }


   //public class ObjectStringConverter
   //{
   //   public static bool TryParse<T>(string s, out T value) where T : new()
   //   {
   //      value = new T();
   //      if (s == null || s == "")
   //         return false;
   //      string[] values = s.Split('|');
   //      foreach (string entry in values)
   //      {


   //      }
   //      return true;
   //   }
   //   public static string FormatString<T>(T value) where T : new()
   //   {


   //   }

   //}


   public class StringDecorator
   {
      public string mValue = "";
      public string mDecorator = "";

      public StringDecorator(string value, string decorator)
      {
         mValue = value;
         mDecorator = decorator;
      }
      public static bool IsDecoratorString(string s)
      {
         return sDecoratorFormat.Match(s).Success;
      }
      public override string ToString()
      {
         return String.Format("_{0}_,_{1}_",mValue,mDecorator);
      } 
      public static string ToString(string value, string decorator)
      {
         return String.Format("_{0}_,_{1}_", value, decorator);
      }

      public static bool TryParse(String s, out StringDecorator decorator)
      {
         decorator = null;

         if (s == null)
            return false;

         Match res = sDecoratorFormat.Match(s);
         if(res.Success == false)
         {
            return false;
         }
         decorator = new StringDecorator(res.Groups["value"].Value, res.Groups["decorator"].Value);      
         return true;
      }

      static Regex sDecoratorFormat = new Regex(@"_(?<value>.+)_,_(?<decorator>.+)_");
   }

}
