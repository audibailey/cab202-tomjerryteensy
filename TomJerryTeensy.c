//
// Created by Audi Bailey on 10/10/19.
//

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <usb_serial.h>
#include "cpu_speed.h"

#include "graphics.h"
#include "macros.h"
#include "lcd_model.h"
#include "cab202_adc.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// Global for controls
volatile uint8_t leftButtonCounter = 0;
volatile uint8_t leftButtonPressed = 0;
volatile uint8_t rightButtonCounter = 0;
volatile uint8_t rightButtonPressed = 0;
volatile uint8_t leftJoystickCounter = 0;
volatile uint8_t leftJoystickPressed = 0;
volatile uint8_t rightJoystickCounter = 0;
volatile uint8_t rightJoystickPressed = 0;
volatile uint8_t upJoystickCounter = 0;
volatile uint8_t upJoystickPressed = 0;
volatile uint8_t downJoystickCounter = 0;
volatile uint8_t downJoystickPressed = 0;
volatile uint8_t centerJoystickCounter = 0;
volatile uint8_t centerJoystickPressed = 0;

#define LED0PIN PB2
#define LED1PIN PB3

uint8_t LED0Brightness, LED1Brightness = 0;
uint8_t LEDBrightnessDirection = 0xFF;
volatile uint8_t LEDBrightnessChangeCount = 0;

// Global vars for game time
int gameTimer = 0;
int gameTimerSeconds, gameTimerMinutes = 0;
int gamePause = 1;
int gameTimerCounter = 0;
int timeOut = 0;
int timeOutCounter = 0;

// Global vars for levels
int gameOver = 0;
int level = 1;
int levelLoaded = 0;
int maxLevel = 2;


#define dashboardBorder 8
#define bitmapXMax 5
#define bitmapYMax 7

// Struct for the characters
typedef struct charac char_spec;
struct charac
{
    double x;
    double y;
    double xDir;
    double yDir;
    double xSpeed;
    double ySpeed;
    double xStart;
    double yStart;
    int health;
    int charScore;
    int charLevelScore;
    int charSpecialScore;
    int super;
};

// Define some constants for the characters
#define TOM_MAX_HEALTH 5
#define JERRY_MAX_HEALTH 5

// Globals for the characters
char_spec tom;
char_spec jerry;

unsigned int tomBitmap[bitmapYMax] = {
        0b11111,
        0b10101,
        0b11111,
        0b10101,
        0b11111,
        0b00000,
        0b00000,
};

unsigned int jerryBitmap[bitmapYMax] = {
        0b11111,
        0b11011,
        0b10101,
        0b11011,
        0b11111,
        0b00000,
        0b00000,
};

unsigned int jerrySuperBitmap[bitmapYMax+1] = {
        0b111111,
        0b110011,
        0b110011,
        0b110011,
        0b110011,
        0b111111,
        0b000000,
        0b000000
};

int charScaleFactor = 1;

//  Globals for the door and define constant for the door img
int doorX, doorY = 0;
int doorCalc = 0;

unsigned int doorBitmap[bitmapYMax] = {
        0b11110,
        0b11110,
        0b11110,
        0b11110,
        0b11110,
        0b00000,
        0b00000,
};

// Struct for the walls
typedef struct env_wall wall_spec;
struct env_wall
{
    double x1;
    double y1;
    double x2;
    double y2;
};

// Globals for the walls
wall_spec walls[6];
int wallCount = 0;
int wallScaleFactor = 1;

// Struct for the items
typedef struct items items_spec;
struct items
{
    int x;
    int y;

};

// Define the constants for the items
#define TRAP_MAX 5
#define CHEESE_MAX 5
#define MILK_MAX 1

// Globals for the cheese
int cheeseTimerSeconds = 0;
items_spec cheeses[CHEESE_MAX];
int cheeseCount = 0;

unsigned int cheeseBitmap[bitmapYMax] = {
        0b11110,
        0b10000,
        0b10000,
        0b11110,
        0b00000,
        0b00000,
        0b00000,
};

// Globals for the traps
int trapTimerSeconds = 0;
items_spec traps[TRAP_MAX];
int trapCount = 0;

unsigned int trapBitmap[bitmapYMax] = {
        0b11111,
        0b00100,
        0b00100,
        0b00100,
        0b00000,
        0b00000,
        0b00000,
};


int milkTimerSeconds = 0;
int milkX, milkY = 0;
int milkCalc = 0;

unsigned int milkBitmap[bitmapYMax] = {
        0b00100,
        0b01110,
        0b01110,
        0b00100,
        0b00000,
        0b00000,
        0b00000,
};

// Struct for the fireworks
typedef struct moving_items weapon_spec;
struct moving_items
{
    double x;
    double y;
    double dx;
    double dy;
    int draw;
};

#define FIREWORKS_MAX 20

// Globals for the fireworks
weapon_spec fireworks[FIREWORKS_MAX];
int fireworkCount = 0;

void setupCharacters(void);
void setupTom(void);
void setupJerry(void);
void resetEverything(void);
void welcome(void);
void setupTomJerry(void);

void usbSerialReadString(char * message){
    int c = 0;
    int buffer_count=0;

    while (c != '\n'){
        c = usb_serial_getchar();
        message[buffer_count] = c;
        buffer_count++;
    }
}

void usbSerialStringSend(char * message) {
    usb_serial_write((uint8_t *) message, strlen(message));
}

