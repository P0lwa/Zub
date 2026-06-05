#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>

class Gear
{
private:
    int teeth;
    float module;
    float thickness;
    float outerRadius;
    float innerRadius;
    float holeRadius;
    bool isInitialized;

    void calculateGeometry();
    void drawTooth(int index, float angleStep, float toothHeight);

public:
    Gear(int numTeeth, float mod, float thick);
    ~Gear();

    void draw();
    void drawWithMaterial(const float* color);

    int getTeeth() const { return teeth; }
    float getOuterRadius() const { return outerRadius; }
    float getInnerRadius() const { return innerRadius; }
    float getModule() const { return module; }
};