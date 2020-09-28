// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "common/particle/Particles.h"
#include "x86/win32/dx9/render/dynamic_vb.h"
#include "x86/win32/dx9/render/effects.h"
#include "x86/win32/dx9/render/texture_manager.h"
#include "x86/win32/dx9/render/render_viewport.h"

namespace gr
{
	// Defines.

	const int ParticleVBSize = 6000;

	// Classes.
	
	class ParticleRenderer
	{
		private:
			typedef std::map<std::string, TextureProxy *> TextureMap;
						
			DynamicVB	mDynamicVB;
			TextureMap mTextureMap;

			unsigned int mParticleCount;
									
			Effects mEffects;
			
			bool mLoadedTextures;
			
			bool mClearBuffer;
			
			void renderQuads(ParticleSystemManager* pManager, const RenderViewport& renderViewport);
			void hdrAccum(const RenderViewport& renderViewport);
						
			// Do NOT copy construct or assign.
			ParticleRenderer(const ParticleRenderer& rhs);
			ParticleRenderer & operator =(const ParticleRenderer &);
										
		public:
			ParticleRenderer();
			~ParticleRenderer();
			
			unsigned int getParticleCount(void) const { return mParticleCount; };
			
			void loadTextures(ParticleSystemManager* pManager);
			
			void oneTimeSceneInit(void);
			void initDeviceObjects(void);
			void deleteDeviceObjects(void);
			void invalidateDeviceObjects(void);
			void restoreDeviceObjects(void);
			void reloadEffects(void);
					
			void flushTextures(void);
			
			bool render(ParticleSystemManager* pManager, const RenderViewport& renderViewport);
	};

} // namespace gr

#endif /* _RENDERER_H_ */