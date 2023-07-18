#include "ar0130.h"
#include "xclk.h"
#include "config.h"
#include <stdio.h>
#include "sccb.h"
#include "global.h"
#include <Arduino.h>
#include "xclk.h"
#include "log.h"
#include "capture.h"
#include "service.h"
#include <stdint.h>

uint16_t ar0130_default_regs[120][2] = {
	{0x301A, 0x0001},
	{0x301A, 0b1000111011000},
	{0x3044, 0x0400},
	{0x3064, 0b1100110000010}, // DISABLE EMB. DATA
	{0x309E, 0x0000},		   // DCDS_PROG_START_ADDR
	{0x30E4, 0x6372},		   // ADC_BITS_6_7
	{0x30E2, 0x7253},		   // ADC_BITS_4_5
	{0x30E0, 0x5470},		   // ADC_BITS_2_3
	{0x30E6, 0xC4CC},		   // ADC_CONFIG1
	{0x30E8, 0x8050},		   // ADC_CONFIG2
	{0x3082, 0x0029},		   // OP MODE CTL
	{0x301E, 0x00C8},		   // DATA_PEDESTAL
	{0x3EDE, 0xC005},		   // DAC_LD_18_19
	{0x3EE2, 0xA46B},		   // DAC_LD_22_23
	{0x3EE0, 0x047D},		   // DAC_LD_20_21
	{0x3EDC, 0x0070},		   // DAC_LD_16_17
	{0x3EE6, 0x4303},		   // DAC_LD_26_27
	{0x3EE4, 0xD208},		   // DAC_LD_24_25
	{0x3ED6, 0x00BD},		   // DAC_LD_10_11
	{0x30EA, 0b110000000000},  // Black Level Correction
	{0x3044, 0b10000000000},	  // Row-wise Noise Correction
	{0x30D4, 0b1110000000000111}, // COLUMN_CORRECTION
	{0x3002, 0x0000},			  // Y_ADDR_START = 2
	{0x3004, 0x0000},			  // X_ADDR_START = 0
	{0x3006, 0x03C0},			  // Y_ADDR_END
	{0x3008, 0x04FF},			  // X_ADDR_END = 1279
	{0x300A, 0x03DE},			  // Vertical blanking / FRAME_LENGTH_LINES
	{0x300C, 0x0672},			  // Horizontal blanking / LINE_LENGTH_PCK = 1650
	{0x3100, 0x0003},
	// {0x31D0, 0x0003},			  // HDR_COMP
	{0x302C, 4},				  // P1=1~16 VT_SYS_CLK_DIV
	{0x302A, 8},				  // P2=4~16 VT_PIX_CLK_DIV
	{0x302E, 4},				  // N=1~63 PRE_PLL_CLK_DIV
	{0x3030, 32},				  // M=32~255 PLL_MULTIPLIER
	{0x3032, 0x0},				  // binning = 00x3056, gain_level & 0xff);
	{0x30B0, 0x1300 | 0b1000100}, // M=32~255 PLL_MULTIPLIER
								  // // XCLK=2MHz<freq/N<24MHz
								  // // fPIXCLK= (fEXTCLK × M) / (N × P1 x P2)
								  // // fPIXCLK= (16*32)/(4*8*4)
	{0x3070, 0x0000},			  // test pattern
	{0x306E, 0},
	{0x311C, 0xFFFF},			  // AE_MAX_EXPOSURE_REG
	{0x311E, 0x0001},			  // AE_MIN_EXPOSURE_REG
	{0x3EDA, 0x0F03}, // DAC_LD_14_15
	{0x3ED8, 0x01EF}, // DAC_LD_12_13
	{0, 0}};

bool status;
long _time = -1;
int _vblank;
int _hblank;
int _black;
int _width;
int _height;
int _gain;
int _xclk;
int xres, yres;
int exp_pin;

long ar0130_get_coarse()
{
	return global_get_coarse();
}

uint16_t ar0130_read16(uint16_t cmd)
{
	return 0; //_sccb->read16(cmd);
}
void ar0130_standby(bool enabled)
{
	if (!enabled)
	{
		// SCCB_Write16_16(0x301A, 0b1100111011110);
		// SCCB_Write16_16(0x301A, 0b1100011011110);
		// SCCB_Write16_16(0x301A, 0x10DC);
		// digitalWrite(EXPOSURE,HIGH);
	}
	else
	{
		// SCCB_Write16_16(0x301A, 0x10D8);
		// SCCB_Write16_16(0x301A, 0b1100111011100);
		// SCCB_Write16_16(0x301A, 0b0100111011100);
		// SCCB_Write16_16(0x301A, 0b1100111011000);
		// digitalWrite(EXPOSURE,LOW);
	}
}

