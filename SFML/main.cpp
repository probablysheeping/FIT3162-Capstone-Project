//Allows conversion between stuff like ImVec2 and Vector2i
// define glm::vecN or include it from another file
namespace glm
{
    struct vec2
    {
        float x, y;
        vec2(float x, float y) : x(x), y(y) {};
    };
    struct vec4
    {
        float x, y, z, w;
        vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {};
    };
}

// define extra conversion here before including imgui, don't do it in the imconfig.h
#define IM_VEC2_CLASS_EXTRA \
    constexpr ImVec2(glm::vec2& f) : x(f.x), y(f.y) {} \
    operator glm::vec2() const { return glm::vec2(x, y); }

#define IM_VEC4_CLASS_EXTRA \
        constexpr ImVec4(const glm::vec4& f) : x(f.x), y(f.y), z(f.z), w(f.w) {} \
        operator glm::vec4() const { return glm::vec4(x,y,z,w); }

#include "imgui.h" 

#include "imgui-SFML.h"
#include "polygon.h"
#include <SFML/Graphics.hpp>
// TODO: Set up boost.geometry

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
static bool selectedpolygon = false;

// This is the data for a polygon not the actual displayed shape


struct Status {
    bool drawPolygon = false;
    bool createPolygon = false;
};

float polygonColour[3] = { 0.f, 0.f, 0.f };


sf::ConvexShape drawPolygon(Polygon* polygon) {
    // Called to draw a polygon in SFML window
    sf::ConvexShape convex;
    int n = polygon->vertices.size(); // vertex count
    convex.setPointCount(n);

    convex.setFillColor(sf::Color((int)(polygonColour[0] * 255), (int)(polygonColour[1] * 255), (int)(polygonColour[2] * 255)));


    for (int i = 0; i < n; i++) {
        convex.setPoint(i, polygon->vertices.at(i));
    }

    return convex;
}



