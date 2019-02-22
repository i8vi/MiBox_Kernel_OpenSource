/*
 * sound/soc/amlogic/auge/audio_utils.c
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

#include "audio_utils.h"
#include "regs.h"
#include "iomap.h"
#include "loopback_hw.h"
#include "spdif_hw.h"

struct snd_elem_info {
	struct soc_enum *ee;
	int reg;
	int shift;
	u32 mask;
};

static unsigned int loopback_enable;

static const char *const loopback_enable_texts[] = {
	"Disable",
	"Enable",
};

static const struct soc_enum loopback_enable_enum =
	SOC_ENUM_SINGLE(EE_AUDIO_LB_CTRL0,
			31,
			ARRAY_SIZE(loopback_enable_texts),
			loopback_enable_texts);


static int loopback_enable_get_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = loopback_enable;

	return 0;
}

static int loopback_enable_set_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	loopback_enable = ucontrol->value.enumerated.item[0];

	return 0;
}


static unsigned int loopback_datain;

static const char *const loopback_datain_texts[] = {
	"TDMIN_A",
	"TDMIN_B",
	"TDMIN_C",
	"SPDIFIN",
	"PDMIN",
};

static const struct soc_enum loopback_datain_enum =
	SOC_ENUM_SINGLE(EE_AUDIO_LB_CTRL0, 0, ARRAY_SIZE(loopback_datain_texts),
			loopback_datain_texts);


static int loopback_datain_get_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = loopback_datain;

	return 0;
}

static int loopback_datain_set_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	loopback_datain = ucontrol->value.enumerated.item[0];
	audiobus_update_bits(EE_AUDIO_LB_CTRL0, 0, loopback_datain);

	return 0;
}

static unsigned int loopback_tdminlb;

static const char *const loopback_tdminlb_texts[] = {
	"TDMOUT_A",
	"TDMOUT_B",
	"TDMOUT_C",
	"TDMIN_A",
	"TDMIN_B",
	"TDMIN_C",
};

static const struct soc_enum loopback_tdminlb_enum =
	SOC_ENUM_SINGLE(EE_AUDIO_TDMIN_LB_CTRL,
			20,
			ARRAY_SIZE(loopback_tdminlb_texts),
			loopback_tdminlb_texts);


static int loopback_tdminlb_get_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = loopback_tdminlb;

	return 0;
}

static int loopback_tdminlb_set_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	loopback_tdminlb = ucontrol->value.enumerated.item[0];
	audiobus_update_bits(EE_AUDIO_TDMIN_LB_CTRL,
		0xf << 20,
		loopback_datain);

	return 0;
}

static int snd_int_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0xffffffff;
	uinfo->count = 1;

	return 0;
}

static int snd_int_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int val;
	unsigned int reg = kcontrol->private_value;

	val = audiobus_read(reg);
	ucontrol->value.integer.value[0] = val;

	pr_info("%s:reg:0x%x, val:0x%x\n",
		__func__,
		reg,
		val);

	return 0;
}

static int snd_int_set(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int val  = (int)ucontrol->value.integer.value[0];
	unsigned int reg = kcontrol->private_value;

	pr_info("%s:reg:0x%x, val:0x%x\n",
		__func__,
		reg,
		val);

	audiobus_write(reg, val);

	return 0;
}

#define SND_INT(xname, type, func)     \
{                                      \
	.iface = SNDRV_CTL_ELEM_IFACE_PCM, \
	.name  = xname,               \
	.info  = snd_int_info,        \
	.get   = snd_int_get,         \
	.put   = snd_int_set,         \
	.private_value = EE_AUDIO_##type##_##func, \
}

static int snd_byte_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0xff;
	uinfo->count = 1;

	return 0;
}

static int snd_byte_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int val;
	struct snd_elem_info *einfo = (void *)kcontrol->private_value;

	val = audiobus_read(einfo->reg);
	val >>= einfo->shift;
	val &=  einfo->mask;

	ucontrol->value.integer.value[0] = val;

	pr_info("%s:reg:0x%x, mask:0x%x, mask val:0x%x\n",
		__func__,
		einfo->reg,
		einfo->mask,
		val);

	return 0;
}

static int snd_byte_set(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int val  = (int)ucontrol->value.integer.value[0];
	struct snd_elem_info *einfo = (void *)kcontrol->private_value;

	if (val < 0)
		val = 0;
	if (val > 255)
		val = 255;

	pr_info("%s:reg:0x%x, mask:0x%x, mask val:0x%x\n",
		__func__,
		einfo->reg,
		einfo->mask,
		val);

	audiobus_update_bits(
		einfo->reg,
		einfo->mask << einfo->shift,
		val << einfo->shift);

	return 0;
}

#define SND_BYTE(xname, type, func, xshift, xmask)   \
{                                      \
	.iface = SNDRV_CTL_ELEM_IFACE_PCM, \
	.name  = xname,                \
	.info  = snd_byte_info,        \
	.get   = snd_byte_get,         \
	.put   = snd_byte_set,         \
	.private_value =               \
	((unsigned long)&(struct snd_elem_info) \
		{.reg = EE_AUDIO_##type##_##func,   \
		 .shift = xshift, .mask = xmask })  \
}

static int snd_enum_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	struct snd_elem_info *einfo = (void *)kcontrol->private_value;
	struct soc_enum *e = (struct soc_enum *)einfo->ee;

	return snd_ctl_enum_info(uinfo, e->shift_l == e->shift_r ? 1 : 2,
					 e->items, e->texts);

}

static int snd_enum_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int val;
	struct snd_elem_info *einfo = (void *)kcontrol->private_value;

	val = audiobus_read(einfo->reg);
	val >>= einfo->shift;
	val &= einfo->mask;
	ucontrol->value.integer.value[0] = val;

	pr_info("%s:reg:0x%x, mask:0x%x val:0x%x\n",
		__func__,
		einfo->reg,
		einfo->mask,
		val);

	return 0;
}

static int snd_enum_set(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_elem_info *einfo = (void *)kcontrol->private_value;
	int val  = (int)ucontrol->value.integer.value[0];

	pr_info("%s:reg:0x%x, swap mask:0x%x, val:0x%x\n",
		__func__,
		einfo->reg,
		einfo->mask,
		val);

	audiobus_update_bits(
		einfo->reg,
		einfo->mask << einfo->shift,
		val << einfo->shift);

	return 0;
}

#define SND_ENUM(xname, type, func, xenum, xshift, xmask)   \
{                                      \
	.iface = SNDRV_CTL_ELEM_IFACE_PCM, \
	.name  = xname,                    \
	.info  = snd_enum_info,            \
	.get   = snd_enum_get,             \
	.put   = snd_enum_set,             \
	.private_value = ((unsigned long)&(struct snd_elem_info) \
		{.reg = EE_AUDIO_##type##_##func,   \
		 .ee = (struct soc_enum *)&xenum,   \
		 .shift = xshift, .mask = xmask }) \
}

static const char * const in_swap_channel_text[] = {
	"Swap To Lane0 Left Channel",
	"Swap To Lane0 Right Channel",
	"Swap To Lane1 Left Channel",
	"Swap To Lane1 Right Channel",
	"Swap To Lane2 Left Channel",
	"Swap To Lane2 Right Channel",
	"Swap To Lane3 Left Channel",
	"Swap To Lane3 Right Channel",
};

static const struct soc_enum in_swap_channel_enum =
	SOC_ENUM_SINGLE_EXT(
		ARRAY_SIZE(in_swap_channel_text),
		in_swap_channel_text);

static const char * const out_swap_channel_text[] = {
	"Swap To CH0",
	"Swap To CH1",
	"Swap To CH2",
	"Swap To CH3",
	"Swap To CH4",
	"Swap To CH5",
	"Swap To CH6",
	"Swap To CH7",
};

static const struct soc_enum out_swap_channel_enum =
	SOC_ENUM_SINGLE_EXT(
		ARRAY_SIZE(out_swap_channel_text),
		out_swap_channel_text);

static const char * const lane0_mixer_text[] = {
	"Disable Mix",
	"Lane0 Mix Left and Right Channel",
};

static const struct soc_enum lane0_mixer_enum =
	SOC_ENUM_SINGLE_EXT(
		ARRAY_SIZE(lane0_mixer_text),
		lane0_mixer_text);

static const char * const lane1_mixer_text[] = {
	"Disable Mix",
	"Lane1 Mix Left and Right Channel",
};

static const struct soc_enum lane1_mixer_enum =
	SOC_ENUM_SINGLE_EXT(
		ARRAY_SIZE(lane1_mixer_text),
		lane1_mixer_text);

static const char * const lane2_mixer_text[] = {
	"Disable Mix",
	"Lane2 Mix Left and Right Channel",
};

static const struct soc_enum lane2_mixer_enum =
	SOC_ENUM_SINGLE_EXT(
		ARRAY_SIZE(lane2_mixer_text),
		lane2_mixer_text);

static const char * const lane3_mixer_text[] = {
	"Disable Mix",
	"Lane3 Mix Left and Right Channel",
};

static const struct soc_enum lane3_mixer_enum =
	SOC_ENUM_SINGLE_EXT(
		ARRAY_SIZE(lane3_mixer_text),
		lane3_mixer_text);

static const char * const spdifin_sample_rate_text[] = {
	"24000",
	"32000",
	"44100",
	"46000",
	"48000",
	"96000",
	"192000",
};

static const struct soc_enum spdifin_sample_rate_enum =
	SOC_ENUM_SINGLE_EXT(
		ARRAY_SIZE(spdifin_sample_rate_text),
		spdifin_sample_rate_text);

static int spdifin_sample_rate_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	int mode = spdifin_get_mode();

	ucontrol->value.enumerated.item[0] = mode;
	return 0;
}

static const char * const spdif_channel_status_text[] = {
	"Channel A Status[31:0]",
	"Channel A Status[63:32]",
	"Channel A Status[95:64]",
	"Channel A Status[127:96]",
	"Channel A Status[159:128]",
	"Channel A Status[191:160]",
	"Channel B Status[31:0]",
	"Channel B Status[63:32]",
	"Channel B Status[95:64]",
	"Channel B Status[127:96]",
	"Channel B Status[159:128]",
	"Channel B Status[191:160]",
};

static const struct soc_enum spdif_channel_status_enum =
	SOC_ENUM_SINGLE_EXT(
		ARRAY_SIZE(spdif_channel_status_text),
		spdif_channel_status_text);

static int spdifin_channel_status;

static int spdifout_channel_status;

#define SPDIFIN_CHSTS_REG \
	EE_AUDIO_SPDIFIN_STAT1

#define SPDIFOUT_CHSTS_REG(xinstance) \
	(EE_AUDIO_SPDIFOUT_CHSTS0 + xinstance)

static int spdif_channel_status_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int i;

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0xffffffff;
	uinfo->count = 1;

	for (i = 0; i < e->items; i++)
		pr_info("Item:%d, %s\n", i, e->texts[i]);

	return 0;
}

static int spdifin_channel_status_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int reg, status;

	pr_info("set which channel status you wanted to get firstly\n");
	reg = SPDIFIN_CHSTS_REG;
	status = spdif_get_channel_status(reg);

	ucontrol->value.enumerated.item[0] = status;

	/*channel status value in printk information*/
	pr_info("%s: 0x%x\n",
		e->texts[spdifin_channel_status],
		status
		);
	return 0;
}

