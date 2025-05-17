#define WIN32_LEAN_AND_MEAN
#include <GL/freeglut.h>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <iostream>
#include <Windows.h>
#include <mmsystem.h>

// Configuration struct for game parameters
struct Config {
    static constexpr float windowWidth = 1000.0f;
    static constexpr float windowHeight = 800.0f;
    static constexpr float buttonH = 30.0f;
    static constexpr float buttonW = 100.0f;
    static constexpr float playerSize = 20.0f; // Triangle bounding box
    static constexpr float playerSpeed = 200.0f; // Pixels per second
    static constexpr float playerMouseStopDist = 10.0f; // Stop when this close to cursor
    static constexpr float bulletSize = 5.0f;
    static constexpr float bulletSpeed = 400.0f;
    static constexpr float bulletCooldown = 0.2f; // Default seconds
    static constexpr float fastBulletCooldown = 0.05f; // Faster shooting cooldown
    static constexpr float enemySize = 20.0f; // Pentagon bounding box
    static constexpr float enemyBaseSpeed = 100.0f;
    static constexpr float enemyRotationSpeed = 90.0f; // Degrees per second
    static constexpr int maxHealth = 3;
    static constexpr float spawnInterval = 1.0f; // Base spawn interval (seconds)
    static constexpr float posX = (windowWidth / 2) - buttonW / 2;
    static constexpr float posY = (windowHeight / 2) - buttonH / 2;
    static constexpr float powerUpSize = 15.0f; // Size of power-ups
    static constexpr float powerUpSpeed = 150.0f; // Speed of power-ups
    static constexpr float powerUpSpawnInterval = 5.0f; // Spawn every 5 seconds
    static constexpr float powerUpRotationSpeed = 90.0f; // Degrees per second
    static constexpr float bulletOffset = 10.0f; // Offset between parallel bullets
    static constexpr int maxBulletCount = 5; // Maximum number of bullets
    static constexpr float bulletPowerUpDuration = 10.0f; // Bullet power-up lasts 10 seconds
    static constexpr float speedPowerUpDuration = 10.0f; // Speed boost lasts 10 seconds
    static constexpr float fasterShootingDuration = 10.0f; // Faster shooting lasts 10 seconds
    static constexpr float invincibilityDuration = 10.0f; // Invincibility lasts 10 seconds
    static constexpr float scoreMultiplierDuration = 10.0f; // Score multiplier lasts 10 seconds
    static constexpr float speedBoostMultiplier = 1.5f; // Speed increase factor
    static constexpr float messageDisplayTime = 2.0f; // Message display duration
    static constexpr float wavePauseDuration = 2.0f; // Pause between waves
};

// Game state
struct GameState {
    float playerX = Config::windowWidth / 2;
    float playerY = 50.0f;
    int health = Config::maxHealth;
    int score = 0;
    int bulletCount = 1; // Number of bullets to shoot
    int wave = 1; // Current wave number
    int enemiesToSpawn = 0; // Enemies left to spawn in wave
    float bulletPowerUpEndTime = 0.0f; // Time when bullet power-up expires
    float speedBoostMultiplier = 1.0f; // Current speed multiplier
    float speedBoostEndTime = 0.0f; // Time when speed boost expires
    float fasterShootingEndTime = 0.0f; // Time when faster shooting expires
    float invincibilityEndTime = 0.0f; // Time when invincibility expires
    float scoreMultiplierEndTime = 0.0f; // Time when score multiplier expires
    float scoreMultiplier = 1.0f; // Score multiplier (e.g., 2.0 for double)
    bool gameOver = false;
    bool paused = false;
    bool useMouseControl = false; // Toggle for mouse vs keyboard movement
    float lastShotTime = 0.0f;
    float lastSpawnTime = 0.0f;
    float lastPowerUpSpawnTime = 0.0f;
    float nextWaveTime = 0.0f; // Time when next wave can start
    std::string message; // Temporary message
    float messageEndTime = 0.0f; // Time when message expires
};
GameState game;

