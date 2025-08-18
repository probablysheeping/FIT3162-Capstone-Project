#include "vectordefs.h"
#include "imgui-SFML.h"
#include "polygon.h"
#include "saving.h"
#include "filelocationchooser.h"

#include <SFML/Graphics.hpp>
// TODO: Set up boost.geometry
#include <iostream>
#include <string>


#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 800
#define FRAME_LIMIT 60
#define WINDOW_DISPLAY_NAME "Convex Polygon IoU"

static bool selectedpolygon = false;

// How long until we autosave
static const sf::Time autosaveTime = sf::seconds(20.f);

std::vector<ImVec2> adjustVertices(std::vector<ImVec2>& vertices) {
    /*
    For each angle check if it is close to 90, 60, 45, 30 or 0 degrees (or 180, etc i cbf typing them all out)
    Vertices MUST BE ORDERED CLOCKWISE
    */
    ImVec2 p, q, r;
    const float delta = 5;
    const int angles[] = { 30, 45, 60, 90, 120, 135, 150, 180 };
    const int n = vertices.size();
    ImVec2 unitvec; //not rly unit vec cuz its not magnitude 1 but pretend it is.
    float t, o1, o2, pqr;
    bool adjusted = false;
    float angle2, angle3, dot;

    ImVec2 qr;

    std::vector<ImVec2> result;
    std::cout << "new bug" << std::endl;
    for (int i = 0; i < n; i++) {
        p = vertices.at(i - 1 >= 0 ? i - 1 : i - 1 + n);
        q = vertices.at(i);
        r = vertices.at(i + 1 < n ? i + 1 : i + 1 - n);
        pqr = static_cast<float>(angle(p, q, r)*180/M_PI);
        std::cout << pqr << std::endl;
        for (int x : angles) {
            if (abs(abs(pqr) - x) <= delta) {

                // using vector projections
                // we need to get a vector in the direction of "where we want QR' to face" (where R' is adjusted vertex)

                if (i == 1) {
                    q = result.at(0);

                }
                else if (i > 1) {
                    q = result.at(i - 1);
                    p = result.at(i - 2);

                }

                o1 = sgn(sideOfLine(p, q, { q.x + 1, q.y }));
                o2 = sgn(sideOfLine(r, p, q));

                angle2 = angle(p, q, { q.x + 1,q.y }); //angle between [0, pi]
                angle3 = -o1*angle2 + o2 * x * M_PI / 180;
                
                unitvec = {std::cosf(angle3), -std::sinf(angle3)};


                //finally project QR onto Q + unitvec
                qr = { r.x - q.x, r.y - q.y };
                dot = dotProduct(qr, unitvec);

                result.push_back({ q.x + dot * unitvec.x, q.y + dot * unitvec.y });
                adjusted = true;
            }
        }

        if (!adjusted) {
            result.push_back(r);
        }


        adjusted = false;    
    }
    vertices = result;

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

    float polygonColour[3] = { 0.f, 0.f, 0.f };

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

    // Autosaving clock
    sf::Clock autosaveClock;

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

                                newPolygon.setVertices(vertices);

                                //orient vertices clockwise
                                if (sgn(newPolygon.signedArea()) == -1) {
                                    std::reverse(vertices.begin(), vertices.end());
                                }

                                adjustVertices(vertices);
                                newPolygon.setVertices(vertices);
                                newPolygon.setColour(polygonColour);
                              
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
                        int i;
                        for (i = 0; i < polygons.size(); i++) {
                            polygon = &polygons.at(i);
                            if (std::find(selectedPolygons.begin(), selectedPolygons.end(), i) == selectedPolygons.end() && polygon->pointInPolygon(p)) {

                                selectedPolygons.push_back(i);
                                polygon->render.setOutlineThickness(1.f);
                                polygon->render.setOutlineColor(sf::Color::Cyan);

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
                        else {
                            area = -1;
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
                if (ImGui::MenuItem("Open", "CTRL+O")) {
                    std::string openLocation = OpenFileDialog();
                    polygons = openFile(openLocation);
                    selectedPolygons.clear();
                }

                if (ImGui::MenuItem("Save", "CTRL+S")) {
                    quickSave(polygons, "\\save.sav");
                }

                if (ImGui::MenuItem("Save As", "CTRL+SHIFT+S")) {
                    std::string saveLocation = SaveFileDialog() + ".sav";
                    if (saveToFile(polygons, saveLocation))
                        std::cout << "Saved file successfully to " << saveLocation << std::endl;
                    else
                        std::cout << "Saved file un-successfully to" << saveLocation << std::endl;

                }
                ImGui::Separator();
                if (ImGui::MenuItem("Settings")) {}
                if (ImGui::MenuItem("Exit")) {
                    break; // Not sure if this is the best way to do this
                }
                ImGui::EndMenu();

            }
            
            ImGui::Separator();
            if (ImGui::MenuItem("Settings")) {
            
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

            if (ImGui::MenuItem("Exit")) {
                break; // Not sure if this is the best way to do this
            }

            ImGui::EndMainMenuBar();
        }
        // Autosaving functionality
        if (autosaveClock.getElapsedTime() >= autosaveTime) {
            quickSave(polygons, "\\autosave.sav");
            autosaveClock.restart();
        }

        // Autosaving functionality
        if (autosaveClock.getElapsedTime() >= autosaveTime) {
            quickSave(polygons, "\\autosave.sav");
            autosaveClock.restart();
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
                    polygons.erase(polygons.begin() + i);

                }
                selectedPolygons.clear();
            }

            if (ImGui::Button("Compute IoU", ImVec2(120, 30)) && selectedPolygons.size() == 2) {
                Polygon intersection = polygons.at(selectedPolygons.at(0));
                for (int i = 1; i < selectedPolygons.size(); i++) {
                    intersection = intersectingPolygon(&intersection, &polygons.at(selectedPolygons.at(i)));
                }

                intersection.setColour(polygonColour);
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
                        polygons.at(i).setColour(polygonColour);
                    }
                }
            }

            ImGui::Text("Area:");
            ImGui::SameLine(); ImGui::Text("%s", area == -1 ? "" : std::to_string(area).c_str());
            ImGui::Text("IoU metric:");
            ImGui::SameLine(); ImGui::Text("%s", IoUArea == -1 ? "" : std::to_string(IoUArea).c_str());

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
