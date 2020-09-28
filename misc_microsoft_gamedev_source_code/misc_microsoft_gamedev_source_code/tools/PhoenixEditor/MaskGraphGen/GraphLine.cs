using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
//using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace graphapp
{

   public class ConnectionLine
   {
      protected Point mStartPoint = new Point(10, 10);
      protected Point mEndPoint = new Point(50, 50);

      protected Color mLineColor = Color.Black;
      protected int mLineWidth = 2;

      public virtual void render(Graphics g)
      {
         Pen myPen = new Pen(mLineColor, mLineWidth);

         //we CAN be 5 segments

         //draw first segment
         g.DrawLine(myPen, mStartPoint, mEndPoint);
      }

      public virtual void setStartPoint(int x, int y)
      {
         mStartPoint.X = x;
         mStartPoint.Y = y;
      }
      public virtual void setEndPoint(int x, int y)
      {
         mEndPoint.X = x;
         mEndPoint.Y = y;
      }

   }

   public class AngledConnectionLine : ConnectionLine
   {
     

      public override void render(Graphics g)
      {
         Pen myPen = new Pen(mLineColor, mLineWidth);

         int minX = Math.Min(mStartPoint.X, mEndPoint.X);
         int minY = Math.Min(mStartPoint.Y, mEndPoint.Y);
         int maxX = Math.Max(mStartPoint.X, mEndPoint.X);
         int maxY = Math.Max(mStartPoint.Y, mEndPoint.Y);

         int midX = (maxX + minX) >> 1;
         int midY = (maxY + minY) >> 1;

         int deltaX = maxX - minX;
         int deltaY = maxY - minY;

         minX -= midX;
         minY -= midY;
         maxX -= midX;
         maxY -= midY;

         if (deltaX > deltaY)    //wide box
         {
            //create a line at a 45 degree angle through the middle

            if (mEndPoint.Y > mStartPoint.Y)
            {
               if (mEndPoint.X < mStartPoint.X)
               {
                  Point botlft = new Point(midX - minY, midY + minY);
                  Point toprht = new Point(midX + minY, midY - minY);

                  g.DrawLine(myPen, botlft, toprht);

                  g.DrawLine(myPen, botlft, mStartPoint);
                  g.DrawLine(myPen, toprht, mEndPoint);
               }
               else
               {
                  Point botlft = new Point(midX - minY, midY - minY);
                  Point toprht = new Point(midX + minY, midY + minY);

                  g.DrawLine(myPen, botlft, toprht);

                  g.DrawLine(myPen, toprht, mStartPoint);
                  g.DrawLine(myPen, botlft, mEndPoint);
               }
              
            }
            else //if (mEndPoint.Y > mStartPoint.Y)
            {
               if (mEndPoint.X < mStartPoint.X)
               {
                  Point toprht = new Point(midX + minY, midY + minY);
                  Point botlft = new Point(midX - minY, midY - minY);

                  g.DrawLine(myPen, botlft, toprht);

                  g.DrawLine(myPen, botlft, mStartPoint);
                  g.DrawLine(myPen, toprht, mEndPoint);
               }
               else
               {
                  Point toprht = new Point(midX + minY, midY - minY);
                  Point botlft = new Point(midX - minY, midY + minY);

                  g.DrawLine(myPen, botlft, toprht);

                  g.DrawLine(myPen, toprht, mStartPoint);
                  g.DrawLine(myPen, botlft, mEndPoint);
               }
            }

         }
         else
         {
            //tall box

            Point topPt = new Point(midX, midY + maxY - maxX);
            Point botPt = new Point(midX, midY + minY + maxX);
            g.DrawLine(myPen, topPt, botPt);

            if (mEndPoint.Y > mStartPoint.Y)
            {
               g.DrawLine(myPen, topPt, mEndPoint);
               g.DrawLine(myPen, botPt, mStartPoint);
            }
            else
            {
               g.DrawLine(myPen, botPt, mEndPoint);
               g.DrawLine(myPen, topPt, mStartPoint);
            }


         }


      }

      public bool isTouchesRect(int minx, int maxx, int miny, int maxy)
      {
          int minX = Math.Min(mStartPoint.X, mEndPoint.X);
          int minY = Math.Min(mStartPoint.Y, mEndPoint.Y);
          int maxX = Math.Max(mStartPoint.X, mEndPoint.X);
          int maxY = Math.Max(mStartPoint.Y, mEndPoint.Y);

          return (minx < maxX && miny < maxY &&
              minX < maxx && minY < maxy)
           ;

      }

   }
}