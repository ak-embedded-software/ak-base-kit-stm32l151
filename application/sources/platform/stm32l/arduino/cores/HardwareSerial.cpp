/*
  HardwareSerial.cpp - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  Modified 23 November 2006 by David A. Mellis
  Modified 28 September 2010 by Mark Sproul
  Modified 14 August 2012 by Alarus
  Modified 3 December 2013 by Matthijs Kooijman
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "sys_ctrl.h"

#include "Arduino.h"
#include "HardwareSerial.h"

// Public Methods //////////////////////////////////////////////////////////////
void HardwareSerial::_rx_complete_irq(char c) {
	rx_buffer_index_t i = (unsigned int)(_rx_buffer_head + 1) % SERIAL_RX_BUFFER_SIZE;

	// if we should be storing the received character into the location
	// just before the tail (meaning that the head would advance to the
	// current location of the tail), we're about to overflow the buffer
	// and so we don't write the character or advance the head.
	if (i != _rx_buffer_tail) {
		_rx_buffer[_rx_buffer_head] = c;
		_rx_buffer_head = i;
	}
}

int HardwareSerial::_tx_empty_irq(char* c) {
	// If interrupts are enabled, there must be more data in the output
	// buffer. Send the next byte
	*c = _tx_buffer[_tx_buffer_tail];
	_tx_buffer_tail = (_tx_buffer_tail + 1) % SERIAL_TX_BUFFER_SIZE;

	if (_tx_buffer_head == _tx_buffer_tail) {
		// Buffer empty, so disable interrupts
		return 0;
	}

	return 1;
}

void HardwareSerial::begin() {
	_pf_init();
}

int HardwareSerial::available(void) {
	return ((unsigned int)(SERIAL_RX_BUFFER_SIZE + _rx_buffer_head - _rx_buffer_tail)) % SERIAL_RX_BUFFER_SIZE;
}

int HardwareSerial::peek(void) {
	if (_rx_buffer_head == _rx_buffer_tail) {
		return -1;
	} else {
		return _rx_buffer[_rx_buffer_tail];
	}
}

int HardwareSerial::read(void) {
	// if the head isn't ahead of the tail, we don't have any characters
	if (_rx_buffer_head == _rx_buffer_tail) {
		return -1;
	} else {
		unsigned char c = _rx_buffer[_rx_buffer_tail];
		_rx_buffer_tail = (rx_buffer_index_t)(_rx_buffer_tail + 1) % SERIAL_RX_BUFFER_SIZE;
		return c;
	}
}

int HardwareSerial::availableForWrite(void) {
	tx_buffer_index_t head;
	tx_buffer_index_t tail;

	ENTRY_CRITICAL();
	head = _tx_buffer_head;
	tail = _tx_buffer_tail;
	EXIT_CRITICAL();

	if (head >= tail) return SERIAL_TX_BUFFER_SIZE - 1 - head + tail;
	return tail - head - 1;
}

void HardwareSerial::flush() {

}

size_t HardwareSerial::write(uint8_t c) {
	bool _flag_trigger_putc = false;

	if (_tx_buffer_head == _tx_buffer_tail) {
		_flag_trigger_putc = true;
	}

	tx_buffer_index_t i = (_tx_buffer_head + 1) % SERIAL_TX_BUFFER_SIZE;
	_tx_buffer[_tx_buffer_head] = c;

	// make atomic to prevent execution of ISR between setting the
	// head pointer and setting the interrupt flag resulting in buffer
	// retransmission
	ENTRY_CRITICAL();
	_tx_buffer_head = i;
	EXIT_CRITICAL();

	if (_flag_trigger_putc) {
		_pf_tringger_putc();
	}

	return 1;
}