// Bullet properties
struct Bullet {
    float x, y, dy;
};
std::vector<Bullet> bullets;

// Enemy properties
struct Enemy {
    float x, y;
    float speed;
    float rotation;
};
std::vector<Enemy> enemies;

// Star properties for space background
struct Star {
    float x, y;
};
std::vector<Star> stars;

// Power-up types
enum class PowerUpType {
    BULLET_INCREASER,
    SPEED_BOOST,
    HEALTH_RESTORE,
    FASTER_SHOOTING,
    INVINCIBILITY,
    SCORE_MULTIPLIER
};

// Power-up properties
struct PowerUp {
    PowerUpType type;
    float x, y;
    float rotation;
};
std::vector<PowerUp> powerUps;

// Movement state
bool keyA, keyD, keyW, keyS;
bool keyUp, keyDown, keyLeft, keyRight;
float mouseX, mouseY;

// Utility functions
void drawTriangle(float x, float y, float size, float r, float g, float b) {
    glColor3f(r, g, b);
    float halfSize = size / 2;
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y + halfSize); // Top
    glVertex2f(x - halfSize, y - halfSize); // Bottom-left
    glVertex2f(x + halfSize, y - halfSize); // Bottom-right
    glEnd();
}

void drawPentagon(float x, float y, float size, float rotation, float r, float g, float b) {
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 5; ++i) {
        float angle = i * 2.0f * 3.1415926535f / 5.0f;
        glVertex2f(size / 2 * cos(angle), size / 2 * sin(angle));
    }
    glEnd();
    glPopMatrix();
}

void drawSquare(float x, float y, float size, float rotation, float r, float g, float b) {
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);
    float halfSize = size / 2;
    glBegin(GL_QUADS);
    glVertex2f(-halfSize, -halfSize);
    glVertex2f(halfSize, -halfSize);
    glVertex2f(halfSize, halfSize);
    glVertex2f(-halfSize, halfSize);
    glEnd();
    glPopMatrix();
}

void drawCircle(float x, float y, float size, float rotation, float r, float g, float b) {
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 20; ++i) {
        float angle = i * 2.0f * 3.1415926535f / 20.0f;
        glVertex2f(size / 2 * cos(angle), size / 2 * sin(angle));
    }
    glEnd();
    glPopMatrix();
}

void drawCross(float x, float y, float size, float rotation, float r, float g, float b) {
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);
    float halfSize = size / 2;
    float thickness = size / 4;
    glBegin(GL_QUADS);
    glVertex2f(-halfSize, -thickness);
    glVertex2f(halfSize, -thickness);
    glVertex2f(halfSize, thickness);
    glVertex2f(-halfSize, thickness);
    glVertex2f(-thickness, -halfSize);
    glVertex2f(thickness, -halfSize);
    glVertex2f(thickness, halfSize);
    glVertex2f(-thickness, halfSize);
    glEnd();
    glPopMatrix();
}

void drawDiamond(float x, float y, float size, float rotation, float r, float g, float b) {
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);
    float halfSize = size / 2;
    glBegin(GL_QUADS);
    glVertex2f(0.0f, halfSize); // Top
    glVertex2f(halfSize, 0.0f); // Right
    glVertex2f(0.0f, -halfSize); // Bottom
    glVertex2f(-halfSize, 0.0f); // Left
    glEnd();
    glPopMatrix();
}

void drawStar(float x, float y, float size, float rotation, float r, float g, float b) {
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);
    float outer = size / 2;
    float inner = outer / 2.5f;
    glBegin(GL_POLYGON);
    for (int i = 0; i < 10; ++i) {
        float radius = (i % 2 == 0) ? outer : inner;
        float angle = i * 3.1415926535f / 5.0f;
        glVertex2f(radius * cos(angle), radius * sin(angle));
    }
    glEnd();
    glPopMatrix();
}