void drawCentred(unsigned char y, char* string) {
    unsigned char stringLength = 0, i = 0;
    while (string[i] != '\0') { stringLength++; i++; }
    char x = (LCD_X/2)-(stringLength * 5 / 2);
    draw_string((x > 0) ? x : 0, y, string, FG_COLOUR);
}

int bitmapWidth(unsigned int *var)
{
    int maxWidth = 0;
    for (int i = 0; i < bitmapYMax; i++) {
        int testWidth = (bitmapXMax+1)-((log(var[i] & -var[i])/log(2)) + 1);
        if (testWidth > maxWidth) {
            maxWidth = testWidth;
        }
    }

    return maxWidth;
}

int bitmapHeight(unsigned int *var)
{
    for (int i = bitmapYMax-1; i >= 0; i--) {
        if (var[i] != 0b00000) {
            return i+1;
        }
    }

    return bitmapYMax;
}

double randRange(double min, double max)
{
    double range = (max - min);
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

void drawBitmap(double x, double y, unsigned int *bitmap) {

        for (int i = 0; i < bitmapXMax; i++) {
            for (int j = 0; j < bitmapYMax; j++) {
                if (BIT_VALUE(bitmap[j], ((bitmapXMax-1) - i)) > 0) {
                    draw_pixel(x+i, y+j, FG_COLOUR);
                }
            }
        }
}

void drawSuperjerry(double x, double y, unsigned int *bitmap) {
    for (int i = 0; i < bitmapXMax+1; i++) {
        for (int j = 0; j < bitmapYMax+1; j++) {
            if (BIT_VALUE(bitmap[j], (bitmapXMax - i)) > 0) {
                draw_pixel(x+i, y+j, FG_COLOUR);
            }
        }
    }
}

int lineIntersect(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) {

    float dA = ((x4-x3)*(y1-y3) - (y4-y3)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));
    float dB = ((x2-x1)*(y1-y3) - (y2-y1)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));

    if (dA >= 0 && dA <= 1 && dB >= 0 && dB <= 1) {
        return 1;
    }
    return 0;
}

int checkLeftRightWallCollision(float x1, float y1, float x2, float y2, float rx, float ry, float rw, float rh) {
    int left = lineIntersect(x1, y1, x2, y2, rx, ry, rx, ry + rh);
    int right = lineIntersect(x1, y1, x2, y2, rx, ry, rx + rw, ry + rh);

    if (left || right) {
        return 1;
    }

    return 0;
}

int checkBottomTopWallCollision(float x1, float y1, float x2, float y2, float rx, float ry, float rw, float rh) {
    int top = lineIntersect(x1, y1, x2, y2, rx, ry, rx + rw, ry);
    int bottom = lineIntersect(x1, y1, x2, y2, rx, ry + rh, rx + rw, ry + rh);

    if (top || bottom) {
        return 1;
    }

    return 0;
}

int wallCollisionY(int x, int y, int l, int h) {
    for (int i = 0; i < wallCount; i++) {
        if (checkBottomTopWallCollision(walls[i].x1, walls[i].y1, walls[i].x2, walls[i].y2, x, y, l, h)) {
            return 1;
        }
    }
    return 0;
}

int wallCollisionX(int x, int y, int l, int h) {
    for (int i = 0; i < wallCount; i++) {
        if (checkLeftRightWallCollision(walls[i].x1, walls[i].y1, walls[i].x2, walls[i].y2, x, y, l, h)) {
            return 1;
        }
    }
    return 0;
}

int wallBitmapCollision(float x1, float y1, float x2, float y2, float rx, float ry, float rw, float rh) {

    int sidesLR = checkLeftRightWallCollision(x1, y1, x2, y2, rx, ry, rw, rh);
    int sidesTB = checkBottomTopWallCollision(x1, y1, x2, y2, rx, ry, rw, rh);

    if (sidesLR || sidesTB) {
        return 1;
    }
    return 0;
}

int collisionWall(int x, int y, int l, int h) {
    for (int i = 0; i < wallCount; i++) {
        if (wallBitmapCollision(walls[i].x1, walls[i].y1, walls[i].x2, walls[i].y2, x, y, l, h)) {
            return 1;
        }
    }
    return 0;
}

