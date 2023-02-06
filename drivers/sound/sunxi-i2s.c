#include <common.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#include <asm/arch/dma.h>
#include <smc.h>
#include <i2s.h>

struct sunxi_i2s_reg {
	u32 ctl;	/* base + 0 , Control register */
	u32 fmt0;
	u32 fmt1;
	u32 ista;
	u32 rxfifo; // 0x10
	u32 fctl; 
	u32 fsta;
	u32 intr;
	u32 txfifo; //0x20
	u32 clkd;
	u32 txcnt;
	u32 rxcnt;
	u32 chcfg; // 0x30
	u32 tx0chsel; //0x34
	u32 tx1chsel; 
	u32 tx2chsel; 
	u32 tx3chsel; //0x40
	u32 tx0chmap;
	u32 tx1chmap;
	u32 tx2chmap;
	u32 tx3chmap; //0x50
	u32 rxchsel; 
	u32 rxchmap; //0x58
};


#define FIC_TX2COUNT(x)		(((x) >>  24) & 0xf)
#define FIC_TX1COUNT(x)		(((x) >>  16) & 0xf)
#define FIC_TXCOUNT(x)		(((x) >>  8) & 0xf)
#define FIC_RXCOUNT(x)		(((x) >>  0) & 0xf)
#define FICS_TXCOUNT(x)		(((x) >>  8) & 0x7f)

#define TIMEOUT_I2S_TX		10000	/* i2s transfer timeout */

/*
 * Sets the i2s transfer control
 *
 * @param i2s_reg	i2s regiter address
 * @param on		1 enable tx , 0 disable tx transfer
 */
#define TX_DRQ					  (7)
#define TX_CNT					   0

#define SDO_EN								 	(8)
#define ASS									 (6)
#define MS									  (5)
#define PCM									 (4)
#define LOOP								   	(3)
#define TXEN								   	(2)
#define RXEN								   	(1)
#define GEN									 (0)
#define HUB_EN								 (31)
#define FTX									 (25)
#define FRX									 (24)
#define TXTL								   	(12)
#define RXTL								   	(4)
#define TXIM								   	(2)
#define RXOM								   	(0)

/*
*	  SUNXI_DA_FAT0
*	  I2S_AP Format Reg
*	  DA_FAT0:codecbase+0x04
*/
#define LRCP								   	(7)
#define BCP									 (6)
#define SR									  (4)
#define WSS									 (2)
#define FMT									 (0)

u32 codec_rdreg(void __iomem *address) {
	return readl(address);
}

void codec_wrreg(void __iomem *address,u32 val)
{
	writel(val, address);
}

u32 codec_wrreg_bits(void __iomem *address, u32 mask,u32 value)
{
	u32 old, new;

	old	= codec_rdreg(address);
	new	= (old & ~mask) | value;
	codec_wrreg(address,new);

	return 0;
}

u32 codec_wr_control(void __iomem *reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	codec_wrreg_bits(reg, mask, reg_val);
	return 0;
}

#define TX_CH0_MAP								  (0)
#define TX_CHSEL									(0)

#define SUNXI_TXCHCFG_TX_SLOT_NUM				(7<<0)
#define SUNXI_DAUDIOTXn_CHEN(v)					((v)<<4)
#define SUNXI_DAUDIOTXn_CHSEL(v)					((v)<<0)
#define CHEN_MASK								0xff
#define CHSEL_MASK								0x7
#define SUNXI_TXCHANMAP_DEFAULT													 (0x76543210)
#define SUNXI_DAUDIOCTL_TXEN									(1<<2)


static int i2s_prepare(struct sunxi_i2s_reg *i2s_reg) {
	u32 reg_val;
	reg_val = readl(&i2s_reg->chcfg);
	reg_val &= ~(SUNXI_TXCHCFG_TX_SLOT_NUM<<0);
	reg_val |= (2-1)<<0;
	writel(reg_val, &i2s_reg->chcfg);

	reg_val = readl(&i2s_reg->tx0chsel);
	reg_val &= ~SUNXI_DAUDIOTXn_CHEN(CHEN_MASK);
	reg_val &= ~SUNXI_DAUDIOTXn_CHSEL(CHSEL_MASK);
	reg_val |= SUNXI_DAUDIOTXn_CHEN((CHEN_MASK>>(2)));
	reg_val |= SUNXI_DAUDIOTXn_CHSEL(2-1);
	writel(reg_val, &i2s_reg->tx0chsel);

	reg_val = SUNXI_TXCHANMAP_DEFAULT;
	writel(reg_val, &i2s_reg->tx0chmap);
	/*clear TX counter*/
	writel(0, &i2s_reg->txcnt);

	/* DAUDIO TX ENABLE */
	reg_val = readl(&i2s_reg->ctl);
	reg_val |= SUNXI_DAUDIOCTL_TXEN;
	writel(reg_val, &i2s_reg->ctl);
	
	return 0;
}

