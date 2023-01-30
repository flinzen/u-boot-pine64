/*
 * wm8904.c  --  WM8904 ALSA SoC Audio driver
 *
 * Copyright 2009-12 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <common.h>
#include <i2c.h>
#include <div64.h>
#include "wm8904.h"
#include "wm8904-pdata.h"


enum wm8904_type {
	WM8904,
	WM8912,
};

#define WM8904_NUM_DCS_CHANNELS 4

#define WM8904_NUM_SUPPLIES 5

/* codec private data */
struct wm8904_priv {
	enum wm8904_type devtype;

	struct wm8904_pdata *pdata;

	int deemph;

	int drc_cfg;

	/* Platform provided ReTune mobile configuration */
	int num_retune_mobile_texts;
	int retune_mobile_cfg;

	/* FLL setup */
	int fll_src;
	int fll_fref;
	int fll_fout;

	/* Clocking configuration */
	unsigned int mclk_rate;
	int sysclk_src;
	unsigned int sysclk_rate;

	int tdm_width;
	int tdm_slots;
	int bclk;
	int fs;

	/* DC servo configuration - cached offset values */
	unsigned short dcs_state[WM8904_NUM_DCS_CHANNELS];
};

static struct wm8904_priv priv;

#if 0
static const struct reg_default wm8904_reg_defaults[] = {
	{ 4,   0x0018 },     /* R4   - Bias Control 0 */
	{ 5,   0x0000 },     /* R5   - VMID Control 0 */
	{ 6,   0x0000 },     /* R6   - Mic Bias Control 0 */
	{ 7,   0x0000 },     /* R7   - Mic Bias Control 1 */
	{ 8,   0x0001 },     /* R8   - Analogue DAC 0 */
	{ 9,   0x9696 },     /* R9   - mic Filter Control */
	{ 10,  0x0001 },     /* R10  - Analogue ADC 0 */
	{ 12,  0x0000 },     /* R12  - Power Management 0 */
	{ 14,  0x0000 },     /* R14  - Power Management 2 */
	{ 15,  0x0000 },     /* R15  - Power Management 3 */
	{ 18,  0x0000 },     /* R18  - Power Management 6 */
	{ 20,  0x945E },     /* R20  - Clock Rates 0 */
	{ 21,  0x0C05 },     /* R21  - Clock Rates 1 */
	{ 22,  0x0006 },     /* R22  - Clock Rates 2 */
	{ 24,  0x0050 },     /* R24  - Audio Interface 0 */
	{ 25,  0x000A },     /* R25  - Audio Interface 1 */
	{ 26,  0x00E4 },     /* R26  - Audio Interface 2 */
	{ 27,  0x0040 },     /* R27  - Audio Interface 3 */
	{ 30,  0x00C0 },     /* R30  - DAC Digital Volume Left */
	{ 31,  0x00C0 },     /* R31  - DAC Digital Volume Right */
	{ 32,  0x0000 },     /* R32  - DAC Digital 0 */
	{ 33,  0x0008 },     /* R33  - DAC Digital 1 */
	{ 36,  0x00C0 },     /* R36  - ADC Digital Volume Left */
	{ 37,  0x00C0 },     /* R37  - ADC Digital Volume Right */
	{ 38,  0x0010 },     /* R38  - ADC Digital 0 */
	{ 39,  0x0000 },     /* R39  - Digital Microphone 0 */
	{ 40,  0x01AF },     /* R40  - DRC 0 */
	{ 41,  0x3248 },     /* R41  - DRC 1 */
	{ 42,  0x0000 },     /* R42  - DRC 2 */
	{ 43,  0x0000 },     /* R43  - DRC 3 */
	{ 44,  0x0085 },     /* R44  - Analogue Left Input 0 */
	{ 45,  0x0085 },     /* R45  - Analogue Right Input 0 */
	{ 46,  0x0044 },     /* R46  - Analogue Left Input 1 */
	{ 47,  0x0044 },     /* R47  - Analogue Right Input 1 */
	{ 57,  0x002D },     /* R57  - Analogue OUT1 Left */
	{ 58,  0x002D },     /* R58  - Analogue OUT1 Right */
	{ 59,  0x0039 },     /* R59  - Analogue OUT2 Left */
	{ 60,  0x0039 },     /* R60  - Analogue OUT2 Right */
	{ 61,  0x0000 },     /* R61  - Analogue OUT12 ZC */
	{ 67,  0x0000 },     /* R67  - DC Servo 0 */
	{ 69,  0xAAAA },     /* R69  - DC Servo 2 */
	{ 71,  0xAAAA },     /* R71  - DC Servo 4 */
	{ 72,  0xAAAA },     /* R72  - DC Servo 5 */
	{ 90,  0x0000 },     /* R90  - Analogue HP 0 */
	{ 94,  0x0000 },     /* R94  - Analogue Lineout 0 */
	{ 98,  0x0000 },     /* R98  - Charge Pump 0 */
	{ 104, 0x0004 },     /* R104 - Class W 0 */
	{ 108, 0x0000 },     /* R108 - Write Sequencer 0 */
	{ 109, 0x0000 },     /* R109 - Write Sequencer 1 */
	{ 110, 0x0000 },     /* R110 - Write Sequencer 2 */
	{ 111, 0x0000 },     /* R111 - Write Sequencer 3 */
	{ 112, 0x0000 },     /* R112 - Write Sequencer 4 */
	{ 116, 0x0000 },     /* R116 - FLL Control 1 */
	{ 117, 0x0007 },     /* R117 - FLL Control 2 */
	{ 118, 0x0000 },     /* R118 - FLL Control 3 */
	{ 119, 0x2EE0 },     /* R119 - FLL Control 4 */
	{ 120, 0x0004 },     /* R120 - FLL Control 5 */
	{ 121, 0x0014 },     /* R121 - GPIO Control 1 */
	{ 122, 0x0010 },     /* R122 - GPIO Control 2 */
	{ 123, 0x0010 },     /* R123 - GPIO Control 3 */
	{ 124, 0x0000 },     /* R124 - GPIO Control 4 */
	{ 126, 0x0000 },     /* R126 - Digital Pulls */
	{ 128, 0xFFFF },     /* R128 - Interrupt Status Mask */
	{ 129, 0x0000 },     /* R129 - Interrupt Polarity */
	{ 130, 0x0000 },     /* R130 - Interrupt Debounce */
	{ 134, 0x0000 },     /* R134 - EQ1 */
	{ 135, 0x000C },     /* R135 - EQ2 */
	{ 136, 0x000C },     /* R136 - EQ3 */
	{ 137, 0x000C },     /* R137 - EQ4 */
	{ 138, 0x000C },     /* R138 - EQ5 */
	{ 139, 0x000C },     /* R139 - EQ6 */
	{ 140, 0x0FCA },     /* R140 - EQ7 */
	{ 141, 0x0400 },     /* R141 - EQ8 */
	{ 142, 0x00D8 },     /* R142 - EQ9 */
	{ 143, 0x1EB5 },     /* R143 - EQ10 */
	{ 144, 0xF145 },     /* R144 - EQ11 */
	{ 145, 0x0B75 },     /* R145 - EQ12 */
	{ 146, 0x01C5 },     /* R146 - EQ13 */
	{ 147, 0x1C58 },     /* R147 - EQ14 */
	{ 148, 0xF373 },     /* R148 - EQ15 */
	{ 149, 0x0A54 },     /* R149 - EQ16 */
	{ 150, 0x0558 },     /* R150 - EQ17 */
	{ 151, 0x168E },     /* R151 - EQ18 */
	{ 152, 0xF829 },     /* R152 - EQ19 */
	{ 153, 0x07AD },     /* R153 - EQ20 */
	{ 154, 0x1103 },     /* R154 - EQ21 */
	{ 155, 0x0564 },     /* R155 - EQ22 */
	{ 156, 0x0559 },     /* R156 - EQ23 */
	{ 157, 0x4000 },     /* R157 - EQ24 */
	{ 161, 0x0000 },     /* R161 - Control Interface Test 1 */
	{ 204, 0x0000 },     /* R204 - Analogue Output Bias 0 */
	{ 247, 0x0000 },     /* R247 - FLL NCO Test 0 */
	{ 248, 0x0019 },     /* R248 - FLL NCO Test 1 */
};
#endif

