/*
 * drivers/amlogic/mtd/m3_nand.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include "aml_mtd.h"

int nand_curr_device = -1;
struct hw_controller *controller;

struct mtd_info *nand_info[NAND_MAX_DEVICE];

#ifdef CONFIG_MTD_DEVICE
static __attribute__((unused)) char dev_name[CONFIG_SYS_MAX_NAND_DEVICE][8];
#endif

#ifndef SZ_1M
#define	SZ_1M	0x100000
#endif

static struct mtd_partition normal_partition_info[] = {
	{
		.name = "logo",
		.offset = 3*SZ_1M,
		.size = 8*SZ_1M,
	},
	/*
	 *{
	 *	.name = "recovery",
	 *	.offset = 11*SZ_1M,
	 *	.size = 10*SZ_1M,
	 *},
	 **/
	{
		.name = "boot",
		.offset = 11*SZ_1M,
		.size = 28*SZ_1M,
	},
	{
		.name = "upgrade",
		.offset = 39*SZ_1M,
		.size = 140*SZ_1M,
	},
	{
		.name = "data",
		.offset = MTDPART_OFS_APPEND,
		.size = MTDPART_SIZ_FULL,
	},

};

static struct aml_nand_platform aml_nand_mid_platform[] = {
	{
		.name = NAND_BOOT_NAME,
		.chip_enable_pad = AML_NAND_CE0,
		.ready_busy_pad = AML_NAND_CE0,
		.platform_nand_data = {
		    .chip =  {
			.nr_chips = 1,
			.options = (NAND_TIMING_MODE5 | NAND_ECC_BCH60_1K_MODE),
		    },
		},
		.rbpin_mode = 1,
		.short_pgsz = 384,
		.ran_mode = 0,
		.T_REA = 20,
		.T_RHOH = 15,
	},
	{
		.name = NAND_NORMAL_NAME,
		.chip_enable_pad = (AML_NAND_CE0) | (AML_NAND_CE1 << 4),
		.ready_busy_pad = (AML_NAND_CE0) | (AML_NAND_CE1 << 4),
		.platform_nand_data = {
		    .chip =  {
			.nr_chips = 1,
			.nr_partitions = ARRAY_SIZE(normal_partition_info),
			.partitions = normal_partition_info,
			.options = (NAND_TIMING_MODE5
				| NAND_ECC_BCH60_1K_MODE
				| NAND_TWO_PLANE_MODE),
		    },
		},
		.rbpin_mode = 1,
		.short_pgsz = 0,
		.ran_mode = 0,
		.T_REA = 20,
		.T_RHOH = 15,
	}
};

struct aml_nand_device aml_nand_mid_device = {
	.aml_nand_platform = aml_nand_mid_platform,
	.dev_num = 2,
};

#define ECC_INFORMATION(name_a, bch_a, size_a, parity_a, user_a) \
	{\
		.name = name_a,\
		.bch_mode = bch_a,\
		.bch_unit_size = size_a,\
		.bch_bytes = parity_a,\
		.user_byte_mode = user_a\
	}

static struct aml_nand_bch_desc m3_bch_list[] = {
	[0] = ECC_INFORMATION("NAND_RAW_MODE",
		NAND_ECC_SOFT_MODE,
		0,
		0,
		0),
	[1] = ECC_INFORMATION("NAND_BCH8_MODE",
		NAND_ECC_BCH8_MODE,
		NAND_ECC_UNIT_SIZE,
		NAND_BCH8_ECC_SIZE,
		2),
	[2] = ECC_INFORMATION("NAND_BCH8_1K_MODE",
		NAND_ECC_BCH8_1K_MODE,
		NAND_ECC_UNIT_1KSIZE,
		NAND_BCH8_1K_ECC_SIZE,
		2),
	[3] = ECC_INFORMATION("NAND_BCH24_1K_MODE",
		NAND_ECC_BCH24_1K_MODE,
		NAND_ECC_UNIT_1KSIZE,
		NAND_BCH24_1K_ECC_SIZE,
		2),
	[4] = ECC_INFORMATION("NAND_BCH30_1K_MODE",
		NAND_ECC_BCH30_1K_MODE,
		NAND_ECC_UNIT_1KSIZE,
		NAND_BCH30_1K_ECC_SIZE,
		2),
	[5] = ECC_INFORMATION("NAND_BCH40_1K_MODE",
		NAND_ECC_BCH40_1K_MODE,
		NAND_ECC_UNIT_1KSIZE,
		NAND_BCH40_1K_ECC_SIZE,
		2),
	[6] = ECC_INFORMATION("NAND_BCH50_1K_MODE",
		NAND_ECC_BCH50_1K_MODE,
		NAND_ECC_UNIT_1KSIZE,
		NAND_BCH50_1K_ECC_SIZE,
		2),
	[7] = ECC_INFORMATION("NAND_BCH60_1K_MODE",
		NAND_ECC_BCH60_1K_MODE,
		NAND_ECC_UNIT_1KSIZE,
		NAND_BCH60_1K_ECC_SIZE,
		2),
	[8] = ECC_INFORMATION("NAND_SHORT_MODE",
		NAND_ECC_SHORT_MODE,
		NAND_ECC_UNIT_SHORT,
		NAND_BCH60_1K_ECC_SIZE,
		2),
};

