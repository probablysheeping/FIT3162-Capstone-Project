#pragma once
#include <vector>
#include "imgui.h"
#include <SFML/Graphics.hpp>

class Polygon {
public:
    Polygon();
    ~Polygon();

    bool pointInPolygon(ImVec2 p);
    float signedArea();
    float polygonArea();

    void setVertices(std::vector<ImVec2> vertices);
    std::vector<ImVec2> getVertices();

    
    float getColour(int index);
    void setColour(float(&color)[3]);


    sf::ConvexShape render;

private:
    std::vector<ImVec2> vertices;
    float colour[3] = { 0.f, 0.f, 0.f };
};

int sgn(double x);
float distanceL2(ImVec2 p, ImVec2 q);
Polygon intersectingPolygon(Polygon* p1, Polygon* p2);
ImVec2 intersectingLines(ImVec2 a, ImVec2 b, ImVec2 p, ImVec2 q);
ImVec2 intersectingSegments(ImVec2 a, ImVec2 b, ImVec2 p, ImVec2 q);
float angle(ImVec2 p, ImVec2 q, ImVec2 r);


float dotProduct(ImVec2 p, ImVec2 q);
float sideOfLine(ImVec2 p, ImVec2 a, ImVec2 b);