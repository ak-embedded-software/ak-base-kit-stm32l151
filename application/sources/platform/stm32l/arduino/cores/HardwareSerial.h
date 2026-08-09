/*
  HardwareSerial.h - Hardware serial library for Wiring
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

  Modified 28 September 2010 by Mark Sproul
  Modified 14 August 2012 by Alarus
  Modified 3 December 2013 by Matthijs Kooijman
*/

#ifndef HardwareSerial_h
#define HardwareSerial_h
#include <stdint.h>
#include "Stream.h"


#define SERIAL_RX_BUFFER_SIZE 256
#define SERIAL_TX_BUFFER_SIZE 256

#if (SERIAL_TX_BUFFER_SIZE>256)
typedef uint16_t tx_buffer_index_t;
#else
typedef uint8_t tx_buffer_index_t;
#endif
#if  (SERIAL_RX_BUFFER_SIZE>256)
typedef uint16_t rx_buffer_index_t;
#else
typedef uint8_t rx_buffer_index_t;
#endif

class HardwareSerial : public Stream {
private:
	typedef void (*pf_init)();
	typedef void (*pf_tringger_putc)();

	pf_init _pf_init;
	pf_tringger_putc _pf_tringger_putc;

	// Has any byte been written to the UART since begin()
	//bool _written;

	volatile rx_buffer_index_t _rx_buffer_head;
	volatile rx_buffer_index_t _rx_buffer_tail;
	volatile tx_buffer_index_t _tx_buffer_head;
	volatile tx_buffer_index_t _tx_buffer_tail;

	// Don't put any members after these buffers, since only the first
	// 32 bytes of this struct can be accessed quickly using the ldd
	// instruction.
	unsigned char _rx_buffer[SERIAL_RX_BUFFER_SIZE];
	unsigned char _tx_buffer[SERIAL_TX_BUFFER_SIZE];

public:
	HardwareSerial() {
		_rx_buffer_head = 0;
		_rx_buffer_tail = 0;
		_tx_buffer_head = 0;
		_tx_buffer_tail = 0;
	}

	HardwareSerial(pf_init __pf_init, pf_tringger_putc __pf_tringger_putc) {
		_pf_init = __pf_init;
		_pf_tringger_putc = __pf_tringger_putc;

		_rx_buffer_head = 0;
		_rx_buffer_tail = 0;
		_tx_buffer_head = 0;
		_tx_buffer_tail = 0;
	}

	void begin();
	virtual int available(void);
	virtual int peek(void);
	virtual int read(void);
	virtual int availableForWrite(void);
	virtual void flush(void);
	virtual size_t write(uint8_t);
	inline size_t write(unsigned long n) { return write((uint8_t)n); }
	inline size_t write(long n) { return write((uint8_t)n); }
	inline size_t write(unsigned int n) { return write((uint8_t)n); }
	inline size_t write(int n) { return write((uint8_t)n); }
	using Print::write; // pull in write(str) and write(buf, size) from Print
	operator bool() { return true; }

	// Interrupt handlers - Not intended to be called externally
	void _rx_complete_irq(char c);
	int _tx_empty_irq(char* c);
};

extern HardwareSerial Serial2;

#endif
