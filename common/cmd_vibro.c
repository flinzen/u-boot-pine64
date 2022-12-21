/*
 * Command for sucking dick.
 *
 * Copyright (C) 2022 cocksucker inc.
 */
#include <common.h>
#include <command.h>
#include <version.h>
#include <linux/compiler.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#include <smc.h>

int gpio_output(u32 port, u32 port_num, u32 val) {
	u32 reg_val;
	volatile __u32     *tmp_addr;
	
	tmp_addr = PIO_REG_DATA(port);
	reg_val = GPIO_REG_READ(tmp_addr);                                                 
	reg_val &= ~(0x01 << port_num);
	reg_val |=  (val & 0x01) << port_num;
	GPIO_REG_WRITE(tmp_addr, reg_val);
	
	return 0;
}

int gpio_cfg(u32 port, u32 port_num, u32 val) {
	u32 reg_val;
	volatile __u32      *tmp_group_func_addr = NULL;
	u32 port_num_func = port_num >> 3;
	tmp_group_func_addr = PIO_REG_CFG(port, port_num_func);
	reg_val = GPIO_REG_READ(tmp_group_func_addr);
	reg_val &= ~(0x07 << (((port_num - (port_num_func<<3))<<2)));
	reg_val |=  1 << (((port_num - (port_num_func<<3))<<2));
	
	GPIO_REG_WRITE(tmp_group_func_addr, reg_val);
	return 0;

}

static int do_vibro(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	gpio_cfg(4, 1, 1);
	gpio_output(4, 1, 1);
	udelay(500*1000);
	gpio_output(4, 1, 0);
	udelay(50*1000);
	gpio_output(4, 1, 1);
	udelay(100*1000);
	gpio_output(4, 1, 0);
	udelay(50*1000);
	gpio_output(4, 1, 1);
	udelay(100*1000);
	gpio_output(4, 1, 0);
	udelay(50*1000);
	gpio_output(4, 1, 1);
	udelay(2*100*1000);
	gpio_output(4, 1, 0);
	udelay(50*1000);
	gpio_output(4, 1, 1);
	udelay(2*100*1000);
	gpio_output(4, 1, 0);
	udelay(50*1000);
	gpio_output(4, 1, 1);
	udelay(3*100*1000);
	gpio_output(4, 1, 0);
	udelay(50*1000);
	gpio_output(4, 1, 1);
	udelay(2*100*1000);
	gpio_output(4, 1, 0);
	udelay(50*1000);
	return 0;
}

U_BOOT_CMD(
	vibro,	1,		1,	do_vibro,
	"turn on vibro",
	"hleb"
);
