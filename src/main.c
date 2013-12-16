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
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "main.h"
#include "net.h"
#include "threads.h"
#include "utils.h"


int n_bridges = 1;
int n_lans = 1;
int end = 1; /*global variable for the end of the program*/
struct timeval tv;


void
initialize_struct() {
	int i,j;
	for(i = 0; i < MAX_NUM_BRIDGES; i++) {
		for(j = 0; j < MAX_NUM_LANS; j++) {
			bridges_table[i][j] = 0;
		}
	}
	for(i = 0; i < MAX_NUM_LANS; i++) {
		lans_table_root[i].lan_name = 0;
		for(j = 0; j < (MAX_NUM_BRIDGES); j++) {
			lans_table_root[i].bridges_name[j][0] = 0;
			lans_table_root[i].bridges_name[j][1] = 0;
		}
	}
}


void
read_bridges() {

	FILE *config;		/* file declaration*/
	char ch;				/* char for file reading*/
	int i = 0;
	config = fopen("configuration", "rt"); /*opening of network configuration file*/
	if (config == NULL) {
		fprintf(stderr,RED "Can't open input file for network configuration!\n");
		exit(1);
	}
	while( ((ch = fgetc(config) ) != EOF && i <= MAX_NUM_LANS + 2)  ) { /*beginning of file*/
		switch(ch) {
			case '\n':
				n_bridges++; /*another bridge found*/
				i = 0;
				break;
			case '-':  /*lan for that bridge found*/
				break;
			case '|':
				break;
			default:
				bridges_table[n_bridges - 1][i] = ch; /*copying file information into my bridges_table*/
				i++;

		}
	}
	n_bridges--;/*I've not count the last while cycle*/
	if(n_bridges > MAX_NUM_BRIDGES) {
		printf("An error occurred: too many bridges! Closing ...\n");
		exit(0);
	}
	if(i > MAX_NUM_LANS + 2) {
		printf("An error occured: too many lans for bridge in %d° position. Closing...\n", n_bridges + 1);
		exit(0);
	}

	fclose(config); /*closing of configration file*/
	printf("Number of bridges is: %d\n", n_bridges);
	return;
}/*end of read_bridges()*/

int
lan_in_table(char lan) { /*this function returns the position of lan_table_root in which there's already a lan inserted*/
	int ind = -1;
	int i;
	for(i = 0; i < MAX_NUM_LANS; i++) {
		if(lans_table_root[i].lan_name && lans_table_root[i].lan_name == lan ) {
			ind = i;
			break;
		}
	}
	return ind;
}


void
read_lans() {
	int i,j;
	int ind = 0; /*position in the array in which there's that lan*/
	int free = 0; /*first free position of bridges_name in which we can store the new found name*/
	for(i = 0; i < n_bridges; i++) {
		for(j = 2; j < (int)strlen(bridges_table[i]) && j < MAX_NUM_LANS + 2; j++) { /*first and second are for bridges name*/
			ind = lan_in_table(bridges_table[i][j]);/* take the index of an existing lan*/
			if(ind != -1) {
				/*finding the first bridges_name free position*/
				free = 1;/*if there's already a lan. there's also a bridge for that lan*/
				while(lans_table_root[ind].bridges_name[free][0] && free < MAX_NUM_BRIDGES ) {/*  we find for an empty space*/
					free++; /* we're sure that there'll be an empty place because read_bridges didn't return error*/
				}
			} else {/*this lan is already not in memory*/
				ind = 0;
				free = 0;
				while(lans_table_root[ind].lan_name != 0 && ind <= MAX_NUM_LANS) {/*we find the first free position*/
					ind++;
				}
				if(ind >= MAX_NUM_LANS) { /*there was not space to store the lan name. ERROR!*/
					printf("An error occured while the storing of a lan name. There are too lans! Closing...\n");
					exit(0);
				}
				/*we store the name of lan in his position*/
				lans_table_root[ind].lan_name = bridges_table[i][j];

			}
			/*we store the bridge name in [ind][free] position*/
			lans_table_root[ind].bridges_name[free][0] = bridges_table[i][0];
			lans_table_root[ind].bridges_name[free][1] = bridges_table[i][1];
		}
	}
	i = 0;
	while(lans_table_root[i].lan_name) /*to count n_lans I've to count elements in the lans_table_root*/
		i++;
	n_lans = i;
	printf("The number of lans is: %d\n", n_lans);
}/* end of read_lans*/

int
main() {
	pthread_t bridges_threads[MAX_NUM_BRIDGES];
	pthread_t lans_threads[MAX_NUM_LANS];
	pthread_t all_threads[3];

	int i = 0;
	int ris;
	int error;
	void *ptr;

	initialize_struct();
	read_bridges(); /*reading file for bridges*/
	read_lans();/*reading file for lans*/

	/*creating threads to launch create_lans and create_bridges*/
	for(i = 0; i < 3; i++) {	
		if(i == 0)
			ris = pthread_create(&all_threads[i], NULL, create_lans_threads, ((void*)lans_threads));

		else if(i == 1)
			ris = pthread_create(&all_threads[i], NULL, create_bridges_threads, ((void*)bridges_threads));
		else
			ris = pthread_create(&all_threads[i], NULL, finish , NULL);

		if (ris){ /* check if pthread_create was good*/
			printf(RED"ERROR; return code from pthread_create() is %d\n",ris);
			exit(-1);
		}
	}

	for(i = 0; i < 3; i++) { /*this is for joining every thread*/

		/*waiting for the end of thread-nd*/
		error = pthread_join(all_threads[i], (void*) &ptr);
		if(error != 0){
			printf("pthread_join() failed: error= %d\n", error);
			exit(-1);
		}
	}
	printf(VIOLET"Goodbye!\n"WHITE);
	pthread_exit(NULL);
}/*end of main*/