int bitmapCollision(float r1x, float r1y, float r1w, float r1h, unsigned int *bitmap, float r2x, float r2y, float r2w, float r2h, unsigned int *bitmap2) {

    if (r1x + r1w >= r2x && r1x <= r2x + r2w && r1y + r1h >= r2y && r1y <= r2y + r2h) {
        int x1 = MAX(r1x, r2x);
        int x2 = MIN(r1x + r1w, r2x + r2w);
        int y1 = MAX(r1y, r2y);
        int y2 = MIN(r1y + r1h, r2y + r2h);
        for (int y = y1; y < y2; ++y)
        {
            for (int x = x1; x < x2; ++x)
            {
                int r1 = (BIT_VALUE(bitmap[(int) (y-r1y)], ((bitmapXMax-1) - ((int) round(x-r1x)))) > 0);
                int r2 = (BIT_VALUE(bitmap2[(int) (y-r2y)], ((bitmapXMax-1) - ((int) round(x-r2x)))) > 0);
                if (r1 && r2)
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void updateWall(void) {
    if (gamePause) {
        return;
    }

    for (int i = 0; i < wallCount; i++) {
        double dx = (walls[i].x2-walls[i].x1);
        double dy = (walls[i].y2-walls[i].y1);
        double angle = atan2(dy, dx);
        double xSpeed = randRange(0, 0.25) * abs(wallScaleFactor);
        double ySpeed = randRange(0, 0.25) * abs(wallScaleFactor);

        if (fmod(angle, M_PI/2) != 0) {
            angle = atan2(dy, dx)-(M_PI/2);
        }

        double xDist = sin(angle) * xSpeed;
        double yDist = cos(angle) * ySpeed;

        if (wallScaleFactor < 0) {
            xDist = (sin(angle) * xSpeed) * -1;
            yDist = (cos(angle) * ySpeed) * -1;
        }


        if (!(walls[i].x1 + (walls[i].x2 - walls[i].x1) >= 0 && walls[i].x1 <= LCD_X &&  walls[i].y1 +  (walls[i].y2 - walls[i].y1) >= 0 && walls[i].y2 <= LCD_Y)){
            if (walls[i].x1+xDist <= 0 && walls[i].x2+xDist <= 0) {
                xDist = LCD_X-1;
            }

            if (walls[i].y1+yDist <= 0 && walls[i].y2+yDist <= 0) {
                yDist = LCD_X-1;
            }

            if (walls[i].x1+xDist >= LCD_X && walls[i].x2+xDist >= LCD_X) {
                xDist -= LCD_X;
            }

            if (walls[i].y1+yDist >= LCD_Y && walls[i].y2+yDist >= LCD_Y) {
                yDist = (yDist - LCD_Y) + dashboardBorder;
            }
        }

        if (wallBitmapCollision(walls[i].x1 + xDist, walls[i].y1 + yDist, walls[i].x2 + xDist, walls[i].y2 + yDist,
                                jerry.x, jerry.y, bitmapWidth(jerryBitmap) - 1, bitmapHeight(jerryBitmap) - 1) && jerry.super == 0) {
            jerry.health--;
            setupJerry();
        }

        if (wallBitmapCollision(walls[i].x1 + xDist, walls[i].y1 + yDist, walls[i].x2 + xDist, walls[i].y2 + yDist,
                                tom.x, tom.y, bitmapWidth(jerryBitmap) - 1, bitmapHeight(jerryBitmap) - 1)) {
            setupTom();
        }

        walls[i].x1 += xDist;
        walls[i].x2 += xDist;
        walls[i].y1 += yDist;
        walls[i].y2 += yDist;
    }
}

void updateTom(void) {
    int new_x = round(tom.x + (tom.xDir * tom.xSpeed));
    int new_y = round(tom.y + (tom.yDir * tom.ySpeed));
    double xDir = rand() * M_PI / RAND_MAX;
    double yDir = rand() * M_PI  / RAND_MAX;
    double xSpeed = randRange(0, 0.5) * charScaleFactor;
    double ySpeed = randRange(0, 0.5) * charScaleFactor;
    int xBounce = 0;
    int yBounce = 0;
    if (new_x <= 1 || new_x >= LCD_X-bitmapWidth(tomBitmap)-1 || wallCollisionX(new_x, new_y, bitmapWidth(tomBitmap)-1, bitmapHeight(tomBitmap)-1)) {
        tom.xDir = cos(xDir);
        tom.xSpeed = xSpeed*charScaleFactor;
        xBounce = 1;
    }
    if (new_y <= dashboardBorder || new_y >= LCD_Y-bitmapHeight(tomBitmap)-1 || wallCollisionY(new_x, new_y, bitmapWidth(tomBitmap)-1, bitmapHeight(tomBitmap)-1)) {
        tom.yDir = cos(yDir);
        tom.ySpeed = ySpeed*charScaleFactor;
        yBounce = 1;
    }
    if (xBounce && yBounce)
    {
        tom.xSpeed = randRange(0, 0.25) * charScaleFactor;
        tom.ySpeed = randRange(0, 0.25) * charScaleFactor;
    }
    if (!gamePause) {
        tom.x += tom.xDir * tom.xSpeed;
        tom.y += tom.yDir * tom.ySpeed;
    }
    if (bitmapCollision(tom.x, tom.y, bitmapWidth(tomBitmap) - 1, bitmapHeight(tomBitmap) - 1, tomBitmap, jerry.x, jerry.y,
                        bitmapWidth(jerryBitmap) - 1, bitmapHeight(jerryBitmap) - 1, jerryBitmap) && jerry.super == 0) {
        jerry.health--;
        setupCharacters();
    } else if (bitmapCollision(tom.x, tom.y, bitmapWidth(tomBitmap) - 1, bitmapHeight(tomBitmap) - 1, tomBitmap, jerry.x, jerry.y,
                               bitmapWidth(jerryBitmap) - 1, bitmapHeight(jerryBitmap) - 1, jerrySuperBitmap) && jerry.super > 0) {
        jerry.charScore++;
        setupTom();
    }
}

void updateFireworks(int approved) {
    if ((centerJoystickPressed && jerry.charSpecialScore >= 3) || (approved && jerry.charSpecialScore >= 3)) {
        float posOne = tom.x - jerry.x;
        float posTwo = tom.y - jerry.y;
        float divNum = sqrt((posOne*posOne) + (posTwo*posTwo));
        fireworks[fireworkCount].dx = posOne * 0.3/divNum;
        fireworks[fireworkCount].dy = posTwo * 0.3/divNum;
        fireworks[fireworkCount].x = jerry.x;
        fireworks[fireworkCount].y = jerry.y;
        fireworks[fireworkCount].draw = 1;
        fireworkCount++;
    }

    for (int i = 0; i < fireworkCount; i++) {
        fireworks[i].x = fireworks[i].x + (fireworks[i].dx*charScaleFactor);
        fireworks[i].y = fireworks[i].y + (fireworks[i].dy*charScaleFactor);
        if(collisionWall(fireworks[i].x, fireworks[i].y, 1, 1) || fireworks[i].y < dashboardBorder || fireworks[i].y > LCD_Y-1 || fireworks[i].x < 0 || fireworks[i].x > LCD_X-1)
        {
            fireworks[i].draw = 0;
            for(int j = i; j < fireworkCount-1; j++)
            {
                fireworks[j] = fireworks[j+1];
            }
            fireworkCount--;
            break;
        }
        if ((fireworks[i].x > tom.x && fireworks[i].y < tom.x+(bitmapWidth(tomBitmap)-1) && fireworks[i].y > tom.y && fireworks[i].y < tom.y+(bitmapHeight(tomBitmap)-1)))
        {
            setupTom();
            for(int j = i; j < fireworkCount-1; j++)
            {
                fireworks[j] = fireworks[j+1];
            }
            fireworkCount--;
            break;
        }
    }
}

void gameOverScreen(char *condition) {
    clear_screen();
    char *message[] = {
            "Jerry Chase",
            "GAME OVER",
            "Press RIGHT BTN",
            "To Begin"
    };

    drawCentred(0, message[0]);
    drawCentred((1*CHAR_HEIGHT), message[1]);
    if (strlen(condition) > 0) {
        drawCentred((2*CHAR_HEIGHT), condition);
    }
    drawCentred((3*CHAR_HEIGHT), message[2]);
    drawCentred((4*CHAR_HEIGHT), message[3]);

    show_screen();

    while(!rightButtonPressed){

    }

    if (rightButtonPressed) {
        resetEverything();
        level = 1;
        setupCharacters();
        setupTomJerry();
        jerry.charScore = 0;
        jerry.charSpecialScore = 0;
        gameTimerMinutes = 0;
        gameTimerSeconds = 0;
        levelLoaded = 0;
        gamePause = 0;

        clear_screen();
    }
}

void updateMilk(void) {
    if (!milkCalc && milkTimerSeconds == 5 && level == 2) {
        double locmilkX = tom.x;
        double locmilkY = tom.y;

        // TODO: dont overlap Cheese or Traps
        milkX = locmilkX;
        milkY = locmilkY;

        milkCalc = 1;
    }

    if (milkCalc) {
        if (bitmapCollision(jerry.x, jerry.y, bitmapWidth(jerryBitmap) - 1, bitmapHeight(jerryBitmap) - 1, jerryBitmap, milkX, milkY,
                            bitmapWidth(milkBitmap) - 1, bitmapWidth(milkBitmap)-1, milkBitmap)) {

            jerry.super = -1;

            milkTimerSeconds = 0;
            milkCalc = 0;
        }
    }

    if (jerry.super) {
        if(LEDBrightnessDirection & 0b00000010) LED0Brightness += 17; 		else LED0Brightness -= 17;
        if(LEDBrightnessDirection & 0b00000100) LED1Brightness += 17; 		else LED1Brightness -= 17;
        if(LED0Brightness > 254) LEDBrightnessDirection &= ~0b00000010;
        else if(LED0Brightness < 1) LEDBrightnessDirection |= 0b00000010;
        if(LED1Brightness > 254) LEDBrightnessDirection &= ~0b00000100;
        else if(LED1Brightness < 1) LEDBrightnessDirection |= 0b00000100;
    } else {
        LED0Brightness = 0;
        LED1Brightness = 0;
        LEDBrightnessDirection = 0xFF;
        LEDBrightnessChangeCount = 0;
    }

}

void updateDoor(void) {
    if (!doorCalc && jerry.charLevelScore >= 5) {
        double locdoorX = (rand() % ((LCD_X-3)-3)) + 3;
        double locdoorY = (rand() % ((LCD_Y-3)-13)) + 13;

        while (collisionWall(locdoorX, locdoorY, bitmapWidth(doorBitmap)-1, bitmapHeight(doorBitmap)-1))
        {
            locdoorX = (rand() % ((LCD_X-3)-3)) + 3;
            locdoorY = (rand() % ((LCD_Y-3)-13)) + 13;
        }

        // TODO: dont overlap Cheese or Traps

        doorX = locdoorX;
        doorY = locdoorY;

        doorCalc = 1;
    }

    if (doorCalc && jerry.charLevelScore >= 5) {
        if (bitmapCollision(jerry.x, jerry.y, bitmapWidth(jerryBitmap) - 1, bitmapHeight(jerryBitmap) - 1, jerryBitmap, doorX, doorY,
                            bitmapWidth(doorBitmap) - 1, bitmapHeight(doorBitmap) - 1, doorBitmap)) {
            level++;
            levelLoaded = 0;
        }
    }
}

void updateTrap(void) {
    if (trapTimerSeconds == 3)
    {
        if (trapCount < TRAP_MAX)
        {
            items_spec trap;
            trap.x = tom.x;
            trap.y = tom.y;

            // TODO: dont overlap Cheese or Traps or Milk or Door

            traps[trapCount] = trap;
            trapCount++;
        }

        trapTimerSeconds = 0;
    }

    for (int i = 0; i < trapCount; i++) {
        if (bitmapCollision(jerry.x, jerry.y, bitmapWidth(jerryBitmap) - 1, bitmapHeight(jerryBitmap) - 1, jerryBitmap, traps[i].x,
                            traps[i].y, bitmapWidth(trapBitmap) - 1, bitmapHeight(trapBitmap) - 1, trapBitmap) && jerry.super == 0) {
            jerry.health--;

            for(int j = i; j < trapCount-1; j++)
            {
                traps[j] = traps[j+1];
            }

            trapCount--;
            break;
        }
    }
}

void updateCheese(void) {
    if (cheeseTimerSeconds == 2)
    {
        if (cheeseCount < CHEESE_MAX)
        {
            items_spec cheese;
            cheese.x = (rand() % ((LCD_X-3)-3)) + 3;
            cheese.y = (rand() % ((LCD_Y-3)-13)) + 13;
            while (collisionWall(cheese.x, cheese.y, bitmapWidth(cheeseBitmap)-1, bitmapHeight(cheeseBitmap)-1))
            {
                cheese.x = (rand() % ((LCD_X-3)-3)) + 3;
                cheese.y = (rand() % ((LCD_Y-3)-13)) + 13;
            }

            // TODO: dont overlap Cheese or Traps or Milk or Door

            cheeses[cheeseCount] = cheese;
            cheeseCount++;
        }
        cheeseTimerSeconds = 0;
    }

    for (int i = 0; i < cheeseCount; i++) {
        if (bitmapCollision(jerry.x, jerry.y, bitmapWidth(jerryBitmap) - 1, bitmapHeight(jerryBitmap) - 1, jerryBitmap, cheeses[i].x,
                            cheeses[i].y, bitmapWidth(cheeseBitmap) - 1, bitmapHeight(cheeseBitmap) - 1, cheeseBitmap)) {
            jerry.charScore++;
            jerry.charLevelScore++;
            jerry.charSpecialScore++;
            for(int j = i; j < cheeseCount-1; j++)
            {
                cheeses[j] = cheeses[j+1];
            }
            cheeseCount--;
            break;
        }
    }
}

void drawDashboard(void) {
    char dashboard[40];
    sprintf(dashboard, "L:%d H:%d S:%d %02d:%02d", level, jerry.health, jerry.charScore, gameTimerMinutes, gameTimerSeconds);
    draw_string(0, 0, dashboard, FG_COLOUR);
    draw_line(0,dashboardBorder,LCD_X,8,FG_COLOUR);
}

void drawTomJerry() {
    if (jerry.super > 0) {
        drawSuperjerry(jerry.x, jerry.y, jerrySuperBitmap);
    } else {
        drawBitmap(jerry.x, jerry.y, jerryBitmap);
    }

    if (!collisionWall(tom.x, tom.y, bitmapWidth(tomBitmap)-1, bitmapHeight(tomBitmap)-1)) {
        drawBitmap(tom.x, tom.y, tomBitmap);
    }
}

void setupJerry(void) {
    jerry.x = jerry.xStart;
    jerry.y = jerry.yStart;

    jerry.super = 0;
}

void setupTom(void) {
    double xDir = rand() * M_PI * 2 / RAND_MAX;
    double yDir = rand() * M_PI * 2 / RAND_MAX;
    double xSpeed = randRange(0, 0.5);
    double ySpeed = randRange(0, 0.5);
    tom.x = tom.xStart;
    tom.y = tom.yStart;
    tom.xDir = cos(xDir);
    tom.yDir = sin(yDir);
    tom.xSpeed = xSpeed;
    tom.ySpeed = ySpeed;
}

void setupCharacters(void) {
    setupTom();
    setupJerry();
}

void serialLevel(void) {
    char inputBuffer[100];
    int wallx0, wally0, wallx1, wally1 = 0;
    int tomX, tomY, jerryX, jerryY = 0;

    int c = usb_serial_getchar() ;
    while ((c < 0 || timeOutCounter < 30)) {
        char timerBuffer[20];
        timeOut = 1;
        clear_screen();
        drawCentred(1, "SEND FILE");
        sprintf(timerBuffer, "Time Left: %d", 30-timeOutCounter);
        drawCentred(CHAR_HEIGHT, timerBuffer);
        show_screen();
        c = usb_serial_getchar();

        if (c > 0 || timeOutCounter > 30) {
            timeOut = 0;
            timeOutCounter = 0;
            break;
        }
    }

    while (c > 0) {
        if (c == 'T') {
            usbSerialReadString(inputBuffer);
            sscanf(inputBuffer, "%d %d", &tomX, &tomY);
            tom.xStart = (double) tomX;
            tom.yStart = (double) tomY;
        } else if (c == 'J') {
            usbSerialReadString(inputBuffer);
            sscanf(inputBuffer, "%d %d", &jerryX, &jerryY);
            jerry.xStart = (double) jerryX;
            jerry.yStart = (double) jerryY;
        } else if (c == 'W') {
            usbSerialReadString(inputBuffer);
            sscanf(inputBuffer, "%d %d %d %d", &wallx0, &wally0, &wallx1, &wally1);
            if (wallx0 < 0) wallx0 = 0;
            if (wally0 < dashboardBorder) wally0 = dashboardBorder;
            if (wallx1 > LCD_X) wallx1 = LCD_X;
            if (wally1 > LCD_Y) wally1 = LCD_Y;

            if (wallx0 > LCD_X) wallx0 = LCD_X;
            if (wally0 > LCD_Y) wally0 = LCD_Y;
            if (wallx1 < 0) wallx1 = 0;
            if (wally1 < dashboardBorder) wally1 = dashboardBorder;

            walls[wallCount].x1 = wallx0;
            walls[wallCount].y1 = wally0;
            walls[wallCount].x2 = wallx1;
            walls[wallCount].y2 = wally1;
            wallCount++;
        }
        c = usb_serial_getchar();
    }
    maxLevel++;
}

void levelOne(void) {
    wall_spec wall0;
    wall0.x1 = 18.0; wall0.y1 = 15.0; wall0.x2 = 13.0; wall0.y2 = 25.0;
    walls[0] = wall0;

    wall_spec wall1;
    wall1.x1 = 25.0; wall1.y1 = 35.0; wall1.x2 = 25.0; wall1.y2 = 45.0;
    walls[1] = wall1;

    wall_spec wall2;
    wall2.x1 = 45.0; wall2.y1 = 10.0; wall2.x2 = 60.0; wall2.y2 = 10.0;
    walls[2] = wall2;

    wall_spec wall3;
    wall3.x1 = 58.0; wall3.y1 = 25.0; wall3.x2 = 72.0; wall3.y2 = 30.0;
    walls[3] = wall3;
    wallCount = 4;

    tom.xStart = LCD_X-5;
    tom.yStart = LCD_Y-9;

    jerry.xStart = 0;
    jerry.yStart = dashboardBorder+1;
}

void drawFireworks(void) {
    for (int i = 0; i < fireworkCount; i++)
    {
        if (fireworks[i].draw)
        {
            draw_pixel(fireworks[i].x, fireworks[i].y, FG_COLOUR);
        }
    }
}

void drawMilk(void) {
    if (milkCalc) {
        drawBitmap(milkX, milkY, milkBitmap);
    }
}

void drawDoor(void) {
    if (doorCalc) {
        drawBitmap(doorX, doorY, doorBitmap);
    }
}

void drawCheese(void ){
    for (int i = 0; i < cheeseCount; i++)
    {
        drawBitmap(cheeses[i].x, cheeses[i].y, cheeseBitmap);
    }
}

void drawTrap(void) {
    for (int i = 0; i < trapCount; i++)
    {
        drawBitmap(traps[i].x, traps[i].y, trapBitmap);
    }
}

void drawWalls(void)
{
    for (int i = 0; i < wallCount; i++)
    {
        draw_line(
                walls[i].x1,
                walls[i].y1,
                walls[i].x2,
                walls[i].y2,
                FG_COLOUR
        );
    }

}

void resetEverything(void) {
    jerry.charLevelScore = 0;
    tom.charLevelScore = 0;
    gamePause = 0;

    cheeseCount = 0;
    trapCount = 0;
    fireworkCount = 0;
    for (int i = 0; i < CHEESE_MAX; i++)
    {
        cheeses[i].x = -100;
        cheeses[i].y = -100;
    }
    for (int i = 0; i < TRAP_MAX; i++)
    {
        traps[i].x = -100;
        traps[i].y = -100;
    }
    for (int i = 0; i < FIREWORKS_MAX; i++)
    {
        fireworks[i].x = -100;
        fireworks[i].y = -100;
    }

    // Reset door data
    doorX = -100;
    doorY = -100;
    doorCalc = 0;

    milkX = -100;
    milkY = -100;
    milkCalc = 0;

    cheeseTimerSeconds = 0;
    trapTimerSeconds = 0;
    milkTimerSeconds = 0;
}

void drawAll(void) {
    clear_screen();

    if (levelLoaded)
    {
        drawWalls();
    }
    else
    {
        for (int i = 0; i < wallCount; i++)
        {
            walls[i].x1 = 0;
            walls[i].y1 = 0;
            walls[i].x2 = 0;
            walls[i].y2 = 0;
        }
        wallCount = 0;

        if (level == 1) {
            levelOne();
            levelLoaded = 1;
            resetEverything();
            setupCharacters();
        } else if (level == 2) {
            serialLevel();
            if (wallCount == 0) {
                gameOverScreen("ERROR");
            } else {
                levelLoaded = 1;
                resetEverything();
                setupCharacters();
            }
        } else {
            gameOverScreen("WIN");
        }
    }

    drawMilk();
    drawFireworks();
    drawTomJerry();
    drawCheese();
    drawTrap();
    drawDoor();
    drawDashboard();

    show_screen();
}

void updateJerry(int serialIn) {
    if (leftJoystickPressed || serialIn == 1) {
        if (!collisionWall(jerry.x-(1*charScaleFactor), jerry.y, bitmapWidth(jerryBitmap)-1, bitmapHeight(jerryBitmap)-1) && jerry.x-(1*charScaleFactor) > 0 && jerry.super == 0) {
            jerry.x -= 1*charScaleFactor;
        } else if (jerry.x-(1*charScaleFactor) > 0 && jerry.super > 0) {
            jerry.x -= 1*charScaleFactor;
        }
    }

    if (rightJoystickPressed || serialIn == 2) {
        if (!collisionWall(jerry.x+(1*charScaleFactor), jerry.y, bitmapWidth(jerryBitmap)-1, bitmapHeight(jerryBitmap)-1) && jerry.x+(1*charScaleFactor) < LCD_X-(bitmapWidth(jerryBitmap)-1) && jerry.super == 0) {
            jerry.x += 1*charScaleFactor;
        } else if (jerry.x+(1*charScaleFactor) < LCD_X-bitmapWidth(jerrySuperBitmap) && jerry.super > 0) {
            jerry.x += 1*charScaleFactor;
        }
    }

    if (upJoystickPressed || serialIn == 3) {
        if (!collisionWall(jerry.x, jerry.y-(1*charScaleFactor), bitmapWidth(jerryBitmap)-1, bitmapHeight(jerryBitmap)-1) && jerry.y-(1*charScaleFactor) > dashboardBorder && jerry.super == 0) {
            jerry.y -= 1*charScaleFactor;
        } else if (jerry.y-(1*charScaleFactor) > dashboardBorder && jerry.super > 0) {
            jerry.y -= 1*charScaleFactor;
        }
    }

    if (downJoystickPressed|| serialIn == 4) {
        if (!collisionWall(jerry.x, jerry.y+(1*charScaleFactor), bitmapWidth(jerryBitmap)-1, bitmapHeight(jerryBitmap)-1) && jerry.y+(1*charScaleFactor) < LCD_Y-(bitmapWidth(jerryBitmap)-1) && jerry.super == 0) { jerry.y += 1*charScaleFactor;
        } else if (jerry.y+(1*charScaleFactor) < LCD_Y-(bitmapWidth(jerrySuperBitmap)-1) && jerry.super > 0) {
            jerry.y += 1*charScaleFactor;
        }
    }

    if (jerry.health == 0) {
        gameOverScreen("DIED");
    }

}

void updateScaling(void) {
    int characterADC = adc_read(0);
    int wallADC = adc_read(1);

    charScaleFactor = characterADC / 256;
    wallScaleFactor = wallADC == 512 ? 1 : (wallADC/128)-4;
}

void process(void) {
    if (rightButtonPressed) {
        gamePause = !gamePause;
    }

    if (leftButtonPressed && level <= maxLevel) {
        level++;
        levelLoaded = 0;
    } else if (leftButtonPressed && level > maxLevel) {
        gameOverScreen("WIN");
    }


    int16_t c = usb_serial_getchar();
    if (c >= 0 && level == 2) {
        if (c == 'w') {
            updateJerry(3);
        } else if (c == 'a') {
            updateJerry(1);
        } else if (c == 's') {
            updateJerry(4);
        } else if (c == 'd') {
            updateJerry(2);
        } else if (c == 'p') {
            gamePause = !gamePause;
        } else if (c == 'f') {
            updateFireworks(1);
        } else if (c == 'l' && level > maxLevel) {
            gameOverScreen("WIN");
        } else if (c == 'l' && level <= maxLevel) {
            level++;
            levelLoaded = 0;
        } else if (c == 'i') {
            char buffer[20];
            sprintf(buffer, "Timestamp: %02d:%02d \r\n", gameTimerMinutes, gameTimerSeconds);
            usbSerialStringSend(buffer);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "Level: %d \r\n", level);
            usbSerialStringSend(buffer);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "Jerry's Lives: %d \r\n", jerry.health);
            usbSerialStringSend(buffer);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "Score: %d \r\n", jerry.charScore);
            usbSerialStringSend(buffer);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "Fireworks: %d \r\n", fireworkCount+1);
            usbSerialStringSend(buffer);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "Traps: %d \r\n", trapCount);
            usbSerialStringSend(buffer);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "Cheese: %d \r\n", cheeseCount);
            usbSerialStringSend(buffer);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "Cheese Consumed: %d \r\n", jerry.charLevelScore);
            usbSerialStringSend(buffer);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "Super Jerry: %s \r\n", jerry.super ? "True" : "False");
            usbSerialStringSend(buffer);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "Paused: %s \r\n", gamePause ? "True" : "False");
            usbSerialStringSend(buffer);
            memset(buffer, 0, sizeof(buffer));
        }
    }


    updateScaling();
    updateWall();
    updateCheese();
    updateTrap();
    updateDoor();
    updateFireworks(0);
    updateMilk();
    updateJerry(0);
    updateTom();
}