static int get_reg(sensor_t *sensor, int reg, int mask)
{
	int ret = SCCB_Read16_16(sensor->slv_addr, reg & 0xFFFF);
	if (ret > 0)
	{
		ret &= mask;
	}
	return ret;
}

static int set_reg(sensor_t *sensor, int reg, int mask, int value)
{
	int ret = 0;
	ret = SCCB_Read16_16(sensor->slv_addr, reg & 0xFFFF);
	if (ret < 0)
	{
		return ret;
	}
	value = (ret & ~mask) | (value & mask);
	ret = SCCB_Write16_16(sensor->slv_addr, reg & 0xFFFF, value);
	return ret;
}

static int set_reg_bits(sensor_t *sensor, uint16_t reg, uint8_t offset, uint8_t length, uint8_t value)
{
	int ret = 0;
	ret = SCCB_Read16_16(sensor->slv_addr, reg);
	if (ret < 0)
	{
		return ret;
	}
	uint16_t mask = ((1 << length) - 1) << offset;
	value = (ret & ~mask) | ((value << offset) & mask);
	ret = SCCB_Write16_16(sensor->slv_addr, reg & 0xFFFF, value);
	return ret;
}

static int get_reg_bits(sensor_t *sensor, uint16_t reg, uint8_t offset, uint8_t length)
{
	int ret = 0;
	ret = SCCB_Read16_16(sensor->slv_addr, reg);
	if (ret < 0)
	{
		return ret;
	}
	uint8_t mask = ((1 << length) - 1) << offset;
	return (ret & mask) >> offset;
}

static int reset(sensor_t *sensor)
{
	int i = 0;
	const uint16_t(*regs)[2];
	#ifdef EXPOSURE
	pinMode(EXPOSURE, OUTPUT);
	digitalWrite(EXPOSURE, LOW);
	#endif
	// Write default regsiters
	for (i = 0, regs = ar0130_default_regs; regs[i][0]; i++)
	{
		SCCB_Write16_16(sensor->slv_addr, regs[i][0], regs[i][1]);
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}

	// SCCB_Write16_16(sensor->slv_addr,0x3112, 0x029F); // AE_DCG_EXPOSURE_HIGH_REG
	// 	vTaskDelay(10 / portTICK_PERIOD_MS);
	// SCCB_Write16_16(sensor->slv_addr,0x3114, 0x0001); // AE_DCG_EXPOSURE_LOW_REG
	// 	vTaskDelay(10 / portTICK_PERIOD_MS);
	// SCCB_Write16_16(sensor->slv_addr,0x3116, 0x02C0); // AE_DCG_GAIN_FACTOR_REG
	// 	vTaskDelay(10 / portTICK_PERIOD_MS);
	// SCCB_Write16_16(sensor->slv_addr,0x3118, 0x005B); // AE_DCG_GAIN_FACTOR_INV_REG
	// 	vTaskDelay(10 / portTICK_PERIOD_MS);
	// SCCB_Write16_16(sensor->slv_addr,0x3102, 800); // AE_LUMA_TARGET_REG
	// 	vTaskDelay(10 / portTICK_PERIOD_MS);
	// SCCB_Write16_16(sensor->slv_addr,0x3104, 0x1000); // AE_HIST_TARGET_REG
	// 	vTaskDelay(10 / portTICK_PERIOD_MS);
	// SCCB_Write16_16(sensor->slv_addr,0x3108, 16);     // ae_min_ev_step_reg
	// 	vTaskDelay(10 / portTICK_PERIOD_MS);
	// SCCB_Write16_16(sensor->slv_addr,0x310A, 2);     // ae_max_ev_step_reg
	// 	vTaskDelay(10 / portTICK_PERIOD_MS);
	// SCCB_Write16_16(sensor->slv_addr,0x3124, 0x7FFF); // AE_ALPHA_V1_REG
	// 	vTaskDelay(10 / portTICK_PERIOD_MS);
	// SCCB_Write16_16(sensor->slv_addr,0x3126, 0x0080); // AE_ALPHA_V1_REG
	// 	vTaskDelay(10 / portTICK_PERIOD_MS);
	return 0;
}