unsigned char pagelist_hynix256[128] = {
	0x00, 0x01, 0x02, 0x03, 0x06, 0x07, 0x0A, 0x0B,
	0x0E, 0x0F, 0x12, 0x13, 0x16, 0x17, 0x1A, 0x1B,
	0x1E, 0x1F, 0x22, 0x23, 0x26, 0x27, 0x2A, 0x2B,
	0x2E, 0x2F, 0x32, 0x33, 0x36, 0x37, 0x3A, 0x3B,

	0x3E, 0x3F, 0x42, 0x43, 0x46, 0x47, 0x4A, 0x4B,
	0x4E, 0x4F, 0x52, 0x53, 0x56, 0x57, 0x5A, 0x5B,
	0x5E, 0x5F, 0x62, 0x63, 0x66, 0x67, 0x6A, 0x6B,
	0x6E, 0x6F, 0x72, 0x73, 0x76, 0x77, 0x7A, 0x7B,

	0x7E, 0x7F, 0x82, 0x83, 0x86, 0x87, 0x8A, 0x8B,
	0x8E, 0x8F, 0x92, 0x93, 0x96, 0x97, 0x9A, 0x9B,
	0x9E, 0x9F, 0xA2, 0xA3, 0xA6, 0xA7, 0xAA, 0xAB,
	0xAE, 0xAF, 0xB2, 0xB3, 0xB6, 0xB7, 0xBA, 0xBB,

	0xBE, 0xBF, 0xC2, 0xC3, 0xC6, 0xC7, 0xCA, 0xCB,
	0xCE, 0xCF, 0xD2, 0xD3, 0xD6, 0xD7, 0xDA, 0xDB,
	0xDE, 0xDF, 0xE2, 0xE3, 0xE6, 0xE7, 0xEA, 0xEB,
	0xEE, 0xEF, 0xF2, 0xF3, 0xF6, 0xF7, 0xFA, 0xFB,
};
unsigned char pagelist_1ynm_hynix256_mtd[128] = {
	0x00, 0x01, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d,
	0x0f, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1b, 0x1d,
	0x1f, 0x21, 0x23, 0x25, 0x27, 0x29, 0x2b, 0x2d,
	0x2f, 0x31, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d,
	0x3f, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d,
	0x4f, 0x51, 0x53, 0x55, 0x57, 0x59, 0x5b, 0x5d,
	0x5f, 0x61, 0x63, 0x65, 0x67, 0x69, 0x6b, 0x6d,
	0x6f, 0x71, 0x73, 0x75, 0x77, 0x79, 0x7b, 0x7d,
	0x7f, 0x81, 0x83, 0x85, 0x87, 0x89, 0x8b, 0x8d,
	0x8f, 0x91, 0x93, 0x95, 0x97, 0x99, 0x9b, 0x9d,
	0x9f, 0xa1, 0xA3, 0xA5, 0xA7, 0xA9, 0xAb, 0xAd,
	0xAf, 0xb1, 0xB3, 0xB5, 0xB7, 0xB9, 0xBb, 0xBd,
	0xBf, 0xc1, 0xC3, 0xC5, 0xC7, 0xC9, 0xCb, 0xCd,
	0xCf, 0xd1, 0xD3, 0xD5, 0xD7, 0xD9, 0xDb, 0xDd,
	0xDf, 0xe1, 0xE3, 0xE5, 0xE7, 0xE9, 0xEb, 0xEd,
	0xEf, 0xf1, 0xF3, 0xF5, 0xF7, 0xF9, 0xFb, 0xFd,
};

#ifdef AML_NAND_UBOOT
void pinmux_select_chip_mtd(unsigned int ce_enable, unsigned int rb_enable)
{
	if (!((ce_enable >> 10) & 1))
		AMLNF_SET_REG_MASK(P_PERIPHS_PIN_MUX_4, (1 << 26));
	if (!((ce_enable >> 10) & 2))
		AMLNF_SET_REG_MASK(P_PERIPHS_PIN_MUX_4, (1 << 27));
	if (!((ce_enable >> 10) & 4))
		AMLNF_SET_REG_MASK(P_PERIPHS_PIN_MUX_4, (1 << 28));
	if (!((ce_enable >> 10) & 8))
		AMLNF_SET_REG_MASK(P_PERIPHS_PIN_MUX_4, (1 << 29));
}
#endif

static int controller_select_chip(struct hw_controller *controller,
	u8 chipnr)
{
#ifdef AML_NAND_UBOOT
	int i;
#endif
	int ret = 0;

	switch (chipnr) {
	case 0:
	case 1:
	case 2:
	case 3:
		controller->chip_selected = controller->ce_enable[chipnr];
		controller->rb_received = controller->rb_enable[chipnr];
#ifdef AML_NAND_UBOOT
		for (i = 0; i < controller->chip_num; i++)
			pinmux_select_chip_mtd(controller->ce_enable[i],
				controller->rb_enable[i]);
#endif

		NFC_SEND_CMD_IDLE(controller, 0);
		break;
	default:
		/*WARN_ON();*/
		/* controller->chip_selected = CE_NOT_SEL; */
		pr_info("%s %d: not support ce num!!!\n", __func__, __LINE__);
		ret = -12;
		break;
	}

	return ret;
}

static void m3_nand_select_chip(struct aml_nand_chip *aml_chip, int chipnr)
{
	controller_select_chip(controller, chipnr);
}

void get_sys_clk_rate_mtd(struct hw_controller *controller, int *rate)
{
	int clk_freq = *rate;
	unsigned int always_on = 0x1 << 24;
	unsigned int clk;
#ifdef AML_NAND_UBOOT
	cpu_id_t cpu_id = get_cpu_id();

	if (cpu_id.family_id == MESON_CPU_MAJOR_ID_AXG)
#else
	if (get_cpu_type() == MESON_CPU_MAJOR_ID_AXG)
		always_on = 0x1 << 28;
#endif
	if ((get_cpu_type() == MESON_CPU_MAJOR_ID_GXBB)
		|| (get_cpu_type() == MESON_CPU_MAJOR_ID_GXL)
		|| (get_cpu_type() == MESON_CPU_MAJOR_ID_AXG)) {
		switch (clk_freq) {
		case 24:
			clk = 0x80000201;
		break;
		case 112:
			clk = 0x80000249;
		break;
		case 200:
			clk = 0x80000245;
		break;
		case 250:
			clk = 0x80000244;
		break;
		default:
			clk = 0x80000245;
		break;
		}
		clk |= always_on;
		amlnf_write_reg32(controller->nand_clk_reg, clk);
		return;
	} else
		pr_info("%s %d: not support cpu type!!!\n",
			__func__, __LINE__);
}

