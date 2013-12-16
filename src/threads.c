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


#include "main.h"
#include "net.h"
#include "threads.h"
#include "utils.h"

pthread_mutex_t bridges_sync_lock;
pthread_cond_t bridges_sync_cond;
pthread_mutex_t lans_sync_lock;
pthread_cond_t lans_sync_cond;

int bridges_sync_count = 0;
int lans_sync_count= 0;

void
bridges_Sync(bridges *my_bridge) {
	pthread_mutex_lock(&bridges_sync_lock); /*thread takes mutual exclusion to manage bridges_sync_count*/
	bridges_sync_count++; /*thread increases number of threads which are synchronizing*/

	if(bridges_sync_count < n_bridges ) { /*thread checks if it has to wait for others threads*/
		pthread_cond_wait(&bridges_sync_cond, &bridges_sync_lock); /*thread waits for a signal*/
	}
	/*here I'm sure that bridges are synchronized*/

	/*print threads information*/
	print_table(my_bridge, NULL, 0, 0, 0);
	pthread_cond_signal(&bridges_sync_cond); /*the first arrived here begins to unlock others threads*/
	pthread_mutex_unlock(&bridges_sync_lock);/*thread releases m.e*/

	return;
}

void
lans_Sync(lans *my_lan) {
	pthread_mutex_lock(&lans_sync_lock); /*thread takes mutual exclusion to manage sync_count*/
	lans_sync_count++; /*thread increases number of threads which are syncronizing*/

	if(lans_sync_count < n_lans) { /*thread checks if it has to wait for others threads*/
		pthread_cond_wait(&lans_sync_cond, &lans_sync_lock); /*thread waits for a signal*/
	}
	pthread_cond_signal(&lans_sync_cond); /*the first arrived here begins to unlock others threads*/
	pthread_mutex_unlock(&lans_sync_lock);/*thread releases m.e*/

	return;
}


void
*bridges_Thread(void *index){
	int ind, j;
	bridges * my_bridge;

	ind = *((int*) index);
	my_bridge = malloc(sizeof(bridges));/*allocating memory for a bridges structure*/
	if(my_bridge == NULL) {
		printf(RED"Malloc error\n");
		printf(WHITE"\n");
		exit(0);
	}

	for(j = 0; j < 2; j++) { /*copying of bridge name from table*/
		my_bridge->b_name[j] = bridges_table[ind][j];
	}
	my_bridge->port = DEFAULT_PORT + atoi(&bridges_table[ind][1]);
	for(j = 2; j < (int)strlen(bridges_table[ind]) && ind < MAX_NUM_BRIDGES; j++) { /*I start from the third character because the first and second are for the bridge name*/
		my_bridge->bridge_lan[j - 2].lan_name = bridges_table[ind][j];
		my_bridge->bridge_lan[j - 2].port = DEFAULT_PORT + (int)(bridges_table[ind][j]); /*to set the same port for every lan, I use the casting to int of the lan name*/
		my_bridge->bridge_lan[j - 2].is_on = 1; /*at the beginning all ports are on*/
		my_bridge->bridge_lan[j - 2].is_designed= 1; /*at the beginning all ports are on*/
		my_bridge->bridge_lan[j - 2].msg = malloc(sizeof(mes));
		if(my_bridge->bridge_lan[j - 2].msg == NULL) {
			printf(RED"Malloc error\n");
			printf(WHITE"\n");
			exit(0);
		}

		my_bridge->bridge_lan[j - 2].msg->hop = 0;
		strncpy(my_bridge->bridge_lan[j - 2].msg->root, my_bridge->b_name, 2); /*at the beginning every bridge believes to be the root and it puts this information in each lan*/
		strncpy(my_bridge->bridge_lan[j - 2].msg->src_name, my_bridge->b_name, 2);

	}
	strncpy(my_bridge->msg.src_name, my_bridge->b_name, 2);
	strncpy(my_bridge->msg.root, my_bridge->b_name, 2); /*at the beginning every bridge believes to be the root*/

	my_bridge->msg.hop = 0; /*first messsage says that the sender of this is root, so hop = 0*/
	bridge(my_bridge); /*launching the bridge*/
	bridges_Sync(my_bridge); /*before dead, every bridge has to be synchronized*/
	j = 0;
	while(my_bridge->bridge_lan[j].port && j < MAX_NUM_LANS) { /*thread before death frees memory busy from each msg of lan*/
		if(my_bridge->bridge_lan[j].msg != NULL)
			free(my_bridge->bridge_lan[j].msg);
		j++;
	}
	if(my_bridge != NULL)
		free(my_bridge); /* thread before death frees memory busy by bridge structure*/
	pthread_exit(index);
}

