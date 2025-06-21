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

#include <SFML/Graphics.hpp>
// TODO: Set up boost.geometry

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
static bool selectedpolygon = false;

// This is the data for a polygon not the actual displayed shape
struct Polygon {
    std::vector<ImVec2> vertices;
    float colour[3] = {0.f, 0.f, 0.f};
};

struct Status {
    bool drawPolygon=false;
    bool createPolygon=false;
};

float polygonColour[3] = { 0.5f, 0.1f, 0.3f };


sf::ConvexShape drawPolygon(Polygon* polygon) {
    // Called to draw a polygon in SFML window
    sf::ConvexShape convex;
    int n = polygon -> vertices.size(); // vertex count
    convex.setPointCount(n);

    convex.setFillColor(sf::Color((int)(polygonColour[0]*255), (int)(polygonColour[1] * 255), (int)(polygonColour[2] * 255)));
    
    
    for (int i = 0; i < n; i++) {
        convex.setPoint(i, polygon -> vertices.at(i));
    }

    return convex;
}

double distanceL2(ImVec2 p, ImVec2 q) {
    // distance using L2 metric
    return sqrt(pow(p.x - q.x, 2) + pow(p.y - q.y, 2));
}



int main()
{   
    // This is the SFML window where polygons will appear.
    // One can set the dimensions based on screen size.
    sf::RenderWindow window(sf::VideoMode(ImVec2(1000,800)), "Placeholder");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::Clock deltaClock;

    Status status;


    std::vector<sf::ConvexShape> drawQueue;
    std::vector<Polygon> polygons;

    //Creating Polygon Variables
    Polygon polygon;
    std::vector<ImVec2> vertices;
    std::vector<sf::CircleShape> vertexDisplays;
    
    bool firstVertex = true;


    
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

                if (mouseButtonPressed->button == sf::Mouse::Button::Left && status.createPolygon) {
                    ImVec2 mousepos = sf::Mouse::getPosition(window);
                    if (!firstVertex && distanceL2(mousepos, vertices.front()) <= 10) {
                        if (vertices.size() < 3) {
                            //TODO: some error message
                            std::cout << "congrats" << std::endl;
                        }
                        else {
                            //TODO: Adjust vertex locations as per requirements
                            polygon.vertices = vertices;

                            for (int i = 0; i < 3; i++) {
                                polygon.colour[i] = polygonColour[i];
                            }

                            polygons.push_back(polygon);
                            sf::ConvexShape toRender = drawPolygon(&polygon);
                            drawQueue.push_back(toRender);

                            
                            vertexDisplays.clear();
                            vertices.clear();
                            polygon = Polygon();
                            status.createPolygon = false;
                        }
                    }
                    else {
                        
                        vertices.push_back(mousepos);

                        vertexDisplays.push_back(sf::CircleShape(2.f));

                        vertexDisplays.back().setFillColor(sf::Color(0,0,0));
                        vertexDisplays.back().setPosition(mousepos);
                        firstVertex = false;
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

                firstVertex = true;
                
            }
            if (ImGui::Button("Delete Polygon", ImVec2(120,30))) {
                // Delete Polygon
            }

            if (ImGui::ColorPicker3("Select Colour", polygonColour)) {
                //Alter Polygon Colour
                std::cout << polygonColour << std::endl;
            }
        }

        ImGui::End();
        
        window.clear(sf::Color::White);
        for (int i = 0; i < drawQueue.size(); i++) {
            window.draw(drawQueue.at(i));
        }
        for (int i = 0; i < vertexDisplays.size(); i++) {
            window.draw(vertexDisplays.at(i));
        }
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}

