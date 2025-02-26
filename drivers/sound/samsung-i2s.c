/*
 * Copyright (C) 2012 Samsung Electronics
 * R. Chandrasekar <rcsekar@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clk.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/i2s-regs.h>
#include <asm/io.h>
#include <common.h>
#include <sound.h>
#include <i2s.h>

#define FIC_TX2COUNT(x)		(((x) >>  24) & 0xf)
#define FIC_TX1COUNT(x)		(((x) >>  16) & 0xf)
#define FIC_TXCOUNT(x)		(((x) >>  8) & 0xf)
#define FIC_RXCOUNT(x)		(((x) >>  0) & 0xf)
#define FICS_TXCOUNT(x)		(((x) >>  8) & 0x7f)

#define TIMEOUT_I2S_TX		100	/* i2s transfer timeout */

/*
 * Sets the frame size for I2S LR clock
 *
 * @param i2s_reg	i2s regiter address
 * @param rfs		Frame Size
 */
static void i2s_set_lr_framesize(struct i2s_reg *i2s_reg, unsigned int rfs)
{
	unsigned int mod = readl(&i2s_reg->mod);

	mod &= ~MOD_RCLK_MASK;

	switch (rfs) {
	case 768:
		mod |= MOD_RCLK_768FS;
		break;
	case 512:
		mod |= MOD_RCLK_512FS;
		break;
	case 384:
		mod |= MOD_RCLK_384FS;
		break;
	default:
		mod |= MOD_RCLK_256FS;
		break;
	}

	writel(mod, &i2s_reg->mod);
}

/*
 * Sets the i2s transfer control
 *
 * @param i2s_reg	i2s regiter address
 * @param on		1 enable tx , 0 disable tx transfer
 */
static void i2s_txctrl(struct i2s_reg *i2s_reg, int on)
{
	unsigned int con = readl(&i2s_reg->con);
	unsigned int mod = readl(&i2s_reg->mod) & ~MOD_MASK;

	if (on) {
		con |= CON_ACTIVE;
		con &= ~CON_TXCH_PAUSE;
	} else {
		con |=  CON_TXCH_PAUSE;
		con &= ~CON_ACTIVE;
	}

	writel(mod, &i2s_reg->mod);
	writel(con, &i2s_reg->con);
}

/*
 * set the bit clock frame size (in multiples of LRCLK)
 *
 * @param i2s_reg	i2s regiter address
 * @param bfs		bit Frame Size
 */
static void i2s_set_bitclk_framesize(struct i2s_reg *i2s_reg, unsigned bfs)
{
	unsigned int mod = readl(&i2s_reg->mod);

	mod &= ~MOD_BCLK_MASK;

	switch (bfs) {
	case 48:
		mod |= MOD_BCLK_48FS;
		break;
	case 32:
		mod |= MOD_BCLK_32FS;
		break;
	case 24:
		mod |= MOD_BCLK_24FS;
		break;
	case 16:
		mod |= MOD_BCLK_16FS;
		break;
	default:
		return;
	}
	writel(mod, &i2s_reg->mod);
}

/*
 * flushes the i2stx fifo
 *
 * @param i2s_reg	i2s regiter address
 * @param flush		Tx fifo flush command (0x00 - do not flush
 *				0x80 - flush tx fifo)
 */
void i2s_fifo(struct i2s_reg *i2s_reg, unsigned int flush)
{
	/* Flush the FIFO */
	setbits_le32(&i2s_reg->fic, flush);
	clrbits_le32(&i2s_reg->fic, flush);
}

/*
 * Set System Clock direction
 *
 * @param i2s_reg	i2s regiter address
 * @param dir		Clock direction
 *
 * @return		int value 0 for success, -1 in case of error
 */
int i2s_set_sysclk_dir(struct i2s_reg *i2s_reg, int dir)
{
	unsigned int mod = readl(&i2s_reg->mod);

	if (dir == SND_SOC_CLOCK_IN)
		mod |= MOD_CDCLKCON;
	else
		mod &= ~MOD_CDCLKCON;

	writel(mod, &i2s_reg->mod);

	return 0;
}

/*
 * Sets I2S Clcok format
 *
 * @param fmt		i2s clock properties
 * @param i2s_reg	i2s regiter address
 *
 * @return		int value 0 for success, -1 in case of error
 */
int i2s_set_fmt(struct i2s_reg *i2s_reg, unsigned int fmt)
{
	unsigned int mod = readl(&i2s_reg->mod);
	unsigned int tmp = 0;
	unsigned int ret = 0;

	/* Format is priority */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_RIGHT_J:
		tmp |= MOD_LR_RLOW;
		tmp |= MOD_SDF_MSB;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		tmp |= MOD_LR_RLOW;
		tmp |= MOD_SDF_LSB;
		break;
	case SND_SOC_DAIFMT_I2S:
		tmp |= MOD_SDF_IIS;
		break;
	default:
		debug("%s: Invalid format priority [0x%x]\n", __func__,
		      (fmt & SND_SOC_DAIFMT_FORMAT_MASK));
		return -1;
	}

	/*
	 * INV flag is relative to the FORMAT flag - if set it simply
	 * flips the polarity specified by the Standard
	 */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		if (tmp & MOD_LR_RLOW)
			tmp &= ~MOD_LR_RLOW;
		else
			tmp |= MOD_LR_RLOW;
		break;
	default:
		debug("%s: Invalid clock ploarity input [0x%x]\n", __func__,
		      (fmt & SND_SOC_DAIFMT_INV_MASK));
		return -1;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		tmp |= MOD_SLAVE;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		/* Set default source clock in Master mode */
		ret = i2s_set_sysclk_dir(i2s_reg, SND_SOC_CLOCK_OUT);
		if (ret != 0) {
			debug("%s:set i2s clock direction failed\n", __func__);
			return -1;
		}
		break;
	default:
		debug("%s: Invalid master selection [0x%x]\n", __func__,
		      (fmt & SND_SOC_DAIFMT_MASTER_MASK));
		return -1;
	}

	mod &= ~(MOD_SDF_MASK | MOD_LR_RLOW | MOD_SLAVE);
	mod |= tmp;
	writel(mod, &i2s_reg->mod);

	return 0;
}

