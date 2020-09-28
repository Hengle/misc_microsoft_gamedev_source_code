using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;

namespace graphapp
{

   public class DAGCanvas :GraphCanvas
   {
      #region ctor
      public DAGCanvas()
         :
         base(256, 256)
      {
         initRules();
      }

      public DAGCanvas(int width, int height):
         base(width,height)
      {
         initRules();
      }

      void initRules()
      {
         addConnectionRule(typeof(InputConnectionPoint),
            new Type[] { typeof(OutputConnectionPoint) },
            new Type[] { typeof(ParamConnectionPoint), typeof(InputConnectionPoint) });

         addConnectionRule(typeof(OutputConnectionPoint),
            new Type[] { typeof(InputConnectionPoint), typeof(ParamConnectionPoint), typeof(ConstraintConnectionPoint) },
            new Type[] { typeof(OutputConnectionPoint) });

         addConnectionRule(typeof(ParamConnectionPoint),
             new Type[] { typeof(OutputConnectionPoint) },
             new Type[] { typeof(ParamConnectionPoint), typeof(InputConnectionPoint), typeof(ConstraintConnectionPoint) });

         addConnectionRule(typeof(ConstraintConnectionPoint),
           new Type[] { typeof(OutputConnectionPoint) },
           new Type[] { typeof(ParamConnectionPoint), typeof(InputConnectionPoint),typeof(ConstraintConnectionPoint) });
      }
      #endregion

