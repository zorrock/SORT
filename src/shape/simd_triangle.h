/*
    This file is a part of SORT(Simple Open Ray Tracing), an open-source cross
    platform physically based renderer.

    Copyright (c) 2011-2019 by Jiayin Cao - All rights reserved.

    SORT is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

#pragma once

#include "core/define.h"
#include "math/ray.h"
#include "math/intersection.h"
#include "math/point.h"

#ifdef SSE_ENABLED
    #include <nmmintrin.h>
#endif

#ifdef  SSE_ENABLED

//! @brief  Triangle4 is more of a simplified resolved data structure holds only bare bone information of triangle.
/**
 * Triangle4 is used in QBVH to accelerate ray triangle intersection using SSE. Its sole purpose is to accelerate 
 * ray triangle intersection by using SSE. Meaning there is no need to provide sophisticated interface of the class.
 * And since it is quite performance sensitive code, everything is inlined and there is no polymorphisms to keep it
 * as simple as possible. However, since there will be extra data kept in the system, it will also insignificantly 
 * incur more cost in term of memory usage.
 */
struct Triangle4{
    __m128  m_p0_x , m_p0_y , m_p0_z ;  /**< Position of point 0 of the triangle. */
    __m128  m_p1_x , m_p1_y , m_p1_z ;  /**< Position of point 1 of the triangle. */
    __m128  m_p2_x , m_p2_y , m_p2_z ;  /**< Position of point 2 of the triangle. */
    __m128  m_mask;

    /**< Pointers to original primitives. */
    const Triangle*  m_ori_pri[4] = { nullptr };

    //! @brief  Push a triangle in the data structure.
    //!
    //! @param  tri     The triangle to be pushed in Triangle4.
    //! @return         Whether the data structure is full.
    bool PushTriangle( const Triangle* tri ){
        if( m_ori_pri[0] == nullptr ){
            m_ori_pri[0] = tri;
            return false;
        }else if( m_ori_pri[1] == nullptr ){
            m_ori_pri[1] = tri;
            return false;
        }else if( m_ori_pri[2] == nullptr ){
            m_ori_pri[2] = tri;
            return false;
        }
        m_ori_pri[3] = tri;
        return true;
    }

    //! @brief  Pack triangle information into sse compatible data.
    void PackData(){
        unsigned int mask[4] = {0};
        float   p0_x[4] , p0_y[4] , p0_z[4] , p1_x[4] , p1_y[4] , p1_z[4] , p2_x[4] , p2_y[4] , p2_z[4];
        for( auto i = 0 ; i < 4 && m_ori_pri[i] ; ++i ){
            const Triangle* triangle = m_ori_pri[i];

            const auto& mem = triangle->GetMeshVisual()->m_memory;
            const auto id0 = triangle->GetIndices().m_id[0];
            const auto id1 = triangle->GetIndices().m_id[1];
            const auto id2 = triangle->GetIndices().m_id[2];

            const auto& mv0 = mem->m_vertices[id0];
            const auto& mv1 = mem->m_vertices[id1];
            const auto& mv2 = mem->m_vertices[id2];

            p0_x[i] = mv0.m_position.x;
            p0_y[i] = mv0.m_position.y;
            p0_z[i] = mv0.m_position.z;

            p1_x[i] = mv1.m_position.x;
            p1_y[i] = mv1.m_position.y;
            p1_z[i] = mv1.m_position.z;

            p2_x[i] = mv2.m_position.x;
            p2_y[i] = mv2.m_position.y;
            p2_z[i] = mv2.m_position.z;

            mask[i] = 0xffffffff;
        }

        m_p0_x = _mm_set_ps( p0_x[3] , p0_x[2] , p0_x[1] , p0_x[0] );
        m_p0_y = _mm_set_ps( p0_y[3] , p0_y[2] , p0_y[1] , p0_y[0] );
        m_p0_z = _mm_set_ps( p0_z[3] , p0_z[2] , p0_z[1] , p0_z[0] );
        m_p1_x = _mm_set_ps( p1_x[3] , p1_x[2] , p1_x[1] , p1_x[0] );
        m_p1_y = _mm_set_ps( p1_y[3] , p1_y[2] , p1_y[1] , p1_y[0] );
        m_p1_z = _mm_set_ps( p1_z[3] , p1_z[2] , p1_z[1] , p1_z[0] );
        m_p2_x = _mm_set_ps( p2_x[3] , p2_x[2] , p2_x[1] , p2_x[0] );
        m_p2_y = _mm_set_ps( p2_y[3] , p2_y[2] , p2_y[1] , p2_y[0] );
        m_p2_z = _mm_set_ps( p2_z[3] , p2_z[2] , p2_z[1] , p2_z[0] );
        m_mask = _mm_set_ps( mask[3] , mask[2] , mask[1] , mask[0] );
    }