static int wm8904_i2c_write(unsigned int reg, unsigned short data)
{
	unsigned char val[2];

	val[0] = (unsigned char)((data >> 8) & 0xff);
	val[1] = (unsigned char)(data & 0xff);
	printf("Write Addr : 0x%04X, Data :  0x%04X\n", reg, data);

	return i2c_write(0x1a, reg, 2, val, 2);
}

static unsigned int wm8904_i2c_read(unsigned int reg, unsigned short *data)
{
	unsigned char val[2];
	int ret;

	ret = i2c_read(0x1a, reg, 2, val, 2);
	if (ret != 0) {
		printf("%s: Error while reading register %#04x\n",
		      __func__, reg);
		return -1;
	}

	*data = val[0];
	*data <<= 8;
	*data |= val[1];

	return 0;
}

static int wm8904_update_bits(unsigned int reg, unsigned short mask,
						unsigned short value)
{
	int change , ret = 0;
	unsigned short old, new;

	if (wm8904_i2c_read(reg, &old) != 0)
		return -1;
	new = (old & ~mask) | (value & mask);
	change  = (old != new) ? 1 : 0;
	if (change)
		ret = wm8904_i2c_write(reg, new);
	if (ret < 0)
		return ret;

	return change;
}

static int wm8904_configure_clocking(void)
{
	struct wm8904_priv *wm8904 = &priv;
	unsigned short clock0, clock2;
	int rate;

	/* Gate the clock while we're updating to avoid misclocking */
	wm8904_i2c_read(WM8904_CLOCK_RATES_2, &clock2);
	wm8904_update_bits(WM8904_CLOCK_RATES_2,
			    WM8904_SYSCLK_SRC, 0);

	/* This should be done on init() for bypass paths */
	switch (wm8904->sysclk_src) {
	case WM8904_CLK_MCLK:
		printf("Using %dHz MCLK\n", wm8904->mclk_rate);

		clock2 &= ~WM8904_SYSCLK_SRC;
		rate = wm8904->mclk_rate;

		/* Ensure the FLL is stopped */
		wm8904_update_bits(WM8904_FLL_CONTROL_1,
				    WM8904_FLL_OSC_ENA | WM8904_FLL_ENA, 0);
		break;

	case WM8904_CLK_FLL:
		printf("Using %dHz FLL clock\n",
			wm8904->fll_fout);

		clock2 |= WM8904_SYSCLK_SRC;
		rate = wm8904->fll_fout;
		break;

	default:
		printf("System clock not configured\n");
		return -1;
	}

	/* SYSCLK shouldn't be over 13.5MHz */
	if (rate > 13500000) {
		clock0 = WM8904_MCLK_DIV;
		wm8904->sysclk_rate = rate / 2;
	} else {
		clock0 = 0;
		wm8904->sysclk_rate = rate;
	}

	wm8904_update_bits(WM8904_CLOCK_RATES_0, WM8904_MCLK_DIV,
			    clock0);

	wm8904_update_bits(WM8904_CLOCK_RATES_2,
			    WM8904_CLK_SYS_ENA | WM8904_SYSCLK_SRC, clock2);

	printf("CLK_SYS is %dHz\n", wm8904->sysclk_rate);

	return 0;
}

void wm8904_set_drc(void)
{
	struct wm8904_priv *wm8904 = &priv;
	struct wm8904_pdata *pdata = wm8904->pdata;
	u16 save, i;

	/* Save any enables; the configuration should clear them. */
	wm8904_i2c_read(WM8904_DRC_0, &save);

	for (i = 0; i < WM8904_DRC_REGS; i++)
		wm8904_update_bits(WM8904_DRC_0 + i, 0xffff,
				    pdata->drc_cfgs[wm8904->drc_cfg].regs[i]);

	/* Reenable the DRC */
	wm8904_update_bits(WM8904_DRC_0,
			    WM8904_DRC_ENA | WM8904_DRC_DAC_PATH, save);
}

static void wm8904_set_retune_mobile(void)
{
	#if 0
	struct wm8904_priv *wm8904 = &priv;
	struct wm8904_pdata *pdata = wm8904->pdata;
	u32 best, best_val, i, cfg;
	u16 save;
	if (!pdata || !wm8904->num_retune_mobile_texts)
		return;

	/* Find the version of the currently selected configuration
	 * with the nearest sample rate. */
	
	cfg = wm8904->retune_mobile_cfg;
	best = 0;
	best_val = 0x7FFFFFFF;
	for (i = 0; i < pdata->num_retune_mobile_cfgs; i++) {
		if (strcmp(pdata->retune_mobile_cfgs[i].name,
			   wm8904->retune_mobile_texts[cfg]) == 0 &&
		    abs(pdata->retune_mobile_cfgs[i].rate
			- wm8904->fs) < best_val) {
			best = i;
			best_val = abs(pdata->retune_mobile_cfgs[i].rate
				       - wm8904->fs);
		}
	}

	printf("ReTune Mobile %s/%dHz for %dHz sample rate\n",
		pdata->retune_mobile_cfgs[best].name,
		pdata->retune_mobile_cfgs[best].rate,
		wm8904->fs);

	/* The EQ will be disabled while reconfiguring it, remember the
	 * current configuration. 
	 */
	wm8904_i2c_read(WM8904_EQ1, &save);

	for (i = 0; i < WM8904_EQ_REGS; i++)
		wm8904_update_bits(WM8904_EQ1 + i, 0xffff,
				pdata->retune_mobile_cfgs[best].regs[i]);

	wm8904_update_bits(WM8904_EQ1, WM8904_EQ_ENA, save);
	#endif
}