void drawHexagon(float x, float y, float size, float rotation, float r, float g, float b) {
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 6; ++i) {
        float angle = i * 2.0f * 3.1415926535f / 6.0f;
        glVertex2f(size / 2 * cos(angle), size / 2 * sin(angle));
    }
    glEnd();
    glPopMatrix();
}

void drawButton(float x, float y, float w, float h, const std::string& label) {
    glColor3f(0.2f, 0.2f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x + 10, y + h / 2 - 5);
    for (char c : label) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}

void drawText(float x, float y, const std::string& text) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}

void initStars(int numStars) {
    stars.clear();
    for (int i = 0; i < numStars; ++i) {
        stars.push_back({
            static_cast<float>(rand() % static_cast<int>(Config::windowWidth)),
            static_cast<float>(rand() % static_cast<int>(Config::windowHeight))
        });
    }
}

void spawnEnemy(float currentTime) {
    float speed = Config::enemyBaseSpeed + std::min(game.score * 5.0f + game.wave * 2.0f, 300.0f);
    enemies.push_back({static_cast<float>(rand() % static_cast<int>(Config::windowWidth - 20) + 10), Config::windowHeight, speed, 0.0f});
    game.lastSpawnTime = currentTime;
}

void spawnPowerUp(float currentTime) {
    PowerUpType type;
    int randType = rand() % 6;
    if (randType == 0) type = PowerUpType::BULLET_INCREASER;
    else if (randType == 1) type = PowerUpType::SPEED_BOOST;
    else if (randType == 2) type = PowerUpType::HEALTH_RESTORE;
    else if (randType == 3) type = PowerUpType::FASTER_SHOOTING;
    else if (randType == 4) type = PowerUpType::INVINCIBILITY;
    else type = PowerUpType::SCORE_MULTIPLIER;
    powerUps.push_back({type, static_cast<float>(rand() % static_cast<int>(Config::windowWidth - 20) + 10), Config::windowHeight, 0.0f});
    game.lastPowerUpSpawnTime = currentTime;
}

void restartGame() {
    game = GameState();
    bullets.clear();
    enemies.clear();
    powerUps.clear();
}