int main()
{
    // This is the SFML window where polygons will appear.
    // One can set the dimensions based on screen size.
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    sf::RenderWindow window(sf::VideoMode(ImVec2(1000, 800)), "Placeholder");
    window.setSize(sf::Vector2u(1000, 800));
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::Clock deltaClock;

    Status status;

    std::vector<Polygon> polygons;

    //Creating Polygon Variables
    Polygon newPolygon;
    std::vector<ImVec2> vertices;
    std::vector<sf::Vertex> newPolygonOutline;

    bool firstVertex = true;
    bool test = true;
    std::vector<int> selectedPolygons;

    float area = -1;

    while (window.isOpen())
    {

        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (const auto mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {

                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    if (status.createPolygon) {
                        ImVec2 mousepos = sf::Mouse::getPosition(window);
                        if (!firstVertex && distanceL2(mousepos, vertices.front()) <= 10) {
                            if (vertices.size() < 3) {
                                //TODO: some error message
                                std::cout << "congrats" << std::endl;
                            }
                            else {
                                //TODO: Adjust vertex locations as per requirements
                                newPolygon.vertices = vertices;

                                for (int i = 0; i < 3; i++) {
                                    newPolygon.colour[i] = polygonColour[i];
                                }
                                newPolygon.render = drawPolygon(&newPolygon);
                                polygons.push_back(newPolygon);

                                vertices.clear();
                                newPolygon = Polygon();
                                status.createPolygon = false;
                            }
                        }
                        else {

                            vertices.push_back(mousepos);

                            newPolygonOutline.back().position = mousepos;
                            newPolygonOutline.push_back(sf::Vertex{ mousepos, sf::Color::Black });
                            firstVertex = false;
                        }
                    }
                    else {
                        // Left click to select a polygon. We unfortunately need to check each one.
                        ImVec2 p = sf::Mouse::getPosition(window);
                        Polygon* polygon;
                        int i = 0;
                        for (i; i < polygons.size(); i++) {
                            polygon = &polygons.at(i);
                            if (std::find(selectedPolygons.begin(), selectedPolygons.end(), i)==selectedPolygons.end() && pointInPolygon(p, polygon)) {

                                selectedPolygons.push_back(i);
                                polygon -> render.setOutlineThickness(1.f);
                                polygon -> render.setOutlineColor(sf::Color::Cyan);
                                //polygon -> render.setFillColor(sf::Color((int)(polygonColour[0] * 255), (int)(polygonColour[1] * 255), (int)(polygonColour[2] * 255)));
                                break;
                            }

                        }
                        if (selectedPolygons.size() == 2) {
                            Polygon intersection = intersectingPolygon(&polygons.at(selectedPolygons.at(0)), &polygons.at(selectedPolygons.at(1)));
                            area = polygonArea(&polygons.at(selectedPolygons.at(0))) + polygonArea(&polygons.at(selectedPolygons.at(1))) - polygonArea(&intersection);
                        }
                        if (selectedPolygons.size() == 1) {
                            area = polygonArea(&polygons.at(selectedPolygons.at(0)));
                        }
                    }
                }
            }
        }



        ImGui::SFML::Update(window, deltaClock.restart());


        //TODO: Add functionality to main menu
        if (ImGui::BeginMainMenuBar()) {

            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open", "CTRL+O")) {}
                if (ImGui::MenuItem("Save", "CTRL+S")) {}
                if (ImGui::MenuItem("Save As", "CTRL+SHIFT+S")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Settings")) {}
                if (ImGui::MenuItem("Exit")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
                if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {} // Disabled item
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "CTRL+X")) {}
                if (ImGui::MenuItem("Copy", "CTRL+C")) {}
                if (ImGui::MenuItem("Paste", "CTRL+V")) {}
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();

        }

        // Window used for creating polygons
        // Needs to be formatted properly. This is just a placeholder UI
        ImGui::SetNextWindowSize(ImVec2(300, 350));
        if (ImGui::Begin("Polygon Creator")) {


            if (ImGui::Button("Create Polygon", ImVec2(120, 30))) {
                // Create Polygon
                status.createPolygon = true;

                vertices.clear();
                newPolygon = Polygon();
                newPolygonOutline.clear();
                newPolygonOutline.push_back(sf::Vertex{ ImVec2(0,0), sf::Color::Black });

                firstVertex = true;

            }
            if (ImGui::Button("Delete Polygon", ImVec2(120, 30))) {
                // Delete Polygon
                for (int i : selectedPolygons) {
                    std::cout << i << std::endl;
                    polygons.erase(polygons.begin() + i);
                    
                }
                selectedPolygons.clear();
            }

            if (ImGui::Button("Compute IoU", ImVec2(120, 30)) && selectedPolygons.size()==2) {
                Polygon intersection = polygons.at(selectedPolygons.at(0));
                for (int i = 1; i < selectedPolygons.size(); i++) {
                    intersection = intersectingPolygon(&intersection, &polygons.at(selectedPolygons.at(i)));
                }
                intersection.render = drawPolygon(&intersection);
                polygons.push_back(intersection);

                //TODO: Calculate IoU Metric and display result.
            }

            if (ImGui::Button("Clear Selected", ImVec2(120, 30))) {
                // User clicked the canvas, so we reset everything.
                for (int j : selectedPolygons) {
                    polygons.at(j).render.setOutlineThickness(0.f);
                }
                selectedPolygons.clear();
                area = -1;
            }

            if (ImGui::ColorPicker3("Select Colour", polygonColour)) {
                //Alter Polygon Colour
                if (!selectedPolygons.empty()) {
                    for (int i : selectedPolygons) {
                        polygons.at(i).render.setFillColor(sf::Color((int)(polygonColour[0] * 255), (int)(polygonColour[1] * 255), (int)(polygonColour[2] * 255)));
                    }
                }
            }

            ImGui::Text("Area:");
            ImGui::SameLine(); ImGui::Text(area == -1 ? "" : std::to_string(area).c_str());

        }

        ImGui::End();

        window.clear(sf::Color::White);

        //Draw everything here

        for (Polygon polygon : polygons) {
            window.draw(polygon.render);
        }

        if (status.createPolygon && !firstVertex) {
            // Draw boundary of supposed polygon
            ImVec2 mousepos = sf::Mouse::getPosition(window);
            newPolygonOutline.back().position = mousepos;
            window.draw(newPolygonOutline.data(), newPolygonOutline.size(), sf::PrimitiveType::LineStrip);
        }

        
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}