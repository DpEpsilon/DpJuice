#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>

#include "overrun.h"

#define FALSE 0
#define TRUE  1
#define BUFFER_SIZE 1024
#define TMP_SIZE 64

int terrain[MAX_SIZE+2][MAX_SIZE+2];

int minerals[MAX_PLAYERS+2];

struct student
{
    int pid, id, x, y, level;
} students[MAX_SIZE*MAX_SIZE+2];
int total_students;

int student_count[MAX_PLAYERS+2];

int playernum, boardsize, playerid;

char buffer[BUFFER_SIZE];
char out_buffer[BUFFER_SIZE];
char tmp[TMP_SIZE];

int name_set = FALSE;

ssize_t packet_size;

char ip_address[20];
int vis_output = FALSE;

int soc;

int cmd_line_args(int args, char * argv[]);
int connect_self();
int process_packet();

int main(int argc, char * argv[])
{
    strcpy(ip_address, "127.0.0.1");
    if (cmd_line_args(argc, argv) == 1) return 1;
    
    for (;;) // Reconnect loop
    {
	name_set = FALSE;
	if (connect_self() == 1) return 1;
	
	for (;;) // Packet processing loop
	{
	    int prc_ret;
	    memset((void *)buffer, 0, sizeof(char)*BUFFER_SIZE);
	    packet_size = recv(soc, (void *)buffer, BUFFER_SIZE, 0);
	    if (packet_size == -1)
	    {
		fprintf(stderr, "error: problem in receiving packet\n");
		fprintf(stderr, "Reconnecting...\n");
		break;
	    }
	    if (packet_size == 0)
	    {
		fprintf(stderr, "error: connection closed by host\n");
		fprintf(stderr, "Reconnecting...\n");
		break;
	    }
	    fprintf(stderr, "%s\n", buffer);
	    prc_ret = process_packet();
	    if (prc_ret == 1)
	    {
		fprintf(stderr, "Exiting...\n");
		return 1;
	    }
	    if (prc_ret == 2)
	    {
		fprintf(stderr, "Reconnecting...\n");
		break;
	    }
	}
    }
    return 0;
}


// Returns 2 if there is a need to reconnect, 1 if an fatal error occurs and 0 otherwise.
int process_packet()
{
    if (memcmp(buffer, "NAME PLEASE", 11) == 0) // NAME PLEASE
    {
	clientRegister(); // setName should be called within this
	
	if (!name_set)
	{
	    fprintf(stderr, "fatal: bot failed to set name\n");
	    return 1;
	}
	
	strcat(out_buffer, " -1 -1 -1\n");

	fprintf(stderr, "out_buffer: %s", out_buffer);
	
	if (send(soc, out_buffer, BUFFER_SIZE, 0) == -1)
	{
	    fprintf(stderr, "error: could not send data\n");
	    return 2;
	}
	return 0;
    }
    if (memcmp(buffer, "NEWGAME", 7) == 0) // NEWGAME
    {
	sscanf(buffer, "%*s %d %d %d %s", &playernum, &boardsize, &playerid, tmp); // %*s because I'm cool like that
	clientInit(playernum, boardsize, playerid);

	strcpy(out_buffer, "READY ");
	strncat(out_buffer, tmp, BUFFER_SIZE/4);
	strcat(out_buffer, "\n");


	fprintf(stderr, "%s\n", out_buffer);
	
	if (send(soc, out_buffer, BUFFER_SIZE, 0) == -1)
	{
	    fprintf(stderr, "error: could not send data\n");
	    return 2;
	}
	return 0;
    }
    if (memcmp(buffer, "GAMEOVER", 8) == 0) // GAMEOVER
    {
	fprintf(stderr, "%s\n", buffer);
	return 0;
    }
    if (memcmp(buffer, "CELL", 4) == 0) // CELL
    {
	int x, y, type;
	sscanf(buffer, "%*s %d %d %d", &x, &y, &type);
	terrain[x][y] = type;
	return 0;
    }
    if (memcmp(buffer, "MINERALS", 8) == 0) // MINERALS
    {
	int pid, count;
	sscanf(buffer, "%*s %d %d", &pid, &count);
	minerals[pid] = count;
    }
    if (memcmp(buffer, "LOCATION", 8) == 0) // LOCATION
    {
	int pid, id, x, y, level;
	sscanf(buffer, "%*s %d %d %d %d %d", &pid, &id, &x, &y, &level);
	students[total_students].pid = pid;
	students[total_students].id = id;
	students[total_students].x = x;
	students[total_students].y = y;
	students[total_students].level = level;
	student_count[pid]++;
	total_students++;
    }
    if (memcmp(buffer, "YOURMOVE", 8) == 0) // YOURMOVE
    {
	int x, y, i;
	// Terrain info
	for (x = 0; x < boardsize; x++)
	{
	    for (y = 0; y < boardsize; y++)
	    {
		clientTerrainInfo(x, y, terrain[x][y]);
	    }
	}

	// Juice info
	for (i = 1; i < playernum; i++) // Yes, pid starts from 1
	{
	    clientJuiceInfo(i, minerals[i]);
	}


	for (i = total_students-1; i >= 0; i--)
	{
	    clientStudentLocation(students[i].pid, students[i].id,
				  students[i].x, students[i].y,
				  students[i].level);
	}
	
	// This is the dangerous bit: 
	clientDoTurn();

	// TODO: Send back build information (since we only build once)
	// But we should be fine with sending it within build() for now
	
	// Reset student count
	memset(student_count, 0, sizeof(int)*MAX_PLAYERS+2);
	total_students = 0;
    }
    return 0;
}


void setName(const char* name)
{
    if (!name_set)
    {
	strcpy(out_buffer, "NAME ");
	
	strncat(out_buffer, name, BUFFER_SIZE/4); // Might as well set a rough limit, since the server will have a further limit
	name_set = TRUE;
    }
    else
    {
	fprintf(stderr, "WARNING: client called setName() more than once\n");
    }
}

void move(int uid, int move)
{
    strcpy(out_buffer, "MOVE ");
    snprintf(tmp, BUFFER_SIZE/4, "%d", uid);
    strncat(out_buffer, tmp, BUFFER_SIZE/4);
    snprintf(tmp, BUFFER_SIZE/4, "%d", move);
    strncat(out_buffer, tmp, BUFFER_SIZE/4);

    if (send(soc, out_buffer, BUFFER_SIZE, 0) == -1)
    {
	fprintf(stderr, "error: could not send data\n");
    }
}

void build(int cost)
{
    strcpy(out_buffer, "BUILD ");
    snprintf(tmp, BUFFER_SIZE/4, "%d", cost);
    strncat(out_buffer, tmp, BUFFER_SIZE/4);

    if (send(soc, out_buffer, BUFFER_SIZE, 0) == -1)
    {
	fprintf(stderr, "error: could not send data\n");
    }
}

// Who the hell put these functions in the header
int getCost(int level)
{
    return level;
}

int getLevel(int cost)
{
    return cost;
}


int connect_self()
{
    struct sockaddr_in sockAddr;
    int Res; // ip in int form
    
    soc = socket(AF_INET, SOCK_STREAM, 0);

    if (soc == -1)
    {
	fprintf(stderr, "fatal: socket fail\n");
	return 1;
    }

    memset(&sockAddr, 0, sizeof(sockAddr));

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(12317);

    fprintf(stderr, "%s\n", ip_address);
    Res = inet_pton(AF_INET, ip_address, &sockAddr.sin_addr);
    if (0 > Res)
    {
	fprintf(stderr, "fatal: not a valid ip address family\n");
	close(soc);
	return 1;
    }
    else if (0 == Res)
    {
	fprintf(stderr, "fatal: not a valid ip address\n");
	close(soc);
	return 1;
    }

    for (;;)
    {
	if (-1 != connect(soc, (struct sockaddr *)&sockAddr, sizeof(sockAddr)))
	{
	    break;
	}
	fprintf(stderr, "error: connect failed\n");

	// UNIX SLEEP FUNCTION: GRRRR
	sleep(2);
    }
    
    return 0;
}

int cmd_line_args(int argc, char * argv[])
{
    int i;
    for (i = 1; i < argc; i++)
    {
	fprintf(stderr, "%s\n", argv[i]);
	if (argv[i][0] == '-')
	{
	    if (argv[i][1] == '-')
	    {
		if (strncmp("--vis-output", argv[i], 20) == 0)
		{
		    vis_output = TRUE;
		}
	    }
	    else
	    {
		if (strncmp("-vo", argv[i], 20) == 0)
		{
		    vis_output = TRUE;
		}
	    }
	}
	else if (ip_address[0] == '\0')
	{
	    fprintf(stderr, "%s\n", argv[i]);
	    strncpy(ip_address, argv[i], 19);
	}
    }
    return 0;
}
