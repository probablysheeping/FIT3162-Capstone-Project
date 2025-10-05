#include "vectordefs.h"
#include "imgui-SFML.h"
#include "polygon.h"
#include "saving.h"
#include "filelocationchooser.h"
#include "logging.h"
#include "tutorial.h"

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
static const sf::Time autosaveTime = sf::seconds(30.f);

void adjustVertices(std::vector<ImVec2>& vertices) {
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
    for (int i = 0; i < n; i++) {
        p = vertices.at(i - 1 >= 0 ? i - 1 : i - 1 + n);
        q = vertices.at(i);
        r = vertices.at(i + 1 < n ? i + 1 : i + 1 - n);
        pqr = static_cast<float>(angle(p, q, r)*180/M_PI);
        logger << currentDateTime() << " PQR: " << pqr << std::endl;
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

void createToolTip(const char* toolTipStr, bool tooltipsEnabled)
{
    if (ImGui::IsItemHovered() && tooltipsEnabled) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", toolTipStr);
        ImGui::EndTooltip();
    }
}

/// <summary>
/// Draws a grid background on the window
/// </summary>
/// <param name="window">The SFML render window</param>
/// <param name="gridSize">Size of each grid cell in pixels</param>
/// <param name="gridColor">Color of the grid lines</param>
void drawGrid(sf::RenderWindow& window, float gridSize = 50.0f, sf::Color gridColor = sf::Color(220, 220, 220))
{
    sf::Vector2u windowSize = window.getSize();
    std::vector<sf::Vertex> gridLines;
    
    // Vertical lines
    for (float x = 0; x <= windowSize.x; x += gridSize) {
        sf::Vertex v1, v2;
        v1.position = sf::Vector2f(x, 0);
        v1.color = gridColor;
        v2.position = sf::Vector2f(x, windowSize.y);
        v2.color = gridColor;
        gridLines.push_back(v1);
        gridLines.push_back(v2);
    }
    
    // Horizontal lines
    for (float y = 0; y <= windowSize.y; y += gridSize) {
        sf::Vertex v1, v2;
        v1.position = sf::Vector2f(0, y);
        v1.color = gridColor;
        v2.position = sf::Vector2f(windowSize.x, y);
        v2.color = gridColor;
        gridLines.push_back(v1);
        gridLines.push_back(v2);
    }
    
    window.draw(gridLines.data(), gridLines.size(), sf::PrimitiveType::Lines);
}

/// <summary>
/// Loads a background image from file with specified resize mode
/// </summary>
/// <param name="texture">The texture to load the image into</param>
/// <param name="sprite">The sprite to display the image</param>
/// <param name="filename">Path to the image file</param>
/// <param name="windowSize">Size of the window for scaling</param>
/// <param name="resizeMode">0: Fit to window, 1: Original size, 2: Custom size</param>
/// <param name="customWidth">Custom width (used when resizeMode = 2)</param>
/// <param name="customHeight">Custom height (used when resizeMode = 2)</param>
/// <returns>True if image loaded successfully</returns>
bool loadBackgroundImage(sf::Texture& texture, sf::Sprite& sprite, const std::string& filename, sf::Vector2u windowSize, int resizeMode = 0, float customWidth = 800.0f, float customHeight = 600.0f)
{
    if (!texture.loadFromFile(filename)) {
        logger << currentDateTime() << " ERROR: Failed to load image from " << filename << std::endl;
        return false;
    }
    
    sprite.setTexture(texture);
    
    sf::Vector2u imageSize = texture.getSize();
    float scaleX = 1.0f, scaleY = 1.0f;
    sf::Vector2f position(0.0f, 0.0f);
    
    switch (resizeMode) {
        case 0: // Fit to window
        {
            scaleX = static_cast<float>(windowSize.x) / imageSize.x;
            scaleY = static_cast<float>(windowSize.y) / imageSize.y;
            float scale = std::min(scaleX, scaleY);
            scaleX = scaleY = scale;
            
            // Center the image
            sf::Vector2f scaledSize = sf::Vector2f(imageSize.x * scale, imageSize.y * scale);
            position = sf::Vector2f((windowSize.x - scaledSize.x) / 2.0f, (windowSize.y - scaledSize.y) / 2.0f);
            break;
        }
        case 1: // Original size
        {
            scaleX = scaleY = 1.0f;
            // Center the image
            position = sf::Vector2f((windowSize.x - imageSize.x) / 2.0f, (windowSize.y - imageSize.y) / 2.0f);
            break;
        }
        case 2: // Custom size
        {
            scaleX = customWidth / imageSize.x;
            scaleY = customHeight / imageSize.y;
            // Center the image
            position = sf::Vector2f((windowSize.x - customWidth) / 2.0f, (windowSize.y - customHeight) / 2.0f);
            break;
        }
    }
    
    sprite.setScale(sf::Vector2f(scaleX, scaleY));
    sprite.setPosition(position);
    
    logger << currentDateTime() << " Image loaded successfully from " << filename << " with resize mode " << resizeMode << std::endl;
    return true;
}

/// <summary>
/// Gets the bounding rectangle of the current image
/// </summary>
sf::FloatRect getImageBounds(sf::Sprite* sprite) {
    if (!sprite) return sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(0, 0));
    return sprite->getGlobalBounds();
}