static int deemph_settings[] = { 0, 32000, 44100, 48000 };

static int wm8904_set_deemph(void)
{
	struct wm8904_priv *wm8904 = &priv;
	int val, i, best;

	/* If we're using deemphasis select the nearest available sample 
	 * rate.
	 */
	if (wm8904->deemph) {
		best = 1;
		for (i = 2; i < ARRAY_SIZE(deemph_settings); i++) {
			if (abs(deemph_settings[i] - wm8904->fs) <
			    abs(deemph_settings[best] - wm8904->fs))
				best = i;
		}

		val = best << WM8904_DEEMPH_SHIFT;
	} else {
		val = 0;
	}

	printf("Set deemphasis %d\n", val);

	return wm8904_update_bits(WM8904_DAC_DIGITAL_1,
				   WM8904_DEEMPH_MASK, val);
}

static struct {
	int ratio;
	unsigned int clk_sys_rate;
} clk_sys_rates[] = {
	{   64,  0 },
	{  128,  1 },
	{  192,  2 },
	{  256,  3 },
	{  384,  4 },
	{  512,  5 },
	{  786,  6 },
	{ 1024,  7 },
	{ 1408,  8 },
	{ 1536,  9 },
};

static struct {
	int rate;
	int sample_rate;
} sample_rates[] = {
	{ 8000,  0  },
	{ 11025, 1  },
	{ 12000, 1  },
	{ 16000, 2  },
	{ 22050, 3  },
	{ 24000, 3  },
	{ 32000, 4  },
	{ 44100, 5  },
	{ 48000, 5  },
};

static struct {
	int div; /* *10 due to .5s */
	int bclk_div;
} bclk_divs[] = {
	{ 10,  0  },
	{ 15,  1  },
	{ 20,  2  },
	{ 30,  3  },
	{ 40,  4  },
	{ 50,  5  },
	{ 55,  6  },
	{ 60,  7  },
	{ 80,  8  },
	{ 100, 9  },
	{ 110, 10 },
	{ 120, 11 },
	{ 160, 12 },
	{ 200, 13 },
	{ 220, 14 },
	{ 240, 16 },
	{ 200, 17 },
	{ 320, 18 },
	{ 440, 19 },
	{ 480, 20 },
};


static int snd_soc_calc_frame_size(int sample_size, int channels, int tdm_slots)
{
	return sample_size * channels * tdm_slots;
}

static int snd_soc_calc_bclk(int fs, int sample_size, int channels, int tdm_slots)
{
	return fs * snd_soc_calc_frame_size(sample_size, channels, tdm_slots);
}

int wm8904_hw_params(int aif_id,
		unsigned int sampling_rate, unsigned int bits_per_sample,
		unsigned int channels)
{
	struct wm8904_priv *wm8904 = &priv;
	u32 ret, i, best, best_val, cur_val;
	unsigned int aif1 = 0;
	unsigned int aif2 = 0;
	unsigned int aif3 = 0;
	unsigned int clock1 = 0;
	unsigned int dac_digital1 = 0;

	/* What BCLK do we need? */
	wm8904->fs = sampling_rate;
	if (wm8904->tdm_slots) {
		printf("Configuring for %d %d bit TDM slots\n",
			wm8904->tdm_slots, wm8904->tdm_width);
		wm8904->bclk = snd_soc_calc_bclk(wm8904->fs,
						 wm8904->tdm_width, 2,
						 wm8904->tdm_slots);
	} else {
		wm8904->bclk = sampling_rate * 32 * 1;
	}

	switch (bits_per_sample) {
	case 16:	
		break;
	case 20:
		aif1 |= 0x40;
		break;
	case 24:
		aif1 |= 0x80;
		break;
	case 32:
		aif1 |= 0xc0;
		break;
	default:
		return -1;
	}


	printf("Target BCLK is %dHz\n", wm8904->bclk);

	ret = wm8904_configure_clocking();
	if (ret != 0)
		return ret;

	/* Select nearest CLK_SYS_RATE */
	best = 0;
	best_val = abs((wm8904->sysclk_rate / clk_sys_rates[0].ratio)
		       - wm8904->fs);
	for (i = 1; i < ARRAY_SIZE(clk_sys_rates); i++) {
		cur_val = abs((wm8904->sysclk_rate /
			       clk_sys_rates[i].ratio) - wm8904->fs);
		if (cur_val < best_val) {
			best = i;
			best_val = cur_val;
		}
	}
	printf("Selected CLK_SYS_RATIO of %d\n",
		clk_sys_rates[best].ratio);
	clock1 |= (clk_sys_rates[best].clk_sys_rate
		   << WM8904_CLK_SYS_RATE_SHIFT);

	/* SAMPLE_RATE */
	best = 0;
	best_val = abs(wm8904->fs - sample_rates[0].rate);
	for (i = 1; i < ARRAY_SIZE(sample_rates); i++) {
		/* Closest match */
		cur_val = abs(wm8904->fs - sample_rates[i].rate);
		if (cur_val < best_val) {
			best = i;
			best_val = cur_val;
		}
	}
	printf("Selected SAMPLE_RATE of %dHz\n",
		sample_rates[best].rate);
	clock1 |= (sample_rates[best].sample_rate
		   << WM8904_SAMPLE_RATE_SHIFT);

	/* Enable sloping stopband filter for low sample rates */
	if (wm8904->fs <= 24000)
		dac_digital1 |= WM8904_DAC_SB_FILT;

	/* BCLK_DIV */
	best = 0;
	best_val = 0xFFFFFFFF;
	for (i = 0; i < ARRAY_SIZE(bclk_divs); i++) {
		cur_val = ((wm8904->sysclk_rate * 10) / bclk_divs[i].div)
			- wm8904->bclk;
		if (cur_val < 0) /* Table is sorted */
			break;
		if (cur_val < best_val) {
			best = i;
			best_val = cur_val;
		}
	}
	wm8904->bclk = (wm8904->sysclk_rate * 10) / bclk_divs[best].div;
	printf("Selected BCLK_DIV of %d for %dHz BCLK\n",
		bclk_divs[best].div, wm8904->bclk);
	aif2 |= bclk_divs[best].bclk_div;

	/* LRCLK is a simple fraction of BCLK */
	printf("LRCLK_RATE is %d\n", wm8904->bclk / wm8904->fs);
	aif3 |= wm8904->bclk / wm8904->fs;

	/* Apply the settings */
	wm8904_update_bits(WM8904_DAC_DIGITAL_1,
			    WM8904_DAC_SB_FILT, dac_digital1);
	wm8904_update_bits(WM8904_AUDIO_INTERFACE_1,
			    WM8904_AIF_WL_MASK, aif1);
	wm8904_update_bits(WM8904_AUDIO_INTERFACE_2,
			    WM8904_BCLK_DIV_MASK, aif2);
	wm8904_update_bits(WM8904_AUDIO_INTERFACE_3,
			    WM8904_LRCLK_RATE_MASK, aif3);
	wm8904_update_bits(WM8904_CLOCK_RATES_1,
			    WM8904_SAMPLE_RATE_MASK |
			    WM8904_CLK_SYS_RATE_MASK, clock1);

	/* Update filters for the new settings */
	wm8904_set_retune_mobile();
	wm8904_set_deemph();

	return 0;
}


