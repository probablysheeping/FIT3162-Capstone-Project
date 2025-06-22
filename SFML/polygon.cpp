#include "polygon.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include "imgui.h"

/*
Implementation of algorithms for polygons
Assumes no overlapping edges and no interior holes.
*/

bool pointInPolygon(ImVec2 p, Polygon* polygon) {
	// Algorithm based on winding number
	// Draw ray from point to positive infinity in y axis (down)

	int windingNumber = 0;

	for (int i = 0; i < polygon->vertices.size(); i++) {
		ImVec2 a = polygon->vertices.at(i);
		ImVec2 b = i == polygon->vertices.size() - 1 ? polygon->vertices.front() : polygon->vertices.at(i + 1);


		// check that P is in the middle of the segment AB if projected onto x axis
		// then check the y coordinate of POI between AB and the ray from P is geq p.y
		float xdiff1 = a.x - p.x;
		float xdiff2 = b.x - p.x;
		if (std::signbit(xdiff1 == 0 ? -1 : xdiff1) != std::signbit(xdiff2 == 0 ? -1 : xdiff2) && ((b.y - a.y) * (p.x - a.x) / (b.x - a.x)) + a.y >= p.y) {
			// now check orientation of line
			if (a.x > p.x) {
				windingNumber += 1;
			}
			else {
				windingNumber -= 1;
			}
		}

	}

	return windingNumber;

}

double signedArea(Polygon* polygon) {
	// https://demonstrations.wolfram.com/SignedAreaOfAPolygon/
	// If vertices are oriented clockwise then signed area is positive
	// Otherwise it is negative
	double result = 0;
	std::vector<ImVec2>* vertices = &(polygon->vertices);
	int j;
	for (int i = 0; i < polygon -> vertices.size(); i++) {
		j = i+1 == vertices->size() ? 0 : i + 1;
		result += (vertices->at(i).x * vertices->at(j).y) - (vertices->at(j).x * vertices->at(i).y);
	}
	return 0.5 * result;
}

int sgn(double x) {
	return (x > 0) - (x < 0);
}

double distanceL2(ImVec2 p, ImVec2 q) {
	// distance using L2 metric
	return sqrt(pow(p.x - q.x, 2) + pow(p.y - q.y, 2));
}

float sideOfLine(ImVec2 p, ImVec2 a, ImVec2 b) {
	// returns whether p is to the "left" of AB or to the "right"
	const int x = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
	return sgn(x);
}

Polygon intersectingPolygon(Polygon* p1, Polygon* p2) {
	// Returns a polygon which is in the intersection of p1 and p2 and maximal in area
	// Sutherland-Hodgman algorithm https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm
	// We do need the orientation of the vertices for this algorithm however.
	 std::vector<ImVec2> outputList = p1 -> vertices;

	 if (signedArea(p1) < 0) {
		 // we need the vertices to be oriented clockwise
		 std::reverse(outputList.begin(), outputList.end());
	 }
	 int i2, j2;
	 for (int i = 0; i < p2->vertices.size(); i++) {
		 // edge is P2V(i), P2V(i+1) (i+1 taken mod |P2V|)
		 // for each edge in result, we need to check if it intersects with P2V(i)P2V(i+1)
		 // if it doesn't nothing needs to be done
		 // if it does we need to cut of the section that is outside
		 i2 = (i + 1) == p2->vertices.size() ? 0 : i + 1;

		 std::vector<ImVec2> newOutputList;
		 
		 for (int j = 0; j < outputList.size(); j++) {
			 // 2nd vertex is inside.
			 j2 = (j - 1) < 0 ? outputList.size() - 1 : j - 1;
			 // If 2nd vertex in visible area
			 if (sideOfLine(outputList.at(j), p2->vertices.at(i), p2->vertices.at(i2)) >= 0) {
				 newOutputList.push_back(outputList.at(i));
				 // If 1st vertex not in visible area
				 if (sideOfLine(outputList.at(j2), p2->vertices.at(i), p2->vertices.at(i2)) < 0) {
					 const ImVec2 POI = intersectingSegments(outputList.at(j2), outputList.at(j), p2->vertices.at(i), p2->vertices.at(i2);
					 newOutputList.push_back(POI);
				 }
			 }
			 // If 1st vertex in visible area
			 else if (sideOfLine(outputList.at(j2), p2->vertices.at(i), p2->vertices.at(i2)) >= 0) {
				  const ImVec2 POI = intersectingSegments(outputList.at(j2), outputList.at(j), p2->vertices.at(i), p2->vertices.at(i2));
				  newOutputList.push_back(POI);

			 }
		 }
		 outputList = newOutputList;
	 }

	 Polygon result;
	 result.vertices = outputList;

	 return result;
}

ImVec2 intersectingSegments(ImVec2 a, ImVec2 b, ImVec2 p, ImVec2 q) {
	/*
	Given line segments AB and PQ, find point of intersection between AB and PQ if it exists
	If it does not return (-1,-1) which can't be shown on the canvas and represent a "garbage value"
	*/
	const float m1 = (b.y - a.y) / (b.x - a.x);
	const float m2 = (q.y - p.y) / (q.x - p.x);

	float x = (a.y - p.y) + p.x * m2 - a.x * m1;
	float y = m1 * (x - p.x) + p.y;

	if (std::max(std::min(a.x, b.x), std::min(p.x, q.x)) <= x <= std::min(std::max(a.x,b.x), std::max(p.x,q.x)) && 
		std::max(std::min(a.y, b.y), std::min(p.y, q.y)) <= y <= std::min(std::max(a.y, b.y), std::max(p.y, q.y))) {

		return ImVec2(x, y);

	}

	return ImVec2(-1, -1);
}