void
*lans_Thread(void *index) {
	int ind, i;
	lans *my_lan;

	ind = *((int*) index);
	my_lan = malloc(sizeof(lans)); /*allocating memory for a lans structure*/
	if(my_lan == NULL) {
		printf(RED"Malloc error\n");
		printf(WHITE"\n");
		exit(0);
	}

	my_lan->l_name = lans_table_root[ind].lan_name; /*copying of lan name from lans table*/
	my_lan->port = DEFAULT_PORT + (int)lans_table_root[ind].lan_name;
	i = 0;
	while(lans_table_root[ind].bridges_name[i][0] && i < MAX_NUM_BRIDGES) {
		/*copying of bridges name in lan structure*/
		my_lan->lan_bridge[i].b_name[0] = lans_table_root[ind].bridges_name[i][0];
		my_lan->lan_bridge[i].b_name[1] = lans_table_root[ind].bridges_name[i][1];
		my_lan->lan_bridge[i].is_on = 1; /*at the beginning every bridge port is on*/
		my_lan->lan_bridge[i].port = DEFAULT_PORT + atoi(&lans_table_root[ind].bridges_name[i][1]); /*to set the same port for every bridge, I use the second character of bridge name*/
		i++;
	}
	lan(my_lan); /*launching the lan*/
	lans_Sync(my_lan); /*before dead, every lan has to be synchronized*/
	if(my_lan!= NULL)
		free(my_lan); /* thread before death frees memory busy by bridge structure*/
	pthread_exit(index);
}

void
* create_bridges_threads(void *b_threads) {
	int t, ris, *p;
	void *ptr;
	int error;
	pthread_t *bridges_threads = (pthread_t*)b_threads;
	for(t = 0; t < n_bridges ; t++) { /*allocating memory for thread paramaters*/
		p = malloc(sizeof(int));
		if(p == NULL) {
			perror(RED"Malloc for the index of a bridge thread failed");
			printf(WHITE"\n");
			pthread_exit(NULL);
		}
		*p = t; /*passing index value to threads*/
		ris = pthread_create(&bridges_threads[t], NULL, bridges_Thread, p); /*creating threads*/

		if (ris){/* check if pthread_create was good*/
			printf("ERROR; return code from pthread_create() is %d\n",ris);
			exit(-1);
		}
	}
	for(t = 0; t < n_bridges; t++) {/*this is for joining every thread*/

		/* waiting for the end of thread-nd */
		error = pthread_join(bridges_threads[t], (void*) &ptr);
		if(error != 0){
			printf("pthread_join() failed: error= %d\n", error);
			exit(-1);
		} else {
			printf(BLUE"bridge thread %d is dying\n", *((int*)ptr));
			free(ptr); /*deallocating structure in which pthread_join returns value*/
		}
	}
	pthread_mutex_destroy(&bridges_sync_lock);
	pthread_cond_destroy(&bridges_sync_cond);
	pthread_exit(NULL);/*exit of father thread*/
}

void
* create_lans_threads(void *l_threads) {
	int t, ris, *p, error;
	void *ptr;

	pthread_t *lans_threads = (pthread_t*)l_threads;
	for( t = 0; t < n_lans; t++) { /*allocating memory for thread parameters*/
		p = malloc(sizeof(int));
		if(p == NULL) {
			perror(RED"malloc for lans bridges failed: ");
			pthread_exit(NULL);
		}
		*p = t; /*passing index value to thread*/
		ris = pthread_create(&lans_threads[t], NULL, lans_Thread, p); /*creating thread*/

		if (ris) { /*check if pthread_create was good*/
			printf(RED"ERROR; reurn code from pthread_create() is %d\n", ris);
			exit(-1);
		}
	}
	for(t = 0; t < n_lans; t++) { /*this is for joining every thread*/

		/*waiting for the end of thread-nd*/
		error = pthread_join(lans_threads[t], (void*) &ptr);
		if(error != 0){
			printf(RED"pthread_join() failed: error= %d\n", error);
			exit(-1);
		} else {
			printf(VIOLET"lan thread %d is dying\n", *((int*)ptr));
			free(ptr); /*deallocating structure in which pthread_join returns value*/
		}
	}
	pthread_mutex_destroy(&lans_sync_lock);
	pthread_cond_destroy(&lans_sync_cond);

	pthread_exit (NULL); /*exit of father thread*/
}
