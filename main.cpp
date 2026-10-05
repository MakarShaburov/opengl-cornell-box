//   Управление (Можно прочитать в консоли)
//   стрелки или WASD  - поворот камеры
//   2 / 3 - приблизить / отдалить
//   z - разбиение полигонов вкл/выкл
//   x - текстура пола
//   c - тени
//   v - сглаживание (antialiasing)
//   b - стеклянный шар
//   r - сбросить камеру
//   Esc - выход

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const float PI = 3.14159265f;

int winW = 600, winH = 600;

bool useSubdiv = true;
bool useTexture = true;
bool useShadows = true;
bool useAA = true;
bool useGlass = true;

int accumBits = 0;
int stencilBits = 0;

// камера: 278, 273, -800 смотрит вдоль +z
float camYaw = 0.0f, camPitch = 0.0f;
float camDist = 1080.0f;
float camTarget[3] = { 278.0f, 273.0f, 280.0f };
float fovY;

GLuint floorTex;
GLuint listRoom = 0, listBlocks, listFloor;

struct vec3 { float x, y, z; };

vec3 mkv(float x, float y, float z) { vec3 r = { x, y, z }; return r; }
vec3 mkv(const float* p) { return mkv(p[0], p[1], p[2]); }
vec3 add(vec3 a, vec3 b) { return mkv(a.x + b.x, a.y + b.y, a.z + b.z); }
vec3 sub(vec3 a, vec3 b) { return mkv(a.x - b.x, a.y - b.y, a.z - b.z); }
vec3 mul(vec3 a, float k) { return mkv(a.x * k, a.y * k, a.z * k); }
float dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
vec3 cross(vec3 a, vec3 b)
{
    return mkv(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
vec3 normalize(vec3 a)
{
    float l = sqrtf(dot(a, a));
    if (l < 1e-6f) return a;
    return mul(a, 1.0f / l);
}

// геометрия из файла "Параметры геометрии"

float floorV[4][3] = {
    { 552.8f, 0.0f,   0.0f },
    {   0.0f, 0.0f,   0.0f },
    {   0.0f, 0.0f, 559.2f },
    { 549.6f, 0.0f, 559.2f }
};

float lightV[4][3] = {
    { 343.0f, 548.8f, 227.0f },
    { 343.0f, 548.8f, 332.0f },
    { 213.0f, 548.8f, 332.0f },
    { 213.0f, 548.8f, 227.0f }
};

float ceilV[4][3] = {
    { 556.0f, 548.8f,   0.0f },
    { 556.0f, 548.8f, 559.2f },
    {   0.0f, 548.8f, 559.2f },
    {   0.0f, 548.8f,   0.0f }
};

float backV[4][3] = {
    { 549.6f,   0.0f, 559.2f },
    {   0.0f,   0.0f, 559.2f },
    {   0.0f, 548.8f, 559.2f },
    { 556.0f, 548.8f, 559.2f }
};

// правая стена (зеленая)
float rightV[4][3] = {
    { 0.0f,   0.0f, 559.2f },
    { 0.0f,   0.0f,   0.0f },
    { 0.0f, 548.8f,   0.0f },
    { 0.0f, 548.8f, 559.2f }
};

// левая стена (красная)
float leftV[4][3] = {
    { 552.8f,   0.0f,   0.0f },
    { 549.6f,   0.0f, 559.2f },
    { 556.0f, 548.8f, 559.2f },
    { 556.0f, 548.8f,   0.0f }
};

float shortBlock[5][4][3] = {
    { { 130.0f, 165.0f,  65.0f }, {  82.0f, 165.0f, 225.0f }, { 240.0f, 165.0f, 272.0f }, { 290.0f, 165.0f, 114.0f } },
    { { 290.0f,   0.0f, 114.0f }, { 290.0f, 165.0f, 114.0f }, { 240.0f, 165.0f, 272.0f }, { 240.0f,   0.0f, 272.0f } },
    { { 130.0f,   0.0f,  65.0f }, { 130.0f, 165.0f,  65.0f }, { 290.0f, 165.0f, 114.0f }, { 290.0f,   0.0f, 114.0f } },
    { {  82.0f,   0.0f, 225.0f }, {  82.0f, 165.0f, 225.0f }, { 130.0f, 165.0f,  65.0f }, { 130.0f,   0.0f,  65.0f } },
    { { 240.0f,   0.0f, 272.0f }, { 240.0f, 165.0f, 272.0f }, {  82.0f, 165.0f, 225.0f }, {  82.0f,   0.0f, 225.0f } }
};

float tallBlock[5][4][3] = {
    { { 423.0f, 330.0f, 247.0f }, { 265.0f, 330.0f, 296.0f }, { 314.0f, 330.0f, 456.0f }, { 472.0f, 330.0f, 406.0f } },
    { { 423.0f,   0.0f, 247.0f }, { 423.0f, 330.0f, 247.0f }, { 472.0f, 330.0f, 406.0f }, { 472.0f,   0.0f, 406.0f } },
    { { 472.0f,   0.0f, 406.0f }, { 472.0f, 330.0f, 406.0f }, { 314.0f, 330.0f, 456.0f }, { 314.0f,   0.0f, 456.0f } },
    { { 314.0f,   0.0f, 456.0f }, { 314.0f, 330.0f, 456.0f }, { 265.0f, 330.0f, 296.0f }, { 265.0f,   0.0f, 296.0f } },
    { { 265.0f,   0.0f, 296.0f }, { 265.0f, 330.0f, 296.0f }, { 423.0f, 330.0f, 247.0f }, { 423.0f,   0.0f, 247.0f } }
};

// шар стоит на низком блоке 
float spherePos[3] = { 185.0f, 165.0f + 55.0f, 169.0f };
float sphereR = 55.0f;

// --- мышь: выбор и перетаскивание объектов ---
enum DragObject { OBJ_NONE = -1, OBJ_SHORT = 0, OBJ_TALL = 1, OBJ_SPHERE = 2 };
DragObject selectedObject = OBJ_NONE;
bool mouseDragging = false;
int lastMouseX = 0, lastMouseY = 0;

float shortOffset[3] = { 0.0f, 0.0f, 0.0f };
float tallOffset[3] = { 0.0f, 0.0f, 0.0f };

float selectedColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

// протяженный источник заменяем 4-мя точечными по углам лампы
float lightPos[4][4] = {
    { 328.0f, 545.0f, 242.0f, 1.0f },
    { 328.0f, 545.0f, 317.0f, 1.0f },
    { 228.0f, 545.0f, 317.0f, 1.0f },
    { 228.0f, 545.0f, 242.0f, 1.0f }
};
// слабый "отраженный" свет, иначе потолок совсем черный
float fillPos[4] = { 278.0f, 260.0f, -150.0f, 1.0f };

// цвета стен
float colWhite[4] = { 0.74f, 0.72f, 0.68f, 1.0f };
float colRed[4] = { 0.66f, 0.07f, 0.05f, 1.0f };
float colGreen[4] = { 0.15f, 0.47f, 0.10f, 1.0f };
float colTex[4] = { 0.95f, 0.93f, 0.90f, 1.0f };

void setMaterial(const float* c)
{
    float spec[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
}

// точка на четырехугольнике по параметрам s,t
vec3 quadPoint(vec3 a, vec3 b, vec3 c, vec3 d, float s, float t)
{
    vec3 p1 = add(mul(a, 1 - s), mul(b, s));
    vec3 p2 = add(mul(d, 1 - s), mul(c, s));
    return add(mul(p1, 1 - t), mul(p2, t));
}

// Рисует четырехугольник, разбитый на n*n маленьких квадов.
// ref - точка, относительно которой разворачиваем нормаль:
// для стен нормаль смотрит к ref (внутрь комнаты), для блоков от ref (наружу)
void drawQuad(const float v[4][3], int n, vec3 ref, bool outward, float texRep)
{
    vec3 a = mkv(v[0]), b = mkv(v[1]), c = mkv(v[2]), d = mkv(v[3]);
    vec3 nrm = normalize(cross(sub(b, a), sub(d, a)));
    vec3 center = mul(add(add(a, b), add(c, d)), 0.25f);

    float side = dot(nrm, sub(ref, center));
    if ((outward && side > 0) || (!outward && side < 0))
        nrm = mul(nrm, -1.0f);

    glNormal3f(nrm.x, nrm.y, nrm.z);
    glBegin(GL_QUADS);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            float s0 = (float)i / n, s1 = (float)(i + 1) / n;
            float t0 = (float)j / n, t1 = (float)(j + 1) / n;
            float ss[4] = { s0, s1, s1, s0 };
            float tt[4] = { t0, t0, t1, t1 };
            for (int k = 0; k < 4; k++) {
                vec3 p = quadPoint(a, b, c, d, ss[k], tt[k]);
                glTexCoord2f(ss[k] * texRep, tt[k] * texRep);
                glVertex3f(p.x, p.y, p.z);
            }
        }
    }
    glEnd();
}


void drawBlock(float faces[5][4][3], int n)
{
    // центр блока - среднее по всем вершинам
    vec3 c = mkv(0, 0, 0);
    for (int f = 0; f < 5; f++)
        for (int k = 0; k < 4; k++)
            c = add(c, mkv(faces[f][k]));
    c = mul(c, 1.0f / 20.0f);

    for (int f = 0; f < 5; f++)
        drawQuad(faces[f], n, c, true, 1.0f);
}

void setSelectedOrNormal(DragObject obj)
{
    if (selectedObject == obj)
        setMaterial(selectedColor);
    else
        setMaterial(colWhite);
}

void drawBlocksInteractive()
{
    int nb = useSubdiv ? 8 : 1;

    glPushMatrix();
    glTranslatef(shortOffset[0], shortOffset[1], shortOffset[2]);
    setSelectedOrNormal(OBJ_SHORT);
    drawBlock(shortBlock, nb);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(tallOffset[0], tallOffset[1], tallOffset[2]);
    setSelectedOrNormal(OBJ_TALL);
    drawBlock(tallBlock, nb);
    glPopMatrix();
}

// текстура плитки для пола
void makeFloorTexture()
{
    const int S = 256;
    static unsigned char img[S][S][3];

    srand(1234);
    for (int y = 0; y < S; y++) {
        for (int x = 0; x < S; x++) {
            int tx = x / 128, ty = y / 128;
            int lx = x % 128, ly = y % 128;

            float c = ((tx + ty) % 2 == 0) ? 0.88f : 0.76f;
            c += (rand() % 100) / 100.0f * 0.05f - 0.025f;
            c += 0.025f * sinf(x * 0.08f + 2.0f * sinf(y * 0.045f)); // разводы как у камня

            if (lx < 3 || ly < 3) c = 0.42f; // швы между плитками
            if (c > 1.0f) c = 1.0f;
            if (c < 0.0f) c = 0.0f;

            img[y][x][0] = (unsigned char)(c * 255);
            img[y][x][1] = (unsigned char)(c * 248);
            img[y][x][2] = (unsigned char)(c * 236);
        }
    }

    glGenTextures(1, &floorTex);
    glBindTexture(GL_TEXTURE_2D, floorTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, S, S, GL_RGB, GL_UNSIGNED_BYTE, img);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

// Всю статичную геометрию в display-списки,
// чтобы при сглаживании не считать заново
void buildLists()
{
    int n = useSubdiv ? 24 : 1;
    int nb = useSubdiv ? 8 : 1;
    vec3 roomCenter = mkv(278.0f, 274.0f, 280.0f);

    if (listRoom) glDeleteLists(listRoom, 3);
    listRoom = glGenLists(3);
    listBlocks = listRoom + 1;
    listFloor = listRoom + 2;

    glNewList(listRoom, GL_COMPILE);
    setMaterial(colWhite);
    drawQuad(ceilV, n, roomCenter, false, 1.0f);
    drawQuad(backV, n, roomCenter, false, 1.0f);
    setMaterial(colRed);
    drawQuad(leftV, n, roomCenter, false, 1.0f);
    setMaterial(colGreen);
    drawQuad(rightV, n, roomCenter, false, 1.0f);

    // сама лампа
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.95f);
    glBegin(GL_QUADS);
    for (int i = 0; i < 4; i++)
        glVertex3f(lightV[i][0], lightV[i][1] - 0.5f, lightV[i][2]);
    glEnd();
    glEnable(GL_LIGHTING);
    glEndList();

    glNewList(listBlocks, GL_COMPILE);
    setMaterial(colWhite);
    drawBlock(shortBlock, nb);
    drawBlock(tallBlock, nb);
    glEndList();

    glNewList(listFloor, GL_COMPILE);
    if (useTexture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, floorTex);
        setMaterial(colTex);
    }
    else {
        setMaterial(colWhite);
    }
    drawQuad(floorV, n, roomCenter, false, 3.0f);
    glDisable(GL_TEXTURE_2D);
    glEndList();
}