int wm8904_set_sysclk(int clk_id,
			     unsigned int freq, int dir)
{
	struct wm8904_priv *wm8904 = &priv;

	switch (clk_id) {
	case WM8904_CLK_MCLK:
		wm8904->sysclk_src = clk_id;
		wm8904->mclk_rate = freq;
		break;

	case WM8904_CLK_FLL:
		wm8904->sysclk_src = clk_id;
		break;

	default:
		return -1;
	}

	printf("Clock source is %d at %uHz\n", clk_id, freq);

	wm8904_configure_clocking();

	return 0;
}

#define SND_SOC_DAIFMT_MASTER_MASK 0xf000
#define SND_SOC_DAIFMT_INV_MASK    0x0f00
#define SND_SOC_DAIFMT_FORMAT_MASK 0x000f
#define SND_SOC_DAIFMT_CBM_CFM (1 << 12)
#define SND_SOC_DAIFMT_CBS_CFS (4 << 12)
#define SND_SOC_DAIFMT_CBM_CFS (3 << 12)
#define SND_SOC_DAIFMT_CBS_CFM (2 << 12)
#define SND_SOC_DAIFMT_NB_NF   (0 << 8)
#define SND_SOC_DAIFMT_IB_NF   (3 << 8)
#define SND_SOC_DAIFMT_IB_IF   (4 << 8)
#define SND_SOC_DAIFMT_NB_IF   (2 << 8)
#define SND_SOC_DAIFMT_DSP_B         5
#define SND_SOC_DAIFMT_DSP_A         4
#define SND_SOC_DAIFMT_I2S           1
#define SND_SOC_DAIFMT_RIGHT_J       2
#define SND_SOC_DAIFMT_LEFT_J        3


int wm8904_set_fmt(unsigned int fmt)
{
	unsigned int aif1 = 0;
	unsigned int aif3 = 0;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		aif3 |= WM8904_LRCLK_DIR;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		aif1 |= WM8904_BCLK_DIR;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		aif1 |= WM8904_BCLK_DIR;
		aif3 |= WM8904_LRCLK_DIR;
		break;
	default:
		return -1;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_B:
		aif1 |= 0x3 | WM8904_AIF_LRCLK_INV;
	case SND_SOC_DAIFMT_DSP_A:
		aif1 |= 0x3;
		break;
	case SND_SOC_DAIFMT_I2S:
		aif1 |= 0x2;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		aif1 |= 0x1;
		break;
	default:
		return -1;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		/* frame inversion not valid for DSP modes */
		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			break;
		case SND_SOC_DAIFMT_IB_NF:
			aif1 |= WM8904_AIF_BCLK_INV;
			break;
		default:
			return -1;
		}
		break;

	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			break;
		case SND_SOC_DAIFMT_IB_IF:
			aif1 |= WM8904_AIF_BCLK_INV | WM8904_AIF_LRCLK_INV;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			aif1 |= WM8904_AIF_BCLK_INV;
			break;
		case SND_SOC_DAIFMT_NB_IF:
			aif1 |= WM8904_AIF_LRCLK_INV;
			break;
		default:
			return -1;
		}
		break;
	default:
		return -1;
	}

	wm8904_update_bits(WM8904_AUDIO_INTERFACE_1,
			    WM8904_AIF_BCLK_INV | WM8904_AIF_LRCLK_INV |
			    WM8904_AIF_FMT_MASK | WM8904_BCLK_DIR, aif1);
	wm8904_update_bits(WM8904_AUDIO_INTERFACE_3,
			    WM8904_LRCLK_DIR, aif3);

	return 0;
}


int wm8904_set_tdm_slot(unsigned int tx_mask,
			       unsigned int rx_mask, int slots, int slot_width)
{
	struct wm8904_priv *wm8904 = &priv;
	int aif1 = 0;

	/* Don't need to validate anything if we're turning off TDM */
	if (slots == 0)
		goto out;

	/* Note that we allow configurations we can't handle ourselves - 
	 * for example, we can generate clocks for slots 2 and up even if
	 * we can't use those slots ourselves.
	 */
	aif1 |= WM8904_AIFADC_TDM | WM8904_AIFDAC_TDM;

	switch (rx_mask) {
	case 3:
		break;
	case 0xc:
		aif1 |= WM8904_AIFADC_TDM_CHAN;
		break;
	default:
		return -1;
	}


	switch (tx_mask) {
	case 3:
		break;
	case 0xc:
		aif1 |= WM8904_AIFDAC_TDM_CHAN;
		break;
	default:
		return -1;
	}

out:
	wm8904->tdm_width = slot_width;
	wm8904->tdm_slots = slots / 2;

	wm8904_update_bits(WM8904_AUDIO_INTERFACE_1,
			    WM8904_AIFADC_TDM | WM8904_AIFADC_TDM_CHAN |
			    WM8904_AIFDAC_TDM | WM8904_AIFDAC_TDM_CHAN, aif1);

	return 0;
}

struct _fll_div {
	u16 fll_fratio;
	u16 fll_outdiv;
	u16 fll_clk_ref_div;
	u16 n;
	u16 k;
};

/* The size in bits of the FLL divide multiplied by 10
 * to allow rounding later */
#define FIXED_FLL_SIZE ((1 << 16) * 10)

static struct {
	unsigned int min;
	unsigned int max;
	u16 fll_fratio;
	int ratio;
} fll_fratios[] = {
	{       0,    64000, 4, 16 },
	{   64000,   128000, 3,  8 },
	{  128000,   256000, 2,  4 },
	{  256000,  1000000, 1,  2 },
	{ 1000000, 13500000, 0,  1 },
};

