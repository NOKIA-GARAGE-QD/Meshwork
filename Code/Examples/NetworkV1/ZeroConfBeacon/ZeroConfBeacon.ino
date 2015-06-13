/**
 * This file is part of the Meshwork project.
 *
 * Copyright (C) 2014, Sinisha Djukic
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
#ifndef __EXAMPLES_ZEROCONFBEACON_H__
#define __EXAMPLES_ZEROCONFBEACON_H__

//First, include the configuration constants file
#include "MeshworkConfiguration.h"



//All build-time configuration, RF selection, EEPROM usage,
//Route Cache table, etc. is defined in a single place here
#include "Config.h"



///////////////////////////////////////////////////////////////////////////////
////////////////////////// SECTION: INCLUDES AND USES /////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <Cosa/EEPROM.hh>
#include <Cosa/Trace.hh>
#include <Cosa/Types.h>
#include <Cosa/IOStream.hh>
#include <Cosa/IOStream/Driver/UART.hh>
#include <Cosa/Watchdog.hh>
#include <Cosa/OutputPin.hh>
#include <Cosa/RTC.hh>
#include <Cosa/Wireless.hh>

#include <Meshwork.h>
#include <Meshwork/L3/Network.h>
#include <Meshwork/L3/NetworkV1/NetworkV1.h>
#include <Meshwork/L3/NetworkV1/NetworkV1.cpp>

#if EX_LED_TRACING
	#include "Utils/LEDTracing.h"
#endif

#if ( MW_RF_SELECT == MW_RF_NRF24L01P )
	#include <NRF24L01P/NRF24L01P.hh>
	#include <NRF24L01P/NRF24L01P.cpp>
#endif

#if ( ( MW_ROUTECACHE_SELECT == MW_ROUTECACHE_RAM ) || ( MW_ROUTECACHE_SELECT == MW_ROUTECACHE_PERSISTENT ) )
	#include <Meshwork/L3/NetworkV1/RouteCache.h>
	#include <Meshwork/L3/NetworkV1/RouteCache.cpp>
	#include <Meshwork/L3/NetworkV1/CachingRouteProvider.h>
	#if ( MW_ROUTECACHE_SELECT == MW_ROUTECACHE_PERSISTENT )
		#include <Meshwork/L3/NetworkV1/RouteCachePersistent.h>
	#endif
#endif

#include <Meshwork/L3/NetworkV1/ZeroConfSerial/ZeroConfSerial.h>
#include <Meshwork/L3/NetworkV1/ZeroConfSerial/ZeroConfSerial.cpp>
#include <Meshwork/L3/NetworkV1/ZeroConfSerial/ZeroConfPersistent.h>

#include <Utils/SerialMessageAdapter.h>
#include <Utils/SerialMessageAdapter.cpp>

using namespace Meshwork::L3::NetworkV1;


///////////////////////////////////////////////////////////////////////////////
////////////////////////// SECTION: MEMBER DECLARATION ////////////////////////
///////////////////////////////////////////////////////////////////////////////

//Our EEPROM instance
EEPROM eeprom;

//RF and Mesh
MW_DECL_NRF24L01P(rf)
#if ( MW_ROUTECACHE_SELECT == MW_ROUTECACHE_PERSISTENT )
	MW_DECLP_ROUTEPROVIDER_ROUTECACHE_PERSISTENT(routeprovider, eeprom, EX_ROUTECACHE_TABLE_EEPROM_OFFSET)
#elif ( MW_ROUTECACHE_SELECT == MW_ROUTECACHE_RAM )
	MW_DECLP_ROUTEPROVIDER_ROUTECACHE_RAM(routeprovider)
#else // MW_ROUTECACHE_NONE
	MW_DECLP_ROUTEPROVIDER_ROUTECACHE_NONE(routeprovider)
#endif
NetworkV1 mesh(&rf, routeprovider, NetworkV1::NWKCAPS_NONE);

//Tracing LEDs
#if EX_LED_TRACING
	MW_DECL_LEDTRACING(ledTracing, mesh, EX_LED_TRACING_SEND, EX_LED_TRACING_RECV, EX_LED_TRACING_ACK)
#endif

MW_DECL_ZEROCONF_PERSISTENT(zeroConfPersistent, zeroConfConfiguration, eeprom, EX_ZC_CONFIGURATION_EEPROM_OFFSET)

//Setup extra UART on Mega
#if MW_BOARD_SELECT == MW_BOARD_MEGA
	#include "Cosa/IOBuffer.hh"
	// Create buffer for HC UART
	static IOBuffer<UART::BUFFER_MAX> ibuf;
	static IOBuffer<UART::BUFFER_MAX> obuf;
	// HC UART will be used for Host-Controller communication
	UART uartHC(3, &ibuf, &obuf);
	SerialMessageAdapter serialMessageAdapter(&uartHC);
#else
	SerialMessageAdapter serialMessageAdapter(&uart);
	IOStream::Device null_device;
#endif

//ZeroConf Serial support
ZeroConfSerial zeroConfSerial(&mesh, &serialMessageAdapter,
								&zeroConfConfiguration.sernum, &zeroConfConfiguration.reporting,
									&zeroConfConfiguration.nwkconfig, &zeroConfConfiguration.devconfig,
										&zeroConfPersistent);
SerialMessageAdapter::SerialMessageListener* serialMessageListeners[1];

//Bootup notification LED
#if defined(EX_LED_BOOTUP)
	OutputPin ledPin(EX_LED_BOOTUP);
	#define LED(state)	ledPin.set(state)
	#define LED_BLINK(state, x)	\
		LED(state); \
		Watchdog::delay(x); \
		LED(!state);
#else
	#define LED(state)	(void) (state)
	#define LED_BLINK(state, x)	(void) (state)
#endif

static const uint8_t 	BEACON_BCAST_PORT 	= 128;//0-127 are reserved for MW
static const char 		BEACON_BCAST_MSG[] 	= "*BEACON*";//some message
static const uint8_t	BEACON_BCAST_MSG_LEN = sizeof(BEACON_BCAST_MSG) -1;//without null termination

///////////////////////////////////////////////////////////////////////////////
////////////////////////// SECTION: BUSINESS LOGIC/CODE ///////////////////////
///////////////////////////////////////////////////////////////////////////////


//Read stored configuration
void readConfig() {
	trace << PSTR("[Config] Reading ZeroConf EEPROM...") << endl;
	zeroConfPersistent.init();
	zeroConfPersistent.read_configuration();

	mesh.setNetworkID(zeroConfConfiguration.nwkconfig.nwkid);
	mesh.setNodeID(zeroConfConfiguration.nwkconfig.nodeid);
	mesh.setNetworkKeyLen(zeroConfConfiguration.nwkconfig.nwkkeylen);
	mesh.setNetworkKey(zeroConfConfiguration.nwkconfig.nwkkey);
	mesh.setChannel(zeroConfConfiguration.nwkconfig.channel);
	mesh.setNetworkCaps(zeroConfConfiguration.devconfig.m_nwkcaps);
	mesh.setDelivery(zeroConfConfiguration.devconfig.m_delivery);

#if ( MW_ROUTECACHE_SELECT == MW_ROUTECACHE_PERSISTENT )
	trace << PSTR("[Config] Reading RouteCache EEPROM...") << endl;
	routecache_routeprovider.init();
	routecache_routeprovider.read_routes();
#endif

	trace << PSTR("[Config] Done") << endl;
}

//Setup sequence
void setup()
{
	//Basic setup
	Watchdog::begin();
	RTC::begin();
	
	//Enable UART for the boot-up config sequence. Blink once and keep lit during config
	LED_BLINK(true, 500);
	LED(true);
	uart.begin(115200);
	
	//Trace debugs only supported on Mega, since it has extra UARTs
#if MW_BOARD_SELECT == MW_BOARD_MEGA
	trace.begin(&uart, NULL);
	trace << PSTR("ZeroConf Beacon: started") << endl;
	uartHC.begin(115200);
#else
	trace.begin(&null_device, NULL);
#endif
	
	//Begin: Init some structures
	serialMessageListeners[0] = &zeroConfSerial;
	serialMessageAdapter.setListeners(serialMessageListeners);
	//End: Init some structures

	readConfig();

	trace << PSTR("Waiting for ZeroConfSerial connection...") << endl;
	//Allow some time for initial configuration
	bool reconfigured = zeroConfSerial.processConfigSequence(EX_STARTUP_AUTOCONFIG_INIT_TIMEOUT, EX_STARTUP_AUTOCONFIG_DEINIT_TIMEOUT, EX_SERIAL_NEXT_MSG_TIMEOUT);
	
	readConfig();

	trace << (reconfigured ? PSTR("New configuration applied") : PSTR("Previous configuration used")) << endl;// << PSTR("Closing serial") << endl;

	//Flush all chars
	uart.flush();
	//Don't disable the UART - for debugging purposes. Blink differently if reconfigured
	//uart.end();
	LED_BLINK(false, reconfigured ? 2000 : 500);
	LED(false);
	
#if EX_LED_TRACING
	mesh.set_radio_listener(&ledTracing);
#endif

	mesh.begin();
}

//Main loop
void loop()
{
	static size_t replySize = 0;
	static uint16_t msgcounter = 0;
	static uint16_t failcounter = 0;
	MW_LOG_DEBUG_TRACE(EX_LOG_ZEROCONFBEACON) << endl << PSTR("*** Beacon: Start ***") << endl;
	MW_LOG_DEBUG_TRACE(EX_LOG_ZEROCONFBEACON) << PSTR("Target Node ID: ") << zeroConfConfiguration.reporting.targetnodeid << endl;
	MW_LOG_DEBUG_TRACE(EX_LOG_ZEROCONFBEACON) << PSTR("Delivery flags: ") << zeroConfConfiguration.devconfig.m_delivery << endl;
	int result = mesh.send(zeroConfConfiguration.devconfig.m_delivery, -1, zeroConfConfiguration.reporting.targetnodeid, BEACON_BCAST_PORT,
			BEACON_BCAST_MSG, BEACON_BCAST_MSG_LEN, (void*) NULL, replySize);
	if ( result == Meshwork::L3::Network::OK )
		msgcounter ++;
	else
		failcounter ++;
	MW_LOG_DEBUG_TRACE(EX_LOG_ZEROCONFBEACON) << PSTR("*** Beacon: End ***") << endl;
	MW_LOG_DEBUG_TRACE(EX_LOG_ZEROCONFBEACON) << endl;
	MW_LOG_INFO(EX_LOG_ZEROCONFBEACON, "[Statistics] Send success=%d, failures=%d", msgcounter, failcounter);
	MW_LOG_DEBUG_TRACE(EX_LOG_ZEROCONFBEACON) << endl;
	Watchdog::delay(EX_BEACON_INTERVAL);
}

#endif
