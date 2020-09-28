//-----------------------------------------------------------------------------
// File: matrix_tracker.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef MATRIX_TRACKER_H
#define MATRIX_TRACKER_H
#pragma once

#include "common/math/vector.h"
#include <d3d9.h>

namespace gr
{
	enum EMatrix
	{
		eInvalidMatrix = -1,

		eModelToWorld,
		eWorldToView,
		eViewToProj,
		eProjToScreen,
		
		eViewToWorld,
		eScreenToProj,

		eNumBasicMatrices,

		eModelToView = eNumBasicMatrices,
		eWorldToProj,
		eModelToProj,
		eScreenToView,

    eNumMatrices
	};

	class MatrixTracker
	{
	public:
		MatrixTracker(void);
		virtual ~MatrixTracker();

		MatrixTracker(const MatrixTracker& rhs);
		MatrixTracker& operator= (const MatrixTracker& rhs);

		void setMatrices(const MatrixTracker& rhs);
			
		void setMatrix(EMatrix basicMatrixIndex, const Matrix44& m);
		
		void setProjToScreenMatrix(int x, int y, int width, int height, float minZ = 0.0f, float maxZ = 1.0f);
		
		void setProjToScreenMatrix(const D3DVIEWPORT9& viewport)
		{
			setProjToScreenMatrix(viewport.X, viewport.Y, viewport.Width, viewport.Height, viewport.MinZ, viewport.MaxZ);
		}

		const Matrix44& getMatrix(EMatrix matrixIndex, bool transposed = false) const;
				
		const char* getMatrixEffectName(EMatrix matrixIndex) const;

    Vec3 getViewVector(const Vec2& screen) const;
    
    Vec4 getViewToProjMul(void) const;
    Vec4 getViewToProjAdd(void) const;
		
		void clear(void);

		void mirrorMatricesToDevice(void) const;
				
	protected:
		Matrix44 mMatrices[eNumMatrices][2];           
		
		void clearMatrices(void);
		void setMatrixPair(EMatrix matrixIndex, const Matrix44& m);
		
		Matrix44 concatMatrices(EMatrix a, EMatrix b, bool aTransposed = false, bool bTransposed = false);
		void updateDirtyBasicMatrix(EMatrix matrix);
		void updateProjToScreenMatrix(void);
	};

} // namespace gr

#endif // MATRIX_TRACKER_H
