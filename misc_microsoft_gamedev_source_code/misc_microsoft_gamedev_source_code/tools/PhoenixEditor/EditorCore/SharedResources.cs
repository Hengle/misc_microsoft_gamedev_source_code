using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.IO;
using System.Windows.Forms;

namespace EditorCore
{
   public class SharedResources
   {
      private SharedResources()
      {
         Bitmap b = new Bitmap(1, 1);
         b.SetPixel(0, 0, Color.Red);
         mBlank = b;// mImages["Blank"] = b;       
      }
      static SharedResources Instance = new SharedResources();
      Dictionary<string, Image> mImages = new Dictionary<string, Image>();
      Image mBlank = null;

      static public Image GetImage(string name)
      {
         try
         {
            if (!Instance.mImages.ContainsKey(name))
            {
               string file = Instance.FindResource(name);
               if (file != "")
               {
                  Instance.mImages[name] = Image.FromFile(file);
               }
               else
               {
                  Instance.mImages[name] = Instance.mBlank;
               }
            }
         }
         catch
         {
            Instance.mImages[name] = Instance.mBlank;
         }                 
         return Instance.mImages[name];         
      }


      string FindResource(string name)
      {
         string found = "";
         if (File.Exists(name) == true)
         {
            return name;
         }

         string basedir = AppDomain.CurrentDomain.SetupInformation.ApplicationBase;

         string tosearch = Path.Combine(basedir, name);
         if(File.Exists(tosearch))
         {
            return tosearch;
         }

         tosearch = Path.Combine(basedir + "Icons\\", name);
         if (File.Exists(tosearch))
         {
            return tosearch;
         }
         return "";
      }

      static Dictionary<string[], ImageList> mImageLists = new Dictionary<string[], ImageList>();

      static public ImageList GetImageList(string[] names)
      {
         if (mImageLists.ContainsKey(names) == false)
         {
            ImageList list = new ImageList();
            foreach (string file in names)
            {
               list.Images.Add(SharedResources.GetImage(file));
            }
            mImageLists[names] = list;
         }
         return mImageLists[names];
      }


   }



}
