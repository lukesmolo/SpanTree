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
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

#include "threads.h"
#include "main.h"
#include "net.h"
#include "utils.h"


#define NO_SLEEP 2 /*if it is commented there'll be not sleep*/
/*#define PRINT 1*/  /*if it is commented there'll be not print of send/recv packages*/
#define DEBUG 1 /*if it is commented there'll be not debug information for the algorithm of spanning tree*/

#define is_better_hop ((int)tmp->root[1] == ((int)msg->root[1])) && (tmp->hop < msg->hop) /*definition when there's msg better beacuse of a lower number of hops*/
#define is_better_root ((int)tmp->root[1]) < ((int)msg->root[1]) /*definition when there's a new root*/
#define is_better_bridge (((int)tmp->root[1]) == ((int)msg->root[1])) && (tmp->hop == msg->hop) && ((int)tmp->src_name[1] < (int)my_bridge->b_name[1]) /*definition when ther's msg better because of a lower bridge index*/

int is_root = 1;



int
is_better(bridges *my_bridge, mes *msg, mes *tmp) { /* this function compares two msg to find the best one*/
	int res;
	if(is_better_root) { /*the new message is better beacuse has a lower root index */
		res = 1;
	} else if(is_better_bridge) { /*the new message is better beacuse has a lower bridge index */
		res = 2;
	} else if(is_better_hop) {  /*the new message is better beacuse has a lower number of hops to the root */
		res = 3;
	} else {
		res = 0; /* new message is not better than the old one*/
	}

	return res;

}


int
bridge_thread_send(bridges *my_bridge, int socketfd, mes *msg, int *tmp_port, int *forward) {
	socklen_t  addr_size;
	int ris, i;
	struct sockaddr_in  To;
	unsigned short int remote_port_number;
	mes *tmp;

	if(*forward) { /*if the bridge has only to forward package ..*/
		tmp = msg;
		tmp->hop++; /*It needs to increment hop*/

	} else { /*else the bridge will send his msg*/
		tmp = &my_bridge->msg;
	}

	i = 0;
	while(my_bridge->bridge_lan[i].port && i < MAX_NUM_LANS) { /*for each lan of a bridge*/

		if(*forward && my_bridge->bridge_lan[i].port == *tmp_port) {
			my_bridge->bridge_lan[i].is_on  = 0; /*if the bridge has only to forward, and package arrives from port X, it needs to close it temporanely*/
		}
		if(my_bridge->bridge_lan[i].is_on && my_bridge->bridge_lan[i].port > 60000) { /*if the port of a lan is open*/
			remote_port_number = htons(my_bridge->bridge_lan[i].port);
			memset(&To, 0, sizeof(To));
			To.sin_family = AF_INET;
			To.sin_addr.s_addr = inet_addr("127.0.0.1"); /* htonl(INADDR_ANY); */
			To.sin_port = remote_port_number;
			addr_size = sizeof(struct sockaddr_in);
			do {

				ris = sendto(socketfd, tmp, sizeof(mes), 0, (struct sockaddr*)&To, addr_size);
				if (ris < (int)sizeof(mes)) {
					printf (RED"sendto of bridge() failed, Error: %d \"%s\"\n", errno,strerror(errno));
					printf(WHITE"\n"); /*if there's a problem with sendto, the control returns to bridge function and msg is lost*/
					return 0;
				}

			} while(errno == EINTR); /*useful to avoid so interruptions*/

#ifdef PRINT
			print_send(BRIDGE, my_bridge->b_name, socketfd, my_bridge->msg.root, ntohs(remote_port_number)); /*print of msg sent from a bridge*/
#endif

		}

		my_bridge->bridge_lan[i].is_on  = 1; /*re-opening the port that was closed*/
		i++;
	}
	*tmp_port = 0; /*port from which msg arrived is now re-opened*/
#ifdef NO_SLEEP
	sleep(NO_SLEEP);
#endif
	return 1;
}

mes
* bridge_thread_recv(bridges *my_bridge, int socketfd, int *tmp_port, int *forward) {
	struct sockaddr_in From;
	unsigned short int remote_port_number;
	unsigned int Fromlen, res;
	mes *tmp;
	int  msglen, i, j, modified;
	memset(&From, 0, sizeof(From));
	Fromlen = sizeof(struct sockaddr_in);

	tmp = malloc(sizeof(mes));
	if(tmp == NULL) {
		printf(RED"Malloc error\n");
		printf(WHITE"\n");
		exit(0);
	}
	modified = 0;
	do {
		msglen = recvfrom ( socketfd, tmp, sizeof(mes), 0, (struct sockaddr*)&From, &Fromlen);
		if (msglen < (int)sizeof(mes)) {
			printf (RED"Message arrived has not size of a mes! Err: %d \"%s",errno,strerror(errno));
			printf(WHITE"\n");
			return NULL;
		}

	} while(errno == EINTR);/*useful to avoid so interruptions*/


	remote_port_number = From.sin_port;
	i = 0;
	while(my_bridge->bridge_lan[i].port != ntohs(remote_port_number) && i < MAX_NUM_LANS) /*focusing on the bridge_lan from which the bridge received*/
		i++;
	res = is_better(my_bridge, &my_bridge->msg, tmp);
	if(res != 1) /*if there isn't a new root...*/
		res = is_better(my_bridge, my_bridge->bridge_lan[i].msg, tmp);

	if(res == 1) { /*there's a new root*/
		gettimeofday(&tv, NULL); /*it needs to take difference between the time and the last modification*/
		*forward = 1; /*set the flag of forwarding active. Now bridge (that is not root) will only forward packages*/
		memcpy(&my_bridge->msg, tmp, sizeof(mes));
		my_bridge->msg.hop++; /*it needs to increase hop*/
		strncpy(my_bridge->msg.src_name, my_bridge->b_name, 2); /*copying of own name in bridge msg*/
		j = 0;
		modified = 1; /*setting the flag for modification on*/
		is_root = 0;
		while(my_bridge->bridge_lan[j].port && j < MAX_NUM_LANS) { /*for each lan in my bridge I've to copy the new root information and re-set the flag of designed to 1*/
			memcpy(my_bridge->bridge_lan[j].msg, &my_bridge->msg, sizeof(mes));
			my_bridge->bridge_lan[j].is_designed = 1; /*bridge has to be designed newly for each lan*/
			j++;
		}
	} else if(res == 2 && !is_root && my_bridge->bridge_lan[i].is_designed) { /*if there's a better msg  because of a better bridge for a lan..*/
		gettimeofday(&tv, NULL); /*it needs to take difference between the time and the last modification*/
		memcpy(my_bridge->bridge_lan[i].msg, tmp, sizeof(mes)); /*the new msg is copied to the i-lan of  bridge*/
		my_bridge->bridge_lan[i].msg->hop++;
		my_bridge->bridge_lan[i].is_designed = 0; /*if the bridge discover that is not designed for the lan i, it sets the flag to 0*/
		modified = 1; /*setting the flag for modification on*/

	} else if(res ==  3 && !is_root &&my_bridge->bridge_lan[i].is_designed) { /*if there's a better msg beacuse of less number of hops...*/
		gettimeofday(&tv, NULL); /*it needs to take difference between the time and the last modification*/
		memcpy(my_bridge->bridge_lan[i].msg, tmp, sizeof(mes));
		my_bridge->bridge_lan[i].msg->hop++;
		my_bridge->bridge_lan[i].is_designed = 0; /*if the bridge discover that is not designed for the lan i, it sets the flag to 0*/

		modified = 1; /*set the flag of forwarding active. Now bridge (that is not root) will only forward packages*/

	}

#ifdef DEBUG
	if(modified) {
		print_table(my_bridge, tmp, res, i, DEBUG);
		modified = 0;
	}
#else
	if(modified) {
		print_table(my_bridge, NULL, 0, 0, 0);
		modified  = 0;
	}
#endif

	*tmp_port = ntohs(remote_port_number);
#ifdef PRINT
	print_recv(BRIDGE, my_bridge->b_name, socketfd,  tmp->root, ntohs(remote_port_number));
#endif

	strncpy(tmp->src_name, my_bridge->b_name, 2); /*at the end it needs to change the src_name of msg that will be forward with the bridge which sends it*/


	return tmp;
}


