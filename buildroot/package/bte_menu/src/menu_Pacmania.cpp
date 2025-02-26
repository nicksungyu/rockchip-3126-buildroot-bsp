#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <GLES/gl.h>
#include <SDL2/SDL_opengles.h>
#include <stdlib.h> //rand()
#include <unistd.h> //usleep
#include <pthread.h>
#include <asm/param.h>
#include <linux/joystick.h>
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define _DEBUG 1
#ifdef _DEBUG
#define DEBUG(format, args...) printf(format, ##args)
#else
#define DEBUG(args...)
#endif

extern "C"{
void IMG_SetJPG_Scale(int scale);
}


#define MAX_AXES 16
#define ANGLE 0
#define MENU_FONT "/usr/share/menu/font/DroidSerif-Bold.ttf"
#define PIC_LOADING_BMP "/usr/share/menu/images/bootlogo.bmp"
#define AUDIO_INTRO       "/usr/share/menu/sound/intro.wav"
#define VOLUME_FILE "/game/docs/volume"
#define LEGAL_PAGE "/usr/share/menu/images/legal.jpg"
#define DISNEY_LEGAL_PAGE "/usr/share/menu/images/disney_legal.png"
#define Animation_Total_Frame 176

int g_volume_level = 0; //fixed in 0 ~ 15
int WIDTH = 1280;
int HEIGHT = 960;
int g_volume_mode = 1; //0:3-way 1:OSD
int g_volume_status = -1; //S0: Middle, S1: Left(Decrease), S2: Right(Increase)
int g_musicthread_stop = 0;
SDL_mutex* g_key_mutex = NULL;
volatile unsigned int g_p1_key = 0x0000;
volatile unsigned int g_p2_key = 0x0000;
volatile unsigned int g_iSelItem = 1; //1:MK1, 2:MK2, 3:MK3
volatile unsigned int g_iSelSetting = 0; //0:main page, 1:setting page
volatile unsigned int g_p1_repeat = 0, g_p2_repeat = 0; //for hold 5 second function
volatile int g_really_quit = 0;
volatile int g_quit = 1;
volatile int g_play = 0;
volatile int g_audio_restart = 0;
volatile unsigned int g_iMenu = 0;//0:Main Page, 1:Manual Page
volatile unsigned int g_vol = 0xffffff;
int g_volume_max = 15;
int g_vol_update = 1;
int g_level15[] = { 0, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 31};
char key_status[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int test_mode = 0;

SDL_Texture* volume[16];
SDL_Texture* copyright[1];
SDL_Surface *screenSurface = NULL;

char volume_osd_pic[16][1024] = {
	"/usr/share/menu/images/volume/0.png",
	"/usr/share/menu/images/volume/1.png",
	"/usr/share/menu/images/volume/2.png",
	"/usr/share/menu/images/volume/3.png",
	"/usr/share/menu/images/volume/4.png",
	"/usr/share/menu/images/volume/5.png",
	"/usr/share/menu/images/volume/6.png",
	"/usr/share/menu/images/volume/7.png",
	"/usr/share/menu/images/volume/8.png",
	"/usr/share/menu/images/volume/9.png",
	"/usr/share/menu/images/volume/10.png",
	"/usr/share/menu/images/volume/11.png",
	"/usr/share/menu/images/volume/12.png",
	"/usr/share/menu/images/volume/13.png",
	"/usr/share/menu/images/volume/14.png",
	"/usr/share/menu/images/volume/15.png",
};

void UpdateVolume(int level, int level_max){
	FILE* vol_fp = NULL;
	int mounted_flag = 0, nand_flag = 0, emmc_flag = 0;

	vol_fp = fopen(VOLUME_FILE, "w+");
	if(vol_fp != NULL){
		fprintf(vol_fp, "%d\n%d", level, level_max);
		fflush(vol_fp);
		fclose(vol_fp);
		system("sync;");
	}else{//check data partition, emmc:/dev/mmcblk0p8, nand:/dev/nande
		struct stat nand_s, emmc_s, mount_status;
		printf("%s can not be created\n", VOLUME_FILE);
		if(stat("/game/docs/", &mount_status) == -1){
                        mounted_flag = -1;
                        printf("failed to stat /game/docs mount point\n");
                }else{
                        mounted_flag = 1;
                        printf("have /game/docs mount point\n");
                        //system("umount /game/docs");
                }
                /*
		if(stat("/opt/SegaDriving/docs/", &mount_status) == -1){
			mounted_flag = -1;
			printf("failed to stat /opt/SegaDriving/docs mount point\n");
		}else{
			mounted_flag = 1;
			printf("have /opt/SegaDriving/docs mount point\n");
			system("umount /opt/SegaDriving/docs");
		}
		*/
		stat("/dev/nande", &nand_s);
		if((nand_s.st_mode & S_IFMT) == S_IFBLK){
			printf("have /dev/nande\n");
			nand_flag = 1;
		}else{
			stat("/dev/mmcblk0p8", &emmc_s);
			if((emmc_s.st_mode & S_IFMT) == S_IFBLK){
				printf("have /dev/mmcblk0p8\n");
				emmc_flag = 1;
			}
		}
		if(nand_flag == 1){
			//printf("formate start\n");
			//system("/sbin/mkfs.ext2 /dev/nande -m 0");
			//printf("formate done, mount again\n");
			//system("mount -o rw,sync,noatime,nodiratime,norelatime,noauto_da_alloc,barrier=0,data=ordered -t ext4 /dev/nande /opt/SegaDriving/docs");
		}else if(emmc_flag == 1){
			//printf("formate start\n");
			//system("/sbin/mkfs.ext2 /dev/mmcblk0p8 -m 0");
			//printf("formate done, mount again\n");
			//system("mount -o rw,sync,noatime,nodiratime,norelatime,noauto_da_alloc,barrier=0,data=ordered -t ext4 /dev/mmcblk0p8 /opt/SegaDriving/docs");
		}
		vol_fp = fopen(VOLUME_FILE, "w+");//try again
		if(vol_fp != NULL){
			fprintf(vol_fp, "%d\n%d", level, level_max);
			fflush(vol_fp);
			fclose(vol_fp);
			system("sync;");
		}
	}
}


void SetALSAMasterVolume(long volume){
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, "default");
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, "Master");
	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

	if (elem) {
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
		snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);
	}

	snd_mixer_close(handle);
}