static int fll_factors(struct _fll_div *fll_div, unsigned int Fref,
		       unsigned int Fout)
{
	u64 Kpart;
	unsigned int K, Ndiv, Nmod, target;
	unsigned int div;
	int i;

	/* Fref must be <=13.5MHz */
	div = 1;
	fll_div->fll_clk_ref_div = 0;
	while ((Fref / div) > 13500000) {
		div *= 2;
		fll_div->fll_clk_ref_div++;

		if (div > 8) {
			printf("Can't scale %dMHz input down to <=13.5MHz\n",
			       Fref);
			return -1;
		}
	}

	printf("Fref=%u Fout=%u\n", Fref, Fout);

	/* Apply the division for our remaining calculations */
	Fref /= div;

	/* Fvco should be 90-100MHz; don't check the upper bound */
	div = 4;
	while (Fout * div < 90000000) {
		div++;
		if (div > 64) {
			printf("Unable to find FLL_OUTDIV for Fout=%uHz\n",
			       Fout);
			return -1;
		}
	}
	target = Fout * div;
	fll_div->fll_outdiv = div - 1;

	printf("Fvco=%dHz\n", target);

	/* Find an appropriate FLL_FRATIO and factor it out of the target */
	for (i = 0; i < ARRAY_SIZE(fll_fratios); i++) {
		if (fll_fratios[i].min <= Fref && Fref <= fll_fratios[i].max) {
			fll_div->fll_fratio = fll_fratios[i].fll_fratio;
			target /= fll_fratios[i].ratio;
			break;
		}
	}
	if (i == ARRAY_SIZE(fll_fratios)) {
		printf("Unable to find FLL_FRATIO for Fref=%uHz\n", Fref);
		return -1;
	}

	/* Now, calculate N.K */
	Ndiv = target / Fref;

	fll_div->n = Ndiv;
	Nmod = target % Fref;
	printf("Nmod=%d\n", Nmod);

	/* Calculate fractional part - scale up so we can round. */
	Kpart = FIXED_FLL_SIZE * (long long)Nmod;

	do_div(Kpart, Fref);

	K = Kpart & 0xFFFFFFFF;

	if ((K % 10) >= 5)
		K += 5;

	/* Move down to proper range now rounding is done */
	fll_div->k = K / 10;

	printf("N=%x K=%x FLL_FRATIO=%x FLL_OUTDIV=%x FLL_CLK_REF_DIV=%x\n",
		 fll_div->n, fll_div->k,
		 fll_div->fll_fratio, fll_div->fll_outdiv,
		 fll_div->fll_clk_ref_div);

	return 0;
}

int wm8904_set_fll(int fll_id, int source,
			  unsigned int Fref, unsigned int Fout)
{
	struct wm8904_priv *wm8904 = &priv;
	struct _fll_div fll_div;
	int ret, val;
	u16 clock2, fll1;

	/* Any change? */
	if (source == wm8904->fll_src && Fref == wm8904->fll_fref &&
	    Fout == wm8904->fll_fout)
		return 0;

	wm8904_i2c_read(WM8904_CLOCK_RATES_2, &clock2);

	if (Fout == 0) {
		printf("FLL disabled\n");

		wm8904->fll_fref = 0;
		wm8904->fll_fout = 0;

		/* Gate SYSCLK to avoid glitches */
		wm8904_update_bits(WM8904_CLOCK_RATES_2,
				    WM8904_CLK_SYS_ENA, 0);

		wm8904_update_bits(WM8904_FLL_CONTROL_1,
				    WM8904_FLL_OSC_ENA | WM8904_FLL_ENA, 0);

		goto out;
	}

	/* Validate the FLL ID */
	switch (source) {
	case WM8904_FLL_MCLK:
	case WM8904_FLL_LRCLK:
	case WM8904_FLL_BCLK:
		ret = fll_factors(&fll_div, Fref, Fout);
		if (ret != 0)
			return ret;
		break;

	case WM8904_FLL_FREE_RUNNING:
		printf("Using free running FLL\n");
		/* Force 12MHz and output/4 for now */
		Fout = 12000000;
		Fref = 12000000;

		memset(&fll_div, 0, sizeof(fll_div));
		fll_div.fll_outdiv = 3;
		break;

	default:
		printf("Unknown FLL ID %d\n", fll_id);
		return -1;
	}

	/* Save current state then disable the FLL and SYSCLK to avoid
	 * misclocking */
	wm8904_i2c_read(WM8904_FLL_CONTROL_1, &fll1);
	wm8904_update_bits(WM8904_CLOCK_RATES_2,
			    WM8904_CLK_SYS_ENA, 0);
	wm8904_update_bits(WM8904_FLL_CONTROL_1,
			    WM8904_FLL_OSC_ENA | WM8904_FLL_ENA, 0);

	/* Unlock forced oscilator control to switch it on/off */
	wm8904_update_bits(WM8904_CONTROL_INTERFACE_TEST_1,
			    WM8904_USER_KEY, WM8904_USER_KEY);

	if (fll_id == WM8904_FLL_FREE_RUNNING) {
		val = WM8904_FLL_FRC_NCO;
	} else {
		val = 0;
	}

	wm8904_update_bits(WM8904_FLL_NCO_TEST_1, WM8904_FLL_FRC_NCO,
			    val);
	wm8904_update_bits(WM8904_CONTROL_INTERFACE_TEST_1,
			    WM8904_USER_KEY, 0);

	switch (fll_id) {
	case WM8904_FLL_MCLK:
		wm8904_update_bits(WM8904_FLL_CONTROL_5,
				    WM8904_FLL_CLK_REF_SRC_MASK, 0);
		break;

	case WM8904_FLL_LRCLK:
		wm8904_update_bits(WM8904_FLL_CONTROL_5,
				    WM8904_FLL_CLK_REF_SRC_MASK, 1);
		break;

	case WM8904_FLL_BCLK:
		wm8904_update_bits(WM8904_FLL_CONTROL_5,
				    WM8904_FLL_CLK_REF_SRC_MASK, 2);
		break;
	}

	if (fll_div.k)
		val = WM8904_FLL_FRACN_ENA;
	else
		val = 0;
	wm8904_update_bits(WM8904_FLL_CONTROL_1,
			    WM8904_FLL_FRACN_ENA, val);

	wm8904_update_bits(WM8904_FLL_CONTROL_2,
			    WM8904_FLL_OUTDIV_MASK | WM8904_FLL_FRATIO_MASK,
			    (fll_div.fll_outdiv << WM8904_FLL_OUTDIV_SHIFT) |
			    (fll_div.fll_fratio << WM8904_FLL_FRATIO_SHIFT));

	wm8904_i2c_write(WM8904_FLL_CONTROL_3, fll_div.k);

	wm8904_update_bits(WM8904_FLL_CONTROL_4, WM8904_FLL_N_MASK,
			    fll_div.n << WM8904_FLL_N_SHIFT);

	wm8904_update_bits(WM8904_FLL_CONTROL_5,
			    WM8904_FLL_CLK_REF_DIV_MASK,
			    fll_div.fll_clk_ref_div 
			    << WM8904_FLL_CLK_REF_DIV_SHIFT);

	printf("FLL configured for %dHz->%dHz\n", Fref, Fout);

	wm8904->fll_fref = Fref;
	wm8904->fll_fout = Fout;
	wm8904->fll_src = source;

	/* Enable the FLL if it was previously active */
	wm8904_update_bits(WM8904_FLL_CONTROL_1,
			    WM8904_FLL_OSC_ENA, fll1);
	wm8904_update_bits(WM8904_FLL_CONTROL_1,
			    WM8904_FLL_ENA, fll1);

out:
	/* Reenable SYSCLK if it was previously active */
	wm8904_update_bits(WM8904_CLOCK_RATES_2,
			    WM8904_CLK_SYS_ENA, clock2);

	return 0;
}

