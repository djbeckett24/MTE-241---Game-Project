#include <lpc17xx.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "GLCD.h"
#include <RTL.h>
#include <math.h>
#include "queue.h"

//Define Pi
#ifndef M_PI
    #define M_PI acos(-1)
#endif

// Declare width of LCD
#ifndef WIDTH
    #define WIDTH 320
#endif

//Declare Height of LCD
#ifndef HEIGHT
    #define HEIGHT 240
#endif

#define DEADPOINTLOC 100000

// enum for the game states
typedef enum {
	GameOverScreen = 0,
	GameScreen = 1
}State;

// declare struc for a point
typedef struct {
	int32_t x;
	int32_t y;
} point_t;

//declare enemy struct
typedef struct {
	double dx;
	double dy;
	point_t point;
	point_t prevPoint;
} enemy_t;

//declare bullet struct
typedef struct {
	double dx;
	double dy;
	point_t point;
	point_t prevPoint;
} bullet_t;

void initialize(void);
void initLED(void);
void initPotentiometer(void);
void initPlayerPoints(void);
void initEnemyPoints(void);
void initBulletPoints(void);

void drawEnemy(point_t point, int draw);
void moveEnemy(int i);

void drawBullet(point_t point, int draw);
void moveBullet(int i);

void drawPlayer(int draw);
void fireBullet(void);
void printLED(int lives, int kills);

int checkPlayerCollision(int index);

double getPlayerAngle(void);

__task void start_tasks(void);
__task void PlayerTask(void);
__task void EnemyTask(void);
__task void BulletTask(void);
__task void RenderTask(void);

// delcare global variables for enemies
node_t *enemyIndexHead = NULL;
enemy_t enemies[20];
int enemyCount = 0;
int liveEnemies = 0;
int enemyPointCount = 0;
point_t enemyPoints[28];

//declare global variables for bullets
node_t *bulletIndexHead = NULL;
bullet_t bullets[20];
int bulletCount = 0;
int liveBullets = 0;
int bulletPointCount = 0;
point_t bulletPoints[5];

//declare global variables for players
double prev_player_angle = 0;
double player_angle = 0;
point_t playerPoints[65];
int playerPointCount = 0;

int kills = 0;
int lives = 3;
int difficulty = 1;

State gameState = GameScreen;

//declare semaphores
OS_SEM renderPlayerLock;
OS_SEM renderEnemyLock;
OS_SEM renderBulletLock;
OS_SEM playerLock;
OS_SEM enemyLock;
OS_SEM bulletLock;

int main(void){
	//run all one time initialization code
	initialize();	
	printf("\nStart\n");
	os_sys_init(start_tasks);
}

void initialize(void){
	uint32_t seed = 2000000;
	//initialize peripheral devices
	initPotentiometer();
	initLED();
	//init the arrays defining the player points centered at (0,0)
	initPlayerPoints();
	initEnemyPoints();
	initBulletPoints();
	//init GLCD for start screen
	GLCD_Init();
	GLCD_Clear(Black);
	GLCD_SetBackColor(Black);
	GLCD_SetTextColor(White);
	GLCD_DisplayString(2, 5, 1, "SPACE NUTZ");
	GLCD_DisplayString(4, 5, 1, "Press Button");

	// Wait for button press to start game and also seed srand
	while((LPC_GPIO2->FIOPIN & (1 << 10))){
		seed+=123;
	}
	srand(seed);
	//reinitialize GLCD for the game
	GLCD_Init();
	GLCD_Clear(Black);
	//set the previous player angle
	prev_player_angle = getPlayerAngle();
}

void initLED(){
	LPC_GPIO2->FIODIR |= 0x0000007C;
	LPC_GPIO1->FIODIR |= 1 << 28;
	LPC_GPIO1->FIODIR |= 1 << 29;
	LPC_GPIO1->FIODIR |= 1 << 31;
}

