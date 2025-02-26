#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <SDL2/SDL_image.h>
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
#include <fstream>
#include <vector>
#include <sys/mount.h>

using namespace std;

#ifdef _DEBUG
#define DEBUG(format, args...) printf(format, ##args)
#else
#define DEBUG(args...)
#endif

extern int WIDTH;
extern int HEIGHT;
extern int test_mode;
extern volatile unsigned int g_vol;
extern void EnablePowerButton(int enable);

#define MENU_FONT "/usr/share/menu/font/DroidSerif-Bold.ttf"
#define AUDIO_TEST_POSTIVE "/usr/share/menu/sound/positive.wav"
#define AUDIO_TEST_NEGATIVE "/usr/share/menu/sound/negative_2.wav"
#define AUDIO_SELECT_FILE "/usr/share/menu/sound/select_ding.wav"
#define ANGLE 0
#define ENG_MODE_BG "/usr/share/menu/images/RK3126_ClassOf1981Deluxe_1280x960_ENG.jpg"
#define QA_MODE_BG "/usr/share/menu/images/RK3126_ClassOf1981Deluxe_1280x960_QA.jpg"
//#define SKIP_CHECKSUM 1
#define MODEL   "/etc/model" //Model Number = SKU Number, 7433 = Mortal Kombat
#define MOO     "/etc/vn" //MOO Version
#define SYSTEM	"System Version : "
#define PCB     "/etc/pcba"

#define YEAR ((((__DATE__ [7] - '0') * 10 + (__DATE__ [8] - '0')) * 10 + (__DATE__ [9] - '0')) * 10 + (__DATE__ [10] - '0'))
#define MONTH (\
  __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 1 : 6) \
: __DATE__ [2] == 'b' ? 2 \
: __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4) \
: __DATE__ [2] == 'y' ? 5 \
: __DATE__ [2] == 'l' ? 7 \
: __DATE__ [2] == 'g' ? 8 \
: __DATE__ [2] == 'p' ? 9 \
: __DATE__ [2] == 't' ? 10 \
: __DATE__ [2] == 'v' ? 11 \
: 12)
#define DAY ((__DATE__ [4] == ' ' ? 0 : __DATE__ [4] - '0') * 10 + (__DATE__ [5] - '0'))
SDL_Texture* test_qabg = NULL;
SDL_Texture* test_engbg = NULL;
SDL_Texture* test_vol[3] = { NULL, NULL, NULL};
SDL_Texture* test_p1_button[11] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
SDL_Texture* test_p2_button[11] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
Mix_Music* test_mix_music[4] = { NULL, NULL, NULL, NULL};
char test_music_name[4][256] = { "/usr/share/menu/sound/1k.mp3", "/usr/share/menu/sound/2k-right.mp3",
"/usr/share/menu/sound/1k-left.mp3", "" };
char cksum_now[1024];
char cksum_save[1024];
long int cksum_total_times = 0;
long int cksum_pass_times = 0;
long int cksum_fail_times = 0;

// format=> X, Y, key, width, height, enable
struct tk{
	int key[7]; //X, Y, key, width, height, enable, status
	char pic_name[256];
	SDL_Texture* tex;
	int which; //for joystick
};

#define TEST_KEY_NUM 11
struct tk test_key[TEST_KEY_NUM] = {
	{ {520, 430, SDLK_UP    , 0, 0, 1, 0}, "/usr/share/menu/images/Arrow_Left.png", NULL}, // P1 UP
	{ {683, 430, SDLK_DOWN  , 0, 0, 1, 0}, "/usr/share/menu/images/Arrow_Right.png", NULL}, // P1 DOWN
	{ {601, 517, SDLK_LEFT  , 0, 0, 1, 0}, "/usr/share/menu/images/Arrow_Down.png", NULL}, // P1 LEFT
	{ {601, 353, SDLK_RIGHT , 0, 0, 1, 0}, "/usr/share/menu/images/Arrow_Up.png", NULL}, // P1 RIGHT
	{ {628, 188, SDLK_x     , 0, 0, 1, 0}, "/usr/share/menu/images/green_button.png", NULL},// G1-C
	{ {628, 104, SDLK_z     , 0, 0, 1, 0}, "/usr/share/menu/images/green_button.png", NULL},// G1-A
	{ {290, 683, SDLK_F1    , 0, 0, 1, 0}, "/usr/share/menu/images/green_button.png", NULL},// Live Button
	{ {566, 685, SDLK_RETURN, 0, 0, 1, 0}, "/usr/share/menu/images/green_button.png", NULL},// P1 Start
	{ {673, 685, SDLK_KP_6  , 0, 0, 1, 0}, "/usr/share/menu/images/green_button.png", NULL},// P2 Start
	{ {297, 543, SDLK_a     , 0, 0, 1, 0}, "/usr/share/menu/images/Vol_0_G.png", NULL},// Power off
	{ {297, 543, SDLK_q     , 0, 0, 1, 0}, "/usr/share/menu/images/Vol_2_G.png", NULL},// Power on
};

int vol_img[3][5] = {
	{297, 272, 0, 0, 0}, //Vol0
	{297, 272, 1, 0, 0}, //Vol1
	{297, 272, 2, 0, 0}, //Vol2
};

void ClearKey(void){
	SDL_Event event;
        while(1){//Read All key event and drop it
                if(SDL_PollEvent(&event) != 0){
                        ;
                }else{
                        break;
                }
        }
}

void FillScreen(SDL_Renderer* renderer, int r, int g, int b, int flush){
	SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	SDL_Rect rect;
	rect.x = 0; rect.y = 0; rect.w = WIDTH; rect.h = HEIGHT;
	SDL_RenderFillRect(renderer, &rect);
	if(flush)
		SDL_RenderPresent(renderer);
}

int ShowText(SDL_Renderer* renderer, TTF_Font* font, char* msg, int x, int y, int flush){
	int texW = 0, texH = 0, ret = 0;
	SDL_Rect dstrect;
	SDL_Color color = {255, 255, 255};
	SDL_Surface* message = TTF_RenderText_Solid(font, msg, color);
	SDL_Texture* txtmsg = SDL_CreateTextureFromSurface(renderer, message);

	SDL_QueryTexture(txtmsg, NULL, NULL, &texW, &texH);
#if defined(LANDSCAP)
	dstrect.x = (WIDTH - texW) / 2;
	dstrect.y = (HEIGHT - texH) / 2 + y;
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopy(renderer, txtmsg, NULL, &dstrect);
#elif defined(PORTRAIT)
	dstrect.x = (WIDTH - texW) / 2 + y;
	dstrect.y = (HEIGHT - texH) / 2;
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopyEx(renderer, txtmsg, NULL, &dstrect, -90, NULL, SDL_FLIP_NONE);
#endif
	ret = texH;
	if(flush)
		SDL_RenderPresent(renderer);
	if(txtmsg) SDL_DestroyTexture(txtmsg);
	if(message) SDL_FreeSurface(message);
	return ret;
}

