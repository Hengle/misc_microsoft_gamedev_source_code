using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace graphapp
{
    public partial class Device_PropertyDlg : Form
    {
        public Device_PropertyDlg()
        {
            InitializeComponent();
        }
        MaskDAGGraphNode mOwnerNode = null;
        int startingControlCount = 0;
        public void initalize(MaskDAGGraphNode gn)
        {
            this.SuspendLayout();

            startingControlCount = this.Controls.Count;

            int textLeft = 10;

            int objLeft = 100;
            int objHeight = 20;

            this.Width = 500;


            mOwnerNode = gn;
            this.Text = mOwnerNode.GetType().ToString() +  " Properties";

            Type type = mOwnerNode.GetType();// Get object type
          System.Reflection.PropertyInfo[] pi = type.GetProperties();
          for (int i = 0; i < pi.Length; i++)
          {
              System.Reflection.PropertyInfo prop = pi[i];

              object[] custAttrib = prop.GetCustomAttributes(false);
              if (custAttrib == null)
                  continue;

              Type ptt = prop.PropertyType;

              for (int k = 0; k < custAttrib.Length; k++)
              {
                  if (custAttrib[k] is ConnectionType)
                  {
                      ConnectionType ct = custAttrib[k] as ConnectionType;
                      if (ct.ConnType == "Param")
                      {
                          Label ll = new Label();
                          ll.Text = ct.Description;
                          ll.Left = textLeft;
                          ll.Top = (this.Controls.Count) * objHeight + 5;

                          
                          Control ctrl = giveControlForType(ptt,prop);

                          ctrl.Left = objLeft;
                          ctrl.Top = (this.Controls.Count) * objHeight + 5;
                          ctrl.Width = 300;

                          this.Controls.Add(ctrl);
                          this.Controls.Add(ll);
                          
                      }

                      break;
                  }
              }
          }


          button1.Top = (this.Controls.Count) * objHeight;
          this.Controls.Add(this.button1);

          this.Height = (this.Controls.Count+3) * objHeight;

          this.ResumeLayout();

        }

        Control giveControlForType(Type t,System.Reflection.PropertyInfo pi)
        {
            Control ct = null;
            if (t == typeof(float))
            {
               float vt = (float)pi.GetValue(mOwnerNode, null);
               pi.SetValue(mOwnerNode, float.MaxValue, null);
               float max = (float)pi.GetValue(mOwnerNode, null);
               pi.SetValue(mOwnerNode, float.MinValue, null);
               float min = (float)pi.GetValue(mOwnerNode, null);
               pi.SetValue(mOwnerNode,vt, null);

                NumericSliderControl c = new NumericSliderControl();
                c.Setup(min, max, true);
                c.NumericValue = vt;
                ct = c;
            }
            else if (t == typeof(int))
            {

               int vt = (int)pi.GetValue(mOwnerNode, null);
               pi.SetValue(mOwnerNode, int.MaxValue, null);
               int max = (int)pi.GetValue(mOwnerNode, null);
               pi.SetValue(mOwnerNode, int.MinValue, null);
               int min = (int)pi.GetValue(mOwnerNode, null);
               pi.SetValue(mOwnerNode, vt, null);

                NumericSliderControl c = new NumericSliderControl();
                c.Setup(min, max, false);
                c.NumericValue = vt;
                ct = c;
            }
            else if (t == typeof(bool))
            {
                CheckBox cb = new CheckBox();
                cb.Checked = (bool)pi.GetValue(mOwnerNode, null);
                ct = cb;
            }
            else
            {
                MessageBox.Show("Type " + t.ToString() + " not supported in dialog!");
            }


            return ct;
        }

        void setControlValueToProperty(Control ctrl, Type t, System.Reflection.PropertyInfo pi)
        {
            if (t == typeof(float))
            {
                NumericSliderControl c = (NumericSliderControl)ctrl;
                pi.SetValue(mOwnerNode, (float)c.NumericValue, null); 
            }
            else if (t == typeof(int))
            {
                NumericSliderControl c = (NumericSliderControl)ctrl;
                pi.SetValue(mOwnerNode, (int)c.NumericValue, null); 
            }
            else if (t == typeof(bool))
            {
                CheckBox c = (CheckBox)ctrl;
                pi.SetValue(mOwnerNode, (bool)c.Checked, null);
            }
            else
            {
                MessageBox.Show("Type " + t.ToString() + " not supported in dialog!");
            }
        }

        void propigateUItoVars()
        {
            //CLM this assumes that no controls have been added since init!
            
            int ctrlCount = startingControlCount;
            Type type = mOwnerNode.GetType();// Get object type
            System.Reflection.PropertyInfo[] pi = type.GetProperties();
            for (int i = 0; i < pi.Length; i++)
            {
                System.Reflection.PropertyInfo prop = pi[i];

                object[] custAttrib = prop.GetCustomAttributes(false);
                if (custAttrib == null)
                    continue;

                Type ptt = prop.PropertyType;

                for (int k = 0; k < custAttrib.Length; k++)
                {
                    if (custAttrib[k] is ConnectionType)
                    {
                        ConnectionType ct = custAttrib[k] as ConnectionType;
                        if (ct.ConnType == "Param")
                        {
                            setControlValueToProperty(this.Controls[ctrlCount*2], ptt,prop);

                            ctrlCount++;
                        }

                        break;
                    }
                }
            }
        }

       

        private void button2_Click(object sender, EventArgs e)
        {
           
        }

        private void button1_Click(object sender, EventArgs e)
        {
            propigateUItoVars();
            mOwnerNode.generatePreview();
        }

        private void Device_PropertyDlg_Load(object sender, EventArgs e)
        {

        }
    }
}