void SetTinymixVolume(int volume){//headphone volume
	if(test_mode) return;
	DEBUG("SetTinyMixVolume: %d\n", volume);
	SetALSAMasterVolume((volume*100)/31);
}

void SetTinymixVolumeTestMode(int volume){//headphone volume
        DEBUG("SetTinyMixVolume: %d\n", volume);
        SetALSAMasterVolume((volume*100)/31);
}

#include <linux/fb.h>
struct fb_var_screeninfo vinfo;
void GetFramebufferRes(void){
	int fbfd = open("/dev/fb0", O_RDWR);
	if(fbfd == -1){
		DEBUG("Error: cannot open framebuffer device\n");
		return;
	}
	if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1){
		DEBUG("Error: reading fixed information\n");
		close(fbfd);
		return;
	}
	DEBUG("Framebuffer:%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
	WIDTH = vinfo.xres;
	HEIGHT = vinfo.yres;
	close(fbfd);
}

SDL_Joystick *joystick = NULL;
void VolLoading(void* render){
	SDL_Renderer* renderer = (SDL_Renderer*)render;
	for(int i = 0; i <=15; i++){
		volume[i] = IMG_LoadTexture(renderer, volume_osd_pic[i]);
	}
}

struct preloading{
	SDL_Renderer* renderer;
	SDL_Surface** animation;
	int loading_index;
	int animation_index;
};
static int PreLoading(void* arg){
	struct preloading* pl = (struct preloading*)arg;
	Uint32 PicStart = 0;
	char anim_sz[1024];
	int i;
	for( i = 1; i <= Animation_Total_Frame; i++){
		pl->animation[i] = NULL;
	}
	PicStart = SDL_GetTicks();
	IMG_SetJPG_Scale(2);
	for( i = 1; i <= Animation_Total_Frame; i++){
		if( i > (pl->animation_index + 30)) { SDL_Delay(10);}

		sprintf(anim_sz, "/usr/share/menu/images/intro-%03d.jpg", i);
		pl->animation[i] = IMG_Load(anim_sz);
		if(!pl->animation[i]){
			DEBUG("IMG_Load: %s\n", IMG_GetError());
		}
		pl->loading_index = i;
	}
	IMG_SetJPG_Scale(1);
	// DEBUG("PreLoading TotalTime=%ld, AvgTime=%lf\n", SDL_GetTicks() - PicStart, (SDL_GetTicks() - PicStart) / Animation_Total_Frame);
	return 0;
}