// AABB collision detection
bool checkCollision(float x1, float y1, float size1, float x2, float y2, float size2) {
    float half1 = size1 * 0.8f / 2;
    float half2 = size2 * 0.8f / 2;
    return x1 - half1 < x2 + half2 && x1 + half1 > x2 - half2 &&
           y1 - half1 < y2 + half2 && y1 + half1 > y2 - half2;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw stars
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POINTS);
    for (const auto& star : stars) {
        glVertex2f(star.x, star.y);
    }
    glEnd();

    if (game.gameOver) {
        float posX = (Config::windowWidth / 2) - (Config::buttonW / 2);
        drawText(posX, Config::posY - 50, "Game Over!");
        drawText(posX, Config::posY, "Score: " + std::to_string(game.score));
        drawButton(posX, Config::posY + 20, Config::buttonW, Config::buttonH, "Restart");
        glutSwapBuffers();
        return;
    }

    if (game.paused) {
        drawText(200, 250, "Game Paused");
        drawButton(200, 220, 100, 30, "Resume");
        glutSwapBuffers();
        return;
    }

    // Draw player (flash if invincible)
    float healthRatio = static_cast<float>(game.health) / Config::maxHealth;
    float r = healthRatio;
    float g = 1.0f;
    if (glutGet(GLUT_ELAPSED_TIME) / 1000.0f < game.invincibilityEndTime) {
        r = g = (sin(glutGet(GLUT_ELAPSED_TIME) / 100.0f) + 1) / 2; // Flashing effect
    }
    drawTriangle(game.playerX, game.playerY, Config::playerSize, r, g, 0.0f);

    // Draw bullets
    for (const auto& b : bullets) {
        drawTriangle(b.x, b.y, Config::bulletSize, 1.0f, 1.0f, 0.0f);
    }

    // Draw enemies
    for (const auto& e : enemies) {
        drawPentagon(e.x, e.y, Config::enemySize, e.rotation, 1.0f, 0.0f, 0.0f);
    }

    // Draw power-ups
    for (const auto& pu : powerUps) {
        switch (pu.type) {
            case PowerUpType::BULLET_INCREASER:
                drawSquare(pu.x, pu.y, Config::powerUpSize, pu.rotation, 0.0f, 1.0f, 0.0f); // Green
                break;
            case PowerUpType::SPEED_BOOST:
                drawCircle(pu.x, pu.y, Config::powerUpSize, pu.rotation, 0.0f, 0.0f, 1.0f); // Blue
                break;
            case PowerUpType::HEALTH_RESTORE:
                drawCross(pu.x, pu.y, Config::powerUpSize, pu.rotation, 1.0f, 1.0f, 0.0f); // Yellow
                break;
            case PowerUpType::FASTER_SHOOTING:
                drawDiamond(pu.x, pu.y, Config::powerUpSize, pu.rotation, 0.5f, 0.0f, 1.0f); // Purple
                break;
            case PowerUpType::INVINCIBILITY:
                drawStar(pu.x, pu.y, Config::powerUpSize, pu.rotation, 1.0f, 1.0f, 1.0f); // White
                break;
            case PowerUpType::SCORE_MULTIPLIER:
                drawHexagon(pu.x, pu.y, Config::powerUpSize, pu.rotation, 1.0f, 0.5f, 0.0f); // Orange
                break;
        }
    }

    // Draw UI
    drawText(10, Config::windowHeight - 30, "Score: " + std::to_string(game.score));
    drawText(10, Config::windowHeight - 50, "Health: " + std::to_string(game.health));
    drawText(10, Config::windowHeight - 70, "Wave: " + std::to_string(game.wave));
    drawText(10, Config::windowHeight - 90, "Control: " + std::string(game.useMouseControl ? "Mouse" : "Keyboard"));
    drawText(10, Config::windowHeight - 110, "Bullets: " + std::to_string(game.bulletCount));
    drawText(10, Config::windowHeight - 130, "Speed: " + std::to_string(static_cast<int>(game.speedBoostMultiplier * 100)) + "%");
    if (game.invincibilityEndTime > glutGet(GLUT_ELAPSED_TIME) / 1000.0f) {
        drawText(10, Config::windowHeight - 150, "Invincible!");
    }
    if (game.scoreMultiplierEndTime > glutGet(GLUT_ELAPSED_TIME) / 1000.0f) {
        drawText(10, Config::windowHeight - 170, "Score x" + std::to_string(static_cast<int>(game.scoreMultiplier)));
    }
    if (game.messageEndTime > glutGet(GLUT_ELAPSED_TIME) / 1000.0f) {
        drawText(10, Config::windowHeight - 190, game.message);
    }

    // Pause button
    drawButton(Config::windowWidth - 80, Config::windowHeight - 40, 80, 30, "Pause");

    glutSwapBuffers();
}