int ShowText(SDL_Renderer* renderer, TTF_Font* font, char* msg, int x, int y, Uint8 r, Uint8 g, Uint8 b, int flush){
	int texW = 0, texH = 0, ret = 0;
	int w,h;
	SDL_Rect dstrect;
	SDL_Color color = {r, g, b};
	SDL_Surface* message = TTF_RenderText_Solid(font, msg, color);
	SDL_Texture* txtmsg = SDL_CreateTextureFromSurface(renderer, message);

	SDL_QueryTexture(txtmsg, NULL, NULL, &texW, &texH);
#if defined(LANDSCAP)
	dstrect.x = (WIDTH - texW) / 2;
	dstrect.y = (HEIGHT - texH) / 2 + y;
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopy(renderer, txtmsg, NULL, &dstrect);
#elif defined(PORTRAIT)
	dstrect.x = (WIDTH - texW) / 2 + y;
	dstrect.y = (HEIGHT - texH) / 2;
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopyEx(renderer, txtmsg, NULL, &dstrect, -90, NULL, SDL_FLIP_NONE);
#endif
	ret = texH;
	if(flush)
		SDL_RenderPresent(renderer);
	if(txtmsg) SDL_DestroyTexture(txtmsg);
	if(message) SDL_FreeSurface(message);
	return ret;
}

int ShowText(SDL_Renderer* renderer, TTF_Font* font, char* msg, int x, int y, SDL_Color color, int flush){
        int texW = 0, texH = 0, ret = 0;
        int w,h;
        SDL_Rect dstrect;
        SDL_Surface* message = TTF_RenderText_Solid(font, msg, color);
        SDL_Texture* txtmsg = SDL_CreateTextureFromSurface(renderer, message);

        SDL_QueryTexture(txtmsg, NULL, NULL, &texW, &texH);
#if defined(LANDSCAP)
        dstrect.x = (WIDTH - texW) / 2;
        dstrect.y = (HEIGHT - texH) / 2 + y;
        dstrect.w = texW;
        dstrect.h = texH;
        SDL_RenderCopy(renderer, txtmsg, NULL, &dstrect);
#elif defined(PORTRAIT)
        dstrect.x = (WIDTH - texW) / 2 + y;
        dstrect.y = (HEIGHT - texH) / 2;
        dstrect.w = texW;
        dstrect.h = texH;
        SDL_RenderCopyEx(renderer, txtmsg, NULL, &dstrect, -90, NULL, SDL_FLIP_NONE);
#endif
        ret = texH;
        if(flush)
                SDL_RenderPresent(renderer);
        if(txtmsg) SDL_DestroyTexture(txtmsg);
        if(message) SDL_FreeSurface(message);
        return ret;
}


//Checksum X
int ShowTextBottom(SDL_Renderer* renderer, TTF_Font* font, char* msg, Uint8 r, Uint8 g, Uint8 b, int flush){
	int texW = 0, texH = 0, ret = 0;
	int w,h;
	SDL_Rect dstrect;
	SDL_Color color = {r, g, b};
	SDL_Surface* message = TTF_RenderText_Solid(font, msg, color);
	SDL_Texture* txtmsg = SDL_CreateTextureFromSurface(renderer, message);

	SDL_QueryTexture(txtmsg, NULL, NULL, &texW, &texH);
#if defined(LANDSCAP)
	dstrect.x = (WIDTH - texW) / 2;
	dstrect.y = (HEIGHT - texH);
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopy(renderer, txtmsg, NULL, &dstrect);
#elif defined(PORTRAIT)
	dstrect.x = (WIDTH - texW);
	dstrect.y = (HEIGHT - texH) / 2;
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopyEx(renderer, txtmsg, NULL, &dstrect, -90, NULL, SDL_FLIP_NONE);
#endif
	ret = texH;
	if(flush)
		SDL_RenderPresent(renderer);
	if(txtmsg) SDL_DestroyTexture(txtmsg);
	if(message) SDL_FreeSurface(message);
	return ret;
}
//PlayTest File
void ShowTextBottom(SDL_Renderer* renderer, TTF_Font* font, char* msg){
	int texW = 0, texH = 0;
	SDL_Rect dstrect;
	SDL_Color color = {255, 255, 255};
	SDL_Surface* message = TTF_RenderText_Solid(font, msg, color);
	SDL_Texture* txtmsg = SDL_CreateTextureFromSurface(renderer, message);

	SDL_QueryTexture(txtmsg, NULL, NULL, &texW, &texH);
#if defined(LANDSCAP)
	dstrect.x = (WIDTH - texW) / 2;
	dstrect.y = (HEIGHT - texH);
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopy(renderer, txtmsg, NULL, &dstrect);
#elif defined(PORTRAIT)
	dstrect.x = (WIDTH - texW / 2 - texH);
	dstrect.y = (HEIGHT - texH) / 2;
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopyEx(renderer, txtmsg, NULL, &dstrect, -90, NULL, SDL_FLIP_NONE);
	DEBUG("dstrect.x=%d, dstrect.y=%d, w=%d, h=%d\n", dstrect.x, dstrect.y, dstrect.w, dstrect.h);
#endif
	SDL_RenderPresent(renderer);
	if(txtmsg) SDL_DestroyTexture(txtmsg);
	if(message) SDL_FreeSurface(message);
}

void ShowTextBottom(SDL_Renderer* renderer, TTF_Font* font, char* msg, Uint8 r, Uint8 g, Uint8 b){
	int texW = 0, texH = 0;
	SDL_Rect dstrect;
	SDL_Color color = {r, g, b};
	SDL_Surface* message = TTF_RenderText_Solid(font, msg, color);
	SDL_Texture* txtmsg = SDL_CreateTextureFromSurface(renderer, message);

	SDL_QueryTexture(txtmsg, NULL, NULL, &texW, &texH);
#if defined(LANDSCAP)
	dstrect.x = (WIDTH - texW) / 2;
	dstrect.y = (HEIGHT - texH * 2);
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopy(renderer, txtmsg, NULL, &dstrect);
#elif defined(PORTRAIT)
	dstrect.x = (WIDTH - texW / 2 - texH);
	dstrect.y = (HEIGHT - texH) / 2;
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopyEx(renderer, txtmsg, NULL, &dstrect, -90, NULL, SDL_FLIP_NONE);
#endif
	SDL_RenderPresent(renderer);
	if(txtmsg) SDL_DestroyTexture(txtmsg);
	if(message) SDL_FreeSurface(message);
}

void Test_Success(SDL_Renderer* renderer){
	char msg[1024];
	TTF_Font* font48 = TTF_OpenFont(MENU_FONT, 48);
	FillScreen(renderer, 0, 0, 0, 0); //Black
	if(font48){
		sprintf(msg, "%s", "Key Test Success!");
		ShowText(renderer, font48, msg, 0, 0, 0, 255, 0, 1);
	}else {DEBUG("open %s failed\n", MENU_FONT);}

	Mix_Chunk *wav = Mix_LoadWAV(AUDIO_TEST_POSTIVE);
	if(wav){
		if(Mix_PlayChannel(-1, wav, 0) == -1){
			DEBUG("Mix_PlayChannel failed [%s]\n", Mix_GetError());
		}
	}
	SDL_Delay(2000);
	if(wav) Mix_FreeChunk(wav);
	if(font48) TTF_CloseFont(font48);
}