    //! @brief  Reset the data for reuse
    void Reset(){
        m_ori_pri[0] = m_ori_pri[1] = m_ori_pri[2] = m_ori_pri[3] = nullptr;
    }
};

//! @brief  With the power of SSE, this utility function helps intersect a ray with four triangles at the cost of one.
//!
//! @param  ray     Ray to be tested against.
//! @param  tri4    Data structure holds four triangles.
//! @param  mask    Mask results that are not valid.
//! @param  ret     The result of intersection.
SORT_FORCEINLINE bool intersectTriangle4( const Ray& ray , const Triangle4& tri4 , Intersection* ret ){
    static const __m128 zeros = _mm_set_ps1( 0.0f );

    __m128i mask = tri4.m_mask;

	// step 0 : translate the vertices to ray coordinate system
	__m128 p0[3] , p1[3] , p2[3];
    p0[0] = _mm_sub_ps(tri4.m_p0_x, ray.m_ori_x);
	p0[1] = _mm_sub_ps(tri4.m_p0_y, ray.m_ori_y);
	p0[2] = _mm_sub_ps(tri4.m_p0_z, ray.m_ori_z);

	p1[0] = _mm_sub_ps(tri4.m_p1_x, ray.m_ori_x);
	p1[1] = _mm_sub_ps(tri4.m_p1_y, ray.m_ori_y);
	p1[2] = _mm_sub_ps(tri4.m_p1_z, ray.m_ori_z);

	p2[0] = _mm_sub_ps(tri4.m_p2_x, ray.m_ori_x);
	p2[1] = _mm_sub_ps(tri4.m_p2_y, ray.m_ori_y);
	p2[2] = _mm_sub_ps(tri4.m_p2_z, ray.m_ori_z);

	// step 1 : pick the major axis to avoid dividing by zero in the sheering pass.
	//          by picking the major axis, we can also make sure we sheer as little as possible
	__m128 p0_x = p0[ray.m_local_x];
	__m128 p0_y = p0[ray.m_local_y];
	__m128 p0_z = p0[ray.m_local_z];

	__m128 p1_x = p1[ray.m_local_x];
	__m128 p1_y = p1[ray.m_local_y];
	__m128 p1_z = p1[ray.m_local_z];

	__m128 p2_x = p1[ray.m_local_x];
	__m128 p2_y = p1[ray.m_local_y];
	__m128 p2_z = p1[ray.m_local_z];

	// step 2 : sheer the vertices so that the ray direction points to ( 0 , 1 , 0 )
	p0_x = _mm_add_ps(p0_x, _mm_mul_ps(p0_y, ray.m_sse_scale_x));
	p0_z = _mm_add_ps(p0_z, _mm_mul_ps(p0_y, ray.m_sse_scale_z));
	p1_x = _mm_add_ps(p1_x, _mm_mul_ps(p1_y, ray.m_sse_scale_x));
	p1_z = _mm_add_ps(p1_z, _mm_mul_ps(p1_y, ray.m_sse_scale_z));
	p2_x = _mm_add_ps(p2_x, _mm_mul_ps(p2_y, ray.m_sse_scale_x));
	p2_z = _mm_add_ps(p2_z, _mm_mul_ps(p2_y, ray.m_sse_scale_z));

	// compute the edge functions
	__m128 e0 = _mm_sub_ps( _mm_mul_ps( p1_x , p2_z ) , _mm_mul_ps( p1_z , p2_x ) );
	__m128 e1 = _mm_sub_ps( _mm_mul_ps( p2_x , p0_z ) , _mm_mul_ps( p2_z , p0_x ) );
	__m128 e2 = _mm_sub_ps( _mm_mul_ps( p0_x , p1_z ) , _mm_mul_ps( p0_z , p1_x ) );

    __m128 c0 = _mm_and_ps( _mm_and_ps( _mm_cmpge_ps( e0 , zeros ) , _mm_cmpge_ps( e1 , zeros ) ) , _mm_cmpge_ps( e2 , zeros ) );
    __m128 c1 = _mm_and_ps( _mm_and_ps( _mm_cmple_ps( e0 , zeros ) , _mm_cmple_ps( e1 , zeros ) ) , _mm_cmple_ps( e2 , zeros ) );
    mask = _mm_and_ps( mask , _mm_and_ps( c0 , c1 ) );
    auto c = _mm_movemask_ps( mask );
    if( 0 == c )
        return false;

    __m128 det = _mm_add_ps( e0 , _mm_add_ps( e1 , e2 ) );
    mask = _mm_and_ps( mask , _mm_cmpneq_ps( det , zeros ) );
    c = _mm_movemask_ps( mask );
    if( 0 == c )
        return false;
    __m128 rcp_det = _mm_rcp_ps( det );

    p0_y = _mm_add_ps( p0_y , ray.m_sse_scale_y );
    p1_y = _mm_add_ps( p1_y , ray.m_sse_scale_y );
    p2_y = _mm_add_ps( p2_y , ray.m_sse_scale_y );

    __m128 t = _mm_add_ps( e0 , p0_y );
    t = _mm_add_ps( t , _mm_mul_ps( e1 , p1_y ) );
    t = _mm_add_ps( t , _mm_mul_ps( e2 , p2_y ) );
    t = _mm_mul_ps( t , rcp_det );

    __m128 ray_min_t = _mm_set_ps1( ray.m_fMin );
    __m128 ray_max_t = _mm_set_ps1( ray.m_fMax );
    mask = _mm_and_ps( _mm_and_ps( mask , _mm_cmpgt_ps( t , ray_min_t ) ) , _mm_cmplt_ps( t , ray_max_t ) );
    c = _mm_movemask_ps( mask );
    if( 0 == c )
        return false;
    
    if( nullptr == ret )
        return true;

    mask = _mm_and_ps( _mm_and_ps( mask , _mm_cmpgt_ps( t , zeros ) ) , _mm_cmple_ps( t , _mm_set_ps1( ret->t ) ) );
    c = _mm_movemask_ps( mask );
    if( 0 == c )
        return false;
    
    // resolve the result, it could be optimized with SIMD later.
    alignas(16) float f_t[4] , f_e1[4] , f_e2[4] , f_rcp_det[4];
    _mm_store_ps( f_t , t );
    _mm_store_ps( f_e1 , e1 );
    _mm_store_ps( f_e2 , e2 );
    _mm_store_ps( f_rcp_det , rcp_det );

    alignas(16) unsigned int b_mask[4];
    _mm_store_ps( (float*)b_mask , mask );

    int     res_i = -1;
    float   res_t = -1.0f;
    if( b_mask[0] && res_t > f_t[0] ){
        res_i = 0;
        res_t = f_t[0];
    }
    if( b_mask[1] && res_t > f_t[1] ){
        res_i = 1;
        res_t = f_t[1];
    }
    if( b_mask[2] && res_t > f_t[2] ){
        res_i = 2;
        res_t = f_t[2];
    }
    if( b_mask[3] && res_t > f_t[3] ){
        res_i = 3;
        res_t = f_t[3];
    }
    
    const auto* triangle = tri4.m_ori_pri[res_i];

    const auto u = f_e1[res_i] * f_rcp_det[res_i];
    const auto v = f_e2[res_i] * f_rcp_det[res_i];
    const auto w = 1 - u - v;

    // store the intersection
    ret->intersect = ray(res_t);

    const auto& mem = triangle->GetMeshVisual()->m_memory;
    const auto id0 = triangle->GetIndices().m_id[0];
    const auto id1 = triangle->GetIndices().m_id[1];
    const auto id2 = triangle->GetIndices().m_id[2];

    const auto& mv0 = mem->m_vertices[id0];
    const auto& mv1 = mem->m_vertices[id1];
    const auto& mv2 = mem->m_vertices[id2];

    // get three vertexes
    ret->gnormal = Normalize(Cross( ( mv2.m_position - mv0.m_position ) , ( mv1.m_position - mv0.m_position ) ));
    ret->normal = ( w * mv0.m_normal + u * mv1.m_normal + v * mv2.m_normal).Normalize();
    ret->tangent = ( w * mv0.m_tangent + u * mv1.m_tangent + v * mv2.m_tangent).Normalize();
    ret->view = -ray.m_Dir;

    const auto uv = w * mv0.m_texCoord + u * mv1.m_texCoord + v * mv2.m_texCoord;
    ret->u = uv.x;
    ret->v = uv.y;
    ret->t = res_t;

    return true;
}

#endif