static void m3_nand_hw_init(struct aml_nand_chip *aml_chip)
{
	int sys_clk_rate, bus_cycle, bus_timing;

	sys_clk_rate = 200;
	get_sys_clk_rate_mtd(controller, &sys_clk_rate);

	/* sys_time = (10000 / sys_clk_rate); */
	bus_cycle  = 6;
	bus_timing = bus_cycle + 1;

	NFC_SET_CFG(controller, 0);
	NFC_SET_TIMING_ASYC(controller, bus_timing, (bus_cycle - 1));
	NFC_SEND_CMD(controller, 1<<31);
}

static void m3_nand_adjust_timing(struct aml_nand_chip *aml_chip)
{
	int sys_clk_rate, bus_cycle, bus_timing;

	if (!aml_chip->T_REA)
		aml_chip->T_REA = 20;
	if (!aml_chip->T_RHOH)
		aml_chip->T_RHOH = 15;

	if (aml_chip->T_REA > 30)
		sys_clk_rate = 112;
	else if (aml_chip->T_REA > 16)
		sys_clk_rate = 200;
	else
		sys_clk_rate = 250;

	get_sys_clk_rate_mtd(controller, &sys_clk_rate);

	/* sys_time = (10000 / sys_clk_rate); */

	bus_cycle  = 6;
	bus_timing = bus_cycle + 1;

	NFC_SET_CFG(controller, 0);
	NFC_SET_TIMING_ASYC(controller, bus_timing, (bus_cycle - 1));

	NFC_SEND_CMD(controller, 1<<31);
}

