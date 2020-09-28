using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace ScreenShotDecoder
{
   public partial class Form1 : Form
   {
      public Form1()
      {
         InitializeComponent();
      }

      private void DecodeFromHex_Click(object sender, EventArgs e)
      {
         //Make sure the two hex inputs are filled out
         string hexValue1 = HexCode1.Text;
         if (hexValue1.Length != 9)
         {
            MessageBox.Show("First Hex code is not 9 characters long");            
            return;
         }
         for (int i=0;i<9;i++)
         {
            if (!Uri.IsHexDigit(hexValue1[i]))             
            {
               MessageBox.Show("First Hex code is not valid");
               return;
            }
         }

         string hexValue2 = HexCode2.Text;
         if (hexValue2.Length != 9)
         {
            MessageBox.Show("Second Hex code is not 9 characters long");
            return;
         }
         for (int i=0;i<9;i++)
         {
            if (!Uri.IsHexDigit(hexValue2[i]))             
            {
               MessageBox.Show("Second Hex code is not valid");
               return;
            }
         }

         //Get the cypher
         byte[] cypher = new byte[8];
         cypher[0] = (byte)Uri.FromHex(hexValue1[0]);
         cypher[1] = (byte)Uri.FromHex(hexValue2[0]);
         cypher[2] = (byte)Uri.FromHex(hexValue1[0]);
         cypher[3] = (byte)Uri.FromHex(hexValue2[0]);
         cypher[4] = (byte)Uri.FromHex(hexValue1[0]);
         cypher[5] = (byte)Uri.FromHex(hexValue2[0]);
         cypher[6] = (byte)Uri.FromHex(hexValue1[0]);
         cypher[7] = (byte)Uri.FromHex(hexValue2[0]);

         //Convert input to hex and decode
         hexValue1 = hexValue1.Substring(1, 8);
         hexValue2 = hexValue2.Substring(1, 8);
         byte[] hex1 = new byte[8];
         for (int i=0;i<8;i++)
         {
            hex1[i] = (byte)((byte)Uri.FromHex(hexValue1[i]) ^ cypher[i]);
         }
         byte[] hex2 = new byte[8];
         for (int i=0;i<8;i++)
         {
            hex2[i] = (byte)((byte)Uri.FromHex(hexValue2[i]) ^ cypher[i]);
         }
         
         //Combine and write out result
         string result = "";
         for (int i=0;i<8;i++)
         {
            if (hex1[i]>9)
            {
               result = result + (char)(55+hex1[i]);
            }
            else
            {
               result = result + hex1[i].ToString(); ;
            }
         }  
         for (int i=0;i<8;i++)
         {
            if (hex2[i] > 9)
            {
               result = result + (char)(55 + hex2[i]);
            }
            else
            {
               result = result + hex2[i].ToString();
            }
         }
         XUIDResult1.Text = result;
         ulong uh1 = Convert.ToUInt64(result, 16 );
         XUIDResult2.Text = uh1.ToString();

      }

      private void DecodeFromImage_Click(object sender, EventArgs e)
      {
         UInt32 xuid = 0;
         if (XUIDCheckInner01.Checked)
            xuid += (uint)Math.Pow(2, 0);
         if (XUIDCheckInner02.Checked)
            xuid += (uint)Math.Pow(2, 1);
         if (XUIDCheckInner03.Checked)
            xuid += (uint)Math.Pow(2, 2);
         if (XUIDCheckInner04.Checked)
            xuid += (uint)Math.Pow(2, 3);
         if (XUIDCheckInner05.Checked)
            xuid += (uint)Math.Pow(2, 4);
         if (XUIDCheckInner06.Checked)
            xuid += (uint)Math.Pow(2, 5);
         if (XUIDCheckInner07.Checked)
            xuid += (uint)Math.Pow(2, 6);
         if (XUIDCheckInner08.Checked)
            xuid += (uint)Math.Pow(2, 7);
         if (XUIDCheckInner09.Checked)
            xuid += (uint)Math.Pow(2, 8);
         if (XUIDCheckInner10.Checked)
            xuid += (uint)Math.Pow(2, 9);
         if (XUIDCheckInner11.Checked)
            xuid += (uint)Math.Pow(2, 10);
         if (XUIDCheckInner12.Checked)
            xuid += (uint)Math.Pow(2, 11);

         if (XUIDCheckOuter01.Checked)
            xuid += (uint)Math.Pow(2, 12);
         if (XUIDCheckOuter02.Checked)
            xuid += (uint)Math.Pow(2, 13);
         if (XUIDCheckOuter03.Checked)
            xuid += (uint)Math.Pow(2, 14);
         if (XUIDCheckOuter04.Checked)
            xuid += (uint)Math.Pow(2, 15);
         if (XUIDCheckOuter05.Checked)
            xuid += (uint)Math.Pow(2, 16);
         if (XUIDCheckOuter06.Checked)
            xuid += (uint)Math.Pow(2, 17);
         if (XUIDCheckOuter07.Checked)
            xuid += (uint)Math.Pow(2, 18);
         if (XUIDCheckOuter08.Checked)
            xuid += (uint)Math.Pow(2, 19);
         if (XUIDCheckOuter09.Checked)
            xuid += (uint)Math.Pow(2, 20);
         if (XUIDCheckOuter10.Checked)
            xuid += (uint)Math.Pow(2, 21);
         if (XUIDCheckOuter11.Checked)
            xuid += (uint)Math.Pow(2, 22);
         if (XUIDCheckOuter12.Checked)
            xuid += (uint)Math.Pow(2, 23);

         XUIDResult2.Text = "(Lower 12 bits)" + xuid.ToString();
         XUIDResult1.Text = Convert.ToString(xuid, 16);
         XUIDResult1.Text = XUIDResult1.Text.ToUpper();
         XUIDResult1.Text = XUIDResult1.Text.PadLeft(6, '0');
         XUIDResult1.Text = "xxxxxxxxxx" + XUIDResult1.Text;
      }

      private void ClearBits_Click(object sender, EventArgs e)
      {
         XUIDCheckInner01.Checked = false;
         XUIDCheckInner02.Checked = false;
         XUIDCheckInner03.Checked = false;
         XUIDCheckInner04.Checked = false;
         XUIDCheckInner05.Checked = false;
         XUIDCheckInner06.Checked = false;
         XUIDCheckInner07.Checked = false;
         XUIDCheckInner08.Checked = false;
         XUIDCheckInner09.Checked = false;
         XUIDCheckInner10.Checked = false;
         XUIDCheckInner11.Checked = false;
         XUIDCheckInner12.Checked = false;
         XUIDCheckOuter01.Checked = false;
         XUIDCheckOuter02.Checked = false;
         XUIDCheckOuter03.Checked = false;
         XUIDCheckOuter04.Checked = false;
         XUIDCheckOuter05.Checked = false;
         XUIDCheckOuter06.Checked = false;
         XUIDCheckOuter07.Checked = false;
         XUIDCheckOuter08.Checked = false;
         XUIDCheckOuter09.Checked = false;
         XUIDCheckOuter10.Checked = false;
         XUIDCheckOuter11.Checked = false;
         XUIDCheckOuter12.Checked = false;
      }

   }
}