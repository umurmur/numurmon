#include <fcntl.h> /* For O_* constants */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "sharedmemory_struct.h"
//#include <getopt.h>
#include <curses.h>
#include <signal.h>


enum{ NOP_SHM, WAIT_ATTACH_SHM, TRY_ATTACH_SHM, MAT_SHM, CLEAN_UP_SHM, RUN_SHM };
shm_t *shmptr = NULL;
char shm_file_name[128];
int shm_fd, waitTime = 0, opt;
uint8_t last, shm_statem = TRY_ATTACH_SHM;



void run_shm(void);
char * format_time( uint32_t secs );
static void finish(int sig);
static void handle_winch(int sig);

int main(int argc, char **argv)
{
	struct stat buf;
	int bindport = 0;

	while ( (opt = getopt(argc, argv, "wb:")) != -1 )
	{
		switch(opt)
		{
			case 'w':
				waitTime = 1;
				break;
			case 'b':
				bindport = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Usage: %s [-w] [-b <port>]\n", argv[0]);
				fprintf(stderr, "\t-w         - Wait for umurmurd to create shm area. useful if you need to start from init.d script\n" );
				fprintf(stderr, "\t-b <port>  - Change this to the port used when starting umurmurd. Defaults to \"/umurmurd:64738\" \n");
				exit(EXIT_FAILURE);
		}
	}
	
	signal(SIGINT, finish);
	signal(SIGWINCH, handle_winch);

	initscr();
	//nonl();
	keypad(stdscr, TRUE);
	noecho();
	cbreak();
	curs_set(0);

		if( has_colors() )
		{
			start_color();
			init_pair(1, COLOR_RED,    COLOR_BLACK);
			init_pair(2, COLOR_GREEN,  COLOR_BLACK);
			init_pair(3, COLOR_YELLOW, COLOR_BLACK);
			init_pair(4, COLOR_BLUE,   COLOR_BLACK);
			init_pair(5, COLOR_CYAN,   COLOR_BLACK);
			init_pair(6, COLOR_WHITE,  COLOR_BLACK);
			init_pair(7, COLOR_WHITE,  COLOR_BLACK);
		}

	clear();
	refresh();

	if( !bindport )
	{
		bindport = 64738;
	}

	sprintf( shm_file_name, "/umurmurd:%i", bindport );

	if( waitTime )
		shm_statem = WAIT_ATTACH_SHM;

	while( shm_statem )
	{
		switch( shm_statem )
		{
			case RUN_SHM:
					run_shm();
					refresh();
				break;
			case WAIT_ATTACH_SHM:
					mvprintw( 0, 0, "Waiting for umurmurd to be run"); refresh();
					while( ( shm_fd = shm_open( shm_file_name, O_RDONLY, 0666 ) ) == -1 )
						sleep( 1 );
					shm_statem = MAT_SHM;
				break;
			case TRY_ATTACH_SHM:
					if( ( shm_fd = shm_open( shm_file_name, O_RDONLY, 0666 ) ) == -1 )
					{
						mvprintw( 0, 0, "umurmurd doesn't seem to be running" ); refresh();
						exit(EXIT_FAILURE);
					}
					shm_statem = MAT_SHM;
				break;
			case MAT_SHM:
					fstat( shm_fd, &buf);
					if( ( shmptr = mmap( 0, buf.st_size, PROT_READ, MAP_SHARED, shm_fd, 0 ) ) == MAP_FAILED )
					{
						exit(EXIT_FAILURE);
					}
					shm_statem = RUN_SHM;
				break;
			case CLEAN_UP_SHM:

				break;

		}
	}
	return 0;
}

uint8_t check_serverTick(void)
{
	last = shmptr->alive;
	sleep( 1 );  // Sleep for 1 sec
	return(shmptr->alive - last);
}