static int m3_nand_options_confirm(struct aml_nand_chip *aml_chip)
{
	struct mtd_info *mtd = aml_chip->mtd;
	struct nand_chip *chip = &aml_chip->chip;
	struct aml_nand_platform *plat = aml_chip->platform;
	struct aml_nand_bch_desc *ecc_supports = aml_chip->bch_desc;
	unsigned int max_bch_mode = aml_chip->max_bch_mode;
	unsigned int options_selected = 0, options_support = 0, options_define;
	unsigned int eep_need_oobsize = 0, ecc_page_num = 0, ecc_bytes;
	int error = 0, i, valid_chip_num = 0;

	if (!strncmp((char *)plat->name,
		NAND_BOOT_NAME,
		strlen((const char *)NAND_BOOT_NAME))) {
		/*ecc_supports[8] is short mode ecc*/
		eep_need_oobsize =
		ecc_supports[8].bch_bytes + ecc_supports[8].user_byte_mode;
		ecc_page_num =
		aml_chip->page_size / ecc_supports[8].bch_unit_size;
		aml_chip->boot_oob_fill_cnt = aml_chip->oob_size -
			eep_need_oobsize * ecc_page_num;
	}

	/*select fit ecc mode by flash oob size */
	for (i = max_bch_mode - 1; i > 0; i--) {
		eep_need_oobsize =
		ecc_supports[i].bch_bytes + ecc_supports[i].user_byte_mode;
		ecc_page_num =
			aml_chip->page_size / ecc_supports[i].bch_unit_size;
		ecc_bytes = aml_chip->oob_size / ecc_page_num;
		if (ecc_bytes >= eep_need_oobsize) {
			options_support = ecc_supports[i].bch_mode;
			break;
		}
	}

	aml_chip->oob_fill_cnt =
		aml_chip->oob_size - eep_need_oobsize * ecc_page_num;
	pr_info("oob_fill_cnt =%d oob_size =%d, bch_bytes =%d\n",
		aml_chip->oob_fill_cnt,
		aml_chip->oob_size,
		ecc_supports[i].bch_bytes);
	pr_info("ecc mode:%d ecc_page_num=%d eep_need_oobsize=%d\n",
		options_support, ecc_page_num, eep_need_oobsize);

	if (options_support != NAND_ECC_SOFT_MODE) {
		chip->ecc.read_page_raw = aml_nand_read_page_raw;
		chip->ecc.write_page_raw = aml_nand_write_page_raw;
		chip->ecc.read_page = aml_nand_read_page_hwecc;
		chip->ecc.write_page = aml_nand_write_page_hwecc;
		chip->ecc.read_oob  = aml_nand_read_oob;
		chip->ecc.write_oob = aml_nand_write_oob;
		chip->block_bad = aml_nand_block_bad;
		chip->block_markbad = aml_nand_block_markbad;
		chip->ecc.mode = NAND_ECC_HW;
	} else {
		chip->ecc.read_page_raw = aml_nand_read_page_raw;
		chip->ecc.write_page_raw = aml_nand_write_page_raw;
		chip->ecc.mode = NAND_ECC_SOFT;
	}
	chip->write_buf = aml_nand_dma_write_buf;
	chip->read_buf = aml_nand_dma_read_buf;

	/* axg only suport bch8/512 & bch8/1k */
	if ((mtd->writesize <= 2048)
		|| (get_cpu_type() == MESON_CPU_MAJOR_ID_AXG))
		options_support = NAND_ECC_BCH8_MODE;

	switch (options_support) {
	case NAND_ECC_BCH8_MODE:
		chip->ecc.strength = 8;
		chip->ecc.size = NAND_ECC_UNIT_SIZE;
		chip->ecc.bytes = NAND_BCH8_ECC_SIZE;
		aml_chip->bch_mode = NAND_ECC_BCH8;
		aml_chip->user_byte_mode = 2;
		aml_chip->ecc_cnt_limit = 6;
		aml_chip->ecc_max = 8;
		chip->ecc.steps = mtd->writesize / chip->ecc.size;
		break;

	case NAND_ECC_BCH8_1K_MODE:
		chip->ecc.strength = 8;
		chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
		chip->ecc.bytes = NAND_BCH8_1K_ECC_SIZE;
		aml_chip->bch_mode = NAND_ECC_BCH8_1K;
		aml_chip->user_byte_mode = 2;
		aml_chip->ecc_cnt_limit = 6;
		aml_chip->ecc_max = 8;
		chip->ecc.steps = mtd->writesize / chip->ecc.size;
		break;
	/*don't support for new chip(greater than m8)*/
	case NAND_ECC_BCH16_1K_MODE:
		chip->ecc.strength = 16;
		chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
		chip->ecc.bytes = NAND_BCH16_1K_ECC_SIZE;
		aml_chip->bch_mode = NAND_ECC_BCH16_1K;
		aml_chip->user_byte_mode = 2;
		aml_chip->ecc_cnt_limit = 14;
		aml_chip->ecc_max = 16;
		chip->ecc.steps = mtd->writesize / chip->ecc.size;
		break;

	case NAND_ECC_BCH24_1K_MODE:
		chip->ecc.strength = 24;
		chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
		chip->ecc.bytes = NAND_BCH24_1K_ECC_SIZE;
		aml_chip->bch_mode = NAND_ECC_BCH24_1K;
		aml_chip->user_byte_mode = 2;
		aml_chip->ecc_cnt_limit = 22;
		aml_chip->ecc_max = 24;
		chip->ecc.steps = mtd->writesize / chip->ecc.size;
		break;

	case NAND_ECC_BCH30_1K_MODE:
		chip->ecc.strength = 30;
		chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
		chip->ecc.bytes = NAND_BCH30_1K_ECC_SIZE;
		aml_chip->bch_mode = NAND_ECC_BCH30_1K;
		aml_chip->user_byte_mode = 2;
		aml_chip->ecc_cnt_limit = 26;
		aml_chip->ecc_max = 30;
		chip->ecc.steps = mtd->writesize / chip->ecc.size;
		break;

	case NAND_ECC_BCH40_1K_MODE:
		chip->ecc.strength = 40;
		chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
		chip->ecc.bytes = NAND_BCH40_1K_ECC_SIZE;
		aml_chip->bch_mode = NAND_ECC_BCH40_1K;
		aml_chip->user_byte_mode = 2;
		aml_chip->ecc_cnt_limit = 34;
		aml_chip->ecc_max = 40;
		chip->ecc.steps = mtd->writesize / chip->ecc.size;
		break;

	case NAND_ECC_BCH50_1K_MODE:
		chip->ecc.strength = 40;
		chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
		chip->ecc.bytes = NAND_BCH50_1K_ECC_SIZE;
		aml_chip->bch_mode = NAND_ECC_BCH50_1K;
		aml_chip->user_byte_mode = 2;
		aml_chip->ecc_cnt_limit = 45;
		aml_chip->ecc_max = 50;
		chip->ecc.steps = mtd->writesize / chip->ecc.size;
		break;

	case NAND_ECC_BCH60_1K_MODE:
		chip->ecc.strength = 60;
		chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
		chip->ecc.bytes = NAND_BCH60_1K_ECC_SIZE;
		aml_chip->bch_mode = NAND_ECC_BCH60_1K;
		aml_chip->user_byte_mode = 2;
		aml_chip->ecc_cnt_limit = 55;
		aml_chip->ecc_max = 60;
		chip->ecc.steps = mtd->writesize / chip->ecc.size;
		break;

	case NAND_ECC_SHORT_MODE:
		chip->ecc.strength = 60;
		chip->ecc.size = NAND_ECC_UNIT_SHORT;
		chip->ecc.bytes = NAND_BCH60_1K_ECC_SIZE;
		aml_chip->bch_mode = NAND_ECC_BCH_SHORT;
		aml_chip->user_byte_mode = 2;
		aml_chip->ecc_cnt_limit = 55;
		aml_chip->ecc_max = 60;
		chip->ecc.steps = mtd->writesize / 512;
		break;
	/*not support for amlogic chip*/
	case NAND_ECC_SOFT_MODE:
		aml_chip->user_byte_mode = 1;
		aml_chip->bch_mode = 0;
		/*don't care*/
		aml_chip->ecc_cnt_limit = 9;
		aml_chip->ecc_max = 16;
		break;
	default:
		pr_info("unknown ecc mode, error!");
		error = -ENXIO;
		break;
	}

	options_selected =
	plat->platform_nand_data.chip.options & NAND_INTERLEAVING_OPTIONS_MASK;
	options_define = (aml_chip->options & NAND_INTERLEAVING_OPTIONS_MASK);
	if (options_selected > options_define) {
		pr_info("INTERLEAV change!\n");
		options_selected = options_define;
	}
	switch (options_selected) {
	case NAND_INTERLEAVING_MODE:
		aml_chip->ops_mode |= AML_INTERLEAVING_MODE;
		mtd->erasesize *= aml_chip->internal_chipnr;
		mtd->writesize *= aml_chip->internal_chipnr;
		mtd->oobsize *= aml_chip->internal_chipnr;
		break;
	default:
		break;
	}

	options_selected =
		plat->platform_nand_data.chip.options & NAND_PLANE_OPTIONS_MASK;
	options_define = (aml_chip->options & NAND_PLANE_OPTIONS_MASK);
	if (options_selected > options_define) {
		pr_info("PLANE change!\n");
		options_selected = options_define;
	}

	valid_chip_num = 0;
	for (i = 0; i < controller->chip_num; i++)
		if (aml_chip->valid_chip[i])
			valid_chip_num++;
	if (aml_chip->ops_mode & AML_INTERLEAVING_MODE)
		valid_chip_num *= aml_chip->internal_chipnr;

	if (valid_chip_num > 2) {
		aml_chip->plane_num = 1;
		pr_info("detect valid_chip_num over 2\n");
	} else {
		switch (options_selected) {
		case NAND_TWO_PLANE_MODE:
			aml_chip->plane_num = 2;
			mtd->erasesize *= 2;
			mtd->writesize *= 2;
			mtd->oobsize *= 2;
			pr_info("two plane!@\n");
			break;
		default:
			aml_chip->plane_num = 1;
			break;
		}
	}

	pr_info("plane_num=%d writesize=0x%x ecc.size=0x%0x bch_mode=%d\n",
		aml_chip->plane_num,
		mtd->writesize,
		chip->ecc.size,
		aml_chip->bch_mode);

	return error;
}