void setupLights()
{
    float amb[4] = { 0.20f, 0.20f, 0.20f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

    float dif[4] = { 0.36f, 0.34f, 0.30f, 1.0f };
    float zero[4] = { 0, 0, 0, 1 };
    for (int i = 0; i < 4; i++) {
        GLenum l = GL_LIGHT0 + i;
        glLightfv(l, GL_AMBIENT, zero);
        glLightfv(l, GL_DIFFUSE, dif);
        glLightfv(l, GL_SPECULAR, dif);
        glLightf(l, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(l, GL_LINEAR_ATTENUATION, 0.0f);
        glLightf(l, GL_QUADRATIC_ATTENUATION, 0.0000022f); // примерно
        glEnable(l);
    }

    float fill[4] = { 0.30f, 0.29f, 0.27f, 1.0f };
    glLightfv(GL_LIGHT4, GL_AMBIENT, zero);
    glLightfv(GL_LIGHT4, GL_DIFFUSE, fill);
    glLightfv(GL_LIGHT4, GL_SPECULAR, zero);
    glEnable(GL_LIGHT4);
}

// матрица проекции тени на плоскость plane от точечного источника light
void shadowMatrix(float m[16], const float plane[4], const float light[4])
{
    float d = plane[0] * light[0] + plane[1] * light[1] + plane[2] * light[2] + plane[3] * light[3];
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++)
            m[col * 4 + row] = (row == col ? d : 0.0f) - light[row] * plane[col];
}

void drawGlassSphere()
{
    float dif[4] = { 0.55f, 0.75f, 0.85f, 0.3f };
    if (selectedObject == OBJ_SPHERE) {
        dif[0] = selectedColor[0];
        dif[1] = selectedColor[1];
        dif[2] = selectedColor[2];
        dif[3] = 0.75f;
    }
    float spec[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 90.0f);

    glPushMatrix();
    glTranslatef(spherePos[0], spherePos[1], spherePos[2]);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_CULL_FACE);

    // сначала задняя половина шара, передняя
    glCullFace(GL_FRONT);
    glutSolidSphere(sphereR, 48, 32);
    glCullFace(GL_BACK);
    glutSolidSphere(sphereR, 48, 32);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glPopMatrix();
}