void run_shm(void)
{

	int cc, cl;
	static int change_in_clients;

	mvprintw( 0, 3, "Server Info" );
	mvchgat( 0, 0, -1, A_REVERSE, 5, NULL );


	mvprintw( 1, 0, " PID" );
	mvprintw( 1, 15, "SHM_FD");
	mvprintw( 1, 36, "CLIENTS(CONECTED/MAX)" );

	mvchgat( 1, 0, -1, A_REVERSE, 0, NULL );

	mvprintw( 2, 1, "%u", shmptr->umurmurd_pid );
	mvprintw( 2, 15, "%s", shm_file_name );
	mvprintw( 2, 50, "%2i/%2i", shmptr->clientcount, shmptr->server_max_clients );

	mvprintw( 3, 3, "Client Info" );
	mvchgat( 3, 0, -1, A_REVERSE, 5, NULL );

	mvprintw( 4, 0, " USER@ADDRESS:PORT/CHANNEL" );
	mvprintw( 4, 46, "ONLINE" );
	mvprintw( 4, 57, "IDLE" );
	
	mvprintw( 4, 65, "M D S" );
	mvchgat( 4, 0, -1, A_REVERSE, 0, NULL );
  
	for( cc = 0 ; cc < shmptr->server_max_clients ; cc++ )
	{

  	cl = cc + 5;

		if( change_in_clients > shmptr->clientcount )
		{
			move( cl, 0);
			clrtoeol();	
		}

			if( !shmptr->client[cc].authenticated )
				continue;
		
		
				if( shmptr->client[cc].availableBandwidth < 10000 )							//FIXME: Re work this miss case were user talking on join doesnt print ipaddress
				{
					attrset( COLOR_PAIR(2) );
					mvprintw( cl, 1, "%s", shmptr->client[cc].username );
					standend();
				}
				else if( shmptr->client[cc].isAdmin )
							{
								attrset( COLOR_PAIR(1) );
								mvprintw( cl, 1, "%s", shmptr->client[cc].username );
								printw( "@%s:%i", shmptr->client[cc].ipaddress,
																	shmptr->client[cc].udp_port );
								standend();
								printw( "/%s", shmptr->client[cc].channel );
							}
							else
								{
									mvprintw( cl, 1, "%s", shmptr->client[cc].username );
									printw( "@%s:%i", shmptr->client[cc].ipaddress,
																		shmptr->client[cc].udp_port );
									printw( "/%s", shmptr->client[cc].channel );													 		
								}
					
		
			mvprintw( cl, 45, "%s  %s", format_time( shmptr->client[cc].online_secs ),
																		format_time( shmptr->client[cc].idle_secs)			);

		

//			shmptr->client[cc].os,
//			shmptr->client[cc].os_version,
//			shmptr->client[cc].release,
//			shmptr->client[cc].availableBandwidth,


			mvaddch( cl, 65, shmptr->client[cc].self_mute || shmptr->client[cc].mute ? (shmptr->client[cc].mute ? 'S' : '*') : ' ' );

			mvaddch( cl, 67, shmptr->client[cc].self_deaf || shmptr->client[cc].deaf ? (shmptr->client[cc].deaf ? 'S' : '*') : ' ' );

			mvaddch( cl, 69, shmptr->client[cc].isSuppressed ? '*' : ' ' );

//			shmptr->client[cc].bUDP,
//			shmptr->client[cc].recording,
//			shmptr->client[cc].bOpus,
//
//
//			shmptr->client[cc].UDPPingAvg,
//			shmptr->client[cc].UDPPingVar,
//			shmptr->client[cc].TCPPingAvg,
//			shmptr->client[cc].TCPPingVar,
//			shmptr->client[cc].UDPPackets,
//			shmptr->client[cc].TCPPackets );
			
	}
	change_in_clients = shmptr->clientcount;
	if( !check_serverTick() )
	{
		exit(EXIT_FAILURE);
	}
}

char * format_time( uint32_t secs )
{
	static char ftime[9];
	int  hour, min, sec, sec_left;
	
		hour = secs / 3600;
		sec_left = secs % 3600;
		min = sec_left / 60;
		sec = sec_left % 60;
	
		sprintf( ftime, "%02i:%02i:%02i", hour, min, sec );
	
	return ftime;
}

static void finish(int sig)
{
	endwin();

	close( shm_fd );
	exit(0);
}

void handle_winch(int sig)
{
	clearok( stdscr, TRUE );
	refresh();
}
