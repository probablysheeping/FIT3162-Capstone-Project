#pragma once
#include <vector>
#include "imgui.h"
#include <SFML/Graphics.hpp>

struct Polygon {
    std::vector<ImVec2> vertices;
    float colour[3] = { 0.f, 0.f, 0.f };
    sf::ConvexShape render;
};

bool pointInPolygon(ImVec2 p, Polygon* polygon);