static int aml_platform_dma_waiting(struct aml_nand_chip *aml_chip)
{
	unsigned int time_out_cnt = 0;

	NFC_SEND_CMD_IDLE(controller, 0);
	NFC_SEND_CMD_IDLE(controller, 0);
	do {
		if (NFC_CMDFIFO_SIZE(controller) <= 0)
			break;
	} while (time_out_cnt++ <= AML_DMA_BUSY_TIMEOUT);

	if (time_out_cnt < AML_DMA_BUSY_TIMEOUT)
		return 0;

	return -EBUSY;
}

static int m3_nand_dma_write(struct aml_nand_chip *aml_chip,
	unsigned char *buf, int len, unsigned int bch_mode)
{
	int ret = 0;
	unsigned int dma_unit_size = 0, count = 0;
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = aml_chip->mtd;
	uint32_t temp;

#ifndef AML_NAND_UBOOT
	memcpy(aml_chip->aml_nand_data_buf, buf, len);
	/*smp_wmb*/
	smp_wmb();
	/*wmb*/
	wmb();
#endif
	if (bch_mode == NAND_ECC_NONE)
		count = 1;
	else if (bch_mode == NAND_ECC_BCH_SHORT) {
		dma_unit_size = (chip->ecc.size >> 3);
		count = len/chip->ecc.size;
	} else
		count = len/chip->ecc.size;

#ifndef AML_NAND_UBOOT
	NFC_SEND_CMD_ADL(controller, aml_chip->data_dma_addr);
	NFC_SEND_CMD_ADH(controller, aml_chip->data_dma_addr);
	NFC_SEND_CMD_AIL(controller, aml_chip->info_dma_addr);
	NFC_SEND_CMD_AIH(controller, aml_chip->info_dma_addr);
#else
	flush_dcache_range((unsigned long)buf, (unsigned long)buf + len);
	flush_dcache_range((unsigned long)aml_chip->user_info_buf,
		(unsigned long)aml_chip->user_info_buf + count * PER_INFO_BYTE);

	NFC_SEND_CMD_ADL(controller, (u32)(unsigned long)buf);
	NFC_SEND_CMD_ADH(controller, (u32)(unsigned long)buf);
	NFC_SEND_CMD_AIL(controller,
		(u32)(unsigned long)aml_chip->user_info_buf);
	NFC_SEND_CMD_AIH(controller,
		(u32)(unsigned long)aml_chip->user_info_buf);
#endif

	if (aml_chip->ran_mode) {
		temp = mtd->writesize>>chip->page_shift;
		if (aml_chip->plane_num == 2)
			NFC_SEND_CMD_SEED(controller,
				(aml_chip->page_addr / temp) * temp);
		else
			NFC_SEND_CMD_SEED(controller, aml_chip->page_addr);
	}
	if (!bch_mode)
		NFC_SEND_CMD_M2N_RAW(controller, 0, len);
	else
		NFC_SEND_CMD_M2N(controller, aml_chip->ran_mode,
	((bch_mode == NAND_ECC_BCH_SHORT) ? aml_chip->bch_info : bch_mode),
	((bch_mode == NAND_ECC_BCH_SHORT) ? 1 : 0), dma_unit_size, count);

	ret = aml_platform_dma_waiting(aml_chip);

	if (aml_chip->oob_fill_cnt > 0) {
		NFC_SEND_CMD_M2N_RAW(controller,
			aml_chip->ran_mode, aml_chip->oob_fill_cnt);
		ret = aml_platform_dma_waiting(aml_chip);
	}
	return ret;
}

