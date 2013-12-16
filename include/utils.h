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

/* numbers to define what is sending/receiving*/
#define BRIDGE 0
#define LAN 1

/*colors*/
#define VIOLET "\x1B[35m"
#define GREEN "\033[0;0;32m"
#define WHITE   "\033[0m"
#define RED "\033[0;0;31m"
#define BLUE "\033[0;0;34m"
#define ORANGE "\033[0;0;33m"
#define CYAN  "\x1B[36m"
#define YELLOW  "\x1B[33m"

void print_recv(int what, char *name, int socket, char *msg, int port);
void print_send(int what, char *name, int socket, char *msg, int port);
void print_table(bridges *my_bridge, mes *tmp, int what, int index, int debug);
void * finish(void * null);