void initPotentiometer(void) {
	LPC_SC->PCONP |= 1 << 12; // Enable Power
	
	LPC_PINCON->PINSEL1 &= ~(0x03 << 18); // clear bits 18 and 19
	LPC_PINCON->PINSEL1 |= (0x01 << 18); // set bit 18
	
	LPC_ADC->ADCR = (1 << 2) |     // select AD0.2 pin
									(4 << 8) |     // ADC clock is 25MHz/5
									(1 << 21);     // enable 
}

void initPlayerPoints(void){
	// create the array of points for drawing the player centered at (0,0)
	int i;
	playerPointCount = 0;
	for (i = -8; i <= 8;i++){
		playerPoints[playerPointCount].y = -5;
		playerPoints[playerPointCount].x = i;
		playerPointCount++;
	}
	for (i = 7; i <= 13;i++){
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = 1;
		playerPointCount++;
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = -1;
		playerPointCount++;
	}
	for (i = 5; i <= 7;i++){
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = 2;
		playerPointCount++;
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = -2;
		playerPointCount++;
	}
	for (i = 3; i <= 5;i++){
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = 3;
		playerPointCount++;
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = -3;
		playerPointCount++;
	}
	for (i = 1; i <= 3;i++){
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = 4;
		playerPointCount++;
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = -4;
		playerPointCount++;
	}
	for (i = -1; i <= 1;i++){
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = 5;
		playerPointCount++;
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = -5;
		playerPointCount++;
	}
	for (i = -3; i <= -1;i++){
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = 6;
		playerPointCount++;
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = -6;
		playerPointCount++;
	}
	for (i = -4; i <= -3;i++){
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = 7;
		playerPointCount++;
		playerPoints[playerPointCount].y = i;
		playerPoints[playerPointCount].x = -7;
		playerPointCount++;
	}
}

void initEnemyPoints(void){
	// create the array of points for drawing an enemy centered at (0,0)
	int i;
	enemyPointCount = 0;
	
	for (i = -2; i <= 2;i++){
		enemyPoints[enemyPointCount].y = i;
		enemyPoints[enemyPointCount].x = 5;
		enemyPointCount++;
		enemyPoints[enemyPointCount].y = i;
		enemyPoints[enemyPointCount].x = -5;
		enemyPointCount++;
		enemyPoints[enemyPointCount].y = 5;
		enemyPoints[enemyPointCount].x = i;
		enemyPointCount++;
		enemyPoints[enemyPointCount].y = -5;
		enemyPoints[enemyPointCount].x = i;
		enemyPointCount++;
	}
	enemyPoints[enemyPointCount].y = -3;
	enemyPoints[enemyPointCount].x = 4;
	enemyPointCount++;
	enemyPoints[enemyPointCount].y = 3;
	enemyPoints[enemyPointCount].x = 4;
	enemyPointCount++;
	enemyPoints[enemyPointCount].y = 3;
	enemyPoints[enemyPointCount].x = -4;
	enemyPointCount++;
	enemyPoints[enemyPointCount].y = -3;
	enemyPoints[enemyPointCount].x = -4;
	enemyPointCount++;
	enemyPoints[enemyPointCount].x = -3;
	enemyPoints[enemyPointCount].y = 4;
	enemyPointCount++;
	enemyPoints[enemyPointCount].x = 3;
	enemyPoints[enemyPointCount].y = 4;
	enemyPointCount++;
	enemyPoints[enemyPointCount].x = 3;
	enemyPoints[enemyPointCount].y = -4;
	enemyPointCount++;
	enemyPoints[enemyPointCount].x = -3;
	enemyPoints[enemyPointCount].y = -4;
	enemyPointCount++;
}

void initBulletPoints(void){
	// create the array of points for drawing a bullet centered at (0,0)
	bulletPointCount = 0;
	bulletPoints[bulletPointCount].y = 0;
	bulletPoints[bulletPointCount].x = 1;
	bulletPointCount++;
	bulletPoints[bulletPointCount].y = 0;
	bulletPoints[bulletPointCount].x = -1;
	bulletPointCount++;
	bulletPoints[bulletPointCount].y = -1;
	bulletPoints[bulletPointCount].x = 0;
	bulletPointCount++;
	bulletPoints[bulletPointCount].y = 1;
	bulletPoints[bulletPointCount].x = 0;
	bulletPointCount++;
	bulletPoints[bulletPointCount].y = 0;
	bulletPoints[bulletPointCount].x = 0;
	bulletPointCount++;
}