static int m3_nand_dma_read(struct aml_nand_chip *aml_chip,
	unsigned char *buf, int len, unsigned int bch_mode)
{
	unsigned int *info_buf = 0;
#ifdef AML_NAND_UBOOT
	int cmp = 0;
#endif
	struct nand_chip *chip = &aml_chip->chip;
	unsigned int dma_unit_size = 0, count = 0, info_times_int_len;
	int ret = 0;
	struct mtd_info *mtd = aml_chip->mtd;
	uint32_t temp;

	info_times_int_len = PER_INFO_BYTE/sizeof(unsigned int);
	if (bch_mode == NAND_ECC_NONE)
		count = 1;
	else if (bch_mode == NAND_ECC_BCH_SHORT) {
		dma_unit_size = (chip->ecc.size >> 3);
		count = len/chip->ecc.size;
	} else
		count = len/chip->ecc.size;

	memset((unsigned char *)aml_chip->user_info_buf,
		0, count*PER_INFO_BYTE);

#ifndef AML_NAND_UBOOT
	WARN_ON(count == 0);
	info_buf =
(u32 *)&(aml_chip->user_info_buf[(count-1)*info_times_int_len]);
	/*smp_wmb*/
	smp_wmb();
	/*wmb*/
	wmb();
	NFC_SEND_CMD_ADL(controller, aml_chip->data_dma_addr);
	NFC_SEND_CMD_ADH(controller, aml_chip->data_dma_addr);
	NFC_SEND_CMD_AIL(controller, aml_chip->info_dma_addr);
	NFC_SEND_CMD_AIH(controller, aml_chip->info_dma_addr);
#else
	flush_dcache_range((unsigned long)aml_chip->user_info_buf,
		(unsigned long)aml_chip->user_info_buf + count*PER_INFO_BYTE);
	invalidate_dcache_range((unsigned long)buf, (unsigned long)buf + len);

	NFC_SEND_CMD_ADL(controller, (u32)(unsigned long)buf);
	NFC_SEND_CMD_ADH(controller, (u32)(unsigned long)buf);
	NFC_SEND_CMD_AIL(controller,
		(u32)(unsigned long)aml_chip->user_info_buf);
	NFC_SEND_CMD_AIH(controller,
		(u32)(unsigned long)aml_chip->user_info_buf);
#endif
	if (aml_chip->ran_mode) {
		temp = mtd->writesize >> chip->page_shift;
		if (aml_chip->plane_num == 2)
			NFC_SEND_CMD_SEED(controller,
				(aml_chip->page_addr / temp) * temp);
		else
				NFC_SEND_CMD_SEED(controller,
					aml_chip->page_addr);
	}

	if (bch_mode == NAND_ECC_NONE)
		NFC_SEND_CMD_N2M_RAW(controller, 0, len);
	else
		NFC_SEND_CMD_N2M(controller, aml_chip->ran_mode,
		((bch_mode == NAND_ECC_BCH_SHORT)?aml_chip->bch_info:bch_mode),
		((bch_mode == NAND_ECC_BCH_SHORT)?1:0), dma_unit_size, count);

	ret = aml_platform_dma_waiting(aml_chip);
	if (ret)
		return ret;
#ifndef AML_NAND_UBOOT
	do {
		/*smp_rmb()*/
		smp_rmb();
	} while (NAND_INFO_DONE(*info_buf) == 0);
#else
	do {
		invalidate_dcache_range((unsigned long)aml_chip->user_info_buf,
		(unsigned long)aml_chip->user_info_buf + count*PER_INFO_BYTE);
		info_buf =
(u32 *)&(aml_chip->user_info_buf[(count-1)*info_times_int_len]);
		cmp = *info_buf;
	} while ((cmp) == 0);
#endif

	/*smp_rmb()*/
	smp_rmb();
	if (buf != aml_chip->aml_nand_data_buf)
		memcpy(buf, aml_chip->aml_nand_data_buf, len);
	/*smp_wmb()*/
	smp_wmb();
	/*wmb()*/
	wmb();
	return 0;
}

static int m3_nand_hwecc_correct(struct aml_nand_chip *aml_chip,
	unsigned char *buf, unsigned int size, unsigned char *oob_buf)
{
	struct nand_chip *chip = &aml_chip->chip;
	unsigned int ecc_step_num, usr_info, tmp_value;
	unsigned int info_times_int_len = PER_INFO_BYTE / sizeof(unsigned int);

	if (size % chip->ecc.size) {
		pr_info("error parameter size for ecc correct %x\n", size);
		return -EINVAL;
	}

	aml_chip->ecc_cnt_cur = 0;
	for (ecc_step_num = 0;
		ecc_step_num < (size / chip->ecc.size); ecc_step_num++) {
		/* check if there have uncorrectable sector */
		tmp_value = ecc_step_num * info_times_int_len;
		usr_info = *(u32 *)(&aml_chip->user_info_buf[tmp_value]);
		if (NAND_ECC_CNT(usr_info) == 0x3f) {
			aml_chip->zero_cnt = NAND_ZERO_CNT(usr_info);
			return -EIO;

		} else
			aml_chip->ecc_cnt_cur =
				NAND_ECC_CNT(usr_info);
	}

	return 0;
}


#ifndef AML_NAND_UBOOT
struct nand_hw_control upper_controller;

static int m3_nand_suspend(struct mtd_info *mtd)
{
	pr_info("m3 nand suspend entered\n");
	return 0;
}

static void m3_nand_resume(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct aml_nand_platform *plat = aml_chip->platform;
	struct nand_chip *chip = &aml_chip->chip;
	u8 onfi_features[4];

	if (!strncmp((char *)plat->name,
		NAND_BOOT_NAME, strlen((const char *)NAND_BOOT_NAME)))
		return;

	chip->select_chip(mtd, 0);
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	if (aml_chip->onfi_mode) {
		aml_nand_set_onfi_features(aml_chip,
			(uint8_t *)(&aml_chip->onfi_mode), ONFI_TIMING_ADDR);
		aml_nand_get_onfi_features(aml_chip,
			onfi_features, ONFI_TIMING_ADDR);
		if (onfi_features[0] != aml_chip->onfi_mode) {
			aml_chip->T_REA = DEFAULT_T_REA;
			aml_chip->T_RHOH = DEFAULT_T_RHOH;
			pr_info("onfi timing mode set failed: %x\n",
				onfi_features[0]);
		}
	}

	/*
	 *if (chip->state == FL_PM_SUSPENDED)
	 *	nand_release_device(mtd);

	 *chip->select_chip(mtd, -1);
	 *spin_lock(&chip->controller->lock);
	 *chip->controller->active = NULL;
	 *chip->state = FL_READY;
	 *wake_up(&chip->controller->wq);
	 *spin_unlock(&chip->controller->lock);
	 **/
	pr_info("m3 nand resume entered\n");
}
#endif

#ifndef AML_NAND_UBOOT
static int m3_nand_probe(struct aml_nand_platform *plat,
	unsigned int dev_num, struct device *dev)