mes
* lan_thread_recv(lans *my_lan, int socketfd, int *tmp_port) {
	struct sockaddr_in  From;
	unsigned short int remote_port_number;
	int msglen;
	unsigned int Fromlen;
	mes *msg;

	/* wait for datagram  */
	msg = malloc(sizeof(mes));

	if(msg == NULL) {
		printf(RED"Malloc error\n");
		printf(WHITE"\n");
		exit(0);
	}
	memset(&From, 0, sizeof(From));
	Fromlen=sizeof(struct sockaddr_in);
	do {
		msglen = recvfrom ( socketfd, msg, sizeof(*msg), 0, (struct sockaddr*)&From, &Fromlen);
		if (msglen < 0) {
			printf (RED"Message arrived is empty! Err: %d \"%s\"\n",errno,strerror(errno));
			printf(WHITE"\n");
			return NULL;
		}

	} while(errno == EINTR );/*useful to avoid so interruptions*/

	remote_port_number = From.sin_port;
#ifdef PRINT
	print_recv(LAN, &my_lan->l_name, socketfd,  msg->root, ntohs(remote_port_number));
#endif
	*tmp_port = ntohs(remote_port_number); /*saving in tmp_port the port from which lan has received*/


#ifdef NO_SLEEP
	sleep(NO_SLEEP + 1);
#endif
	return msg;
}


int
lan_thread_send(lans *my_lan, int socketfd, mes *msg, int *tmp_port) {
	int ris, i;
	struct sockaddr_in To;
	socklen_t addr_size;
	unsigned short int remote_port_number;


	i = 0;	
	while(my_lan->lan_bridge[i].port && i < MAX_NUM_BRIDGES) { /*for each bridge on the lan forward the msg*/

		if(*tmp_port != my_lan->lan_bridge[i].port && (strncmp(my_lan->lan_bridge[i].b_name, msg->src_name, 2) && my_lan->lan_bridge[i].port > 60000) ) { /*if the port for sending the msg is not the same from which lan received or the msg src is not the same of the bridge to which the lan has to send...*/
			remote_port_number = htons(my_lan->lan_bridge[i].port);
			memset(&To, 0, sizeof(struct sockaddr_in));
			To.sin_family = AF_INET;
			To.sin_addr.s_addr = inet_addr("127.0.0.1"); /* htonl(INADDR_ANY); */
			To.sin_port = remote_port_number;
			addr_size = sizeof(struct sockaddr_in);
			do {

				ris = sendto(socketfd, msg, sizeof(*msg), 0, (struct sockaddr*)&To, addr_size);
				if (ris < sizeof(mes)) {
					printf (RED"sendto() of a lan failed, Error: %d \"%s\"\n", errno,strerror(errno));
					printf(WHITE"\n");
					return 0;
				}
			} while(errno ==EINTR); /*useful to avoid so interruptions*/
#ifdef PRINT
			print_send(LAN, &my_lan->l_name, socketfd, msg->root, ntohs(remote_port_number));
#endif

		}
		i++;
	}

#ifdef NO_SLEEP
	sleep(NO_SLEEP + 1);
#endif

	return 1;

}