void drawBlockShadow(float faces[5][4][3], const float offset[3], int n)
{
    glPushMatrix();
    glTranslatef(offset[0], offset[1], offset[2]);
    drawBlock(faces, n);
    glPopMatrix();
}

// Тени на пол Пол помечен единицей, тень рисуется только там,
// От каждого из 4-х источников своя полупрозрачная тень получается полутень.
void drawShadows()
{
    float plane[4] = { 0.0f, 1.0f, 0.0f, 0.0f }; // y = 0
    float m[16];

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_STENCIL_TEST);

    for (int i = 0; i < 4; i++) {
        if (i > 0) {
            // заново отмечаем пол
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthFunc(GL_LEQUAL);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glCallList(listFloor);
            glDepthFunc(GL_LESS);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }

        shadowMatrix(m, plane, lightPos[i]);

        glDisable(GL_DEPTH_TEST);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

        glPushMatrix();
        glMultMatrixf(m);
        glColor4f(0.0f, 0.0f, 0.0f, 0.16f);
        int nb = useSubdiv ? 8 : 1;
        drawBlockShadow(shortBlock, shortOffset, nb);
        drawBlockShadow(tallBlock, tallOffset, nb);
        if (useGlass) {
            // от стекла тень светлее
            glColor4f(0.0f, 0.02f, 0.03f, 0.06f);
            glTranslatef(spherePos[0], spherePos[1], spherePos[2]);
            glutSolidSphere(sphereR, 24, 16);
        }
        glPopMatrix();

        glEnable(GL_DEPTH_TEST);
    }

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// jx, jy - сдвиг в долях пикселя для сглаживания
void setProjection(float jx, float jy)
{
    float zNear = 50.0f, zFar = 4000.0f;
    float top = zNear * tanf(fovY * PI / 360.0f);
    float bottom = -top;
    float right = top * (float)winW / winH;
    float left = -right;

    float dx = -jx * (right - left) / winW;
    float dy = -jy * (top - bottom) / winH;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(left + dx, right + dx, bottom + dy, top + dy, zNear, zFar);
    glMatrixMode(GL_MODELVIEW);
}