void drawEnemy(point_t point, int draw){
	// function for drawing a single enemy
	// accepts a point and whether to draw or not
	int xR;
	int yR;
	int i;
	
	for(i = 0; i < enemyPointCount; i++){
		// for each point in the enemy point array, shift it by the location passed in
		// if draw is 1, it draws in red, if draw is 0, it draws in black, therefore erasing it
		xR = enemyPoints[i].x + point.x;
		yR = enemyPoints[i].y + point.y;
		if(xR >= 0 && yR >= 0 && xR <= WIDTH && yR <= HEIGHT){
			GLCD_SetTextColor(draw == 1 ? Red : Black);
			GLCD_PutPixel(xR, yR);
		}
	}	
}

void moveEnemy(int i) {
	// function for moving a single enemy, it accepts the index of the enemy in the enemy array
	if (enemies[i].point.x == DEADPOINTLOC || enemies[i].point.y == DEADPOINTLOC) {
		//if the enemy is at the dead point the enemy does not get moved
		return;
	}
	//update the movement distance ratio per tick based on the trig relationship to have the enemy converge to the player
	enemies[i].dy = (enemies[i].point.y - 10)/sqrt((enemies[i].point.y - 10)*(enemies[i].point.y - 10)+(enemies[i].point.x - 160)*(enemies[i].point.x - 160));
	enemies[i].dx  = (enemies[i].point.x - 160)/sqrt((enemies[i].point.y - 10)*(enemies[i].point.y - 10)+(enemies[i].point.x - 160)*(enemies[i].point.x - 160));	
	//set previous point to current point
	enemies[i].prevPoint.x = enemies[i].point.x;
	enemies[i].prevPoint.y = enemies[i].point.y;
	//update current point based on the difficulty and movement ratio
	enemies[i].point.x = enemies[i].point.x - difficulty * enemies[i].dx;
	enemies[i].point.y = enemies[i].point.y - difficulty * enemies[i].dy;
}

void drawBullet(point_t point, int draw){
	//function for drawing a single bullet
	// if draw is 1, it draws in magenta, if draw is 0, it draws in black, therefore erasing it
	int xR;
	int yR;
	int i;
	
	for(i = 0; i < bulletPointCount; i++){
		xR = bulletPoints[i].x + point.x;
		yR = bulletPoints[i].y + point.y;
		if(xR >= 0 && yR >= 0 && xR <= WIDTH && yR <= HEIGHT){
			GLCD_SetTextColor(draw == 1 ? Magenta : Black);
			GLCD_PutPixel(xR,yR);
		}
	}	
}

void moveBullet(int i) {
	// function for moving a single bullet, it accepts the index of the bullet in the bullet array
	if (bullets[i].point.x == DEADPOINTLOC && bullets[i].point.y == DEADPOINTLOC) {
		//if the bullet is at the dead point the bullet does not get moved
		return;
	}
	//set previous point to current point
	bullets[i].prevPoint.x = bullets[i].point.x;
	bullets[i].prevPoint.y = bullets[i].point.y;
	//update the current bullet location for its moved point
	bullets[i].point.x = bullets[i].point.x + 7 * bullets[i].dx;
	bullets[i].point.y = bullets[i].point.y + 7 * bullets[i].dy;
	
	//if the bullet is off the screen, remove it from the array
	if (bullets[i].point.x > WIDTH || bullets[i].point.x < 0 || bullets[i].point.y > HEIGHT || bullets[i].point.y < 0) {
		//decrements live bullets
		liveBullets--;
		//enque the dead bullets index into the bullet queue so the next bullet that is made will use this index to conserve memory
		enqueue(&bulletIndexHead, i);
		//erase the dead bullet so it doesn't stay on the screen
		drawBullet(bullets[i].prevPoint, 0);
		//set the bullets point to the Dead point
		bullets[i].point.x = DEADPOINTLOC;
		bullets[i].point.y = DEADPOINTLOC;
	}
}

