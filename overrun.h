#ifndef __PHAIS_H
#define __PHAIS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <math.h>
   /////////////////////////////////////////////////////////////////////
   // The following are constants for possible terrain components
#define WALL                (-99)
#define BLANK_CELL          0

   // The following are constants representing possible moves
#define NORTH               0
#define EAST                1
#define SOUTH               2  
#define WEST                3
#define EXTRACT             4

   // Limits of certain inputs
#define MAX_PLAYERS         90
#define MAX_SIZE            100
#define MAX_STUDENT_ID      100000
   /////////////////////////////////////////////////////////////////////


   /////////////////////////////////////////////////////////////////////
   // The following must be implemented by the client:

   /*
    *   This is called when your client connects to the server. You need to provide
    *   a name using setName.
    */

   void clientRegister();


   /*
    *   This is called when the game is about to begin. It tells you how many players
    *   there are in playerCount, and the size of the square board in boardSize, and
    *   your player ID. Your player ID corresponds to your unit's playerID, and the 
    *   negation of your ID is what your base ID is. So a person with a player ID of 2
    *   will have units with playerID 2, and their base will be -2.
    *   You are not required to call anything in here.
    */
   void clientInit(int playerCount, int boardSize, int playerID);

   /*
    *   This is called for each player currently in the game once before your turn.
    *   It tells you information about a player (pid), in particular how many minerals
    *   that player has (juiceCount).
    */
   void clientJuiceInfo(int pid, int juiceCount);

   /*
    *   This is called before you take your turn, telling you what a certain square (the one at x,y) is.
    *   There will be size * size calls to this function every single turn, and so even
    *   squares which have not been modified will be sent. The value of type represents
    *   the following:
    *
    *         0        = Blanks square
    *         (-99)    = Wall
    *         > 0      = Mineral patch with that many minerals (so a value of 10 means 10 minerals
    *                    in that patch)
    *         < 0      = Player number -n's base. If n == your ID, it is your base.
    *   The blank square and wall constants are found in this file, under BLANK_CELL and WALL
    */
   void clientTerrainInfo(int x, int y, int type);


   /*
    *   This is called for each unit currently in the game once before you take your turn.
    *   It tells you the player who owns it (pid), what the id of the unit is for referencing
    *   means (id is constant throughout the game), the current location in x and y coordinates
    *   and also its level.
    */
   void clientStudentLocation(int pid, int id, int x, int y, int level);

   /*
    *   This is called when it is time for you to take your turn. While in the function, you may call
    *   move and build throughout to tell the server what you want to do. When you exit the function, your
    *   move will actually be implemented.
    */
   void clientDoTurn();

   // The following are available to the client:

   /*
    *   This will send to the server what you want your name to be.
    */
   void setName(const char* name);

   /*
    *   Sends a move to the server. You specify the move by giving it the unit id (this is the same as
    *   clientStudentLocation's id, which is the only way you can get this) and what move to make. It will
    *   implement the moves IN ORDER that they are called, so the unit specified in the first call to move 
    *   will move first, the second unit will be moved second ect. The value for move is found in this
    *   file, in the constants NORTH, EAST, SOUTH, WEST, EXTRACT.
    */
   void move(int uid, int move);

   /*
    *   This will send to the server what you want to build that turn. If you don't call this, nothing will be
    *   built. You can assume that the server will build the highest possible unit for this cost.
    */
   void build(int cost);

   /*
    *   Given a level, it will tell you what the cost to build a unit is
    */
   int getCost (int level);

   /*
    *   Given a cost, this will tell you what level it will produce.
    */
   int getLevel (int cost);

   /////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif
