#include <SDL.h>
#include <SDL_opengl.h>
#include <cmath>
#include <SDL_image.h>
#include <cassert>


#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif



/**
* Author: [Pablo O'Hop]
* Assignment: Lunar Lander
* Date due: 2024-09-03, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/




const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const float GRAVITY = 0.00001f;
const float THRUST = 0.00005f;
const float ACCELERATION = 0.00002f;
const float FRICTION = 1.0f;
const int TERRAIN_POINTS = 10;
const float MAX_VELOCITY = 0.01f;
const int NUM_PLATFORMS = 2;
const char LOSE_FILEPATH[] = "/Users/pabloohop/Desktop/SDLSimple/SDLSimple/images/mfailed.png";
const char WIN_FILEPATH[] = "/Users/pabloohop/Desktop/SDLSimple/SDLSimple/images/maccombanner.jpg";
//the above filepaths were the only way i could get the images to work. you might have
//to adjust the names so they fit with your filepaths. this LITERALLY was the only way
//they worked on my computer. Also they were in a folder titled "images" on my computer.



SDL_Window* displayWindow;
bool gameIsRunning = true;
float landerX = 0.0f;
float landerY = 0.25f;
float landerVelocityX = 0.0f;
float landerVelocityY = 0.0f;
float landerAccelerationX = 0.0f;
float landerAngle = 0.0f;
bool thrusting = false;

float terrain[TERRAIN_POINTS * 2];
struct Platform {
    float startX;
    float endX;
    float y;
};

GLuint LoadTexture(const char* filepath) {
    SDL_Surface* surface = IMG_Load(filepath);
    if (surface == NULL) {
        assert(false);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    SDL_PixelFormat* format = surface->format;
    GLenum textureFormat = GL_RGB;
    if (format->BytesPerPixel == 4) {
        textureFormat = GL_RGBA;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, textureFormat, surface->w, surface->h, 0, textureFormat, GL_UNSIGNED_BYTE, surface->pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    SDL_FreeSurface(surface);

    return textureID;
}


Platform platforms[NUM_PLATFORMS];

void GenerateTerrain() {
    float step = 2.0f / (TERRAIN_POINTS - 1);
    for (int i = 0; i < TERRAIN_POINTS; ++i) {
        terrain[i * 2] = -1.0f + step * i;
        terrain[i * 2 + 1] = ((rand() % 200) / 1000.0f) - 0.5f;
    }

    for (int i = 0; i < NUM_PLATFORMS; ++i) {
        int segment = rand() % (TERRAIN_POINTS - 1);
        platforms[i].startX = terrain[segment * 2];
        platforms[i].endX = terrain[(segment + 1) * 2];
        platforms[i].y = terrain[segment * 2 + 1];
    }
}

void DrawLander() {
    glPushMatrix();
    glTranslatef(landerX, landerY, 0.0f);
    glRotatef(landerAngle, 0.0f, 0.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.02f, -0.02f);
    glVertex2f(0.02f, -0.02f);
    glVertex2f(0.0f, 0.02f);
    glEnd();
    if (thrusting) {
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.5f, 0.0f);
        glVertex2f(-0.01f, -0.02f);
        glVertex2f(0.01f, -0.02f);
        glVertex2f(0.0f, -0.05f);
        glEnd();
    }
    glPopMatrix();
}

void DrawTerrain() {
    glBegin(GL_LINE_STRIP);
    glColor3f(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < TERRAIN_POINTS; ++i) {
        glVertex2f(terrain[i * 2], terrain[i * 2 + 1]);
    }
    glEnd();
}

void DrawPlatforms() {
    glColor3f(1.0f, 0.0f, 0.0f);
    for (int i = 0; i < NUM_PLATFORMS; ++i) {
        glBegin(GL_LINES);
        glVertex2f(platforms[i].startX, platforms[i].y);
        glVertex2f(platforms[i].endX, platforms[i].y);
        glEnd();
    }
}
bool gameOver = false;
bool gameWon = false;

void UpdateLander() {
    landerVelocityY -= GRAVITY;
    if (thrusting) {
        float angleRadians = landerAngle * M_PI / 180.0f;
        landerVelocityY += THRUST * cos(angleRadians);
        landerVelocityX += THRUST * sin(angleRadians);
    }

    landerVelocityX += landerAccelerationX;
    landerVelocityX *= FRICTION;
    landerAccelerationX = 0.0f;

    landerVelocityX = fminf(fmaxf(landerVelocityX, -MAX_VELOCITY), MAX_VELOCITY);
    landerVelocityY = fminf(fmaxf(landerVelocityY, -MAX_VELOCITY), MAX_VELOCITY);

    landerX += landerVelocityX;
    landerY += landerVelocityY;

    if (landerX < -1.0f) landerX = -1.0f;
    if (landerX > 1.0f) landerX = 1.0f;
    if (landerY < -1.0f) landerY = -1.0f;
    if (landerY > 1.0f) landerY = 1.0f;

    for (int i = 0; i < TERRAIN_POINTS - 1; ++i) {
        float x1 = terrain[i * 2];
        float y1 = terrain[i * 2 + 1];
        float x2 = terrain[(i + 1) * 2];
        float y2 = terrain[(i + 1) * 2 + 1];
        if (landerX >= x1 && landerX <= x2) {
            float terrainHeight = y1 + (y2 - y1) * (landerX - x1) / (x2 - x1);
            if (landerY <= terrainHeight) {
                gameOver = true;
                gameWon = false;
                break;
            }
        }
    }

    for (int i = 0; i < NUM_PLATFORMS; ++i) {
        if (landerX >= platforms[i].startX && landerX <= platforms[i].endX && landerY <= platforms[i].y) {
            gameOver = true;
            gameWon = true;
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Homework 3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    GLuint winTexture = LoadTexture(WIN_FILEPATH);
    GLuint loseTexture = LoadTexture(LOSE_FILEPATH);
    
    GenerateTerrain();

    SDL_Event event;
    while (gameIsRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                gameIsRunning = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    thrusting = true;
                } else if (event.key.keysym.sym == SDLK_LEFT) {
                    landerAccelerationX -= ACCELERATION;
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    landerAccelerationX += ACCELERATION;
                }
            } else if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    thrusting = false;
                }
            }
        }

        UpdateLander();

        glClear(GL_COLOR_BUFFER_BIT);
        DrawTerrain();
        DrawPlatforms();
        DrawLander();

        if (gameOver) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, gameWon ? winTexture : loseTexture);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex2f(-1, -1);
            glTexCoord2f(1, 1); glVertex2f(1, -1);
            glTexCoord2f(1, 0); glVertex2f(1, 1);
            glTexCoord2f(0, 0); glVertex2f(-1, 1);
            glEnd();
            glDisable(GL_TEXTURE_2D);
        }

        SDL_GL_SwapWindow(displayWindow);
    }

    SDL_Quit();
    return 0;
}