#include "testmode.cpp"
void DrawVol(SDL_Renderer* renderer){
	int angle = 0;
	SDL_Rect volume_size;
	SDL_QueryTexture(volume[0], NULL, NULL, &volume_size.w, &volume_size.h);
	SDL_Rect target_pos;
#if defined(PORTRAIT)
	target_pos.x = (WIDTH - volume_size.w) - 70;
	target_pos.y = (HEIGHT - volume_size.h) / 2;
#else//defined(LANDSCAP)
	target_pos.x = (WIDTH - volume_size.w) / 2;
	target_pos.y = (HEIGHT - volume_size.h) - 70;
#endif
	target_pos.w = volume_size.w;
	target_pos.h = volume_size.h;
	SDL_RenderCopyEx(renderer, volume[g_volume_level], NULL, &target_pos, angle, NULL, SDL_FLIP_NONE);
}

static int MusicThread(void* render){
	pid_t pid;
	int ret = 0, status = 0;
	Mix_Music *intro = Mix_LoadMUS(AUDIO_INTRO);
	DEBUG("play wav sound start\n");
	g_musicthread_stop = 0;
	if(intro == NULL){
		DEBUG("Mix_LoadMUS /usr/share/menu/sound/intro.wav failed[%s]\n", Mix_GetError());
	}else{
		if(Mix_PlayMusic(intro, 1) == -1){
			DEBUG("Mix_PlayMusic failed[%s]\n", Mix_GetError());
		}else{
			while( Mix_PlayingMusic() == 1 ){
				SDL_Delay(10);
				if(g_musicthread_stop == 1)
					break;
			}
		}
		Mix_FreeMusic(intro);	
	}
	DEBUG("play sound end\n");
	return 0;
}

