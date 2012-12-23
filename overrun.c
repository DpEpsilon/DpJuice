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
#define BUFFER_SIZE 6000
#define COMMAND_SIZE 1024
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

char in_buffer[BUFFER_SIZE+10];
char out_buffer[BUFFER_SIZE+10];
char tmp[TMP_SIZE];

int command_upto = 0;
char command_buffer[COMMAND_SIZE];

int name_set = FALSE;

int has_built = FALSE;

ssize_t packet_size;

char ip_address[20];
int vis_output = FALSE;

int soc;

int cmd_line_args(int args, char * argv[]);
int connect_self();
int process_packet(char *buffer);

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
			memset(in_buffer, 0, sizeof(char)*BUFFER_SIZE);
			packet_size = recv(soc, in_buffer, BUFFER_SIZE, 0);
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
            int buf_upto = 0;

            while (in_buffer[buf_upto] != '\0') {
                command_buffer[command_upto++] = in_buffer[buf_upto];
                if (in_buffer[buf_upto] == '\n') {
                    command_buffer[command_upto+1] = '\0';
                    prc_ret = process_packet(command_buffer);
                    command_upto = 0;
                }
                buf_upto++;
            }
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


// Returns 2 if there is a need to reconnect,
// 1 if an fatal error occurs and 0 otherwise.
int process_packet(char *buffer)
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

		if (send(soc, out_buffer, strlen(out_buffer), 0) == -1)
		{
			fprintf(stderr, "error: could not send data\n");
			return 2;
		}
    } else if (memcmp(buffer, "NEWGAME", 7) == 0) {
        // %*s because I'm cool like that
		sscanf(buffer, "%*s %d %d %d %s",
               &playernum, &boardsize, &playerid, tmp);

		clientInit(playernum, boardsize, playerid);

		strcpy(out_buffer, "READY ");
		strncat(out_buffer, tmp, BUFFER_SIZE/4);
		strcat(out_buffer, "\n");
	
		if (send(soc, out_buffer, strlen(out_buffer), 0) == -1)
		{
			fprintf(stderr, "error: could not send data\n");
			return 2;
		}
    } else if (memcmp(buffer, "GAMEOVER", 8) == 0) {
        fprintf(stderr, buffer);
    } else if (memcmp(buffer, "CELL", 4) == 0) {
		int x, y, type;
		sscanf(buffer, "%*s %d %d %d", &x, &y, &type);
		terrain[x][y] = type;
    } else if (memcmp(buffer, "MINERALS", 8) == 0) {
		int pid, count;
		sscanf(buffer, "%*s %d %d", &pid, &count);
		minerals[pid] = count;
        
    } else if (memcmp(buffer, "LOCATION", 8) == 0) {
		int pid, id, x, y, level;
		sscanf(buffer, "%*s %d %d %d %d %d", &pid, &id, &x, &y, &level);
		students[total_students].pid = pid;
		students[total_students].id = id;
		students[total_students].x = x;
		students[total_students].y = y;
		students[total_students].level = level;
		student_count[pid]++;
		total_students++;
    } else if (memcmp(buffer, "YOURMOVE", 8) == 0) {
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
		for (i = 1; i <= playernum; i++) // Yes, pid starts from 1
		{
			clientJuiceInfo(i, minerals[i]);
		}

		for (i = total_students-1; i >= 0; i--)
		{
			clientStudentLocation(students[i].pid, students[i].id,
								  students[i].x, students[i].y,
								  students[i].level);
		}

        has_built = FALSE;
		// This is the dangerous bit: 
		clientDoTurn();

        if (!has_built) {
            build(0);
        }

		// TODO: Send back build information (since we only build once)
		// But we should be fine with sending it within build() for now
	
		// Reset student count
		memset(student_count, 0, sizeof(int)*MAX_PLAYERS+2);
		total_students = 0;
    } else {
        fprintf(stderr, "WARNING: unknown header:\n");
    }
    return 0;
}


void setName(const char* name)
{
    if (!name_set)
    {
		strcpy(out_buffer, "NAME ");

        // Might as well set a rough limit,
        // since the server will have a further limit
		strncat(out_buffer, name, BUFFER_SIZE/4);
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
    snprintf(tmp, BUFFER_SIZE/4, "%d ", uid);
    strncat(out_buffer, tmp, BUFFER_SIZE/4);
    snprintf(tmp, BUFFER_SIZE/4, "%d\n", move);
    strncat(out_buffer, tmp, BUFFER_SIZE/4);

    if (send(soc, out_buffer, strlen(out_buffer), 0) == -1)
    {
		fprintf(stderr, "error: could not send data\n");
    }
}

void build(int cost)
{
    if (has_built) {
        fprintf(stderr, "WARNING: client called build() more than once "
                "in this turn\n");
    }
    has_built = TRUE;
    strcpy(out_buffer, "BUILD ");
    snprintf(tmp, BUFFER_SIZE/4, "%d\n", cost);
    strncat(out_buffer, tmp, BUFFER_SIZE/4);

    if (send(soc, out_buffer, strlen(out_buffer), 0) == -1)
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
