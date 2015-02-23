/*-
 * mdc_decode.c
 *   Decodes a specific format of 1200 BPS XOR-precoded MSK data burst
 *   from input audio samples.
 *
 * Author: Matthew Kaufman (matthew@eeph.com)
 *
 * Copyright (c) 2005, 2010  Matthew Kaufman  All rights reserved.
 * 
 *  This file is part of Matthew Kaufman's MDC Encoder/Decoder Library
 *
 *  The MDC Encoder/Decoder Library is free software; you can
 *  redistribute it and/or modify it under the terms of version 2 of
 *  the GNU General Public License as published by the Free Software
 *  Foundation.
 *
 *  If you cannot comply with the terms of this license, contact
 *  the author for alternative license arrangements or do not use
 *  or redistribute this software.
 *
 *  The MDC Encoder/Decoder Library is distributed in the hope
 *  that it will be useful, but WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 *  USA.
 *
 *  or see http://www.gnu.org/copyleft/gpl.html
 *
-*/

#include <stdlib.h>
#include "mdc_decode.h"
#include "mdc_common.c"

mdc_decoder_t * mdc_decoder_new(int sampleRate)
{
	mdc_decoder_t *decoder;
	mdc_int_t i;

	decoder = (mdc_decoder_t *)malloc(sizeof(mdc_decoder_t));
	if(!decoder)
		return (mdc_decoder_t *) 0L;

//	decoder->hyst = 3.0/256.0; - deprecated (zerocrossing)
//	decoder->incr = (1200.0 * TWOPI) / ((mdc_float_t)sampleRate);

	if(sampleRate == 8000)
	{
		decoder->incru = 644245094;
	} else if(sampleRate == 16000)
	{
		decoder->incru = 322122547;
	} else if(sampleRate == 22050)
	{
		decoder->incru = 233739716;
	} else if(sampleRate == 32000)
	{
		decoder->incru = 161061274;
	} else if(sampleRate == 44100)
	{
		decoder->incru = 116869858;
	} else if(sampleRate == 48000)
	{
		decoder->incru = 107374182;
	} else
	{
		// WARNING: lower precision than above
		decoder->incru = 1200 * 2 * (0x80000000 / sampleRate);
	}

	decoder->good = 0;
	decoder->indouble = 0;
	decoder->level = 0;


	for(i=0; i<MDC_ND; i++)
	{
		// decoder->du[i].th = 0.0 + ( ((mdc_float_t)i) * (TWOPI/(mdc_float_t)MDC_ND));
		decoder->du[i].thu = i * 2 * (0x80000000 / MDC_ND);
#ifdef PLL
		decoder->du[i].plt =  decoder->du[i].thu;
#endif
		// decoder->du[i].zc = 0; - deprecated
		decoder->du[i].xorb = 0;
		decoder->du[i].invert = 0;
		decoder->du[i].shstate = -1;
		decoder->du[i].shcount = 0;
	#ifdef FOURPOINT
		decoder->du[i].nlstep = i;
	#endif
	}

	decoder->callback = (mdc_decoder_callback_t)0L;

	return decoder;
}

static void _clearbits(mdc_decoder_t *decoder, mdc_int_t x)
{
	mdc_int_t i;
	for(i=0; i<112; i++)
		decoder->du[x].bits[i] = 0;
}




