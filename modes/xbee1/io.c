/*
  libxbee - a C library to aid the use of Digi's XBee wireless modules
            running in API mode (AP=2).

  Copyright (C) 2009  Attie Grande (attie@attie.co.uk)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../internal.h"
#include "../../xbee_int.h"
#include "../../mode.h"
#include "../../pkt.h"
#include "../../log.h"
#include "../common.h"
#include "io.h"

xbee_err xbee_s1_io_parseInputs(struct xbee *xbee, struct xbee_pkt *pkt, unsigned char *data, int len) {
	int sampleCount;
	int sample, channel;
	int ioMask;

	sampleCount = data[0];
	data++;

	ioMask = ((data[0] << 8) & 0xFF00) | (data[1] & 0xFF);
	data += 2;
	for (sample = 0; sample < sampleCount; sample++) {
		int digitalValue;
		int mask;

		digitalValue = ((data[0] << 8) & 0x0100) | (data[1] & 0xFF);

		if (ioMask & 0x01FF) {
			mask = 0x0001;
			for (channel = 0; channel <= 8; channel++, mask <<= 1) {
				if (ioMask & mask) {
					if (xbee_pktDigitalAdd(pkt, channel, digitalValue & mask)) {
						xbee_log(1,"Failed to add digital sample information to packet (channel D%d)", channel);
					}
				}
			}
			data += 2;
		}

		mask = 0x0200;
		for (channel = 0; channel <= 5; channel++, mask <<= 1) {
			if (ioMask & mask) {
				if (xbee_pktAnalogAdd(pkt, channel, ((data[0] << 8) & 0x3F) | (data[1] & 0xFF))) {
					xbee_log(1,"Failed to add analog sample information to packet (channel A%d)", channel);
				}
				data += 2;
			}
		}
	}

	return XBEE_ENONE;
}

/* ######################################################################### */

xbee_err xbee_s1_io_rx_func(struct xbee *xbee, unsigned char identifier, struct xbee_buf *buf, struct xbee_frameInfo *frameInfo, struct xbee_conAddress *address, struct xbee_pkt **pkt) {
	struct xbee_pkt *iPkt;
	xbee_err ret;
	int addrLen;
	
	if (!xbee || !frameInfo || !buf || !address || !pkt) return XBEE_EMISSINGPARAM;
	
	if (buf->len < 1) return XBEE_ELENGTH;
	
	switch (buf->data[0]) {
		case 0x82: addrLen = 8; break;
		case 0x83: addrLen = 2; break;
		default: return XBEE_EINVAL;
	}
	
	if (buf->len < addrLen + 3) return XBEE_ELENGTH;
	
	if (addrLen == 8) {
		address->addr64_enabled = 1;
		memcpy(address->addr64, &(buf->data[1]), addrLen);
	} else {
		address->addr16_enabled = 1;
		memcpy(address->addr16, &(buf->data[1]), addrLen);
	}
	
	if ((ret = xbee_pktAlloc(&iPkt, NULL, buf->len - (addrLen + 3))) != XBEE_ENONE) return ret;
	
	iPkt->rssi = buf->data[addrLen + 1];
	iPkt->settings = buf->data[addrLen + 2];
	
	iPkt->dataLen = buf->len - (addrLen + 3);
	if (iPkt->dataLen > 0) {
		memcpy(iPkt->data, &(buf->data[addrLen + 3]), iPkt->dataLen);
	}
	iPkt->data[iPkt->dataLen] = '\0';
	
	xbee_s1_io_parseInputs(xbee, iPkt, iPkt->data, iPkt->dataLen);
	
	*pkt = iPkt;
	
	return XBEE_ENONE;
}

/* ######################################################################### */

const struct xbee_modeDataHandlerRx xbee_s1_16bitIo_rx  = {
	.identifier = 0x83,
	.func = xbee_s1_io_rx_func,
};
const struct xbee_modeConType xbee_s1_16bitIo = {
	.name = "16-bit I/O",
	.allowFrameId = 0,
	.useTimeout = 0,
	.rxHandler = &xbee_s1_16bitIo_rx,
	.txHandler = NULL,
};

/* ######################################################################### */

const struct xbee_modeDataHandlerRx xbee_s1_64bitIo_rx  = {
	.identifier = 0x82,
	.func = xbee_s1_io_rx_func,
};
const struct xbee_modeConType xbee_s1_64bitIo = {
	.name = "64-bit I/O",
	.allowFrameId = 0,
	.useTimeout = 0,
	.rxHandler = &xbee_s1_64bitIo_rx,
	.txHandler = NULL,
};