      #region execute
      public DAGMask execute(int width, int height)
      {
         List<int> outputNodes = findIndexesOfNodesOfType(typeof(Device_ToCurrentMask));

         if(outputNodes.Count ==0)
         {
            MessageBox.Show("Error: Cannot compute graph:\n Missing ToCurrentMask output node", "Cannot execute graph", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return null;
         }
         else if (outputNodes.Count > 1)
         {
            MessageBox.Show("Error: Cannot compute graph:\n Multiple ToCurrentMask output nodes found.", "Cannot execute graph", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return null;
         }

         Device_ToCurrentMask dcm = mNodes[outputNodes[0]] as Device_ToCurrentMask;
         
         OutputGenerationParams ogp = new OutputGenerationParams();
         ogp.Width = width;
         ogp.Height = height;

         if (!dcm.computeOutput(null, ogp))
            return null;

         DAGMask dMask = dcm.ResultMask.Clone();
          
         return dMask;

      }
      #endregion

      #region saveLoad
      public class MaskDAGConnectionXML
      {
         [XmlElement("OwnerDeviceID")]
         public Guid mOwnerDeviceID;
         [XmlElement("ConnectionName")]
         public string mConnectionName;
         [XmlElement("NeighborDeviceID")]
         public Guid mNeighborDeviceID;
         [XmlElement("NeighborConnectionName")]
         public string mNeighborConnectionName;
      };
      public class MaskDAGXML
      {
         [XmlArrayItem(ElementName = "Device_CurrentMask", Type = typeof(Device_CurrentMask))]
         [XmlArrayItem(ElementName = "Device_MaskFromList", Type = typeof(Device_MaskFromList))]
         [XmlArrayItem(ElementName = "Device_TerrainHeight", Type = typeof(Device_TerrainHeight))]
         [XmlArrayItem(ElementName = "Device_BillowNoise", Type = typeof(Device_BillowNoise))]
         [XmlArrayItem(ElementName = "Device_PerlinNoise", Type = typeof(Device_PerlinNoise))]
         [XmlArrayItem(ElementName = "Device_Constant", Type = typeof(Device_Constant))]
         [XmlArrayItem(ElementName = "Device_fBmNoise", Type = typeof(Device_fBmNoise))]
         [XmlArrayItem(ElementName = "Device_Gradient", Type = typeof(Device_Gradient))]
         [XmlArrayItem(ElementName = "Device_RidgedNoise", Type = typeof(Device_RidgedNoise))]
         [XmlArrayItem(ElementName = "Device_VoronoiNoise", Type = typeof(Device_VoronoiNoise))]
         [XmlArrayItem(ElementName = "Device_Chooser", Type = typeof(Device_Chooser))]
         [XmlArrayItem(ElementName = "Device_Splitter", Type = typeof(Device_Splitter))]
         [XmlArrayItem(ElementName = "Device_Clamp", Type = typeof(Device_Clamp))]
         [XmlArrayItem(ElementName = "Device_Combiner", Type = typeof(Device_Combiner))]
         [XmlArrayItem(ElementName = "Device_Distort", Type = typeof(Device_Distort))]
         [XmlArrayItem(ElementName = "Device_Flipper", Type = typeof(Device_Flipper))]
         [XmlArrayItem(ElementName = "Device_Mirror", Type = typeof(Device_Mirror))]
         [XmlArrayItem(ElementName = "Device_Inverter", Type = typeof(Device_Inverter))]
         [XmlArrayItem(ElementName = "Device_Smooth", Type = typeof(Device_Smooth))]
         [XmlArrayItem(ElementName = "Device_Terrace", Type = typeof(Device_Terrace))]
         [XmlArrayItem(ElementName = "Device_FileOutput", Type = typeof(Device_FileOutput))]
         [XmlArrayItem(ElementName = "Device_ToCurrentMask", Type = typeof(Device_ToCurrentMask))]
         [XmlArrayItem(ElementName = "Device_SelectHeight", Type = typeof(Device_SelectHeight))]
         [XmlArrayItem(ElementName = "Device_SelectSlope", Type = typeof(Device_SelectSlope))]
         [XmlArrayItem(ElementName = "Device_HydraulicErosion", Type = typeof(Device_HydraulicErosion))]
         [XmlArrayItem(ElementName = "Device_BiasGain", Type = typeof(Device_BiasGain))]
         [XmlArrayItem(ElementName = "Device_Curves", Type = typeof(Device_Curves))]
         [XmlArrayItem(ElementName = "Device_RandomClampedFloat", Type = typeof(Device_RandomClampedFloat))]
         [XmlArrayItem(ElementName = "Device_RandomInt", Type = typeof(Device_RandomInt))]
         [XmlArrayItem(ElementName = "Device_DistanceTransform", Type = typeof(Device_DistanceTransform))]
         [XmlArrayItem(ElementName = "Device_Expand", Type = typeof(Device_Expand))]
         [XmlArrayItem(ElementName = "Device_Contract", Type = typeof(Device_Contract))]
         [XmlArrayItem(ElementName = "Device_GaussianBlur", Type = typeof(Device_GaussianBlur))]
         [XmlArrayItem(ElementName = "Device_MedianBlur", Type = typeof(Device_MedianBlur))]
         [XmlArrayItem(ElementName = "Device_RadialGradient", Type = typeof(Device_RadialGradient))]
         
         
         [XmlArray("MaskDAGGraphNodes")]
         public List<MaskDAGGraphNode> mNodes = new List<MaskDAGGraphNode>();

         [XmlArray("MaskDAGConnections")]
         public List<MaskDAGConnectionXML> mConnections = new List<MaskDAGConnectionXML>();
      };


      
      public override bool saveCanvasToDisk(string filename)
      {
         MaskDAGXML xmldat = prepCanvasForSave();
     
         EditorCore.Controls.Micro.BaseLoader<MaskDAGXML>.Save(filename, xmldat);

         return true;
      }
      public override bool loadCanvasFromDisk(string filename)
      {
        

         MaskDAGXML xmldat = EditorCore.Controls.Micro.BaseLoader<MaskDAGXML>.Load(filename);

         if(xmldat == null)
         {
            MessageBox.Show("Could not load " + filename + ". An error occured");
            return false;
         }
         populateCanvasFromLoad(xmldat);

         return true;
      }
      public override bool saveCanvasToMemoryStream(MemoryStream ms)
      {
         try
         {
            MaskDAGXML xmldat = prepCanvasForSave();

            XmlSerializer s = new XmlSerializer(typeof(MaskDAGXML), new Type[] { });
            XmlTextWriter xmlTextWriter = new XmlTextWriter(ms, Encoding.UTF8);
            s.Serialize(xmlTextWriter, xmldat);
            ms = (MemoryStream)xmlTextWriter.BaseStream;
            ms.Seek(0, SeekOrigin.Begin);

         }
         catch (System.Exception ex)
         {
            ex.ToString();
            return false;
         }

         return true;
      }     
      public override bool loadCanvasFromMemoryStream(MemoryStream ms)
      {
         MaskDAGXML xmldat = null;
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(MaskDAGXML), new Type[] { });
            xmldat = ((MaskDAGXML)s.Deserialize(ms));

            populateCanvasFromLoad(xmldat);
         }
         catch (System.Exception ex)
         {
            ex.ToString();
            return false;
         }

         return true;
      }

      //general functions
      MaskDAGXML prepCanvasForSave()
      {

         MaskDAGXML xmldat = new MaskDAGXML();

         for (int i = 0; i < mNodes.Count; i++)
            if (mNodes[i].mDepthLayer == 0)
               xmldat.mNodes.Add(mNodes[i] as MaskDAGGraphNode);

         //generate connection records
         for (int i = 0; i < mNodes.Count; i++)
         {
            MaskDAGGraphNode dnode = (mNodes[i] as MaskDAGGraphNode);
            if (dnode != null)
               xmldat.mConnections.AddRange(dnode.generateConnectionLists());
         }
         return xmldat;
      }
      void populateCanvasFromLoad(MaskDAGXML xmldat)
      {
         cleanCanvas();
         newCanvas();

         for (int i = 0; i < xmldat.mNodes.Count; i++)
         {
            Type t = xmldat.mNodes[i].GetType();
            MaskDAGGraphNode val = null;
            try
            {
               System.Reflection.ConstructorInfo ci = t.GetConstructor(new Type[] { typeof(GraphCanvas) });
               val = ((MaskDAGGraphNode)ci.Invoke(new object[] { this }));
            }
            catch (Exception e)
            {
               e.ToString();
               return;
            }
            val.load(xmldat.mNodes[i]);
            addCanvasNode(val);
         }

         //load our connections now
         for (int i = 0; i < xmldat.mConnections.Count; i++)
         {
            CanvasNode start = findConnectionNode(xmldat.mConnections[i].mOwnerDeviceID, xmldat.mConnections[i].mConnectionName);
            CanvasNode end = findConnectionNode(xmldat.mConnections[i].mNeighborDeviceID, xmldat.mConnections[i].mNeighborConnectionName);
            if (start == null || end == null) continue;

            addConnection(start, end);

         }
      }
      CanvasNode findConnectionNode(Guid parentUID, string paramT)
      {
         for (int i = 0; i < mNodes.Count; i++)
         {
            if(mNodes[i] is ConnectionPoint)
            {
               ConnectionPoint cp = mNodes[i] as ConnectionPoint;
               if (cp.OwnerNode.UniqueID == parentUID && cp.ID == paramT)
                  return mNodes[i];
            }
         }
            return null;
      }
      #endregion