static int spdifin_channel_status_set(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int chst = ucontrol->value.enumerated.item[0];
	int ch, valid_bits;

	if (chst < 0 || chst > e->items - 1) {
		pr_err("out of value, fixed it\n");

		if (chst < 0)
			chst = 0;
		if (chst > e->items - 1)
			chst = e->items - 1;
	}
	ch = (chst >= 6);
	valid_bits = (chst >= 6) ? (chst - 6) : chst;

	spdifin_channel_status = chst;
	pr_info("%s\n",
		e->texts[spdifin_channel_status]);

	spdifin_set_channel_status(ch, valid_bits);

	return 0;
}

static int spdifout_channel_status_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int reg, status;

	pr_info("set which channel status you wanted to get firstly\n");
	reg = SPDIFOUT_CHSTS_REG(spdifout_channel_status);
	status = spdif_get_channel_status(reg);

	ucontrol->value.enumerated.item[0] = status;

	/*channel status value in printk information*/
	pr_info("%s: reg:0x%x, status:0x%x\n",
		e->texts[spdifout_channel_status],
		reg,
		status
		);
	return 0;
}

static int spdifout_channel_status_set(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int chst = ucontrol->value.enumerated.item[0];

	if (chst < 0 || chst > e->items - 1) {
		pr_err("out of value, fixed it\n");

		if (chst < 0)
			chst = 0;
		if (chst > e->items - 1)
			chst = e->items - 1;
	}

	spdifout_channel_status = chst;
	pr_info("%s\n",
		e->texts[chst]);

	return 0;
}


#define SPDIFIN_CHSTATUS(xname, xenum)   \
{                                        \
	.iface = SNDRV_CTL_ELEM_IFACE_PCM,   \
	.name  = xname,                      \
	.info  = spdif_channel_status_info,  \
	.get   = spdifin_channel_status_get, \
	.put   = spdifin_channel_status_set,   \
	.private_value = (unsigned long)&xenum \
}

#define SPDIFOUT_CHSTATUS(xname, xenum)   \
{                                         \
	.iface = SNDRV_CTL_ELEM_IFACE_PCM,    \
	.name  = xname,                       \
	.info  = spdif_channel_status_info,   \
	.get   = spdifout_channel_status_get, \
	.put   = spdifout_channel_status_set, \
	.private_value = (unsigned long)&xenum\
}


#define SND_MIX(xname, type, xenum, xshift, xmask)   \
	SND_ENUM(xname, type, CTRL0, xenum, xshift, xmask)

#define SND_SWAP(xname, type, xenum, xshift, xmask)   \
	SND_ENUM(xname, type, SWAP, xenum, xshift, xmask)

#define TDM_MASK(xname, type, func)   \
	SND_INT(xname, type, func)

#define TDM_MUTE(xname, type, func)   \
	SND_INT(xname, type, func)

#define TDM_GAIN(xname, type, func, xshift, xmask)   \
	SND_BYTE(xname, type, func, xshift, xmask)