void drawPlayer(int draw){
	// draw player function that takes in whether to draw or not
	int i;
	double size = 10;
	double x0 = WIDTH/2;
	double y0 = size;
	double x;
	double y;
	double xR;
	double yR;
	//if draw is 1, it will draw at the player angle
	//if draw is 0, it will erase at the previous angle
	double angle = draw == 1 ? player_angle : prev_player_angle;
	
	for(i = 0; i < playerPointCount; i++){
		//for each point in the player point array, rotate the point about its zero value based on the angle
		x = playerPoints[i].x;
		y = playerPoints[i].y;
		//Rotated points
		xR = (x)*cos(angle) - (y)*sin(angle)+x0;
		yR = (y)*cos(angle) + (x)*sin(angle)+y0;
		if((int)xR >= 0 && (int)yR >= 0 && (int)xR <= WIDTH && (int)yR <= HEIGHT){
			GLCD_SetTextColor(draw == 1 ? Green : Black);
			GLCD_PutPixel(xR,yR);
		}
	}
	//set the previous player angle to be the player angle if it is drawn in the ne location
	if (draw == 1){
		prev_player_angle = angle;
	}
}

double getPlayerAngle(void){
	// function to return the player angle based on the potentiometer value
	int ADC_Value;
	double potValue;

	LPC_ADC->ADCR |= 1 << 24;
	while (LPC_ADC->ADGDR & 0x8000 == 0);
	ADC_Value = (LPC_ADC->ADGDR);
	
	potValue = (ADC_Value >> 4) & 0xFFF;
	return -(M_PI/180.0)*((potValue/13.0)-157.5);
}

void fireBullet() {
	// function to fire a bullet
	int index;
	double angle = player_angle;
	 // only allow a max of 15 bullets on the screen at a time
	if (liveBullets > 15) {
		return;
	}
	// if there is an available index in the queue, it will be used
	if ((index=dequeue(&bulletIndexHead)) > 0) {
	} 
	// otherwise a new index is created
	else {
		bulletCount++;
		index = bulletCount;
	}
	liveBullets++;
	
	//initalize the movement parameters, and the start point, then move the bullet
	bullets[index].point.x = WIDTH/2;
	bullets[index].point.y = 10;
	bullets[index].dy = cos(-angle);
	bullets[index].dx = sin(-angle);
	moveBullet(index);
}

int checkPlayerCollision(int index) {
	//function to check if a single enemy collided with the player, given the enemies index
	if (enemies[index].point.x > WIDTH/2 - 20 && enemies[index].point.x < WIDTH/2 + 20) {
		if (enemies[index].point.y > 10 - 10 && enemies[index].point.y < 10 + 10) {
			liveEnemies--;
			lives--;
			//add the dead enemies index to the waiting queue to be reused
			enqueue(&enemyIndexHead, index);
			enemies[index].point.x = DEADPOINTLOC;
			enemies[index].point.y = DEADPOINTLOC;
			return 1;
		}
	}
	return 0;
}

int checkBulletCollision(int index) {
	//function to check if a bullet (given an index) collides with any enemy
	int i;
	int hitbox = 7;
	for (i = 0; i < enemyCount; i++) {
		if (bullets[index].point.x > enemies[i].point.x - hitbox && bullets[index].point.x < enemies[i].point.x + hitbox) {
			if (bullets[index].point.y > enemies[i].point.y - hitbox && bullets[index].point.y < enemies[i].point.y + hitbox) {
				liveEnemies--;
				liveBullets--;
				kills++;
				
				//increase the difficulty every 5 kills
				if (kills % 5 == 0){
					difficulty ++;
				}
				
				//add the dead enemies index to the waiting queue to be reused
				enqueue(&enemyIndexHead, i);
				enemies[i].point.x = DEADPOINTLOC;
				enemies[i].point.y = DEADPOINTLOC;
				//add the dead bullets index to the waiting queue to be reused
				enqueue(&bulletIndexHead, index);
				bullets[index].point.x = DEADPOINTLOC;
				bullets[index].point.y = DEADPOINTLOC;
				
				return i;
			}
		}
	}
	return -1;
}

