#define _USE_MATH_DEFINES
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

// CONSTRUCTOR AND DESTRUCTOR

Polygon::Polygon() {
}

//unecessary?
Polygon::~Polygon() {
}

// CLASS METHODS

bool Polygon::pointInPolygon(ImVec2 p) {
	// Algorithm based on winding number
	// Draw ray from point to positive infinity in y axis (down)

	int windingNumber = 0;

	for (int i = 0; i < this->vertices.size(); i++) {
		ImVec2 a = this->vertices.at(i);
		ImVec2 b = i == this->vertices.size() - 1 ? this->vertices.front() : this->vertices.at(i + 1);


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

float Polygon::signedArea() {
	// https://demonstrations.wolfram.com/SignedAreaOfAPolygon/
	// If vertices are oriented clockwise then signed area is positive
	// Otherwise it is negative
	double result = 0;
	std::vector<ImVec2>* vertices = &(this->vertices);
	int j;
	for (int i = 0; i < this->vertices.size(); i++) {
		j = i + 1 == vertices->size() ? 0 : i + 1;
		result += (vertices->at(i).x * vertices->at(j).y) - (vertices->at(j).x * vertices->at(i).y);
	}
	return 0.5 * result;
}

float Polygon::polygonArea() {
	return abs(this->signedArea());
}

// GETTERS AND SETTERS

void Polygon::setVertices(std::vector<ImVec2> vertices)
{
	this->vertices = vertices;
	sf::ConvexShape convex;

	int n = vertices.size(); // vertex count
	convex.setPointCount(n);
	convex.setFillColor(sf::Color((int)(this->colour[0] * 255), (int)(this->colour[1] * 255), (int)(this->colour[2] * 255)));
	for (int i = 0; i < n; i++) {
		convex.setPoint(i, vertices.at(i));
	}
	this->render = convex;
}

std::vector<ImVec2> Polygon::getVertices()
{
	return this->vertices;
}

void Polygon::translate(ImVec2 delta) {
	for (auto& v : this->vertices) {
		v.x += delta.x;
		v.y += delta.y;
	}

	// Update SFML convex shape to match new vertices
	this->render.setPointCount(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i) {
		this->render.setPoint(i, sf::Vector2f(vertices[i].x, vertices[i].y));
	}
}

ImVec2 Polygon::centroid() const {
	ImVec2 c = { 0.f, 0.f };
	if (vertices.empty()) return c;
	for (const auto& v : vertices) {
		c.x += v.x;
		c.y += v.y;
	}
	c.x /= static_cast<float>(vertices.size());
	c.y /= static_cast<float>(vertices.size());
	return c;
}

void Polygon::setColour(float (&color)[3])
{
	std::copy(std::begin(color), std::end(color), std::begin(this->colour));
	this->render.setFillColor(sf::Color((int)(this->colour[0] * 255), (int)(this->colour[1] * 255), (int)(this->colour[2] * 255)));
}

float Polygon::getColour(int index)
{
	if (index < 0 || index > 2)
		return -1;
	else
		return this->colour[index];
}

// Conversion operator
Polygon::operator std::string() const {
	std::string returnString;

	returnString = returnString + 
		"POLYGON\n" + 
		"VERTICES " + std::to_string(this->vertices.size()) + "\n";

	for (ImVec2 vertex : this->vertices)
		returnString = returnString + 
		std::to_string(vertex.x) + ' ' + 
		std::to_string(vertex.y) + '\n';

	returnString = returnString + "COLOUR "
		+ std::to_string(this->colour[0]) + ' '
		+ std::to_string(this->colour[1]) + ' '
		+ std::to_string(this->colour[2]) + '\n'
		+ "END\n";

	return returnString;
}

// Stream output operator
std::ostream& operator<<(std::ostream& os, const Polygon& obj) {
	os << static_cast<std::string>(obj);
	return os;
}


int sgn(double x) {
	return (x > 0) - (x < 0);
}

float distanceL2(ImVec2 p, ImVec2 q) {
	// distance using L2 metric
	return sqrtf(pow(p.x - q.x, 2) + pow(p.y - q.y, 2));
}

float sideOfLine(ImVec2 p, ImVec2 a, ImVec2 b) {
	// returns whether p is to the "left" of AB or to the "right"
	// left = -1, right = 1, on = 0
	const int x = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
	return sgn(x);
}

/**
* COLOUR BLENDING USING HSL
*/
struct HSL {
	float h; // [0, 360)
	float s; // [0, 1]
	float l; // [0, 1]
};

// Convert RGB (0..1) to HSL
HSL rgbToHsl(float r, float g, float b) {
	float max = std::max({ r, g, b });
	float min = std::min({ r, g, b });
	float h, s, l;
	l = (max + min) * 0.5f;

	if (max == min) {
		h = s = 0.0f; // achromatic
	}
	else {
		float d = max - min;
		s = l > 0.5f ? d / (2.0f - max - min) : d / (max + min);

		if (max == r)
			h = fmod(((g - b) / d + (g < b ? 6.0f : 0.0f)), 6.0f);
		else if (max == g)
			h = ((b - r) / d + 2.0f);
		else
			h = ((r - g) / d + 4.0f);

		h *= 60.0f; // convert to degrees
	}
	return { h, s, l };
}

// Helper for HSL -> RGB
float hueToRgb(float p, float q, float t) {
	if (t < 0.0f) t += 1.0f;
	if (t > 1.0f) t -= 1.0f;
	if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
	if (t < 1.0f / 2.0f) return q;
	if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
	return p;
}

// Convert HSL -> RGB (0..1)
std::array<float, 3> hslToRgb(const HSL& hsl) {
	float r, g, b;
	float h = hsl.h / 360.0f;
	float s = hsl.s;
	float l = hsl.l;

	if (s == 0.0f) {
		r = g = b = l; // achromatic
	}
	else {
		float q = l < 0.5f ? l * (1.0f + s) : (l + s - l * s);
		float p = 2.0f * l - q;
		r = hueToRgb(p, q, h + 1.0f / 3.0f);
		g = hueToRgb(p, q, h);
		b = hueToRgb(p, q, h - 1.0f / 3.0f);
	}
	return { r, g, b };
}

Polygon intersectingPolygon(Polygon* p1, Polygon* p2) {
	// Returns a polygon which is in the intersection of p1 and p2 and maximal in area
	// Sutherland-Hodgman algorithm https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm
	// We do need the orientation of the vertices for this algorithm however.
	 std::vector<ImVec2> outputList = p1->getVertices();

	 if (p1->signedArea() < 0) {
		 // we need the vertices to be oriented clockwise
		 std::reverse(outputList.begin(), outputList.end());
	 }
	 int i2, j2;
	 for (int i = 0; i < p2->getVertices().size(); i++) {
		 // edge is P2V(i), P2V(i+1) (i+1 taken mod |P2V|)
		 // for each edge in result, we need to check if it intersects with P2V(i)P2V(i+1)
		 // if it doesn't nothing needs to be done
		 // if it does we need to cut of the section that is outside
		 i2 = (i + 1) == p2->getVertices().size() ? 0 : i + 1;

		 std::vector<ImVec2> newOutputList;
		 
		 for (int j = 0; j < outputList.size(); j++) {
			 // 2nd vertex is inside.
			 j2 = (j + 1) == outputList.size() ? 0 : j + 1;
			 // If 2nd vertex in visible area
			 if (sideOfLine(outputList.at(j2), p2->getVertices().at(i), p2->getVertices().at(i2)) >= 0) {
				 
				 // If 1st vertex not in visible area
				 if (sideOfLine(outputList.at(j), p2->getVertices().at(i), p2->getVertices().at(i2)) < 0) {
					 const ImVec2 POI = intersectingSegments(outputList.at(j), outputList.at(j2), p2->getVertices().at(i), p2->getVertices().at(i2));
					 // Block failed vertex (-1,-1)
					 if (POI.x != -1 && POI.y != -1)
						newOutputList.push_back(POI);
				 }

				 newOutputList.push_back(outputList.at(j2));
			 }
			 // If 1st vertex in visible area
			 else if (sideOfLine(outputList.at(j), p2->getVertices().at(i), p2->getVertices().at(i2)) >= 0) {
				  const ImVec2 POI = intersectingSegments(outputList.at(j), outputList.at(j2), p2->getVertices().at(i), p2->getVertices().at(i2));
				  // Block failed vertex (-1,-1)
				  if (POI.x != -1 && POI.y != -1)
					newOutputList.push_back(POI);
				  

			 }
		 }
		 outputList = newOutputList;

	 }

	 // Failure case
	 if (outputList.size() < 3) {
		 Polygon empty;
		 empty.setVertices({});
		 float black[3] = { 0,0,0 };
		 empty.setColour(black);
		 return empty;
	 }

	 float r1 = p1->getColour(0), g1 = p1->getColour(1), b1 = p1->getColour(2);
	 float r2 = p2->getColour(0), g2 = p2->getColour(1), b2 = p2->getColour(2);

	 HSL hsl1 = rgbToHsl(r1, g1, b1);
	 HSL hsl2 = rgbToHsl(r2, g2, b2);

	 // Average hue (with wraparound)
	 float dh = std::fmod(hsl2.h - hsl1.h + 540.0f, 360.0f) - 180.0f;
	 float blendedH = std::fmod(hsl1.h + dh * 0.5f + 360.0f, 360.0f);

	 // Average saturation and lightness
	 float blendedS = (hsl1.s + hsl2.s) * 0.5f;
	 float blendedL = (hsl1.l + hsl2.l) * 0.5f;

	 HSL blendedHsl{ blendedH, blendedS, blendedL };
	 auto rgb = hslToRgb(blendedHsl);
	 float colour[3] = { rgb[0], rgb[1], rgb[2] };
	
    
	 Polygon result;
	 result.setVertices(outputList);
	 result.setColour(colour);

	 return result;
}

ImVec2 intersectingLines(ImVec2 a, ImVec2 b, ImVec2 p, ImVec2 q) {
	/*
	Given line segments AB and PQ, find point of intersection between AB and PQ if it exists
	If it does not return (-1,-1) which can't be shown on the canvas and represent a "garbage value"
	*/
	const float d = (a.x - b.x) * (p.y - q.y) - (a.y - b.y) * (p.x - q.x);

	float x = ((a.x * b.y - a.y * b.x) * (p.x - q.x) - (a.x - b.x) * (p.x * q.y - p.y * q.x)) / d;
	float y = ((a.x * b.y - a.y * b.x) * (p.y - q.y) - (a.y - b.y) * (p.x * q.y - p.y * q.x)) / d;

	return ImVec2(x, y);
}

ImVec2 intersectingSegments(ImVec2 a, ImVec2 b, ImVec2 p, ImVec2 q)
{
	// Calculate line intersection using the same formula as above
	float d = (a.x - b.x) * (p.y - q.y) - (a.y - b.y) * (p.x - q.x);
	if (d == 0) return ImVec2(-1, -1); // Lines are parallel
	
	float x = ((a.x * b.y - a.y * b.x) * (p.x - q.x) - (a.x - b.x) * (p.x * q.y - p.y * q.x)) / d;
	float y = ((a.x * b.y - a.y * b.x) * (p.y - q.y) - (a.y - b.y) * (p.x * q.y - p.y * q.x)) / d;
	
	const ImVec2 output = ImVec2(x, y);

	// Check if intersection point is within both line segments
	if (std::max(std::min(a.x, b.x), std::min(p.x, q.x)) <= output.x && 
		output.x <= std::min(std::max(a.x,b.x), std::max(p.x,q.x)) && 
		std::max(std::min(a.y, b.y), std::min(p.y, q.y)) <= output.y && 
		output.y <= std::min(std::max(a.y, b.y), std::max(p.y, q.y))) {

		return output;
	}

	return ImVec2(-1, -1);
}

float dotProduct(ImVec2 p, ImVec2 q) {
	return p.x * q.x + p.y * q.y;
}

float angle(ImVec2 p, ImVec2 q, ImVec2 r) {
	/*
	Returns the value of the angle PQR in radians
	PQ . QR = |PQ||QR|cos(PQR)
	*/
	const ImVec2 qp = { p.x - q.x,p.y - q.y };
	const ImVec2 qr = { r.x - q.x,r.y - q.y };

	const double a = sqrtf(pow(qp.x, 2) + pow(qp.y, 2));
	const double b = sqrtf(pow(qr.x, 2) + pow(qr.y, 2));

	return std::acosf(dotProduct(qp, qr) / (a*b));
}