void Test_Fail(SDL_Renderer* renderer){
	char msg[1024];
	TTF_Font* font48 = TTF_OpenFont(MENU_FONT, 48);
	FillScreen(renderer, 0, 0, 0, 0); //Black
	if(font48){
		sprintf(msg, "%s", "Key Test Fail!");
		ShowText(renderer, font48, msg, 0, 0, 255, 0, 0, 1);
	}else {DEBUG("open %s failed\n", MENU_FONT);}

	Mix_Chunk *wav = Mix_LoadWAV(AUDIO_TEST_NEGATIVE);
	if(wav){
		if(Mix_PlayChannel(-1, wav, 2) == -1){ //2 is mean 2+1=3 times
			DEBUG("Mix_PlayChannel failed [%s]\n", Mix_GetError());
		}
	}
	SDL_Delay(6000);
	if(wav) Mix_FreeChunk(wav);
	if(font48) TTF_CloseFont(font48);
}

int Check_1P_AB_Button(char* msg){
	SDL_Event event;
	ClearKey();
	while(1){
		if(SDL_PollEvent(&event) != 0){
			switch(event.type){
				case SDL_KEYDOWN:
					if(SDLK_x == event.key.keysym.sym){
						return 1;
					}
					if(SDLK_z == event.key.keysym.sym){
						return 2;
					}
				default:
					break;
			}
		}
		SDL_Delay(5);
	}
}

void Check_1P_A_Button(char* msg){
	SDL_Event event;
	ClearKey();
	while(1){
		if(SDL_PollEvent(&event) != 0){
			switch(event.type){
				case SDL_KEYDOWN:
					if(SDLK_x == event.key.keysym.sym){
						return;
					}
					break;
				default:
					break;
			}
		}
		SDL_Delay(5);
	}
}

int Check_1P_A_Button_2sec(){
	SDL_Event event;
	Uint32 start = 0;
	int a_button = 0;
	ClearKey();
	while(1){
		if(SDL_PollEvent(&event) != 0){
			switch(event.type){
				case SDL_KEYUP:
					if(SDLK_x == event.key.keysym.sym){
						if(a_button == 1){
							printf("%s: x key press up %ld\n", __func__, SDL_GetTicks());
							if( ((SDL_GetTicks() - start) / 1000.0) >= 2.0){
								return 2;//hold a button for 2 sec
							}else{
								return 1;
							}
						}
					}
					else if( SDLK_z == event.key.keysym.sym ) // Press B, enter Wifi Test for BTE
						return 0;
					break;
				case SDL_KEYDOWN:
					if(SDLK_x == event.key.keysym.sym && event.key.repeat == 0){
						start = SDL_GetTicks();
						a_button = 1;
						printf("%s: x key press down %ld\n", __func__, start);
					}
					break;
				default:
					break;
			}
		}
		if(a_button == 1){
			if( ((SDL_GetTicks() - start) / 1000.0) >= 2.0){
				return 2;//hold a button for 2 sec
			}
		}
		SDL_Delay(5);
	}
}

int PlayTestFile(SDL_Renderer* renderer, TTF_Font* font, Mix_Music* mu, char* start_msg, char* end_msg){
	char msg_tmp[1024];
	memset(msg_tmp, '\0', sizeof(msg_tmp));

	SDL_Event event;
	sprintf(msg_tmp, "%s", "Sound Testing...");
	ShowText(renderer, font, msg_tmp, 0, 0, 0);
	ShowTextBottom(renderer, font, start_msg);
	if(mu != NULL){
		if(Mix_PlayMusic(mu, 1) == -1){
			DEBUG("Mix_PlayMusic failed[%s]\n", Mix_GetError());
		}else{
			while(Mix_PlayingMusic() == 1){
				if(SDL_PollEvent(&event) != 0){
					if(event.type == SDL_KEYDOWN){
						if(SDLK_x == event.key.keysym.sym){
							Mix_HaltMusic();
							return -1;
						}
					}
				}
			}
		}
	}

	if(end_msg != NULL)
		ShowTextBottom(renderer, font, end_msg);
	return 1;
}

void DrawQAKey(SDL_Renderer* renderer, TTF_Font* font, int player, int index, int flush){
	SDL_Rect tp;
	if(test_qabg){
		SDL_RenderCopy(renderer, test_qabg, NULL, NULL);
	}
	if(test_key[index].tex){
		tp.x = test_key[index].key[0]; tp.y = test_key[index].key[1];
		tp.w = test_key[index].key[3]; tp.h = test_key[index].key[4];
		SDL_RenderCopy(renderer, test_key[index].tex, NULL, &tp);
	}
	if(flush == 1)
		SDL_RenderPresent(renderer);
}

int CheckQAVol(SDL_Renderer* renderer, TTF_Font* font, int vol){
	Uint32 start_time = SDL_GetTicks();
	int idle_time_5 = 5;
	SDL_Delay(5);
	while(1){
		if(g_vol == vol){
			return 1;
		}
		if(((SDL_GetTicks() - start_time) / 1000.0) > idle_time_5){
			return -5;
		}
		SDL_Delay(5);
	}
}

int CheckQAKey(SDL_Renderer* renderer, TTF_Font* font, int key){
	SDL_Event event;
	Uint32 start_time = SDL_GetTicks();
	int idle_time_5 = 5, idle_time_10 = 10;
	int button_down = 0;
	//ClearKey();
	while(1){
		if(SDL_PollEvent(&event) != 0){
			switch(event.type){
				case SDL_KEYUP:
					if((event.key.keysym.sym == key) && (button_down == 1)){
						return key;
					}else{
						return -1;
					}
					break;
				case SDL_KEYDOWN:
					if(event.key.keysym.sym == key){
						start_time = SDL_GetTicks();
						button_down = 1;
					}else{
						return -1;
					}
					break;
				default:
					break;
			}
		}
		if(((SDL_GetTicks() - start_time) / 1000.0) > idle_time_5){
			return -5;
		}
		SDL_Delay(5);
	}
}

static int TestMusicThread(void* render){
	Mix_Chunk *select = Mix_LoadWAV(AUDIO_SELECT_FILE);
	DEBUG("test music thread play sound start\n");
	if(select == NULL){
		DEBUG("Mix_LoadMUS /pic/select_ding.wav failed[%s]\n", Mix_GetError());
	}else{
		if(Mix_PlayChannel(-1, select, 0) == -1){
			DEBUG("Mix_PlayChannel failed [%s]\n", Mix_GetError());
		}
		SDL_Delay(1500);
	}
	DEBUG("test music thread play sound end\n");
	if(Mix_PlayingMusic() == 1){
		Mix_HaltMusic();
	}
	if(select) Mix_FreeChunk(select);
	return 0;
}