static const struct snd_kcontrol_new snd_auge_controls[] = {
	/* loopback enable */
	SOC_ENUM_EXT("Loopback Enable",
		     loopback_enable_enum,
		     loopback_enable_get_enum,
		     loopback_enable_set_enum),

	/* loopback data in source */
	SOC_ENUM_EXT("Loopback datain source",
		     loopback_datain_enum,
		     loopback_datain_get_enum,
		     loopback_datain_set_enum),

	/* loopback data tdmin lb */
	SOC_ENUM_EXT("Loopback tmdin lb source",
		     loopback_tdminlb_enum,
		     loopback_tdminlb_get_enum,
		     loopback_tdminlb_set_enum),

	/*TDMIN_A swap*/
	SND_SWAP("TDMIN_A Ch0 Swap", TDMIN_A, in_swap_channel_enum, 0, 0x7),
	SND_SWAP("TDMIN_A Ch1 Swap", TDMIN_A, in_swap_channel_enum, 4, 0x7),
	SND_SWAP("TDMIN_A Ch2 Swap", TDMIN_A, in_swap_channel_enum, 8, 0x7),
	SND_SWAP("TDMIN_A Ch3 Swap", TDMIN_A, in_swap_channel_enum, 12, 0x7),
	SND_SWAP("TDMIN_A Ch4 Swap", TDMIN_A, in_swap_channel_enum, 16, 0x7),
	SND_SWAP("TDMIN_A Ch5 Swap", TDMIN_A, in_swap_channel_enum, 20, 0x7),
	SND_SWAP("TDMIN_A Ch6 Swap", TDMIN_A, in_swap_channel_enum, 24, 0x7),
	SND_SWAP("TDMIN_A Ch7 Swap", TDMIN_A, in_swap_channel_enum, 28, 0x7),
	/*TDMIN_A lane0~3 mask*/
	TDM_MASK("TDMIN_A Lane0 Channel Mask", TDMIN_A, MASK0),
	TDM_MASK("TDMIN_A Lane1 Channel Mask", TDMIN_A, MASK1),
	TDM_MASK("TDMIN_A Lane2 Channel Mask", TDMIN_A, MASK2),
	TDM_MASK("TDMIN_A Lane3 Channel Mask", TDMIN_A, MASK3),
	/*TDMIN_A lane0~3 mute vale, when mute, the channel value*/
	TDM_MUTE("TDMIN_A MUTE_VAL", TDMIN_A, MUTE_VAL),
	/*TDMIN_A lane0~3 mute*/
	TDM_MUTE("TDMIN_A Lane0 Mute Channel", TDMIN_A, MUTE0),
	TDM_MUTE("TDMIN_A Lane1 Mute Channel", TDMIN_A, MUTE1),
	TDM_MUTE("TDMIN_A Lane2 Mute Channel", TDMIN_A, MUTE2),
	TDM_MUTE("TDMIN_A Lane3 Mute Channel", TDMIN_A, MUTE3),

	/*TDMIN_B swap*/
	SND_SWAP("TDMIN_B Ch0 Swap", TDMIN_B, in_swap_channel_enum, 0, 0x7),
	SND_SWAP("TDMIN_B Ch1 Swap", TDMIN_B, in_swap_channel_enum, 4, 0x7),
	SND_SWAP("TDMIN_B Ch2 Swap", TDMIN_B, in_swap_channel_enum, 8, 0x7),
	SND_SWAP("TDMIN_B Ch3 Swap", TDMIN_B, in_swap_channel_enum, 12, 0x7),
	SND_SWAP("TDMIN_B Ch4 Swap", TDMIN_B, in_swap_channel_enum, 16, 0x7),
	SND_SWAP("TDMIN_B Ch5 Swap", TDMIN_B, in_swap_channel_enum, 20, 0x7),
	SND_SWAP("TDMIN_B Ch6 Swap", TDMIN_B, in_swap_channel_enum, 24, 0x7),
	SND_SWAP("TDMIN_B Ch7 Swap", TDMIN_B, in_swap_channel_enum, 28, 0x7),
	/*TDMIN_B lane0~3 mask*/
	TDM_MASK("TDMIN_B Lane0 Channel Mask", TDMIN_B, MASK0),
	TDM_MASK("TDMIN_B Lane1 Channel Mask", TDMIN_B, MASK1),
	TDM_MASK("TDMIN_B Lane2 Channel Mask", TDMIN_B, MASK2),
	TDM_MASK("TDMIN_B Lane3 Channel Mask", TDMIN_B, MASK3),
	/*TDMIN_B lane0~3 mute vale*/
	TDM_MUTE("TDMIN_B MUTE_VAL", TDMIN_B, MUTE_VAL),
	/*TDMIN_B lane0~3 mute*/
	TDM_MUTE("TDMIN_B Lane0 Mute Channel", TDMIN_B, MUTE0),
	TDM_MUTE("TDMIN_B Lane1 Mute Channel", TDMIN_B, MUTE1),
	TDM_MUTE("TDMIN_B Lane2 Mute Channel", TDMIN_B, MUTE2),
	TDM_MUTE("TDMIN_B Lane3 Mute Channel", TDMIN_B, MUTE3),

	/*TDMIN_C swap*/
	SND_SWAP("TDMIN_C Ch0 Swap", TDMIN_C, in_swap_channel_enum, 0, 0x7),
	SND_SWAP("TDMIN_C Ch1 Swap", TDMIN_C, in_swap_channel_enum, 4, 0x7),
	SND_SWAP("TDMIN_C Ch2 Swap", TDMIN_C, in_swap_channel_enum, 8, 0x7),
	SND_SWAP("TDMIN_C Ch3 Swap", TDMIN_C, in_swap_channel_enum, 12, 0x7),
	SND_SWAP("TDMIN_C Ch4 Swap", TDMIN_C, in_swap_channel_enum, 16, 0x7),
	SND_SWAP("TDMIN_C Ch5 Swap", TDMIN_C, in_swap_channel_enum, 20, 0x7),
	SND_SWAP("TDMIN_C Ch6 Swap", TDMIN_C, in_swap_channel_enum, 24, 0x7),
	SND_SWAP("TDMIN_C Ch7 Swap", TDMIN_C, in_swap_channel_enum, 28, 0x7),
	/*TDMIN_C lane0~3 mask*/
	TDM_MASK("TDMIN_C Lane0 Channel Mask", TDMIN_C, MASK0),
	TDM_MASK("TDMIN_C Lane1 Channel Mask", TDMIN_C, MASK1),
	TDM_MASK("TDMIN_C Lane2 Channel Mask", TDMIN_C, MASK2),
	TDM_MASK("TDMIN_C Lane3 Channel Mask", TDMIN_C, MASK3),
	/*TDMIN_C lane0~3 mute vale*/
	TDM_MUTE("TDMIN_C MUTE_VAL", TDMIN_C, MUTE_VAL),
	/*TDMIN_C lane0~3 mute*/
	TDM_MUTE("TDMIN_C Lane0 Mute Channel", TDMIN_C, MUTE0),
	TDM_MUTE("TDMIN_C Lane1 Mute Channel", TDMIN_C, MUTE1),
	TDM_MUTE("TDMIN_C Lane2 Mute Channel", TDMIN_C, MUTE2),
	TDM_MUTE("TDMIN_C Lane3 Mute Channel", TDMIN_C, MUTE3),

	/*TDMIN_LB swap*/
	SND_SWAP("TDMIN_LB Ch0 Swap", TDMIN_LB, in_swap_channel_enum, 0, 0x7),
	SND_SWAP("TDMIN_LB Ch1 Swap", TDMIN_LB, in_swap_channel_enum, 4, 0x7),
	SND_SWAP("TDMIN_LB Ch2 Swap", TDMIN_LB, in_swap_channel_enum, 8, 0x7),
	SND_SWAP("TDMIN_LB Ch3 Swap", TDMIN_LB, in_swap_channel_enum, 12, 0x7),
	SND_SWAP("TDMIN_LB Ch4 Swap", TDMIN_LB, in_swap_channel_enum, 16, 0x7),
	SND_SWAP("TDMIN_LB Ch5 Swap", TDMIN_LB, in_swap_channel_enum, 20, 0x7),
	SND_SWAP("TDMIN_LB Ch6 Swap", TDMIN_LB, in_swap_channel_enum, 24, 0x7),
	SND_SWAP("TDMIN_LB Ch7 Swap", TDMIN_LB, in_swap_channel_enum, 28, 0x7),
	/*TDMIN_LB lane0~3 mask*/
	TDM_MASK("TDMIN_LB Lane0 Channel Mask", TDMIN_LB, MASK0),
	TDM_MASK("TDMIN_LB Lane1 Channel Mask", TDMIN_LB, MASK1),
	TDM_MASK("TDMIN_LB Lane2 Channel Mask", TDMIN_LB, MASK2),
	TDM_MASK("TDMIN_LB Lane3 Channel Mask", TDMIN_LB, MASK3),
	/*TDMIN_LB lane0~3 mute vale*/
	TDM_MUTE("TDMIN_LB MUTE_VAL", TDMIN_LB, MUTE_VAL),
	/*TDMIN_LB lane0~3 mute*/
	TDM_MUTE("TDMIN_LB Lane0 Mute Channel", TDMIN_LB, MUTE0),
	TDM_MUTE("TDMIN_LB Lane1 Mute Channel", TDMIN_LB, MUTE1),
	TDM_MUTE("TDMIN_LB Lane2 Mute Channel", TDMIN_LB, MUTE2),
	TDM_MUTE("TDMIN_LB Lane3 Mute Channel", TDMIN_LB, MUTE3),

	/*TDMOUT_A swap*/
	SND_SWAP("TDMOUT_A Lane0 Left Channel Swap",
		TDMOUT_A, out_swap_channel_enum, 0, 0x7),
	SND_SWAP("TDMOUT_A Lane0 Right Channel Swap",
		TDMOUT_A, out_swap_channel_enum, 4, 0x7),
	SND_SWAP("TDMOUT_A Lane1 Left Channel Swap",
		TDMOUT_A, out_swap_channel_enum, 8, 0x7),
	SND_SWAP("TDMOUT_A Lane1 Right Channel Swap",
		TDMOUT_A, out_swap_channel_enum, 12, 0x7),
	SND_SWAP("TDMOUT_A Lane2 Left Channel Swap",
		TDMOUT_A, out_swap_channel_enum, 16, 0x7),
	SND_SWAP("TDMOUT_A Lane2 Right Channel Swap",
		TDMOUT_A, out_swap_channel_enum, 20, 0x7),
	SND_SWAP("TDMOUT_A Lane3 Left Channel Swap",
		TDMOUT_A, out_swap_channel_enum, 24, 0x7),
	SND_SWAP("TDMOUT_A Lane3 Right Channel Swap",
		TDMOUT_A, out_swap_channel_enum, 28, 0x7),
	/*TDMOUT_A mask value*/
	TDM_MASK("TDMOUT_A Lane0 MASK_VAL", TDMOUT_A, MASK_VAL),
	/*TDMOUT_A lane0~3 mask*/
	TDM_MASK("TDMOUT_A Lane0 Channel Mask", TDMOUT_A, MASK0),
	TDM_MASK("TDMOUT_A Lane1 Channel Mask", TDMOUT_A, MASK1),
	TDM_MASK("TDMOUT_A Lane2 Channel Mask", TDMOUT_A, MASK2),
	TDM_MASK("TDMOUT_A Lane3 Channel Mask", TDMOUT_A, MASK3),
	/*TDMOUT_A gain, enable data * gain*/
	TDM_GAIN("TDMOUT_A GAIN Enable", TDMOUT_A, CTRL1, 26, 0x1),
	TDM_GAIN("TDMOUT_A GAIN CH0", TDMOUT_A, GAIN0, 0, 0xff),
	TDM_GAIN("TDMOUT_A GAIN CH1", TDMOUT_A, GAIN0, 8, 0xff),
	TDM_GAIN("TDMOUT_A GAIN CH2", TDMOUT_A, GAIN0, 16, 0xff),
	TDM_GAIN("TDMOUT_A GAIN CH3", TDMOUT_A, GAIN0, 24, 0xff),
	TDM_GAIN("TDMOUT_A GAIN CH4", TDMOUT_A, GAIN1, 0, 0xff),
	TDM_GAIN("TDMOUT_A GAIN CH5", TDMOUT_A, GAIN1, 8, 0xff),
	TDM_GAIN("TDMOUT_A GAIN CH6", TDMOUT_A, GAIN1, 16, 0xff),
	TDM_GAIN("TDMOUT_A GAIN CH7", TDMOUT_A, GAIN1, 24, 0xff),
	/*TDMOUT_A lane0~3 mute vale*/
	TDM_MUTE("TDMOUT_A MUTE_VAL", TDMOUT_A, MUTE_VAL),
	/*TDMOUT_A lane0~3 mute*/
	TDM_MUTE("TDMOUT_A Lane0 Mute Channel", TDMOUT_A, MUTE0),
	TDM_MUTE("TDMOUT_A Lane1 Mute Channel", TDMOUT_A, MUTE1),
	TDM_MUTE("TDMOUT_A Lane2 Mute Channel", TDMOUT_A, MUTE2),
	TDM_MUTE("TDMOUT_A Lane3 Mute Channel", TDMOUT_A, MUTE3),
	/*TDMOUT_A lane0~3 mixer*/
	SND_MIX("TDMOUT_A Lane0 Mixer Channel",
		TDMOUT_A, lane0_mixer_enum, 20, 0x1),
	SND_MIX("TDMOUT_A Lane1 Mixer Channel",
		TDMOUT_A, lane1_mixer_enum, 21, 0x1),
	SND_MIX("TDMOUT_A Lane2 Mixer Channel",
		TDMOUT_A, lane2_mixer_enum, 22, 0x1),
	SND_MIX("TDMOUT_A Lane3 Mixer Channel",
		TDMOUT_A, lane3_mixer_enum, 23, 0x1),

	/*TDMOUT_B swap*/
	SND_SWAP("TDMOUT_B Lane0 Left Channel Swap",
		TDMOUT_B, out_swap_channel_enum, 0, 0x7),
	SND_SWAP("TDMOUT_B Lane0 Right Channel Swap",
		TDMOUT_B, out_swap_channel_enum, 4, 0x7),
	SND_SWAP("TDMOUT_B Lane1 Left Channel Swap",
		TDMOUT_B, out_swap_channel_enum, 8, 0x7),
	SND_SWAP("TDMOUT_B Lane1 Right Channel Swap",
		TDMOUT_B, out_swap_channel_enum, 12, 0x7),
	SND_SWAP("TDMOUT_B Lane2 Left Channel Swap",
		TDMOUT_B, out_swap_channel_enum, 16, 0x7),
	SND_SWAP("TDMOUT_B Lane2 Right Channel Swap",
		TDMOUT_B, out_swap_channel_enum, 20, 0x7),
	SND_SWAP("TDMOUT_B Lane3 Left Channel Swap",
		TDMOUT_B, out_swap_channel_enum, 24, 0x7),
	SND_SWAP("TDMOUT_B Lane3 Right Channel Swap",
		TDMOUT_B, out_swap_channel_enum, 28, 0x7),
	/*TDMOUT_B mask value*/
	TDM_MASK("TDMOUT_B Lane0 MASK_VAL", TDMOUT_B, MASK_VAL),
	/*TDMOUT_B lane0~3 mask*/
	TDM_MASK("TDMOUT_B Lane0 Channel Mask", TDMOUT_B, MASK0),
	TDM_MASK("TDMOUT_B Lane1 Channel Mask", TDMOUT_B, MASK1),
	TDM_MASK("TDMOUT_B Lane2 Channel Mask", TDMOUT_B, MASK2),
	TDM_MASK("TDMOUT_B Lane3 Channel Mask", TDMOUT_B, MASK3),
	/*TDMOUT_B gain, enable data * gain*/
	TDM_GAIN("TDMOUT_B GAIN Enable", TDMOUT_B, CTRL1, 26, 0x1),
	TDM_GAIN("TDMOUT_B GAIN CH0", TDMOUT_B, GAIN0, 0, 0xff),
	TDM_GAIN("TDMOUT_B GAIN CH1", TDMOUT_B, GAIN0, 8, 0xff),
	TDM_GAIN("TDMOUT_B GAIN CH2", TDMOUT_B, GAIN0, 16, 0xff),
	TDM_GAIN("TDMOUT_B GAIN CH3", TDMOUT_B, GAIN0, 24, 0xff),
	TDM_GAIN("TDMOUT_B GAIN CH4", TDMOUT_B, GAIN1, 0, 0xff),
	TDM_GAIN("TDMOUT_B GAIN CH5", TDMOUT_B, GAIN1, 8, 0xff),
	TDM_GAIN("TDMOUT_B GAIN CH6", TDMOUT_B, GAIN1, 16, 0xff),
	TDM_GAIN("TDMOUT_B GAIN CH7", TDMOUT_B, GAIN1, 24, 0xff),
	/*TDMOUT_B lane0~3 mute vale*/
	TDM_MUTE("TDMOUT_B MUTE_VAL", TDMOUT_B, MUTE_VAL),
	/*TDMOUT_B lane0~3 mute*/
	TDM_MUTE("TDMOUT_B Lane0 Mute Channel", TDMOUT_B, MUTE0),
	TDM_MUTE("TDMOUT_B Lane1 Mute Channel", TDMOUT_B, MUTE1),
	TDM_MUTE("TDMOUT_B Lane2 Mute Channel", TDMOUT_B, MUTE2),
	TDM_MUTE("TDMOUT_B Lane3 Mute Channel", TDMOUT_B, MUTE3),
	/*TDMOUT_B lane0~3 mixer*/
	SND_MIX("TDMOUT_B Lane0 Mixer Channel",
		TDMOUT_B, lane0_mixer_enum, 20, 0x1),
	SND_MIX("TDMOUT_B Lane1 Mixer Channel",
		TDMOUT_B, lane1_mixer_enum, 21, 0x1),
	SND_MIX("TDMOUT_B Lane2 Mixer Channel",
		TDMOUT_B, lane2_mixer_enum, 22, 0x1),
	SND_MIX("TDMOUT_B Lane3 Mixer Channel",
		TDMOUT_B, lane3_mixer_enum, 23, 0x1),

	/*TDMOUT_C swap*/
	SND_SWAP("TDMOUT_C Lane0 Left Channel Swap",
		TDMOUT_C, out_swap_channel_enum, 0, 0x7),
	SND_SWAP("TDMOUT_C Lane0 Right Channel Swap",
		TDMOUT_C, out_swap_channel_enum, 4, 0x7),
	SND_SWAP("TDMOUT_C Lane1 Left Channel Swap",
		TDMOUT_C, out_swap_channel_enum, 8, 0x7),
	SND_SWAP("TDMOUT_C Lane1 Right Channel Swap",
		TDMOUT_C, out_swap_channel_enum, 12, 0x7),
	SND_SWAP("TDMOUT_C Lane2 Left Channel Swap",
		TDMOUT_C, out_swap_channel_enum, 16, 0x7),
	SND_SWAP("TDMOUT_C Lane2 Right Channel Swap",
		TDMOUT_C, out_swap_channel_enum, 20, 0x7),
	SND_SWAP("TDMOUT_C Lane3 Left Channel Swap",
		TDMOUT_C, out_swap_channel_enum, 24, 0x7),
	SND_SWAP("TDMOUT_C Lane3 Right Channel Swap",
		TDMOUT_C, out_swap_channel_enum, 28, 0x7),
	/*TDMOUT_C mask value*/
	TDM_MASK("TDMOUT_C Lane0 MASK_VAL", TDMOUT_C, MASK_VAL),
	/*TDMOUT_C lane0~3 mask*/
	TDM_MASK("TDMOUT_C Lane0 Channel Mask", TDMOUT_C, MASK0),
	TDM_MASK("TDMOUT_C Lane1 Channel Mask", TDMOUT_C, MASK1),
	TDM_MASK("TDMOUT_C Lane2 Channel Mask", TDMOUT_C, MASK2),
	TDM_MASK("TDMOUT_C Lane3 Channel Mask", TDMOUT_C, MASK3),
	/*TDMOUT_C gain, enable data * gain*/
	TDM_GAIN("TDMOUT_C GAIN Enable", TDMOUT_C, CTRL1, 26, 0x1),
	TDM_GAIN("TDMOUT_C GAIN CH0", TDMOUT_C, GAIN0, 0, 0xff),
	TDM_GAIN("TDMOUT_C GAIN CH1", TDMOUT_C, GAIN0, 8, 0xff),
	TDM_GAIN("TDMOUT_C GAIN CH2", TDMOUT_C, GAIN0, 16, 0xff),
	TDM_GAIN("TDMOUT_C GAIN CH3", TDMOUT_C, GAIN0, 24, 0xff),
	TDM_GAIN("TDMOUT_C GAIN CH4", TDMOUT_C, GAIN1, 0, 0xff),
	TDM_GAIN("TDMOUT_C GAIN CH5", TDMOUT_C, GAIN1, 8, 0xff),
	TDM_GAIN("TDMOUT_C GAIN CH6", TDMOUT_C, GAIN1, 16, 0xff),
	TDM_GAIN("TDMOUT_C GAIN CH7", TDMOUT_C, GAIN1, 24, 0xff),
	/*TDMOUT_C lane0~3 mute vale*/
	TDM_MUTE("TDMOUT_C MUTE_VAL", TDMOUT_C, MUTE_VAL),
	/*TDMOUT_C lane0~3 mute*/
	TDM_MUTE("TDMOUT_C Lane0 Mute Channel", TDMOUT_C, MUTE0),
	TDM_MUTE("TDMOUT_C Lane1 Mute Channel", TDMOUT_C, MUTE1),
	TDM_MUTE("TDMOUT_C Lane2 Mute Channel", TDMOUT_C, MUTE2),
	TDM_MUTE("TDMOUT_C Lane3 Mute Channel", TDMOUT_C, MUTE3),
	/*TDMOUT_C lane0~3 mixer*/
	SND_MIX("TDMOUT_C Lane0 Mixer Channel",
		TDMOUT_C, lane0_mixer_enum, 20, 0x1),
	SND_MIX("TDMOUT_C Lane1 Mixer Channel",
		TDMOUT_C, lane1_mixer_enum, 21, 0x1),
	SND_MIX("TDMOUT_C Lane2 Mixer Channel",
		TDMOUT_C, lane2_mixer_enum, 22, 0x1),
	SND_MIX("TDMOUT_C Lane3 Mixer Channel",
		TDMOUT_C, lane3_mixer_enum, 23, 0x1),

	/* SPDIFIN sample rate */
	SOC_ENUM_EXT("SPDIFIN Sample Rate",
			 spdifin_sample_rate_enum,
			 spdifin_sample_rate_get,
			 NULL),
	/* SPDIFIN Channel Status */
	SPDIFIN_CHSTATUS("SPDIFIN Channel Status",
				spdif_channel_status_enum),

	/*SPDIFOUT swap*/
	SND_SWAP("SPDIFOUT Lane0 Left Channel Swap",
		SPDIFOUT, out_swap_channel_enum, 0, 0x7),
	SND_SWAP("SPDIFOUT Lane0 Right Channel Swap",
		SPDIFOUT, out_swap_channel_enum, 4, 0x7),
	/*SPDIFOUT mixer*/
	SND_MIX("SPDIFOUT Mixer Channel",
		SPDIFOUT, lane0_mixer_enum, 23, 0x1),
	/* SPDIFIN Channel Status */
	SPDIFOUT_CHSTATUS("SPDIFOUT Channel Status",
				spdif_channel_status_enum),
};


