// File: spherical_harmonic.h
#pragma once
#ifndef SPHERICAL_HARMONIC
#define SPHERICAL_HARMONIC

#include "cubemap.h"

namespace gr
{
	struct SphericalHarmonic
	{
		typedef std::vector< Vec<9> > Vec9Vector;
		
		// Returns coefficient array index given l and m:
		// l [0,2]
		// m [-l, l]
		static int index(int l, int m)
		{
			DebugRangeIncl(l, 2);
			DebugRangeIncl(m, -l, l);
			return DebugRange(l * (l + 1) + m + 1 - 1, 9);
		}
				
		// Evaluates the SH basis given a unitized direction.
		static Vec<9> evaluate(const Vec3& v) 
		{
			float y00 = .282095f;
			float y11 = .488603f * -v[0];
			float y10 = .488603f * v[2];
			float y1m1 = .488603f * -v[1];

			float y2m2 = 1.092548f * v[0] * v[1];
			float y2m1 = 1.092548f * -v[1] * v[2];
			float y21 = 1.092548f * -v[0] * v[2];
			float y20 = .315392f * (3.0f * v[2] * v[2] - 1.0f);
			float y22 = .546274f * (v[0] * v[0] - v[1] * v[1]);
				
			Vec<9> ret;
			ret[index(0, 0)] = y00;
			ret[index(1, 1)] = y11;
			ret[index(1, 0)] = y10;
			ret[index(1,-1)] = y1m1;
			ret[index(2,-2)] = y2m2;
			ret[index(2,-1)] = y2m1;
			ret[index(2,1)] = y21;
			ret[index(2,0)] = y20;
			ret[index(2,2)] = y22;
		
			// Same as: order 3, degree 2, 3^2=9 coeffs	
			//float x[9];
			//D3DXSHEvalDirection(x, 3, &D3DXVECTOR3(v[0], v[1], v[2]));
			
			return ret;
		}

	#if 0	
		// Projects function into the SH basis.
		static Vec9Vector projectD3DX(IDirect3DCubeTexture9* pCubeTex)
		{	
			float c[3][9];
				
			Utils::ClearObj(c);
			Verify(SUCCEEDED(D3DXSHProjectCubeMap(3, pCubeTex, c[0], c[1], c[2])));

			printf("SphericalHarmonic::projectD3DX:\n");		
			for (int comp = 0; comp < 3; comp++)
			{
				for (int x = 0; x < 9; x++)
					printf("%f ", c[comp][x]);
				printf("\n");
			}
			
			Vec9Vector ret(3);
			for (int comp = 0; comp < 3; comp++)
				for (int x = 0; x < 9; x++)
					ret[comp][x] = c[comp][x];
			
			return ret;
		}
	#endif	
		
		// Projects function into the SH basis.
		static Vec9Vector project(const CubeMap& cubemap)
		{
			Vec<9> coeff[3];
			Utils::ClearObj(coeff);
			
			double areaSum = 0;
			
			for (int f = 0; f < 6; f++)
			{
				for (int y = 0; y < cubemap.height(); y++)
				{
					for (int x = 0; x < cubemap.width(); x++)
					{
						const Vec3& sample = cubemap.pixel(f, x, y);
						const Vec3& dir = cubemap.vector(f, x, y);
						const double area = cubemap.sphericalArea(f, x, y);
						
						areaSum += area;
						
						Vec<9> f(SphericalHarmonic::evaluate(dir));
						
						for (int c = 0; c < 3; c++)
							coeff[c] += f * area * sample[c];
					}
				}
			}
			
			coeff[0] *= (Math::fPi * 4.0f) / areaSum;
			coeff[1] *= (Math::fPi * 4.0f) / areaSum;
			coeff[2] *= (Math::fPi * 4.0f) / areaSum;
			
			printf("SphericalHarmonic::project:\n");
			for (int comp = 0; comp < 3; comp++)
			{
				for (int x = 0; x < 9; x++)
					printf("%f ", coeff[comp][x]);
				printf("\n");
			}
		
			Vec9Vector ret(3);
			ret[0] = coeff[0];	
			ret[1] = coeff[1];	
			ret[2] = coeff[2];	
			return ret;
		}
				
		static Vec3 computeIrradSlow(const Vec9Vector & coeff, const Vec3& dir)
		{
			const Vec<9> e(evaluate(dir));
						
			// three, nine dimensional dot products
			Vec3 ret;
			for (int comp = 0; comp < 3; comp++)
				ret[comp] = e * coeff[comp];
				
			return ret;
		}
		
		struct ComputeIrradState
		{
			Vec<4> cAr;
			Vec<4> cAg;	
			Vec<4> cAb;
			Vec<4> cBr;
			Vec<4> cBg;
			Vec<4> cBb;
			Vec<3> cC;
			
