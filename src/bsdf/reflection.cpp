/*
 * filename :	reflection.cpp
 *
 * programmer :	Cao Jiayin
 */

// include the header file
#include "reflection.h"
#include "geometry/vector.h"
#include "bsdf/bsdf.h"

// evaluate bxdf
Spectrum Reflection::f( const Vector& wo , const Vector& wi ) const
{
	return Spectrum();
}

// sample a direction randomly
Spectrum Reflection::sample_f( const Vector& wo , Vector& wi , float* pdf ) const
{
	wi = Vector( -wo.x , wo.y , -wo.z );

	return m_fresnel->Evaluate( CosTheta(wi) , CosTheta(wo) ) / AbsCosTheta( wi );
}
