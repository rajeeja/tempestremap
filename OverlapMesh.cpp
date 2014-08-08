///////////////////////////////////////////////////////////////////////////////
///
///	\file    OverlapMesh.cpp
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

#include "Defines.h"
#include "OverlapMesh.h"
#include "MeshUtilitiesFuzzy.h"
#include "MeshUtilitiesExact.h"

#include "Announce.h"

#include <unistd.h>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////

#define VERBOSE

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Enumerator defining a type of intersection.
///	</summary>
enum IntersectType {
	IntersectType_None,
	IntersectType_Edge,
	IntersectType_Node
};

///	<summary>
///		A segment connecting two nodes that also has an associated face
///		on the First and Second mesh.
///	</summary>
class PathSegment : public Edge {

public:
	///	<summary>
	///		Constructor for node intersections.
	///	</summary>
	PathSegment(
		int a_node0,
		int a_node1,
		Edge::Type a_type,
		int a_ixFirstFace,
		int a_ixSecondFace,
		IntersectType a_inttype,
		int a_ixIntersect
	) :
		Edge(a_node0, a_node1, a_type),
		ixFirstFace(a_ixFirstFace),
		ixSecondFace(a_ixSecondFace),
		inttype(a_inttype),
		ixIntersect(a_ixIntersect)
	{
	}

	///	<summary>
	///		Constructor for edge intersections.
	///	</summary>
	PathSegment(
		int a_node0,
		int a_node1,
		Edge::Type a_type,
		int a_ixFirstFace,
		int a_ixSecondFace,
		int a_ixIntersect,
		const Edge & a_edgeIntersect
	) :
		Edge(a_node0, a_node1, a_type),
		ixFirstFace(a_ixFirstFace),
		ixSecondFace(a_ixSecondFace),
		ixIntersect(a_ixIntersect),
		edgeIntersect(a_edgeIntersect)
	{
		inttype = IntersectType_Edge;
	}

public:
	///	<summary>
	///		Origin face on first mesh.
	///	</summary>
	int ixFirstFace;

	///	<summary>
	///		Origin face on second mesh.
	///	</summary>
	int ixSecondFace;

	///	<summary>
	///		Type of intersection that occurs to end this PathSegment.
	///	</summary>
	IntersectType inttype;

	///	<summary>
	///		This is the local index of the edge which one will hit if moving
	///		counter-clockwise around ixSecondFace.
	///	</summary>
	int ixIntersect;

	///	<summary>
	///		If inttype is IntersectType_Edge, this is the index of the edge
	///		on the SecondMesh which has been intersected.
	///	</summary>
	Edge edgeIntersect;
};

