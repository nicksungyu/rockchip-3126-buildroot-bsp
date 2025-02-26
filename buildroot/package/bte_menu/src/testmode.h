#ifndef __TESTMODE_H__
#define __TESTMODE_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include <string.h>


enum GAME_BUTTON 
{
	GAME_BTN_A,
	GAME_BTN_B,
	GAME_BTN_X,
	GAME_BTN_Y,
	GAME_BTN_TL,
	GAME_BTN_TR,
	GAME_BTN_SELECT,
	GAME_BTN_START,
	GAME_BTN_MODE,
	GAME_BTN_THUMBL,
	GAME_BTN_THUMBR,
};

enum GAME_JOYSTICK_AXIS
{
	AXIS_X,				//wheel
	AXIS_Y,
	AXIS_Z,				//break
	AXIS_RX,
	AXIS_RY,
	AXIS_RZ,			//gas
	AXIS_HAT0X,
	AXIS_HAT0Y,
};

enum JOYSTICK_KEY
{
	KEY_START,
	KEY_MUSIC,
	KEY_MUSIC_LEFT,
	KEY_MUSIC_RIGHT,
	KEY_BURST,
	KEY_GEAR_UP,
	KEY_GEAR_DOWN,
	KEY_GEAR_MID,
};

enum DISPLAY_VR_MOTION_TYPE
{
	MOTION_GAS,
	MOTION_GAS_BACK,
	MOTION_BREAK,
	MOTION_BREAK_BACK,
	MOTION_WHEEL_LEFT,
	MOTION_WHEEL_RIGHT,
	MOTION_WHEEL_CENTER,
};

enum KEY_TEST_STEPS
{
	STEP_START_KEY,
	STEP_WHEEL_LEFT,
	STEP_WHEEL_RIGHT,
	STEP_WHEEL_CENTER,
	STEP_BREAK,
	STEP_BREAK_BACK,
	STEP_GAS,
	STEP_GAS_BACK,
	STEP_MUSIC_KEY,
	STEP_MUSIC_LEFT_KEY,
	STEP_MUSIC_RIGHT_KEY,
	STEP_BURST_KEY,
	STEP_GEAR_UP,
	STEP_GEAR_DOWN,
	STEP_GEAR_MID,
	STEP_VOL_DOWN,
	STEP_VOL_MID,
	STEP_VOL_UP,
	STEP_DONE,
};


enum KEY_VOL_MODE
{
	VOL_DOWN_MODE,
	VOL_MID_MODE,	
	VOL_UP_MODE,
};

enum CMD_VR_IOCTRL
{
	CMD_NORMAL_MODE,
	CMD_START_TEST_MODE,
	CMD_START_FACTORY_CALIBRATION,
	CMD_SAVE_WHELL_DATA,
	CMD_SAVE_BREAK_DATA,
	CMD_SAVE_GAS_DATA,
	CMD_RECIEVED_12BIT_DATA,
	CMD_USER_FLASH_WRITE,
	CMD_START_USER_CALIBRATION,
	CMD_GET_VR_DATA,
	CMD_GET_CALBRIATION_DATA,
	CMD_GET_ADC_DATA,
	CMD_GET_12BIT_DATA,
	CMD_ERASE_DATA,
	CMD_WRITE_DATA,
};


enum MODE_CALIBRATION
{
	MODE_NORMAL,
	MODE_TEST,
};


struct VR_DATA {
	unsigned char vr_wheel;
	unsigned char vr_break;
	unsigned char vr_gas;
	char vr_gear;
};

struct CAL_DATA {
	unsigned char wheel_low;
	unsigned char wheel_mid_low;
	unsigned char wheel_mid_high;
	unsigned char wheel_high;
	unsigned char gas_low;
	unsigned char gas_high;
	unsigned char break_low;
	unsigned char break_high;
};

struct ADC_DATA {
	unsigned char wheel_data;
	unsigned char gas_data;
	unsigned char break_data;
	unsigned char adc_wheelh;
	unsigned char adc_wheell;
	unsigned char adc_gash;
	unsigned char adc_gasl;
	unsigned char adc_breakh;
	unsigned char adc_breakl;
};


static int wheel[256] = 
{
-32768, -32512, -32256, -32000, -31744, -31488, -31232, -30976,
-30720, -30464, -30208, -29952, -29696, -29440, -29184, -28928,
-28672, -28416, -28160, -27904, -27648, -27392, -27136, -26880,
-26624, -26368, -26112, -25856, -25600, -25344, -25088, -24832,
-24576, -24320, -24064, -23808, -23552, -23296, -23040, -22784,
-22528, -22272, -22016, -21760, -21504, -21248, -20992, -20736,
-20480, -20224, -19968, -19712, -19456, -19200, -18944, -18688,
-18432, -18176, -17920, -17664, -17408, -17152, -16896, -16640,
-16384, -16128, -15872, -15616, -15360, -15104, -14848, -14592,
-14336, -14080, -13824, -13568, -13312, -13056, -12800, -12544,
-12288, -12032, -11776, -11520, -11264, -11008, -10752, -10496,
-10240, -9984, -9728, -9472, -9216, -8960, -8704, -8448,
-8192, -7936, -7680, -7424, -7168, -6912, -6656, -6400,
-6144, -5888, -5632, -5376, -5120, -4864, -4608, -4352,
-4096, -3840, -3584, -3328, -3072, -2816, -2560, -2304,
-2048, -1792, -1536, -1280, -1024, -768, -512, -256,
0, 256, 512, 768, 1024, 1280, 1536, 1792,
2048, 2304, 2560, 2816, 3072, 3328, 3584, 3840,
4096, 4352, 4608, 4864, 5120, 5376, 5632, 5888,
6144, 6400, 6656, 6912, 7168, 7424, 7680, 7936,
8192, 8448, 8704, 8960, 9216, 9472, 9728, 9984,
10240, 10496, 10752, 11008, 11264, 11520, 11776, 12032,
12288, 12544, 12800, 13056, 13312, 13568, 13824, 14080,
14336, 14592, 14848, 15104, 15360, 15616, 15872, 16128,
16384, 16640, 16896, 17152, 17408, 17664, 17920, 18176,
18432, 18688, 18944, 19200, 19456, 19712, 19968, 20224,
20480, 20736, 20992, 21248, 21504, 21760, 22016, 22272,
22528, 22784, 23040, 23296, 23552, 23808, 24064, 24320,
24576, 24832, 25088, 25344, 25600, 25856, 26112, 26368,
26624, 26880, 27136, 27392, 27648, 27904, 28160, 28416,
28672, 28928, 29184, 29440, 29696, 29952, 30208, 30464,
30720, 30976, 31232, 31488, 31744, 32000, 32256, 32767
};


void ClearKey(void);
void StartTestMode(SDL_Renderer* renderer, TTF_Font* font);
int PlayTestFile(SDL_Renderer* renderer, TTF_Font* font, Mix_Music* mu, char* start_msg, char* end_msg);
int ShowTextBottom(SDL_Renderer* renderer, TTF_Font* font, char* msg, Uint8 r, Uint8 g, Uint8 b, int flush);
void ShowTextBottom(SDL_Renderer* renderer, TTF_Font* font, char* msg);
void ShowTextBottom(SDL_Renderer* renderer, TTF_Font* font, char* msg, Uint8 r, Uint8 g, Uint8 b);
void FactoryCalibrationMode(SDL_Renderer* renderer, TTF_Font* font);

#endif
