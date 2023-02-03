#include <malloc.h>
#include <sound.h>
#include <common.h>
#include <asm/io.h>
#include <i2c.h>
#include <i2s.h>
#include "wm8904.h"
/* defines */
#define SOUND_400_HZ 440
#define SOUND_BITS_IN_BYTE 8

static struct i2stx_info g_i2stx_pri;

int sound_init(const void *blob) {
	int ret;
	ret = 0;
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	u32 status;
	u16 answer;
	status = i2c_read(0x1a, 0x0, 2, (u8 *)&answer, 2);
	printf("answer: %d, status %d", answer, status);
 
	struct i2stx_info *pi2s_tx = &g_i2stx_pri;

/* Get the I2S Values */
	pi2s_tx->rfs=32;
	pi2s_tx->bfs=1;
	pi2s_tx->audio_pll_clk=24576000;
	pi2s_tx->samplingrate=48000;
	pi2s_tx->bitspersample=16;
	pi2s_tx->channels=2;
	pi2s_tx->base_address=0x1c22000;
	pi2s_tx->id=0;
	wm8904_init(pi2s_tx->samplingrate, pi2s_tx->audio_pll_clk,
		pi2s_tx->bitspersample, pi2s_tx->channels);
	
	ret = i2s_tx_init(pi2s_tx);
	if (ret) {
		printf("%s: Failed to init i2c transmit: ret=%d\n", __func__,
		      ret);
		return ret;
	}

	return ret;
}

int sound_play_dma(uint32_t msec, uint32_t frequency) {
	unsigned int *data;
	unsigned long data_size;
	unsigned int ret = 0;
	if (frequency == 0) { 
		udelay(msec * 1000); 
		return 0;
	}

	/*Buffer length computation */
	data_size = g_i2stx_pri.samplingrate * g_i2stx_pri.channels;
	data_size *= (g_i2stx_pri.bitspersample / SOUND_BITS_IN_BYTE);
	data = malloc(data_size);

	if (data == NULL) {
		debug("%s: malloc failed\n", __func__);
		return -1;
	}

	sound_create_square_wave((unsigned short *)data,
				 data_size / sizeof(unsigned short),
				 frequency);
	ret = i2s_prepare_tx_data(&g_i2stx_pri, data,
					   (data_size / 4));
	if (ret < 0) {
		free(data);
		printf("error");
	}
	

	return ret;
}

int sound_play(uint32_t msec, uint32_t frequency) {
	unsigned int *data;
	unsigned long data_size;
	unsigned int ret = 0;
	if (frequency == 0) { 
		udelay(msec * 1000); 
		return 0;
	}

	/*Buffer length computation */
	data_size = g_i2stx_pri.samplingrate * g_i2stx_pri.channels;
	data_size *= (g_i2stx_pri.bitspersample / SOUND_BITS_IN_BYTE);
	data = malloc(data_size);

	if (data == NULL) {
		debug("%s: malloc failed\n", __func__);
		return -1;
	}

	sound_create_square_wave((unsigned short *)data,
				 data_size / sizeof(unsigned short),
				 frequency);
	while (msec >= 1000) {
		ret = i2s_transfer_tx_data(&g_i2stx_pri, data,
					   (data_size / 4));
		msec -= 1000;
	}
	if (msec) {
		unsigned long size =
			(data_size * msec) / (sizeof(int) * 1000);

		ret = i2s_transfer_tx_data(&g_i2stx_pri, data, size);
	}

	free(data);

	return ret;
}