int wm8904_digital_mute(int mute)
{
	int val;

	if (mute)
		val = WM8904_DAC_MUTE;
	else
		val = 0;

	wm8904_update_bits(WM8904_DAC_DIGITAL_1, WM8904_DAC_MUTE, val);

	return 0;
}

static enum soc_bias_level bias_level = SND_SOC_BIAS_OFF;

int wm8904_set_bias_level(enum soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		/* VMID resistance 2*50k */
		wm8904_update_bits(WM8904_VMID_CONTROL_0,
				    WM8904_VMID_RES_MASK,
				    0x1 << WM8904_VMID_RES_SHIFT);

		/* Normal bias current */
		wm8904_update_bits(WM8904_BIAS_CONTROL_0,
				    WM8904_ISEL_MASK, 2 << WM8904_ISEL_SHIFT);
		break;

	case SND_SOC_BIAS_STANDBY:
		if (bias_level == SND_SOC_BIAS_OFF) {
			/* Enable bias */
			wm8904_update_bits(WM8904_BIAS_CONTROL_0,
					    WM8904_BIAS_ENA, WM8904_BIAS_ENA);

			/* Enable VMID, VMID buffering, 2*5k resistance */
			wm8904_update_bits(WM8904_VMID_CONTROL_0,
					    WM8904_VMID_ENA |
					    WM8904_VMID_RES_MASK,
					    WM8904_VMID_ENA |
					    0x3 << WM8904_VMID_RES_SHIFT);

			/* Let VMID ramp */
			udelay(1000);
		}

		/* Maintain VMID with 2*250k */
		wm8904_update_bits(WM8904_VMID_CONTROL_0,
				    WM8904_VMID_RES_MASK,
				    0x2 << WM8904_VMID_RES_SHIFT);

		/* Bias current *0.5 */
		wm8904_update_bits(WM8904_BIAS_CONTROL_0,
				    WM8904_ISEL_MASK, 0);
		break;

	case SND_SOC_BIAS_OFF:
		/* Turn off VMID */
		wm8904_update_bits(WM8904_VMID_CONTROL_0,
				    WM8904_VMID_RES_MASK | WM8904_VMID_ENA, 0);

		/* Stop bias generation */
		wm8904_update_bits(WM8904_BIAS_CONTROL_0,
				    WM8904_BIAS_ENA, 0);


		break;
	}
	printf("%s: wm8904 bias level: %d\n", __func__, level);
	bias_level = level;
	return 0;
}

#define WM8904_RATES SNDRV_PCM_RATE_8000_96000

#define WM8904_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

int wm8904_probe()
{
	struct wm8904_priv *wm8904 = &priv;

	switch (wm8904->devtype) {
	case WM8904:
		break;
	case WM8912:
		break;
	default:
		printf("Unknown device type %d\n",
			wm8904->devtype);
		return -1;
	}


	return 0;
}

int wm8904_remove(void)
{
	return 0;
}

int wm8904_i2c_probe()
{
	struct wm8904_priv *wm8904 = &priv;
	u16 val;
	int ret, i;

	wm8904->devtype = WM8904;
	//wm8904->pdata = {};

	//powermanagement

	ret = wm8904_i2c_read(WM8904_SW_RESET_AND_ID, &val);
	if (ret < 0) {
		printf("Failed to read ID register: %d\n", ret);
		goto err_enable;
	}
	if (val != 0x8904) {
		printf("Device is not a WM8904, ID is %x\n", val);
		ret = -1;
		goto err_enable;
	}

	ret = wm8904_i2c_read(WM8904_REVISION, &val);
	if (ret < 0) {
		printf("Failed to read device revision: %d\n",
			ret);
		goto err_enable;
	}
	printf("revision %c\n", val + 'A');

	ret = wm8904_i2c_write(WM8904_SW_RESET_AND_ID, 0);
	if (ret < 0) {
		printf("Failed to issue reset: %d\n", ret);
		goto err_enable;
	}

	/* Change some default settings - latch VU and enable ZC */
	wm8904_update_bits(WM8904_ADC_DIGITAL_VOLUME_LEFT,
			   WM8904_ADC_VU, WM8904_ADC_VU);
	wm8904_update_bits(WM8904_ADC_DIGITAL_VOLUME_RIGHT,
			   WM8904_ADC_VU, WM8904_ADC_VU);
	wm8904_update_bits(WM8904_DAC_DIGITAL_VOLUME_LEFT,
			   WM8904_DAC_VU, WM8904_DAC_VU);
	wm8904_update_bits(WM8904_DAC_DIGITAL_VOLUME_RIGHT,
			   WM8904_DAC_VU, WM8904_DAC_VU);
	wm8904_update_bits(WM8904_ANALOGUE_OUT1_LEFT,
			   WM8904_HPOUT_VU | WM8904_HPOUTLZC,
			   WM8904_HPOUT_VU | WM8904_HPOUTLZC);
	wm8904_update_bits(WM8904_ANALOGUE_OUT1_RIGHT,
			   WM8904_HPOUT_VU | WM8904_HPOUTRZC,
			   WM8904_HPOUT_VU | WM8904_HPOUTRZC);
	wm8904_update_bits(WM8904_ANALOGUE_OUT2_LEFT,
			   WM8904_LINEOUT_VU | WM8904_LINEOUTLZC,
			   WM8904_LINEOUT_VU | WM8904_LINEOUTLZC);
	wm8904_update_bits(WM8904_ANALOGUE_OUT2_RIGHT,
			   WM8904_LINEOUT_VU | WM8904_LINEOUTRZC,
			   WM8904_LINEOUT_VU | WM8904_LINEOUTRZC);
	wm8904_update_bits(WM8904_CLOCK_RATES_0,
			   WM8904_SR_MODE, 0);

	/* Apply configuration from the platform data. */
	if (wm8904->pdata) {
		for (i = 0; i < WM8904_GPIO_REGS; i++) {
			if (!wm8904->pdata->gpio_cfg[i])
				continue;

			wm8904_update_bits(
					   WM8904_GPIO_CONTROL_1 + i,
					   0xffff,
					   wm8904->pdata->gpio_cfg[i]);
		}

		/* Zero is the default value for these anyway */
		for (i = 0; i < WM8904_MIC_REGS; i++)
			wm8904_update_bits(
					   WM8904_MIC_BIAS_CONTROL_0 + i,
					   0xffff,
					   wm8904->pdata->mic_cfg[i]);
	}

	/* Set Class W by default - this will be managed by the Class
	 * G widget at runtime where bypass paths are available.
	 */
	wm8904_update_bits(WM8904_CLASS_W_0,
			    WM8904_CP_DYN_PWR, WM8904_CP_DYN_PWR);

	/* Use normal bias source */
	wm8904_update_bits(WM8904_BIAS_CONTROL_0,
			    WM8904_POBCTRL, 0);

	if (ret != 0)
		return ret;

	return 0;

err_enable:
	return ret;
}

