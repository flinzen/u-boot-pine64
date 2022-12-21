#include <malloc.h>
#include <sound.h>
#include <common.h>
#include <asm/io.h>


int sound_init(const void *blob) {
	int ret;
	ret = 0;
#if 0   
	struct i2stx_info *pi2s_tx = &g_i2stx_pri;

/* Get the I2S Values */
	if (get_sound_i2s_values(pi2s_tx, blob) < 0) {
		debug(" FDT I2S values failed\n");
		return -1;
	}

	if (codec_init(blob, pi2s_tx) < 0) {
		debug(" Codec init failed\n");
		return -1;
	}

	ret = i2s_tx_init(pi2s_tx);
	if (ret) {
		debug("%s: Failed to init i2c transmit: ret=%d\n", __func__,
		      ret);
		return ret;
	}
#endif

	return ret;
}

int sound_play(uint32_t msec, uint32_t frequency) {
	return 0;
}
