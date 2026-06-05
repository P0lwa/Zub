#include "Gear.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Gear::Gear(int numTeeth, float mod, float thick)
    : teeth(numTeeth), module(mod), thickness(thick), isInitialized(false)
{
    calculateGeometry();
}

Gear::~Gear()
{
}

void Gear::calculateGeometry()
{
    outerRadius = (teeth + 2.0f) * module / 2.0f;
    innerRadius = (teeth - 2.5f) * module / 2.0f;
    holeRadius = innerRadius * 0.4f;
    isInitialized = true;
}

void Gear::drawTooth(int index, float angleStep, float toothHeight)
{
    float toothWidthAngle = angleStep * 0.4f;
    float angle = index * angleStep;

    float rad = angle * M_PI / 180.0f;
    float toothWidthRad = toothWidthAngle * M_PI / 180.0f;

    float baseRadius = innerRadius;

    glBegin(GL_QUADS);

    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(baseRadius * cosf(rad - toothWidthRad / 2.0f),
        baseRadius * sinf(rad - toothWidthRad / 2.0f),
        thickness / 2.0f);
    glVertex3f((baseRadius + toothHeight) * cosf(rad - toothWidthRad / 2.0f),
        (baseRadius + toothHeight) * sinf(rad - toothWidthRad / 2.0f),
        thickness / 2.0f);
    glVertex3f((baseRadius + toothHeight) * cosf(rad + toothWidthRad / 2.0f),
        (baseRadius + toothHeight) * sinf(rad + toothWidthRad / 2.0f),
        thickness / 2.0f);
    glVertex3f(baseRadius * cosf(rad + toothWidthRad / 2.0f),
        baseRadius * sinf(rad + toothWidthRad / 2.0f),
        thickness / 2.0f);

    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(baseRadius * cosf(rad + toothWidthRad / 2.0f),
        baseRadius * sinf(rad + toothWidthRad / 2.0f),
        -thickness / 2.0f);
    glVertex3f((baseRadius + toothHeight) * cosf(rad + toothWidthRad / 2.0f),
        (baseRadius + toothHeight) * sinf(rad + toothWidthRad / 2.0f),
        -thickness / 2.0f);
    glVertex3f((baseRadius + toothHeight) * cosf(rad - toothWidthRad / 2.0f),
        (baseRadius + toothHeight) * sinf(rad - toothWidthRad / 2.0f),
        -thickness / 2.0f);
    glVertex3f(baseRadius * cosf(rad - toothWidthRad / 2.0f),
        baseRadius * sinf(rad - toothWidthRad / 2.0f),
        -thickness / 2.0f);

    glNormal3f(cosf(rad), sinf(rad), 0.0f);
    glVertex3f((baseRadius + toothHeight) * cosf(rad - toothWidthRad / 2.0f),
        (baseRadius + toothHeight) * sinf(rad - toothWidthRad / 2.0f),
        thickness / 2.0f);
    glVertex3f((baseRadius + toothHeight) * cosf(rad - toothWidthRad / 2.0f),
        (baseRadius + toothHeight) * sinf(rad - toothWidthRad / 2.0f),
        -thickness / 2.0f);
    glVertex3f((baseRadius + toothHeight) * cosf(rad + toothWidthRad / 2.0f),
        (baseRadius + toothHeight) * sinf(rad + toothWidthRad / 2.0f),
        -thickness / 2.0f);
    glVertex3f((baseRadius + toothHeight) * cosf(rad + toothWidthRad / 2.0f),
        (baseRadius + toothHeight) * sinf(rad + toothWidthRad / 2.0f),
        thickness / 2.0f);

    glEnd();
}

void Gear::draw()
{
    if (!isInitialized) return;

    float angleStep = 360.0f / teeth;
    float toothHeight = outerRadius - innerRadius;

    GLUquadric* quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);
    gluQuadricTexture(quadric, GL_TRUE);

    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quadric, innerRadius, innerRadius, thickness, 32, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, thickness / 2.0f);
    gluDisk(quadric, 0.0f, innerRadius, 32, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -thickness / 2.0f);
    glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    gluDisk(quadric, 0.0f, innerRadius, 32, 1);
    glPopMatrix();

    gluDeleteQuadric(quadric);

    for (int i = 0; i < teeth; ++i)
    {
        drawTooth(i, angleStep, toothHeight);
    }

    float black[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, black);

    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    GLUquadric* holeQuadric = gluNewQuadric();
    gluQuadricNormals(holeQuadric, GLU_SMOOTH);
    gluCylinder(holeQuadric, holeRadius, holeRadius, thickness + 0.02f, 16, 1);
    gluDeleteQuadric(holeQuadric);
    glPopMatrix();
}

void Gear::drawWithMaterial(const float* color)
{
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
    float specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 60.0f);

    draw();
}