void welcome(void) {
    char *message[] = {
            "Jerry Chase",
            "Perdana Bailey",
            "n10245065",
            "Press RIGHT BTN",
            "To Begin"
    };

    drawCentred((1*CHAR_HEIGHT-3), message[0]);
    drawCentred((2*CHAR_HEIGHT-3), message[1]);
    drawCentred((3*CHAR_HEIGHT-3), message[2]);
    drawCentred((4*CHAR_HEIGHT-3), message[3]);
    drawCentred((5*CHAR_HEIGHT-3), message[4]);

    show_screen();

    while(!rightButtonPressed){

    }
    lcd_clear();
    clear_screen();
    gamePause = 0;
}

void setupTomJerry(void) {
    jerry.health = JERRY_MAX_HEALTH;
    tom.health = TOM_MAX_HEALTH;
}

void setupTimer1(void) {
    cli();

    CLEAR_BIT(TCCR1A,WGM10);
    CLEAR_BIT(TCCR1A,WGM11);
    CLEAR_BIT(TCCR1B,WGM12);
    CLEAR_BIT(TCCR1B,WGM13);
    SET_BIT(TCCR1B,CS00);

    SET_BIT(TIMSK1, TOIE1);

    sei();
}

ISR(TIMER1_OVF_vect) {
    uint8_t mask = 0b00001111;
    leftButtonCounter = ((leftButtonCounter<<1) & mask) | BIT_VALUE(PINF,6);
    rightButtonCounter = ((rightButtonCounter<<1) & mask) | BIT_VALUE(PINF,5);
    leftJoystickCounter = ((leftJoystickCounter<<1) & mask) | BIT_VALUE(PINB,1);
    rightJoystickCounter = ((rightJoystickCounter<<1) & mask) | BIT_VALUE(PIND,0);
    upJoystickCounter = ((upJoystickCounter<<1) & mask) | BIT_VALUE(PIND,1);
    downJoystickCounter = ((downJoystickCounter<<1) & mask) | BIT_VALUE(PINB,7);
    centerJoystickCounter = ((centerJoystickCounter<<1) & mask) | BIT_VALUE(PINB,0);

    if (leftButtonCounter == mask)
    {
        leftButtonPressed = 1;
    }
    if (leftButtonCounter == 0)
    {
        leftButtonPressed = 0;
    }

    if (rightButtonCounter == mask)
    {
        rightButtonPressed = 1;
    }
    if (rightButtonCounter == 0)
    {
        rightButtonPressed = 0;
    }

    if (leftJoystickCounter == mask)
    {
        leftJoystickPressed = 1;
    }
    if (leftJoystickCounter == 0)
    {
        leftJoystickPressed = 0;
    }

    if (rightJoystickCounter == mask)
    {
        rightJoystickPressed = 1;
    }
    if (rightJoystickCounter == 0)
    {
        rightJoystickPressed = 0;
    }

    if (downJoystickCounter == mask)
    {
        downJoystickPressed = 1;
    }
    if (downJoystickCounter == 0)
    {
        downJoystickPressed = 0;
    }

    if (upJoystickCounter == mask)
    {
        upJoystickPressed = 1;
    }
    if (upJoystickCounter == 0)
    {
        upJoystickPressed = 0;
    }

    if (centerJoystickCounter == mask)
    {
        centerJoystickPressed = 1;
    }
    if (centerJoystickCounter == 0)
    {
        centerJoystickPressed = 0;
    }

    gameTimerCounter++;
    double time = ((gameTimerCounter * 256.0 + TCNT0 ) * 256.0 / 8000000);
    if (time >= 1) {
        if (!gamePause)
        {
            gameTimer++;
            if (gameTimer == 1)
            {
                gameTimerSeconds++;
                cheeseTimerSeconds++;
                trapTimerSeconds++;
                milkTimerSeconds++;

                if(gameTimerSeconds >= 60)
                {
                    gameTimerMinutes++;
                    gameTimerSeconds = 0;
                }

                gameTimer = 0;
            }
        }

        if (timeOut) {
            timeOutCounter++;
        }

        if (jerry.super == -1) {
            jerry.super = 1;
        } else if (jerry.super > 0) {
            jerry.super++;
        }

        if (jerry.super > 10) {
            jerry.super = 0;
        }
        gameTimerCounter = 0;
    }
}

