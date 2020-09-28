//-----------------------------------------------------------------------------
// File: matrix_tracker.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "matrix_tracker.h"
#include "common/utils/utils.h"
#include "device.h"

namespace gr
{
	namespace 
	{
		// Effect Matrix Names
		struct 
		{
			const char* pName;
			EMatrix eMatrix;
		} gMatrixNames[] = 
		{
			{ "gModelToWorld",	eModelToWorld },
			{ "gWorldToView",		eWorldToView  },
			{ "gViewToProj",		eViewToProj   },
			{ "gProjToScreen",	eProjToScreen },
			{	"gViewToWorld",		eViewToWorld  },
			{	"gScreenToProj",	eScreenToProj },
			{ "gModelToView",		eModelToView  },
			{ "gWorldToProj",		eWorldToProj  },
			{	"gModelToProj",		eModelToProj  },
			{ "gScreenToView",	eScreenToView }
		};
		const int NumMatrixNames = sizeof(gMatrixNames) / sizeof(gMatrixNames[0]);
	} // anonymous namespace

	MatrixTracker::MatrixTracker()
	{
		clear();
	}

	MatrixTracker::~MatrixTracker()
	{
	}

	MatrixTracker::MatrixTracker(const MatrixTracker& rhs)
	{
		*this = rhs;
	}

	void MatrixTracker::setMatrices(const MatrixTracker& rhs)
	{
		for (int i = 0; i < eNumMatrices; i++)
		{
			mMatrices[i][0] = rhs.mMatrices[i][0];
			mMatrices[i][1] = rhs.mMatrices[i][1];
		}
	}

	MatrixTracker& MatrixTracker::operator= (const MatrixTracker& rhs)
	{
		for (int i = 0; i < eNumMatrices; i++)
		{
			mMatrices[i][0] = rhs.mMatrices[i][0];
			mMatrices[i][1] = rhs.mMatrices[i][1];
		}
	
		return *this;
	}

	void MatrixTracker::clear(void)
	{
		for (int i = 0; i < eNumMatrices; i++)
			mMatrices[i][0] = mMatrices[i][1] = Matrix44::I;
	}

	void MatrixTracker::setMatrixPair(EMatrix matrixIndex, const Matrix44& m)
	{
		mMatrices[DebugRange<int>(matrixIndex, eNumMatrices)][0] = m;
		mMatrices[matrixIndex][1] = m.transposed();
	}

	const Matrix44& MatrixTracker::getMatrix(EMatrix matrixIndex, bool transposed) const
	{
		return mMatrices[DebugRange<int>(matrixIndex, eNumMatrices)][transposed];
	}

	const char* MatrixTracker::getMatrixEffectName(EMatrix matrixIndex) const
	{
		return gMatrixNames[DebugRange<int>(matrixIndex, eNumMatrices)].pName;
	}

	Matrix44 MatrixTracker::concatMatrices(EMatrix a, EMatrix b, bool aTransposed, bool bTransposed)
	{
		return getMatrix(a, aTransposed) * getMatrix(b, bTransposed);
	}

	void MatrixTracker::updateDirtyBasicMatrix(EMatrix matrix)
	{
		//eModelToView,
		//eWorldToProj,
		//eModelToProj,
		//eScreenToView

		// eViewToWorld,
		// eScreenToProj

		switch (matrix)
		{
			case eModelToWorld:
			{
				setMatrixPair(eModelToView, concatMatrices(eModelToWorld, eWorldToView));
				setMatrixPair(eModelToProj, concatMatrices(eModelToWorld, eWorldToProj));
			
				break;
			}
			case eWorldToView:
			{
				setMatrixPair(eViewToWorld, getMatrix(eWorldToView).inverseSlow());

				setMatrixPair(eModelToView, concatMatrices(eModelToWorld, eWorldToView));
				setMatrixPair(eWorldToProj, concatMatrices(eWorldToView, eViewToProj));
				setMatrixPair(eModelToProj, concatMatrices(eModelToWorld, eWorldToProj));
				
				break;
			}
			case eViewToProj:
			{
				setMatrixPair(eWorldToProj, concatMatrices(eWorldToView, eViewToProj));
				setMatrixPair(eModelToProj, concatMatrices(eModelToWorld, eWorldToProj));
				
				// screenToProj * projToView
				Matrix44 screenToProj(getMatrix(eProjToScreen).inverseSlow());
				Matrix44 projToView(getMatrix(eViewToProj).inverseSlow());
				setMatrixPair(eScreenToView, screenToProj * projToView);
								
				break;
			}
			case eProjToScreen:
			{
				setMatrixPair(eScreenToProj, getMatrix(eProjToScreen).inverseSlow());

				// screenToProj * projToView
				setMatrixPair(eScreenToView, getMatrix(eProjToScreen).inverseSlow() * getMatrix(eViewToProj).inverseSlow());
				
				break;
			}
			default:
				Verify(false);
		}
	}

	void MatrixTracker::setProjToScreenMatrix(int x, int y, int width, int height, float minZ, float maxZ)
	{
		Matrix44 projToScreen;
		projToScreen.setColumn(0, Vec4(width * .5f, 0, 0, x + width * .5f));
		projToScreen.setColumn(1, Vec4(0.0f, height * -.5f, 0, y + height * .5f));
		projToScreen.setColumn(2, Vec4(0.0f, 0.0f, maxZ - minZ, minZ));
		projToScreen.setColumn(3, Vec4(0,0,0,1));

		setMatrixPair(eProjToScreen, projToScreen);
		
		updateDirtyBasicMatrix(eProjToScreen);
	}
	  
	void MatrixTracker::setMatrix(EMatrix basicMatrixIndex, const Matrix44& m)
	{
		setMatrixPair(DebugRange<EMatrix>(basicMatrixIndex, eNumBasicMatrices), m);
		updateDirtyBasicMatrix(basicMatrixIndex);
	}
		
  Vec3 MatrixTracker::getViewVector(const Vec2& screen) const
	{
		return (Vec4(screen[0], screen[1], 0.0f, 1.0f) * getMatrix(eScreenToView)).project().toVector().normalize();
	}
	
	void MatrixTracker::mirrorMatricesToDevice(void) const
	{
		D3D::setTransformUntracked(D3DTS_WORLD, getMatrix(eModelToWorld));
		D3D::setTransformUntracked(D3DTS_VIEW, getMatrix(eWorldToView));
		D3D::setTransformUntracked(D3DTS_PROJECTION, getMatrix(eViewToProj));
	}
	
	Vec4 MatrixTracker::getViewToProjMul(void) const
	{
		const Matrix44& viewToProj = getMatrix(eViewToProj);
		
		return Vec4(viewToProj[0][0], viewToProj[1][1], viewToProj[2][2], viewToProj[2][3]);
	}
	
	Vec4 MatrixTracker::getViewToProjAdd(void) const
	{
		const Matrix44& viewToProj = getMatrix(eViewToProj);
		return Vec4(viewToProj[3][0], viewToProj[3][1], viewToProj[3][2], viewToProj[3][3]);
	}

} // namespace gr