#else
static int m3_nand_probe(struct aml_nand_platform *plat, unsigned int dev_num)
#endif
{
	struct aml_nand_chip *aml_chip = NULL;
	struct nand_chip *chip = NULL;
	struct mtd_info *mtd = NULL;
	int err = 0;
	unsigned int nand_type = 0;
	struct aml_nand_device *aml_nand_device = NULL;
/*#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 13)*/
#if 0
	int tmp_val, i;
#endif

	if (!plat) {
		pr_info("no platform specific information\n");
		goto exit_error;
	}

	aml_chip = kzalloc(sizeof(*aml_chip), GFP_KERNEL);
	if (aml_chip == NULL) {
		err = -ENOMEM;
		goto exit_error;
	}

	/* initialize mtd info data struct */
	aml_chip->controller = controller;
	/* to it's own plat data */
	aml_chip->platform = plat;
	aml_chip->bch_desc = m3_bch_list;
	aml_chip->max_bch_mode = ARRAY_SIZE(m3_bch_list);
	chip = &aml_chip->chip;
	mtd = nand_to_mtd(chip);
	aml_chip->mtd = mtd;
	plat->aml_chip = aml_chip;
	chip->priv = aml_chip;
	mtd->priv = chip;
	mtd->name = plat->name;
#ifndef AML_NAND_UBOOT
	/*fixit ,hardware support all address*/
	/* dev->coherent_dma_mask = DMA_BIT_MASK(32); */
	aml_nand_device = plat->aml_nand_device;
	pr_info("%s() aml_nand_device %p\n", __func__, aml_nand_device);
	if (aml_nand_device) {
		aml_chip->bl_mode = aml_nand_device->bl_mode;
		aml_chip->fip_copies = aml_nand_device->fip_copies;
		aml_chip->fip_size = aml_nand_device->fip_size;
	}


	aml_chip->device = dev;
	mtd->dev.parent = dev->parent;
	mtd->owner = THIS_MODULE;
	chip->controller =  &upper_controller;
#endif

	/*register amlogic hw controller functions*/
	aml_chip->aml_nand_hw_init = m3_nand_hw_init;
	aml_chip->aml_nand_adjust_timing = m3_nand_adjust_timing;
	aml_chip->aml_nand_select_chip = m3_nand_select_chip;
	aml_chip->aml_nand_options_confirm = m3_nand_options_confirm;
	aml_chip->aml_nand_dma_read = m3_nand_dma_read;
	aml_chip->aml_nand_dma_write = m3_nand_dma_write;
	aml_chip->aml_nand_hwecc_correct = m3_nand_hwecc_correct;
	aml_chip->aml_nand_cmd_ctrl = aml_platform_cmd_ctrl;
	aml_chip->aml_nand_write_byte = aml_platform_write_byte;
	aml_chip->aml_nand_wait_devready = aml_platform_wait_devready;
	aml_chip->aml_nand_get_user_byte = aml_platform_get_user_byte;
	aml_chip->aml_nand_set_user_byte = aml_platform_set_user_byte;
	aml_chip->aml_nand_command = aml_nand_base_command;
	aml_chip->aml_nand_block_bad_scrub =
		aml_nand_block_bad_scrub_update_bbt;

	aml_chip->ran_mode = plat->ran_mode;
	aml_chip->rbpin_detect = plat->rbpin_detect;

	err = aml_nand_init(aml_chip);
	if (err)
		goto exit_error;
	if (aml_chip->support_new_nand == 1) {
		nand_type =
	((aml_chip->new_nand_info.type < 10) && (aml_chip->new_nand_info.type));
		if (nand_type && nand_boot_flag) {
			pr_info("detect CHIP revB with Hynix new nand error\n");
			aml_chip->err_sts = NAND_CHIP_REVB_HY_ERR;
		}
	}
	if (!strncmp((char *)plat->name,
		NAND_BOOT_NAME, strlen((const char *)NAND_BOOT_NAME))) {
		/* interface is chip->erase_cmd on 3.14*/
		chip->erase = m3_nand_boot_erase_cmd;
		chip->ecc.read_oob  = m3_nand_boot_read_oob;
		chip->ecc.read_page = m3_nand_boot_read_page_hwecc;
		chip->ecc.write_page = m3_nand_boot_write_page_hwecc;
		chip->write_page = m3_nand_boot_write_page;
/*#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 13)*/
#if 0
		if (chip->ecc.layout)
			chip->ecc.layout->oobfree[0].length =
			((mtd->writesize / 512) * aml_chip->user_byte_mode);
		chip->ecc.layout->oobavail = 0;
		tmp_val =  ARRAY_SIZE(chip->ecc.layout->oobfree);
	for (i = 0; chip->ecc.layout->oobfree[i].length && i < tmp_val; i++)
		chip->ecc.layout->oobavail +=
			chip->ecc.layout->oobfree[i].length;
		mtd->oobavail = chip->ecc.layout->oobavail;
		mtd->ecclayout = chip->ecc.layout;

#endif
	} else {
		aml_ubootenv_init(aml_chip);
		aml_key_init(aml_chip);
		amlnf_dtb_init(aml_chip);
	}

#ifndef AML_NAND_UBOOT
	mtd->_suspend = m3_nand_suspend;
	mtd->_resume = m3_nand_resume;
#endif
#ifdef AML_NAND_UBOOT
	/*need to set device_boot_flag here*/
	device_boot_flag = NAND_BOOT_FLAG;
#endif
	nand_info[dev_num] = mtd;
	return 0;

exit_error:
	kfree(aml_chip);
	mtd->name = NULL;
	return err;
}