			ComputeIrradState(const Vec9Vector & coeff)
			{
				const float n0 = 1.0f/(2.0f*sqrt(Math::fPi));
				const float n1 = sqrt(3.0f)/(2.0f*sqrt(Math::fPi));
				const float n2 = sqrt(15.0f)/(2.0f*sqrt(Math::fPi));
				const float n3 = sqrt(5.0f)/(4.0f*sqrt(Math::fPi));
				const float n4 = sqrt(5.0f)/(4.0f*sqrt(Math::fPi));
					
				const float h0 = 1.0f; // unused, but present for clarity (actually Pi/Pi)
				const float h1 = 2.0f/3.0f;
				const float h2 = 1.0f/4.0f;
								
				const float c0 = n0;
				const float c1 = h1*n1;
				const float c2 = h2*n2;
				const float c3 = h2*n4;
				const float c4 = c2*.5f;
				
				const float fC0 = 1.0f/(2.0f*sqrt(Math::fPi));
				const float fC1 = (float)sqrt(3.0f)/(3.0f*sqrt(Math::fPi));
				const float fC2 = (float)sqrt(15.0f)/(8.0f*sqrt(Math::fPi));
				const float fC3 = (float)sqrt(5.0f)/(16.0f*sqrt(Math::fPi));
				const float fC4 = 0.5f*fC2;
								
				cAr = Vec<4>(
					-c1*coeff[0][index(1,1)],
					-c1*coeff[0][index(1,-1)],
					c1*coeff[0][index(1,0)],
					c0*coeff[0][index(0,0)] - c3 * coeff[0][index(2,0)]
					);
					
				cAg = Vec<4>(
					-c1*coeff[1][index(1,1)],
					-c1*coeff[1][index(1,-1)],
					c1*coeff[1][index(1,0)],
					c0*coeff[1][index(0,0)] - c3 * coeff[1][index(2,0)]
					);
					
				cAb = Vec<4>(
					-c1*coeff[2][index(1,1)],
					-c1*coeff[2][index(1,-1)],
					c1*coeff[2][index(1,0)],
					c0*coeff[2][index(0,0)] - c3 * coeff[2][index(2,0)]
					);
					
				cBr = Vec<4>(
					c2*coeff[0][index(2,-2)],
					-c2*coeff[0][index(2,-1)],
					3.0f*c3*coeff[0][index(2,0)],
					-c2*coeff[0][index(2,1)]
					);	
					
				cBg = Vec<4>(
					c2*coeff[1][index(2,-2)],
					-c2*coeff[1][index(2,-1)],
					3.0f*c3*coeff[1][index(2,0)],
					-c2*coeff[1][index(2,1)]
					);				
					
				cBb = Vec<4>(
					c2*coeff[2][index(2,-2)],
					-c2*coeff[2][index(2,-1)],
					3.0f*c3*coeff[2][index(2,0)],
					-c2*coeff[2][index(2,1)]
					);
					
				cC = Vec<3>(
					c4*coeff[0][index(2,2)],
					c4*coeff[1][index(2,2)],
					c4*coeff[2][index(2,2)]
					);
			}
		};
		
		// Intended for vertex/pixel shader implementation.
		static Vec3 computeIrradFast(const ComputeIrradState& state, const Vec3& dir)
		{
			Vec<4> normal(dir, 1.0f);
			
			Vec<3> linearConstantColor;
			linearConstantColor[0] = normal * state.cAr;
			linearConstantColor[1] = normal * state.cAg;
			linearConstantColor[2] = normal * state.cAb;
			
			Vec<4> r2(Vec<4>::multiply(Vec<4>(normal[0], normal[1], normal[2], normal[2]), Vec<4>(normal[1], normal[2], normal[2], normal[0])));
			
			Vec<3> firstQuadraticColor;
			firstQuadraticColor[0] = r2 * state.cBr;
			firstQuadraticColor[1] = r2 * state.cBg;
			firstQuadraticColor[2] = r2 * state.cBb;
			
			normal[0] = normal[0] * normal[0];
			normal[1] = normal[1] * normal[1];
			normal[0] = normal[0] - normal[1];
			
			Vec<3> finalQuadraticColor = state.cC * normal[0];
						
			return linearConstantColor + firstQuadraticColor + finalQuadraticColor;
		}
				