int snd_card_add_kcontrols(struct snd_soc_card *card)
{
	pr_info("%s card:%p\n", __func__, card);
	return snd_soc_add_card_controls(card,
		snd_auge_controls, ARRAY_SIZE(snd_auge_controls));

}

int loopback_parse_of(struct device_node *node,
	struct loopback_cfg *lb_cfg)
{
	int ret;

	ret = of_property_read_u32(node, "lb_mode",
		&lb_cfg->lb_mode);
	if (ret) {
		pr_err("failed to get lb_mode, set it default\n");
		lb_cfg->lb_mode = 0;
		ret = 0;
	}

	ret = of_property_read_u32(node, "datain_src",
		&lb_cfg->datain_src);
	if (ret) {
		pr_err("failed to get datain_src\n");
		ret = -EINVAL;
		goto fail;
	}
	ret = of_property_read_u32(node, "datain_chnum",
		&lb_cfg->datain_chnum);
	if (ret) {
		pr_err("failed to get datain_chnum\n");
		ret = -EINVAL;
		goto fail;
	}
	ret = of_property_read_u32(node, "datain_chmask",
		&lb_cfg->datain_chmask);
	if (ret) {
		pr_err("failed to get datain_chmask\n");
		ret = -EINVAL;
		goto fail;
	}