void setupTimer0(void) {
    cli();
    CLEAR_BIT(TCCR0B,WGM02);
    CLEAR_BIT(TCCR0B, CS02);
    CLEAR_BIT(TCCR0B,CS01);
    SET_BIT(TCCR0B,CS00);
    SET_BIT(TIMSK0, TOIE0);
    sei();
}

ISR(TIMER0_OVF_vect) {

    if (jerry.super) {
        if(LEDBrightnessChangeCount < LED0Brightness) PORTB |= (1 << LED0PIN);
        else PORTB &= ~(1 << LED0PIN);

        if(LEDBrightnessChangeCount < LED1Brightness) PORTB |= (1 << LED1PIN);
        else PORTB &= ~(1 << LED1PIN);
        LEDBrightnessChangeCount++;
    } else {
        CLEAR_BIT(PORTB, 2);
        CLEAR_BIT(PORTB,3);
        LEDBrightnessChangeCount = 0;
    }
}

void setupLED(void) {
    SET_BIT(DDRB, 2);
    SET_BIT(DDRB, 3);
    SET_BIT(DDRC, 7);

    SET_BIT(PORTC, 7);
}

void setupLCD(void) {
    lcd_init(LCD_DEFAULT_CONTRAST);
    lcd_clear();
}

void setupControls(void) {
    CLEAR_BIT(DDRB, 0); // center
    CLEAR_BIT(DDRD, 1); // up
    CLEAR_BIT(DDRB, 7); // down
    CLEAR_BIT(DDRB, 1); // left
    CLEAR_BIT(DDRD, 0); // right
    CLEAR_BIT(DDRF, 6); // sw2 (left)
    CLEAR_BIT(DDRF, 5); // sw3 (right)
}

void setupUSB(void) {
    usb_init();

    while (!usb_configured()) {
        clear_screen();
        drawCentred(2, "SERIAL NOT CONNECTED");
        show_screen();
    }

    clear_screen();
}

void setup(void) {
    set_clock_speed(CPU_8MHz);
    setupControls();
    setupUSB();
    setupLCD();
    setupLED();
    setupTimer0();
    setupTimer1();
    srand(gameTimerCounter);
    setupTomJerry();
    adc_init();

    welcome();
}

int main(void) {
    setup();
    for ( ;; ) {
        drawAll();
        process();
    }
}