static void _procbits(mdc_decoder_t *decoder, int x)
{
	mdc_int_t lbits[112];
	mdc_int_t lbc = 0;
	mdc_int_t i, j, k;
	mdc_u8_t data[14];
	mdc_u16_t ccrc;
	mdc_u16_t rcrc;

	for(i=0; i<16; i++)
	{
		for(j=0; j<7; j++)
		{
			k = (j*16) + i;
			lbits[lbc] = decoder->du[x].bits[k];
			++lbc;
		}
	}

	for(i=0; i<14; i++)
	{
		data[i] = 0;
		for(j=0; j<8; j++)
		{
			k = (i*8)+j;

			if(lbits[k])
				data[i] |= 1<<j;
		}
	}


	ccrc = _docrc(data, 4);
	rcrc = data[5] << 8 | data[4];

	if(ccrc == rcrc)
	{

		if(decoder->du[x].shstate == 2)
		{
			decoder->extra0 = data[0];
			decoder->extra1 = data[1];
			decoder->extra2 = data[2];
			decoder->extra3 = data[3];

			for(k=0; k<MDC_ND; k++)
				decoder->du[k].shstate = -1;

			decoder->good = 2;
			decoder->indouble = 0;

		}
		else
		{
			if(!decoder->indouble)
			{
				decoder->good = 1;
				decoder->op = data[0];
				decoder->arg = data[1];
				decoder->unitID = (data[2] << 8) | data[3];
				decoder->crc = (data[4] << 8) | data[5];
	

				switch(data[0])
				{
				/* list of opcode that mean 'double packet' */
				case 0x35:
				case 0x55:
					decoder->good = 0;
					decoder->indouble = 1;
					decoder->du[x].shstate = 2;
					decoder->du[x].shcount = 0;
					_clearbits(decoder, x);
					break;
				default:
					for(k=0; k<MDC_ND; k++)
						decoder->du[k].shstate = -1;	// only in the single-packet case, double keeps rest going
					break;
				}
			}
			else
			{
				// any subsequent good decoder allowed to attempt second half
				decoder->du[x].shstate = 2;
				decoder->du[x].shcount = 0;
				_clearbits(decoder, x);
			}
		}

	}
	else
	{
#if 0
		printf("bad: ");
		for(i=0; i<14; i++)
			printf("%02x ",data[i]);
		printf("%x\n",ccrc);
#endif

		decoder->du[x].shstate = -1;
	}

	if(decoder->good)
	{
		if(decoder->callback)
		{
			(decoder->callback)( (int)decoder->good,
								(unsigned char)decoder->op,
								(unsigned char)decoder->arg,
								(unsigned short)decoder->unitID,
								(unsigned char)decoder->extra0,
								(unsigned char)decoder->extra1,
								(unsigned char)decoder->extra2,
								(unsigned char)decoder->extra3,
								decoder->callback_context);
			decoder->good = 0;

		}
	}
}


static int _onebits(mdc_u32_t n)
{
	int i=0;
	while(n)
	{
		++i;
		n &= (n-1);
	}
	return i;
}

static void _shiftin(mdc_decoder_t *decoder, int x)
{
	int bit = decoder->du[x].xorb;
	int gcount;

	switch(decoder->du[x].shstate)
	{
	case -1:
		decoder->du[x].synchigh = 0;
		decoder->du[x].synclow = 0;
		decoder->du[x].shstate = 0;
		// deliberately fall through
	case 0:
		decoder->du[x].synchigh <<= 1;
		if(decoder->du[x].synclow & 0x80000000)
			decoder->du[x].synchigh |= 1;
		decoder->du[x].synclow <<= 1;
		if(bit)
			decoder->du[x].synclow |= 1;

		gcount = _onebits(0x000000ff & (0x00000007 ^ decoder->du[x].synchigh));
		gcount += _onebits(0x092a446f ^ decoder->du[x].synclow);

		if(gcount <= MDC_GDTHRESH)
		{
 //printf("sync %d  %x %x \n",gcount,decoder->du[x].synchigh, decoder->du[x].synclow);
			decoder->du[x].shstate = 1;
			decoder->du[x].shcount = 0;
			_clearbits(decoder, x);
		}
		else if(gcount >= (40 - MDC_GDTHRESH))
		{
 //printf("isync %d\n",gcount);
			decoder->du[x].shstate = 1;
			decoder->du[x].shcount = 0;
			decoder->du[x].xorb = !(decoder->du[x].xorb);
			decoder->du[x].invert = !(decoder->du[x].invert);
			_clearbits(decoder, x);
		}
		return;
	case 1:
	case 2:
		decoder->du[x].bits[decoder->du[x].shcount] = bit;
		decoder->du[x].shcount++;
		if(decoder->du[x].shcount > 111)
		{
			_procbits(decoder, x);
		}
		return;

	default:
		return;
	}
}

