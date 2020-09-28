using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Xceed.Editors;
using Xceed.SmartUI;
using Xceed.SmartUI.Controls.OptionList;

namespace ParticleSystem
{
   public partial class TexturePage : UserControl
   {
      private ParticleEmitter mData;
      private bool bInitialized = false;
      public TexturePage()
      {
         InitializeComponent();

         //-- register event handlers
         smartOptionList1.ItemClick += new Xceed.SmartUI.SmartItemClickEventHandler(smartOptionList1_ItemClick);
         smartOptionList2.ItemClick += new Xceed.SmartUI.SmartItemClickEventHandler(smartOptionList2_ItemClick);         
         for (int i = 0; i < smartOptionList1.Items.Count; i++)
         {
            smartOptionList1.Items[i].Tag = new TextureData.DiffuseLayerBlendType();
         }

         for (int j = 0; j < smartOptionList2.Items.Count; j++)
         {
            smartOptionList2.Items[j].Tag = new TextureData.DiffuseLayerBlendType();
         }         

         //-- set the corresponding radiobutton tags
         smartOptionList1.Items[0].Tag = TextureData.DiffuseLayerBlendType.eBlendMultiply;
         smartOptionList1.Items[1].Tag = TextureData.DiffuseLayerBlendType.eBlendAlpha;

         smartOptionList2.Items[0].Tag = TextureData.DiffuseLayerBlendType.eBlendMultiply;
         smartOptionList2.Items[1].Tag = TextureData.DiffuseLayerBlendType.eBlendAlpha;
      }

      public void setData(ParticleEmitter e)
      {
         mData = e;
         this.textureControl1.setData(mData.Textures.Diffuse);                  
         this.textureControl4.setData(mData.Textures.Diffuse2);
         this.textureControl6.setData(mData.Textures.Diffuse3);
         this.textureControl3.setData(mData.Textures.Intensity);

         this.uvControl1.setData(mData.Textures.Diffuse.UVAnimation);                  
         this.uvControl4.setData(mData.Textures.Diffuse2.UVAnimation);
         this.uvControl6.setData(mData.Textures.Diffuse3.UVAnimation);
         this.uvControl3.setData(mData.Textures.Intensity.UVAnimation);

         getModifiedData();
         bInitialized = true;
      }

      private void setModifiedData()
      {
         if (!bInitialized)
            return;

      }

      private void getModifiedData()
      {
         for (int i = 0; i < smartOptionList1.Items.Count; i++)
         {
            if (mData.Textures.DiffuseLayer1To2BlendMode == (TextureData.DiffuseLayerBlendType)smartOptionList1.Items[i].Tag)
            {
               ((RadioButtonNode)smartOptionList1.Items[i]).Checked = true;
               break;
            }
         }

         for (int i = 0; i < smartOptionList2.Items.Count; i++)
         {
            if (mData.Textures.DiffuseLayer2To3BlendMode == (TextureData.DiffuseLayerBlendType)smartOptionList2.Items[i].Tag)
            {
               ((RadioButtonNode)smartOptionList2.Items[i]).Checked = true;
               break;
            }
         }         
      }

      private void smartOptionList1_ItemClick(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         mData.Textures.DiffuseLayer1To2BlendMode = (TextureData.DiffuseLayerBlendType)e.Item.Tag;
      }

      private void smartOptionList2_ItemClick(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         mData.Textures.DiffuseLayer2To3BlendMode = (TextureData.DiffuseLayerBlendType)e.Item.Tag;
      }      
   }
}