static int set_pixformat(sensor_t *sensor, pixformat_t pixformat)
{
	int ret = 0;
	sensor->pixformat = pixformat;

	switch (pixformat)
	{
	case PIXFORMAT_RAW:
		// set_reg_bits(sensor, 0x12, 0, 3, 0x4);
		return ret;
		break;
	default:
		return -1;
	}

	// Delay
	vTaskDelay(30 / portTICK_PERIOD_MS);

	return ret;
}

static int set_framesize(sensor_t *sensor, framesize_t framesize)
{
	int ret = 0;
	uint16_t w = resolution[framesize].width;
	uint16_t h = resolution[framesize].height;

	SCCB_Write16_16(sensor->slv_addr, 0x3006, sensor->status.vstart + h - 1); // Y_ADDR_END
	SCCB_Write16_16(sensor->slv_addr, 0x3008, sensor->status.hstart + w - 1); // X_ADDR_END = 1279
	sensor->status.framesize = framesize;
	// Delay
	vTaskDelay(30 / portTICK_PERIOD_MS);

	return ret;
}
static int set_vstart(sensor_t *sensor, uint16_t vstart)
{
	int ret = 0;
	SCCB_Write16_16(sensor->slv_addr, 0x3002, vstart); // Y_ADDR_START = 2
	sensor->status.vstart = vstart;
	// Delay
	vTaskDelay(30 / portTICK_PERIOD_MS);

	return ret;
}

static int set_hstart(sensor_t *sensor, uint16_t hstart)
{
	int ret = 0;
	SCCB_Write16_16(sensor->slv_addr, 0x3004, hstart); // X_ADDR_START = 0
	sensor->status.hstart = hstart;
	// Delay
	vTaskDelay(30 / portTICK_PERIOD_MS);

	return ret;
}
int set_binning(sensor_t *sensor, uint16_t binning)
{
	int ret = 0;
	sensor->status.binning = binning == 1 ? 1 : 0;
	ret |= SCCB_Write16_16(sensor->slv_addr, 0x3032, binning == 1 ? 2 : 0);
	return ret;
}
int set_binning_mode(sensor_t *sensor, int binning_mode)
{
	int ret = 0;
	sensor->status.binning_mode = binning_mode;
	return ret;
}
static int set_colorbar(sensor_t *sensor, int value)
{
	int ret = 0;
	sensor->status.colorbar = value;

	ret |= SCCB_Write16_16(sensor->slv_addr, 0x3070, value);

	return ret;
}
static int set_blc(sensor_t *sensor, int value)
{
	int ret = 0;
	if (set_reg_bits(sensor, 0x30EA, 15, 1, value) >= 0)
	{
		sensor->status.blc = !!value;
	}
	return ret;
}

static int set_gain_ctrl(sensor_t *sensor, int enable)
{
	if (set_reg_bits(sensor, 0x3100, 1, 1, enable) >= 0)
	{
		sensor->status.agc = !!enable;
	}
	return sensor->status.agc;
}

static int set_exposure_ctrl(sensor_t *sensor, int enable)
{
	LOG_UART("set aec:%d\n",enable);
	{
		SCCB_Write16_16(sensor->slv_addr,0x3064, enable?0b1100110000010:0b1100000000010); // DISABLE EMB. DATA
		delay(5);
		if (set_reg_bits(sensor, 0x3100, 0, 1, enable) >= 0)
		{
			sensor->status.aec = !!enable;
		}
	}
	return sensor->status.aec;
}

static int set_hmirror(sensor_t *sensor, int enable)
{
	if (set_reg_bits(sensor, 0x3040, 14, 1, enable) >= 0)
	{
		sensor->status.hmirror = !!enable;
	}
	return sensor->status.hmirror;
}

static int set_vflip(sensor_t *sensor, int enable)
{
	if (set_reg_bits(sensor, 0x3040, 15, 1, enable) >= 0)
	{
		sensor->status.hmirror = !!enable;
	}
	return sensor->status.vflip;
}