/// <summary>
/// Gets resize handle rectangles for the image
/// </summary>
std::vector<sf::FloatRect> getResizeHandles(sf::FloatRect imageBounds, float handleSize) {
    std::vector<sf::FloatRect> handles;
    float hs = handleSize / 2.0f;
    
    // Corner handles (0-3)
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x - hs, imageBounds.position.y - hs), sf::Vector2f(handleSize, handleSize))); // Top-left
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x + imageBounds.size.x - hs, imageBounds.position.y - hs), sf::Vector2f(handleSize, handleSize))); // Top-right
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x + imageBounds.size.x - hs, imageBounds.position.y + imageBounds.size.y - hs), sf::Vector2f(handleSize, handleSize))); // Bottom-right
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x - hs, imageBounds.position.y + imageBounds.size.y - hs), sf::Vector2f(handleSize, handleSize))); // Bottom-left
    
    // Edge handles (4-7)
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x + imageBounds.size.x/2 - hs, imageBounds.position.y - hs), sf::Vector2f(handleSize, handleSize))); // Top
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x + imageBounds.size.x - hs, imageBounds.position.y + imageBounds.size.y/2 - hs), sf::Vector2f(handleSize, handleSize))); // Right
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x + imageBounds.size.x/2 - hs, imageBounds.position.y + imageBounds.size.y - hs), sf::Vector2f(handleSize, handleSize))); // Bottom
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x - hs, imageBounds.position.y + imageBounds.size.y/2 - hs), sf::Vector2f(handleSize, handleSize))); // Left
    
    return handles;
}

/// <summary>
/// Draws resize handles for the image
/// </summary>
void drawResizeHandles(sf::RenderWindow& window, sf::FloatRect imageBounds, float handleSize) {
    auto handles = getResizeHandles(imageBounds, handleSize);
    
    for (const auto& handle : handles) {
        sf::RectangleShape handleRect(sf::Vector2f(handleSize, handleSize));
        handleRect.setPosition(handle.position);
        handleRect.setFillColor(sf::Color::White);
        handleRect.setOutlineColor(sf::Color::Black);
        handleRect.setOutlineThickness(1.0f);
        window.draw(handleRect);
    }
    
    // Draw selection border
    sf::RectangleShape border(imageBounds.size);
    border.setPosition(imageBounds.position);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color::Blue);
    border.setOutlineThickness(2.0f);
    window.draw(border);
}

