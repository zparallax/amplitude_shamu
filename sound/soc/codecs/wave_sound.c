/*
 * wave_sound.c
 * 
 * Copyright 2015 zparallax <zparallax1@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * 
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/mfd/wcd9xxx/core.h>
#include <linux/mfd/wcd9xxx/wcd9xxx_registers.h>
#include <linux/mfd/wcd9xxx/wcd9320_registers.h>
#include <linux/mfd/wcd9xxx/pdata.h>
#include "wcd9320.h"
#include "wcd9xxx-resmgr.h"
#include "wcd9xxx-common.h"

// Definitions
#define WAVE_SOUND_DEFAULT 	0
#define WAVE_SOUND_MIN_VERSION	1
#define WAVE_SOUND_MAX_VERSION	0
// Set the default value for debug
#define WAVE_SOUND_DEBUG	0

#define WAVE_SOUND_HEADPHONE_DEFAULT	0
#define WAVE_SOUND_HEADPHONE_REG_OFFSET	0
#define WAVE_SOUND_HEADPHONE_MIN_GAIN	-30
#define WAVE_SOUND_HEADPHONE_MAX_GAIN	30

#define WAVE_SOUND_SPEAKER_DEFAULT	0
#define WAVE_SOUND_SPEAKER_REG_OFFSET	-4
#define WAVE_SOUND_SPEAKER_MIN_GAIN	-30
#define WAVE_SOUND_SPEAKER_MAX_GAIN	30

#define WAVE_SOUND_MIC_DEFAULT	0
#define WAVE_SOUND_MIC_REG_OFFSET		0
#define WAVE_SOUND_MIC_MIN_GAIN	-30
#define WAVE_SOUND_MIC_MAX_GAIN	30

// pointer to codec
static struct snd_soc_codec *codec_ptr;
// Variable to set the sound control
static int enable_sound;
// Variable to debug
static int debug;

static int headphone_gain_volume_l;
static int headphone_gain_volume_r;
static int speaker_gain_volume_l;
static int speaker_gain_volume_r;
static int mic_gain_level;

extern unsigned int taiko_read(struct snd_soc_codec *codec,
				unsigned int reg);
extern int taiko_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value);

void save_taiko_codec_pointer(struct snd_soc_codec *codec_instance) {
	if(debug)
		printk("wave_sound: Saving pointer instance...\n");
		
	codec_ptr = codec_instance;
	
	if(debug)
		printk("wave_sound: Pointer instance saved...\n");
}
EXPORT_SYMBOL(save_taiko_codec_pointer);


static void reset_audio_values(void) {
	headphone_gain_volume_l = WAVE_SOUND_HEADPHONE_DEFAULT;
	headphone_gain_volume_r = WAVE_SOUND_HEADPHONE_DEFAULT;
	speaker_gain_volume_l = WAVE_SOUND_SPEAKER_DEFAULT;
	speaker_gain_volume_r = WAVE_SOUND_SPEAKER_DEFAULT;
	mic_gain_level = WAVE_SOUND_MIC_DEFAULT;
	
	if(debug)
		printk("wave_sound: Resetting values to it's initial state...\n");
}

static void reset_registered_values(void) {
	taiko_write(codec_ptr, TAIKO_A_CDC_RX1_VOL_CTL_B2_CTL, WAVE_SOUND_HEADPHONE_DEFAULT + WAVE_SOUND_HEADPHONE_REG_OFFSET);
	taiko_write(codec_ptr, TAIKO_A_CDC_RX2_VOL_CTL_B2_CTL, WAVE_SOUND_HEADPHONE_DEFAULT + WAVE_SOUND_HEADPHONE_REG_OFFSET);
	taiko_write(codec_ptr, TAIKO_A_CDC_RX3_VOL_CTL_B2_CTL, WAVE_SOUND_SPEAKER_DEFAULT + WAVE_SOUND_SPEAKER_REG_OFFSET);
	taiko_write(codec_ptr, TAIKO_A_CDC_RX7_VOL_CTL_B2_CTL, WAVE_SOUND_SPEAKER_DEFAULT + WAVE_SOUND_SPEAKER_REG_OFFSET);
	taiko_write(codec_ptr, TAIKO_A_CDC_TX7_VOL_CTL_GAIN, WAVE_SOUND_MIC_DEFAULT + WAVE_SOUND_MIC_REG_OFFSET);
}

unsigned int sound_value_set_taiko_write(unsigned int reg, unsigned int value) {
	// Prevent do anything if it's not active
	if(!enable_sound)
		return value;
		
	if(debug)
		printk("wave_sound: current register %x, current value %d\n", reg, value);
		
	switch(reg) {
		
		case TAIKO_A_CDC_RX1_VOL_CTL_B2_CTL:
			value = headphone_gain_volume_l;
			break;
		case TAIKO_A_CDC_RX2_VOL_CTL_B2_CTL:
			value = headphone_gain_volume_r;
			break;
		case TAIKO_A_CDC_RX3_VOL_CTL_B2_CTL:
			value = speaker_gain_volume_l;
			break;
		case TAIKO_A_CDC_RX7_VOL_CTL_B2_CTL:
			value = speaker_gain_volume_r;
			break;
		case TAIKO_A_CDC_TX7_VOL_CTL_GAIN:
			value = mic_gain_level;
			break;
		default:
			break;
	}
	
	return value;
}
EXPORT_SYMBOL(sound_value_set_taiko_write);

/************************************
 * Sys interface objects
 * *********************************/