static int set_quality(sensor_t *sensor, int quality)
{
	sensor->status.quality = quality;
	return 0;
}
static int set_hblank(sensor_t *sensor, int blank)
{
	SCCB_Write16_16(sensor->slv_addr, 0x300C, (1650 + blank));
	return 0;
}
static int set_vblank(sensor_t *sensor, int blank)
{
	SCCB_Write16_16(sensor->slv_addr, 0x300A, (990 + blank));
	return 0;
}
static int set_aec_exposure(sensor_t *sensor, uint16_t coarse_value, uint16_t fine_value)
{
	LOG_UART("exp:%d\n",coarse_value);
	int ret = 0;
	if(coarse_value==0)
	{
	    set_exposure_ctrl(sensor, 1);
		return ret;
	}
	else 
	{
	    set_exposure_ctrl(sensor, 0);
		delay(5);
	}
	SCCB_Write16_16(sensor->slv_addr, 0x3012, coarse_value);
	delay(5);
	SCCB_Write16_16(sensor->slv_addr, 0x3014, 1040 - fine_value);
	delay(5);
	sensor->status.coarse = coarse_value;
	return ret;
}
static int set_agc_gain(sensor_t *sensor, float gain)
{
	int ret = 0;
	int gain_level = gain * 32;
	// int rb_gain = (uint8_t)(gain * 48) & 0xff;
	// SCCB_Write16_16(sensor->slv_addr, 0x3056, gain_level & 0xff);
	// delay(5);
	// SCCB_Write16_16(sensor->slv_addr, 0x3058, rb_gain);
	// delay(5);
	// SCCB_Write16_16(sensor->slv_addr, 0x305A, rb_gain);
	// delay(5);
	// SCCB_Write16_16(sensor->slv_addr, 0x305C, gain_level & 0xff);
	// delay(5);

	SCCB_Write16_16(sensor->slv_addr, 0x305E, gain_level); // xxx.yyyyy (1x~8x)
	sensor->status.agc_gain = gain_level;
	// Writing a gain to this register is equivalent to writing that code to each of the 4 color-specific gain registers.
	SCCB_Write16_16(sensor->slv_addr, 0x3EE4, 0xD208 | ((0b1 & (gain_level >> 10))) << 8);
	delay(5);
	// Absolute gain programming of column amplifier
	SCCB_Write16_16(sensor->slv_addr, 0x30B0, 0x1000 | ((0b11 & (gain_level >> 8)) << 4)); // 1x~8x

	LOG_UART("gain:%d\n",gain_level);
	return ret;
}
static int set_agc_gain_custom(sensor_t *sensor, float gain_r, float gain_gr, float gain_gb, float gain_b)
{
	int ret = 0;
	uint16_t gr = (uint16_t)(gain_gr*32);
	uint16_t b = (uint16_t)(gain_b*32);
	uint16_t r = (uint16_t)(gain_r*32);
	uint16_t gb = (uint16_t)(gain_gb*32);
	// SCCB_Write16_16(sensor->slv_addr, 0x305E, gr);
	// delay(5);
	SCCB_Write16_16(sensor->slv_addr, 0x3056, gr);
	delay(5);
	SCCB_Write16_16(sensor->slv_addr, 0x3058, b);
	delay(5);
	SCCB_Write16_16(sensor->slv_addr, 0x305A, r);
	delay(5);
	SCCB_Write16_16(sensor->slv_addr, 0x305C, gb);
	delay(5);
	// LOG_UART("gain:%d %d %d %d\n",gr,b,r,gb);
	return ret;
}

static int init_status(sensor_t *sensor)
{
	// sensor->status.brightness = SCCB_Read(sensor->slv_addr, 0x55);
	// sensor->status.contrast = SCCB_Read(sensor->slv_addr, 0x56);
	// sensor->status.saturation = 0;
	// sensor->status.ae_level = 0;

	// sensor->status.gainceiling = SCCB_Read(sensor->slv_addr, 0x87);
	// sensor->status.awb = get_reg_bits(sensor, 0x13, 1, 1);
	// sensor->status.awb_gain = SCCB_Read(sensor->slv_addr, 0xa6);
	set_aec_exposure(sensor,2,100);
	set_agc_gain(sensor, 1.0f);
	sensor->status.quality = 80;
    sensor->set_binning(sensor, 0);
    sensor->set_gain_ctrl(sensor, 0);
    sensor->set_exposure_ctrl(sensor, 0);
	sensor->status.aec = get_reg_bits(sensor, 0x3100, 0, 1);
	sensor->status.agc = get_reg_bits(sensor, 0x3100, 1, 1);
	sensor->status.agc_gain = SCCB_Read16_16(sensor->slv_addr, 0x305E);
	// sensor->status.raw_gma = get_reg_bits(sensor, 0xf1, 1, 1);
	// sensor->status.lenc = get_reg_bits(sensor, 0xf1, 0, 1);
	// sensor->status.hmirror = get_reg_bits(sensor, 0x1e, 5, 1);
	// sensor->status.vflip = get_reg_bits(sensor, 0x1e, 4, 1);

	// sensor->status.colorbar = SCCB_Read(sensor->slv_addr, 0xb9);
	// sensor->status.sharpness = SCCB_Read(sensor->slv_addr, 0x70);

	return 0;
}