void renderScene(float jx, float jy)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    setProjection(jx, jy);
    glLoadIdentity();

    float yaw = camYaw * PI / 180.0f, pitch = camPitch * PI / 180.0f;
    float dir[3] = { sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch) };
    gluLookAt(camTarget[0] - camDist * dir[0],
        camTarget[1] - camDist * dir[1],
        camTarget[2] - camDist * dir[2],
        camTarget[0], camTarget[1], camTarget[2],
        0.0, 1.0, 0.0);

    for (int i = 0; i < 4; i++)
        glLightfv(GL_LIGHT0 + i, GL_POSITION, lightPos[i]);
    glLightfv(GL_LIGHT4, GL_POSITION, fillPos);

    glCallList(listRoom);
    drawBlocksInteractive();

    // пол рисуем последним из непрозрачного, чтобы в стенсил попала только видимая его часть
    if (useShadows && stencilBits > 0) {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glCallList(listFloor);
        glDisable(GL_STENCIL_TEST);
        drawShadows();
    }
    else {
        glCallList(listFloor);
    }

    if (useGlass)
        drawGlassSphere();
}

// смещения для 8 проходов
const float jitter8[8][2] = {
    { -0.334818f,  0.435331f }, {  0.286438f, -0.393495f },
    {  0.459462f,  0.141540f }, { -0.414498f, -0.192829f },
    { -0.183790f,  0.082102f }, { -0.079263f, -0.317383f },
    {  0.102254f,  0.299133f }, {  0.164216f, -0.054399f }
};

