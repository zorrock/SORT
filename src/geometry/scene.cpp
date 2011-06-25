/*
 * filename :	scene.cpp
 *
 * programmer :	Cao Jiayin
 */

// include the header
#include "scene.h"
#include "geometry/intersection.h"
#include "accel/accelerator.h"

// initialize default data
void Scene::_init()
{
	m_pAccelerator = 0;
}

// load the scene from script file
bool Scene::LoadScene( const string& str )
{
	// temporary
	TriMesh* mesh = new TriMesh();
	Transform t0 = Translate( Vector( 2 , 0 , 0 ) ) * RotateZ( 1.0f );
	if( mesh->LoadMesh( "../res/cube.obj" , t0 ) )
		m_meshBuf.push_back( mesh );
	else
		delete mesh;

	// create another instance
	mesh = new TriMesh();
	Transform t1 = Translate( Vector( 0 , 1 , 0 ) ) * RotateX( 1.0f ) ;
	if( mesh->LoadMesh( "../res/cube.obj" , t1 ) )
		m_meshBuf.push_back( mesh );
	else
		delete mesh;

	// create another instance
	mesh = new TriMesh();
	Transform t2 = Translate( Vector( 0 , 1 , 0 ) ) * RotateX( -1.0f ) * Translate( Vector( 1 , 1 , 0 ) );
	if( mesh->LoadMesh( "../res/cube.obj" , t2 ) )
		m_meshBuf.push_back( mesh );
	else
		delete mesh;

	// generate triangle buffer after parsing from file
	_generateTriBuf();

	return true;
}

// get the intersection between a ray and the scene
bool Scene::GetIntersect( const Ray& r , Intersection* intersect ) const
{
	// brute force intersection test if there is no accelerator
	if( m_pAccelerator == 0 )
		return _bfIntersect( r , intersect );

	return m_pAccelerator->GetIntersect( r , intersect );
}

// get the intersection between a ray and the scene 
bool Scene::_bfIntersect( const Ray& r , Intersection* intersect ) const
{
	bool bInter = false;
	float t = FLT_MAX;
	int n = (int)m_triBuf.size();
	for( int k = 0 ; k < n ; k++ )
	{
		Intersection in;
		const BBox box = m_triBuf[k]->GetBBox();
		
		if( m_triBuf[k]->GetIntersect( r , &in ) && in.t < t )
		{
			bInter = true;
			t = in.t;
			*intersect = in;
		}
	}

	return bInter;
}

// release the memory of the scene
void Scene::Release()
{
	SAFE_DELETE( m_pAccelerator );

	vector<Primitive*>::iterator it = m_triBuf.begin();
	while( it != m_triBuf.end() )
	{
		delete *it;
		it++;
	}
	m_triBuf.clear();

	vector<TriMesh*>::iterator tri_it = m_meshBuf.begin();
	while( tri_it != m_meshBuf.end() )
	{
		delete *tri_it;
		tri_it++;
	}
	m_meshBuf.clear();
}

// generate triangle buffer
void Scene::_generateTriBuf()
{
	// iterator the mesh
	vector<TriMesh*>::iterator it = m_meshBuf.begin();
	while( it != m_meshBuf.end() )
	{
		(*it)->FillTriBuf( m_triBuf );
		it++;
	}
}