void TestModeInit(SDL_Renderer* renderer, TTF_Font* font){
	int i = 0;
	test_mix_music[0] = Mix_LoadMUS(test_music_name[0]);
	test_mix_music[1] = Mix_LoadMUS(test_music_name[1]);
	test_mix_music[2] = Mix_LoadMUS(test_music_name[2]);
	test_qabg = IMG_LoadTexture(renderer, QA_MODE_BG);
	if(test_qabg == NULL) {DEBUG("Load QA background texture failed\n");}
	test_engbg = IMG_LoadTexture(renderer, ENG_MODE_BG);
	if(test_engbg == NULL) {DEBUG("Load ENG background texture failed\n");}
	for(int i = 0; i < TEST_KEY_NUM; i++){
		test_key[i].tex = IMG_LoadTexture(renderer, test_key[i].pic_name);
	}
	test_p2_button[0] = IMG_LoadTexture(renderer, "/usr/share/menu/images/Arrow_Right.png");
	test_p2_button[1] = IMG_LoadTexture(renderer, "/usr/share/menu/images/Arrow_Left.png");
	test_p2_button[2] = IMG_LoadTexture(renderer, "/usr/share/menu/images/Arrow_Up.png");
	test_p2_button[3] = IMG_LoadTexture(renderer, "/usr/share/menu/images/Arrow_Down.png");
	test_p2_button[4] = IMG_LoadTexture(renderer, "/usr/share/menu/images/green_button.png");
	test_p2_button[5] = test_p2_button[4];
	test_p2_button[6] = test_p2_button[4];
	test_p2_button[7] = test_p2_button[4];
	test_p2_button[8] = test_p2_button[4];
	test_p2_button[9] = test_p2_button[4];
	test_p2_button[10] = test_p2_button[4];
	test_vol[0] = IMG_LoadTexture(renderer, "/usr/share/menu/images/Vol_0_G.png");
	test_vol[1] = IMG_LoadTexture(renderer, "/usr/share/menu/images/Vol_1_G.png");
	test_vol[2] = IMG_LoadTexture(renderer, "/usr/share/menu/images/Vol_2_G.png");
	for(i = 0; i < TEST_KEY_NUM; i++){
		SDL_QueryTexture(test_key[i].tex, NULL, NULL, &test_key[i].key[3], &test_key[i].key[4]);
	}
	SDL_QueryTexture(test_vol[0], NULL, NULL, &vol_img[0][3], &vol_img[0][4]);
	SDL_QueryTexture(test_vol[1], NULL, NULL, &vol_img[1][3], &vol_img[1][4]);
	SDL_QueryTexture(test_vol[2], NULL, NULL, &vol_img[2][3], &vol_img[2][4]);
}

void get_cksum_save(void){
	FILE* fd = NULL;
	int ret = -1, i = 0, j = 0, index = 0;
	struct stat s;

	memset(cksum_save, 0, 1024);
	ret = stat("/dev/rknand0p7", &s);
	if( ret == 0 ){
		system("mount /dev/rknand0p7 /mnt");
	}else{
		ret = stat("/dev/mmcblk0p7", &s);
		if( ret == 0){
			system("mount /dev/mmcblk0p7 /mnt");
		}
	}
	if( ret == 0 ){
		fd = fopen("/mnt/cksum.txt", "r");
		if(fd != NULL){
			for(i = 0; i < 10; i++){
				if(i == 5){
					*(cksum_save + index) = ' ';
					index++;
				}
				j = fread(cksum_save + index, 1, 1, fd);
				index++;
			}
			fclose(fd);
		}
	}
	DEBUG("cksum_save[%s]\n", cksum_save);
	system("umount /mnt");
}

void get_times(int pass, int fail, char action){
	FILE* fd = NULL;
	int ret = -1;
	struct stat s;

	ret = stat("/dev/rknand0p7", &s);
	if( ret == 0 ){
		system("mount /dev/rknand0p7 /mnt");
	}else{
		ret = stat("/dev/mmcblk0p7", &s);
		if( ret == 0 ){
			system("mount /dev/mmcblk0p7 /mnt");
		}
	}
	if( ret == 0 ){
		if(action == 'w') {
			fd = fopen("/mnt/times.txt", "r+");
			if(fd != NULL){
				fscanf(fd, "%ld,%ld,%ld\n", &cksum_total_times, &cksum_pass_times, &cksum_fail_times);
				fclose(fd);
			}
			fd = fopen("/mnt/times.txt", "w+");
			if(fd != NULL){
				cksum_total_times++;
				if(pass > 0) cksum_pass_times++;
				if(fail > 0) cksum_fail_times++;
				fprintf(fd, "%ld,%ld,%ld\n", cksum_total_times, cksum_pass_times, cksum_fail_times);
				fclose(fd);
			}
		} else if(action == 'r') {
			fd = fopen("/mnt/times.txt", "r+");
                        if(fd != NULL){
                                fscanf(fd, "%ld,%ld,%ld\n", &cksum_total_times, &cksum_pass_times, &cksum_fail_times);
                                fclose(fd);
                        }
		} // else if
	}
	system("umount /mnt");
}

void do_cksum(void){
	int i = 0, j = 0, index = 0, ret = 0;
	int rootfs_size = 0; // rootfs size(multiples of 512)
	char command[80] = "";
	FILE* fd = NULL;
	struct stat s;
	ret = stat("/dev/rknand0p4", &s);
	DEBUG("/dev/rknand0p4 0x%x, 0x%x, ret = %d\n", s.st_mode, S_ISBLK(s.st_mode), ret);
	if( ret == 0 ){
		system("mount /dev/rknand0p7 /mnt");
		fd = fopen("/mnt/size.txt", "r");
		if(fd != NULL) {
			fscanf(fd, "%d", &rootfs_size);	
			fclose(fd);
		} // if

		sprintf(command, "dd if=/dev/rknand0p4 of=/tmp/rootfs.img bs=512 count=%ld", rootfs_size);
		system(command);
		system("printf \"%010d\" `cksum /tmp/rootfs.img | awk '{print $1}'` > /tmp/cksum.txt");
	}else{
		ret = stat("/dev/mmcblk0p5", &s);
		DEBUG("/dev/mmcblk0p5 0x%x, 0x%x, ret = %d\n", s.st_mode, S_ISBLK(s.st_mode), ret);
		if( ret == 0 ){
			system("mount /dev/mmcblk0p7 /mnt");
			fd = fopen("/mnt/size.txt", "r");
			if(fd != NULL) {
				fscanf(fd, "%d", &rootfs_size);
				fclose(fd);
			} // if
			
			sprintf(command, "dd if=/dev/mmcblk0p4 of=/tmp/rootfs.img bs=512 count=%ld", rootfs_size);
			system(command);
			system("printf \"%010d\" `cksum /tmp/rootfs.img | awk '{print $1}'` > /tmp/cksum.txt");
		}
	}

	system("umount /mnt");
	system("rm /tmp/rootfs.img");

	get_cksum_save();
	memset(cksum_now, 0, 1024);
	if(ret == 0){
		fd = fopen("/tmp/cksum.txt", "r");
		if(fd != NULL){
			for(i = 0; i < 10; i++){
				if(i == 5){
					*(cksum_now + index) = ' ';
					index++;
				}
				j = fread(cksum_now + index, 1, 1, fd);
				index++;
			}
			fclose(fd);
		}
	}
	DEBUG("cksum_now=[%s]\n", cksum_now);
}

