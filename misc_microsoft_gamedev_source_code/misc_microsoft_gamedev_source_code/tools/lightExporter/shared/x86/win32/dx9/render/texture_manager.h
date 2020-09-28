//-----------------------------------------------------------------------------
// File: texture_manager.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "common/utils/utils.h"
#include "common/utils/string.h"

#include <d3d9.h>

namespace gr
{
	class TextureProxy
	{
	public:
		TextureProxy(const BigString& filename) : 
			mFilename(filename)
		{
		}
    
		virtual ~TextureProxy()
		{
		}

		virtual IDirect3DBaseTexture9* get(void) = 0;

		virtual void tick(float deltaT)							{ }
		virtual void oneTimeSceneInit(void)					{	}
		virtual void initDeviceObjects(void)				{ }
		virtual void deleteDeviceObjects(void)			{ }
		virtual void invalidateDeviceObjects(void)	{ }
		virtual void restoreDeviceObjects(void)			{	}
		virtual void startOfFrame(void)							{ }

		const BigString& filename(void) const
		{
			return mFilename;
		}

	protected:
		BigString mFilename;
	};

	class TextureFactory
	{
	public:
		virtual ~TextureFactory()
		{
		}
    		
		virtual TextureProxy* create(const char* pName) = 0;

		virtual void flush(void)										{ };
		virtual void tick(float deltaT)							{ }
		virtual void oneTimeSceneInit(void)					{	}
		virtual void initDeviceObjects(void)				{ }
		virtual void deleteDeviceObjects(void)			{ }
		virtual void invalidateDeviceObjects(void)	{ }
		virtual void restoreDeviceObjects(void)			{	}
		virtual void startOfFrame(void)							{ }
	};

	class TextureManager
	{
	public:
		TextureManager()
		{
		}

		~TextureManager()
		{
			clear();
		}

		// assumes ownership!
		void add(TextureFactory* pFactory)
		{
			mpFactories.push_back(pFactory);
		}

		void clear(void)
		{
			Utils::DeletePointerVector(mpFactories);
			mpFactories.clear();
		}
				
		TextureProxy* create(const char* pName)
		{
			TextureProxy* pRet = NULL;

			for (int i = mpFactories.size() - 1; i >= 0; i--)
			{
				pRet = mpFactories[i]->create(pName);
				if (pRet)
					break;
			}

			return pRet;
		}

		void flush(void)
		{
			for (int i = 0; i < mpFactories.size(); i++)
				mpFactories[i]->flush();
		}

		void tick(float deltaT) 
		{
			for (int i = 0; i < mpFactories.size(); i++)
				mpFactories[i]->tick(deltaT);
		}

		void oneTimeSceneInit(void)
		{
			for (int i = 0; i < mpFactories.size(); i++)
				mpFactories[i]->oneTimeSceneInit();
		}

		void initDeviceObjects(void)
		{
			for (int i = 0; i < mpFactories.size(); i++)
				mpFactories[i]->initDeviceObjects();
		}

		void deleteDeviceObjects(void)
		{
			for (int i = 0; i < mpFactories.size(); i++)
				mpFactories[i]->deleteDeviceObjects();
		}
		void invalidateDeviceObjects(void)
		{
			for (int i = 0; i < mpFactories.size(); i++)
				mpFactories[i]->invalidateDeviceObjects();
		}

		void restoreDeviceObjects(void)
		{
			for (int i = 0; i < mpFactories.size(); i++)
				mpFactories[i]->restoreDeviceObjects();
		}

		void startOfFrame(void)
		{
			for (int i = 0; i < mpFactories.size(); i++)
				mpFactories[i]->startOfFrame();
		}
	
	protected:
		std::vector<TextureFactory*> mpFactories;
	};
	
} // namespace gr

#endif // TEXTURE_MANAGER_H

