//-----------------------------------------------------------------------------
// File: d3d_texture_factory.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef D3D_TEXTURE_FACTORY_H
#define D3D_TEXTURE_FACTORY_H

#include "texture_manager.h"
#include "common/filesys/file_system.h"

#include "x86/win32/dx9/render/device.h"

#include <d3d9.h>

namespace gr
{
	class D3DTextureProxy : public TextureProxy
	{
	public:
		D3DTextureProxy(const BigString& filename) : 
			TextureProxy(filename),
			mpTexture(NULL)
		{
			initDeviceObjects();
		}

		virtual ~D3DTextureProxy()
		{
			deleteDeviceObjects();
		}

		virtual IDirect3DBaseTexture9* get(void)
		{
			return mpTexture;
		}
		
		virtual void initDeviceObjects(void)
		{
			if (!mpTexture)
			{
				std::auto_ptr<Stream> pStream = std::auto_ptr<Stream>(gFileSystem.createStream(mFilename));
				if (pStream.get())
				{
					std::vector<char> buffer(pStream->size());
					
					if (pStream->size() == pStream->readBytes(&buffer[0], pStream->size()))
					{
						HRESULT hres = D3DXCreateTextureFromFileInMemory(
							D3D::getDevice(),
							&buffer[0],
							pStream->size(),
							&mpTexture);
						if (FAILED(hres))
						{
							Status("D3DTextureProxy::initDeviceObjects: D3DXCreateTextureFromFileInMemory failed with HRES 0x%X\n", hres);
						}
					}
				}
			}
		}

		virtual void deleteDeviceObjects(void)
		{
			D3D::safeRelease(mpTexture);
		}
		
	protected:
		IDirect3DTexture9* mpTexture;
	};

	class D3DCubeTextureProxy : public TextureProxy
	{
	public:
		D3DCubeTextureProxy(const BigString& filename) : 
			TextureProxy(filename),
			mpCubeTexture(NULL)
		{
			initDeviceObjects();
		}

		virtual ~D3DCubeTextureProxy()
		{
			deleteDeviceObjects();
		}

		virtual IDirect3DBaseTexture9* get(void)
		{
			return mpCubeTexture;
		}
		
		virtual void initDeviceObjects(void)
		{
			if (!mpCubeTexture)
			{
				std::auto_ptr<Stream> pStream = std::auto_ptr<Stream>(gFileSystem.createStream(mFilename));
				if (pStream.get())
				{
					std::vector<char> buffer(pStream->size());
					
					if (pStream->size() == pStream->readBytes(&buffer[0], pStream->size()))
					{
						HRESULT hres = D3DXCreateCubeTextureFromFileInMemory(
							D3D::getDevice(),
							&buffer[0],
							pStream->size(),
							&mpCubeTexture);
						if (FAILED(hres))
						{
							Status("D3DCubeTextureProxy::initDeviceObjects: D3DXCreateCubeTextureFromFileInMemory failed with HRES 0x%X\n", hres);
						}
					}
				}
			}
		}

		virtual void deleteDeviceObjects(void)
		{
			D3D::safeRelease(mpCubeTexture);
		}

	protected:
		LPDIRECT3DCUBETEXTURE9 mpCubeTexture;
	};

	class D3DTextureFactory : public TextureFactory
	{
	public:
		D3DTextureFactory()
		{
		}

		virtual ~D3DTextureFactory()
		{
			flush();
		}

		virtual void flush(void)
		{
			Utils::DeletePointerVector(mpTextures);
			mpTextures.clear();
		}
				
		virtual TextureProxy* create(const char* pName)
		{
			const char* pExtensions[] = 
			{
				"DDS",
				"TGA",
				"JPG",
				"BMP",
				"HDR",
				"PNG"
			};
			const int NumExtensions = sizeof(pExtensions)/sizeof(pExtensions[0]);
			
			TextureProxy* pTex = NULL;
			for (int extIndex = 0; extIndex < NumExtensions; extIndex++)
			{
				const BigString filename((BigString(pName) + "." + pExtensions[extIndex]).tolower());
				
				pTex = createTextureFromFile(filename);
				if (pTex)
					break;
			}
			
			return pTex;
		}
   
		virtual void tick(float deltaT)
		{
			for (int i = 0; i < mpTextures.size(); i++)
				mpTextures[i]->tick(deltaT);
		}

		virtual void oneTimeSceneInit(void)					
		{
			for (int i = 0; i < mpTextures.size(); i++)
				mpTextures[i]->oneTimeSceneInit();
		}

		virtual void initDeviceObjects(void)
		{
			for (int i = 0; i < mpTextures.size(); i++)
				mpTextures[i]->initDeviceObjects();
		}

		virtual void deleteDeviceObjects(void)
		{
			for (int i = 0; i < mpTextures.size(); i++)
				mpTextures[i]->deleteDeviceObjects();
		}

		virtual void invalidateDeviceObjects(void)
		{
			for (int i = 0; i < mpTextures.size(); i++)
				mpTextures[i]->invalidateDeviceObjects();
		}

		virtual void restoreDeviceObjects(void)
		{
			for (int i = 0; i < mpTextures.size(); i++)
				mpTextures[i]->restoreDeviceObjects();
		}

		virtual void startOfFrame(void)						
		{
			for (int i = 0; i < mpTextures.size(); i++)
				mpTextures[i]->startOfFrame();
		}

	protected:
		std::vector<TextureProxy*> mpTextures;
		
		TextureProxy* createTextureFromFile(const BigString& filename)
		{
			for (int i = 0; i < mpTextures.size(); i++)
				if (filename == mpTextures[i]->filename())
					return mpTextures[i];

			TextureProxy* pTex = new D3DCubeTextureProxy(filename);

			if (!pTex->get())
			{
				delete pTex;
				
				pTex = new D3DTextureProxy(filename);
				if (!pTex->get())
				{
					delete pTex;

					Status("D3DTextureFactory::create: Unable to find texture \"%s\"\n", filename.c_str());
					return NULL;
				}
				else
					Status("D3DTextureFactory::create: Loaded 2D texture \"%s\"\n", filename.c_str());
			}
			else
				Status("D3DTextureFactory::create: Loaded cubemap \"%s\"\n", filename.c_str());

			mpTextures.push_back(pTex);
		
			return pTex;
		}
	};

} // namespace gr
	
#endif // D3D_TEXTURE_FACTORY_H