#include "wifi-testmode.cpp"
// ********* Pacmania QA mode *********
int Pacmania_QAKeyTest(SDL_Renderer* renderer, TTF_Font* font)
{
	printf("Pacmania_QAKeyTest Start\n");
	char msg[1024];
	int index = 0, ret = 0;
	bool quit = false;
	Uint32 start_time = 0, current_time = 0;
	SDL_Rect src_rect;
	
	// QA_Test : Live button / A / B / P1-Start
	for( index = 0; index < TEST_KEY_NUM; index++ ) {
		if ( test_key[index].key[5] == 1 ) {
			DrawQAKey(renderer, font, 1, index, 1); // (renderer, font, player, index, flush)
                        ret = CheckQAKey(renderer, font, test_key[index].key[2]);
                        DrawQAKey(renderer, font, 1, index, 0);
                        if(ret > 0){
                                sprintf(msg, "%s", "Success");
                                ShowTextBottom(renderer, font, msg, 0, 255, 0);
                                SDL_Delay(100);
                        } // if

                        else {
                                sprintf(msg, "%s", "Fail");
                                ShowTextBottom(renderer, font, msg, 255, 0, 0);
                                SDL_Delay(100); // ms unit, 100ms
                                goto qa_key_exit;
                        } // else
		} // if
	} // for

	// Check Volume Key
        for( index = 0; index <= 2; index++ ) {
                src_rect.x = vol_img[index][0]; // image location x
                src_rect.y = vol_img[index][1]; // image location y
                src_rect.w = vol_img[index][3]; // image width
                src_rect.h = vol_img[index][4]; // image height

                SDL_RenderCopy(renderer, test_qabg, NULL, NULL);
		SDL_RenderCopy(renderer, test_vol[index], NULL, &src_rect);
                SDL_RenderPresent(renderer);
                ret = CheckQAVol(renderer, font, index);
                if(ret > 0){
                        SDL_RenderCopy(renderer, test_qabg, NULL, NULL);
                        SDL_RenderCopy(renderer, test_vol[index], NULL, &src_rect);
                        sprintf(msg, "%s", "Success");
                        ShowTextBottom(renderer, font, msg, 0, 255, 0);
                        SDL_Delay(100);
                } // if

                else {
                        sprintf(msg, "%s", "Fail");
                        ShowTextBottom(renderer, font, msg, 0, 255, 0);
                        SDL_Delay(100);
                        goto qa_key_exit;
                } // else
        } // for

qa_key_exit:
	if ( ret < 0 ) {
                printf("Test Fail\n");
                Test_Fail(renderer);
                return -1;
        } else {
                Test_Success(renderer);
                return 1;
        } // else
}

// ********** Pacmania ENG MODE **********
bool g_engkeythread_quit = false;

static int Pacmania_EngKeyThread(void* ptr)
{
	SDL_Event event;
	SDL_Rect rect;

	static unsigned int old_eng_p1 = 0x0000;

	printf("Pacmania Eng KeyThread Start\n");
	while(!g_engkeythread_quit){
		if( SDL_PollEvent(&event) != 0 ) {		
			switch( event.type ) {
				case SDL_KEYUP:
					if( event.key.repeat == 0 ){
                                        	for(int i = 0; i < TEST_KEY_NUM; i++){
                                                	if(event.key.keysym.sym == test_key[i].key[2]){
                                                        	test_key[i].key[6] = 0; // status change
                                                	} //if
                                        	} // for
					} // if
					break;
				case SDL_KEYDOWN:
					if( event.key.repeat == 0){
                                        	for(int i = 0; i < TEST_KEY_NUM; i++){
                                                	if(event.key.keysym.sym == test_key[i].key[2]){
                                                        	test_key[i].key[6] = 1; // status change
                                                	} //if
                                        	} // for
					} // if
					break;
				default:
					break;
			} // switch
		} // if

		SDL_Delay(1);
	} // while
	
	printf("Pacmania Eng KeyThread Stop\n");
        return 0;
}

void Pacmania_EngKeyTest(SDL_Renderer *renderer, TTF_Font *font){

	SDL_Rect rect;

	Uint32 start_time = 0, current_time = 0;
	int eng_key_change = 0, idle_time = 30;
	int ret_key = 0;
	int quit_eng = 0;
	char msg[1024], msg1[1024];

	SDL_Thread *keythread = NULL;

	printf("Pacmania EngKey Test\n");
	keythread = SDL_CreateThread(Pacmania_EngKeyThread, "Pacmania EngKey Thread", (void*)NULL);

	start_time = SDL_GetTicks();
	while(!quit_eng){
		eng_key_change = 0;
		if( test_engbg )
			SDL_RenderCopy(renderer, test_engbg, NULL, NULL );

		// Draw volume key 
		if( g_vol == 0 ){
			rect.x = vol_img[0][0];
			rect.y = vol_img[0][1];
			rect.w = vol_img[0][3];
			rect.h = vol_img[0][4];
		
			SDL_RenderCopy(renderer, test_vol[0], NULL, &rect);	
		} else if( g_vol == 1 ){
			rect.x = vol_img[1][0];
                        rect.y = vol_img[1][1];
                        rect.w = vol_img[1][3];
                        rect.h = vol_img[1][4];

                        SDL_RenderCopy(renderer, test_vol[1], NULL, &rect);
		} else if( g_vol == 2 ){
			rect.x = vol_img[2][0];
                        rect.y = vol_img[2][1];
                        rect.w = vol_img[2][3];
                        rect.h = vol_img[2][4];

                        SDL_RenderCopy(renderer, test_vol[2], NULL, &rect);
		} // else if 

		// Draw Power button the middle of image
		if(test_key[TEST_KEY_NUM-2].key[6] != 1 && test_key[TEST_KEY_NUM-1].key[6] != 1){
			rect.x = 297;
                        rect.y = 543;
                        rect.w = vol_img[1][3];
                        rect.h = vol_img[1][4];

			SDL_RenderCopy(renderer, test_vol[1], NULL, &rect);
		} // if

		for(int i = 0; i < TEST_KEY_NUM; i++){
                        if(test_key[i].key[5] == 1 && test_key[i].key[6] == 1){
                                rect.x = test_key[i].key[0];
                                rect.y = test_key[i].key[1];
                                rect.w = test_key[i].key[3];
                                rect.h = test_key[i].key[4];

                                SDL_RenderCopy(renderer, test_key[i].tex, NULL, &rect);
				eng_key_change = 1;
                        }// if
                } // for

		SDL_RenderPresent(renderer);
		SDL_Delay(10);							

		if( eng_key_change == 1 ) start_time = SDL_GetTicks();	
		if(((SDL_GetTicks() - start_time) / 1000.0) > idle_time){
			DEBUG("Leave Eng Key Test\n");
			quit_eng = 1;
			g_engkeythread_quit = true;
		} //if
	}// while

	SDL_WaitThread(keythread, &ret_key);
	printf("End ENG Test Mode\n");
}

// ********* Mouse USB *********