void updateTitle()
{
    char buf[200];
    sprintf_s(buf, "Cornell Box  [z]Light:%s [x]Tex:%s [c]Shadows:%s [v]Sm:%s [b]GlassObj:%s",
        useSubdiv ? "on" : "off", useTexture ? "on" : "off",
        useShadows ? "on" : "off", useAA ? "on" : "off", useGlass ? "on" : "off");
    glutSetWindowTitle(buf);
}

void display()
{
    if (useAA && accumBits > 0) {
        // сглаживание через аккумуляторный буфер
        // рендерим сцену 8 раз со сдвигом меньше пикселя и усредняем
        glClear(GL_ACCUM_BUFFER_BIT);
        for (int i = 0; i < 8; i++) {
            renderScene(jitter8[i][0], jitter8[i][1]);
            glAccum(GL_ACCUM, 1.0f / 8.0f);
        }
        glAccum(GL_RETURN, 1.0f);
    }
    else {
        renderScene(0.0f, 0.0f);
    }
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w;
    winH = h;
    glViewport(0, 0, w, h);
}


vec3 cameraForward()
{
    float yaw = camYaw * PI / 180.0f, pitch = camPitch * PI / 180.0f;
    return normalize(mkv(sinf(yaw) * cosf(pitch),
        sinf(pitch),
        cosf(yaw) * cosf(pitch)));
}

vec3 cameraRight()
{
    return normalize(cross(cameraForward(), mkv(0.0f, 1.0f, 0.0f)));
}

vec3 cameraUp()
{
    return normalize(cross(cameraRight(), cameraForward()));
}

void makeMouseRay(int mx, int my, vec3& origin, vec3& direction)
{
    vec3 forward = cameraForward();
    vec3 right = cameraRight();
    vec3 up = cameraUp();

    float aspect = (float)winW / (float)winH;
    float tanHalf = tanf(fovY * PI / 360.0f);
    float nx = 2.0f * mx / (float)winW - 1.0f;
    float ny = 1.0f - 2.0f * my / (float)winH;

    direction = normalize(add(add(forward,
        mul(right, nx * aspect * tanHalf)),
        mul(up, ny * tanHalf)));

    origin = sub(mkv(camTarget[0], camTarget[1], camTarget[2]),
        mul(forward, camDist));
}

bool raySphere(vec3 ro, vec3 rd, vec3 center, float radius, float& t)
{
    vec3 oc = sub(ro, center);
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0f) return false;

    float sd = sqrtf(disc);
    float t0 = -b - sd;
    float t1 = -b + sd;
    t = (t0 >= 0.0f) ? t0 : t1;
    return t >= 0.0f;
}

