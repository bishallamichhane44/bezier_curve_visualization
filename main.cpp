#include "curveutils.hpp"
#include <SFML/Graphics.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <limits>

sf::Color convertHexToRGB(const std::string& hexCode) {
    unsigned int hexValue;
    std::stringstream converter;
    converter << std::hex << hexCode;
    converter >> hexValue;

    sf::Uint8 red = (hexValue >> 16) & 0xFF;
    sf::Uint8 green = (hexValue >> 8) & 0xFF;
    sf::Uint8 blue = hexValue & 0xFF;

    return sf::Color(red, green, blue);
}

void renderCurveSegment(sf::VertexArray& controlPoints, float progress, sf::Color curveColor, 
                        std::vector<sf::CircleShape>& trailMarkers, sf::RenderWindow& display)
{
    if (progress > 1.0f) progress = 1.0f;

    for (std::size_t i = 0; i < controlPoints.getVertexCount(); i++)
    {
        controlPoints[i].color = curveColor;
    }
    controlPoints.setPrimitiveType(sf::LineStrip);
    display.draw(controlPoints);

    if (controlPoints.getVertexCount() < 2)
        return;

    sf::VertexArray intermediatePoints;
    std::vector<sf::CircleShape> intermediateMarkers;
    for (std::size_t i = 0; i < controlPoints.getVertexCount() - 1; i++)
    {
        sf::Vector2f start = controlPoints[i].position, end = controlPoints[i + 1].position;

        sf::Vector2f interpolatedPos = { (1 - progress) * start.x + progress * end.x, 
                                         (1 - progress) * start.y + progress * end.y };
        sf::CircleShape marker;
        marker.setRadius(5.0f);
        marker.setPosition({ interpolatedPos.x - 5.0f, interpolatedPos.y - 5.0f });
        marker.setFillColor(curveColor);

        if (controlPoints.getVertexCount() == 2)
        {
            sf::Color trailColor = convertHexToRGB("FF6B6B");
            sf::CircleShape trailDot;
            trailDot.setRadius(2.5f);
            trailDot.setPosition({ interpolatedPos.x - 2.5f, interpolatedPos.y - 2.5f });
            trailDot.setFillColor(trailColor);

            trailMarkers.push_back(trailDot);
        }

        intermediateMarkers.push_back(marker);
        intermediatePoints.append(sf::Vertex(interpolatedPos));
    }

    for (auto& marker : intermediateMarkers)
    {
        display.draw(marker);
    }

    if (intermediateMarkers.size() != 1)
    {
        ColorRGB rgbValue;
        rgbValue.red = curveColor.r / 255.0;
        rgbValue.green = curveColor.g / 255.0;
        rgbValue.blue = curveColor.b / 255.0;

        ColorHSV hsvValue = convertRGBtoHSV(rgbValue);
        hsvValue.saturation = 1.0;
        hsvValue.hue += 10;
        if (hsvValue.hue >= 360.0) hsvValue.hue -= 360.0;

        rgbValue = convertHSVtoRGB(hsvValue);
        renderCurveSegment(intermediatePoints, progress, 
                           sf::Color(static_cast<sf::Uint8>(rgbValue.red * 255), 
                                     static_cast<sf::Uint8>(rgbValue.green * 255), 
                                     static_cast<sf::Uint8>(rgbValue.blue * 255)), 
                           trailMarkers, display);
    }
}

ColorHSV convertRGBtoHSV(ColorRGB input)
{
    ColorHSV output;
    double minVal, maxVal, delta;

    minVal = std::min({input.red, input.green, input.blue});
    maxVal = std::max({input.red, input.green, input.blue});

    output.value = maxVal;
    delta = maxVal - minVal;
    if (delta < 0.00001)
    {
        output.saturation = 0;
        output.hue = 0;
        return output;
    }
    if (maxVal > 0.0)
    {
        output.saturation = (delta / maxVal);
    }
    else
    {
        output.saturation = 0.0;
        output.hue = NAN;
        return output;
    }
    if (input.red >= maxVal)
        output.hue = (input.green - input.blue) / delta;
    else if (input.green >= maxVal)
        output.hue = 2.0 + (input.blue - input.red) / delta;
    else
        output.hue = 4.0 + (input.red - input.green) / delta;

    output.hue *= 60.0;

    if (output.hue < 0.0)
        output.hue += 360.0;

    return output;
}