static ssize_t wave_sound_show(struct device *dev,
	struct device_attribute *attr, char *buf) {
	return sprintf(buf, "Wave status: %d\n", enable_sound);
}

static ssize_t wave_sound_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count) {
	unsigned int ret = -EINVAL;
	int val;
	ret = sscanf(buf, "%d", &val);
	if (ret != 1)
		return -EINVAL;
	
	if((val == 0) || (val == 1)) {
		enable_sound = val;
		
		reset_audio_values();
		reset_registered_values();
	}
	
	return count;
}

static ssize_t headphone_volume_show(struct device *dev,
	struct device_attribute *attr, char *buf) {
	return sprintf(buf, "Headphone volume: Left=%u, Right=%u\n",
			taiko_read(codec_ptr,
				TAIKO_A_CDC_RX1_VOL_CTL_B2_CTL),
			taiko_read(codec_ptr,
				TAIKO_A_CDC_RX2_VOL_CTL_B2_CTL));
}

static ssize_t headphone_volume_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count) {
	unsigned int ret = -EINVAL;
	int val_l;
	int val_r;
	
	if(!enable_sound)
		return count;
		
	ret = sscanf(buf, "%d %d", &val_l, &val_r);
	
	// Validate that both left and right values does exists
	if(ret != 2)
		return -EINVAL;
		
	// Avoid setting MAX or MIN values
	if(val_l > WAVE_SOUND_HEADPHONE_MAX_GAIN)
		val_l = WAVE_SOUND_HEADPHONE_MAX_GAIN;
		
	if(val_l < WAVE_SOUND_HEADPHONE_MIN_GAIN)
		val_l = WAVE_SOUND_HEADPHONE_MIN_GAIN;
		
	if(val_r > WAVE_SOUND_HEADPHONE_MAX_GAIN)
		val_r = WAVE_SOUND_HEADPHONE_MAX_GAIN;
		
	if(val_r < WAVE_SOUND_HEADPHONE_MIN_GAIN)
		val_r = WAVE_SOUND_HEADPHONE_MIN_GAIN;
		
	headphone_gain_volume_l = val_l;
	headphone_gain_volume_r = val_r;
	
	taiko_write(codec_ptr, TAIKO_A_CDC_RX1_VOL_CTL_B2_CTL,
		headphone_gain_volume_l + WAVE_SOUND_HEADPHONE_REG_OFFSET);
		
	taiko_write(codec_ptr, TAIKO_A_CDC_RX2_VOL_CTL_B2_CTL,
		headphone_gain_volume_r + WAVE_SOUND_HEADPHONE_REG_OFFSET);
	
	if(debug)
		printk("Wave sound: Headphone values: Left=%d, Right=%d",
			headphone_gain_volume_l, headphone_gain_volume_r);
	
	return count;
}

static ssize_t speaker_volume_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Speaker volume: Left=%u, Right=%u\n",
			taiko_read(codec_ptr,
				TAIKO_A_CDC_RX3_VOL_CTL_B2_CTL),
			taiko_read(codec_ptr,
				TAIKO_A_CDC_RX7_VOL_CTL_B2_CTL));
}

