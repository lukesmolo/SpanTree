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
#define MAX_NUM_BRIDGES 5
#define MAX_NUM_LANS 12

/*structure of message for a lan in a bridge*/
typedef struct mes {

	int hop;
	char src_name[2];
	char root[2];


} mes;

/*structure of a lan for a bridge*/
typedef struct b_lan {
	char lan_name; /* name of lan*/
	int port; /*port of bridge open to this lan*/
	mes *msg; /*msg for a lan*/
	int is_designed; /*is this bridge designed for this lan?*/
	int is_on; /*is this port opened?*/
} b_lan;

/*structure for a bridge*/
typedef struct bridges {
	char b_name[2]; /*bridge name*/
	int port; /*bridge port*/
	struct b_lan bridge_lan[12]; /*array of lan for a bridge*/
	mes msg; /*master msg for a bridge*/
} bridges;

/*structure of a bridge for a lan*/
typedef struct l_bridge {
	char b_name[2]; /*bridge name*/
	int port; /*bridge port*/
	int is_on; /*bridge flag to indicate if lan has to send to this bridge*/
} l_bridge;

/*structure for a lan*/
typedef struct lans {
	char l_name; /*lan name*/
	int port; /*lan port*/
	struct l_bridge lan_bridge[5]; /*array of bridges for a lan*/
} lans;

/*structure for data of lan table*/
typedef struct lans_table {
	char lan_name;
	char bridges_name[MAX_NUM_BRIDGES][2];
} lans_table;


extern pthread_t bridges_threads[MAX_NUM_BRIDGES];
extern pthread_t bridges_lans[MAX_NUM_LANS];
extern int n_bridges;
extern int n_lans;
extern int end;
extern struct timeval tv;

char bridges_table[MAX_NUM_BRIDGES][MAX_NUM_LANS + 2 ]; /*table in which we temporarily record information for bridges threads. First and second character are for the bridge name, so max number of lans plus 2*/
lans_table lans_table_root[MAX_NUM_LANS]; /*table in which we temporarily record information for lans threads*/