bool rayAABB(vec3 ro, vec3 rd, vec3 mn, vec3 mx, float& tHit)
{
    float tmin = 0.0f, tmax = 1e30f;
    float roA[3] = { ro.x, ro.y, ro.z };
    float rdA[3] = { rd.x, rd.y, rd.z };
    float mnA[3] = { mn.x, mn.y, mn.z };
    float mxA[3] = { mx.x, mx.y, mx.z };

    for (int i = 0; i < 3; ++i) {
        if (fabsf(rdA[i]) < 1e-7f) {
            if (roA[i] < mnA[i] || roA[i] > mxA[i]) return false;
        }
        else {
            float a = (mnA[i] - roA[i]) / rdA[i];
            float b = (mxA[i] - roA[i]) / rdA[i];
            if (a > b) { float tmp = a; a = b; b = tmp; }
            if (a > tmin) tmin = a;
            if (b < tmax) tmax = b;
            if (tmin > tmax) return false;
        }
    }
    tHit = tmin;
    return true;
}

void blockBounds(float faces[5][4][3], const float offset[3], vec3& mn, vec3& mx)
{
    mn = mkv(1e30f, 1e30f, 1e30f);
    mx = mkv(-1e30f, -1e30f, -1e30f);

    for (int f = 0; f < 5; ++f)
        for (int k = 0; k < 4; ++k) {
            vec3 p = mkv(faces[f][k]);
            p.x += offset[0];
            p.y += offset[1];
            p.z += offset[2];

            if (p.x < mn.x) mn.x = p.x;
            if (p.y < mn.y) mn.y = p.y;
            if (p.z < mn.z) mn.z = p.z;
            if (p.x > mx.x) mx.x = p.x;
            if (p.y > mx.y) mx.y = p.y;
            if (p.z > mx.z) mx.z = p.z;
        }
}

bool rayPlane(vec3 ro, vec3 rd, float y, vec3& hit)
{
    if (fabsf(rd.y) < 1e-7f) return false;

    float t = (y - ro.y) / rd.y;
    if (t < 0.0f) return false;

    hit = add(ro, mul(rd, t));
    return true;
}

void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        vec3 ro, rd;
        makeMouseRay(x, y, ro, rd);

        float bestT = 1e30f;
        float t;
        DragObject hit = OBJ_NONE;

        if (useGlass && raySphere(ro, rd, mkv(spherePos), sphereR, t) && t < bestT) {
            bestT = t;
            hit = OBJ_SPHERE;
        }

        vec3 mn, mx;
        blockBounds(shortBlock, shortOffset, mn, mx);
        if (rayAABB(ro, rd, mn, mx, t) && t < bestT) {
            bestT = t;
            hit = OBJ_SHORT;
        }

        blockBounds(tallBlock, tallOffset, mn, mx);
        if (rayAABB(ro, rd, mn, mx, t) && t < bestT) {
            bestT = t;
            hit = OBJ_TALL;
        }

        selectedObject = hit;
        mouseDragging = (hit != OBJ_NONE);
        lastMouseX = x;
        lastMouseY = y;

        if (mouseDragging)
            glutSetCursor(GLUT_CURSOR_CROSSHAIR);

        glutPostRedisplay();
    }
    else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        mouseDragging = false;
        selectedObject = OBJ_NONE;
        glutSetCursor(GLUT_CURSOR_INHERIT);
        glutPostRedisplay();
    }
}

// Пересечение луча мыши с плоскостью, параллельной экрану.
// Плоскость задается точкой и нормалью камеры.
bool rayCameraPlane(vec3 ro, vec3 rd, vec3 planePoint, vec3 planeNormal, vec3& hit)
{
    float denom = dot(rd, planeNormal);
    if (fabsf(denom) < 1e-7f)
        return false;

    float t = dot(sub(planePoint, ro), planeNormal) / denom;
    if (t < 0.0f)
        return false;

    hit = add(ro, mul(rd, t));
    return true;
}

vec3 selectedObjectCenter()
{
    if (selectedObject == OBJ_SHORT) {
        vec3 c = mkv(0, 0, 0);
        for (int f = 0; f < 5; ++f)
            for (int k = 0; k < 4; ++k)
                c = add(c, mkv(shortBlock[f][k]));
        c = mul(c, 1.0f / 20.0f);
        return add(c, mkv(shortOffset));
    }

    if (selectedObject == OBJ_TALL) {
        vec3 c = mkv(0, 0, 0);
        for (int f = 0; f < 5; ++f)
            for (int k = 0; k < 4; ++k)
                c = add(c, mkv(tallBlock[f][k]));
        c = mul(c, 1.0f / 20.0f);
        return add(c, mkv(tallOffset));
    }

    return mkv(spherePos);
}

