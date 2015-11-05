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

int shm_fd;
shm_t *shmptr = NULL;
char shm_file_name[128];

char tcolor[8] = "\033[31m", ncolor[8] = "\033[37m";

char *tc_on, *tc_off;

int waitTime = 0, opt;
uint8_t last, shm_statem = TRY_ATTACH_SHM;

void run_shm(void);
char * format_time( uint32_t secs );

static void finish(int sig);
void handle_winch(int sig);
                                            
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
	
	signal(SIGINT, finish);      		/* arrange interrupts to terminate */
  signal(SIGWINCH, handle_winch);
  initscr();      								/* initialize the curses library */
  
    if (has_colors())
    {
        start_color();

        /*
         * Simple color assignment, often all we need.  Color pair 0 cannot
         * be redefined.  This example uses the same value for the color
         * pair as for the foreground color, though of course that is not
         * necessary:
         */
        init_pair(1, COLOR_RED,    COLOR_BLACK);
        init_pair(2, COLOR_GREEN,  COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_BLUE,   COLOR_BLACK);
        init_pair(5, COLOR_CYAN,   COLOR_BLACK);
        init_pair(6, COLOR_WHITE,  COLOR_BLACK);
        init_pair(7, COLOR_WHITE,  COLOR_BLACK);
    } 
  
  //cbreak();
  
  nonl();
  //intrflush(stdscr, FALSE);
 // keypad(stdscr, TRUE);
  
  clear();
  refresh();

	shmptr = NULL;

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
				mvprintw( 0,0, "Waiting for umurmurd to be run"); refresh();//fflush(stdout);
				while( ( shm_fd = shm_open( shm_file_name, O_RDONLY, 0666 ) ) == -1 )
					//sleep( 1 );
				shm_statem = MAT_SHM;
				break;
			case TRY_ATTACH_SHM:
				if( ( shm_fd = shm_open( shm_file_name, O_RDONLY, 0666 ) ) == -1 )
				{
					mvprintw( 0,0, "umurmurd doesn't seem to be running" ); refresh();
					exit(EXIT_FAILURE);
				}
				shm_statem = MAT_SHM;
				break;
			case MAT_SHM:
				fstat( shm_fd, &buf);
				if( ( shmptr = mmap(0, buf.st_size, PROT_READ, MAP_SHARED, shm_fd, 0) ) == MAP_FAILED )
				{
					exit(EXIT_FAILURE);
				}
				//mvprintw( 0, 0, "umumurd PID: %u", shmptr->umurmurd_pid ); refresh();
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

	int cc;
	static int change_in_clients;

	//mvprintw( 1, 2, "%s  Clients(CONECTED/MAX)  %i/%i", shm_file_name, shmptr->clientcount, shmptr->server_max_clients );
  
  mvprintw( 0, 3, "Server Info");
  mvchgat(0, 0, -1, A_REVERSE, 5, NULL);
  
  
  mvprintw( 1, 0, " PID");
  mvprintw( 1, 36, "Clients(CONECTED/MAX)" );
	//mvprintw( 1, 47, "IDLE" );

  mvchgat(1, 0, -1, A_REVERSE, 0, NULL);
  
  mvprintw( 2, 1, "%u", shmptr->umurmurd_pid);

  mvprintw( 3, 3, "Client Info");
  mvchgat( 3, 0, -1, A_REVERSE, 5, NULL);
	
  mvprintw( 4, 0, " USER@ADDRESS:PORT");
  mvprintw( 4, 36, "ONLINE" );
	mvprintw( 4, 47, "IDLE" );
	//mvprintw( 3, 47, "IDLE" );
	mvprintw( 4, 55, "M D S" );
  mvchgat(4, 0, -1, A_REVERSE, 0, NULL);
  

  
  

	for( cc = 0 ; cc < shmptr->server_max_clients ; cc++ )
	{

  if( change_in_clients > shmptr->clientcount )
  {
	  move( cc + 5, 0);
		clrtoeol();
	
	}

		if( !shmptr->client[cc].authenticated )
			continue;
		
		
		if( shmptr->client[cc].availableBandwidth < 10000 )
		{
				attrset( COLOR_PAIR(2) );
        mvprintw( cc + 5, 1, "%s", shmptr->client[cc].username );
        standend();
		}
		else if( shmptr->client[cc].isAdmin )
       	 {
        		attrset( COLOR_PAIR(1) );
        		mvprintw( cc + 5, 1, "%s", shmptr->client[cc].username );
						printw( "@%s:%i", shmptr->client[cc].ipaddress,
						    				 	shmptr->client[cc].udp_port );        		
        		standend();
       	}
		  	else
		  		{
		    			mvprintw( cc + 5, 1, "%s", shmptr->client[cc].username );
							printw( "@%s:%i", shmptr->client[cc].ipaddress,
						    				 	shmptr->client[cc].udp_port );		
					}
        
					
		
// 		printw( "@%s:%i", shmptr->client[cc].ipaddress,
// 						    				 	shmptr->client[cc].udp_port );
						    				 	
		mvprintw( cc + 5, 35, "%s", format_time( shmptr->client[cc].online_secs ) );
		
		printw( "  %s",	 format_time( shmptr->client[cc].idle_secs ) );
		
// 		mvprintw( 3, 3, " %s%s%s@%s:%i in channel: %s\
// 			\tclient_OS: %s %s\
// 			\tclient_info: %s\
// 			\tavailableBandwidth: %i\
// 			\tOnline(secs): %s Idle(secs): %i\
// 			\tusingUDP=%i\
// 			\tdeaf=%i, mute=%i\
// 			\tself_deaf=%i, self_mute=%i\
// 			\trecording=%i\
// 			\tbOpus=%i\
// 			\tisAdmin=%i\
// 			\tisSuppressed=%i\
// 			\tUDP_Avg/Var: %3.2f/%3.2f\
// 			\tTCP_Avg/Var: %3.2f/%3.2f\
// 			\tUDP_C/TCP_C: %i/%i",
// 			"",//tc_on,
// 			shmptr->client[cc].username,
// 			"", //tc_off,
// 			shmptr->client[cc].ipaddress,
// 			shmptr->client[cc].udp_port,
// 			shmptr->client[cc].channel,
// 			shmptr->client[cc].os,
// 			shmptr->client[cc].os_version,
// 			shmptr->client[cc].release,
// 			shmptr->client[cc].availableBandwidth,
// 			//shmptr->client[cc].online_secs,
// 			format_time( shmptr->client[cc].online_secs ),
// 			shmptr->client[cc].idle_secs,
// 
// 			shmptr->client[cc].bUDP,
// 			shmptr->client[cc].deaf,
// 			shmptr->client[cc].mute,
// 			shmptr->client[cc].self_deaf,
// 			shmptr->client[cc].self_mute,
// 			shmptr->client[cc].recording,
// 			shmptr->client[cc].bOpus,
// 
// 			shmptr->client[cc].isAdmin,
// 			shmptr->client[cc].isSuppressed,
// 
// 			shmptr->client[cc].UDPPingAvg,
// 			shmptr->client[cc].UDPPingVar,
// 			shmptr->client[cc].TCPPingAvg,
// 			shmptr->client[cc].TCPPingVar,
// 			shmptr->client[cc].UDPPackets,
// 			shmptr->client[cc].TCPPackets ); //fflush(stdout);  // fflush need because of sleep() call
			
	}
	change_in_clients = shmptr->clientcount;
	if( !check_serverTick() )
	{
		exit(EXIT_FAILURE); //You dont have to exit you could just report the fact that the data is not valid
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

    /* do your non-curses wrapup here */
    
    exit(0);
}

void handle_winch(int sig)
{
	//endwin();
	clearok( stdscr, TRUE );    //clearok( curscr, TRUE );
	refresh();
}