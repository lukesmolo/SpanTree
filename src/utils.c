/*
 * Copyright (C) 2013 Luca Sciullo
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "main.h"
#include "net.h"
#include "threads.h"
#include "utils.h"



pthread_mutex_t print_lock_stdout;

void
*finish(void *null) {

	struct timeval t;
	time_t diff;
	char c;
	int res;
	int timeout;

	while(end) {
		t.tv_usec = 0;
		t.tv_sec = 40; /*set time to wake up select*/

		do {
			res =select(0, NULL, NULL, NULL, &t);

		} while( (res < 0) && (errno == EINTR) ); /*useful to avoid so interruptions*/

		gettimeofday(&t, NULL);
		diff = t.tv_sec - tv.tv_sec; /*calculating the difference between now and the last editing of a bridge table */

		timeout = n_bridges * 20;

		pthread_mutex_lock(&print_lock_stdout);
		if(diff >= timeout) { /*if the timeout is expired..*/
			printf(RED"Timeout of %d expired\n"WHITE, timeout);

			do {
				printf(WHITE"Would you like to continue the test [y/n]\n");
				scanf(" %c", &c);
				if(c != 'n' && c!='y') {
					printf(RED"Error! You have to insert y or n\n");
				}
			} while(c != 'n' && c!='y');
			if( c == 'n') {
				printf(WHITE"Closing ...\n");
				end = 0;
			}
			else {
				printf(WHITE"Continuing...\n");
			}	
		} else {
			printf(RED"Timeout was not expired. The difference now is: %ld\n", diff);
			printf(WHITE"Continuing...\n");
		}

		pthread_mutex_unlock(&print_lock_stdout);
	}

	pthread_exit(NULL);

}



void
int_to_port(int what, int port, char from_to[]) { /*this funtion converts int to char to obtain the name of a lan or a bridge*/

	if(what == LAN) {
		from_to[0] = 'B';
		sprintf(&from_to[1], "%d", (port - DEFAULT_PORT));
	}
	else {
		from_to[0] = (char)(port - DEFAULT_PORT);
		from_to[1] = 0;
	}
	return;
}

void
print_table(bridges *my_bridge, mes* tmp, int what, int index, int debug) {

	int i;
	pthread_mutex_lock(&print_lock_stdout);
	if(debug) {
		switch(what) {
			case 1: printf(GREEN"New root (%.2s-%c)\t(%.2s-%d-%.2s), (%.2s-%d-%.2s)\n", my_bridge->b_name, my_bridge->bridge_lan[index].lan_name, my_bridge->bridge_lan[index].msg->root, my_bridge->bridge_lan[index].msg->hop, my_bridge->bridge_lan[index].msg->src_name, tmp->root, tmp->hop, tmp->src_name);
				  break;
			case 2: printf(ORANGE"Different bridge (%.2s-%c)\t(%.2s-%d-%.2s), (%.2s-%d-%.2s)\n", my_bridge->b_name, my_bridge->bridge_lan[index].lan_name, my_bridge->bridge_lan[index].msg->root, my_bridge->bridge_lan[index].msg->hop, my_bridge->bridge_lan[index].msg->src_name, tmp->root, tmp->hop, tmp->src_name);
				  break;
			case 3: printf(VIOLET"Less hop (%.2s-%c)\t(%.2s-%d-%.2s), (%.2s-%d-%.2s)\n", my_bridge->b_name, my_bridge->bridge_lan[index].lan_name, my_bridge->bridge_lan[index].msg->root, my_bridge->bridge_lan[index].msg->hop, my_bridge->bridge_lan[index].msg->src_name, tmp->root, tmp->hop, tmp->src_name);
				  break;
			default: ;
		}
	}
	printf(GREEN"%s\t\t", my_bridge->b_name);
	i = 0;
	while(my_bridge->bridge_lan[i].port && i < MAX_NUM_LANS) {
		if(my_bridge->bridge_lan[i].is_designed) {
			printf(BLUE"%c\t", my_bridge->bridge_lan[i].lan_name);
		}
		i++;
	}
	printf(WHITE"\n");
	pthread_mutex_unlock(&print_lock_stdout);
}


void
print_recv(int what, char *name, int socket, char *msg, int port) {

	char from[5];
	int_to_port(what, port,  from);


	pthread_mutex_lock(&print_lock_stdout);

	if(what == LAN) { /*if what is 1, a lan has received something*/

		printf("[");
		printf(CYAN"%s", name);
		printf(WHITE"]\t");
	} else { /*this is the bridge case*/

		printf("[");
		printf(BLUE"%s", name);
		printf(WHITE"]\t");
	}
	printf(GREEN"Received "WHITE"at socket ");
	printf(ORANGE"%d ", socket);
	printf(WHITE"msg: ");
	printf(YELLOW "%s ", msg);
	printf(WHITE"from ");
	printf(RED"%s"WHITE, from);
	printf(":");
	printf(VIOLET"%d", port);
	printf(WHITE "\n");
	pthread_mutex_unlock(&print_lock_stdout);
	return;
}
void
print_send(int what, char *name, int socket, char *msg, int port) {

	char to[5];
	pthread_mutex_lock(&print_lock_stdout);
	int_to_port(what, port,  to);
	if(what == LAN) {
		printf("[");
		printf(CYAN"%s", name);
		printf(WHITE"]\t");
	} else { /*this is the bridge case*/
		printf("[");
		printf(BLUE"%s", name);
		printf(WHITE"]\t");
	}
	printf(ORANGE"Sent " WHITE"from socket ");
	printf(ORANGE"%d ", socket);
	printf(WHITE"msg: ");
	printf(YELLOW "%s ", msg);
	printf(WHITE"to ");
	printf(RED"%s"WHITE, to);
	printf(":");
	printf(VIOLET"%d", port);
	printf(WHITE "\n");

	pthread_mutex_unlock(&print_lock_stdout);
	return;
}