	ret = of_property_read_u32(node, "datalb_src",
		&lb_cfg->datalb_src);
	if (ret) {
		pr_err("failed to get datalb_src\n");
		ret = -EINVAL;
		goto fail;
	}
	ret = of_property_read_u32(node, "datalb_chnum",
		&lb_cfg->datalb_chnum);
	if (ret) {
		pr_err("failed to get datalb_chnum\n");
		ret = -EINVAL;
		goto fail;
	}
	ret = of_property_read_u32(node, "datalb_chmask",
		&lb_cfg->datalb_chmask);
	if (ret) {
		pr_err("failed to get datalb_chmask\n");
		ret = -EINVAL;
		goto fail;
	}


	loopback_datain = lb_cfg->datain_src;
	loopback_tdminlb = lb_cfg->datalb_src;

	pr_info("parse loopback:, \tlb mode:%d\n",
		lb_cfg->lb_mode);
	pr_info("\tdatain_src:%d, datain_chnum:%d, datain_chumask:%x\n",
		lb_cfg->datain_src,
		lb_cfg->datain_chnum,
		lb_cfg->datain_chmask);
	pr_info("\tdatalb_src:%d, datalb_chnum:%d, datalb_chumask:%x\n",
		lb_cfg->datalb_src,
		lb_cfg->datalb_chnum,
		lb_cfg->datalb_chmask);

fail:
	return ret;
}

