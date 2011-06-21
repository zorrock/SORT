/*
 * filename :	intersection.h
 *
 * programmer :	Cao Jiayin
 */

#ifndef	SORT_INTERSECTION
#define	SORT_INTERSECTION

// include the header
#include "point.h"

///////////////////////////////////////////////////////////////////////
//	defination of intersection
class	Intersection
{
// public method
public :
	// default constructor
	Intersection();
	// destructor
	~Intersection();

// public field
public:
	// the interesection point
	Point	intersect;
	// the normal
	Vector	normal;
	// the uv coordinate
	float	u , v;
};

#endif