static int CheckVolumeSwitchThread(void* ptr){
	int fd = 0, ret = 0, level = -1, level_max = -1, test_mode = *(int*)ptr;
	char msg[32];
	FILE* vol_fp = NULL;
	static int changed = 0;
	Uint32 start_time = SDL_GetTicks();
	memset(msg, 0, sizeof(msg));
	int default_level = 7, default_level_max = 15;
	
	DEBUG("check %s file\n", VOLUME_FILE);
	if(access(VOLUME_FILE, F_OK) != -1 ){
		DEBUG("%s file exist\n", VOLUME_FILE);
		vol_fp = fopen(VOLUME_FILE, "r+");
		if(vol_fp){
			ret = fscanf(vol_fp, "%d\n%d", &level, &level_max);
			if(ret == 1){// have file but only have level data, without level_max
				level_max = default_level_max;
				DEBUG("read %s[%d] have leve data[%d]\n", VOLUME_FILE, ret, level);
			}else if(ret == 0){ // have file but no data
				level = default_level; level_max = default_level_max;
				DEBUG("read %s[%d] no data\n", VOLUME_FILE, ret);
			}else if(ret == 2){
				DEBUG("Got correct level=%d, level_max=%d\n", level, level_max);
			}else{//read VOLUME_FILE fail
				level = default_level; level_max = default_level_max;
				DEBUG("read %s[%d] fail\n", VOLUME_FILE, ret);
			}
			fclose(vol_fp);
		}else{
			level = default_level; level_max = default_level_max;
			DEBUG("read %s fail\n", VOLUME_FILE);
		}
	}else{//File not exist, create new file
		DEBUG("%s file not exist\n", VOLUME_FILE);
		vol_fp = fopen(VOLUME_FILE, "w+");
		level = default_level; level_max = default_level_max;
		if(vol_fp){
			DEBUG("create new %s\n", VOLUME_FILE);
			fprintf(vol_fp, "%d\n%d", level, level_max);
			fclose(vol_fp);
		}else{
			DEBUG("create %s fail\n", VOLUME_FILE);
		}
	}
	//Convert level 100 to level 50
	DEBUG("before calculate volume_level[%d], level=%d, level_max=%d\n", g_volume_level, level, level_max);
	g_volume_level = (((float)level / (float)level_max) * 15.0);
	DEBUG("Initialize Audio Volume: tinymix:%d[%d], level:%d, level_max:%d\n", g_level15[g_volume_level], g_volume_level, level, level_max);
	SetTinymixVolume(g_level15[g_volume_level]);
	
	fd = open("/proc/a40i_vol_proc", O_NONBLOCK, O_RDONLY);
	while(!g_quit){
		int tmp;
		lseek(fd, 0, SEEK_SET);
		read(fd, msg, 1);
		tmp = atoi(msg);
		if(tmp != g_vol){
			start_time = SDL_GetTicks();
			if(tmp == 0){
				g_volume_status = 0;
				g_volume_level--;
				if(g_volume_level <= 0) g_volume_level = 0;
				SetTinymixVolume(g_level15[g_volume_level]);
				UpdateVolume(g_volume_level, g_volume_max);
			}else if(tmp == 2){
				g_volume_status = 2;
				g_volume_level++;
				if(g_volume_level >= 15) g_volume_level = 15;
				SetTinymixVolume(g_level15[g_volume_level]);
				UpdateVolume(g_volume_level, g_volume_max);
			}
		} 
		
		if((SDL_GetTicks() - start_time) > 200 && (tmp == 0 || tmp == 2)){//check every 0.2 sec
			if(tmp == 0){//Decrease
				start_time = SDL_GetTicks();
				g_volume_level--;
				if(g_volume_level <= 0) g_volume_level = 0;
				g_volume_status = 0;
				DEBUG("volume=%d\n", g_volume_level);
				//SetALSAMasterVolume(g_level15[g_volume_level]);
				SetTinymixVolume(g_level15[g_volume_level]);
				UpdateVolume(g_volume_level, g_volume_max);
				changed = 1;
			}else if(tmp == 2){//Increase
				start_time = SDL_GetTicks();
				g_volume_level++;
				if(g_volume_level >= 15) g_volume_level = 15;
				g_volume_status = 2;
				DEBUG("volume=%d\n", g_volume_level);
					//SetALSAMasterVolume(g_level15[g_volume_level]);
				SetTinymixVolume(g_level15[g_volume_level]);
				UpdateVolume(g_volume_level, g_volume_max);
				changed = 1;
			}
		}else if(changed == 1){
			if((SDL_GetTicks() - start_time) > 3000){//OSD Status:Close
				g_volume_status = -1;
				changed = 0;
			}else{//OSD Status:Show
				g_volume_status = 1;
			}
		}

		if(tmp != g_vol)
			g_vol = tmp;
		
		SDL_Delay(5);
		if(g_audio_restart == 1) break;
	}
	g_volume_status = -1;
	changed = 0;
	close(fd);
	DEBUG("CheckVolumeSwitchThread Exit, changed=%d\n",changed);
	return 0;
}

