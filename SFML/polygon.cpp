#include "polygon.h"
#include <iostream>
#include <cmath>
#include "imgui.h"

// Implementation of algorithms for polygons
// Assumes no overlapping edges and no interior holes.

bool pointInPolygon(ImVec2 p, Polygon* polygon) {
	// Algorithm based on winding number
	// O(n) worst case, but practically it is nearly constant.
	// Draw ray from point to positive infinity in y axis (down)
	int windingNumber = 0;

	for (int i = 0; i < polygon->vertices.size(); i++) {
		ImVec2 a = polygon->vertices.at(i);
		ImVec2 b = i == polygon->vertices.size() - 1 ? polygon->vertices.front() : polygon->vertices.at(i + 1);


		// check that P is in the middle of the segment AB if projected onto x axis
		// then check the y coordinate of POI between AB and the ray from P is geq p.y

		if (std::signbit(a.x - p.x) != std::signbit(b.x - p.x) && ((b.y - a.y) * (p.x - a.x) / (b.x - a.x)) + a.y >= p.y) {
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