#ifdef AML_NAND_UBOOT
void nand_init(void)
{
	struct aml_nand_platform *plat = NULL;
	int i, ret;

	controller = kzalloc(sizeof(struct hw_controller), GFP_KERNEL);
	if (controller == NULL)
		return;

	controller->chip_num = 1; /* assume chip num is 1 */
	for (i = 0; i < MAX_CHIP_NUM; i++) {
		controller->ce_enable[i] =
			(((CE_PAD_DEFAULT >> i*4) & 0xf) << 10);
		controller->rb_enable[i] =
			(((RB_PAD_DEFAULT >> i*4) & 0xf) << 10);
	}

	/* set nf controller register base and nand clk base */
	controller->reg_base = (void *)(uint32_t *)NAND_BASE_APB;
	controller->nand_clk_reg = (void *)(uint32_t *)NAND_CLK_CNTL;

	/*
	 *pr_info("nand register base %p, nand clock register %p\n",
	 *	controller->reg_base,
	 *	controller->nand_clk_reg);
	 **/

	for (i = 0; i < aml_nand_mid_device.dev_num; i++) {
		plat = &aml_nand_mid_device.aml_nand_platform[i];
		if (!plat) {
			pr_info("error for not platform data\n");
			continue;
		}
		ret = m3_nand_probe(plat, i);
		if (ret) {
			pr_info("nand init faile: %d\n", ret);
			continue;
		}
	}
	nand_curr_device = 1; /* fixit */
}
#else
int nand_init(struct platform_device *pdev)
{
	struct aml_nand_platform *plat = NULL;
	struct resource *res_mem, *res_irq;
	int i, ret, size;

	controller = kzalloc(sizeof(struct hw_controller), GFP_KERNEL);
	if (controller == NULL)
		return -ENOMEM;

	controller->device = &pdev->dev;
	controller->chip_num = 1; /* assume chip num is 1 */
	for (i = 0; i < MAX_CHIP_NUM; i++) {
		controller->ce_enable[i] = (((CE_PAD_DEFAULT>>i*4)&0xf) << 10);
		controller->rb_enable[i] = (((RB_PAD_DEFAULT>>i*4)&0xf) << 10);
	}

	/*getting nand platform device IORESOURCE_MEM*/
	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_mem) {
		dev_err(&pdev->dev, "cannot obtain I/O memory region\n");
		return -ENODEV;
	}
	size = resource_size(res_mem);
	/*mapping Nand Flash Controller register address*/
	/*request mem erea*/
	if (!devm_request_mem_region(&pdev->dev,
		res_mem->start,
		size,
		dev_name(&pdev->dev))) {
		dev_err(&pdev->dev, "Memory region busy\n");
		return -EBUSY;
	}


	/* set nf controller register base and nand clk base */
	controller->reg_base = devm_ioremap_nocache(&pdev->dev,
					res_mem->start, size);
	if (controller->reg_base == NULL) {
		dev_err(&pdev->dev, "ioremap Nand Flash IO fail\n");
		return -ENOMEM;
	}
	pr_info("nand_clk_ctrl 0x%x\n",
		aml_nand_mid_device.nand_clk_ctrl);

	controller->nand_clk_reg = devm_ioremap_nocache(&pdev->dev,
					aml_nand_mid_device.nand_clk_ctrl,
					sizeof(int));
	if (controller->nand_clk_reg == NULL) {
		dev_err(&pdev->dev, "ioremap External Nand Clock IO fail\n");
		return -ENOMEM;
	}
	pr_info("nand register base %p, nand clock register %p\n",
		controller->reg_base,
		controller->nand_clk_reg);

	res_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res_irq) {
		dev_err(&pdev->dev, "irq res get fail\n");
		return -ENODEV;
	}
	controller->irq = res_irq->start;

	/*pinmux select*/
	controller->nand_pinctrl = devm_pinctrl_get(controller->device);
	if (IS_ERR(controller->nand_pinctrl)) {
		pr_info("get pinctrl error");
		return PTR_ERR(controller->nand_pinctrl);
	}
	controller->nand_rbstate =
		pinctrl_lookup_state(controller->nand_pinctrl,
			"nand_rb_mod");
	if (IS_ERR(controller->nand_rbstate)) {
		devm_pinctrl_put(controller->nand_pinctrl);
		return PTR_ERR(controller->nand_rbstate);
	}
	controller->nand_norbstate =
		pinctrl_lookup_state(controller->nand_pinctrl,
			"nand_norb_mod");
	if (IS_ERR(controller->nand_norbstate)) {
		devm_pinctrl_put(controller->nand_pinctrl);
		return PTR_ERR(controller->nand_norbstate);
	}
	ret = pinctrl_select_state(controller->nand_pinctrl,
			controller->nand_norbstate);
	if (ret < 0)
		pr_info("%s:%d %s can't get pinctrl",
			__func__,
			__LINE__,
			dev_name(controller->device));
	controller->nand_idlestate =
		pinctrl_lookup_state(controller->nand_pinctrl,
			"nand_cs_only");
	if (IS_ERR(controller->nand_idlestate)) {
		devm_pinctrl_put(controller->nand_pinctrl);
		return PTR_ERR(controller->nand_idlestate);
	}

	for (i = 0; i < aml_nand_mid_device.dev_num; i++) {
		plat = &aml_nand_mid_device.aml_nand_platform[i];
		if (!plat) {
			pr_info("error for not platform data\n");
			continue;
		}
#ifndef AML_NAND_UBOOT
		/* to get the glb fip infos */
		plat->aml_nand_device = &aml_nand_mid_device;
		pr_info("plat->aml_nand_device %p\n", plat->aml_nand_device);
		ret = m3_nand_probe(plat, i, &pdev->dev);
#else
		ret = m3_nand_probe(plat, i);
#endif
		if (ret) {
			pr_info("nand init failed:%d\n", ret);
			continue;
		}
	}
	nand_curr_device = 1; /* fixit */
	return ret;
}
#endif