void Animation(SDL_Renderer* renderer, TTF_Font* font, SDL_Surface** animation){
	int i;
	Uint32 start_time = 0, current_time = 0, test_time =0;
	pid_t pid = 0;
	SDL_Surface *pic;
	char anim_sz[1024];
	SDL_Thread *loadingthread, *musicthread;
	
	int ret_loading = 0, ret_musicthread = 0;
	{
		DEBUG("Animation Preloading Start\n");
		struct preloading pl;
		pl.renderer = renderer; pl.animation = animation; pl.loading_index = 0; pl.animation_index = 0;
		loadingthread = SDL_CreateThread(PreLoading, "PreLoadingThread", (void*)&pl);
		usleep(2000000); // sleep 2s
		DEBUG("Animation Start\n");
		start_time = SDL_GetTicks();
		musicthread = SDL_CreateThread(MusicThread, "MusicThread", (void*)renderer);
		IMG_SetJPG_Scale(2); // reduce the resolution
		for(i = 1; i<= Animation_Total_Frame; i++){
			if(animation[i] && pl.loading_index >= i){
				test_time = SDL_GetTicks();
				SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, animation[i]);
				SDL_RenderCopyEx(renderer, texture, NULL, NULL, ANGLE, NULL, SDL_FLIP_NONE);
				if(g_volume_mode == 1 && g_volume_status != -1){
					DrawVol(renderer);
				}
				SDL_RenderPresent(renderer);
				SDL_Delay(12); // ms 
				SDL_DestroyTexture(texture);
				SDL_FreeSurface(animation[i]);
				pl.animation_index = i;
				// DEBUG("animation index=%d, loading index=%d, diff=%d Time=[%lf]sec\n", pl.animation_index, pl.loading_index, pl.loading_index - pl.animation_index, (SDL_GetTicks() - test_time)/1000.0);
			}else{
				// DEBUG("Catch up and skip it[%d]!!\n", i);
				SDL_Delay(60);//average loading time is 51ms
			}
		}
		DEBUG("Animation Showing Time=[%lf]sec\n", (SDL_GetTicks() - start_time) / 1000.0);
	}
	IMG_SetJPG_Scale(1);
	DEBUG("sdl wait loading thread\n");
	SDL_WaitThread(loadingthread, &ret_loading);
	DEBUG("sdl wait music thread\n");
	g_musicthread_stop = 1;
	SDL_WaitThread(musicthread, &ret_musicthread);
	DEBUG("sdl wait loading/thread done\n");
}

int TestMode(void){//Test Mode enable by P1/P2 start, A button and middle volume switch.
	char data[128];
	FILE* fp = fopen("/proc/a40i_testmode", "r");
	memset(data, 0, sizeof(data));
	if(fp){
		fread(data, 1, 1, fp);
		fclose(fp);
		DEBUG("Test Mode=%d\n", atoi(data));
		return atoi(data);
	}
	return 0;
}

void VolumeInit(){
	char data[10];
	int volume = 0;
	FILE *fp = fopen(VOLUME_FILE, "r");
	if(fp){
		fread(data, strlen(data)+1, 1, fp);
		fclose(fp);
		volume = atoi(data);
		DEBUG("Volume=%d\n", volume);
		SetTinymixVolume(g_level15[volume]);
	}
}

void EnablePowerButton(int enable){
        int _fd = -1;
        _fd = open("/dev/ioctl_dev", O_RDWR);
        if(_fd >= 0){
		printf("%s: fd=%d\n", __func__, _fd);
                ioctl(_fd, 0, &enable);
                close(_fd);
        }
}

void EnablePowerKeyEvent(int enable){
	int _fd = -1;
	_fd = open("/dev/ioctl_dev", O_RDWR);
	if(_fd >= 0){
		ioctl(_fd, 1, &enable);
		close(_fd);
	} // if
}

