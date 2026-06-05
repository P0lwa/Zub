#include <windows.h>
#include "Render.h"
#include "GUItextRectangle.h"
#include "MyShaders.h"
#include "ObjLoader.h"
#include "Texture.h"
#include "Gear.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "debout.h"
#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;

bool texturing = true;      // Включение/выключение текстур
bool lightning = true;      // Включение/выключение освещения
bool alpha = false;         // Включение/выключение альфа-наложения (прозрачности)

Gear* gear1 = nullptr;      // Ведущая шестерня (12 зубьев)
Gear* gear2 = nullptr;      // Ведомое колесо (24 зуба)
float rotation_speed = 1.0f; // Скорость вращения
float angle1 = 0.0f;        // Угол поворота первой шестерни
float angle2 = 0.0f;        // Угол поворота второй шестерни
float center_distance = 0.0f; // Межосевое расстояние

// Обработка нажатий клавиш (L, T, A, F, W, S, R)
void switchModes(OpenGL* sender, KeyEventArg arg)
{
    switch (arg.key)
    {
    case 'L':
    case 'l':
        lightning = !lightning;
        break;
    case 'T':
    case 't':
        texturing = !texturing;
        break;
    case 'A':
    case 'a':
        alpha = !alpha;
        break;
    case 'F':
    case 'f':
        light.SetPosition(camera.x(), camera.y(), camera.z());
        break;
    case 'G':
    case 'g':
        break;
    case 'W':
    case 'w':
        rotation_speed = (rotation_speed < 5.0f) ? rotation_speed + 0.5f : 5.0f;
        break;
    case 'S':
    case 's':
        rotation_speed = (rotation_speed > -5.0f) ? rotation_speed - 0.5f : -5.0f;
        break;
    case 'R':
    case 'r':
        rotation_speed = 1.0f;
        angle1 = 0.0f;
        angle2 = 0.0f;
        break;
    }
}

// Функция умножения матриц
template <typename T, int M1, int N1, int M2, int N2> void MatrixMultiply(const T* a, const T* b, T* c)
{
    for (int i = 0; i < M1; ++i)
    {
        for (int j = 0; j < N2; ++j)
        {
            c[i * N2 + j] = T(0);
            for (int k = 0; k < N1; ++k)
            {
                c[i * N2 + j] += a[i * N1 + k] * b[k * N2 + j];
            }
        }
    }
}

GuiTextRectangle text;      // Текстовый прямоугольник для вывода информации
GLuint texId;
ObjModel f;
Shader cassini_sh;
Shader phong_sh;
Shader vb_sh;
Shader simple_texture_sh;
Texture stankin_tex, vb_tex, monkey_tex, gear_texture;

// Инициализация сцены (выполняется один раз перед первым рендером)
void initRender()
{
    // Настройка шейдеров
    cassini_sh.VshaderFileName = "shaders/v.vert";
    cassini_sh.FshaderFileName = "shaders/cassini.frag";
    cassini_sh.LoadShaderFromFile();
    cassini_sh.Compile();

    phong_sh.VshaderFileName = "shaders/v.vert";
    phong_sh.FshaderFileName = "shaders/light.frag";
    phong_sh.LoadShaderFromFile();
    phong_sh.Compile();

    vb_sh.VshaderFileName = "shaders/v.vert";
    vb_sh.FshaderFileName = "shaders/vb.frag";
    vb_sh.LoadShaderFromFile();
    vb_sh.Compile();

    simple_texture_sh.VshaderFileName = "shaders/v.vert";
    simple_texture_sh.FshaderFileName = "shaders/textureShader.frag";
    simple_texture_sh.LoadShaderFromFile();
    simple_texture_sh.Compile();

    // Загрузка текстур
    stankin_tex.LoadTexture("textures/stankin.png");
    vb_tex.LoadTexture("textures/vb.png");
    monkey_tex.LoadTexture("textures/monkey.png");
    gear_texture.LoadTexture("textures/texture.png");  // Текстура для шестерен

    f.LoadModel("models//monkey.obj_m");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    camera.caclulateCameraPos();

    // Привязка камеры к событиям мыши
    gl.WheelEvent.reaction(&camera, &Camera::Zoom);
    gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
    gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
    gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
    gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);

    // Привязка источника света к событиям
    gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
    gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
    gl.KeyUpEvent.reaction(&light, &Light::StopDrug);

    // Привязка обработки клавиш
    gl.KeyDownEvent.reaction(switchModes);
    text.setSize(512, 180);

    // Установка позиции камеры
    camera.setPosition(8.0f, 4.0f, 8.0f);
    light.SetPosition(5.0f, 5.0f, 10.0f);

    // Создание шестерен (расчет параметров)
    // Модуль m=0.6, число зубьев Z1=12, Z2=24
    gear1 = new Gear(12, 0.6f, 1.2f);
    gear2 = new Gear(24, 0.6f, 1.2f);

    // Расчет межосевого расстояния: a = m * (Z1 + Z2) / 2
    center_distance = 0.6f * (12 + 24) / 2.0f;
}