      #region selection / filtering
      protected override void filterForConnection(CanvasNode node, connectionRule rule)
      {
          foreach (CanvasNode gn in mNodes)
          {
              Type gnType = gn.GetType();
              if (!rule.isValidType(gnType))
              {
                  if (!(gn is ConnectionPoint))
                  {
                      gn.IsVisible = true;
                  }
                  else
                  {
                      gn.IsVisible = false;
                  }
                  gn.IsEnabled = false;
              }
              else
              {
                    ConnectionPoint cp = node as ConnectionPoint;
                    ConnectionPoint cpg = gn as ConnectionPoint;
                    if (cp.OwnerNode == cpg.OwnerNode ||
                        cp.ParamType.Type != cpg.ParamType.Type)
                    {
                        gn.IsVisible = false;
                        gn.IsEnabled = false;
                    }
              }
          }
      }

      protected override bool isConnectionType(Type objType)
      {
          if (objType != typeof(ConnectionPoint))
              return false;

          for (int i = 0; i < mConnectionRules.Count; i++)
          {
              if (mConnectionRules[i].mTargetType == objType)
                  return true;
          }
          return false;
       }

      protected List<int> findIndexesOfNodesOfType(Type t)
      {
         List<int> it = new List<int>();

         for (int i = 0; i < mNodes.Count; i++)
         {
            if(mNodes[i].GetType() == t)
            {
               it.Add(i);
            }
         }
         return it;
      }
      #endregion

       #region UI
       public override void onKeyUp(KeyEventArgs keyEvent)
      {
          base.onKeyUp(keyEvent);

          if (keyEvent.KeyCode == Keys.Return)
          {
              //BUILD!

             
          }
          else if (keyEvent.KeyCode == Keys.P)
          {
             //BUILD!

             
          }
      }

      public override void onDoubleClick(Point mousePoint, MouseEventArgs mouseEvent)
      {
          CanvasNode selectedNode = intersectNodeMouseLoc(mouseEvent.Location);
          if (selectedNode != null)
          {
              MaskDAGGraphNode gn = selectedNode as MaskDAGGraphNode;
              if (gn == null)
                  return;
              if (mouseEvent.Button == MouseButtons.Left)
              {
                gn.displayPropertiesDlg();
              }
          }
       }
       #endregion

       #region prviewProcessing

      
       public delegate void onUpdateCalculate(ref DAGMask m);
      public onUpdateCalculate onUpdateCallback;

      public void generatePreview(CanvasNode cn)
      {
          OutputGenerationParams ogp = new OutputGenerationParams();
          ogp.Width = 128;
          ogp.Height = 128;

          MaskDAGGraphNode gn = (MaskDAGGraphNode)cn;
          MaskParam mp = new MaskParam();
          InputConnectionPoint icp = new InputConnectionPoint(mp,true,"Preview",gn,this);
          gn.computeOutput(icp,ogp);

          if (onUpdateCallback != null)
          {
              DAGMask m = mp.Value;
              onUpdateCallback(ref m);
          }
       }
       #endregion

       #region components

       //inputs
      private System.Windows.Forms.ToolStripMenuItem currentMaskToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem maskFromListToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem terrainHeightToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem inputsToolStripMenuItem;

      //generators
      private System.Windows.Forms.ToolStripMenuItem constantScalarToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem perlinNoiseToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem voroniNoiseToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem billowNoiseToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem fBmNoiseToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem ridgedNoiseToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem gradientNoiseToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem radialGradientToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem generatorsToolStripMenuItem;


      //modifiers
      private System.Windows.Forms.ToolStripMenuItem combinerToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem inverterToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem flipperToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem mirrorToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem clampToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem smoothToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem terraceToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem distortToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem biasgainToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem curvesToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem distanceTransToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem expandToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem contractToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem gaussianBlurToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem medianBlurToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem gradiateToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem modifiersToolStripMenuItem;

      //Graph Logic
      private System.Windows.Forms.ToolStripMenuItem splitterToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem chooserToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem graphLogicToolStripMenuItem;