void ShowLegalPage(SDL_Renderer *renderer, const char *legal_file_name){
	SDL_Texture* loading = IMG_LoadTexture(renderer, legal_file_name);
        SDL_Rect loading_size;
        SDL_QueryTexture(loading, NULL, NULL, &loading_size.w, &loading_size.h);
        SDL_Rect target_pos;
        target_pos.x = (WIDTH - loading_size.w) / 2;
        target_pos.y = (HEIGHT - loading_size.h) / 2;
        target_pos.w = loading_size.w;
        target_pos.h = loading_size.h;
        DEBUG("loading picture(w=%d,h=%d), x=%d,y=%d,w=%d,h=%d\n", loading_size.w, loading_size.h, target_pos.x, target_pos.y, target_pos.w, target_pos.h);
        SDL_RenderCopyEx(renderer, loading, NULL, &target_pos, 270, NULL, SDL_FLIP_NONE);
        SDL_RenderPresent(renderer);
	SDL_DestroyTexture(loading);
}

int GetVolumeData(){
        int fd =0, level = -1, level_max = -1, ret = 0;
        char msg[32];
        FILE* vol_fp = NULL;
        int default_level = 7, default_level_max = 15;

        if(access(VOLUME_FILE, F_OK) != -1 ){
                DEBUG("%s file exist\n", VOLUME_FILE);
                vol_fp = fopen(VOLUME_FILE, "r+");
                if(vol_fp){
                        ret = fscanf(vol_fp, "%d\n%d", &level, &level_max);
                        if(ret == 1){// have file but only have level data, without level_max
                                level_max = default_level_max;
                                DEBUG("read %s[%d] have leve data[%d]\n", VOLUME_FILE, ret, level);
                        }else if(ret == 0){ // have file but no data
                                level = default_level; level_max = default_level_max;
                                DEBUG("read %s[%d] no data\n", VOLUME_FILE, ret);
                        }else if(ret == 2){
                                DEBUG("Got correct level=%d, level_max=%d\n", level, level_max);
                        }else{//read VOLUME_FILE fail
                                level = default_level; level_max = default_level_max;
                                DEBUG("read %s[%d] fail\n", VOLUME_FILE, ret);
                        }
                        fclose(vol_fp);
                } else {//File not exist, create new file
                        DEBUG("%s file not exist\n", VOLUME_FILE);
                        vol_fp = fopen(VOLUME_FILE, "w+");
                        level = default_level; level_max = default_level_max;
                        if(vol_fp){
                                DEBUG("create new %s\n", VOLUME_FILE);
                                fprintf(vol_fp, "%d\n%d", level, level_max);
                                fclose(vol_fp);
                        }else{
                                DEBUG("create %s fail\n", VOLUME_FILE);
                        }
                }
        }

        return level;
}

