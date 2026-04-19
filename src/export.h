#ifndef EXPORT_H
#define EXPORT_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstdint>
#include <algorithm>

struct PrintSettings {
    std::string frontImagePath;
    std::string backImagePath;
    int copies;
    float paperWidthMm;
    float paperHeightMm;
    float cardWidthMm;
    float cardHeightMm;
    bool duplex;
    int flipEdge; // 0 = Long Edge, 1 = Short Edge
    bool isLandscape;
    
    // Margins
    float marginTop;
    float marginBottom;
    float marginLeft;
    float marginRight;
};

struct CardPosition {
    float x_mm;
    float y_mm;
    bool isFront;
    int page; // 0 for front page, 1 for back page
};

inline std::vector<CardPosition> CalculateLayout(const PrintSettings& settings) {
    std::vector<CardPosition> positions;
    
    float pW = settings.isLandscape ? settings.paperHeightMm : settings.paperWidthMm;
    float pH = settings.isLandscape ? settings.paperWidthMm : settings.paperHeightMm;
    
    float spacing = 5.0f;
    
    float availW = pW - settings.marginLeft - settings.marginRight;
    float availH = pH - settings.marginTop - settings.marginBottom;
    
    int cols = (availW + spacing) / (settings.cardWidthMm + spacing);
    int rows = (availH + spacing) / (settings.cardHeightMm + spacing);
    
    if (cols <= 0) cols = 1;
    if (rows <= 0) rows = 1;
    
    int maxPerPage = cols * rows;
    
    // How many cards to actually draw on page 1
    int cardsOnPage = std::min(settings.copies, maxPerPage);
    
    // Center the grid within the available area
    float gridW = cardsOnPage > cols ? (cols * settings.cardWidthMm + (cols - 1) * spacing) : (cardsOnPage * settings.cardWidthMm + (cardsOnPage - 1) * spacing);
    float startX = settings.marginLeft + (availW - gridW) / 2.0f;
    float startY = settings.marginTop;
    
    for (int i = 0; i < cardsOnPage; ++i) {
        int r = i / cols;
        int c = i % cols;
        
        float x = startX + c * (settings.cardWidthMm + spacing);
        float y = startY + r * (settings.cardHeightMm + spacing);
        
        // Front page (Page 0)
        positions.push_back({x, y, true, 0});
        
        // Back page (Page 1)
        if (settings.duplex) {
            float backX = x;
            float backY = y;
            
            if (settings.flipEdge == 0) { // Long Edge (Left-Right flip)
                backX = pW - x - settings.cardWidthMm;
            } else { // Short Edge (Top-Bottom flip)
                backY = pH - y - settings.cardHeightMm;
            }
            
            positions.push_back({backX, backY, false, 1});
        }
    }
    
    return positions;
}

#endif // EXPORT_H