#if 0 // zerocrossing deprecated
static void _zcproc(mdc_decoder_t *decoder, int x)
{
	switch(decoder->du[x].zc)
	{
	case 2:
	case 4:
		break;
	case 3:
		decoder->du[x].xorb = !(decoder->du[x].xorb);
		break;
	default:
		return;
	}

	_shiftin(decoder, x);
}
#endif

#ifdef FOURPOINT

static void _nlproc(mdc_decoder_t *decoder, int x)
{
	mdc_float_t vnow;
	mdc_float_t vpast;

	switch(decoder->du[x].nlstep)
	{
	case 3:
		vnow = ((-0.60 * decoder->du[x].nlevel[3]) + (.97 * decoder->du[x].nlevel[1]));
		vpast = ((-0.60 * decoder->du[x].nlevel[7]) + (.97 * decoder->du[x].nlevel[9]));
		break;
	case 8:
		vnow = ((-0.60 * decoder->du[x].nlevel[8]) + (.97 * decoder->du[x].nlevel[6]));
		vpast = ((-0.60 * decoder->du[x].nlevel[2]) + (.97 * decoder->du[x].nlevel[4]));
		break;
	default:
		return;
	}

	decoder->du[x].xorb = (vnow > vpast) ? 1 : 0;
	if(decoder->du[x].invert)
		decoder->du[x].xorb = !(decoder->du[x].xorb);
	_shiftin(decoder, x);
}
#endif

int mdc_decoder_process_samples(mdc_decoder_t *decoder,
                                mdc_sample_t *samples,
                                int numSamples)
{
	mdc_int_t i, j, k;
	mdc_sample_t sample;
#ifndef MDC_FIXEDMATH
	mdc_float_t value;
#else
	mdc_int_t value;
#endif

	if(!decoder)
		return -1;

	for(i = 0; i<numSamples; i++)
	{
		sample = samples[i];

#ifdef MDC_FIXEDMATH
#if defined(MDC_SAMPLE_FORMAT_U8)
		value = ((mdc_int_t)sample) - 127;
#elif defined(MDC_SAMPLE_FORMAT_U16)
		value = ((mdc_int_t)sample) - 32767;
#elif defined(MDC_SAMPLE_FORMAT_S16)
		value = (mdc_int_t) sample;
#elif defined(MDC_SAMPLE_FORMAT_FLOAT)
#error "fixed-point math not allowed with float sample format"
#else
#error "no known sample format set"
#endif // sample format

#endif // is MDC_FIXEDMATH

#ifndef MDC_FIXEDMATH
#if defined(MDC_SAMPLE_FORMAT_U8)
		value = (((mdc_float_t)sample) - 128.0)/256;
#elif defined(MDC_SAMPLE_FORMAT_U16)
		value = (((mdc_float_t)sample) - 32768.0)/65536.0;
#elif defined(MDC_SAMPLE_FORMAT_S16)
		value = ((mdc_float_t)sample) / 65536.0;
#elif defined(MDC_SAMPLE_FORMAT_FLOAT)
		value = sample;
#else
#error "no known sample format set"
#endif // sample format
#endif // not MDC_FIXEDMATH


#if 0  // zerocrossing is deprecated - doesn't work well for xor-precoded
		if(decoder->level == 0)
		{
			if(value > decoder->hyst)
			{
				for(k=0; k<MDC_ND; k++)
					decoder->du[k].zc++;
				decoder->level = 1;
			}
		}
		else
		{
			if(value < (-1.0 * decoder->hyst))
			{
				for(k=0; k<MDC_ND; k++)
					decoder->du[k].zc++;
				decoder->level = 0;
			}
		}
		

		for(j=0; j<MDC_ND; j++)
		{
			decoder->du[j].th += decoder->incr;
			if(decoder->du[j].th >= TWOPI)
			{
				_zcproc(decoder, j);
				decoder->du[j].th -= TWOPI;
				decoder->du[j].zc = 0;
			}
		}
#endif

#ifdef PLL
		decoder->zthu += decoder->incru;

		if(value > 0)
		{
			if(decoder->zprev == 0)
			{
				if((decoder->zthu + decoder->incru) < decoder->zthu)	// XXX widen this up later
				{
					for(j=0; j<MDC_ND; j++)
					{
						mdc_u32_t offset = decoder->du[j].thu - decoder->du[j].plt;
						mdc_u32_t roffset = decoder->du[j].plt - decoder->du[j].thu;

					//	printf("%d off by %08x\n",j,offset);
						if(offset < 0x42371c72)
						{
							decoder->du[j].thu -= ((1 * offset) / 3);
						} else if(offset > 0xbf38e38e)
						{
							decoder->du[j].thu += ((1 * roffset) / 3);	// ???
						} else if(offset > 0x7fffffff)
						{

						} else
						{
						}
					}
				}
				decoder->zprev = 1;
				decoder->zthu = 0;
			}
		}
		else
		{
			decoder->zprev = 0;
		}


#endif

#if defined(ONEPOINT)

		for(j=0; j<MDC_ND; j++)
		{
			// decoder->du[j].th += decoder->incr;
			mdc_u32_t lthu = decoder->du[j].thu;
			decoder->du[j].thu += decoder->incru;
		//	if(decoder->du[j].th >= TWOPI)
			if(decoder->du[j].thu < lthu) // wrapped
			{
				if(value > 0)
					decoder->du[j].xorb = 1;
				else
					decoder->du[j].xorb = 0;
				if(decoder->du[j].invert)
					decoder->du[j].xorb = !(decoder->du[j].xorb);
				_shiftin(decoder, j);
				//decoder->du[j].th -= TWOPI;
			}
		}

#elif defined(FOURPOINT)	/* ZEROCROSSING */

#ifdef MDC_FIXEDMATH
#error "fixed-point math not allowed for fourpoint strategy"
#endif

		for(j=0; j<MDC_ND; j++)
		{
			//decoder->du[j].th += (5.0 * decoder->incr);
			mdc_u32_t lthu = decoder->du[j].thu;
			decoder->du[j].thu += 5 * decoder->incru;
		//	if(decoder->du[j].th >= TWOPI)
			if(decoder->du[j].thu < lthu) // wrapped
			{
				decoder->du[j].nlstep++;
				if(decoder->du[j].nlstep > 9)
					decoder->du[j].nlstep = 0;
				decoder->du[j].nlevel[decoder->du[j].nlstep] = value;	

				_nlproc(decoder, j);

				//decoder->du[j].th -= TWOPI;
			}
		}

#else
#error "no decode strategy chosen"
#endif
	}

	

	if(decoder->good)
		return decoder->good;

	return 0;
}