      //Output
      private System.Windows.Forms.ToolStripMenuItem fileOutputToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem toCurrentMaskOutputToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem outputsToolStripMenuItem;

      //Selectors
      private System.Windows.Forms.ToolStripMenuItem selectHeightToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem selectSlopeToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem selectGradientToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem selectorsToolStripMenuItem;

      //Numeric
      private System.Windows.Forms.ToolStripMenuItem randomClampedFloatToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem randomIntToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem numericsToolStripMenuItem;

      //Erosion
      private System.Windows.Forms.ToolStripMenuItem hydraulicErosionToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem erosionToolStripMenuItem;

      private System.Windows.Forms.ToolStripMenuItem newNodeToolStripMenuItem;
      

      protected override void InitializeComponent()
      {
         base.InitializeComponent();
         this.contextMenuStrip1.SuspendLayout();

         #region inputsToolStripMenuItem


         // 
         // currentMaskToolStripMenuItem
         // 
         this.currentMaskToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.currentMaskToolStripMenuItem.Name = "currentMaskToolStripMenuItem";
         this.currentMaskToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.currentMaskToolStripMenuItem.Text = "Current Mask";
         this.currentMaskToolStripMenuItem.Click += new EventHandler(currentMaskToolStripMenuItem_Click);


         // 
         // maskFromListToolStripMenuItem
         // 
         this.maskFromListToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.maskFromListToolStripMenuItem.Name = "maskFromListToolStripMenuItem";
         this.maskFromListToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.maskFromListToolStripMenuItem.Text = "Mask From List";
         this.maskFromListToolStripMenuItem.Click += new EventHandler(maskFromListToolStripMenuItem_Click);

         
         // 
         // terrainHeightToolStripMenuItem
         // 
         this.terrainHeightToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.terrainHeightToolStripMenuItem.Name = "terrainHeightToolStripMenuItem";
         this.terrainHeightToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.terrainHeightToolStripMenuItem.Text = "Terrain Heights";
         this.terrainHeightToolStripMenuItem.Click += new EventHandler(terrainHeightToolStripMenuItem_Click);


               // 
         // inputsToolStripMenuItem
         // 
         this.inputsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.inputsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
          //  this.currentMaskToolStripMenuItem,
            this.maskFromListToolStripMenuItem,
            this.terrainHeightToolStripMenuItem});
         this.inputsToolStripMenuItem.Name = "inputsToolStripMenuItem";
         this.inputsToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.inputsToolStripMenuItem.Text = "Inputs";

        #endregion

         #region generatorsToolStripMenuItem
         // 
         // constantScalarToolStripMenuItem
         // 
         this.constantScalarToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.constantScalarToolStripMenuItem.Name = "constantScalarToolStripMenuItem";
         this.constantScalarToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.constantScalarToolStripMenuItem.Text = "Constant Scalar";
         this.constantScalarToolStripMenuItem.Click += new System.EventHandler(this.constantScalarToolStripMenuItem_Click);

         // 
         

         // 
         // perlinNoiseToolStripMenuItem
         // 
         this.perlinNoiseToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.perlinNoiseToolStripMenuItem.Name = "perlinNoiseToolStripMenuItem";
         this.perlinNoiseToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.perlinNoiseToolStripMenuItem.Text = "Perlin Noise";
         this.perlinNoiseToolStripMenuItem.Click +=new EventHandler(perlinNoiseToolStripMenuItem_Click);

           // 
         // billowNoiseToolStripMenuItem
         // 
         this.billowNoiseToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.billowNoiseToolStripMenuItem.Name = "billowNoiseToolStripMenuItem";
         this.billowNoiseToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.billowNoiseToolStripMenuItem.Text = "Billow Noise";
         this.billowNoiseToolStripMenuItem.Click +=new EventHandler(billowNoiseToolStripMenuItem_Click);


                     // 
         // fBmNoiseToolStripMenuItem
         // 
         this.fBmNoiseToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.fBmNoiseToolStripMenuItem.Name = "fBmNoiseToolStripMenuItem";
         this.fBmNoiseToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.fBmNoiseToolStripMenuItem.Text = "fBm Noise";
         this.fBmNoiseToolStripMenuItem.Click +=new EventHandler(fBmNoiseToolStripMenuItem_Click);


                               // 
         // ridgedNoiseToolStripMenuItem
         // 
         this.ridgedNoiseToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.ridgedNoiseToolStripMenuItem.Name = "ridgedNoiseToolStripMenuItem";
         this.ridgedNoiseToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.ridgedNoiseToolStripMenuItem.Text = "Ridged Noise";
         this.ridgedNoiseToolStripMenuItem.Click +=new EventHandler(ridgedNoiseToolStripMenuItem_Click);

         // 
         // voroniNoiseToolStripMenuItem
         // 
         this.voroniNoiseToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.voroniNoiseToolStripMenuItem.Name = "voroniNoiseToolStripMenuItem";
         this.voroniNoiseToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.voroniNoiseToolStripMenuItem.Text = "Voronoi Noise";
         this.voroniNoiseToolStripMenuItem.Click +=new EventHandler(voroniNoiseToolStripMenuItem_Click);