/*
 * Sets the sample width in bits
 *
 * @param blc		samplewidth (size of sample in bits)
 * @param i2s_reg	i2s regiter address
 *
 * @return		int value 0 for success, -1 in case of error
 */
int i2s_set_samplesize(struct i2s_reg *i2s_reg, unsigned int blc)
{
	unsigned int mod = readl(&i2s_reg->mod);

	mod &= ~MOD_BLCP_MASK;
	mod &= ~MOD_BLC_MASK;

	switch (blc) {
	case 8:
		mod |= MOD_BLCP_8BIT;
		mod |= MOD_BLC_8BIT;
		break;
	case 16:
		mod |= MOD_BLCP_16BIT;
		mod |= MOD_BLC_16BIT;
		break;
	case 24:
		mod |= MOD_BLCP_24BIT;
		mod |= MOD_BLC_24BIT;
		break;
	default:
		debug("%s: Invalid sample size input [0x%x]\n",
		      __func__, blc);
		return -1;
	}
	writel(mod, &i2s_reg->mod);

	return 0;
}

int i2s_transfer_tx_data(struct i2stx_info *pi2s_tx, unsigned int *data,
				unsigned long data_size)
{
	int i;
	int start;
	struct i2s_reg *i2s_reg =
				(struct i2s_reg *)pi2s_tx->base_address;

	if (data_size < FIFO_LENGTH) {
		debug("%s : Invalid data size\n", __func__);
		return -1; /* invalid pcm data size */
	}

	/* fill the tx buffer before stating the tx transmit */
	for (i = 0; i < FIFO_LENGTH; i++)
		writel(*data++, &i2s_reg->txd);

	data_size -= FIFO_LENGTH;
	i2s_txctrl(i2s_reg, I2S_TX_ON);
	start = get_timer(0);
	while (data_size > 0) {
		if (!(CON_TXFIFO_FULL & (readl(&i2s_reg->con)))) {
			writel(*data++, &i2s_reg->txd);
			data_size--;
			start = get_timer(0);
		} else {
			if (get_timer(start) > TIMEOUT_I2S_TX) {
				i2s_txctrl(i2s_reg, I2S_TX_OFF);
				debug("%s: I2S Transfer Timeout\n", __func__);
				return -1;
			}
		}
	}
	i2s_txctrl(i2s_reg, I2S_TX_OFF);

	return 0;
}

int i2s_tx_init(struct i2stx_info *pi2s_tx)
{
	int ret;
	struct i2s_reg *i2s_reg =
				(struct i2s_reg *)pi2s_tx->base_address;
	if (pi2s_tx->id == 0) {
		/* Initialize GPIO for I2S-0 */
		exynos_pinmux_config(PERIPH_ID_I2S0, 0);

		/* Set EPLL Clock */
		ret = set_epll_clk(pi2s_tx->samplingrate * pi2s_tx->rfs * 4);
	} else if (pi2s_tx->id == 1) {
		/* Initialize GPIO for I2S-1 */
		exynos_pinmux_config(PERIPH_ID_I2S1, 0);

		/* Set EPLL Clock */
		ret = set_epll_clk(pi2s_tx->audio_pll_clk);
	} else {
		debug("%s: unsupported i2s-%d bus\n", __func__, pi2s_tx->id);
		return -1;
	}

	if (ret != 0) {
		debug("%s: epll clock set rate failed\n", __func__);
		return -1;
	}

	/* Select Clk Source for Audio 0 or 1 */
	ret = set_i2s_clk_source(pi2s_tx->id);
	if (ret == -1) {
		debug("%s: unsupported clock for i2s-%d\n", __func__,
		      pi2s_tx->id);
		return -1;
	}

	if (pi2s_tx->id == 0) {
		/*Reset the i2s module */
		writel(CON_RESET, &i2s_reg->con);

		writel(MOD_OP_CLK | MOD_RCLKSRC, &i2s_reg->mod);
		/* set i2s prescaler */
		writel(PSREN | PSVAL, &i2s_reg->psr);
	} else {
		/* Set Prescaler to get MCLK */
		ret = set_i2s_clk_prescaler(pi2s_tx->audio_pll_clk,
				(pi2s_tx->samplingrate * (pi2s_tx->rfs)),
				pi2s_tx->id);
	}
	if (ret == -1) {
		debug("%s: unsupported prescalar for i2s-%d\n", __func__,
		      pi2s_tx->id);
		return -1;
	}

	/* Configure I2s format */
	ret = i2s_set_fmt(i2s_reg, (SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			  SND_SOC_DAIFMT_CBM_CFM));
	if (ret == 0) {
		i2s_set_lr_framesize(i2s_reg, pi2s_tx->rfs);
		ret = i2s_set_samplesize(i2s_reg, pi2s_tx->bitspersample);
		if (ret != 0) {
			debug("%s:set sample rate failed\n", __func__);
			return -1;
		}

		i2s_set_bitclk_framesize(i2s_reg, pi2s_tx->bfs);
		/* disable i2s transfer flag and flush the fifo */
		i2s_txctrl(i2s_reg, I2S_TX_OFF);
		i2s_fifo(i2s_reg, FIC_TXFLUSH);
	} else {
		debug("%s: failed\n", __func__);
	}

	return ret;
}