int mdc_decoder_get_packet(mdc_decoder_t *decoder, 
                           unsigned char *op,
			   unsigned char *arg,
			   unsigned short *unitID)
{
	if(!decoder)
		return -1;

	if(decoder->good != 1)
		return -1;

	if(op)
		*op = decoder->op;

	if(arg)
		*arg = decoder->arg;

	if(unitID)
		*unitID = decoder->unitID;

	decoder->good = 0;

	return 0;
}

int mdc_decoder_get_double_packet(mdc_decoder_t *decoder, 
                           unsigned char *op,
			   unsigned char *arg,
			   unsigned short *unitID,
                           unsigned char *extra0,
                           unsigned char *extra1,
                           unsigned char *extra2,
                           unsigned char *extra3)
{
	if(!decoder)
		return -1;

	if(decoder->good != 2)
		return -1;

	if(op)
		*op = decoder->op;

	if(arg)
		*arg = decoder->arg;

	if(unitID)
		*unitID = decoder->unitID;

	if(extra0)
		*extra0 = decoder->extra0;
	if(extra1)
		*extra1 = decoder->extra1;
	if(extra2)
		*extra2 = decoder->extra2;
	if(extra3)
		*extra3 = decoder->extra3;

	decoder->good = 0;

	return 0;
}

int mdc_decoder_set_callback(mdc_decoder_t *decoder, mdc_decoder_callback_t callbackFunction, void *context)
{
	if(!decoder)
		return -1;

	decoder->callback = callbackFunction;
	decoder->callback_context = context;

	return 0;
}