int main(int argc, char *argv[]) {
	SDL_Window *window;
	SDL_Renderer* renderer;
	
	SDL_Thread *keythread,  *clicksoundthread, *checkvolthread, *checktrackballthread;
	int ret_key, ret_clicksound, ret_checkvol, ret_checktrackball, i;
	Uint32 P1Start = 0, P2Start = 0, PicStart = 0;
	int m_volume_level = 0;
	static unsigned int old_p1 = 0xffffff, old_p2 = 0xffffff, old_vol = 0xffffff, old_trackball = 0xffffff;
	
	long long sdl_quit_time = 0;
	// SDL_Texture *cursor, *starwars_btn[4];
	SDL_Texture* main_page[15], *sec[15], *setting_page[15];
	
	SDL_Surface* animation[Animation_Total_Frame];
	
	int bFirst = 1, auto_launch = 0;
	printf("BTE Launcher Program[%s %s]\n", __DATE__, __TIME__);
	// Setting OpenGL attribute before creating window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);

	GetFramebufferRes();
	
	keythread = clicksoundthread = checkvolthread = checktrackballthread = NULL;
	window = NULL;
	renderer = NULL;
	ret_key = ret_clicksound = ret_checkvol = ret_checktrackball = P1Start = P2Start = 0;
	g_key_mutex = SDL_CreateMutex();
	for(int i = 0; i < 15; i++){
		main_page[i] = NULL;
		sec[i] = NULL;
		setting_page[i] = NULL;
	}
	old_p1 = old_p2 = old_vol = old_trackball = 0xffffff;
	g_really_quit = g_quit = g_iMenu = g_play = auto_launch = 0;
	g_vol = 0xffffff;
	
	// SDL_Init
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0) {
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return 1;
	}else{
		DEBUG("SDL Init Done\n");
		DEBUG("%d joysticks were found.\n", SDL_NumJoysticks());
		for(int i = 0; i < SDL_NumJoysticks(); i++ ){
			DEBUG("Joysticks Name[%s]\n", SDL_JoystickNameForIndex(i));
		}
		SDL_JoystickEventState(SDL_ENABLE);
		joystick = SDL_JoystickOpen(0);
		DEBUG("Joystick[0] has %d buttons, %d axes\n", SDL_JoystickNumButtons(joystick), SDL_JoystickNumAxes(joystick));
		SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
	}

	int flags = IMG_INIT_JPG | IMG_INIT_PNG;
	int initted = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	if((initted&flags) != flags) {
		SDL_Log("IMG_Init: Failed to init required jpg and png support!\n");
		SDL_Log("IMG_Init: %s\n", IMG_GetError());
	}

	if(TTF_Init() < 0){ // Initialize the truetype font API
		SDL_Log("TTF_Init: %s\n", TTF_GetError());
	}
	TTF_Font* font = TTF_OpenFont(MENU_FONT, 28);

	flags = MIX_INIT_MP3;
	int result = 0;
	if(flags != (result = Mix_Init(flags))){
		DEBUG("Could not initialize mixer(result:%d). %s\n", result, Mix_GetError());
	}

	int nMix_Result = Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 640);
	DEBUG("Mix_OpenAudio result: %d\n", nMix_Result);
	
	// SDL Create Window
	window = SDL_CreateWindow("title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
	
	if(window == NULL){
		DEBUG("Could not create SDL window: %s", SDL_GetError());
		return -1;
	} else {
		DEBUG("force use OpenGLES2\n");
		SDL_GLContext context = SDL_GL_CreateContext(window);
		SDL_GL_MakeCurrent(window, context);
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);	
		
		if(renderer == NULL){
			DEBUG("could not get window: %s\n", SDL_GetError());
		}
	}

	//VolumeInit();
	VolLoading(renderer);
	
	test_mode = TestMode();
	checkvolthread = SDL_CreateThread(CheckVolumeSwitchThread, "CheckVolThread", (void*)&test_mode);

	if(test_mode == 1 || test_mode == 6){
		EnablePowerButton(0);
		EnablePowerKeyEvent(1);
		m_volume_level = g_volume_level; // set m_volume_level 0
		g_volume_level = 15;
		SetTinymixVolumeTestMode(g_level15[g_volume_level]);
		DEBUG("Entry Test mode volume Level: %d[%d], volume: %d\n", g_level15[g_volume_level], g_volume_level
			, m_volume_level);
		StartTestMode(renderer, font);
		test_mode = 0;
		g_volume_level = 7;
                SetTinymixVolumeTestMode(g_level15[g_volume_level]);
                printf("StartTestMode End\n");
		SDL_Delay(4000);
	} else if(test_mode == 2){
		EnablePowerButton(1);
 		WiFiTestMode(renderer, font);
	} // else if
	
	DEBUG("vol thread loop exit\n");

	// Animation Play
	Animation(renderer, font, animation);
	SDL_Delay(1000);

	ShowLegalPage(renderer, LEGAL_PAGE);
	SDL_Delay(3000);

	g_quit = 1;
	SDL_WaitThread(checkvolthread, &ret_checkvol);
	SDL_Delay(10);

	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyMutex(g_key_mutex);
	SDL_DestroyWindow(window);
	TTF_CloseFont(font);
	TTF_Quit();
	Mix_CloseAudio();
	SDL_Quit();
	EnablePowerButton(1);
	system("echo 3 > /proc/sys/vm/drop_caches");
	system("echo \"1080000\" > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq");
	exit(0);
}//main

