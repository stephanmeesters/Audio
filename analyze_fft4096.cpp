/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <Arduino.h>
#include "analyze_fft4096.h"
#include "sqrt_integer.h"
#include "utility/dspinst.h"


// 140312 - PAH - slightly faster copy
static void copy_to_fft_buffer(void *destination, const void *source)
{
	const uint16_t *src = (const uint16_t *)source;
	uint32_t *dst = (uint32_t *)destination;

	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		*dst++ = *src++;  // real sample plus a zero for imaginary
	}
}

static void apply_window_to_fft_buffer(void *buffer)
{
	int16_t *buf = (int16_t *)buffer;
	const int16_t *win = (int16_t *)AudioWindowHanning4096;

	for (int i=0; i < 1024; i++) {
		int32_t val = *buf * *win++;
		//*buf = signed_saturate_rshift(val, 16, 15);
		*buf = val >> 15;
		buf += 2;
	}

}

void AudioAnalyzeFFT4096::update(void)
{
	audio_block_t *block;
	
	//Serial.println("update 4096");

	block = receiveReadOnly();
	if (!block)
	{
		//Serial.println("no block" );
		return;
	}

#if defined(KINETISK)

	// build up buffer to twice the FFT window length
	if(state < 31)
	{
		copy_to_fft_buffer(buffer+state*0x100, block);
		state++;
	}
	
	// buffer filled, process
	else
	{
		//blocklist[state] = block;
		
		copy_to_fft_buffer(buffer+state*0x100, block);
		
		if (bWindowFunction) apply_window_to_fft_buffer(buffer);
		
		//arm_rfft_q15(&arm_rfft_sR_q15_len4096, buffer, output);
		arm_cfft_q15(&arm_cfft_sR_q15_len4096, buffer, 0, 1);
		arm_cmplx_mag_q15(buffer, output, 2048);			// full fft size

		outputflag = true;
		
		arm_copy_q15(buffer+16*0x100, buffer, 4096); // half FFT size
		
		state = 16;
		
	}

#endif
	release(block);
//#endif
}