int loopback_prepare(
	struct snd_pcm_substream *substream,
	struct loopback_cfg *lb_cfg)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int bitwidth;
	unsigned int datain_toddr_type, datalb_toddr_type;
	unsigned int datain_msb, datain_lsb, datalb_msb, datalb_lsb;
	struct lb_cfg datain_cfg;
	struct lb_cfg datalb_cfg;
	struct audio_data ddrdata;
	struct data_in datain;
	struct data_lb datalb;

	if (!lb_cfg)
		return -EINVAL;

	bitwidth = snd_pcm_format_width(runtime->format);
	switch (lb_cfg->datain_src) {
	case DATAIN_TDMA:
	case DATAIN_TDMB:
	case DATAIN_TDMC:
	case DATAIN_PDM:
		datain_toddr_type = 0;
		datain_msb = 32 - 1;
		datain_lsb = 0;
		break;
	case DATAIN_SPDIF:
		datain_toddr_type = 3;
		datain_msb = 27;
		datain_lsb = 4;
		if (bitwidth <= 24)
			datain_lsb = 28 - bitwidth;
		else
			datain_lsb = 4;
		break;
	default:
		pr_err("unsupport data in source:%d\n",
			lb_cfg->datain_src);
		return -EINVAL;
	}

	switch (lb_cfg->datalb_src) {
	case TDMINLB_TDMOUTA:
	case TDMINLB_TDMOUTB:
	case TDMINLB_TDMOUTC:
	case TDMINLB_PAD_TDMINA:
	case TDMINLB_PAD_TDMINB:
	case TDMINLB_PAD_TDMINC:
		if (bitwidth == 24) {
			datalb_toddr_type = 4;
			datalb_msb = 32 - 1;
			datalb_lsb = 32 - bitwidth;
		} else {
			datalb_toddr_type = 0;
			datalb_msb = 32 - 1;
			datalb_lsb = 0;
		}
		break;
	default:
		pr_err("unsupport data lb source:%d\n",
			lb_cfg->datalb_src);
		return -EINVAL;
	}

	datain_cfg.ext_signed    = 0;
	datain_cfg.chnum         = lb_cfg->datain_chnum;
	datain_cfg.chmask        = lb_cfg->datain_chmask;
	ddrdata.combined_type    = datain_toddr_type;
	ddrdata.msb              = datain_msb;
	ddrdata.lsb              = datain_lsb;
	ddrdata.src              = lb_cfg->datain_src;
	datain.config            = &datain_cfg;
	datain.ddrdata           = &ddrdata;

	datalb_cfg.ext_signed    = 0;
	datalb_cfg.chnum         = lb_cfg->datalb_chnum;
	datalb_cfg.chmask        = lb_cfg->datalb_chmask;
	datalb.config            = &datalb_cfg;
	datalb.ddr_type          = datalb_toddr_type;
	datalb.msb               = datalb_msb;
	datalb.lsb               = datalb_lsb;

	datain_config(&datain);
	datalb_config(&datalb);

	datalb_ctrl(lb_cfg->datalb_src);
	lb_enable_ex(lb_cfg->lb_mode, true);

	return 0;
}

int loopback_is_enable(void)
{
	return (loopback_enable == 1);
}