void generateEnemy(void) { 
	//function to generate an enemy at a random spot on the outside of the screen
	int32_t x;
	int32_t y;
	int index;
	
	//generate the enemy on a random side of a screen
	uint32_t side = rand() % 3 + 1;
	
	//if it is on either side of the screen,
	//the y value will generate randomly 
	//the x value will be either 0 or the width of the screen
	if(side == 1 || side == 3){
		y = (rand() % (HEIGHT-50)) + 50;
		x = side == 1 ? 0 : WIDTH;
	}
	//if it is on tghe opposite side of the screen,
	//the y value will be the height of the screen 
	//the x value will generate randomly along the width of the screen
	else {
		y = HEIGHT;
		x = rand() % WIDTH;
	}
	//if there is an index in the enemy index queue, it will be used
	if ((index=dequeue(&enemyIndexHead)) > 0) {
	}
	//otherwise a new index will be created
	else {
		enemyCount++;
		index = enemyCount;
	}
	//increment living enemies, and set its initial location
	liveEnemies++;
	enemies[index].point.x = x;
	enemies[index].point.y = y;
}

void printLED(int lives, int kills){
	//function to print the lives and kills to the LEDs
	int c;
	int k;
	
	LPC_GPIO2->FIOCLR |= 0x0000007C;
	LPC_GPIO1->FIOCLR |= 1 << 28;
	LPC_GPIO1->FIOCLR |= 1 << 29;
	LPC_GPIO1->FIOCLR |= 1 << 31;
	
	if(lives == 3){
		LPC_GPIO1->FIOSET |= 1 << 28;
		LPC_GPIO1->FIOSET |= 1 << 29;
		LPC_GPIO1->FIOSET |= 1 << 31;
	}
	else if(lives == 2){
		LPC_GPIO1->FIOSET |= 1 << 28;
		LPC_GPIO1->FIOSET |= 1 << 29;
	}
	else if(lives == 1){
		LPC_GPIO1->FIOSET |= 1 << 28;
	}
	
	for (c = 4; c >= 0; c--)
  {
    k = kills >> c;
 
    if (k & 1){
      if (c == 4){
				LPC_GPIO2->FIOSET |= 1 << 2;
			}
			else if (c == 3){
				LPC_GPIO2->FIOSET |= 1 << 3;
			}
			else if (c == 2){
				LPC_GPIO2->FIOSET |= 1 << 4;
			}
			else if (c == 1){
				LPC_GPIO2->FIOSET |= 1 << 5;
			}
			else if (c == 0){
				LPC_GPIO2->FIOSET |= 1 << 6;
			}
		}
  }
}

__task void start_tasks() {
	//Start all task and init semaphores to desired values
	os_sem_init (&renderPlayerLock, 0);
	os_sem_init (&renderEnemyLock, 0);
	os_sem_init (&renderBulletLock, 0);
	os_sem_init (&playerLock, 1);
	os_sem_init (&enemyLock, 1);
	os_sem_init (&bulletLock, 1);
	
	os_tsk_create(PlayerTask, 1);
	os_tsk_create(EnemyTask, 1);
	os_tsk_create(BulletTask, 1);
	os_tsk_create(RenderTask, 1);
	os_tsk_delete_self();
}

__task void PlayerTask(void){
	while(1){
		//if the game is in the game state, update the player angle based on the potentiometer value
		if (gameState == GameScreen){
			//wait for the semaphore from the render task
			os_sem_wait (&playerLock, 0xFFFF);
			player_angle = getPlayerAngle();
			//release the semaphore so the render task can run
			os_sem_send (&renderPlayerLock);
		}
		os_tsk_pass();
 	}
}

