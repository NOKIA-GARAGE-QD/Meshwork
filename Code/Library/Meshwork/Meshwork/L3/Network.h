/**
 * This file is part of the Meshwork project.
 *
 * Copyright (C) 2013, Sinisha Djukic
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 */
#ifndef __MESHWORK_L3_NETWORK_H__
#define __MESHWORK_L3_NETWORK_H__

#include "Cosa/Wireless.hh"
#include "Cosa/Types.h"
#include "Cosa/Power.hh"

//#define SUPPORT_DELIVERY_ROUTED_DISABLE
//#define SUPPORT_DELIVERY_FLOOD_DISABLE
//#define SUPPORT_REROUTING_DISABLE

#ifndef SUPPORT_DELIVERY_ROUTED_DISABLE
#define SUPPORT_DELIVERY_ROUTED
#endif

#ifndef SUPPORT_DELIVERY_FLOOD_DISABLE
#define SUPPORT_DELIVERY_FLOOD
#endif

#ifndef SUPPORT_REROUTING_DISABLE
#define SUPPORT_REROUTING
#endif

//Meshwork::L3::Network
class Meshwork {
public:

  class L3 {
  public:
  
	  class Network {
	  public:

		class ACKProvider {
		public:
		  //returns 0 for no payload or number of bytes written in the payload buffer
		  virtual int returnACKPayload(uint8_t src, uint8_t port, void* buf, uint8_t len, void* bufACK, size_t lenACK) = 0;
		};//end of Meshwork::L3::Network::ACKProvider
		
		// Define node roles.
		static const uint8_t ROLE_ROUTER_NODE = 0x00;
		static const uint8_t ROLE_EDGE_NODE = 0x01;
		static const uint8_t ROLE_CONTROLLER_NODE = 0x03;
		static const uint8_t ROLE_GATEWAY_NODE = 0x03;

		// Defines network capabilities.
		static const uint8_t NWKCAPS_ALWAYS_LISTENING 	= 0x01;//always reachable
		static const uint8_t NWKCAPS_PERIODIC_WAKEUP 	= 0x02;//sleeps but wakes up periodically
		static const uint8_t NWKCAPS_ALWAYS_SLEEPING 	= 0x03;//never wakes up, only sends msgs
		static const uint8_t NWKCAPS_ROUTER 			= 0x04;//routing node
		static const uint8_t NWKCAPS_EDGE 				= 0x08;//edge node

		//define delivery methods
		/** Defines direct delivery. */
		static const uint8_t DELIVERY_DIRECT = 0x01;
		/** Defines routed message delivery. */
	#ifdef SUPPORT_DELIVERY_ROUTED
		static const uint8_t DELIVERY_ROUTED = 0x02;
	#ifdef SUPPORT_DELIVERY_FLOOD
		/** Defines flood routing message delivery. */
		static const uint8_t DELIVERY_FLOOD = 0x04;
	#endif
	#endif
		/** Defines exhaustive delivery approach. */
		static const uint8_t DELIVERY_EXHAUSTIVE =
								DELIVERY_DIRECT
	#ifdef SUPPORT_DELIVERY_ROUTED
								| DELIVERY_ROUTED
	#ifdef SUPPORT_DELIVERY_FLOOD
								| DELIVERY_FLOOD
	#endif
	#endif
								;

		//Positive values define success
		//Negative values define errors
		//Zero value undefined?
		
		//OK code group
		/** Message sent/received correctly. */
		static const int8_t OK = 1;
		/** Internal message received and can be ignored. */
		static const int8_t OK_MESSAGE_INTERNAL = 2;
		/** Message irrelevant for this node has been received and can be ignored. */
		static const int8_t OK_MESSAGE_IGNORED = 3;
		/** Warning, ACK message too long. */
		static const int8_t OK_WARNING_ACK_TOO_LONG = 4;
		
		//Wrong parameters code group
		/** Unknown delivery invalid or incompatible with destination. */
		static const int8_t ERROR_DELIVERY_METHOD_INVALID = -10;
		/** Payload to long for delivery. */
		static const int8_t ERROR_PAYLOAD_TOO_LONG = -11;
		/** Retry count is invalid. */
		static const int8_t ERROR_INVALID_RETRY_COUNT = -12;
		
		//Internal network errors code group
		/** Message ignored due to max hops reached. */
		static const int8_t ERROR_MESSAGE_IGNORED_MAX_HOPS_REACHED = -20;
		
		//ACK errors code group
		/** No acknowledge received. */
		static const int8_t ERROR_ACK_NOT_RECEIVED = -30;
		/** Sending an ACK has failed. */
		static const int8_t ERROR_ACK_SEND_FAILED = -31;
		
		//Routing errors code group
		/** No known routes to the destination. */
		static const int8_t ERROR_NO_KNOWN_ROUTES = -40;
		/** Rerouting a message has failed. */
		static const int8_t ERROR_REROUTE_FAILED = -41;

		//protected fields
		protected:
			Wireless::Driver* m_driver;
			uint8_t m_nwkcaps;
			uint8_t m_delivery;
			uint8_t m_retry;

		//public constructor and functions
		public:

			Network(Wireless::Driver* driver,
						uint8_t nwkcaps = ROLE_ROUTER_NODE,
							uint8_t delivery = DELIVERY_EXHAUSTIVE,
									uint8_t retry = 2) :
			  m_driver(driver),
			  m_nwkcaps(nwkcaps),
			  m_delivery(delivery),
			  m_retry(retry) {}

			Wireless::Driver* get_driver() {
			  return m_driver;
			}

			virtual bool begin(const void* config = NULL) {
				return m_driver == NULL ? false : m_driver->begin();
			}

			virtual bool end() {
				return m_driver == NULL ? false : m_driver->end();
			}

			//main send method
			virtual int send(uint8_t delivery, uint8_t retry,
								uint8_t dest, uint8_t port,
								const void* buf, size_t len,
								void* bufACK, size_t& lenACK);

			//convenience send method
			int send(uint8_t dest, uint8_t port,
						const void* buf, size_t len,
						void* bufACK, size_t& maxACKLen) {
				return send(m_delivery, m_retry, dest, port, buf, len, bufACK, maxACKLen);
			}
			
			//convenience broadcast method
			virtual int broadcast(uint8_t port, const void* buf, size_t len) {
				size_t none = 0;
				return send(Wireless::Driver::BROADCAST, port, buf, len, NULL, none);
			}

			//main recv method
			virtual int recv(uint8_t& src, uint8_t& port, void* data, size_t& dataLenMax,
					uint32_t ms, Meshwork::L3::Network::ACKProvider* ackProvider);
		};//end of Meshwork::L3::Network
	};//end of Meshwork::L3
};//end of Meshwork
#endif