static void i2s_txctrl(struct sunxi_i2s_reg *i2s_reg, int on)
{
	u32 reg_val;
	u32 hub_en = 0;
	/*flush TX FIFO*/
	reg_val = readl(&i2s_reg->fctl);
	reg_val |= (1<<25);
	writel(reg_val, &i2s_reg->fctl);
	/*clear TX counter*/
	writel(0, &i2s_reg->txcnt);

	if (on) {
		/* enable DMA DRQ mode for play */
		reg_val = readl(&i2s_reg->intr);
		reg_val |= (1<<7);
		writel(reg_val, &i2s_reg->intr);
	} else {
		/* DISBALE dma DRQ mode */
		reg_val = readl(&i2s_reg->intr);
		reg_val &= ~(1<<7);
		writel(reg_val, &i2s_reg->intr);

		/*DISABLE TXEN*/
		reg_val = readl(&i2s_reg->ctl);
		reg_val &= ~(1<<2);
		writel(reg_val, &i2s_reg->ctl);
	}
	if (hub_en) {
		reg_val = readl(&i2s_reg->fctl);
		reg_val |= (1<<31);
		writel(reg_val, &i2s_reg->fctl);
	} else {
		reg_val = readl(&i2s_reg->fctl);
		reg_val &= ~(1<<31);
		writel(reg_val, &i2s_reg->fctl);
	}
}


/*
 * flushes the i2stx fifo
 *
 * @param i2s_reg	i2s regiter address
 * @param flush		Tx fifo flush command (0x00 - do not flush
 *				0x80 - flush tx fifo)
 */
void i2s_fifo(struct sunxi_i2s_reg *i2s_reg, unsigned int flush)
{
	/*flush TX FIFO*/
	codec_wr_control(&i2s_reg->ctl, 0x1, FTX, flush);
}

#define SUNXI_DAUDIOCTL_LRCKOUT								 (1<<17)
#define SUNXI_DAUDIOCTL_BCLKOUT								 (1<<18)
#define SUNXI_DAUDIOTXn_OFFSET(v)					((v)<<12)
#define SUNXI_DAUDIORXCHSEL_RXOFFSET(v)				((v)<<12)
#define SUNXI_DAUDIOFAT0_BCLK_POLAYITY						  (1<<7)
#define SUNXI_DAUDIOFAT0_LRCK_POLAYITY						  (1<<19)
#define SUNXI_DAUDIOCTL_GEN									 (1<<0)
#define SUNXI_DAUDIOCTL_SDO0EN								  (1<<8)
#define SUNXI_DAUDIOCLKD_MCLKOEN								(1<<8)
#define SUNXI_DAUDIOCLKD_MCLKDIV(v)							 ((v)<<0)

int i2s_global_enable(struct sunxi_i2s_reg *i2s_reg,bool on)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;

	reg_val = readl(&i2s_reg->ctl);
	reg_val1 = readl(&i2s_reg->clkd);
	if (!on) {
		reg_val &= ~SUNXI_DAUDIOCTL_GEN;
		reg_val &= ~SUNXI_DAUDIOCTL_SDO0EN;
		reg_val1 &= ~SUNXI_DAUDIOCLKD_MCLKOEN;
	} else {
		reg_val |= SUNXI_DAUDIOCTL_GEN;
		reg_val |= SUNXI_DAUDIOCTL_SDO0EN;
		reg_val1 |= SUNXI_DAUDIOCLKD_MCLKOEN;
		reg_val1 |= SUNXI_DAUDIOCLKD_MCLKDIV(1);
	}
	writel(reg_val, &i2s_reg->ctl);
	writel(reg_val1, &i2s_reg->clkd);

	return 0;
}

