/*
 * Copyright (C) 2012 Samsung Electronics
 * Rajeshwari Shinde <rajeshwari.s@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <fdtdec.h>
#include <sound.h>

DECLARE_GLOBAL_DATA_PTR;

/* Initilaise sound subsystem */
static int do_init(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;

	ret = sound_init(gd->fdt_blob);
	if (ret) {
		printf("Initialise Audio driver failed\n");
		return CMD_RET_FAILURE;
	}

	return 0;
}

/* play sound from buffer */
static int do_play(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = 0;
	int msec = 1000;
	int freq = 400;

	if (argc > 1)
		msec = simple_strtoul(argv[1], NULL, 10);
	if (argc > 2)
		freq = simple_strtoul(argv[2], NULL, 10);

	ret = sound_play(msec, freq);
	if (ret) {
		printf("play failed");
		return CMD_RET_FAILURE;
	}

	return 0;
}

static int do_melody(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]) {
	const u16 melodyEnd = 4096;
	const u16 melodyIntro[] = { 440,90,0,90,440,90,0,90,220,90,220,90,220,90,0,900,440,90,0,90,440,90,0,90,220,90,220,90,220,90,0,900,440,90,0,90,440,90,0,90,220,90,220,90,220,90,0,900,293,90,0,90,293,90,0,90,146,90,146,90,146,90,0,900,melodyEnd };
	u32 i = 0;
	int ret = 0;
	while (melodyIntro[i] != melodyEnd) {
		ret = sound_play(melodyIntro[i+1], melodyIntro[i]);
		if (ret) {
			return CMD_RET_FAILURE;
		}
		i+=2;
	}
	return ret;
}

static int do_play_dma(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]) {
	int ret = 0;
	int msec = 1000;
	int freq = 400;

	if (argc > 1)
		msec = simple_strtoul(argv[1], NULL, 10);
	if (argc > 2)
		freq = simple_strtoul(argv[2], NULL, 10);

	ret = sound_play_dma(msec, freq);
	if (ret) {
		printf("play failed");
		return CMD_RET_FAILURE;
	}
	return 0;
	
}

static int do_play_file(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]) {
	int ret = 0;
	int msec = 1000;
	int freq = 400;

	if (argc > 1)
		msec = simple_strtoul(argv[1], NULL, 10);
	if (argc > 2)
		freq = simple_strtoul(argv[2], NULL, 10);

	ret = sound_play_file(msec, freq);
	if (ret) {
		printf("play failed");
		return CMD_RET_FAILURE;
	}
	return 0;
	
}

static cmd_tbl_t cmd_sound_sub[] = {
	U_BOOT_CMD_MKENT(init, 0, 1, do_init, "", ""),
	U_BOOT_CMD_MKENT(play, 2, 1, do_play, "", ""),
	U_BOOT_CMD_MKENT(dma, 2, 1, do_play_dma, "", ""),
	U_BOOT_CMD_MKENT(melody, 0, 1, do_melody, "", ""),
	U_BOOT_CMD_MKENT(file, 2, 1, do_play_file, "", ""),
};

/* process sound command */
static int do_sound(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *c;

	if (argc < 1)
		return CMD_RET_USAGE;

	/* Strip off leading 'sound' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_sound_sub[0], ARRAY_SIZE(cmd_sound_sub));

	if (c)
		return c->cmd(cmdtp, flag, argc, argv);
	else
		return CMD_RET_USAGE;
}

U_BOOT_CMD(
	sound, 4, 1, do_sound,
	"sound sub-system",
	"init - initialise the sound driver\n"
	"sound play [len] [freq] - play a sound for len ms at freq hz\n"
);