int
bridge(bridges *my_bridge) {
	struct sockaddr_in Local;
	fd_set socketRead, socketWrite;
	unsigned short int local_port_number;
	mes *msg;
	int ris;
	int *forward;
	int socketfd;
	int *tmp_port;
	int optval;


	local_port_number = htons(my_bridge->port);
	socketfd = socket (AF_INET, SOCK_DGRAM, 0);

	if(socketfd == SOCKET_ERROR) {
		printf(RED"socket() failed, Err: %d \"%s\"", errno,strerror(errno));
		printf(WHITE"\n");
		exit(1);
	}

	optval = 1;
	ris = setsockopt(socketfd,SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
	if(ris < 0) {
		perror("setsockopt error");
		fflush(stderr);
		exit(1);
	}

	memset(&Local, 0, sizeof(Local));
	Local.sin_family = AF_INET;
	Local.sin_addr.s_addr = htonl(INADDR_ANY);
	Local.sin_port = local_port_number;

	ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));

	if(ris == SOCKET_ERROR) {
		printf (RED"bind() 1 failed, Err: %d \"%s\"",errno,strerror(errno));
		printf(WHITE"\n");
		exit(1);
	}


	forward = malloc(sizeof(int));
	tmp_port= malloc(sizeof(int));
	if(forward == NULL || tmp_port == NULL) {
		printf(RED"Malloc error\n");
		printf(WHITE"\n");
		exit(0);
	}
	*forward = 0;
	*tmp_port= 0;

	while(end) {
		do {
			FD_ZERO(&socketWrite);
			FD_ZERO(&socketRead);
			FD_SET(socketfd,&socketWrite);
			FD_SET(socketfd,&socketRead);

			ris = select(socketfd + 1, &socketRead, &socketWrite, 0, 0);

		} while( (ris < 0) && (errno == EINTR) ); /*useful to avoid so interruptions*/

		if(FD_ISSET(socketfd, &socketRead)) { /*if the socket to read is ready..*/
			msg = bridge_thread_recv(my_bridge, socketfd, tmp_port, forward);
			if(!msg && *forward == 1) { /*if there was an error during bridge_recv and the bridge has only to forward, it needs to continue with while because there're not packages to forward*/
				continue;
			}
		}
		if(FD_ISSET(socketfd, &socketWrite) && msg) { /*if socket for writing is ready and msg is not NULL...*/
			ris = bridge_thread_send(my_bridge, socketfd, msg, tmp_port, forward);
		}


	}
	close(socketfd);
	free(tmp_port);
	free(forward);
	if(msg)
		free(msg);/*it needs to free msg*/
	return 1;
}

int
lan(lans *my_lan) {
	struct sockaddr_in Local;
	fd_set socketRead, socketWrite;
	unsigned short int local_port_number;
	int ris;
	mes *msg = NULL;
	int socketfd;
	int *tmp_port;
	int optval;

	local_port_number = htons(my_lan->port);
	socketfd = socket (AF_INET, SOCK_DGRAM, 0);

	if(socketfd == SOCKET_ERROR) {
		printf(RED"socket() failed, Err: %d \"%s\"", errno,strerror(errno));
		printf(WHITE"\n");
		exit(1);
	}
	memset(&Local, 0, sizeof(Local));
	Local.sin_family = AF_INET;
	Local.sin_addr.s_addr = htonl(INADDR_ANY);
	Local.sin_port = local_port_number;
	optval = 1;
	ris = setsockopt(socketfd,SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
	if(ris < 0) {
		perror("setsockopt error");
		fflush(stderr);
		exit(1);
	}

	ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));

	if(ris == SOCKET_ERROR) {
		printf (RED"bind() 1 failed, Err: %d \"%s\"",errno,strerror(errno));
		printf(WHITE"\n");
		exit(1);
	}

	tmp_port = malloc(sizeof(int));
	if(tmp_port == NULL) {
		printf(RED"Malloc error\n");
		printf(WHITE"\n");
		exit(0);
	}


	while(end) {
		do {
			FD_ZERO(&socketWrite);
			FD_ZERO(&socketRead);
			FD_SET(socketfd,&socketWrite);
			FD_SET(socketfd,&socketRead);

			ris = select(socketfd + 1, &socketRead, &socketWrite, 0, 0);
		} while( (ris < 0) && (errno == EINTR) );

		if(FD_ISSET(socketfd, &socketRead)) {/*if socket for reading is ready and msg is not NULL...*/
			msg = lan_thread_recv(my_lan, socketfd, tmp_port);
			if(!msg) { /*if there was an error during lan_receive, there's not msg to forward, so  it needs to continue with while*/
				continue;
			}
			if(FD_ISSET(socketfd, &socketWrite)) { /*if the socket to send is ready...*/
				ris = lan_thread_send(my_lan, socketfd, msg, tmp_port);

			}
		}
	}

	close(socketfd);
	free(tmp_port);/*it needs to free tmp_port*/

	return 1;

}/*end lan*/