int DetectMouse(){
        SDL_Event event;
        Uint32 start_time = SDL_GetTicks();
        int idle_time_10 = 10;
        ClearKey();
        while(1){
                if(SDL_PollEvent(&event) != 0){
                        switch(event.type){
                                case SDL_MOUSEBUTTONDOWN:
                                        DEBUG("mouse btn down\n");
                                        return 0;
                                        break;
                                case SDL_KEYDOWN:
                                        if(SDLK_UP == event.key.keysym.sym){
                                                DEBUG("switch UP stick\n");
                                                start_time = SDL_GetTicks();
                                        }else if(SDLK_DOWN == event.key.keysym.sym){
                                                DEBUG("switch DOWN stick\n");
                                                start_time = SDL_GetTicks();
                                        }else if(SDLK_LEFT == event.key.keysym.sym){
                                                DEBUG("switch LEFT stick\n");
                                                start_time = SDL_GetTicks();
                                        }else if(SDLK_RIGHT == event.key.keysym.sym){
                                                DEBUG("switch RIGHT stick\n");
                                                start_time = SDL_GetTicks();
                                        }
                                        break;
                                default:
                                        break;
                        }
                }
                if(((SDL_GetTicks() - start_time) / 1000.0) > idle_time_10){
                        return -10;
                }
                SDL_Delay(5);
        }
}

static int len = 0; // show tm.txt content len
static char msg1[1024]; // show tm.txt content
static char msg2[1024]; // show sd storage size

void Device_Test_Fail(SDL_Renderer* renderer, int type){
        char msg[1024];
        int y0 = 0, y1 = 0, y2 = 0;
        bool sdcard_test = false;
        TTF_Font* font48 = TTF_OpenFont(MENU_FONT, 48);
        TTF_Font* font24 = TTF_OpenFont(MENU_FONT, 24);
        FillScreen(renderer, 0, 0, 0, 0); //Black
        if(font48){
                switch(type){
                        case 0:
                                sprintf(msg, "%s", "Headset Test Fail!");
                                break;
                        case 1:
                                sprintf(msg, "%s", "tm.txt file doesn't have \"TasteMaker\" word!");
                                sdcard_test = true;
                                break;
                        case 2:
                                sprintf(msg, "%s", "USB Storage Test Fail!");
                                break;
                        case 3:
                                sprintf(msg, "%s", "Timeout!");
                                break;
			case 4: 
                                break;
			case 5:
				sprintf(msg, "%s", "tm.txt file not found!");
                                sdcard_test = true;
                                break;
                }

                if (sdcard_test) {
                        y0 = ShowText(renderer, font48, msg, 0, 0, 255, 0, 0, 0);
                        sprintf(msg, "The Length of string : %d", len);
                        y1 = ShowText(renderer, font24, msg, 0, y0, 0);
                        sprintf(msg, "The content of tm.txt : %s", msg1);
                        y2 = ShowText(renderer, font24, msg, 0, y0 + y1, 0);
                        sprintf(msg, "The SD Card size : %s", msg2);
                        ShowText(renderer, font24, msg, 0, y0 + y1 + y2, 1);
                } // if
                else 
                        ShowText(renderer, font48, msg, 0, 0, 255, 0, 0, 1);
        }else {DEBUG("open %s failed\n", MENU_FONT);}

        Mix_Chunk *wav = Mix_LoadWAV(AUDIO_TEST_NEGATIVE);
        if(wav){
                if(Mix_PlayChannel(-1, wav, 2) == -1){ //2 is mean 2+1=3 times
                        DEBUG("Mix_PlayChannel failed [%s]\n", Mix_GetError());
                }
        }
        if(wav) Mix_FreeChunk(wav);
        if(font48) TTF_CloseFont(font48);
        SDL_Delay(6000000);
}

// ********* MOUSE USB *********

int USB_Mouse_Test(SDL_Renderer* renderer, TTF_Font* font){
	int idle_time = 30;
	int ret = 0, y0 = 0, y1 = 0, draw = 1;
	char msg[1024];
	
	SDL_Event event;
	sprintf(msg, "%s", "USB Test : Press Mouse Button");
	y0 = ShowText(renderer, font, msg, 0, 0, 1);
	
	if( DetectMouse() < 0 ) {
		Device_Test_Fail(renderer, 3);
		return -10;
	} // if

	Uint32 start_time = SDL_GetTicks();
	ClearKey();
	while(1) {
		if(draw){
			draw = 0;
			sprintf(msg, "%s", "USB Test : Press Mouse Button");
			y0 = ShowText(renderer, font, msg, 0, 0, 0);
			sprintf(msg, "%s", "USB Port Test :  Pass (O)");
			y1 = ShowText(renderer, font, msg, 0, y0, 0, 255, 0, 0);
			sprintf(msg, "%s", "End of USB Port, Press A Button");
			ShowTextBottom(renderer, font, msg);
			Check_1P_A_Button(msg);
			return 0;
		} // if

		if(SDL_PollEvent(&event) != 0){
			switch(event.type){
				case SDL_KEYDOWN:
					if(SDLK_UP == event.key.keysym.sym){
						DEBUG("switch UP stick\n");
						start_time = SDL_GetTicks();
					}else if(SDLK_DOWN == event.key.keysym.sym){
						DEBUG("switch DOWN stick\n");
						start_time = SDL_GetTicks();
					}else if(SDLK_LEFT == event.key.keysym.sym){
						DEBUG("switch LEFT stick\n");
						start_time = SDL_GetTicks();
					} else if(SDLK_RIGHT == event.key.keysym.sym){
						DEBUG("switch RIGHT stick\n");
						start_time = SDL_GetTicks();
					} // else if
					break;
				default:
					break;
			}
		} // if

		if(((SDL_GetTicks() - start_time) / 1000.0) > idle_time ){//timeout
			Device_Test_Fail(renderer, 3);
			return -10;
		} // if
		SDL_Delay(5);
	} // while
}