         // 
         // gradientNoiseToolStripMenuItem
         // 
         this.gradientNoiseToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.gradientNoiseToolStripMenuItem.Name = "gradientNoiseToolStripMenuItem";
         this.gradientNoiseToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.gradientNoiseToolStripMenuItem.Text = "Gradient";
         this.gradientNoiseToolStripMenuItem.Click += new EventHandler(gradientNoiseToolStripMenuItem_Click);

          // 
         // radialGradientToolStripMenuItem
         // 
         this.radialGradientToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.radialGradientToolStripMenuItem.Name = "radialGradientToolStripMenuItem";
         this.radialGradientToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.radialGradientToolStripMenuItem.Text = "Radial Gradient";
         this.radialGradientToolStripMenuItem.Click += new EventHandler(radialGradientToolStripMenuItem_Click);

         
         

         // 
         // generatorsToolStripMenuItem
         // 
         this.generatorsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.generatorsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.billowNoiseToolStripMenuItem,
            this.constantScalarToolStripMenuItem,
            this.fBmNoiseToolStripMenuItem,
            this.gradientNoiseToolStripMenuItem,
            this.perlinNoiseToolStripMenuItem,
            this.radialGradientToolStripMenuItem,
            this.ridgedNoiseToolStripMenuItem,
            this.voroniNoiseToolStripMenuItem,
            
         });
         this.generatorsToolStripMenuItem.Name = "generatorsToolStripMenuItem";
         this.generatorsToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.generatorsToolStripMenuItem.Text = "Generators";

         #endregion

         #region modifiersToolStripMenuItem

         // 
         // combinerToolStripMenuItem
         // 
         this.combinerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.combinerToolStripMenuItem.Name = "combinerToolStripMenuItem";
         this.combinerToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.combinerToolStripMenuItem.Text = "Combiner";
         this.combinerToolStripMenuItem.Click += new EventHandler(combinerToolStripMenuItem_Click);

         // 
         // distortToolStripMenuItem
         // 
         this.distortToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.distortToolStripMenuItem.Name = "distortToolStripMenuItem";
         this.distortToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.distortToolStripMenuItem.Text = "Distort";
         this.distortToolStripMenuItem.Click += new EventHandler(distortToolStripMenuItem_Click);

          // 
         // inverterToolStripMenuItem
         // 
         this.inverterToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.inverterToolStripMenuItem.Name = "inverterToolStripMenuItem";
         this.inverterToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.inverterToolStripMenuItem.Text = "Inverter";
         this.inverterToolStripMenuItem.Click += new EventHandler(inverterToolStripMenuItem_Click);


           // 
         // flipperToolStripMenuItem
         // 
         this.flipperToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.flipperToolStripMenuItem.Name = "flipperToolStripMenuItem";
         this.flipperToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.flipperToolStripMenuItem.Text = "Flipper";
         this.flipperToolStripMenuItem.Click += new EventHandler(flipperToolStripMenuItem_Click);


         // 
         // mirrorToolStripMenuItem
         // 
         this.mirrorToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.mirrorToolStripMenuItem.Name = "mirrorToolStripMenuItem";
         this.mirrorToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.mirrorToolStripMenuItem.Text = "Mirror";
         this.mirrorToolStripMenuItem.Click += new EventHandler(mirrorToolStripMenuItem_Click);


         

          // 
         // clampToolStripMenuItem
         // 
         this.clampToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.clampToolStripMenuItem.Name = "clampToolStripMenuItem";
         this.clampToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.clampToolStripMenuItem.Text = "Clamp";
         this.clampToolStripMenuItem.Click += new EventHandler(clampToolStripMenuItem_Click);

          

          // 
         // smoothToolStripMenuItem
         // 
         this.smoothToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.smoothToolStripMenuItem.Name = "smoothToolStripMenuItem";
         this.smoothToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.smoothToolStripMenuItem.Text = "Smooth";
         this.smoothToolStripMenuItem.Click += new EventHandler(smoothToolStripMenuItem_Click);

         // 
         // terraceToolStripMenuItem
         // 
         this.terraceToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.terraceToolStripMenuItem.Name = "terraceToolStripMenuItem";
         this.terraceToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.terraceToolStripMenuItem.Text = "Terrace";
         this.terraceToolStripMenuItem.Click += new EventHandler(terraceToolStripMenuItem_Click);

          // 
         // biasgainToolStripMenuItem
         // 
         this.biasgainToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.biasgainToolStripMenuItem.Name = "biasgainToolStripMenuItem";
         this.biasgainToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.biasgainToolStripMenuItem.Text = "Bias / Gain";
         this.biasgainToolStripMenuItem.Click += new EventHandler(biasgainToolStripMenuItem_Click);

         // 
         // curvesToolStripMenuItem
         // 
         this.curvesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.curvesToolStripMenuItem.Name = "curvesToolStripMenuItem";
         this.curvesToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.curvesToolStripMenuItem.Text = "Curves";
         this.curvesToolStripMenuItem.Click += new EventHandler(curvesToolStripMenuItem_Click);

         // 
         // distanceTransToolStripMenuItem
         // 
         this.distanceTransToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.distanceTransToolStripMenuItem.Name = "distanceTransToolStripMenuItem";
         this.distanceTransToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.distanceTransToolStripMenuItem.Text = "Distance Transform";
         this.distanceTransToolStripMenuItem.Click += new EventHandler(distanceTransToolStripMenuItem_Click);

         // 
         // expandToolStripMenuItem
         // 
         this.expandToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.expandToolStripMenuItem.Name = "expandToolStripMenuItem";
         this.expandToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.expandToolStripMenuItem.Text = "Expand";
         this.expandToolStripMenuItem.Click += new EventHandler(expandToolStripMenuItem_Click);

         // 
         // contractToolStripMenuItem
         // 
         this.contractToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.contractToolStripMenuItem.Name = "contractToolStripMenuItem";
         this.contractToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.contractToolStripMenuItem.Text = "Contract";
         this.contractToolStripMenuItem.Click += new EventHandler(contractToolStripMenuItem_Click);

         // 
         // gaussianBlurToolStripMenuItem
         // 
         this.gaussianBlurToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.gaussianBlurToolStripMenuItem.Name = "gaussianBlurToolStripMenuItem";
         this.gaussianBlurToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.gaussianBlurToolStripMenuItem.Text = "Gaussian Blur";
         this.gaussianBlurToolStripMenuItem.Click += new EventHandler(gaussianBlurToolStripMenuItem_Click);

         // 
         // medianBlurToolStripMenuItem
         // 
         this.medianBlurToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.medianBlurToolStripMenuItem.Name = "medianBlurToolStripMenuItem";
         this.medianBlurToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.medianBlurToolStripMenuItem.Text = "Median Blur";
         this.medianBlurToolStripMenuItem.Click += new EventHandler(medianBlurToolStripMenuItem_Click);



         // 
         // modifiersToolStripMenuItem
         // 
         this.modifiersToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.modifiersToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.biasgainToolStripMenuItem,
            this.clampToolStripMenuItem,
            this.combinerToolStripMenuItem,
            this.contractToolStripMenuItem,
            this.curvesToolStripMenuItem,
            this.distanceTransToolStripMenuItem,
            this.distortToolStripMenuItem,
            this.expandToolStripMenuItem,
            this.flipperToolStripMenuItem,
            this.gaussianBlurToolStripMenuItem,
            this.inverterToolStripMenuItem,
            this.medianBlurToolStripMenuItem,
            this.mirrorToolStripMenuItem,
            this.smoothToolStripMenuItem,
            this.terraceToolStripMenuItem,
         });
         this.modifiersToolStripMenuItem.Name = "modifiersToolStripMenuItem";
         this.modifiersToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.modifiersToolStripMenuItem.Text = "Modifiers";
         #endregion

         #region erosion
         // 
         // hydraulicErosionToolStripMenuItem
         // 
         this.hydraulicErosionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.hydraulicErosionToolStripMenuItem.Name = "hydraulicErosionToolStripMenuItem";
         this.hydraulicErosionToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.hydraulicErosionToolStripMenuItem.Text = "Hydraulic Erosion";
         this.hydraulicErosionToolStripMenuItem.Click += new EventHandler(hydraulicErosionToolStripMenuItem_Click);


         // 
         // erosionToolStripMenuItem
         // 
         this.erosionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.erosionToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.hydraulicErosionToolStripMenuItem,
         });
         this.erosionToolStripMenuItem.Name = "erosionToolStripMenuItem";
         this.erosionToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.erosionToolStripMenuItem.Text = "Erosion";
         #endregion

         #region graphLogicToolStripMenuItem
         // 
         // splitterToolStripMenuItem
         // 
         this.splitterToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.splitterToolStripMenuItem.Name = "splitterToolStripMenuItem";
         this.splitterToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.splitterToolStripMenuItem.Text = "Splitter";
         this.splitterToolStripMenuItem.Click += new EventHandler(splitterToolStripMenuItem_Click);
   
        
       
         // 
         // chooserToolStripMenuItem
         // 
         this.chooserToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.chooserToolStripMenuItem.Name = "chooserToolStripMenuItem";
         this.chooserToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.chooserToolStripMenuItem.Text = "Chooser";
         this.chooserToolStripMenuItem.Click += new EventHandler(chooserToolStripMenuItem_Click);


         // 
         // graphLogicToolStripMenuItem
         // 
         this.graphLogicToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.graphLogicToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.splitterToolStripMenuItem,
            this.chooserToolStripMenuItem});
         this.graphLogicToolStripMenuItem.Name = "graphLogicToolStripMenuItem";
         this.graphLogicToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.graphLogicToolStripMenuItem.Text = "Graph Logic";
         #endregion

         #region outputsToolStripMenuItem
         // 
         // fileOutputToolStripMenuItem
         // 
         this.fileOutputToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.fileOutputToolStripMenuItem.Name = "fileOutputToolStripMenuItem";
         this.fileOutputToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.fileOutputToolStripMenuItem.Text = "File Output";
         this.fileOutputToolStripMenuItem.Click += new EventHandler(fileOutputToolStripMenuItem_Click);

         // 
         // toCurrentMaskOutputToolStripMenuItem
         // 
         this.toCurrentMaskOutputToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toCurrentMaskOutputToolStripMenuItem.Name = "toCurrentMaskOutputToolStripMenuItem";
         this.toCurrentMaskOutputToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.toCurrentMaskOutputToolStripMenuItem.Text = "To Current Mask";
         this.toCurrentMaskOutputToolStripMenuItem.Click += new EventHandler(toCurrentMaskOutputToolStripMenuItem_Click);


         // 
         // outputsToolStripMenuItem
         // 
         this.outputsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.outputsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toCurrentMaskOutputToolStripMenuItem,
            this.fileOutputToolStripMenuItem});
         this.outputsToolStripMenuItem.Name = "outputsToolStripMenuItem";
         this.outputsToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.outputsToolStripMenuItem.Text = "Outputs";

         #endregion

         #region selectorsToolStripMenuItem

       
         // 
         // selectHeightToolStripMenuItem
         // 
         this.selectHeightToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.selectHeightToolStripMenuItem.Name = "selectHeightToolStripMenuItem";
         this.selectHeightToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.selectHeightToolStripMenuItem.Text = "Select Height";
         this.selectHeightToolStripMenuItem.Click += new EventHandler(selectHeightToolStripMenuItem_Click);

         // 
         // selectSlopeToolStripMenuItem
         // 
         this.selectSlopeToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.selectSlopeToolStripMenuItem.Name = "selectSlopeToolStripMenuItem";
         this.selectSlopeToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.selectSlopeToolStripMenuItem.Text = "Select Slope";
         this.selectSlopeToolStripMenuItem.Click += new EventHandler(selectSlopeToolStripMenuItem_Click);

         // 
         // selectGradientToolStripMenuItem
         // 
         this.selectGradientToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.selectGradientToolStripMenuItem.Name = "selectGradientToolStripMenuItem";
         this.selectGradientToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.selectGradientToolStripMenuItem.Text = "Select Gradient";
         this.selectGradientToolStripMenuItem.Click += new EventHandler(selectGradientToolStripMenuItem_Click);


         // 
         // selectorsToolStripMenuItem
         // 
         this.selectorsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.selectorsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.selectHeightToolStripMenuItem,
            this.selectSlopeToolStripMenuItem,
         //   this.selectGradientToolStripMenuItem
         });
         this.selectorsToolStripMenuItem.Name = "selectorsToolStripMenuItem";
         this.selectorsToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.selectorsToolStripMenuItem.Text = "Selectors";

         #endregion

         #region numericsToolStripMenuItem

         

          // 
         // randomClampedFloatToolStripMenuItem
         // 
         this.randomClampedFloatToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.randomClampedFloatToolStripMenuItem.Name = "randomClampedFloatToolStripMenuItem";
         this.randomClampedFloatToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.randomClampedFloatToolStripMenuItem.Text = "Random Float [0,1]";
         this.randomClampedFloatToolStripMenuItem.Click += new EventHandler(randomClampedFloatToolStripMenuItem_Click);
         // 
         // randomIntToolStripMenuItem
         // 
         this.randomIntToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.randomIntToolStripMenuItem.Name = "randomIntToolStripMenuItem";
         this.randomIntToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.randomIntToolStripMenuItem.Text = "Random Integer";
         this.randomIntToolStripMenuItem.Click += new EventHandler(randomIntToolStripMenuItem_Click);




         // 
         // numericsToolStripMenuItem
         // 
         this.numericsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.numericsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.randomClampedFloatToolStripMenuItem,
            this.randomIntToolStripMenuItem
         });
         this.numericsToolStripMenuItem.Name = "numericsToolStripMenuItem";
         this.numericsToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.numericsToolStripMenuItem.Text = "Numerics";

         #endregion

         // 
         // newNodeToolStripMenuItem
         // 
         this.newNodeToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.newNodeToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.inputsToolStripMenuItem,
            this.generatorsToolStripMenuItem,
            this.modifiersToolStripMenuItem,
            this.erosionToolStripMenuItem,
            this.selectorsToolStripMenuItem,
            this.numericsToolStripMenuItem,
            this.graphLogicToolStripMenuItem,
            this.outputsToolStripMenuItem});
         this.newNodeToolStripMenuItem.Name = "newNodeToolStripMenuItem";
         this.newNodeToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.newNodeToolStripMenuItem.Text = "New Node";

         this.contextMenuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.newNodeToolStripMenuItem});
         this.contextMenuStrip1.ResumeLayout(false);
      }

      void splitterToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Splitter gn = new Device_Splitter(this);
         mNodes.Add(gn);
      }
      private void constantScalarToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Constant gn = new Device_Constant(this);
         mNodes.Add(gn);
      }

     
      private void perlinNoiseToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_PerlinNoise gn = new Device_PerlinNoise(this);
         mNodes.Add(gn);
      }
      
      private void voroniNoiseToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_VoronoiNoise gn = new Device_VoronoiNoise(this);
         mNodes.Add(gn);
      }
      private void fileOutputToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_FileOutput gn = new Device_FileOutput(this);
         mNodes.Add(gn);
      }
      private void billowNoiseToolStripMenuItem_Click(object sender, EventArgs e)
      {
          Device_BillowNoise gn = new Device_BillowNoise(this);
         mNodes.Add(gn);
      }
      private void fBmNoiseToolStripMenuItem_Click(object sender, EventArgs e)
      {
          Device_fBmNoise gn = new Device_fBmNoise(this);
         mNodes.Add(gn);
      }
      private void ridgedNoiseToolStripMenuItem_Click(object sender, EventArgs e)
      {
          Device_RidgedNoise gn = new Device_RidgedNoise(this);
         mNodes.Add(gn);
      }

      private void inverterToolStripMenuItem_Click(object sender, EventArgs e)
      {
          Device_Inverter gn = new Device_Inverter(this);
         mNodes.Add(gn);
      }
      private void flipperToolStripMenuItem_Click(object sender, EventArgs e)
      {
          Device_Flipper gn = new Device_Flipper(this);
          mNodes.Add(gn);
      }

      private void clampToolStripMenuItem_Click(object sender, EventArgs e)
      {
          Device_Clamp gn = new Device_Clamp(this);
         mNodes.Add(gn);
      }
      private void smoothToolStripMenuItem_Click(object sender, EventArgs e)
      {
          Device_Smooth gn = new Device_Smooth(this);
         mNodes.Add(gn);
      } 
      private void selectHeightToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_SelectHeight gn = new Device_SelectHeight(this);
         mNodes.Add(gn);
      }
      private void selectSlopeToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_SelectSlope gn = new Device_SelectSlope(this);
         mNodes.Add(gn);
      }
      private void chooserToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Chooser gn = new Device_Chooser(this);
         mNodes.Add(gn);
      }
      private void combinerToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Combiner gn = new Device_Combiner(this);
         mNodes.Add(gn);
      }
      private void terraceToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Terrace gn = new Device_Terrace(this);
         mNodes.Add(gn);
      }

      private void gradientNoiseToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Gradient gn = new Device_Gradient(this);
         mNodes.Add(gn);
      }
      private void toCurrentMaskOutputToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_ToCurrentMask gn = new Device_ToCurrentMask(this);
         mNodes.Add(gn);
      }
      private void currentMaskToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_CurrentMask gn = new Device_CurrentMask(this);
         mNodes.Add(gn);
      }
      private void terrainHeightToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_TerrainHeight gn = new Device_TerrainHeight(this);
         mNodes.Add(gn);
      }
      private void selectGradientToolStripMenuItem_Click(object sender, EventArgs e)
      {
       //  Device_SelectGradient gn = new Device_SelectGradient(this);
       //  mNodes.Add(gn);
      }
      private void distortToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Distort gn = new Device_Distort(this);
         mNodes.Add(gn);
      }
      private void hydraulicErosionToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_HydraulicErosion gn = new Device_HydraulicErosion(this);
         mNodes.Add(gn);
      }
      
      private void biasgainToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_BiasGain gn = new Device_BiasGain(this);
         mNodes.Add(gn);
      }
      private void curvesToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Curves gn = new Device_Curves(this);
         mNodes.Add(gn);
      }
      private void maskFromListToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_MaskFromList gn = new Device_MaskFromList(this);
         mNodes.Add(gn);
      }
      private void mirrorToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Mirror gn = new Device_Mirror(this);
         mNodes.Add(gn);
      }
      private void randomClampedFloatToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_RandomClampedFloat gn = new Device_RandomClampedFloat(this);
         mNodes.Add(gn);
      }
      private void randomIntToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_RandomInt gn = new Device_RandomInt(this);
         mNodes.Add(gn);
      }
      private void distanceTransToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_DistanceTransform gn = new Device_DistanceTransform(this);
         mNodes.Add(gn);
      }
      private void expandToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Expand gn = new Device_Expand(this);
         mNodes.Add(gn);
      }
      private void contractToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_Contract gn = new Device_Contract(this);
         mNodes.Add(gn);
      }
      private void gaussianBlurToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_GaussianBlur gn = new Device_GaussianBlur(this);
         mNodes.Add(gn);
      }
      private void medianBlurToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_MedianBlur gn = new Device_MedianBlur(this);
         mNodes.Add(gn);
      }
    
      private void radialGradientToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Device_RadialGradient gn = new Device_RadialGradient(this);
         mNodes.Add(gn);
      }
      
      
      #endregion

   }
}