int i2s_set_fmt(struct sunxi_i2s_reg *i2s_reg, u32 fmt) {
	u32 reg_val = 0;
	u32 reg_val1 = 0;
	u32 reg_val2 = 0;
	/* master or slave selection */
	reg_val = readl(&i2s_reg->ctl);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master, ap is slave*/
			reg_val &= ~SUNXI_DAUDIOCTL_LRCKOUT;
			reg_val &= ~SUNXI_DAUDIOCTL_BCLKOUT;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave,ap is master*/
			reg_val |= SUNXI_DAUDIOCTL_LRCKOUT;
			reg_val |= SUNXI_DAUDIOCTL_BCLKOUT;
			break;
		default:
			printf("unknwon master/slave format\n");
			return -1;
	}
	writel(reg_val, &i2s_reg->ctl);
	/* pcm or tdm mode selection */
	reg_val = readl(&i2s_reg->ctl);
	reg_val1 = readl(&i2s_reg->tx0chsel);
	reg_val2 = readl(&i2s_reg->rxchsel);
	reg_val &= ~(3<<4);
	reg_val1 &= ~(SUNXI_DAUDIOTXn_OFFSET(3));
	reg_val2 &= ~(SUNXI_DAUDIORXCHSEL_RXOFFSET(3));
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:		/* i2s mode */
			reg_val  |= (1<<4);
			reg_val1 |= SUNXI_DAUDIOTXn_OFFSET(1);
			reg_val2 |= SUNXI_DAUDIORXCHSEL_RXOFFSET(1);
			break;
		case SND_SOC_DAIFMT_RIGHT_J:	/* Right Justified mode */
			reg_val  |= (2<<4);
			break;
		case SND_SOC_DAIFMT_LEFT_J:	 /* Left Justified mode */
			reg_val  |= (1<<4);
			reg_val1 |= SUNXI_DAUDIOTXn_OFFSET(0);
			reg_val2 |= SUNXI_DAUDIORXCHSEL_RXOFFSET(0);
			break;
		case SND_SOC_DAIFMT_DSP_A:	  /* L data msb after FRM LRC */
			reg_val  |= (0<<4);
			break;
		case SND_SOC_DAIFMT_DSP_B:	  /* L data msb during FRM LRC */
			reg_val  |= (0<<4);
			break;
		default:
			return -1;
	}
	writel(reg_val, &i2s_reg->ctl);
	writel(reg_val1, &i2s_reg->tx0chsel);
	writel(reg_val2, &i2s_reg->rxchsel);
	/* DAI signal inversions */
	reg_val1 = readl(&i2s_reg->fmt0);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:	 /* normal bit clock + frame */
			reg_val1 &= ~SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			reg_val1 &= ~SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			break;
		case SND_SOC_DAIFMT_NB_IF:	 /* normal bclk + inv frm */
			reg_val1 |= SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			reg_val1 &= ~SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			break;
		case SND_SOC_DAIFMT_IB_NF:	 /* invert bclk + nor frm */
			reg_val1 &= ~SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			reg_val1 |= SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			break;
		case SND_SOC_DAIFMT_IB_IF:	 /* invert bclk + frm */
			reg_val1 |= SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			reg_val1 |= SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			break;
	}
	writel(reg_val1, &i2s_reg->fmt0);

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
int i2s_hw_params(struct sunxi_i2s_reg *i2s_reg, u32 bits_per_sample)
{
	u32 reg_val = 0;
	u32 sample_resolution = 0;
	switch (bits_per_sample)
	{
	case 16:
		sample_resolution = 16;
		break;
	case 20:
		sample_resolution = 24;
		break;
	case 24:
		sample_resolution = 24;
		break;
	case 32:
		sample_resolution = 24;
		break;
	default:
		return -1;
	}
	reg_val = readl(&i2s_reg->fmt0);
	reg_val &= ~(7<<4);
	if(sample_resolution== 16)
		reg_val |= (3<<4);
	else if(sample_resolution == 20)
		reg_val |= (4<<4);
	else
		reg_val |= (5<<4);
	writel(reg_val, &i2s_reg->fmt0);

	if (sample_resolution == 24) {
		reg_val = readl(&i2s_reg->fctl);
		reg_val &= ~(1<<2);
		writel(reg_val, &i2s_reg->fctl);
	} else {
		reg_val = readl(&i2s_reg->fctl);
		reg_val |= (1<<2);
		writel(reg_val, &i2s_reg->fctl);
	}
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
int i2s_set_samplesize(struct sunxi_i2s_reg *i2s_reg, unsigned int blc)
{
	int rs_value = 0;
	int sample_resolution = 16;
	switch (blc)
	{
		case 16:
			sample_resolution = 16;
			break;
		case 20:
			sample_resolution = 24;
			break;
		case 24:
			sample_resolution = 24;
			break;
		case 32:
			sample_resolution = 24;
			break;
		default:
			return -1;
	}

	/* sample rate */
	switch(sample_resolution)
	{
		case 16: rs_value = 0;
			break;
		case 20: rs_value = 1;
			break;
		case 24: rs_value = 2;
			break;
		default:
			return -1;
	}
	codec_wr_control(&i2s_reg->fmt0, 0x3, SR, rs_value);

	if(sample_resolution == 24)
		codec_wr_control(&i2s_reg->fctl, 0xf, RXOM, 0x1);
	else
		codec_wr_control(&i2s_reg->fctl, 0xf, RXOM, 0x5);

	return 0;
}

int i2s_transfer_tx_data(struct i2stx_info *pi2s_tx, unsigned int *data,
				unsigned long data_size)
{
	int i;
	ulong start;
	u32 istat;
	struct sunxi_i2s_reg *i2s_reg =
				(struct sunxi_i2s_reg *)pi2s_tx->base_address;
	if (data_size < FIFO_LENGTH) {
		debug("%s : Invalid data size\n", __func__);
		return -1; /* invalid pcm data size */
	}
	/* fill the tx buffer before stating the tx transmit */
	i2s_txctrl(i2s_reg, I2S_TX_ON);
		
	for (i = 0; i < FIFO_LENGTH; i++)
		writel(*data++, &i2s_reg->txfifo);
	i2s_prepare(i2s_reg);
	data_size -= FIFO_LENGTH;
	start = get_timer(0);
	while (data_size > 0) {
		istat = readl(&i2s_reg->ista);
		if ((0x10 & (istat))) {
			writel(*data++, &i2s_reg->txfifo);
			data_size--;
			start = get_timer(0);
		} else {
			if (get_timer(start) > TIMEOUT_I2S_TX) {
				i2s_txctrl(i2s_reg, I2S_TX_OFF);
				printf("%s: I2S Transfer Timeout\n", __func__);
				return -1;
			}
		}
	   ;
	}
	i2s_txctrl(i2s_reg, I2S_TX_OFF);
	return 0;
}

int i2s_dma_config_start(struct sunxi_i2s_reg *i2s_reg, dma_addr_t addr,__u32 length, ulong dma_chan)
{
	int ret = 0;
	sunxi_dma_setting_t dma_set;

	dma_set.loop_mode = 1;
	dma_set.wait_cyc = 8;
	dma_set.data_block_size = 0;
	dma_set.cfg.src_drq_type = DMAC_CFG_SRC_TYPE_DRAM;
	dma_set.cfg.src_addr_mode = DMAC_CFG_SRC_ADDR_TYPE_LINEAR_MODE;
	dma_set.cfg.src_burst_length = DMAC_CFG_SRC_1_BURST; //8
	dma_set.cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_16BIT;
	dma_set.cfg.reserved0 = 0;

	dma_set.cfg.dst_drq_type = DMAC_CFG_DEST_TYPE_I2S_0_TX;
	dma_set.cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_IO_MODE;
	dma_set.cfg.dst_burst_length = DMAC_CFG_DEST_1_BURST; //8
	dma_set.cfg.dst_data_width = DMAC_CFG_DEST_DATA_WIDTH_16BIT;
	dma_set.cfg.reserved1 = 0;

	if ( sunxi_dma_setting(dma_chan, &dma_set) < 0) {
		printf("uboot: sunxi_dma_setting for nand faild!\n");
		return -1;
	}
	ret = sunxi_dma_start(dma_chan, addr, (uint)(&i2s_reg->txfifo), length);
	if (ret < 0) {
		printf("uboot: sunxi_dma_start for nand faild!\n");
		return -1;
	}

	return 0;
}


int i2s_prepare_tx_data(struct i2stx_info *pi2s_tx, unsigned int *data,
				unsigned long data_size)
{
	ulong dma_chan;
	struct sunxi_i2s_reg *i2s_reg =
				(struct sunxi_i2s_reg *)pi2s_tx->base_address;
	if (data_size < FIFO_LENGTH) {
		debug("%s : Invalid data size\n", __func__);
		return -1; /* invalid pcm data size */
	}
	/* fill the tx buffer before stating the tx transmit */
	i2s_txctrl(i2s_reg, I2S_TX_ON);
	/*
	for (i = 0; i < FIFO_LENGTH; i++)
		writel(*data++, &i2s_reg->txfifo);
	*/
	u32 dma_gating, dma_gating_reset;
	dma_gating = readl(A64_CCU + 0x60);
	dma_gating_reset = readl(A64_CCU + 0x2C0);
	writel(dma_gating_reset | (1 << 6), A64_CCU + 0x2C0);
	writel(dma_gating | (1 << 6), A64_CCU + 0x60);
	
	i2s_prepare(i2s_reg);
	sunxi_dma_init();
	dma_chan = sunxi_dma_request(0);
	i2s_dma_config_start(i2s_reg, (uint)data, data_size, dma_chan);
	//data_size -= FIFO_LENGTH;
	return 0;
}

#define MCLKO_EN							   	(8)
#define BCLKDIV								 (4)
#define MCLKDIV								 (0)

#define WSS									 (2)


static int sunxi_i2s_set_clkdiv(struct sunxi_i2s_reg *i2s_reg, int samplerate, u32 pcm_lrck_period, u32 slot_width)
{
	u32 reg_val = 0;
	u32 mclk_div = 0;
	u32 bclk_div = 0;

	reg_val = readl(&i2s_reg->clkd);
	/*i2s mode*/
	switch (samplerate) {
		case 192000:
		case 96000:
		case 48000:
		case 32000:
		case 24000:
		case 12000:
		case 16000:
		case 8000:
			bclk_div = ((24576000/samplerate)/(2*pcm_lrck_period));
			mclk_div = 1;
		break;
		default:
			bclk_div = ((22579200/samplerate)/(2*pcm_lrck_period));
			mclk_div = 1;
		break;
	}

	switch(mclk_div)
	{
		case 1: mclk_div = 1;
			break;
		case 2: mclk_div = 2;
			break;
		case 4: mclk_div = 3;
			break;
		case 6: mclk_div = 4;
			break;
		case 8: mclk_div = 5;
			break;
		case 12: mclk_div = 6;
			 break;
		case 16: mclk_div = 7;
			 break;
		case 24: mclk_div = 8;
			 break;
		case 32: mclk_div = 9;
			 break;
		case 48: mclk_div = 10;
			 break;
		case 64: mclk_div = 11;
			 break;
		case 96: mclk_div = 12;
			 break;
		case 128: mclk_div = 13;
			 break;
		case 176: mclk_div = 14;
			 break;
		case 192: mclk_div = 15;
			 break;
	}

	reg_val &= ~(0xf<<0);
	reg_val |= mclk_div<<0;
	switch(bclk_div)
	{
		case 1: bclk_div = 1;
			break;
		case 2: bclk_div = 2;
			break;
		case 4: bclk_div = 3;
			break;
		case 6: bclk_div = 4;
			break;
		case 8: bclk_div = 5;
			break;
		case 12: bclk_div = 6;
			break;
		case 16: bclk_div = 7;
			break;
		case 24: bclk_div = 8;
			break;
		case 32: bclk_div = 9;
			break;
		case 48: bclk_div = 10;
			break;
		case 64: bclk_div = 11;
			break;
		case 96: bclk_div = 12;
			break;
		case 128: bclk_div = 13;
			break;
		case 176: bclk_div = 14;
			break;
		case 192:bclk_div = 15;
	}
	reg_val &= ~(0xf<<4);
	reg_val |= bclk_div<<4;
	writel(reg_val, &i2s_reg->clkd);

	reg_val = readl(&i2s_reg->fmt0);
	reg_val &= ~(0x3ff<<20);
	reg_val &= ~(0x3ff<<8);
	reg_val |= (pcm_lrck_period-1)<<8;
	reg_val |= (1-1)<<20;
	writel(reg_val, &i2s_reg->fmt0);

	reg_val = readl(&i2s_reg->fmt0);
	reg_val &= ~(7<<0);
	if(slot_width == 16)
		reg_val |= (3<<0);
	else if(slot_width == 20)
		reg_val |= (4<<0);
	else if(slot_width == 24)
		reg_val |= (5<<0);
	else if(slot_width == 28)
		reg_val |= (6<<0);
	else
		reg_val |= (7<<0);

	/*pcm mode
	*	(Only apply in PCM mode) LRCK width
	*	0: LRCK = 1 BCLK width(short frame)
	*	1: LRCK = 2 BCLK width(long frame)
	*/
	if(0)
		reg_val |= (1<<30);
	else
		reg_val &= ~(1<<30);
	writel(reg_val, &i2s_reg->fmt0);

	reg_val = readl(&i2s_reg->fmt1);
	reg_val |= 0<<7;
	reg_val |= 0<<6;
	/*linear or u/a-law*/
	reg_val &= ~(0xf<<0);
	reg_val |= (0)<<2;
	reg_val |= (0)<<0;
	writel(reg_val, &i2s_reg->fmt1);

	return 0;
}

static int gpio_cfg(u32 port, u32 port_num, u32 val) {
	u32 reg_val;
	u32 port_num_func = port_num >> 3;
	
	volatile __u32	  *tmp_group_func_addr = NULL;
	tmp_group_func_addr = PIO_REG_CFG(port, port_num_func);
	
	reg_val = GPIO_REG_READ(tmp_group_func_addr);
	reg_val &= ~(0x07 << (((port_num - (port_num_func<<3))<<2)));
	reg_val |=  val << (((port_num - (port_num_func<<3))<<2));
	
	GPIO_REG_WRITE(tmp_group_func_addr, reg_val);
		
	reg_val = PIO_REG_CFG_VALUE(port, port_num_func);
	return 0;
}

static int set_pll_clk(u32 clk) {
	//writel(0x1, A64_CCU + 0x2f0);
	writel(0x2, A64_CCU + 0x320);
	writel(0x80035514, A64_CCU + 0x08);
	while ((0x10000000 & (readl(A64_CCU + 0x08))) == 0) {
		udelay(10);
	}
	writel(0x80030000, A64_CCU + 0xB0);
	u32 gating_i2s, gating_i2s_reset;
	gating_i2s_reset = readl(A64_CCU + 0x2d0);
	writel(gating_i2s_reset | (1 << 12), A64_CCU + 0x2d0);
	gating_i2s = readl(A64_CCU + 0x68);
	writel(gating_i2s | (1 << 12), A64_CCU + 0x68);
	return 0;
}

int i2s_tx_init(struct i2stx_info *pi2s_tx)
{
	int ret;
	struct sunxi_i2s_reg *i2s_reg =
				(struct sunxi_i2s_reg *)pi2s_tx->base_address;
	gpio_cfg(2, 7, 3);
	gpio_cfg(2, 6, 3);
	gpio_cfg(2, 5, 3);
	gpio_cfg(2, 4, 3);
	gpio_cfg(2, 3, 3);
	if (pi2s_tx->id == 0) {
		/* Initialize GPIO for I2S-0 */

		/* Set EPLL Clock */
		ret = set_pll_clk(pi2s_tx->audio_pll_clk);
	} else if (pi2s_tx->id == 1) {
		/* Set EPLL Clock */
		ret = set_pll_clk(pi2s_tx->audio_pll_clk);
	} else {
		printf("%s: unsupported i2s-%d bus\n", __func__, pi2s_tx->id);
		return -1;
	}

	if (ret != 0) {
		printf("%s: epll clock set rate failed\n", __func__);
		return -1;
	}

	/* Select Clk Source for Audio 0 or 1 */
	//ret = set_i2s_clk_source(pi2s_tx->id);
	if (ret == -1) {
		printf("%s: unsupported clock for i2s-%d\n", __func__,
			  pi2s_tx->id);
		return -1;
	}
	/* Configure I2s format */
	ret = i2s_set_fmt(i2s_reg, (SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			  SND_SOC_DAIFMT_CBS_CFS));
	if (ret == 0) {
		//i2s_set_lr_framesize(i2s_reg, pi2s_tx->rfs);
		i2s_hw_params(i2s_reg, pi2s_tx->bitspersample);
		ret = sunxi_i2s_set_clkdiv(i2s_reg, pi2s_tx->samplingrate, 32, 32);
		if (ret != 0) {
			printf("%s:set sample rate failed\n", __func__);
			return -1;
		}
		/* disable i2s transfer flag and flush the fifo */
		i2s_global_enable(i2s_reg, 0);
		i2s_txctrl(i2s_reg, I2S_TX_OFF);
		i2s_fifo(i2s_reg, 1);
		i2s_global_enable(i2s_reg, 1);
	} else {
		printf("%s: failed\n", __func__);
	}

	return ret;
}