int wm8904_i2c_remove(void)
{
	return 0;
}

#define SND_SOC_DAPM_PRE_PMU        0x1
#define SND_SOC_DAPM_POST_PMU       0x2 
#define SND_SOC_DAPM_PRE_PMD        0x4
#define SND_SOC_DAPM_POST_PMD       0x8 

static int out_pga_event(int event, int aif)
{
	struct wm8904_priv *wm8904 = &priv;
	int reg;
	unsigned short val;
	int dcs_mask;
	int dcs_l, dcs_r;
	int dcs_l_reg, dcs_r_reg;
	int timeout;
	int pwr_reg;

	/* This code is shared between HP and LINEOUT; we do all our
	 * power management in stereo pairs to avoid latency issues so
	 * we reuse shift to identify which rather than strcmp() the
	 * name. */
	reg = aif;

	switch (reg) {
	case WM8904_ANALOGUE_HP_0:
		pwr_reg = WM8904_POWER_MANAGEMENT_2;
		dcs_mask = WM8904_DCS_ENA_CHAN_0 | WM8904_DCS_ENA_CHAN_1;
		dcs_r_reg = WM8904_DC_SERVO_8;
		dcs_l_reg = WM8904_DC_SERVO_9;
		dcs_l = 0;
		dcs_r = 1;
		break;
	case WM8904_ANALOGUE_LINEOUT_0:
		pwr_reg = WM8904_POWER_MANAGEMENT_3;
		dcs_mask = WM8904_DCS_ENA_CHAN_2 | WM8904_DCS_ENA_CHAN_3;
		dcs_r_reg = WM8904_DC_SERVO_6;
		dcs_l_reg = WM8904_DC_SERVO_7;
		dcs_l = 2;
		dcs_r = 3;
		break;
	default:
		return -1;
	}

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* Power on the PGAs */
		wm8904_update_bits(pwr_reg,
				    WM8904_HPL_PGA_ENA | WM8904_HPR_PGA_ENA,
				    WM8904_HPL_PGA_ENA | WM8904_HPR_PGA_ENA);

		/* Power on the amplifier */
		wm8904_update_bits(reg,
				    WM8904_HPL_ENA | WM8904_HPR_ENA,
				    WM8904_HPL_ENA | WM8904_HPR_ENA);


		/* Enable the first stage */
		wm8904_update_bits(reg,
				    WM8904_HPL_ENA_DLY | WM8904_HPR_ENA_DLY,
				    WM8904_HPL_ENA_DLY | WM8904_HPR_ENA_DLY);

		/* Power up the DC servo */
		wm8904_update_bits(WM8904_DC_SERVO_0,
				    dcs_mask, dcs_mask);

		/* Either calibrate the DC servo or restore cached state
		 * if we have that.
		 */
		if (wm8904->dcs_state[dcs_l] || wm8904->dcs_state[dcs_r]) {
			printf("Restoring DC servo state\n");

			wm8904_i2c_write(dcs_l_reg,
				      wm8904->dcs_state[dcs_l]);
			wm8904_i2c_write(dcs_r_reg,
				      wm8904->dcs_state[dcs_r]);

			wm8904_i2c_write(WM8904_DC_SERVO_1, dcs_mask);

			timeout = 20;
		} else {
			printf("Calibrating DC servo\n");

			wm8904_i2c_write(WM8904_DC_SERVO_1,
				dcs_mask << WM8904_DCS_TRIG_STARTUP_0_SHIFT);

			timeout = 500;
		}

		/* Wait for DC servo to complete */
		dcs_mask <<= WM8904_DCS_CAL_COMPLETE_SHIFT;
		do {
			wm8904_i2c_read(WM8904_DC_SERVO_READBACK_0, &val);
			if ((val & dcs_mask) == dcs_mask)
				break;

			udelay(1000);
		} while (--timeout);

		if ((val & dcs_mask) != dcs_mask)
			printf("DC servo timed out\n");
		else
			printf("DC servo ready\n");

		/* Enable the output stage */
		wm8904_update_bits(reg,
				    WM8904_HPL_ENA_OUTP | WM8904_HPR_ENA_OUTP,
				    WM8904_HPL_ENA_OUTP | WM8904_HPR_ENA_OUTP);
		break;

	case SND_SOC_DAPM_POST_PMU:
		/* Unshort the output itself */
		wm8904_update_bits(reg,
				    WM8904_HPL_RMV_SHORT |
				    WM8904_HPR_RMV_SHORT,
				    WM8904_HPL_RMV_SHORT |
				    WM8904_HPR_RMV_SHORT);

		break;

	case SND_SOC_DAPM_PRE_PMD:
		/* Short the output */
		wm8904_update_bits(reg,
				    WM8904_HPL_RMV_SHORT |
				    WM8904_HPR_RMV_SHORT, 0);
		break;

	case SND_SOC_DAPM_POST_PMD:
		/* Cache the DC servo configuration; this will be
		 * invalidated if we change the configuration. */
		wm8904_i2c_read(dcs_l_reg, &wm8904->dcs_state[dcs_l]);
		wm8904_i2c_read(dcs_r_reg, &wm8904->dcs_state[dcs_r]);

		wm8904_update_bits(WM8904_DC_SERVO_0,
				    dcs_mask, 0);

		/* Disable the amplifier input and output stages */
		wm8904_update_bits(reg,
				    WM8904_HPL_ENA | WM8904_HPR_ENA |
				    WM8904_HPL_ENA_DLY | WM8904_HPR_ENA_DLY |
				    WM8904_HPL_ENA_OUTP | WM8904_HPR_ENA_OUTP,
				    0);

		/* PGAs too */
		wm8904_update_bits(pwr_reg,
				    WM8904_HPL_PGA_ENA | WM8904_HPR_PGA_ENA,
				    0);
		break;
	}

	return 0;
}

