/*
 *  This example creates whatever it can make and moves around randomly.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "overrun.hh"

struct unit {
   int ownerID;
   int unitID;
   int x;
   int y;
   int level;
};

struct unitOnSquare {
   int ownerID;
   int unitID;
   int level; // A level 0 unit means one does not exist on that square.
};

int amountOfPlayers;
int size;

int playersMinerals[MAX_PLAYERS];
int terrain[MAX_SIZE][MAX_SIZE];

struct unitOnSquare whatOnSquare[MAX_SIZE][MAX_SIZE];

int myID;

// Number of units in the game. We reset this after every turn
int numUnits;
struct unit allUnits[MAX_SIZE * MAX_SIZE];


int dx[] = {0, 1, 0, -1};
int dy[] = {-1, 0, 1, 0};

// AI logic will mostly go here.
void clientDoTurn() {
   int i, j;
   for (i = 0; i < numUnits; i++) {
      // It is my unit
      if (allUnits[i].ownerID == myID) {
         // Move randomly
         move (allUnits[i].unitID, rand () % 5); 
      }
   }
   // Finally, toss a coin to decide to build a dude.
   if (rand () % 2 == 0) {
      // Build with some minerals we have
      build (rand () % playersMinerals[myID]);
   }

   // We now reset the data we have stored to get the new data next round.
   for (i = 0; i < size; i++) {
      for (j = 0; j < size; j++) {
         whatOnSquare[i][j].level = 0; // We erase eveything set on squares
      }
   }
   numUnits = 0; // This will erase all the units we stored in allUnits.
}



void clientRegister() {
   // We need to call setName ()
   setName ("Randombot");

   // We also want to seed the random generator
   srand (time (NULL));
}

// All of these function simply get data into our AI
void clientInit(int playerCount, int boardSize, int playerID) {
   amountOfPlayers = playerCount;
   size = boardSize;
   myID = playerID;
   numUnits = 0;
}

void clientJuiceInfo(int pid, int juiceCount) {
   playersMinerals[pid] = juiceCount;
}

void clientTerrainInfo(int x, int y, int type) {
   terrain[y][x] = type;
}

void clientStudentLocation(int pid, int id, int x, int y, int level) {
   whatOnSquare[y][x].ownerID = pid;
   whatOnSquare[y][x].unitID = id;
   whatOnSquare[y][x].level = level;
   allUnits[numUnits].ownerID = pid;
   allUnits[numUnits].unitID = id;
   allUnits[numUnits].x = x;
   allUnits[numUnits].y = y;
   numUnits++;
}

