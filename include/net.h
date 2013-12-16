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
#define SOCKET_ERROR   ((int)-1)
#define SIZEBUF 100000L
#define DEFAULT_PORT 60000/*this is the base on which other ports are created*/

int bridge_thread_send(bridges *my_bridge, int socketfd, mes *msg, int *tmp_port, int *forward);
mes * bridge_thread_recv(bridges *my_bridge, int socketfd, int *tmp_port, int *forward);
int lan_thread_send(lans *my_lan, int socketfd, mes *msg, int *tmp_port);
mes * lan_thread_recv(lans *my_lan, int socketfd, int *tmp_port);

int bridge(bridges *my_bridge);
int lan(lans *my_lan);


