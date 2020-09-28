using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using System.Globalization;
using Xceed.Grid;

namespace gr2Data2
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            Xceed.Grid.Licenser.LicenseKey = "GRD36-AF1AG-2N6H8-XH5A";
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            // Create a jagged array
            object[][] data = new object[10][] {
                new object[ 4 ] { "Canada" , 31500000 , "Ottawa" , Image.FromFile("c:\\image.bmp") },
                new object[ 4 ] { "Switzerland" , 7300000, "Bern"   , "23.3 ºc"},
                new object[ 4 ] { "France" , 59500000 , "Paris" , "27.3 ºc" } ,
                new object[ 4 ] { "USA" , 278000000 , "Washington" , "14.1 ºc" } ,
                new object[ 4 ] { "UK" , 59700000, "London" , "23.7 ºc" } ,
                new object[ 4 ] { "Belgium" , 10300000, "Brussels" , "21.8 ºc" } ,
                new object[ 4 ] { "Italy" , 57700000 , "Rome" , "29.6 ºc"} ,
                new object[ 4 ] { "Spain" , 40000000 , "Madrid" , "31.8 ºc" } ,
                new object[ 4 ] { "Germany" , 83000000, "Berlin" , "25.1 ºc" } ,
                new object[ 4 ] { "Japan" , 126800000, "Tokyo" , "17.2 ºc" } };

            gridControl1.DataSource = data;

            NumberFormatInfo nfi = new NumberFormatInfo();
            nfi.NumberGroupSeparator = " ";

            gridControl1.Columns[1].FormatProvider = nfi;
            gridControl1.Columns[1].FormatSpecifier = "n0";


            gridControl1.Columns[0].Title = "Country";
            gridControl1.Columns[1].Title = "Population";
            gridControl1.Columns[2].Title = "Capital";
            gridControl1.Columns[3].Title = "Average Temperature";

        }
    }
}