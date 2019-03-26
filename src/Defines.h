///////////////////////////////////////////////////////////////////////////////
///
///	\file    OverlapMesh.h
///	\author  Paul Ullrich
///	\version March 7, 2014
///
///	<remarks>
///		Copyright 2000-2014 Paul Ullrich
///
///		This file is distributed as part of the Tempest source code package.
///		Permission is granted to use, copy, modify and distribute this
///		source code and its documentation under the terms of the GNU General
///		Public License.  This software is provided "as is" without express
///		or implied warranty.
///	</remarks>

#ifndef _DEFINES_H_
#define _DEFINES_H_

///////////////////////////////////////////////////////////////////////////////

typedef double Real;
typedef Real   REAL;  // for use with triangles.h

///////////////////////////////////////////////////////////////////////////////
//
// Defines for floating point tolerance.
//
static const Real HighTolerance      = 1.0e-10;
static const Real ReferenceTolerance = 1.0e-12;

///////////////////////////////////////////////////////////////////////////////
//
// These defines determine the behavior of GenerateOverlapMesh.
//
// If OVERLAPMESH_RETAIN_REPEATED_NODES is specified this function will make
// no effort to remove repeated nodes during the overlap mesh generation
// calculation.
//
// If OVERLAPMESH_USE_UNSORTED_MAP is specified node removal will use an
// std::unsorted_map() which may nonetheless produce some coincident nodes
// if some very unlikely conditions are met.
//
// If OVERLAPMESH_USE_NODE_MULTIMAP is specified node removal will use the
// node_multimap_3d which is guaranteed to produce no coincident nodes (but
// is the slowest).
//
//#define OVERLAPMESH_RETAIN_REPEATED_NODES
#define OVERLAPMESH_USE_UNSORTED_MAP
//#define OVERLAPMESH_USE_NODE_MULTIMAP

///////////////////////////////////////////////////////////////////////////////
//
// This define specifies the bin width for the std::unsorted_map() and
// node_multimap_3d.
//
#define OVERLAPMESH_BIN_WIDTH 1.0e-1

///////////////////////////////////////////////////////////////////////////////
//
// This define specifies that exact arithmetic should be used in the overlap
// mesh calculation.
//
#define USE_EXACT_ARITHMETIC

///////////////////////////////////////////////////////////////////////////////
//
// Defines required by Triangle package
//
#define ANSI_DECLARATORS
#define VOID int

///////////////////////////////////////////////////////////////////////////////

#endif