float view_matrix[16];
double full_time = 0;
int location = 0;

// Функция отрисовки сцены (вызывается каждый кадр)
void Render(double delta_time)
{
    full_time += delta_time;

    // Перемещение света в камеру при нажатии F
    if (gl.isKeyPressed('F') || gl.isKeyPressed('f'))
    {
        light.SetPosition(camera.x(), camera.y(), camera.z());
    }
    camera.SetUpCamera();
    glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);

    // Отрисовка осей координат
    gl.DrawAxes();
    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    // Настройка освещения
    if (lightning)
    {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

        // Основной источник света
        float light0_pos[] = { light.x(), light.y(), light.z(), 1.0f };
        float light0_ambient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
        float light0_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float light0_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
        glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

        // Дополнительный источник света сверху
        float light1_pos[] = { 0.0f, 10.0f, 0.0f, 1.0f };
        float light1_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        float light1_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
        float light1_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
        glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
        glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
        glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
    }
    if (texturing)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (alpha)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glShadeModel(GL_SMOOTH);
    light.SetUpLight();

    // Отрисовка квадратика с овалом Кассини (шейдер)
    cassini_sh.UseShader();
    location = glGetUniformLocationARB(cassini_sh.program, "Time");
    glUniform1fARB(location, (float)full_time);
    location = glGetUniformLocationARB(cassini_sh.program, "size");
    glUniform2fARB(location, 100.0f, 100.0f);
    glPushMatrix();
    glTranslated(0, -1.2, 0);
    glBegin(GL_QUADS);
    glNormal3d(0, 0, 1); glTexCoord2d(1, 1); glVertex3d(0.5, 0.5, 0);
    glTexCoord2d(1, 0); glVertex3d(0.5, -0.5, 0);
    glTexCoord2d(0, 0); glVertex3d(-0.5, -0.5, 0);
    glTexCoord2d(0, 1); glVertex3d(-0.5, 0.5, 0);
    glEnd();
    glPopMatrix();

    // Отрисовка квадратика с освещением (шейдер Фонга)
    phong_sh.UseShader();
    float light_pos[4] = { light.x(), light.y(), light.z(), 1.0f };
    float light_pos_v[4];
    MatrixMultiply<float, 1, 4, 4, 4>(light_pos, view_matrix, light_pos_v);
    location = glGetUniformLocationARB(phong_sh.program, "Ia"); glUniform3fARB(location, 1.0f, 1.0f, 1.0f);
    location = glGetUniformLocationARB(phong_sh.program, "Id"); glUniform3fARB(location, 1.0f, 1.0f, 1.0f);
    location = glGetUniformLocationARB(phong_sh.program, "Is"); glUniform3fARB(location, 1.0f, 1.0f, 1.0f);
    location = glGetUniformLocationARB(phong_sh.program, "ma"); glUniform3fARB(location, 0.1f, 0.1f, 0.1f);
    location = glGetUniformLocationARB(phong_sh.program, "md"); glUniform3fARB(location, 0.6f, 0.6f, 0.6f);
    location = glGetUniformLocationARB(phong_sh.program, "ms"); glUniform4fARB(location, 0.0f, 1.0f, 0.0f, 300.0f);
    location = glGetUniformLocationARB(phong_sh.program, "light_pos_v"); glUniform3fvARB(location, 1, light_pos_v);
    glPushMatrix();
    glTranslated(0, 0, 0);
    glBegin(GL_QUADS);
    glNormal3d(0, 0, 1); glTexCoord2d(1, 1); glVertex3d(0.5, 0.5, 0);
    glTexCoord2d(1, 0); glVertex3d(0.5, -0.5, 0);
    glTexCoord2d(0, 0); glVertex3d(-0.5, -0.5, 0);
    glTexCoord2d(0, 1); glVertex3d(-0.5, 0.5, 0);
    glEnd();
    glPopMatrix();

    // Отрисовка простого квадратика (без шейдеров)
    Shader::DontUseShaders();
    glBindTexture(GL_TEXTURE_2D, 0);
    glPushMatrix();
    glTranslated(1.2, 0, 0);
    glBegin(GL_QUADS);
    glNormal3d(0, 0, 1); glTexCoord2d(1, 1); glVertex3d(0.5, 0.5, 0);
    glTexCoord2d(1, 0); glVertex3d(0.5, -0.5, 0);
    glTexCoord2d(0, 0); glVertex3d(-0.5, -0.5, 0);
    glTexCoord2d(0, 1); glVertex3d(-0.5, 0.5, 0);
    glEnd();
    glPopMatrix();

    // Отрисовка квадратика с ВБ (шейдер с двумя текстурами)
    vb_sh.UseShader();
    glActiveTexture(GL_TEXTURE0); stankin_tex.Bind();
    glActiveTexture(GL_TEXTURE1); vb_tex.Bind();
    location = glGetUniformLocationARB(vb_sh.program, "time"); glUniform1fARB(location, (float)full_time);
    location = glGetUniformLocationARB(vb_sh.program, "tex_stankin"); glUniform1iARB(location, 0);
    location = glGetUniformLocationARB(vb_sh.program, "tex_vb"); glUniform1iARB(location, 1);
    glPushMatrix();
    glTranslated(0, 1.2, 0);
    glBegin(GL_QUADS);
    glNormal3d(0, 0, 1); glTexCoord2d(1, 1); glVertex3d(0.5, 0.5, 0);
    glTexCoord2d(1, 0); glVertex3d(0.5, -0.5, 0);
    glTexCoord2d(0, 0); glVertex3d(-0.5, -0.5, 0);
    glTexCoord2d(0, 1); glVertex3d(-0.5, 0.5, 0);
    glEnd();
    glPopMatrix();

    // Отрисовка обезьянки без шейдеров
    glPushMatrix();
    Shader::DontUseShaders();
    glActiveTexture(GL_TEXTURE0); monkey_tex.Bind();
    glShadeModel(GL_SMOOTH);
    glTranslated(-1, 0, 0.5);
    glScaled(0.1, 0.1, 0.1);
    glRotated(180, 0, 0, 1);
    f.Draw();
    glPopMatrix();

    // Отрисовка обезьянки с шейдером текстур
    simple_texture_sh.UseShader();
    location = glGetUniformLocationARB(simple_texture_sh.program, "tex"); glUniform1iARB(location, 0);
    glActiveTexture(GL_TEXTURE0); monkey_tex.Bind();
    glPushMatrix();
    glTranslated(-1, 1, 0.5);
    glScaled(0.1, 0.1, 0.1);
    glRotated(180, 0, 0, 1);
    f.Draw();
    glPopMatrix();

    // === ОТРИСОВКА ШЕСТЕРЕН С ТЕКСТУРОЙ, ОСВЕЩЕНИЕМ И АЛЬФА-НАЛОЖЕНИЕМ ===
    // Анимация вращения с кинематическим соотношением
    angle1 += rotation_speed * 0.5f;
    angle2 = -(angle1 * 12.0f / 24.0f);  // Передаточное отношение u = Z2/Z1 = 2

    // Отключаем шейдеры для шестерен
    Shader::DontUseShaders();

    // Включаем освещение
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    // Альфа-канал: если alpha включена - 0.7 (прозрачность), иначе 1.0 (непрозрачные)
    float gear_alpha = alpha ? 0.7f : 1.0f;

    // Включаем текстуру ТОЛЬКО если texturing = true
    if (texturing)
    {
        glEnable(GL_TEXTURE_2D);
        gear_texture.Bind();  // Применяем текстуру texture.png
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
    }

    // Первая шестерня - СЕРЕБРИСТАЯ СТАЛЬ
    glPushMatrix();
    glRotatef(angle1, 0.0f, 0.0f, 1.0f);

    float silver_ambient[] = { 0.3f, 0.3f, 0.35f, gear_alpha };
    float silver_diffuse[] = { 0.6f, 0.6f, 0.65f, gear_alpha };
    float silver_specular[] = { 1.0f, 1.0f, 1.0f, gear_alpha };
    glMaterialfv(GL_FRONT, GL_AMBIENT, silver_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, silver_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, silver_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);

    gear1->draw();
    glPopMatrix();

    // Вторая шестерня - БРОНЗА/ЗОЛОТО
    glPushMatrix();
    glTranslatef(center_distance, 0.0f, 0.0f);  // Смещение на межосевое расстояние
    glRotatef(angle2, 0.0f, 0.0f, 1.0f);

    float bronze_ambient[] = { 0.3f, 0.2f, 0.05f, gear_alpha };
    float bronze_diffuse[] = { 0.8f, 0.6f, 0.15f, gear_alpha };
    float bronze_specular[] = { 1.0f, 1.0f, 0.7f, gear_alpha };
    glMaterialfv(GL_FRONT, GL_AMBIENT, bronze_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, bronze_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, bronze_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);

    gear2->draw();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);

    // Сброс трансформаций
    glLoadIdentity();
    camera.SetUpCamera();
    Shader::DontUseShaders();

    // Отрисовка индикатора источника света
    if (lightning)
    {
        light.DrawLightGizmo();
    }

    // === ОТРИСОВКА ИНФОРМАЦИОННОГО ТЕКСТА ===
    glActiveTexture(GL_TEXTURE0);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(3)
        << L"=== УПРАВЛЕНИЕ ШЕСТЕРНЯМИ ===\n"
        << L"W - увеличить скорость\n"
        << L"S - уменьшить скорость\n"
        << L"R - сбросить положение\n"
        << L"\n"
        << L"=== ОБЩИЕ НАСТРОЙКИ ===\n"
        << "T - " << (texturing ? L"[вкл]выкл" : L"вкл[выкл]") << L" текстур\n"
        << "L - " << (lightning ? L"[вкл]выкл" : L"вкл[выкл]") << L" освещение\n"
        << "A - " << (alpha ? L"[вкл]выкл" : L"вкл[выкл]") << L" альфа-наложение\n"
        << L"F - свет в камеру\n"
        << L"G - двигать свет (мышью)\n"
        << L"\n"
        << L"Скорость: " << std::setprecision(2) << rotation_speed << '\n'
        << L"Свет: (" << std::setw(6) << light.x() << "," << std::setw(6) << light.y() << "," << std::setw(6) << light.z() << ")\n"
        << L"Камера: (" << std::setw(6) << camera.x() << "," << std::setw(6) << camera.y() << "," << std::setw(6) << camera.z() << ")\n"
        << L"delta_time: " << std::setprecision(5) << delta_time << '\n';

    text.setPosition(10, gl.getHeight() - 10 - 180);
    text.setText(ss.str().c_str());
    text.Draw();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}