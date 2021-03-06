/*-
 * mdc_encode.c
 *  Encodes a specific format from 1200 BPS MSK data burst
 *  to output audio samples.
 *
 * 9 October 2010 - typedefs for easier porting
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
#include "mdc_encode.h"
#include "mdc_common.c"

#if defined(MDC_SAMPLE_FORMAT_U8)

static mdc_sample_t sintable[] = {
      127, 130, 133, 136, 139, 142, 145, 148, 151, 154, 157, 160, 163, 166, 169, 172,
	  175, 178, 180, 183, 186, 189, 191, 194, 196, 199, 201, 204, 206, 209, 211, 213,
	  215, 218, 220, 222, 224, 226, 227, 229, 231, 233, 234, 236, 237, 239, 240, 241,
	  242, 244, 245, 246, 247, 247, 248, 249, 250, 250, 251, 251, 251, 252, 252, 252,
	  252, 252, 252, 252, 251, 251, 251, 250, 250, 249, 248, 247, 247, 246, 245, 244,
	  242, 241, 240, 239, 237, 236, 234, 233, 231, 229, 227, 226, 224, 222, 220, 218,
	  215, 213, 211, 209, 206, 204, 201, 199, 196, 194, 191, 189, 186, 183, 180, 178,
	  175, 172, 169, 166, 163, 160, 157, 154, 151, 148, 145, 142, 139, 136, 133, 130,
	  127, 124, 121, 118, 115, 112, 109, 106, 103, 100,  97,  94,  91,  88,  85,  82,
	  79,  76,  74,  71,  68,  65,  63,  60,  58,  55,  53,  50, 48,  45,  43,  41,
	  39,  36,  34,  32,  30,  28,  27,  25, 23,  21,  20,  18,  17,  15,  14,  13,
	  12,  10,   9,   8, 7,   7,   6,   5,   4,   4,   3,   3,   3,   2,   2,   2,
	  2,   2,   2,   2,   3,   3,   3,   4,   4,   5,   6,   7, 7,   8,   9,  10,
	  12,  13,  14,  15,  17,  18,  20,  21, 23,  25,  27,  28,  30,  32,  34,  36,
	  39,  41,  43,  45, 48,  50,  53,  55,  58,  60,  63,  65,  68,  71,  74,  76,
	  79,  82,  85,  88,  91,  94,  97, 100, 103, 106, 109, 112, 115, 118, 121, 124 };

#elif defined(MDC_SAMPLE_FORMAT_U16)

static mdc_sample_t sintable[] = {
	32768, 33552, 34337, 35120, 35902, 36682, 37460, 38235,
	39007, 39775, 40538, 41297, 42051, 42799, 43542, 44277,
	45006, 45728, 46441, 47147, 47843, 48531, 49209, 49877,
	50535, 51182, 51819, 52443, 53056, 53657, 54245, 54820,
	55381, 55930, 56464, 56984, 57489, 57980, 58455, 58915,
	59359, 59787, 60199, 60594, 60972, 61334, 61678, 62005,
	62314, 62606, 62879, 63134, 63371, 63590, 63790, 63971,
	64134, 64278, 64402, 64508, 64595, 64662, 64710, 64739,
	64749, 64739, 64710, 64662, 64595, 64508, 64402, 64278,
	64134, 63971, 63790, 63590, 63371, 63134, 62879, 62606,
	62314, 62005, 61678, 61334, 60972, 60594, 60199, 59787,
	59359, 58915, 58455, 57980, 57489, 56984, 56464, 55930,
	55381, 54820, 54245, 53657, 53056, 52443, 51819, 51182,
	50535, 49877, 49209, 48531, 47843, 47147, 46441, 45728,
	45006, 44277, 43542, 42799, 42051, 41297, 40538, 39775,
	39007, 38235, 37460, 36682, 35902, 35120, 34337, 33552,
	32768, 31983, 31198, 30415, 29633, 28853, 28075, 27300,
	26528, 25760, 24997, 24238, 23484, 22736, 21993, 21258,
	20529, 19807, 19094, 18388, 17692, 17004, 16326, 15658,
	15000, 14353, 13716, 13092, 12479, 11878, 11290, 10715,
	10154,  9605,  9071,  8551,  8046,  7555,  7080,  6620,
 	6176,  5748,  5336,  4941,  4563,  4201,  3857,  3530,
 	3221,  2929,  2656,  2401,  2164,  1945,  1745,  1564,
 	1401,  1257,  1133,  1027,   940,   873,   825,   796,
  	787,   796,   825,   873,   940,  1027,  1133,  1257,
 	1401,  1564,  1745,  1945,  2164,  2401,  2656,  2929,
 	3221,  3530,  3857,  4201,  4563,  4941,  5336,  5748,
 	6176,  6620,  7080,  7555,  8046,  8551,  9071,  9605,
	10154, 10715, 11290, 11878, 12479, 13092, 13716, 14353,
	15000, 15658, 16326, 17004, 17692, 18388, 19094, 19807,
	20529, 21258, 21993, 22736, 23484, 24238, 24997, 25760,
	26528, 27300, 28075, 28853, 29633, 30415, 31198, 31983 };

#elif defined(MDC_SAMPLE_FORMAT_S16)

static mdc_sample_t sintable[] = {
	     0,    784,   1569,   2352,   3134,   3914,   4692,   5467, 
	  6239,   7007,   7770,   8529,   9283,  10031,  10774,  11509, 
	 12238,  12960,  13673,  14379,  15075,  15763,  16441,  17109, 
	 17767,  18414,  19051,  19675,  20288,  20889,  21477,  22052, 
	 22613,  23162,  23696,  24216,  24721,  25212,  25687,  26147, 
	 26591,  27019,  27431,  27826,  28204,  28566,  28910,  29237, 
	 29546,  29838,  30111,  30366,  30603,  30822,  31022,  31203, 
	 31366,  31510,  31634,  31740,  31827,  31894,  31942,  31971, 
	 31981,  31971,  31942,  31894,  31827,  31740,  31634,  31510, 
	 31366,  31203,  31022,  30822,  30603,  30366,  30111,  29838, 
	 29546,  29237,  28910,  28566,  28204,  27826,  27431,  27019, 
	 26591,  26147,  25687,  25212,  24721,  24216,  23696,  23162, 
	 22613,  22052,  21477,  20889,  20288,  19675,  19051,  18414, 
	 17767,  17109,  16441,  15763,  15075,  14379,  13673,  12960, 
	 12238,  11509,  10774,  10031,   9283,   8529,   7770,   7007,
	  6239,   5467,   4692,   3914,   3134,   2352,   1569,    784,
	     0,   -784,  -1569,  -2352,  -3134,  -3914,  -4692,  -5467,
	 -6239,  -7007,  -7770,  -8529,  -9283, -10031, -10774, -11509,
	-12238, -12960, -13673, -14379, -15075, -15763, -16441, -17109,
	-17767, -18414, -19051, -19675, -20288, -20889, -21477, -22052,
	-22613, -23162, -23696, -24216, -24721, -25212, -25687, -26147,
	-26591, -27019, -27431, -27826, -28204, -28566, -28910, -29237,
	-29546, -29838, -30111, -30366, -30603, -30822, -31022, -31203,
	-31366, -31510, -31634, -31740, -31827, -31894, -31942, -31971,
	-31981, -31971, -31942, -31894, -31827, -31740, -31634, -31510,
	-31366, -31203, -31022, -30822, -30603, -30366, -30111, -29838,
	-29546, -29237, -28910, -28566, -28204, -27826, -27431, -27019,
	-26591, -26147, -25687, -25212, -24721, -24216, -23696, -23162,
	-22613, -22052, -21477, -20889, -20288, -19675, -19051, -18414,
	-17767, -17109, -16441, -15763, -15075, -14379, -13673, -12960,
	-12238, -11509, -10774, -10031,  -9283,  -8529,  -7770,  -7007,
	 -6239,  -5467,  -4692,  -3914,  -3134,  -2352,  -1569,   -784 };

#elif defined(MDC_SAMPLE_FORMAT_FLOAT)

static mdc_sample_t sintable[] = {
	 0.000000,  0.024541,  0.049068,  0.073565,  0.098017,  0.122411,  0.146730,  0.170962,
	 0.195090,  0.219101,  0.242980,  0.266713,  0.290285,  0.313682,  0.336890,  0.359895,
	 0.382683,  0.405241,  0.427555,  0.449611,  0.471397,  0.492898,  0.514103,  0.534998,
	 0.555570,  0.575808,  0.595699,  0.615232,  0.634393,  0.653173,  0.671559,  0.689541,
	 0.707107,  0.724247,  0.740951,  0.757209,  0.773010,  0.788346,  0.803208,  0.817585,
	 0.831470,  0.844854,  0.857729,  0.870087,  0.881921,  0.893224,  0.903989,  0.914210,
	 0.923880,  0.932993,  0.941544,  0.949528,  0.956940,  0.963776,  0.970031,  0.975702,
	 0.980785,  0.985278,  0.989177,  0.992480,  0.995185,  0.997290,  0.998795,  0.999699,
	 1.000000,  0.999699,  0.998795,  0.997290,  0.995185,  0.992480,  0.989177,  0.985278,
	 0.980785,  0.975702,  0.970031,  0.963776,  0.956940,  0.949528,  0.941544,  0.932993,
	 0.923880,  0.914210,  0.903989,  0.893224,  0.881921,  0.870087,  0.857729,  0.844854,
	 0.831470,  0.817585,  0.803208,  0.788346,  0.773010,  0.757209,  0.740951,  0.724247,
	 0.707107,  0.689541,  0.671559,  0.653173,  0.634393,  0.615232,  0.595699,  0.575808,
	 0.555570,  0.534998,  0.514103,  0.492898,  0.471397,  0.449611,  0.427555,  0.405241,
	 0.382683,  0.359895,  0.336890,  0.313682,  0.290285,  0.266713,  0.242980,  0.219101,
	 0.195090,  0.170962,  0.146730,  0.122411,  0.098017,  0.073565,  0.049068,  0.024541,
	 0.000000, -0.024541, -0.049068, -0.073565, -0.098017, -0.122411, -0.146730, -0.170962,
	-0.195090, -0.219101, -0.242980, -0.266713, -0.290285, -0.313682, -0.336890, -0.359895,
	-0.382683, -0.405241, -0.427555, -0.449611, -0.471397, -0.492898, -0.514103, -0.534998,
	-0.555570, -0.575808, -0.595699, -0.615232, -0.634393, -0.653173, -0.671559, -0.689541,
	-0.707107, -0.724247, -0.740951, -0.757209, -0.773010, -0.788346, -0.803208, -0.817585,
	-0.831470, -0.844854, -0.857729, -0.870087, -0.881921, -0.893224, -0.903989, -0.914210,
	-0.923880, -0.932993, -0.941544, -0.949528, -0.956940, -0.963776, -0.970031, -0.975702,
	-0.980785, -0.985278, -0.989177, -0.992480, -0.995185, -0.997290, -0.998795, -0.999699,
	-1.000000, -0.999699, -0.998795, -0.997290, -0.995185, -0.992480, -0.989177, -0.985278,
	-0.980785, -0.975702, -0.970031, -0.963776, -0.956940, -0.949528, -0.941544, -0.932993,
	-0.923880, -0.914210, -0.903989, -0.893224, -0.881921, -0.870087, -0.857729, -0.844854,
	-0.831470, -0.817585, -0.803208, -0.788346, -0.773010, -0.757209, -0.740951, -0.724247,
	-0.707107, -0.689541, -0.671559, -0.653173, -0.634393, -0.615232, -0.595699, -0.575808,
	-0.555570, -0.534998, -0.514103, -0.492898, -0.471397, -0.449611, -0.427555, -0.405241,
	-0.382683, -0.359895, -0.336890, -0.313682, -0.290285, -0.266713, -0.242980, -0.219101,
	-0.195090, -0.170962, -0.146730, -0.122411, -0.098017, -0.073565, -0.049068, -0.024541 };
#else
#error "no known sample format defined"
#endif

mdc_encoder_t * mdc_encoder_new(int sampleRate)
{
	mdc_encoder_t *encoder;

	encoder = (mdc_encoder_t *)malloc(sizeof(mdc_encoder_t));
	if(!encoder)
		return (mdc_encoder_t *) 0L;

	encoder->loaded = 0;

	if(sampleRate == 8000)
	{
		encoder->incru = 644245094;
		encoder->incru18 = 966367642;
	} else if(sampleRate == 16000)
	{
		encoder->incru = 322122547;
		encoder->incru18 = 483183820;
	} else if(sampleRate == 22050)
	{
		encoder->incru = 233739716;
		encoder->incru18 = 350609575;
	} else if(sampleRate == 32000)
	{
		encoder->incru = 161061274;
		encoder->incru18 = 241591910;
	} else if(sampleRate == 44100)
	{
		encoder->incru = 116869858;
		encoder->incru18 = 175304788;
	} else if(sampleRate == 48000)
	{
		encoder->incru = 107374182;
		encoder->incru18 = 161061274;
	} else
	{
		// WARNING: lower precision than above
		encoder->incru = 1200 * 2 * (0x80000000 / sampleRate);
		encoder->incru18 = 1800 * 2 * (0x80000000 / sampleRate);
	}

	return encoder;
}

static mdc_u8_t * _enc_leader(mdc_u8_t *data)
{
	data[0] = 0x55;
	data[1] = 0x55;
	data[2] = 0x55;
	data[3] = 0x55;
	data[4] = 0x55;
	data[5] = 0x55;
	data[6] = 0x55;

	data[7] = 0x07;
	data[8] = 0x09;
	data[9] = 0x2a;
	data[10] = 0x44;
	data[11] = 0x6f;

	return &(data[12]);
}

static mdc_u8_t * _enc_str(mdc_u8_t *data)
{
	mdc_u16_t ccrc;
	mdc_int_t i, j;
	mdc_int_t k;
	mdc_int_t m;
	mdc_int_t csr[7];
	mdc_int_t b;
	mdc_int_t lbits[112];

	ccrc = _docrc(data, 4);

	data[4] = ccrc & 0x00ff;
	data[5] = (ccrc >> 8) & 0x00ff;

	data[6] = 0; 

	for(i=0; i<7; i++)
		csr[i] = 0;

	for(i=0; i<7; i++)
	{
		data[i+7] = 0;
		for(j=0; j<=7; j++)
		{
			for(k=6; k > 0; k--)
				csr[k] = csr[k-1];
			csr[0] = (data[i] >> j) & 0x01;
			b = csr[0] + csr[2] + csr[5] + csr[6];
			data[i+7] |= (b & 0x01) << j;
		}
	}

	k=0;
	m=0;
	for(i=0; i<14; i++)
	{
		for(j=0; j<=7; j++)
		{
			b = 0x01 & (data[i] >> j);
			lbits[k] = b;
			k += 16; 
			if(k > 111)
				k = ++m;
		}
	}

	k = 0;
	for(i=0; i<14; i++)
	{
		data[i] = 0;
		for(j=7; j>=0; j--)
		{
			if(lbits[k])
				data[i] |= 1<<j;
			++k;
		}
	}


	return &(data[14]);
}

int mdc_encoder_set_packet(mdc_encoder_t *encoder,
                           unsigned char op,
			   unsigned char arg,
			   unsigned short unitID)
{
	mdc_u8_t *dp;


	if(!encoder)
		return -1;

	if(encoder->loaded)
		return -1;

	encoder->state = 0;

	dp = _enc_leader(encoder->data);

	dp[0] = op;
	dp[1] = arg;
	dp[2] = (unitID >> 8) & 0x00ff;
	dp[3] = unitID & 0x00ff;

	_enc_str(dp);

	encoder->loaded = 26;

	return 0;
}

int mdc_encoder_set_double_packet(mdc_encoder_t *encoder,
                                  unsigned char op,
				  unsigned char arg,
				  unsigned short unitID,
				  unsigned char extra0,
				  unsigned char extra1,
				  unsigned char extra2,
				  unsigned char extra3)
{
	mdc_u8_t *dp;

	if(!encoder)
		return -1;

	if(encoder->loaded)
		return -1;

	encoder->state = 0;

	dp = _enc_leader(encoder->data);

	dp[0] = op;
	dp[1] = arg;
	dp[2] = (unitID >> 8) & 0x00ff;
	dp[3] = unitID & 0x00ff;

	dp = _enc_str(dp);

	dp[0] = extra0;
	dp[1] = extra1;
	dp[2] = extra2;
	dp[3] = extra3;

	_enc_str(dp);

	encoder->loaded = 40;

	return 0;
}

static mdc_sample_t  _enc_get_samp(mdc_encoder_t *encoder)
{
	mdc_int_t b;
	mdc_int_t ofs;

	mdc_u32_t lthu = encoder->thu;
	encoder->thu += encoder->incru;


	if(encoder->thu  < lthu) // wrap
	{
		encoder->ipos++;
		if(encoder->ipos > 7)
		{
			encoder->ipos = 0;
			encoder->bpos++;
			if(encoder->bpos >= encoder->loaded)
			{
				encoder->state = 0;
				return sintable[0];
			}
		}

		b = 0x01 & (encoder->data[encoder->bpos] >> (7-(encoder->ipos)));

		if(b != encoder->lb)
		{
			encoder->xorb = 1;
			encoder->lb = b;
		}
		else
			encoder->xorb = 0;
	}

	if(encoder->xorb)
		encoder->tthu += encoder->incru18;
	else
		encoder->tthu += encoder->incru;

	ofs = (int)(encoder->tthu >> 24);

	return sintable[ofs];
}

int mdc_encoder_get_samples(mdc_encoder_t *encoder,
                            mdc_sample_t *buffer,
			    int bufferSize)
{
	mdc_int_t i;

	if(!encoder)
		return -1;

	if(!(encoder->loaded))
		return 0;

	if(encoder->state == 0)
	{
		encoder->tthu = 0;
		encoder->thu = 0;
		encoder->bpos = 0;
		encoder->ipos = 0;
		encoder->state = 1;
		encoder->xorb = 1;
		encoder->lb = 0;
	}

	i = 0;
	while((i < bufferSize) && encoder->state)
	{
		buffer[i++] = _enc_get_samp(encoder);
	}

#ifdef FILL_FINAL
	while(i<bufferSize)
	{
		buffer[i++] = sintable[0];
	}
#endif

	if(encoder->state == 0)
		encoder->loaded = 0;
	return i;
}