/// <summary>
/// Creates an ImageState from current application state
/// </summary>
ImageState getCurrentImageState(bool hasImage, const std::string& imagePath, sf::Sprite* sprite, float opacity, bool enabled) {
    ImageState state;
    state.hasImage = hasImage;
    state.imagePath = imagePath;
    state.opacity = opacity;
    state.enabled = enabled;
    
    if (hasImage && sprite) {
        sf::Vector2f position = sprite->getPosition();
        sf::Vector2f scale = sprite->getScale();
        state.positionX = position.x;
        state.positionY = position.y;
        state.scaleX = scale.x;
        state.scaleY = scale.y;
    }
    
    return state;
}

/// <summary>
/// This is the SFML window where polygons will appear.
/// </summary>
void runProgram()
{
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    // One can set the dimensions based on screen size.
    sf::RenderWindow window(sf::VideoMode(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT)), WINDOW_DISPLAY_NAME);
    window.setSize(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT));
    window.setFramerateLimit(FRAME_LIMIT);
    if (!ImGui::SFML::Init(window))
        throw std::runtime_error("SFML Window could not initialise!");

    sf::Clock deltaClock;

    // This is the data for a polygon not the actual displayed shape
    struct {
        bool drawPolygon = false;
        bool createPolygon = false;
    } status;

    // Settings
    bool logSavingEnabled = true;
    bool autosaveEnabled = true;
    bool fullscreenEnabled = false;
    bool tooltipsEnabled = true;
    bool gridEnabled = true;
    float gridSize = 50.0f;
    bool imageEnabled = false;
    float imageOpacity = 0.7f;
    float imageScale = 1.0f;

    float polygonColour[3] = { 0.f, 0.f, 0.f };

    std::vector<Polygon> polygons;

    // Background image variables
    sf::Texture backgroundTexture;
    sf::Sprite* backgroundSprite = nullptr;
    bool hasBackgroundImage = false;
    std::string currentImagePath = "";
    
    // Image resize dialog variables
    bool showImageResizeDialog = false;
    std::string pendingImagePath = "";
    int resizeMode = 0; // 0: Fit to window, 1: Original size, 2: Custom size
    float customWidth = 800.0f;
    float customHeight = 600.0f;
    sf::Vector2u originalImageSize;
    
    // Interactive image resize variables
    bool imageResizeMode = false;
    bool isDraggingImage = false;
    int dragHandle = -1; // -1: none, 0-3: corners, 4-7: edges, 8: move
    sf::Vector2f dragStartPos;
    sf::Vector2f imageStartPos;
    sf::Vector2f imageStartSize;
    const float handleSize = 8.0f;

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

    // Setup tutorial
    Tutorial tutorial;
    tutorial.addStep("Welcome", "Welcome to Convex Polygon IoU. Let's run through how this program works!");
    tutorial.addStep("Selecting a Colour", "Before choosing a polygon, you must first select its colour.\n\nThe program provides several ways to select your desired colour:\n\n1. Colour Picker\n    - On the left you will see a colour square.\n    - Click anywhere inside the square to pick a base colour.\n    - The selected colour will appear in the \"Select Colour\" preview box to the right.\n\n2. Using the RGB and HSV Sliders\n    - Below the colour picker, there are boxes for RGB and HSV values. Each box acts as a slider. Drag to increase/decrease values (0-255).\n\n3. Hex Codes\n    - At the bottom, there is a hexidecimal input field.\n    - Click the box and type your desired hex value, the colour will actively update as you type.");
    tutorial.addStep("Drawing a Polygon", "After choosing a colour, the nexts step is to draw your polygon.\nThis is done by selecting vertices on the canvas.\n\n1. Optional: Import Background Image\n    - Go to File > Import Image to load an image as a background reference.\n    - Choose from resize options: Fit to Window, Original Size, or Custom Size.\n    - Enable 'Image Resize Mode' in Settings for interactive mouse resizing.\n    - Use Settings menu to adjust image opacity and scale after importing.\n    - Your image settings are automatically saved with your project!\n\n2. Placing vertices\n    - Click anywhere on the canvas to place the first vertex.\n    - Continue clicking to add additional vertices. Each click will mark a new corner of your polygon.\n\n3. Connecting the Shape\n    - As vertices are added, lines will automatically connect them.\n    - When you click back on the origin vertex, the outline closes and the polygon is created.\n\nNote that at any point after your polygon is created, you can change the colour. Simply select the polygon you wish to recolour and go back through the steps as in the previous page.");
    tutorial.addStep("Calculating the Intersection over Union", "Once two polygons have been created, we can measure their IoU value.\n\n1. Selecting Polygons\n    - Click on the first polygon you wish to calculate the IoU value for. The selected polygon will be outlined with a cyan line edge.\n    - Select the second polygon.\n\n2. Click the \"Compute IoU\" button.\n    - The calculated IoU will be printed in the bottom of the polygon creator menu.");
    tutorial.addStep("Deleting Polygons", "Deleting a polygon is as simple as selecting the single polygon you wish to delete (outlined in cyan), and clicking the \"Delete Polygon\" button in the menu.");
    tutorial.addStep("End", "You have successfully completed the tutorial! If you want to go through this tutorial again please press the 'Tutorial' button on the main menu bar.");

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {

                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    ImVec2 mousepos = sf::Mouse::getPosition(window);
                    
                    // Handle image resize mode
                    if (imageResizeMode && hasBackgroundImage && imageEnabled) {
                        sf::FloatRect imageBounds = getImageBounds(backgroundSprite);
                        auto handles = getResizeHandles(imageBounds, handleSize);
                        
                        // Check if clicking on a resize handle
                        for (int i = 0; i < handles.size(); i++) {
                            if (handles[i].contains(sf::Vector2f(mousepos.x, mousepos.y))) {
                                isDraggingImage = true;
                                dragHandle = i;
                                dragStartPos = mousepos;
                                imageStartPos = backgroundSprite->getPosition();
                                imageStartSize = imageBounds.size;
                                break;
                            }
                        }
                        
                        // Check if clicking inside image for moving
                        if (!isDraggingImage && imageBounds.contains(sf::Vector2f(mousepos.x, mousepos.y))) {
                            isDraggingImage = true;
                            dragHandle = 8; // Move mode
                            dragStartPos = mousepos;
                            imageStartPos = backgroundSprite->getPosition();
                        }
                        
                        // If we're in resize mode and clicked somewhere, don't create polygons
                        if (isDraggingImage) {
                            continue;
                        }
                    }
                    
                    
                    if (status.createPolygon) {
                        ImVec2 mousepos = sf::Mouse::getPosition(window);
                        if (!firstVertex && distanceL2(mousepos, vertices.front()) <= 10) {
                            if (vertices.size() < 3) {
                                logger << currentDateTime() << " ERROR: Number of vertices less than three and instead is " << vertices.size() << std::endl;
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

                                logger << currentDateTime() << " NEW " << newPolygon;

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

                            logger << currentDateTime() << " Intersection " << intersection;

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
            
            if (const auto mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseButtonReleased->button == sf::Mouse::Button::Left) {
                    isDraggingImage = false;
                    dragHandle = -1;
                }
            }
            
            if (const auto mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                if (isDraggingImage && hasBackgroundImage && backgroundSprite) {
                    ImVec2 currentPos = sf::Mouse::getPosition(window);
                    sf::Vector2f delta = sf::Vector2f(currentPos.x - dragStartPos.x, currentPos.y - dragStartPos.y);
                    
                    if (dragHandle == 8) { // Move mode
                        backgroundSprite->setPosition(imageStartPos + delta);
                    } else if (dragHandle >= 0 && dragHandle <= 7) { // Resize handles
                        sf::Vector2f newScale = backgroundSprite->getScale();
                        sf::Vector2f newPos = backgroundSprite->getPosition();
                        
                        // Calculate new scale based on handle
                        sf::Vector2u textureSize = backgroundTexture.getSize();
                        float scaleFactorX = 1.0f;
                        float scaleFactorY = 1.0f;
                        
                        switch (dragHandle) {
                            case 0: // Top-left corner
                                scaleFactorX = (imageStartSize.x - delta.x) / textureSize.x;
                                scaleFactorY = (imageStartSize.y - delta.y) / textureSize.y;
                                newPos = imageStartPos + delta;
                                break;
                            case 1: // Top-right corner
                                scaleFactorX = (imageStartSize.x + delta.x) / textureSize.x;
                                scaleFactorY = (imageStartSize.y - delta.y) / textureSize.y;
                                newPos = sf::Vector2f(imageStartPos.x, imageStartPos.y + delta.y);
                                break;
                            case 2: // Bottom-right corner
                                scaleFactorX = (imageStartSize.x + delta.x) / textureSize.x;
                                scaleFactorY = (imageStartSize.y + delta.y) / textureSize.y;
                                break;
                            case 3: // Bottom-left corner
                                scaleFactorX = (imageStartSize.x - delta.x) / textureSize.x;
                                scaleFactorY = (imageStartSize.y + delta.y) / textureSize.y;
                                newPos = sf::Vector2f(imageStartPos.x + delta.x, imageStartPos.y);
                                break;
                            case 4: // Top edge
                                scaleFactorY = (imageStartSize.y - delta.y) / textureSize.y;
                                newPos = sf::Vector2f(imageStartPos.x, imageStartPos.y + delta.y);
                                scaleFactorX = backgroundSprite->getScale().x;
                                break;
                            case 5: // Right edge
                                scaleFactorX = (imageStartSize.x + delta.x) / textureSize.x;
                                scaleFactorY = backgroundSprite->getScale().y;
                                break;
                            case 6: // Bottom edge
                                scaleFactorY = (imageStartSize.y + delta.y) / textureSize.y;
                                scaleFactorX = backgroundSprite->getScale().x;
                                break;
                            case 7: // Left edge
                                scaleFactorX = (imageStartSize.x - delta.x) / textureSize.x;
                                scaleFactorY = backgroundSprite->getScale().y;
                                newPos = sf::Vector2f(imageStartPos.x + delta.x, imageStartPos.y);
                                break;
                        }
                        
                        // Prevent negative scaling
                        scaleFactorX = std::max(0.1f, scaleFactorX);
                        scaleFactorY = std::max(0.1f, scaleFactorY);
                        
                        backgroundSprite->setScale(sf::Vector2f(scaleFactorX, scaleFactorY));
                        backgroundSprite->setPosition(newPos);
                        
                        // Update the global imageScale to match the sprite's scale
                        imageScale = scaleFactorX; // Assuming uniform scaling
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
                    auto result = openFile(openLocation);
                    polygons = result.first;
                    ImageState loadedImageState = result.second;
                    selectedPolygons.clear();

                    logger << currentDateTime() << " File opened from " << openLocation << std::endl;

                    logger << "Polygons in file: \n";
                    for (Polygon polygon : polygons)
                        logger << polygon;

                    // Restore image state if present
                    if (loadedImageState.hasImage && !loadedImageState.imagePath.empty()) {
                        if (backgroundSprite) {
                            delete backgroundSprite;
                        }
                        backgroundSprite = new sf::Sprite(backgroundTexture);
                        
                        if (backgroundTexture.loadFromFile(loadedImageState.imagePath)) {
                            backgroundSprite->setTexture(backgroundTexture);
                            backgroundSprite->setPosition(sf::Vector2f(loadedImageState.positionX, loadedImageState.positionY));
                            backgroundSprite->setScale(sf::Vector2f(loadedImageState.scaleX, loadedImageState.scaleY));
                            
                            // Set the initial color/opacity
                            sf::Color imageColor = sf::Color::White;
                            imageColor.a = static_cast<std::uint8_t>(loadedImageState.opacity * 255);
                            backgroundSprite->setColor(imageColor);
                            
                            hasBackgroundImage = true;
                            currentImagePath = loadedImageState.imagePath;
                            imageOpacity = loadedImageState.opacity;
                            imageEnabled = loadedImageState.enabled;
                            imageScale = loadedImageState.scaleX; // Update global scale to match loaded scale
                            
                            logger << currentDateTime() << " Image restored from " << loadedImageState.imagePath << std::endl;
                        } else {
                            logger << currentDateTime() << " ERROR: Failed to load saved image from " << loadedImageState.imagePath << std::endl;
                            hasBackgroundImage = false;
                            currentImagePath = "";
                        }
                    } else {
                        // Clear any existing image
                        if (backgroundSprite) {
                            delete backgroundSprite;
                            backgroundSprite = nullptr;
                        }
                        hasBackgroundImage = false;
                        currentImagePath = "";
                        imageEnabled = false;
                    }
                }

                if (ImGui::MenuItem("Import Image")) {
                    std::string imageLocation = OpenImageDialog();
                    if (!imageLocation.empty()) {
                        // Load texture temporarily to get original size
                        sf::Texture tempTexture;
                        if (tempTexture.loadFromFile(imageLocation)) {
                            pendingImagePath = imageLocation;
                            originalImageSize = tempTexture.getSize();
                            customWidth = static_cast<float>(originalImageSize.x);
                            customHeight = static_cast<float>(originalImageSize.y);
                            showImageResizeDialog = true;
                        }
                    }
                }
                createToolTip("Import an image to draw polygons on top of", tooltipsEnabled);

                if (ImGui::MenuItem("Save", "CTRL+S")) {
                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                    quickSave(polygons, imageState, "save.sav");
                }

                if (ImGui::MenuItem("Save As", "CTRL+SHIFT+S")) {
                    std::string saveLocation = SaveFileDialog() + ".sav";
                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                    if (saveToFile(polygons, imageState, saveLocation))
                        logger << currentDateTime() << " Saved file successfully to " << saveLocation << std::endl;
                    else
                        logger << currentDateTime() << " Saved file un-successfully to " << saveLocation << std::endl;

                }
                ImGui::EndMenu();

            }
            createToolTip("Save and open files", tooltipsEnabled);

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
            createToolTip("Modify program state", tooltipsEnabled);

            if (ImGui::BeginMenu("Settings")) {
                ImGui::MenuItem("Logging", nullptr, &logSavingEnabled);
                ImGui::MenuItem("Autosaving", nullptr, &autosaveEnabled);
                ImGui::MenuItem("Tooltips", nullptr, &tooltipsEnabled);
                ImGui::MenuItem("Grid", nullptr, &gridEnabled);
                ImGui::SliderFloat("Grid Size", &gridSize, 10.0f, 100.0f);
                ImGui::Separator();
                if (hasBackgroundImage) {
                    ImGui::MenuItem("Show Image", nullptr, &imageEnabled);
                    ImGui::MenuItem("Image Resize Mode", nullptr, &imageResizeMode);
                    createToolTip("Enable interactive resizing with mouse drag handles", tooltipsEnabled);
                    ImGui::SliderFloat("Image Opacity", &imageOpacity, 0.1f, 1.0f);
                    ImGui::SliderFloat("Image Scale", &imageScale, 0.1f, 3.0f);
                }
                if (ImGui::MenuItem("Full Screen", nullptr, &fullscreenEnabled)) {
                    logger << currentDateTime() << " Full screen " << (fullscreenEnabled ? "enabled." : "disabled.") << std::endl;
                    window.close();
                    if (fullscreenEnabled)
                        window.create(sf::VideoMode::getDesktopMode(), WINDOW_DISPLAY_NAME, sf::State::Fullscreen);
                    else
                        window.create(sf::VideoMode::getDesktopMode(), WINDOW_DISPLAY_NAME, sf::State::Windowed);
                }
                ImGui::EndMenu();
            }
            createToolTip("Change program functionality", tooltipsEnabled);

            if (ImGui::MenuItem("Tutorial")) {
                logger << currentDateTime() << " Tutorial started.\n";
                tutorial.start();
            }
            createToolTip("Start tutorial", tooltipsEnabled);

            if (ImGui::MenuItem("Exit")) {
                logger << currentDateTime() << " User exit.\n";
                window.close();
            }
            createToolTip("Exit from program", tooltipsEnabled);

            ImGui::EndMainMenuBar();
        }
        // Autosaving functionality
        if (autosaveClock.getElapsedTime() >= autosaveTime) {
            ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
            quickSave(polygons, imageState, "autosave.sav");
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
            createToolTip("Click on canvas to create vertices", tooltipsEnabled);
            if (ImGui::Button("Delete Polygon", ImVec2(120, 30))) {
                // Delete Polygon
                for (int i : selectedPolygons) {
                    polygons.erase(polygons.begin() + i);

                }
                selectedPolygons.clear();
                logger << currentDateTime() << " Polygon deleted.\n";
            }
            createToolTip("Select polygon and then click delete", tooltipsEnabled);

            if (ImGui::Button("Compute IoU", ImVec2(120, 30)) && selectedPolygons.size() == 2) {
                // Save when computing IoU
                if (autosaveEnabled) {
                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                    quickSave(polygons, imageState, "autosave.sav");
                }

                Polygon intersection = polygons.at(selectedPolygons.at(0));
                for (int i = 1; i < selectedPolygons.size(); i++) {
                    intersection = intersectingPolygon(&intersection, &polygons.at(selectedPolygons.at(i)));
                }

                polygons.push_back(intersection);

                // TODO: Calculate IoU Metric and display result.
                IoUArea = intersection.polygonArea() / (polygons.at(selectedPolygons.at(0)).polygonArea() + polygons.at(selectedPolygons.at(1)).polygonArea());
            }
            createToolTip("Select polygons and then calculate", tooltipsEnabled);

            if (ImGui::Button("Clear Selected", ImVec2(120, 30))) {
                // User clicked the canvas, so we reset everything.
                for (int j : selectedPolygons) {
                    polygons.at(j).render.setOutlineThickness(0.f);
                }
                selectedPolygons.clear();
                area = -1;
            }
            createToolTip("Unselect all polygons", tooltipsEnabled);

            if (ImGui::ColorPicker3("Select Colour", polygonColour)) {
                //Alter Polygon Colour
                if (!selectedPolygons.empty()) {
                    for (int i : selectedPolygons) {
                        polygons.at(i).setColour(polygonColour);
                    }
                }
            }
            createToolTip("Change selected polygon colour", tooltipsEnabled);

            ImGui::Text("Area:");
            ImGui::SameLine(); ImGui::Text("%s", area == -1 ? "" : std::to_string(area).c_str());
            ImGui::Text("IoU metric:");
            ImGui::SameLine(); ImGui::Text("%s", IoUArea == -1 ? "" : std::to_string(IoUArea).c_str());

        }

        ImGui::End();

        // Image Resize Dialog
        if (showImageResizeDialog) {
            ImGui::SetNextWindowSize(ImVec2(400, 300));
            ImGui::SetNextWindowPos(ImVec2(window.getSize().x / 2 - 200, window.getSize().y / 2 - 150));
            if (ImGui::Begin("Import Image - Resize Options", &showImageResizeDialog, ImGuiWindowFlags_NoResize)) {
                
                ImGui::Text("Original Size: %dx%d", originalImageSize.x, originalImageSize.y);
                ImGui::Text("Window Size: %dx%d", window.getSize().x, window.getSize().y);
                ImGui::Separator();
                
                ImGui::Text("Choose resize option:");
                ImGui::RadioButton("Fit to Window (maintain aspect ratio)", &resizeMode, 0);
                createToolTip("Scale image to fit window while keeping proportions", tooltipsEnabled);
                
                ImGui::RadioButton("Original Size (no scaling)", &resizeMode, 1);
                createToolTip("Keep image at its original pixel dimensions", tooltipsEnabled);
                
                ImGui::RadioButton("Custom Size", &resizeMode, 2);
                createToolTip("Specify exact width and height for the image", tooltipsEnabled);
                
                if (resizeMode == 2) {
                    ImGui::Indent();
                    ImGui::SliderFloat("Width", &customWidth, 50.0f, static_cast<float>(window.getSize().x * 2));
                    ImGui::SliderFloat("Height", &customHeight, 50.0f, static_cast<float>(window.getSize().y * 2));
                    
                    if (ImGui::Button("Reset to Original")) {
                        customWidth = static_cast<float>(originalImageSize.x);
                        customHeight = static_cast<float>(originalImageSize.y);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Fit to Window")) {
                        float scaleX = static_cast<float>(window.getSize().x) / originalImageSize.x;
                        float scaleY = static_cast<float>(window.getSize().y) / originalImageSize.y;
                        float scale = std::min(scaleX, scaleY);
                        customWidth = originalImageSize.x * scale;
                        customHeight = originalImageSize.y * scale;
                    }
                    ImGui::Unindent();
                }
                
                ImGui::Separator();
                
                if (ImGui::Button("Import", ImVec2(120, 30))) {
                    if (backgroundSprite) {
                        delete backgroundSprite;
                    }
                    backgroundSprite = new sf::Sprite(backgroundTexture);
                    hasBackgroundImage = loadBackgroundImage(backgroundTexture, *backgroundSprite, pendingImagePath, window.getSize(), resizeMode, customWidth, customHeight);
                    if (hasBackgroundImage) {
                        // Set the initial color/opacity
                        sf::Color imageColor = sf::Color::White;
                        imageColor.a = static_cast<std::uint8_t>(imageOpacity * 255);
                        backgroundSprite->setColor(imageColor);
                        
                        // Update global scale to match the loaded image's scale
                        imageScale = backgroundSprite->getScale().x;
                        
                        imageEnabled = true;
                        currentImagePath = pendingImagePath;
                    }
                    showImageResizeDialog = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 30))) {
                    showImageResizeDialog = false;
                }
            }
            ImGui::End();
        }

        window.clear(sf::Color::White);
        
        // Draw background image
        if (hasBackgroundImage && imageEnabled) {
            // Update sprite properties based on settings
            backgroundSprite->setScale(sf::Vector2f(imageScale, imageScale));
            sf::Color imageColor = backgroundSprite->getColor();
            imageColor.a = static_cast<std::uint8_t>(imageOpacity * 255);
            backgroundSprite->setColor(imageColor);
            window.draw(*backgroundSprite);
        }
        
        // Draw grid background
        if (gridEnabled) {
            drawGrid(window, gridSize);
        }

        // Draw everything here

        for (Polygon polygon : polygons) {
            window.draw(polygon.render);
        }
        
        // Draw resize handles if in resize mode
        if (imageResizeMode && hasBackgroundImage && imageEnabled && backgroundSprite) {
            sf::FloatRect imageBounds = getImageBounds(backgroundSprite);
            drawResizeHandles(window, imageBounds, handleSize);
        }

        if (status.createPolygon && !firstVertex) {
            // Draw boundary of supposed polygon
            ImVec2 mousepos = sf::Mouse::getPosition(window);
            newPolygonOutline.back().position = mousepos;
            window.draw(newPolygonOutline.data(), newPolygonOutline.size(), sf::PrimitiveType::LineStrip);
        }

        if (tutorial.isActive())
            tutorial.render();


        ImGui::SFML::Render(window);
        window.display();

    }

    // Cleanup
    if (backgroundSprite) {
        delete backgroundSprite;
    }

    ImGui::SFML::Shutdown();

    // Save logs to file
    if (logSavingEnabled) {
        saveLogToFile("log");
    }
}

int main()
{
    try {
        runProgram();
    }
    catch (const std::exception& ex) {
        logger << "Unhandled exception: " << ex.what() << std::endl;
        saveLogToFile("crash");
        return 1;
    }

    return 0;
}