int HeadsetDetect(SDL_Renderer* renderer, TTF_Font* font){
        DEBUG("Detect Headset.\n");
        SDL_Event event;
        int idle_time_30 = 30;
        int ret = 0, fd = 0, y0 = 0, y1 = 0, y2 = 0, y3 = 0, hp_stage = 0;
        int input_status_org = 1;
        int input_stage = 0;
        bool detect_ok = false;
        char data[128];
        char msg[1024], msg2[1024];
        memset(data, 0, sizeof(data));
        memset(msg, 0, sizeof(msg));
        memset(msg2, 0, sizeof(msg2));

        fd = open("/sys/class/gpio/gpio3/value", O_RDONLY);
        lseek(fd, 0, SEEK_SET);
        read(fd, data, 1);
        if(atoi(data) == 0){
                //hp already plugin
                return -2;
        }

        sprintf(msg, "%s", "Please Insert Headset");
        ShowText(renderer, font, msg, 0, 0, 1);
        Uint32 start_time = SDL_GetTicks();
        ClearKey();
        while(1){
                lseek(fd, 0, SEEK_SET);
                read(fd, data, 1);
                if(atoi(data) != input_status_org){
                        input_status_org = atoi(data);
                        if(atoi(data) == 0 && input_stage == 0){
                                sprintf(msg, "%s", "Headset Inserted");
                                y0 = ShowText(renderer, font, msg, 0, 0, 0, 255, 0, 0);
                                sprintf(msg, "%s", "Please Wear Headset and Press A Button");
                                ShowText(renderer, font, msg, 0, y0, 1);
                                Check_1P_A_Button(msg);

                                //sound track test
                                FillScreen(renderer, 0, 0, 0, 0); //Black
                                sprintf(msg, "%s", "[Headset] Playing 1kHz(Right+Left) Test Tune");
                                sprintf(msg2, "%s", "[Headset] Play 1kHz(Right+Left) Done, Press A Button");
                                ret = PlayTestFile(renderer, font, test_mix_music[0], msg, msg2);
                                if(ret == 1) Check_1P_A_Button(msg2);

                                sprintf(msg, "%s", "[Headset] Playing 2kHz(Right) Test Tune");
                                sprintf(msg2, "%s", "[Headset] Play 2kHz(Right) Done, Press A Button");
                                ret = PlayTestFile(renderer, font, test_mix_music[1], msg, msg2);
                                if(ret == 1) Check_1P_A_Button(msg2);

                                sprintf(msg, "%s", "[Headset] Playing 1kHz(Left) Test Tune");
                                sprintf(msg2, "%s", "[Headset] Play 2kHz(Left) Done, Press A Button");
                                ret = PlayTestFile(renderer, font, test_mix_music[2], msg, msg2);
                                if(ret == 1) Check_1P_A_Button(msg2);

				sprintf(msg, "%s", "Please Remove Headset");
				ShowText(renderer, font, msg, 0, 0, 1);
				start_time = SDL_GetTicks();//reset timer
				input_stage = 1;
			} else if(atoi(data) == 1 && input_stage == 1) {
				sprintf(msg, "%s", "Headset Removed");
				y0 = ShowText(renderer, font, msg, 0, 0, 0, 255, 0, 0);
				sprintf(msg, "%s", "Headset Test Finished, Press A Button");
				ShowText(renderer, font, msg, 0, y0, 1);
				Check_1P_A_Button(msg);
				close(fd);
				return -1;
			} // else if
                }

                if(SDL_PollEvent(&event) != 0){
                        switch(event.type){
                                case SDL_KEYDOWN:
                                        if(SDLK_UP == event.key.keysym.sym){
                                                DEBUG("switch UP stick\n");
                                                start_time = SDL_GetTicks();
                                        }else if(SDLK_DOWN == event.key.keysym.sym){
                                                DEBUG("switch DOWN stick\n");
                                                start_time = SDL_GetTicks();
                                        }else if(SDLK_LEFT == event.key.keysym.sym){
                                                DEBUG("switch LEFT stick\n");
                                                start_time = SDL_GetTicks();
                                        }else if(SDLK_RIGHT == event.key.keysym.sym){
                                                DEBUG("switch RIGHT stick\n");
                                                start_time = SDL_GetTicks();
                                        }
                                default:
                                        break;
                        }
                }

                if(((SDL_GetTicks() - start_time) / 1000.0) > idle_time_30){
                        close(fd);
                        return -20;
                }
                SDL_Delay(5);
        }
}		
 
