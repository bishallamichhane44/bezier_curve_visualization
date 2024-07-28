#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

// Color conversion structures
struct ColorRGB {
    double red;
    double green;
    double blue;
};

struct ColorHSV {
    double hue;
    double saturation;
    double value;
};

// Function declarations
sf::Color convertHexToRGB(const std::string& hexCode);
void renderCurveSegment(sf::VertexArray& controlPoints, float progress, sf::Color curveColor, 
                        std::vector<sf::CircleShape>& trailMarkers, sf::RenderWindow& display);
ColorHSV convertRGBtoHSV(ColorRGB input);
ColorRGB convertHSVtoRGB(ColorHSV input);