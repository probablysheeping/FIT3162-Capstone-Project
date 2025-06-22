#include "polygon.h"
#include <iostream>
#include <cmath>
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

double distanceL2(ImVec2 p, ImVec2 q) {
	// distance using L2 metric
	return sqrt(pow(p.x - q.x, 2) + pow(p.y - q.y, 2));
}

float area(Polygon polygon) {
	return NULL;
}

Polygon intersection(Polygon* p1, Polygon* p2) {
	// Returns a polygon which is in the intersection of p1 and p2 and maximal in area
	Polygon result = *p1;
	return result;
}