void StartTestMode(SDL_Renderer* renderer, TTF_Font* font){
	int i = 0, ret = 0, y1 = 0, y2 = 0, y3 = 0, y4 = 0, y5 = 0, y6 = 0, y7 = 0, y8 = 0, y9 = 0, y10 = 0, ret_musicthread = 0, ab_key = 0;
	char msg[1024], filename[1024], msg2[1024];
	char model_ver[1024], moo_ver[1024], pcb_ver[1024];
	FILE* fp = NULL, *fp_times = NULL;
	TTF_Font* font20 = TTF_OpenFont(MENU_FONT, 20);
	bool cksum_flag = false;
	SDL_Color green = {0, 255, 0}, red = {255, 0, 0};

	SDL_Thread* musicthread;
	DEBUG("StartTestMode\n");

	TestModeInit(renderer, font);
	musicthread = SDL_CreateThread(TestMusicThread, "TestMusicThread", (void*)renderer);

	memset(model_ver, '\0', 1024);
	memset(moo_ver, '\0', 1024);
	memset(pcb_ver, '\0', 1024);
	
	// Project Number
	fp = fopen(MODEL, "r");
	if(fp != NULL){
		fgets(model_ver, 1024, fp);
		DEBUG("model [%s]len=%d\n", model_ver, strlen(model_ver));
		if(strlen(model_ver) > 1)
			model_ver[strlen(model_ver) - 1] = '\0';
		fclose(fp);
	}else printf("can not find model[%s]\n", MODEL);

	// Date Code
	fp = fopen(MOO, "r");
	if(fp != NULL){
		fgets(moo_ver, 1024, fp);
		DEBUG("moo [%s]len=%d\n", moo_ver, strlen(moo_ver));
		if(strlen(moo_ver) > 1) 
			moo_ver[strlen(moo_ver) - 1] = '\0';
		fclose(fp);
	}else printf("can not find MOO[%s]\n", MOO);

	// PCBA Version
	fp = fopen(PCB, "r");
	if(fp != NULL){
		fgets(pcb_ver, 1024, fp);
		DEBUG("pcb [%s]len=%d\n", pcb_ver, strlen(pcb_ver));
		if(strlen(pcb_ver) > 1)
			pcb_ver[strlen(pcb_ver) - 1] = '\0';
		fclose(fp);
	}else printf("can not find PCB[%s]\n", PCB);

	/*
        // DDR Size
        string str = "";
        vector<string> buffer;
        system("free | grep \"Mem\" | awk '{print $2}' > /tmp/memdump.log");
        ifstream fp1;
        fp1.open("/tmp/memdump.log", ifstream::in);
        if(!fp1) printf("can not find dmesg.log\n");

        while(fp1 >> str) buffer.push_back(str);
	*/
Test1:	//Record code info: Show Version
	sprintf(msg, "%s", model_ver);
	y1 = ShowText(renderer, font, msg, 0, 0, 0);
	sprintf(msg, "%s", moo_ver);
	y2 = ShowText(renderer, font, msg, 0, y1, 0);
	sprintf(msg, "%s %04d-%02d-%02d", SYSTEM, YEAR, MONTH, DAY);
	y3 = ShowText(renderer, font, msg, 0, y1 + y2, 0);
	sprintf(msg, "%s", pcb_ver);
	y4 = ShowText(renderer, font, msg, 0, y1 + y2 + y3, 0);
	do_cksum();
	sprintf(msg, "CheckSum:%s(%c)", cksum_now, (!strcmp(cksum_now, cksum_save) ? 'O':'X'));
	if(!strcmp(cksum_now, cksum_save)){//checksum check pass
		y5 = ShowText(renderer, font, msg, 0, y1 + y2 + y3 + y4, 0);
		get_times(0, 0, 'r');
		cksum_flag = true;
		sprintf(msg, "Total:%ld, Passed:%ld, Failed:%ld", cksum_total_times, cksum_pass_times, cksum_fail_times);
		y6 = ShowText(renderer, font, msg, 0, y1 + y2 + y3 + y4 + y5, 1);
		/*
		// show ddr size
		strcpy(ddr_size, buffer[3].c_str());
		sprintf(msg, "DDR Size: %s(%c)", ddr_size, (atoi(ddr_size) == 256) ? 'O' : 'X');
		y7 = ShowText(renderer, font, msg, 0, y1 + y2 + y3 + y4 + y5 + y6, (atoi(ddr_size) == 256 ? green : red), 0);
		*/
	}else{//cksum compare fail
		TTF_Font* font50 = TTF_OpenFont(MENU_FONT, 250);
		y5 = ShowText(renderer, font, msg, 0, y1 + y2 + y3 + y4, 0);
		get_times(0, 1, 'w');
		sprintf(msg, "Total:%ld, Passed:%ld, Failed:%ld", cksum_total_times, cksum_pass_times, cksum_fail_times);
		y6 = ShowText(renderer, font, msg, 0, y1 + y2 + y3 + y4 + y5, 0);
		/*
		// show ddr size
                sprintf(msg, "DDR Size: %s(%c)", ddr_size, (atoi(ddr_size) == 256) ? 'O' : 'X');
                y7 = ShowText(renderer, font, msg, 0, y1 + y2 + y3 + y4 + y5 + y6, (atoi(ddr_size) == 256 ? green : red), 0);
		*/
		DEBUG("CheckSum failed sleep 60 seconds\n");
		sprintf(msg, "%s", "X");
		ShowTextBottom(renderer, font50, msg, 255, 0, 0, 1);
#if defined(SKIP_CHECKSUM)
		TTF_CloseFont(font50);
		printf("checksum error, skip checksum\n");
#else
		sleep(60);
		TTF_CloseFont(font50);
		goto DONE;
#endif
	}

	printf("checking 1p ab button\n");
	sprintf(msg, "%s", "Record code info");
	ab_key = Check_1P_AB_Button(msg);
	printf("check 1p ab button done\n");

	printf("** test_mode **: %d\n", test_mode);
	if(test_mode == 6)
		goto MicrophoneTest;

Test2:  //LCD screen test
	FillScreen(renderer, 255, 0, 0, 1); //Red
	sprintf(msg, "%s", "Red LCD Screen");
	Check_1P_A_Button(msg);
	FillScreen(renderer, 0, 0, 255, 1); //Blue
	sprintf(msg, "%s", "Blue LCD Screen");
	Check_1P_A_Button(msg);
	FillScreen(renderer, 0, 255, 0, 1); //Green
	sprintf(msg, "%s", "Green LCD Screen");
	Check_1P_A_Button(msg);
	FillScreen(renderer, 0, 0, 0, 1); //Black
	sprintf(msg, "%s", "Black LCD Screen");
	Check_1P_A_Button(msg);
	FillScreen(renderer, 255, 255, 255, 1);//white
	sprintf(msg, "%s", "White LCD Screen");
	Check_1P_A_Button(msg);
Test3:  //Sound Test
	FillScreen(renderer, 0, 0, 0, 0); //Black
	sprintf(msg, "%s", "Playing 1kHz(Right+Left) Test Tune");
	sprintf(msg2, "%s", "Play 1kHz(Right+Left) Done, Press A Button");
	ret = PlayTestFile(renderer, font, test_mix_music[0], msg, msg2);
	if(ret == 1) Check_1P_A_Button(msg2);

	sprintf(msg, "%s", "Playing 2kHz(Right) Test Tune");
	sprintf(msg2, "%s", "Play 2kHz(Right) Done, Press A Button");
	ret = PlayTestFile(renderer, font, test_mix_music[1], msg, msg2);
	if(ret == 1) Check_1P_A_Button(msg2);

	sprintf(msg, "%s", "Playing 1kHz(Left) Test Tune");
	sprintf(msg2, "%s", "Play 2kHz(Left) Done, Press A Button");
	ret = PlayTestFile(renderer, font, test_mix_music[2], msg, msg2);

	if(ret == 1) Check_1P_A_Button(msg2);
	
	ret = USB_Mouse_Test(renderer, font);
	if( ret < 0 ) goto DONE;

MicrophoneTest:
	// In Classof81 Game, Factory mode(custom) doesn't need HeadsetTest. But in headset mode(for BTE), keep this mode.
	if ( test_mode != 1 ) {
		ret = HeadsetDetect(renderer, font);
		if(ret < -1){
			Device_Test_Fail(renderer, 0);
			goto DONE;
		} // if
	} // if

	if(cksum_flag) get_times(1, 0, 'w');

	sprintf(msg, "%s", "A Button to QA/QC Mode, Hold A Button for 2 sec to ENG Mode, B Button to Wi-Fi Test Mode");
	ShowTextBottom(renderer, font20, msg);

	ret = Check_1P_A_Button_2sec();
	printf("Check 1P A Button 2 sec or B button(ret = %d)\n", ret);
Test4: // key test
        if(ret == 0){
		EnablePowerButton(1);
		WiFiTestMode(renderer, font);
        } else if(ret == 1){//QA/QC Key Test
		DEBUG("QA Mode\n");
		ret = Pacmania_QAKeyTest(renderer, font);
		if(ret > 0) WiFiTestMode(renderer, font);
        }else if(ret == 2){//Eng Key Test
                DEBUG("Eng Mode\n");
		Pacmania_EngKeyTest(renderer, font);
	} // else if
DONE:
	for(i = 0; i <= 2; i++){
		if(test_mix_music[i])
			Mix_FreeMusic(test_mix_music[i]);
	}
	if(test_qabg)  SDL_DestroyTexture(test_qabg);
	if(test_engbg) SDL_DestroyTexture(test_engbg);
	for(i = 0; i < TEST_KEY_NUM; i++){
		if(test_key[i].tex)
			SDL_DestroyTexture(test_key[i].tex);
	}
	if(test_vol[0]) SDL_DestroyTexture(test_vol[0]);
	if(test_vol[1]) SDL_DestroyTexture(test_vol[1]);
	if(test_vol[2]) SDL_DestroyTexture(test_vol[2]);
	SDL_WaitThread(musicthread, &ret_musicthread);
}

int StartResetMode(SDL_Renderer* renderer, TTF_Font* font){
	int item = 0;
	int resetmode_exit = 0;
	SDL_Event event;
	SDL_Texture* tex_reset_no = IMG_LoadTexture(renderer, "/pic/reset_no.jpg");
	SDL_Texture* tex_reset_yes  = IMG_LoadTexture(renderer, "/pic/reset_yes.jpg");
	SDL_RenderCopyEx(renderer, tex_reset_no, NULL, NULL, ANGLE, NULL, SDL_FLIP_NONE);
	SDL_RenderPresent(renderer);
	while(resetmode_exit == 0){
		if( SDL_PollEvent(&event) != 0 ){
			switch( event.type ){
				case SDL_KEYUP:
					if(SDLK_RIGHT == event.key.keysym.sym){
						item = 0;
						SDL_RenderCopyEx(renderer, tex_reset_no, NULL, NULL, ANGLE, NULL, SDL_FLIP_NONE);
						SDL_RenderPresent(renderer);
					}else if(SDLK_LEFT == event.key.keysym.sym){
						item = 1;
						SDL_RenderCopyEx(renderer, tex_reset_yes, NULL, NULL, ANGLE, NULL, SDL_FLIP_NONE);
						SDL_RenderPresent(renderer);
					}else if(SDLK_x == event.key.keysym.sym){
						resetmode_exit = 1;
						FillScreen(renderer, 0, 0, 0, 1); //Black
					}
					break;
			}
		}
		SDL_Delay(10);
	}
RESET_DONE:
	SDL_DestroyTexture(tex_reset_no);
	SDL_DestroyTexture(tex_reset_yes);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	return item;
}
