#include "Core/NUITypes.h"

// Test the drawLine function signature
class TestRenderer {
public:
    virtual void drawLine(const NUIPoint& start, const NUIPoint& end, float thickness, const NUIColor& color) = 0;
};

// Test the corrected drawLine calls
void testDrawLine() {
    TestRenderer* renderer = nullptr;
    float x = 10.0f, y = 20.0f, charWidth = 30.0f, charHeight = 40.0f, lineWidth = 2.0f;
    NUIColor color = {1.0f, 0.0f, 0.0f, 1.0f};
    
    // These should compile correctly now
    renderer->drawLine(NUIPoint(x + charWidth*0.1f, y), NUIPoint(x + charWidth*0.1f, y + charHeight), lineWidth, color);
    renderer->drawLine(NUIPoint(x + charWidth*0.1f, y), NUIPoint(x + charWidth*0.7f, y), lineWidth, color);
    renderer->drawLine(NUIPoint(x + charWidth*0.1f, y + charHeight*0.5f), NUIPoint(x + charWidth*0.7f, y + charHeight*0.5f), lineWidth, color);
}

int main() {
    return 0;
}