		static Vec9Vector cosineConvolution(const Vec9Vector & coeff)
		{
			// convolve input against cosine lobe
			
			Vec9Vector irradCoeff(3);
			irradCoeff[0] = Vec<9>::multiply(coeff[0], SphericalHarmonic::irradConvolutionCoefficients());
			irradCoeff[1] = Vec<9>::multiply(coeff[1], SphericalHarmonic::irradConvolutionCoefficients());
			irradCoeff[2] = Vec<9>::multiply(coeff[2], SphericalHarmonic::irradConvolutionCoefficients());
			
			return irradCoeff;
		}
		
		static unprojectRaw(CubeMap& dst, const Vec9Vector & coeff)
		{
			for (int f = 0; f < 6; f++)
			{
				for (int y = 0; y < dst.height(); y++)
				{
					for (int x = 0; x < dst.width(); x++)
					{
						const Vec3 dir(dst.vector(f, x, y));
											
						dst.pixel(f, x, y) = computeIrradSlow(coeff, dir);
					}
				}
			}
		}
		
		static unprojectIrrad(CubeMap& dst, const Vec9Vector & coeff)
		{
			ComputeIrradState state(coeff);
			
			for (int f = 0; f < 6; f++)
			{
				for (int y = 0; y < dst.height(); y++)
				{
					for (int x = 0; x < dst.width(); x++)
					{
						const Vec3 dir(dst.vector(f, x, y));
											
						dst.pixel(f, x, y) = computeIrradFast(state, dir);
					}
				}
			}
		}
		
		// Cosine lobe convolution coefficients: convolution(project(cosine_lobe)).
		// Multiply against lighting coefficients to create the irradiance coefficients.
		static Vec<9> irradConvolutionCoefficients(void) 
		{
			Vec<9> ret;
			
			ret[index(0,0)] = 3.141593f;
			ret[index(1,1)] = 2.094395f;
			ret[index(1,0)] = 2.094395f;
			ret[index(1,-1)] = 2.094395f;
			ret[index(2,-2)] = .785398f;
			ret[index(2,-1)] = .785398f;
			ret[index(2,1)] = .785398f;
			ret[index(2,0)] = .785398f;
			ret[index(2,2)] = .785398f;
			
			// Divide by Pi to convert irradiance to radiance (conservation of energy).
			ret /= Math::fPi;
					
			return ret;
		}
		
		// symmetricFunction must be the coefficients of a circularly symmetric function in Z.
		// Returned coefficients ready to be used for convolution purposes.
		static Vec<9> evalConvolution(const Vec<9>& symmetricFunction)
		{
			Vec<9> ret;
			
			ret[0] = sqrt((4.0f * Math::fPi) / (2.0f * 0.0f + 1.0f)) * symmetricFunction[index(0,0)];
			
			ret[1] = sqrt((4.0f * Math::fPi) / (2.0f * 1.0f + 1.0f)) * symmetricFunction[index(1,0)];
			ret[2] = sqrt((4.0f * Math::fPi) / (2.0f * 1.0f + 1.0f)) * symmetricFunction[index(1,0)];
			ret[3] = sqrt((4.0f * Math::fPi) / (2.0f * 1.0f + 1.0f)) * symmetricFunction[index(1,0)];
			
			ret[4] = sqrt((4.0f * Math::fPi) / (2.0f * 2.0f + 1.0f)) * symmetricFunction[index(2,0)];
			ret[5] = sqrt((4.0f * Math::fPi) / (2.0f * 2.0f + 1.0f)) * symmetricFunction[index(2,0)];
			ret[6] = sqrt((4.0f * Math::fPi) / (2.0f * 2.0f + 1.0f)) * symmetricFunction[index(2,0)];
			ret[7] = sqrt((4.0f * Math::fPi) / (2.0f * 2.0f + 1.0f)) * symmetricFunction[index(2,0)];
			ret[8] = sqrt((4.0f * Math::fPi) / (2.0f * 2.0f + 1.0f)) * symmetricFunction[index(2,0)];
				
			return ret;
		}

	#if 0	
			// vertex/pixel normal
			const Vec3 v(0,0,1);

			// project response func. - unshadowed cosine lobe
			CubeMap c(128, 128);
			for (int f = 0; f < 6; f++)
				for (int y = 0; y < 128; y++)
					for (int x = 0; x < 128; x++)
						c.pixel(f, x, y) = Vec3(Math::Max(0.0f, c.vector(f, x, y) * v));
											
			SphericalHarmonic::Vec9Vector ret = SphericalHarmonic::project(c);
			
			Vec3 sample1 = irrad.sample(v);

			// dot response func (ret) and light coefficients to find radiance
			// divide by Pi to convert radiance to irradiance		
			Vec3 sample2(ret[0] * coeff[0] / Math::fPi, ret[1] * coeff[1] / Math::fPi, ret[2] * coeff[2] / Math::fPi);
	#endif

	};

} // namespace gr

#endif // SPHERICAL_HARMONIC