void motion(int x, int y)
{
    if (!mouseDragging || selectedObject == OBJ_NONE)
        return;

    vec3 ro0, rd0, ro1, rd1;
    makeMouseRay(lastMouseX, lastMouseY, ro0, rd0);
    makeMouseRay(x, y, ro1, rd1);

    // Фигура двигается в плоскости, параллельной плоскости камеры.
    // То есть движение мыши вправо/влево и вверх/вниз переносится
    // непосредственно в экранную плоскость, без привязки к мировому Y.
    vec3 planePoint = selectedObjectCenter();
    vec3 planeNormal = cameraForward();

    vec3 p0, p1;
    if (rayCameraPlane(ro0, rd0, planePoint, planeNormal, p0) &&
        rayCameraPlane(ro1, rd1, planePoint, planeNormal, p1)) {

        vec3 delta = sub(p1, p0);

        if (selectedObject == OBJ_SHORT) {
            shortOffset[0] += delta.x;
            shortOffset[1] += delta.y;
            shortOffset[2] += delta.z;
        }
        else if (selectedObject == OBJ_TALL) {
            tallOffset[0] += delta.x;
            tallOffset[1] += delta.y;
            tallOffset[2] += delta.z;
        }
        else {
            spherePos[0] += delta.x;
            spherePos[1] += delta.y;
            spherePos[2] += delta.z;
        }
    }

    lastMouseX = x;
    lastMouseY = y;
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27: exit(0); break;
    case 'z': useSubdiv = !useSubdiv; buildLists(); break;
    case 'x': useTexture = !useTexture; buildLists(); break;
    case 'c': useShadows = !useShadows; break;
    case 'v': useAA = !useAA; break;
    case 'b': useGlass = !useGlass; break;

    case 'a': case 'A': camYaw -= 3.0f; break;
    case 'd': case 'D': camYaw += 3.0f; break;
    case 's': case 'S': camPitch += 3.0f; break;
    case 'w': case 'W': camPitch -= 3.0f; break;

    case '2':
        camDist -= 30.0f;
        if (camDist < 600.0f) camDist = 600.0f;
        break;
    case '3':
        camDist += 30.0f;
        if (camDist > 1600.0f) camDist = 1600.0f;
        break;
    case 'r': case 'R':
        camYaw = camPitch = 0.0f;
        camDist = 1080.0f;
        break;
    }
    if (camYaw > 30.0f) camYaw = 30.0f;
    if (camYaw < -30.0f) camYaw = -30.0f;
    if (camPitch > 25.0f) camPitch = 25.0f;
    if (camPitch < -25.0f) camPitch = -25.0f;
    updateTitle();
    glutPostRedisplay();
}

void special(int key, int x, int y)
{
    if (key == GLUT_KEY_LEFT)  camYaw -= 3.0f;
    if (key == GLUT_KEY_RIGHT) camYaw += 3.0f;
    if (key == GLUT_KEY_DOWN)    camPitch += 3.0f;
    if (key == GLUT_KEY_UP)  camPitch -= 3.0f;

    if (camYaw > 30.0f) camYaw = 30.0f;
    if (camYaw < -30.0f) camYaw = -30.0f;
    if (camPitch > 25.0f) camPitch = 25.0f;
    if (camPitch < -25.0f) camPitch = -25.0f;

    glutPostRedisplay();
}

void init()
{
    fovY = 2.0f * atanf(0.0125f / 0.035f) * 180.0f / PI;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearAccum(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    glGetIntegerv(GL_ACCUM_RED_BITS, &accumBits);
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    if (accumBits == 0) printf("Accumulation buffer not available, antialiasing is off\n");
    if (stencilBits == 0) printf("Stencil buffer not available, shadows are off\n");

    setupLights();
    makeFloorTexture();
    buildLists();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_ACCUM);
    glutInitWindowSize(winW, winH);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Cornell Box");

    init();
    updateTitle();

    printf("Cornell Box\n");
    printf("arrows/WASD - rotate camera, 2/3 - zoom, r - reset\n");
    printf("z - lights, x - texture, c - shadows, v - smoothing, b - glass sphere\n");
    printf("left mouse - select and drag a figure\n");
    printf("Esc - exit\n");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMainLoop();
    return 0;
}
