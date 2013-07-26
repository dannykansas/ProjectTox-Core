/* friend_requests.c
 * 
 * Handle friend requests.
 * 
 *  Copyright (C) 2013 Tox project All Rights Reserved.
 *
 *  This file is part of Tox.
 *
 *  Tox is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Tox is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Tox.  If not, see <http://www.gnu.org/licenses/>.
 *  
 */

#include "friend_requests.h"

uint8_t self_public_key[crypto_box_PUBLICKEYBYTES];

/* Try to send a friendrequest to peer with public_key
   data is the data in the request and length is the length. 
   return -1 if failure.
   return  0 if it sent the friend request directly to the friend.
   return the number of peers it was routed through if it did not send it directly.*/
int send_friendrequest(uint8_t * public_key, uint8_t * data, uint32_t length)
{
    uint8_t packet[MAX_DATA_SIZE];
    int len = create_request(packet, public_key, data, length, 32); /* 32 is friend request packet id */
    if(len == -1)
    {
        return -1;
    }
    IP_Port ip_port = DHT_getfriendip(public_key);
    if(ip_port.ip.i == 1)
    {
        return -1;
    }
    if(ip_port.ip.i != 0)
    {
        if(sendpacket(ip_port, packet, len) != -1)
        {
            return 0;
        }
        return -1;
    }
    
    int num = route_tofriend(public_key, packet, len);
    if(num == 0)
    {
        return -1;
    }
    return num;
}

static void (*handle_friendrequest)(uint8_t *, uint8_t *, uint16_t);
static uint8_t handle_friendrequest_isset = 0;

/* set the function that will be executed when a friend request is received. */
void callback_friendrequest(void (*function)(uint8_t *, uint8_t *, uint16_t))
{
    handle_friendrequest = function;
    handle_friendrequest_isset = 1;
}

int friendreq_handlepacket(uint8_t * packet, uint32_t length, IP_Port source)
{

    if(packet[0] == 32)
    {
        if(length <= crypto_box_PUBLICKEYBYTES * 2 + crypto_box_NONCEBYTES + 1 + ENCRYPTION_PADDING &&
        length > MAX_DATA_SIZE + ENCRYPTION_PADDING)
        {
            return 1;
        }
        if(memcmp(packet + 1, self_public_key, crypto_box_PUBLICKEYBYTES) == 0)//check if request is for us.
        {
            if(handle_friendrequest_isset == 0)
            {
                return 1;
            }
            uint8_t public_key[crypto_box_PUBLICKEYBYTES];
            uint8_t data[MAX_DATA_SIZE];
            int len = handle_request(public_key, data, packet, length);
            if(len == -1)
            {
                return 1;
            }
            (*handle_friendrequest)(public_key, data, len);
        }
        else//if request is not for us, try routing it.
        {
            if(route_packet(packet + 1, packet, length) == length)
            {
                return 0;
            }
        }
    }
        return 1;
}