///	<summary>
///		A vector of PathSegments.
///	</summary>
typedef std::vector<PathSegment> PathSegmentVector;

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Generate a PathSegmentVector describing the path around the face
///		ixCurrentFirstFace.
///	</summary>
template <class MeshUtilities>
void GeneratePath(
	const Mesh & meshFirst,
	const Mesh & meshSecond,
	const std::vector<int> & vecSecondNodeMap,
	int ixCurrentFirstFace,
	PathSegmentVector & vecTracedPath,
	Mesh & meshOverlap
) {

	// MeshUtilities object
	MeshUtilities utils;

	// Get the NodeVectors
	const NodeVector & nodevecFirst = meshFirst.nodes;
	const NodeVector & nodevecSecond = meshSecond.nodes;
	const NodeVector & nodevecOverlap = meshOverlap.nodes;

	// Current face
	const Face & faceFirstCurrent = meshFirst.faces[ixCurrentFirstFace];

	// Starting point
	Node nodeCurrent =
		nodevecFirst[meshFirst.faces[ixCurrentFirstFace][0]];

	// Find the starting face on the second mesh
	FindFaceStruct aFindFaceStruct;
	utils.FindFaceFromNode(
		meshSecond,
		nodeCurrent,
		aFindFaceStruct);

	// No faces found
	if (aFindFaceStruct.vecFaceIndices.size() == 0) {
		_EXCEPTIONT("No initial face found!");
	}

	// Current face on second mesh
	int ixCurrentSecondFace = aFindFaceStruct.vecFaceIndices[0];

	// This node lies on the boundary between faces.
	if (aFindFaceStruct.vecFaceIndices.size() > 1) {

		ixCurrentSecondFace =
			utils.FindFaceNearNode(
				meshSecond,
				nodeCurrent,
				nodevecFirst[faceFirstCurrent[1]],
				faceFirstCurrent.edges[0].type,
				aFindFaceStruct);
	}

#ifdef VERBOSE
	printf("\nStarting Node: %i", meshFirst.faces[ixCurrentFirstFace][0]);
	printf("\nNext Node: %i", meshFirst.faces[ixCurrentFirstFace][1]);
#endif
	printf("\nFaces: %i %i\n", ixCurrentFirstFace, ixCurrentSecondFace);

	// Trace along all edges of current face
	for (int i = 0; i < faceFirstCurrent.edges.size(); i++) {

		std::cout << ixCurrentSecondFace << std::endl;

		// Equal node indices indicate a non-edge
		if (faceFirstCurrent.edges[i][0] == faceFirstCurrent.edges[i][1]) {
			continue;
		}

		// Initialize the trace
		const Edge & edgeFirstCurrent = faceFirstCurrent.edges[i];

		const Node & nodeFirstBegin = nodevecFirst[edgeFirstCurrent[0]];
		const Node & nodeFirstEnd = nodevecFirst[edgeFirstCurrent[1]];

		int ixOverlapNodeCurrent = edgeFirstCurrent[0];
		Node nodeCurrent = nodevecFirst[ixOverlapNodeCurrent];

		Node nodeLastIntersection = nodeFirstBegin;

		// Repeat until we hit the end of this edge
		for (;;) {

#ifdef VERBOSE
			printf("---\n");
#endif

			const Face & faceSecondCurrent =
				meshSecond.faces[ixCurrentSecondFace];

			// Find all intersections between this edge and the
			// second face
			bool fCoincidentEdge = false;

			// Index within faceSecondCurrent of the intersection
			// Node or Edge.
			int ixIntersectionSecondEdge;

			std::vector<Node> nodeIntersections;

			for (int j = 0; j < faceSecondCurrent.edges.size(); j++) {
				const Edge & edgeSecondCurrent =
					faceSecondCurrent.edges[j];

				// Equal node indices indicating a zero edge
				if (edgeSecondCurrent[0] == edgeSecondCurrent[1]) {
					_EXCEPTIONT("Zero Edge detected");
				}

				fCoincidentEdge =
					utils.CalculateEdgeIntersections(
				 		//meshOverlap.nodes[ixOverlapNodeCurrent],
						meshOverlap.nodes[edgeFirstCurrent[0]],
						meshOverlap.nodes[edgeFirstCurrent[1]],
						edgeFirstCurrent.type,
						nodevecSecond[edgeSecondCurrent[0]],
						nodevecSecond[edgeSecondCurrent[1]],
						edgeSecondCurrent.type,
						nodeIntersections);
/*
				if (nodeIntersections.size() >= 1) {
					nodeIntersections[0].Print("int");
					meshOverlap.nodes[ixOverlapNodeCurrent].Print("B");
					std::cout << (nodeIntersections[i] == meshOverlap.nodes[ixOverlapNodeCurrent]) << std::endl;
				}
*/

				for (int i = 0; i < nodeIntersections.size(); i++) {
					bool fEqualNodes =
						utils.AreNodesEqual(
							nodeIntersections[i],
							nodeLastIntersection);

					if (fEqualNodes) {
						nodeIntersections.erase(nodeIntersections.begin()+i);
						i--;
					}
				}

				if (fCoincidentEdge) {
					nodeIntersections.clear();
					continue;
				}

				if (nodeIntersections.size() > 1) {
					_EXCEPTIONT("Not implemented: Non-convex intersections");
				}

				if (nodeIntersections.size() == 1) {
					ixIntersectionSecondEdge = j;
					break;
				}
			}

			// Done with this edge
			if (nodeIntersections.size() == 0) {
/*
				Face::NodeLocation loc;
				int ixLocation;

				meshSecond.faces[ixCurrentSecondFace].ContainsNode(
					meshSecond.nodes,
					meshOverlap.nodes[edgeFirstCurrent[1]],
					loc,
					ixLocation);

				if (loc == Face::NodeLocation_Edge) {
					_EXCEPTIONT("Intersection failure");
				}
*/
				vecTracedPath.push_back(PathSegment(
					ixOverlapNodeCurrent,
					edgeFirstCurrent[1],
					edgeFirstCurrent.type,
					ixCurrentFirstFace,
					ixCurrentSecondFace,
					IntersectType_None,
					0));

				break;
			}

			// Coincident edges
			if (fCoincidentEdge) {
				_EXCEPTIONT("Not implemented");
			}

			// Invalid option
			if (nodeIntersections.size() > 1) {
				_EXCEPTIONT("Logic Error");
			}

			// Set last intersection
			nodeLastIntersection = nodeIntersections[0];

			// Find next face on meshSecond
			const Edge & edgeSecondCurrent =
				faceSecondCurrent.edges[ixIntersectionSecondEdge];

			const Node & nodeSecondEdge0 =
				nodevecSecond[edgeSecondCurrent[0]];
			const Node & nodeSecondEdge1 =
				nodevecSecond[edgeSecondCurrent[1]];

			int ixOverlapNodeNext;

			// Special case:  Intersection with Edge is exactly an
			// beginpoint / endpoint of edgeFirstCurrent
			if (utils.AreNodesEqual(
					nodeIntersections[0], meshOverlap.nodes[edgeFirstCurrent[1]])
			) {
				// Next edge
				int iNext = (i + 1) % faceFirstCurrent.edges.size();

				const Edge & edgeFirstNext =
					faceFirstCurrent.edges[iNext];

				// Update SecondMesh face
				EdgeMapConstIterator iter =
					meshSecond.edgemap.find(edgeSecondCurrent);

				if (iter == meshSecond.edgemap.end()) {
					_EXCEPTIONT("Logic error");
				}

				int ixNextSecondFace;

				// Path hits the beginpoint of the Edge
				if (utils.AreNodesEqual(nodeIntersections[0], nodeSecondEdge0)) {
					ixNextSecondFace =
						utils.FindFaceNearNode(
							meshSecond,
							edgeSecondCurrent[0],
							nodevecFirst[edgeFirstNext[1]],
							edgeFirstNext.type);

					if (ixNextSecondFace == ixCurrentSecondFace) {
						printf("WARNING: Face does not change across Edge (1)\n");
					}

					// If face changes insert a Node bifurcation
					if (ixNextSecondFace != ixCurrentSecondFace) {
						vecTracedPath.push_back(PathSegment(
							ixOverlapNodeCurrent,
							edgeFirstCurrent[1],
							edgeFirstCurrent.type,
							ixCurrentFirstFace,
							ixCurrentSecondFace,
							IntersectType_Node,
							ixIntersectionSecondEdge));
					}

				// Path hits the endpoint of the Edge
				} else if (
					utils.AreNodesEqual(nodeIntersections[0], nodeSecondEdge1)
				) {
					ixNextSecondFace =
						utils.FindFaceNearNode(
							meshSecond,
							edgeSecondCurrent[1],
							nodevecFirst[edgeFirstNext[1]],
							edgeFirstNext.type);

					if (ixNextSecondFace == ixCurrentSecondFace) {
						printf("WARNING: Face does not change across Edge (2)\n");
					}

					// If face changes insert a Node bifuraction
					if (ixNextSecondFace != ixCurrentSecondFace) {
						vecTracedPath.push_back(PathSegment(
							ixOverlapNodeCurrent,
							edgeFirstCurrent[1],
							edgeFirstCurrent.type,
							ixCurrentFirstFace,
							ixCurrentSecondFace,
							IntersectType_Node,
							(ixIntersectionSecondEdge + 1)
								% faceSecondCurrent.edges.size()));
					}

				// Path hits the Edge directly
				} else {
					const FacePair & facepair = iter->second;

					// Find the set of possible Faces
					std::vector<int> vecPossibleFaces;
					vecPossibleFaces.push_back(facepair[0]);
					vecPossibleFaces.push_back(facepair[1]);

					// Build the FindFaceStruct
					FindFaceStruct aNextFindFaceStruct;

					aNextFindFaceStruct.vecFaceIndices.push_back(facepair[0]);
					aNextFindFaceStruct.vecFaceIndices.push_back(facepair[1]);

					const Face & face0 = meshSecond.faces[facepair[0]];
					const Face & face1 = meshSecond.faces[facepair[1]];

					int iEdge0 = face0.GetEdgeIndex(edgeSecondCurrent);
					int iEdge1 = face1.GetEdgeIndex(edgeSecondCurrent);

					aNextFindFaceStruct.vecFaceLocations.push_back(iEdge0);
					aNextFindFaceStruct.vecFaceLocations.push_back(iEdge1);

					aNextFindFaceStruct.loc = Face::NodeLocation_Edge;

					// Find the next Face index
					ixNextSecondFace =
						utils.FindFaceNearNode(
							meshSecond,
							nodevecFirst[edgeFirstNext[0]],
							nodevecFirst[edgeFirstNext[1]],
							edgeFirstNext.type,
							aNextFindFaceStruct);

					if (ixNextSecondFace == ixCurrentSecondFace) {
						printf("WARNING: Face does not change across Edge (3)\n");
					}

					// If face changes insert an Edge bifurcation
					if (ixNextSecondFace != ixCurrentSecondFace) {
						vecTracedPath.push_back(PathSegment(
							ixOverlapNodeCurrent,
							edgeFirstCurrent[1],
							edgeFirstCurrent.type,
							ixCurrentFirstFace,
							ixCurrentSecondFace,
							ixIntersectionSecondEdge,
							edgeSecondCurrent));
					}
				}

				// Remain on the same face
				if (ixNextSecondFace == ixCurrentSecondFace) {
					vecTracedPath.push_back(PathSegment(
						ixOverlapNodeCurrent,
						edgeFirstCurrent[1],
						edgeFirstCurrent.type,
						ixCurrentFirstFace,
						ixCurrentSecondFace,
						IntersectType_None,
						ixIntersectionSecondEdge));

				}

				// Update OverlapNodeCurrent
				ixOverlapNodeCurrent = edgeFirstCurrent[1];

				ixCurrentSecondFace = ixNextSecondFace;

				break;
			}

			// FirstEdge hits nodeSecondEdge0
			if (utils.AreNodesEqual(nodeIntersections[0], nodeSecondEdge0)) {
				ixOverlapNodeNext =
					vecSecondNodeMap[edgeSecondCurrent[0]];

				vecTracedPath.push_back(PathSegment(
					ixOverlapNodeCurrent,
					ixOverlapNodeNext,
					edgeFirstCurrent.type,
					ixCurrentFirstFace,
					ixCurrentSecondFace,
					IntersectType_Node,
					ixIntersectionSecondEdge));

				int ixPrevSecondFace = ixCurrentSecondFace;

				ixCurrentSecondFace =
					utils.FindFaceNearNode(
						meshSecond,
						edgeSecondCurrent[0],
						nodeFirstEnd,
						edgeFirstCurrent.type);

				if (ixPrevSecondFace == ixCurrentSecondFace) {
					printf("WARNING: Face does not change across Edge (4)\n");
				}

				ixOverlapNodeCurrent = ixOverlapNodeNext;

				if (ixOverlapNodeNext == edgeFirstCurrent[1]) {
					break;
				}

				continue;
			
			// FirstEdge hits nodeSecondEdge1
			} else if (
				utils.AreNodesEqual(nodeIntersections[0], nodeSecondEdge1)
			) {
				ixOverlapNodeNext =
					vecSecondNodeMap[edgeSecondCurrent[1]];

				vecTracedPath.push_back(PathSegment(
					ixOverlapNodeCurrent,
					ixOverlapNodeNext,
					edgeFirstCurrent.type,
					ixCurrentFirstFace,
					ixCurrentSecondFace,
					IntersectType_Node,
					(ixIntersectionSecondEdge + 1)
						% faceSecondCurrent.edges.size()));

				int ixPrevSecondFace = ixCurrentSecondFace;

				ixCurrentSecondFace =
					utils.FindFaceNearNode(
						meshSecond,
						edgeSecondCurrent[1],
						nodeFirstEnd,
						edgeFirstCurrent.type);

				if (ixPrevSecondFace == ixCurrentSecondFace) {
					printf("WARNING: Face does not change across Edge (5)\n");
				}

				ixOverlapNodeCurrent = ixOverlapNodeNext;

				if (ixOverlapNodeNext == edgeFirstCurrent[1]) {
					break;
				}

				continue;

			// General intersection between edgeFirstCurrent and
			// edgeSecondCurrent.
			} else {

				// Push a new intersection into the array of nodes
				ixOverlapNodeNext =
					static_cast<int>(meshOverlap.nodes.size());

				meshOverlap.nodes.push_back(nodeIntersections[0]);

				// Intersection found with edge
				vecTracedPath.push_back(PathSegment(
					ixOverlapNodeCurrent,
					ixOverlapNodeNext,
					edgeFirstCurrent.type,
					ixCurrentFirstFace,
					ixCurrentSecondFace,
					ixIntersectionSecondEdge,
					edgeSecondCurrent));

				// Update OverlapNodeCurrent
				ixOverlapNodeCurrent = ixOverlapNodeNext;

				// Update SecondMesh face
				EdgeMapConstIterator iter =
					meshSecond.edgemap.find(edgeSecondCurrent);

				if (iter == meshSecond.edgemap.end()) {
					_EXCEPTIONT("Logic error");
				}

				const FacePair & facepair = iter->second;

				int ixPrevSecondFace = ixCurrentSecondFace;

				std::vector<int> vecPossibleFaces;
				vecPossibleFaces.push_back(facepair[0]);
				vecPossibleFaces.push_back(facepair[1]);

				// Build the FindFaceStruct
				FindFaceStruct aNextFindFaceStruct;

				aNextFindFaceStruct.vecFaceIndices.push_back(facepair[0]);
				aNextFindFaceStruct.vecFaceIndices.push_back(facepair[1]);

				const Face & face0 = meshSecond.faces[facepair[0]];
				const Face & face1 = meshSecond.faces[facepair[1]];

				int iEdge0 = face0.GetEdgeIndex(edgeSecondCurrent);
				int iEdge1 = face1.GetEdgeIndex(edgeSecondCurrent);

				aNextFindFaceStruct.vecFaceLocations.push_back(iEdge0);
				aNextFindFaceStruct.vecFaceLocations.push_back(iEdge1);

				aNextFindFaceStruct.loc = Face::NodeLocation_Edge;

				// Find the next Face index near this Node
				ixCurrentSecondFace = 
					utils.FindFaceNearNode(
						meshSecond,
						nodeIntersections[0],
						meshOverlap.nodes[edgeFirstCurrent[1]],
						edgeFirstCurrent.type,
						aNextFindFaceStruct);

				if (ixPrevSecondFace == ixCurrentSecondFace) {
					printf("WARNING: Face does not change across Edge (3)\n");
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

bool GenerateOverlapFaces(
	const Mesh & meshSecond,
	const std::vector<int> & vecSecondNodeMap,
	const PathSegmentVector & vecTracedPath,
	Mesh & meshOverlap
) {

	// Array indicating which elements of vecTracedPath have been used
	std::vector<bool> vecTracedPathUsed;
	vecTracedPathUsed.resize(vecTracedPath.size(), false);

	// The set of interior faces (from the Second mesh)
	std::set<int> setSecondFacesAdded;
	for (int j = 0; j < vecTracedPath.size(); j++) {
		setSecondFacesAdded.insert(vecTracedPath[j].ixSecondFace);

#ifdef VERBOSE
		printf("%i %i : %i\n", vecTracedPath[j][0], vecTracedPath[j][1], vecTracedPath[j].ixSecondFace);
#endif
	}

	// Set of faces from meshSecond that should be added
	std::set<int> setSecondFacesToAdd;

	// Loop through all possible starting PathSegments
	for (int j = 0; j < vecTracedPath.size(); j++) {

		// Ignore starting PathSegments that have already been used
		if (vecTracedPathUsed[j]) {
			continue;
		}

		// Build the new face
		Face faceOverlap(0);

		// Origin node of this fac
		int ixOverlapOriginNode = vecTracedPath[j][0];

		// Current second face
		int ixCurrentSecondFace =
			vecTracedPath[j].ixSecondFace;

		const Face & faceSecondCurrent =
			meshSecond.faces[ixCurrentSecondFace];

		// Search may require multiple trips along FirstMesh and SecondMesh
		for (;;) {

			// Loop along edge of FirstFace until we find a branch
			// into SecondMesh or hit our origin.
			for (;; j++) {
				faceOverlap.edges.push_back(vecTracedPath[j]);

				// Mark traced path edge as used
				if (vecTracedPathUsed[j]) {
					_EXCEPTIONT("Trying to reuse traced path edge");
				}

				vecTracedPathUsed[j] = true;

				printf("P%i: %i %i\n",
					j, vecTracedPath[j][0], vecTracedPath[j][1]);

				// Found a branch into the interior
				if (vecTracedPath[j].inttype != IntersectType_None) {
					break;
				}

				// Hit origin; finish this face
				if (vecTracedPath[j][1] == ixOverlapOriginNode) {
					goto ContinueToNextFace;
				}
			}

			// Determine the index of intersection
			int ixCurrentSecondEdge = vecTracedPath[j].ixIntersect;

			int ixCurrentOverlapNode = vecTracedPath[j][1];

			// Loop around the interior of faceSecondCurrent
			int nEdgesCompleted = 0;
			for (;;) {

				const Edge & edgeSecondCurrent =
					faceSecondCurrent.edges[ixCurrentSecondEdge];

				// Check for infinite loop
				if (nEdgesCompleted > faceSecondCurrent.edges.size()) {
					printf("Possible infinite loop - aborting\n");
					return false;
				}

				nEdgesCompleted++;

				// Identical endpoints; advance the edge
				if (edgeSecondCurrent[0] == edgeSecondCurrent[1]) {
					ixCurrentSecondEdge =
						(ixCurrentSecondEdge + 1)
							% faceSecondCurrent.edges.size();

					ixCurrentOverlapNode =
						vecSecondNodeMap[edgeSecondCurrent[1]];

					continue;
				}

				// Determine if this edge exits onto FirstMesh
				int ixExitNode = InvalidNode;

				int k;
				for (
					k = (j + 1) % vecTracedPath.size();
					k != j;
					k = (k + 1) % vecTracedPath.size()
				) {
					int ixIntersectNode =
						faceSecondCurrent[vecTracedPath[k].ixIntersect];

					IntersectType inttype = vecTracedPath[k].inttype;

					if (ixCurrentOverlapNode == vecTracedPath[k][1]) {
						continue;
					}

					// Check for node intersections
					if (inttype == IntersectType_Node) {
						if (vecTracedPath[k][1] ==
							vecSecondNodeMap[edgeSecondCurrent[0]]
						) {
							ixExitNode = vecTracedPath[k][1];
							break;
						}

						if (vecTracedPath[k][1] ==
							vecSecondNodeMap[edgeSecondCurrent[1]]
						) {
							ixExitNode = vecTracedPath[k][1];
							break;
						}
					}

					// Check for edge intersections
					if ((inttype == IntersectType_Edge) &&
						(edgeSecondCurrent == vecTracedPath[k].edgeIntersect)
					) {
						ixExitNode = vecTracedPath[k][1];
						break;
					}
				}

				// Add the interior face to the list of faces to be added
				EdgeMapConstIterator iter =
					meshSecond.edgemap.find(edgeSecondCurrent);

				if (iter == meshSecond.edgemap.end()) {
					_EXCEPTIONT("Logic error");
				}

				const FacePair & facepair = iter->second;

				if (facepair[0] == ixCurrentSecondFace) {
					setSecondFacesToAdd.insert(facepair[1]);
				} else if (facepair[1] == ixCurrentSecondFace) {
					setSecondFacesToAdd.insert(facepair[0]);
				} else {
					_EXCEPTIONT("Logic error");
				}

				// An exit node is found
				if (ixExitNode != InvalidNode) {

					// Check if the traced path becomes active
					int jNext = (k + 1) % vecTracedPath.size();
					if (vecTracedPath[jNext].ixSecondFace ==
						ixCurrentSecondFace
					) {

						printf("S: %i %i\n",
							ixCurrentOverlapNode, ixExitNode);

						faceOverlap.edges.push_back(Edge(
							ixCurrentOverlapNode,
							ixExitNode,
							edgeSecondCurrent.type));

						j = jNext;
						if (ixExitNode == ixOverlapOriginNode) {
							goto ContinueToNextFace;
						} else {
							break;
						}
					}
				}

				printf("T: %i (%i) %i\n",
					ixCurrentOverlapNode,
					vecSecondNodeMap[edgeSecondCurrent[0]],
					vecSecondNodeMap[edgeSecondCurrent[1]]);

				// Push this edge into the overlap mesh
				faceOverlap.edges.push_back(Edge(
					ixCurrentOverlapNode,
					vecSecondNodeMap[edgeSecondCurrent[1]],
					edgeSecondCurrent.type));

				// Advance the edge
				ixCurrentSecondEdge =
					(ixCurrentSecondEdge + 1)
						% faceSecondCurrent.edges.size();

				ixCurrentOverlapNode =
					vecSecondNodeMap[edgeSecondCurrent[1]];

				if (ixCurrentOverlapNode == ixOverlapOriginNode) {
					goto ContinueToNextFace;
				}
			}
		}

ContinueToNextFace:
		printf("PUSH %lu\n", faceOverlap.edges.size());
		// Push this Face into the overlap Mesh
		meshOverlap.faces.push_back(faceOverlap);

		// Reset the search counter
		j = 0;

		// Check if all segments have been used
		int nUsedSegments = 0;
		for (int k = 0; k < vecTracedPath.size(); k++) {
			if (vecTracedPathUsed[k]) {
				nUsedSegments++;
			}
		}
		if (nUsedSegments == static_cast<int>(vecTracedPath.size())) {
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////
	// Find interior faces from meshSecond to add to meshOverlap

	// Remove added faces from set of faces to add
	std::set<int>::iterator iterToAdd;
	std::set<int>::iterator iterAdded = setSecondFacesAdded.begin();
	for (; iterAdded != setSecondFacesAdded.end(); iterAdded++) {
		iterToAdd = setSecondFacesToAdd.find(*iterAdded);

		if (iterToAdd != setSecondFacesToAdd.end()) {
			setSecondFacesToAdd.erase(iterToAdd);
		}
	}

	iterToAdd = setSecondFacesToAdd.begin();
	while (iterToAdd != setSecondFacesToAdd.end()) {

		const Face & faceSecondCurrent =
			meshSecond.faces[*iterToAdd];

		// Add this face to meshOverlap
		Face faceOverlapCurrent(faceSecondCurrent.edges.size());

		for (int i = 0; i < faceOverlapCurrent.edges.size(); i++) {
			faceOverlapCurrent.edges[i][0] =
				vecSecondNodeMap[faceSecondCurrent.edges[i][0]];
			faceOverlapCurrent.edges[i][1] =
				vecSecondNodeMap[faceSecondCurrent.edges[i][1]];
			faceOverlapCurrent.edges[i].type =
				faceSecondCurrent.edges[i].type;
		}

		meshOverlap.faces.push_back(faceOverlapCurrent);

		setSecondFacesAdded.insert(*iterToAdd);

		// Add further interior faces
		bool fMoreFacesToAdd = false;

		for (int i = 0; i < faceSecondCurrent.edges.size(); i++) {

			if (faceSecondCurrent.edges[i][0] ==
				faceSecondCurrent.edges[i][1]
			) {
				continue;
			}

			EdgeMapConstIterator iterEdge =
				meshSecond.edgemap.find(faceSecondCurrent.edges[i]);

			if (iterEdge == meshSecond.edgemap.end()) {
				_EXCEPTIONT("Edge not found in EdgeMap");
			}

			int iOtherFace;
			if (iterEdge->second[0] == *iterToAdd) {
				iOtherFace = iterEdge->second[1];
			} else if (iterEdge->second[1] == *iterToAdd) {
				iOtherFace = iterEdge->second[0];
			} else {
				_EXCEPTIONT("EdgeMap consistency error");
			}

			if (setSecondFacesAdded.find(iOtherFace) ==
				setSecondFacesAdded.end()
			) {
				setSecondFacesToAdd.insert(iOtherFace);
				fMoreFacesToAdd = true;
			}
		}

		if (fMoreFacesToAdd) {
			setSecondFacesToAdd.erase(iterToAdd);
			iterToAdd = setSecondFacesToAdd.begin();
		} else {
			iterToAdd++;
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

void GenerateOverlapMesh(
	const Mesh & meshFirst,
	const Mesh & meshSecond,
	Mesh & meshOverlap,
	int nVerbosity
) {
	meshOverlap.Clear();

	// Get the two NodeVectors
	const NodeVector & nodevecFirst = meshFirst.nodes;
	const NodeVector & nodevecSecond = meshSecond.nodes;
	const NodeVector & nodevecOverlap = meshOverlap.nodes;

	// Construct the coincident node vector
	std::vector<int> vecSecondNodeMap;
	int nCoincidentNodes =
		BuildCoincidentNodeVector(
			meshFirst, meshSecond, vecSecondNodeMap);

	Announce("Number of coincident nodes [%i]", nCoincidentNodes);

	// Insert all nodes from the two NodeVectors
	for (int i = 0; i < nodevecFirst.size(); i++) {
		meshOverlap.nodes.push_back(nodevecFirst[i]);
	}
	const int ixOverlapSecondNodesBegin = meshOverlap.nodes.size();

	for (int i = 0; i < nodevecSecond.size(); i++) {
		meshOverlap.nodes.push_back(nodevecSecond[i]);
		if (vecSecondNodeMap[i] != InvalidNode) {
			continue;
		}
	}
	const int ixOverlapNewNodesBegin = meshOverlap.nodes.size();

	// Build the meshSecond node map
	for (int i = 0; i < vecSecondNodeMap.size(); i++) {
		if (vecSecondNodeMap[i] == InvalidNode) {
			vecSecondNodeMap[i] = ixOverlapSecondNodesBegin + i;
		}
	}

	// Loop through all Faces on the first Mesh
	int ixCurrentFirstFace = 0;

#pragma message "OpenMP here"
	for (; ixCurrentFirstFace < meshFirst.faces.size(); ixCurrentFirstFace++) {
	//for (int ixCurrentFirstFace = 389; ixCurrentFirstFace < 390; ixCurrentFirstFace++) {

		// Generate the path
		PathSegmentVector vecTracedPath;

		GeneratePath<MeshUtilitiesFuzzy>(
			meshFirst,
			meshSecond,
			vecSecondNodeMap,
			ixCurrentFirstFace,
			vecTracedPath,
			meshOverlap
		);

		// Find Faces associated with the TracedPath
		bool fSuccess =
			GenerateOverlapFaces(
				meshSecond,
				vecSecondNodeMap,
				vecTracedPath,
				meshOverlap
			);

		if (!fSuccess) {
			_EXCEPTIONT("OverlapMesh generation failed");
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