void update(int value) {
    static float lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    if (game.gameOver || game.paused) {
        glutTimerFunc(16, update, 0);
        return;
    }

    // Check power-up expirations
    if (currentTime > game.bulletPowerUpEndTime && game.bulletCount > 1) {
        game.bulletCount = 1;
    }
    if (currentTime > game.speedBoostEndTime && game.speedBoostMultiplier > 1.0f) {
        game.speedBoostMultiplier = 1.0f;
    }
    if (currentTime > game.fasterShootingEndTime) {
        // No action needed; effectiveBulletCooldown handled in shooting
    }
    if (currentTime > game.scoreMultiplierEndTime && game.scoreMultiplier > 1.0f) {
        game.scoreMultiplier = 1.0f;
    }

    // Player movement
    float effectiveSpeed = Config::playerSpeed * game.speedBoostMultiplier;
    if (game.useMouseControl) {
        float dx = mouseX - game.playerX;
        float dy = mouseY - game.playerY;
        float distance = sqrt(dx * dx + dy * dy);
        if (distance > Config::playerMouseStopDist) {
            float speed = effectiveSpeed * deltaTime;
            float moveX = (dx / distance) * speed;
            float moveY = (dy / distance) * speed;
            game.playerX += moveX;
            game.playerY += moveY;
            game.playerX = std::max(Config::playerSize / 2, std::min(Config::windowWidth - Config::playerSize / 2, game.playerX));
            game.playerY = std::max(Config::playerSize / 2, std::min(Config::windowHeight - Config::playerSize / 2, game.playerY));
        }
    } else {
        if ((keyA || keyLeft) && game.playerX > Config::playerSize / 2) game.playerX -= effectiveSpeed * deltaTime;
        if ((keyD || keyRight) && game.playerX < Config::windowWidth - Config::playerSize / 2) game.playerX += effectiveSpeed * deltaTime;
        if ((keyW || keyUp) && game.playerY < Config::windowHeight - Config::playerSize / 2) game.playerY += effectiveSpeed * deltaTime;
        if ((keyS || keyDown) && game.playerY > Config::playerSize / 2) game.playerY -= effectiveSpeed * deltaTime;
    }

    // Update bullets
    for (auto& b : bullets) {
        b.y += b.dy * deltaTime;
    }
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) {
        return b.y > Config::windowHeight;
    }), bullets.end());

    // Update enemies
    for (auto& e : enemies) {
        e.y -= e.speed * deltaTime;
        e.rotation += Config::enemyRotationSpeed * deltaTime;
    }

    // Update power-ups
    for (auto& pu : powerUps) {
        pu.y -= Config::powerUpSpeed * deltaTime;
        pu.rotation += Config::powerUpRotationSpeed * deltaTime;
    }

    // Spawn enemies (wave-based)
    float spawnInterval = std::max(Config::spawnInterval / (1.0f + game.score * 0.01f), 0.5f);
    if (game.enemiesToSpawn == 0 && enemies.empty() && currentTime > game.nextWaveTime) {
        game.enemiesToSpawn = game.wave / 2 + 1; // Fewer enemies per wave
        game.wave++;
        game.message = "Wave " + std::to_string(game.wave) + " Started!";
        game.messageEndTime = currentTime + Config::messageDisplayTime;
        game.nextWaveTime = currentTime + Config::wavePauseDuration;
    }
    if (game.enemiesToSpawn > 0 && currentTime - game.lastSpawnTime > spawnInterval) {
        spawnEnemy(currentTime);
        game.enemiesToSpawn--;
        game.lastSpawnTime = currentTime - spawnInterval + 0.5f; // Increased delay
    }

    // Spawn power-ups
    if (currentTime - game.lastPowerUpSpawnTime > Config::powerUpSpawnInterval) {
        spawnPowerUp(currentTime);
    }

    // Collision detection (bullets vs enemies)
    for (auto e = enemies.begin(); e != enemies.end();) {
        bool hit = false;
        for (auto b = bullets.begin(); b != bullets.end();) {
            if (checkCollision(b->x, b->y, Config::bulletSize, e->x, e->y, Config::enemySize)) {
                b = bullets.erase(b);
                hit = true;
                game.score += static_cast<int>(1 * game.scoreMultiplier);
                PlaySound(TEXT("C:\\c++\\shooter\\sounds\\enemyhit.wav"), NULL, SND_ASYNC | SND_FILENAME);
                break;
            } else {
                ++b;
            }
        }
        if (hit) {
            e = enemies.erase(e);
        } else {
            ++e;
        }
    }

    // Collision detection (player vs enemies)
    for (auto e = enemies.begin(); e != enemies.end();) {
        if (checkCollision(game.playerX, game.playerY, Config::playerSize, e->x, e->y, Config::enemySize)) {
            if (currentTime > game.invincibilityEndTime) {
                game.health--;
                PlaySound(TEXT("C:\\c++\\shooter\\sounds\\playerhit.wav"), NULL, SND_ASYNC | SND_FILENAME);
                if (game.health <= 0) {
                    game.gameOver = true;
                }
            }
            e = enemies.erase(e);
        } else {
            ++e;
        }
    }

    // Collision detection (player vs power-ups)
    for (auto pu = powerUps.begin(); pu != powerUps.end();) {
        if (checkCollision(game.playerX, game.playerY, Config::playerSize, pu->x, pu->y, Config::powerUpSize)) {
            switch (pu->type) {
                case PowerUpType::BULLET_INCREASER:
                    if (game.bulletCount < Config::maxBulletCount) {
                        game.bulletCount++;
                        game.bulletPowerUpEndTime = currentTime + Config::bulletPowerUpDuration;
                        game.message = "Bullet Power-Up!";
                        game.messageEndTime = currentTime + Config::messageDisplayTime;
                        PlaySound(TEXT("C:\\c++\\shooter\\sounds\\powerup.wav"), NULL, SND_ASYNC | SND_FILENAME);
                    }
                    break;
                case PowerUpType::SPEED_BOOST:
                    game.speedBoostMultiplier = Config::speedBoostMultiplier;
                    game.speedBoostEndTime = currentTime + Config::speedPowerUpDuration;
                    game.message = "Speed Boost!";
                    game.messageEndTime = currentTime + Config::messageDisplayTime;
                    PlaySound(TEXT("C:\\c++\\shooter\\sounds\\powerup.wav"), NULL, SND_ASYNC | SND_FILENAME);
                    break;
                case PowerUpType::HEALTH_RESTORE:
                    if (game.health < Config::maxHealth) {
                        game.health++;
                        game.message = "Health Restored!";
                        game.messageEndTime = currentTime + Config::messageDisplayTime;
                        PlaySound(TEXT("C:\\c++\\shooter\\sounds\\powerup.wav"), NULL, SND_ASYNC | SND_FILENAME);
                    }
                    break;
                case PowerUpType::FASTER_SHOOTING:
                    game.fasterShootingEndTime = currentTime + Config::fasterShootingDuration;
                    game.message = "Faster Shooting!";
                    game.messageEndTime = currentTime + Config::messageDisplayTime;
                    PlaySound(TEXT("C:\\c++\\shooter\\sounds\\powerup.wav"), NULL, SND_ASYNC | SND_FILENAME);
                    break;
                case PowerUpType::INVINCIBILITY:
                    game.invincibilityEndTime = currentTime + Config::invincibilityDuration;
                    game.message = "Invincibility!";
                    game.messageEndTime = currentTime + Config::messageDisplayTime;
                    PlaySound(TEXT("C:\\c++\\shooter\\sounds\\powerup.wav"), NULL, SND_ASYNC | SND_FILENAME);
                    break;
                case PowerUpType::SCORE_MULTIPLIER:
                    game.scoreMultiplier = 2.0f;
                    game.scoreMultiplierEndTime = currentTime + Config::scoreMultiplierDuration;
                    game.message = "Score Multiplier!";
                    game.messageEndTime = currentTime + Config::messageDisplayTime;
                    PlaySound(TEXT("C:\\c++\\shooter\\sounds\\powerup.wav"), NULL, SND_ASYNC | SND_FILENAME);
                    break;
            }
            pu = powerUps.erase(pu);
        } else {
            ++pu;
        }
    }

    // Remove off-screen enemies
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) {
        return e.y < 0;
    }), enemies.end());

    // Remove off-screen power-ups
    powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(), [](const PowerUp& pu) {
        return pu.y < 0;
    }), powerUps.end());

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboardDown(unsigned char key, int x, int y) {
    if (key == 'a' || key == 'A') keyA = true;
    if (key == 'd' || key == 'D') keyD = true;
    if (key == 'w' || key == 'W') keyW = true;
    if (key == 's' || key == 'S') keyS = true;
    if (key == 'p' || key == 'P') game.paused = !game.paused;
    if (key == 'r' || key == 'R') restartGame();
    if (key == 'm' || key == 'M') game.useMouseControl = !game.useMouseControl;
    if (key == ' ') {
        float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        float effectiveCooldown = (currentTime < game.fasterShootingEndTime) ? Config::fastBulletCooldown : Config::bulletCooldown;
        if (currentTime - game.lastShotTime > effectiveCooldown) {
            float startX = game.playerX - (game.bulletCount - 1) * Config::bulletOffset / 2;
            for (int i = 0; i < game.bulletCount; ++i) {
                bullets.push_back({startX + i * Config::bulletOffset, game.playerY + Config::playerSize / 2, Config::bulletSpeed});
            }
            game.lastShotTime = currentTime;
            PlaySound(TEXT("C:\\c++\\shooter\\sounds\\shoot.wav"), NULL, SND_ASYNC | SND_FILENAME);
        }
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    if (key == 'a' || key == 'A') keyA = false;
    if (key == 'd' || key == 'D') keyD = false;
    if (key == 'w' || key == 'W') keyW = false;
    if (key == 's' || key == 'S') keyS = false;
}

void specialDown(int key, int x, int y) {
    if (key == GLUT_KEY_UP) keyUp = true;
    if (key == GLUT_KEY_DOWN) keyDown = true;
    if (key == GLUT_KEY_LEFT) keyLeft = true;
    if (key == GLUT_KEY_RIGHT) keyRight = true;
}

void specialUp(int key, int x, int y) {
    if (key == GLUT_KEY_UP) keyUp = false;
    if (key == GLUT_KEY_DOWN) keyDown = false;
    if (key == GLUT_KEY_LEFT) keyLeft = false;
    if (key == GLUT_KEY_RIGHT) keyRight = false;
}

void mouse(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return;

    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    int flippedY = Config::windowHeight - y;

    if (!game.gameOver && !game.paused && button == GLUT_LEFT_BUTTON) {
        float effectiveCooldown = (currentTime < game.fasterShootingEndTime) ? Config::fastBulletCooldown : Config::bulletCooldown;
        if (currentTime - game.lastShotTime > effectiveCooldown) {
            float startX = game.playerX - (game.bulletCount - 1) * Config::bulletOffset / 2;
            for (int i = 0; i < game.bulletCount; ++i) {
                bullets.push_back({startX + i * Config::bulletOffset, game.playerY + Config::playerSize / 2, Config::bulletSpeed});
            }
            game.lastShotTime = currentTime;
            PlaySound(TEXT("sounds/shoot.wav"), NULL, SND_ASYNC | SND_FILENAME);
        }
    }

    if (!game.gameOver && x >= Config::windowWidth - 80 && x <= Config::windowWidth && flippedY >= Config::windowHeight - 40 && flippedY <= Config::windowHeight - 10) {
        game.paused = true;
    }

    if (game.paused && x >= 200 && x <= 300 && flippedY >= 220 && flippedY <= 250) {
        game.paused = false;
    }

    if (game.gameOver && x >= Config::posX && x <= Config::posX + 100 && flippedY >= Config::posY && flippedY <= Config::posY + 40) {
        restartGame();
    }
}

void passiveMotion(int x, int y) {
    mouseX = static_cast<float>(x);
    mouseY = Config::windowHeight - static_cast<float>(y);
}

void init() {
    srand(static_cast<unsigned int>(time(nullptr)));
    glClearColor(0, 0, 0, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, Config::windowWidth, 0, Config::windowHeight);
    initStars(200);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(static_cast<int>(Config::windowWidth), static_cast<int>(Config::windowHeight));
    glutCreateWindow("Topdown Shooter Game");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(passiveMotion);
    glutTimerFunc(16, update, 0);
    glutMainLoop();
    return 0;
}
