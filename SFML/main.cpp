#include "vectordefs.h"
#include "imgui-SFML.h"
#include "polygon.h"
#include "saving.h"

#include <SFML/Graphics.hpp>
// TODO: Set up boost.geometry
#include <iostream>
#include <string>

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 800
#define FRAME_LIMIT 60
#define WINDOW_DISPLAY_NAME "Convex Polygon IoU"

static bool selectedpolygon = false;

std::vector<ImVec2> adjustVertices(std::vector<ImVec2> vertices) {
    /*
    For each angle check if it is close to 90, 60, 45, 30 or 0 degrees (or 180, etc i cbf typing them all out)

    */
    ImVec2 p, q, r;
    const float delta = 2;
    const int angles[] = { 0, 30, 45, 60, 90, 120, 135, 150, 180 };
    const int n = vertices.size();
    ImVec2 unitvec;
    double t;
    double pqr;
    bool adjusted = false;
    double angle2;

    std::vector<ImVec2> result;

    for (int i = 0; i < n; i++) {
        p = vertices.at(i);
        q = vertices.at(i + 1 < n ? i + 1 : i + 1 - n);
        r = vertices.at(i + 2 < n ? i + 2 : i + 2 - n);
        pqr = static_cast<float>(angle(p, q, r)*180/M_PI);
        for (int x : angles) {
            if (abs(abs(pqr) - x) <= delta) {
                // We will move q.
                angle2 = angle(ImVec2(1, 0), q, p);
                unitvec = { static_cast<float>(std::cos(angle2 + x)), static_cast<float>(std::sin(angle2 + x)) };
                t = ((r.x - p.x) * (q.y - p.y) - (r.y - p.y) * (q.x - p.x)) / (unitvec.y * (q.x - p.x) - unitvec.x * (p.y - q.y));
                result.push_back({ static_cast<float>(t * unitvec.x + r.x), static_cast<float>(t * unitvec.y + r.y) });
                adjusted = true;
            }
        }
        if (!adjusted) {
            result.push_back(q);
        }
        adjusted = false;    
    }
    
    return result;
}

/// <summary>
/// This is the SFML window where polygons will appear.
/// </summary>
/// <returns></returns>
int main()
{
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    // One can set the dimensions based on screen size.
    sf::RenderWindow window(sf::VideoMode(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT)), WINDOW_DISPLAY_NAME);
    window.setSize(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT));
    window.setFramerateLimit(FRAME_LIMIT);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::Clock deltaClock;

    // This is the data for a polygon not the actual displayed shape
    struct {
        bool drawPolygon = false;
        bool createPolygon = false;
    } status;

    float polygonColour[3] = {0.f, 0.f, 0.f};
   
    std::vector<Polygon> polygons;

    // Creating Polygon Variables
    Polygon newPolygon;
    std::vector<ImVec2> vertices;
    std::vector<sf::Vertex> newPolygonOutline;

    bool firstVertex = true;
    bool test = true;
    std::vector<int> selectedPolygons;

    double area = -1;
    double IoUArea = -1;

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {

                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    if (status.createPolygon) {
                        ImVec2 mousepos = sf::Mouse::getPosition(window);
                        if (!firstVertex && distanceL2(mousepos, vertices.front()) <= 10) {
                            if (vertices.size() < 3) {
                                std::cout << "ERROR MESSAGE" << std::endl;
                            }
                            else {
                                //TODO: Adjust vertex locations as per requirements
                                //newPolygon.vertices = adjustVertices(vertices);
                                newPolygon.setVertices(vertices);
                                newPolygon.setColour(polygonColour[0], polygonColour[1], polygonColour[2]);
                                newPolygon.drawPolygon();
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
                            if (std::find(selectedPolygons.begin(), selectedPolygons.end(), i)==selectedPolygons.end() && polygon->pointInPolygon(p)) {

                                selectedPolygons.push_back(i);
                                polygon->render.setOutlineThickness(1.f);
                                polygon->render.setOutlineColor(sf::Color::Cyan);
                                //polygon -> render.setFillColor(sf::Color((int)(polygonColour[0] * 255), (int)(polygonColour[1] * 255), (int)(polygonColour[2] * 255)));
                                break;
                            }

                        }
                        if (selectedPolygons.size() == 2) {
                            Polygon intersection = intersectingPolygon(&polygons.at(selectedPolygons.at(0)), &polygons.at(selectedPolygons.at(1)));

                            area = polygons.at(selectedPolygons.at(0)).polygonArea() + polygons.at(selectedPolygons.at(1)).polygonArea() - intersection.polygonArea();
                        }
                        if (selectedPolygons.size() == 1) {
                            area = polygons.at(selectedPolygons.at(0)).polygonArea();
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
                if (ImGui::MenuItem("Save", "CTRL+S")) {
                    std::string saveLocation = getExecutablePath();
                    if (saveLocation != NULL_SAVE_PATH) {
                        saveLocation += "\\save.sav";
                        if (saveToFile(polygons, saveLocation))
                            std::cout << "Saved file successfully to " << saveLocation << std::endl;
                        else
                            std::cout << "Saved file un-successfully to" << saveLocation << std::endl;
                    }
                }
                if (ImGui::MenuItem("Save As", "CTRL+SHIFT+S")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Settings")) {}
                if (ImGui::MenuItem("Exit")) {
                    break; // Not sure if this is the best way to do this
                }
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
        ImGui::SetNextWindowSize(ImVec2(350, 600));
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

                intersection.setColour(polygonColour[0], polygonColour[1], polygonColour[2]);
                intersection.drawPolygon();
                polygons.push_back(intersection);

                // TODO: Calculate IoU Metric and display result.
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
            ImGui::Text("IoU metric:");
            ImGui::SameLine(); ImGui::Text(IoUArea == -1 ? "" : std::to_string(IoUArea).c_str());

        }

        ImGui::End();

        window.clear(sf::Color::White);

        // Draw everything here

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
