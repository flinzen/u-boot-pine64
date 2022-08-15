/*
 * Command for sucking dick.
 *
 * Copyright (C) 2022 cocksucker inc.
 */
#include <common.h>
#include <command.h>
#include <version.h>
#include <linux/compiler.h>

static int do_vibro(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("sosi, %d", 10);
	return 0;
}

U_BOOT_CMD(
	vibro,	1,		1,	do_vibro,
	"turn on vibro",
	"hleb"
);