__task void EnemyTask(void){
	int i;
	int count;
	while(1){
		if (gameState == GameScreen){
			//wait for the semaphore from the render task
			os_sem_wait (&enemyLock, 0xFFFF);
			
			//dynamically generate enemies based on the number of kills
			//every other kill will cause the enmies to spawn faster
			if (count % (30 - (kills/3 < 40 ? kills/3 : 29))  == 0) {
				generateEnemy();
			}
			for (i = 0; i < enemyCount; i++) {
				//move each enemy on the board
				moveEnemy(i);
			}
			count++;
			//release the semaphore so the render task can run
			os_sem_send (&renderEnemyLock);
		}
		os_tsk_pass();
 	}
}

__task void BulletTask(void){
	int i;
	int buttonDown;
	int previous= 0;
	
	while(1){
		if (gameState == GameScreen){
			//wait for the semaphore from the render task
			os_sem_wait (&bulletLock, 0xFFFF);
			buttonDown = !(LPC_GPIO2->FIOPIN & (1 << 10));
			// if the button is pressed and released, fire a bullet
			if(buttonDown && buttonDown != previous) {
				fireBullet();
			}
			previous = buttonDown;
			//move each bullet on the screen
			for (i = 0; i < bulletCount; i++) {
				moveBullet(i);
			}
			//release the semaphore so the render task can run
			os_sem_send (&renderBulletLock);
		}
		os_tsk_pass();
 	}
}

__task void RenderTask(void){ 
	int i;
	int deletedEnemy = 0;
	
	// do all of the rendering and collision detection
	
	while(1){
		if (gameState == GameScreen){
			//wait for the player, bullet, and enemy tasks to finish updating
			os_sem_wait (&renderBulletLock, 0xFFFF);
			os_sem_wait (&renderEnemyLock, 0xFFFF);
			os_sem_wait (&renderPlayerLock, 0xFFFF);
			
			//erase the previous player location and draw the player in the new location
			drawPlayer(0);
			drawPlayer(1);
			
			for (i = 0; i < bulletCount; i++) {
				//if the bullet is dead, skip this iteration of the for loop
				if (bullets[i].point.x == DEADPOINTLOC && bullets[i].point.y == DEADPOINTLOC) {
					continue;
				}
				//erase the previous bullet
				drawBullet(bullets[i].prevPoint, 0);
				//check if the bullet killed an enemy
				deletedEnemy = checkBulletCollision(i);
				
				if (deletedEnemy == -1) {
					//draw the bullet if it didn't collide with an enemy
					drawBullet(bullets[i].point, 1);
				} else {
					//erase the enemy that was killed
					drawEnemy(enemies[deletedEnemy].prevPoint, 0);
				}
			}
			for (i = 0; i < enemyCount; i++) {
				//if the enemy is dead, skip this iteration of the for loop
				if (enemies[i].point.x == DEADPOINTLOC && enemies[i].point.y == DEADPOINTLOC) {
					continue;
				}
				//erase the previous location of the enemy
				drawEnemy(enemies[i].prevPoint, 0);
				//check if the enemy collided with the player, if not, draw the enemy in its new position
				if (checkPlayerCollision(i) != 1) {
					drawEnemy(enemies[i].point, 1);
				}
			}
			//print the lives and kills to the LED's
			printLED(lives, kills);

			//release the semphores so the bullet, player, and enemy tasks can run
			os_sem_send (&bulletLock);
			os_sem_send (&enemyLock);
			os_sem_send (&playerLock);
			
			//if you lose all your lives, go to the game over screen
			if(lives <= 0){
				char finalScore;
				sprintf(&finalScore, "Score: %d", kills);
				gameState = GameOverScreen;
				GLCD_Clear(Black);
				GLCD_SetBackColor(Black);
				GLCD_SetTextColor(White);
				GLCD_DisplayString(3, 5, 1, "Game Over");
				GLCD_DisplayString(4, 5, 1, &finalScore);
				GLCD_DisplayString(6, 5, 1, "Press Reset");
			}
		}
	}
}