int wm8904_init(
			int sampling_rate, int mclk_freq,
			int bits_per_sample, unsigned int channels)
{
	int ret = 0;

	wm8904_i2c_probe();
	printf("%s: wm8904 probe i2c success\n", __func__);
	ret =  wm8904_set_sysclk(WM8904_CLK_MCLK,
							mclk_freq, 1);
	if (ret < 0) {
		printf("%s: wm8904 codec set sys clock failed\n", __func__);
		return ret;
	}

	ret = wm8904_hw_params(0, sampling_rate,
						bits_per_sample, channels);

	if (ret == 0) {
		ret = wm8904_set_fmt(SND_SOC_DAIFMT_I2S |
						SND_SOC_DAIFMT_NB_NF |
						SND_SOC_DAIFMT_CBS_CFS);
	}else {
		printf("hw params failed\n");
	}
	out_pga_event(SND_SOC_DAPM_PRE_PMU, WM8904_ANALOGUE_HP_0);
	out_pga_event(SND_SOC_DAPM_POST_PMU, WM8904_ANALOGUE_HP_0);
	out_pga_event(SND_SOC_DAPM_PRE_PMU, WM8904_ANALOGUE_LINEOUT_0);
	out_pga_event(SND_SOC_DAPM_POST_PMU, WM8904_ANALOGUE_LINEOUT_0);
	wm8904_set_bias_level(SND_SOC_BIAS_OFF);
	wm8904_set_bias_level(SND_SOC_BIAS_STANDBY);
	wm8904_set_bias_level(SND_SOC_BIAS_PREPARE);
	wm8904_set_bias_level(SND_SOC_BIAS_ON);

	/*wm8904_update_bits(WM8904_CLOCK_RATES_2, WM8904_CLK_DSP_ENA, WM8904_CLK_DSP_ENA);
	wm8904_update_bits(WM8904_POWER_MANAGEMENT_6, WM8904_DACL_ENA, WM8904_DACL_ENA);
	wm8904_update_bits(WM8904_POWER_MANAGEMENT_6, WM8904_DACR_ENA, WM8904_DACR_ENA);
	wm8904_update_bits(WM8904_CHARGE_PUMP_0, WM8904_CP_ENA, WM8904_CP_ENA);
	wm8904_update_bits(WM8904_ANALOGUE_LINEOUT_0, WM8904_LINEOUTL_ENA_OUTP, WM8904_LINEOUTL_ENA_OUTP);
	wm8904_update_bits(WM8904_ANALOGUE_LINEOUT_0, WM8904_LINEOUTR_ENA_OUTP, WM8904_LINEOUTR_ENA_OUTP);
	wm8904_update_bits(WM8904_ANALOGUE_HP_0, WM8904_HPL_ENA_OUTP, WM8904_HPL_ENA_OUTP);
	wm8904_update_bits(WM8904_ANALOGUE_HP_0, WM8904_HPR_ENA_OUTP, WM8904_HPR_ENA_OUTP);
	wm8904_update_bits(WM8904_ANALOGUE_HP_0, WM8904_HPL_ENA_DLY, WM8904_HPL_ENA_DLY);
	wm8904_update_bits(WM8904_ANALOGUE_HP_0, WM8904_HPR_ENA_DLY, WM8904_HPR_ENA_DLY);
	wm8904_update_bits(WM8904_ANALOGUE_LINEOUT_0, WM8904_LINEOUTR_ENA_DLY, WM8904_LINEOUTR_ENA_DLY);
	wm8904_update_bits(WM8904_ANALOGUE_LINEOUT_0, WM8904_LINEOUTL_ENA_DLY, WM8904_LINEOUTL_ENA_DLY);
	//wm8904_update_bits(WM8904_DCS_ENA_CHAN_0 | WM8904_DCS_ENA_CHAN_1 | WM8904_DCS_ENA_CHAN_2 | WM8904_DCS_ENA_CHAN_3, WM8904_DCS_ENA_CHAN_0 | WM8904_DCS_ENA_CHAN_1 | WM8904_DCS_ENA_CHAN_2 | WM8904_DCS_ENA_CHAN_3);
	//wm8904_update_bits(WM8904_DCS_TRIG_STARTUP_0 | WM8904_DCS_TRIG_STARTUP_1 | WM8904_DCS_TRIG_STARTUP_2 | WM8904_DCS_TRIG_STARTUP_3, WM8904_DCS_TRIG_STARTUP_0 | WM8904_DCS_TRIG_STARTUP_1 | WM8904_DCS_TRIG_STARTUP_2 | WM8904_DCS_TRIG_STARTUP_3);
	wm8904_update_bits(WM8904_ANALOGUE_HP_0, WM8904_HPL_ENA_OUTP, WM8904_HPL_ENA_OUTP);
	wm8904_update_bits(WM8904_ANALOGUE_HP_0, WM8904_HPR_ENA_OUTP, WM8904_HPR_ENA_OUTP);
	wm8904_update_bits(WM8904_ANALOGUE_LINEOUT_0, WM8904_LINEOUTL_ENA_OUTP, WM8904_LINEOUTL_ENA_OUTP);
	wm8904_update_bits(WM8904_ANALOGUE_LINEOUT_0, WM8904_LINEOUTR_ENA_OUTP ,WM8904_LINEOUTR_ENA_OUTP);
	wm8904_update_bits(WM8904_ANALOGUE_HP_0, WM8904_HPL_RMV_SHORT, WM8904_HPL_RMV_SHORT);
	wm8904_update_bits(WM8904_ANALOGUE_HP_0, WM8904_HPR_RMV_SHORT, WM8904_HPR_RMV_SHORT);
	wm8904_update_bits(WM8904_ANALOGUE_LINEOUT_0, WM8904_LINEOUTL_RMV_SHORT, WM8904_LINEOUTL_RMV_SHORT);
	wm8904_update_bits(WM8904_ANALOGUE_LINEOUT_0, WM8904_LINEOUTR_RMV_SHORT, WM8904_LINEOUTR_RMV_SHORT);*/
	return ret;
}