ColorRGB convertHSVtoRGB(ColorHSV input)
{
    double tempH, p, q, t, ff;
    long i;
    ColorRGB output;

    if (input.saturation <= 0.0)
    {
        output.red = input.value;
        output.green = input.value;
        output.blue = input.value;
        return output;
    }
    tempH = input.hue;
    if (tempH >= 360.0) tempH = 0.0;
    tempH /= 60.0;
    i = static_cast<long>(tempH);
    ff = tempH - i;
    p = input.value * (1.0 - input.saturation);
    q = input.value * (1.0 - (input.saturation * ff));
    t = input.value * (1.0 - (input.saturation * (1.0 - ff)));

    switch (i)
    {
    case 0:
        output.red = input.value;
        output.green = t;
        output.blue = p;
        break;
    case 1:
        output.red = q;
        output.green = input.value;
        output.blue = p;
        break;
    case 2:
        output.red = p;
        output.green = input.value;
        output.blue = t;
        break;
    case 3:
        output.red = p;
        output.green = q;
        output.blue = input.value;
        break;
    case 4:
        output.red = t;
        output.green = p;
        output.blue = input.value;
        break;
    case 5:
    default:
        output.red = input.value;
        output.green = p;
        output.blue = q;
        break;
    }
    return output;
}

int main()
{
    sf::RenderWindow canvas(sf::VideoMode(1080, 720), "Curve Generator");

    sf::VertexArray controlPoints;
    controlPoints.setPrimitiveType(sf::LineStrip);
    std::vector<sf::CircleShape> trailMarkers;
    std::vector<sf::VertexArray> generatedCurves;

    float animationProgress = 0.0f;
    sf::Clock animationTimer;
    bool isDrawingCurve = false;
    bool isPaused = false;
    sf::Time pausedTime = sf::Time::Zero;

    const float DEFAULT_SPEED = 0.01f;
    float speedMultiplier = 1.0f;
    float currentSpeed = DEFAULT_SPEED;

    sf::Color canvasBackground = convertHexToRGB("1A1A2E");
    sf::Color controlPointColor = convertHexToRGB("FF6B6B");
    sf::Color curveColor = convertHexToRGB("4ECCA3");
    sf::Color pointerColor = convertHexToRGB("FFFFFF");

    int activeControlPoint = -1;

    sf::CircleShape pointerIndicator(5.0f);
    pointerIndicator.setFillColor(pointerColor);
    pointerIndicator.setOrigin(5.0f, 5.0f);

    // Font for displaying speed
    sf::Font font;
    if (!font.loadFromFile("arial-font/arial.ttf")) // Make sure you have this font file or change to an available font
    {
        std::cout << "Failed to load font" << std::endl;
        return -1;
    }

    sf::Text speedText;
    speedText.setFont(font);
    speedText.setCharacterSize(24);
    speedText.setFillColor(convertHexToRGB("4ECCA3")); // Using the curveColor
    speedText.setStyle(sf::Text::Bold);
    speedText.setOutlineColor(convertHexToRGB("1A1A2E")); // Using the canvasBackground color
    speedText.setOutlineThickness(2.0f);
    speedText.setPosition(20, 20);

    while (canvas.isOpen())
    {
        sf::Event canvasEvent;
        while (canvas.pollEvent(canvasEvent))
        {
            if (canvasEvent.type == sf::Event::Closed)
                canvas.close();
            if (canvasEvent.type == sf::Event::MouseButtonPressed)
            {
                if (canvasEvent.mouseButton.button == sf::Mouse::Left)
                {
                    if (!isDrawingCurve)
                    {
                        sf::Vector2f clickPosition = { static_cast<float>(canvasEvent.mouseButton.x), 
                                                       static_cast<float>(canvasEvent.mouseButton.y) };
                        controlPoints.append(sf::Vertex(clickPosition));

                        sf::CircleShape controlPointMarker;
                        controlPointMarker.setRadius(3.0f);
                        controlPointMarker.setPosition({ clickPosition.x - 3.0f, clickPosition.y - 3.0f });
                        controlPointMarker.setFillColor(controlPointColor);
                        trailMarkers.push_back(controlPointMarker);
                    }
                    else if (activeControlPoint != -1)
                    {
                        sf::Vector2f newPosition = { static_cast<float>(canvasEvent.mouseButton.x), 
                                                     static_cast<float>(canvasEvent.mouseButton.y) };
                        controlPoints[activeControlPoint].position = newPosition;
                        trailMarkers[activeControlPoint].setPosition({ newPosition.x - 3.0f, newPosition.y - 3.0f });
                    }
                }
            }
            if (canvasEvent.type == sf::Event::KeyPressed)
            {
                 if (canvasEvent.key.code == sf::Keyboard::Enter)
                {
                    isDrawingCurve = true;
                    animationTimer.restart();
                    animationProgress = 0.0f;
                    generatedCurves.push_back(controlPoints);
                    isPaused = false;
                    pausedTime = sf::Time::Zero;
                }
                if (canvasEvent.key.code == sf::Keyboard::R)
                {
                    controlPoints.clear();
                    trailMarkers.clear();
                    generatedCurves.clear();
                    isDrawingCurve = false;
                    animationProgress = 0.0f;
                    activeControlPoint = -1;
                    speedMultiplier = 1.0f;
                    currentSpeed = DEFAULT_SPEED;
                }
                if (canvasEvent.key.code == sf::Keyboard::D)
                {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(canvas);
                    float minDist = std::numeric_limits<float>::max();
                    for (std::size_t i = 0; i < controlPoints.getVertexCount(); ++i)
                    {
                        float distance = std::hypot(controlPoints[i].position.x - mousePos.x, 
                                                    controlPoints[i].position.y - mousePos.y);
                        if (distance < minDist)
                        {
                            minDist = distance;
                            activeControlPoint = i;
                        }
                    }
                }
                if (canvasEvent.key.code == sf::Keyboard::U)
                {
                    if (!generatedCurves.empty())
                    {
                        generatedCurves.pop_back();
                    }
                }
                if (canvasEvent.key.code == sf::Keyboard::P)
                {
                    isPaused = !isPaused;
                    if (isPaused)
                    {
                        pausedTime = animationTimer.getElapsedTime();
                    }
                    else
                    {
                        animationTimer.restart();
                    }
                }
                if (canvasEvent.key.code == sf::Keyboard::RBracket)
                {
                    speedMultiplier *= 1.1f; // Increase speed by 10%
                    currentSpeed = DEFAULT_SPEED * speedMultiplier;
                }
                if (canvasEvent.key.code == sf::Keyboard::LBracket)
                {
                    speedMultiplier *= 0.9f; // Decrease speed by 10%
                    currentSpeed = DEFAULT_SPEED * speedMultiplier;
                }
            }
        }

        sf::Vector2i cursorPosition = sf::Mouse::getPosition(canvas);
        pointerIndicator.setPosition(static_cast<float>(cursorPosition.x), static_cast<float>(cursorPosition.y));

        canvas.clear(canvasBackground);

        for (const auto& curve : generatedCurves)
        {
            canvas.draw(curve);
        }

        if (isDrawingCurve && controlPoints.getVertexCount() > 1)
        {
            renderCurveSegment(controlPoints, animationProgress, curveColor, trailMarkers, canvas);
            if (!isPaused)
            {
                sf::Time elapsedTime = animationTimer.getElapsedTime() + pausedTime;
                if (elapsedTime.asMilliseconds() >= 10 && animationProgress < 1.0f)
                {
                    animationProgress += currentSpeed;
                    pausedTime = sf::Time::Zero;
                    animationTimer.restart();
                }
            }
        }

        for (auto& marker : trailMarkers)
        {
            canvas.draw(marker);
        }

        canvas.draw(pointerIndicator);

        // Update and draw speed text
        std::ostringstream speedStream;
        speedStream << "Speed: " << std::fixed << std::setprecision(2) << speedMultiplier << "x";
        speedText.setString(speedStream.str());
        canvas.draw(speedText);

        canvas.display();
    }

    return 0;
}