static int set_dummy(sensor_t *sensor, int val) { return -1; }
static int set_gainceiling_dummy(sensor_t *sensor, gainceiling_t val) { return -1; }
static int set_res_raw(sensor_t *sensor, int startX, int startY, int endX, int endY, int offsetX, int offsetY, int totalX, int totalY, int outputX, int outputY, bool scale, bool binning) { return -1; }
static int _set_pll(sensor_t *sensor, int bypass, int multiplier, int sys_div, int root_2x, int pre_div, int seld5, int pclk_manual, int pclk_div) { return -1; }

#include "soc/lcd_cam_struct.h"
static int set_xclk(sensor_t *sensor, int xclk)
{
	int ret = 0;
	sensor->xclk_freq_hz = xclk * 1000000U;
	
    LCD_CAM.cam_ctrl.cam_clkm_div_num = 160000000 / sensor->xclk_freq_hz;
	return ret;
}

int ar0130_detect(int slv_addr, sensor_id_t *id)
{
	if (AR0130_SCCB_ADDR == slv_addr)
	{
		SCCB_Write16_16(slv_addr, 0x3000, 0x2402);
		uint16_t PID = SCCB_Read16_16(slv_addr, 0x3000);
		if (AR0130_PID == PID)
		{
			id->PID = PID;
			id->VER = 0;  // SCCB_Read(slv_addr, 0xFD);
			id->MIDL = 0; // SCCB_Read(slv_addr, 0xFC);
			id->MIDH = 0; // SCCB_Read(slv_addr, 0xFD);
			return PID;
		}
		else
		{
			LOG_UART("Mismatch PID=0x%x\n", PID);
		}
	}
	return 0;
}
static int take_photo(sensor_t *sensor, bool preview, fileformat_t format)
{
	// delay(5);
	if(preview)
	{
		// delay(20);
		service_turn_off();
		// delay(100);
	}
	#ifdef EXPOSURE
	digitalWrite(LED_IO, HIGH);
	while (digitalRead(VSYNC));
	digitalWrite(EXPOSURE, HIGH);
	#endif
	capture_start(preview, format);
	return 0;
}
static int take_photo_end(sensor_t *sensor)
{
	#ifdef EXPOSURE
	digitalWrite(EXPOSURE, LOW);
	digitalWrite(LED_IO, LOW);
	#endif
	return 0;
}
int ar0130_init(sensor_t *sensor)
{
	// Set function pointers
	pinMode(VSYNC,INPUT);
	sensor->reset = reset;
	sensor->init_status = init_status;
	sensor->take_photo = take_photo;
	sensor->take_photo_end = take_photo_end;
	sensor->set_binning = set_binning;
	sensor->set_binning_mode = set_binning_mode;
	sensor->set_pixformat = set_pixformat;
	sensor->set_framesize = set_framesize;
	sensor->set_vstart = set_vstart;
	sensor->set_hstart = set_hstart;
	sensor->set_vblank = set_vblank;
	sensor->set_hblank = set_hblank;
	sensor->set_colorbar = set_colorbar;
	sensor->set_blc = set_blc;
	sensor->set_gain_ctrl = set_gain_ctrl;
	sensor->set_exposure_ctrl = set_exposure_ctrl;
	sensor->set_hmirror = set_hmirror;
	sensor->set_vflip = set_vflip;
	sensor->set_agc_gain = set_agc_gain;
	sensor->set_agc_gain_custom = set_agc_gain_custom;
	sensor->set_aec_exposure = set_aec_exposure;

	// not supported
	sensor->set_brightness = set_dummy;
	sensor->set_contrast = set_dummy;
	sensor->set_whitebal = set_dummy;
	sensor->set_awb_gain = set_dummy;
	sensor->set_raw_gma = set_dummy;
	sensor->set_lenc = set_dummy;
	sensor->set_sharpness = set_dummy;
	sensor->set_saturation = set_dummy;
	sensor->set_denoise = set_dummy;
	sensor->set_quality = set_quality;
	sensor->set_special_effect = set_dummy;
	sensor->set_wb_mode = set_dummy;
	sensor->set_ae_level = set_dummy;
	sensor->set_gainceiling = set_gainceiling_dummy;

	// sensor->get_reg = get_reg;
	// sensor->set_reg = set_reg;
	sensor->set_res_raw = set_res_raw;
	sensor->set_pll = _set_pll;
	sensor->set_xclk = set_xclk;

	LOG_UART("AR0130 Attached");

	return 0;
}