static ssize_t speaker_volume_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int ret = -EINVAL;
	int val_l;
	int val_r;
	
	if(!enable_sound)
		return count;
		
	ret = sscanf(buf, "%d %d", &val_l, &val_r);
	
	if (ret != 2)
		return -EINVAL;
		
	if(val_l > WAVE_SOUND_SPEAKER_MAX_GAIN)
		val_l = WAVE_SOUND_SPEAKER_MAX_GAIN;
		
	if(val_l < WAVE_SOUND_SPEAKER_MIN_GAIN)
		val_l = WAVE_SOUND_SPEAKER_MIN_GAIN;
		
	if(val_r > WAVE_SOUND_SPEAKER_MAX_GAIN)
		val_r = WAVE_SOUND_SPEAKER_MAX_GAIN;
		
	if(val_r < WAVE_SOUND_SPEAKER_MIN_GAIN)
		val_r = WAVE_SOUND_SPEAKER_MIN_GAIN;
		
	speaker_gain_volume_l = val_l;
	speaker_gain_volume_r = val_r;
	
	taiko_write(codec_ptr, TAIKO_A_CDC_RX3_VOL_CTL_B2_CTL,
		speaker_gain_volume_l + WAVE_SOUND_SPEAKER_REG_OFFSET);
		
	taiko_write(codec_ptr, TAIKO_A_CDC_RX7_VOL_CTL_B2_CTL,
		speaker_gain_volume_r + WAVE_SOUND_SPEAKER_REG_OFFSET);
		
	if (debug)
		printk("Wave sound: speaker volume Left=%d Right=%d\n",
		speaker_gain_volume_l, speaker_gain_volume_r);
		
	return count;
}

static ssize_t microphone_volume_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n",
		taiko_read(codec_ptr,
			TAIKO_A_CDC_TX3_VOL_CTL_GAIN));

}

static ssize_t microphone_volume_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count) {
	unsigned int ret = -EINVAL;
	unsigned int val;
	
	if(!enable_sound)
		return count;
		
	ret = sscanf(buf, "%d", &val);
	
	if (ret != 1)
		return -EINVAL;
	
	if (val > WAVE_SOUND_MIC_MAX_GAIN)
		val = WAVE_SOUND_MIC_MAX_GAIN;

	if (val < WAVE_SOUND_HEADPHONE_MIN_GAIN)
		val = WAVE_SOUND_HEADPHONE_MIN_GAIN;
		
	mic_gain_level = val;
	
	taiko_write(codec_ptr, TAIKO_A_CDC_TX3_VOL_CTL_GAIN, mic_gain_level);
	
	return count;
}

static ssize_t debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	// return current debug status
	return sprintf(buf, "Debug status: %d\n", debug);
}

static ssize_t debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int ret = -EINVAL;
	unsigned int val;

	// check data and store if valid
	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;
		
	if ((val == 0) || (val == 1))
		debug = val;

	return count;
}

static ssize_t version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Version: %u.%u\n", WAVE_SOUND_MAX_VERSION, WAVE_SOUND_MIN_VERSION);
}

/*******************************************
 * Sysfs folder
 * ****************************************/
 
static DEVICE_ATTR(enable_sound, S_IRUGO | S_IWUGO,
	wave_sound_show, wave_sound_store);
static DEVICE_ATTR(headphone_volume, S_IRUGO | S_IWUGO,
	headphone_volume_show, headphone_volume_store);
static DEVICE_ATTR(speaker_volume, S_IRUGO | S_IWUGO,
	speaker_volume_show, speaker_volume_store);
static DEVICE_ATTR(mic_gain_level, S_IRUGO | S_IWUGO,
	microphone_volume_show, microphone_volume_store);
static DEVICE_ATTR(debug, S_IRUGO | S_IWUGO, debug_show, debug_store);
static DEVICE_ATTR(version, S_IRUGO | S_IWUGO, version_show, NULL);

static struct attribute *wave_sound_attributes[] = {
	&dev_attr_enable_sound.attr,
	&dev_attr_headphone_volume.attr,
	&dev_attr_speaker_volume.attr,
	&dev_attr_mic_gain_level.attr,
	&dev_attr_debug.attr,
	&dev_attr_version.attr,
	NULL
};

static struct attribute_group wave_sound_control_group = {
	.attrs = wave_sound_attributes,
};

static struct miscdevice wave_sound_control_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "wave_sound",
};

static int wave_sound_init(void) {
	misc_register(&wave_sound_control_device);
	if (sysfs_create_group(&wave_sound_control_device.this_device->kobj,
				&wave_sound_control_group) < 0) {
		printk("Wave sound: failed to create sys fs object.\n");
		return 0;
	}
	
	enable_sound = WAVE_SOUND_DEFAULT;
	debug = WAVE_SOUND_DEBUG;
	
	reset_audio_values();
	
	printk("Wave sound: has started, everything has been received well\n");
	
	return 0;
}

static void wave_sound_exit(void) {
	sysfs_remove_group(&wave_sound_control_device.this_device->kobj,
		&wave_sound_control_group);
	
	printk("Wave sound: has stopped, everything set\n");
}

module_init(wave_sound_init);
module_exit(wave_sound_exit);
MODULE_LICENSE("GPL and additional rights");
MODULE_AUTHOR("Apolo Lopez <zparallax1@gmail.com>");
MODULE_DESCRIPTION("Wave Sound 0.x");
