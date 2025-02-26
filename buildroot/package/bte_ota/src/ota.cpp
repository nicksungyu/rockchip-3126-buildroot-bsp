#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include <string.h>
#include <asm/param.h>
#include <linux/joystick.h>
#include <sys/mount.h>
/////////////////
#define MENU_FONT "/root/DroidSerif-Bold.ttf"
int WIDTH=1280;
int HEIGHT=1024;
SDL_Window *window = NULL;
SDL_Renderer* renderer = NULL;
#include <linux/fb.h>
struct fb_var_screeninfo vinfo;
void GetFramebufferRes(void){
	int fbfd = open("/dev/disp", O_RDWR);
	if(fbfd == -1){
		return;
	}
	if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1){
		close(fbfd);
		return;
	}
	printf("WIDTH=%d, HEIGHT=%d\n", WIDTH, HEIGHT);
	WIDTH = vinfo.xres;
	HEIGHT = vinfo.yres;
	close(fbfd);
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

int ShowText(SDL_Renderer* renderer, TTF_Font* font, const char* msg, int x, int y, int flush){
	int texW = 0, texH = 0, ret = 0;
	SDL_Rect dstrect;
	SDL_Color color = {255, 255, 255};
	SDL_Color red = {255, 0, 0};
	SDL_Surface* message = TTF_RenderText_Solid(font, msg, color);
	SDL_Texture* txtmsg = SDL_CreateTextureFromSurface(renderer, message);

	ShowText(renderer, font, "Updating, Please do not power off your machine.", 0, 255, 255, 0, 0, 0);
	SDL_QueryTexture(txtmsg, NULL, NULL, &texW, &texH);
#if defined(LANDSCAP)
	dstrect.x = (WIDTH - texW) / 2;
	dstrect.y = (HEIGHT - texH) / 2 + y + 64;
	dstrect.w = texW;
	dstrect.h = texH;
	SDL_RenderCopy(renderer, txtmsg, NULL, &dstrect);
#elif defined(PORTRAIT)
	dstrect.x = (WIDTH - texW) / 2 + y + 64;
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

/////////////////
#define OTA_FILE "ota.zip"
#define BOOT_PARTITION "/dev/rknand0p3"
#define ROOTFS_PARTITION "/dev/rknand0p4"
enum TYPE_FLASH{
	EMMC_FLASH,
	NAND_FLASH,
};
int flash_type = 0;
char system_model[256];
int system_model_pass = 0;
char ota_md5[256];
int ota_md5_pass = 0;
int ota_key_ready = 0;
char ota_pass[2048];
char boot1_md5[256];
int boot1_md5_pass = 0;
int boot1_ready = 0;
char boot_md5[256];
int boot_md5_pass = 0;
int boot_ready = 0;
char rootfs_md5[256];
int rootfs_md5_pass = 0;
int rootfs_ready = 0;

void filter_newline(char* str){
	int i = 0;
	for(i = 0; i < strlen(str); i++){
		if(*(str + i) == '\n') *(str + i) = '\0';
	}
}

int CheckMD5Sum(const char* filename, const char* md5){//pass:1, fail:0
	FILE * fp = NULL;
	char cmd[1024];
	char now_md5[1024];
	sprintf(cmd, "unzip -p %s %s | md5sum | awk '{print $1}'", "/tmp/ota.zip", filename);
	fp = popen(cmd, "r");
	if(fp){
		fgets(now_md5, sizeof(now_md5), fp);
		filter_newline(now_md5);
		//printf("now[%s], recod[%s]\n", now_md5, md5);
		if(!strcmp(now_md5, md5)){
			pclose(fp);
			return 1;
		}
		pclose(fp);
	}
	return 0;
}

struct burn_param_t {
        void *buffer;
        long length;
};
#define BLKREADBOOT0            _IO('v', 125)
#define BLKREADBOOT1            _IO('v', 126)
#define BLKBURNBOOT0            _IO('v', 127)
#define BLKBURNBOOT1            _IO('v', 128)

int BurnBoot1(char *device_fd){
	struct stat buf;
	struct stat st_uboot;
	struct burn_param_t param;
	unsigned char uboot[2 * 1024 * 1024];
	memset(uboot, 0x0, sizeof(uboot));
	param.buffer = uboot;
	param.length = sizeof(uboot);

	int fd = open(device_fd, O_RDWR);

	if(fd < 0){
		printf("can't open %s partition\n", device_fd);
		return -1; //fail
	}

	fstat(fd, &buf);
	if(!S_ISBLK(buf.st_mode)){
		printf("%s is not a block device\n", device_fd);
		return -1;
	}

	stat("/tmp/uboot.img", &st_uboot);
	param.length = st_uboot.st_size;
	FILE* fp = fopen("/tmp/uboot.img", "rb");
	if(fp != NULL){
		printf("write uboot start, size[%d]\n", param.length);
		fread(uboot, sizeof(char), param.length, fp);
		ioctl(fd, BLKBURNBOOT1, &param);
		printf("write uboot done\n");
		fclose(fp);
	}

	close(fd);
	return 0;//success
}

int get_flash_type(){
	int ret = EMMC_FLASH;
	struct stat s;
	stat("/dev/rknand0p7", &s);
	if((s.st_mode & S_IFMT) == S_IFBLK){
		ret = NAND_FLASH;
	}

	return ret;
}

int main(int argc, char** argv){
	int ret = 0, i = 0;
	int update_status = EXIT_SUCCESS;
	flash_type = get_flash_type();
	printf("flash type: %d\n", flash_type);

	if(flash_type == EMMC_FLASH){
		ret = mount("/dev/mmcblk0p7", "/mnt", "vfat", 0, "");	
	} else {
		ret = mount("/dev/rknand0p7", "/mnt", "vfat", 0, "");
	}

	system("echo \"1\" > /mnt/recovery.txt");
	system("echo 3 > /proc/sys/vm/drop_caches");
	umount2("/mnt", MNT_FORCE);

	//GetFramebufferRes();
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0){
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	if(TTF_Init() < 0){
		SDL_Log("TTF_Init: %s\n", TTF_GetError());
		return EXIT_FAILURE;
	}

	TTF_Font* font = TTF_OpenFont(MENU_FONT, 25);
	window = SDL_CreateWindow("title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	if(argc >= 2){
		printf("out of parameter. %d\n", argc);
		goto show_errror;
	}
	printf("Checking update files......\n");
	ShowText(renderer, font, "Checking Update Files", 0, 255, 1);

	// CHECK List.txt
	ret = system("unzip -l /tmp/ota.zip list.txt > /dev/null");
	if(ret == 0){ //list.txt OK
		FILE * fp = NULL;
		char buf[2048];
		memset(buf, '\0', sizeof(buf));
		fp = popen("unzip -p /tmp/ota.zip list.txt", "r");
		while(!feof(fp)){
			fgets(buf, sizeof(buf), fp);
			if(strlen(buf) <= 0 && feof(fp) == 1) break;
			if(!strncmp("Project Number :", buf, strlen("Project Number :"))){
				FILE* model_fp = fopen("/etc/model", "r");
				if(fp){
					fread(system_model, sizeof(system_model), 1, model_fp);
					fclose(model_fp);
					if(!strcmp(buf, system_model)){
						ShowText(renderer, font, "Checked Model:Pass", 0, 255, 1);
						system_model_pass = 1;
					}else{
						ShowText(renderer, font, "Checked Model:Fail", 0, 255, 1);
						printf("check model fail\n");
						sleep(3);
						goto exit;
					}
				}else{
					ShowText(renderer, font, "Checked Model:Fail", 0, 255, 1);
					printf("check model fail\n");
					sleep(3);
					goto exit;
				}
			}else if(!strncmp("ota.key.enc,", buf, strlen("ota.key.enc,"))){
				strcpy(ota_md5, buf + strlen("ota.key.enc,"));
				filter_newline(ota_md5);
				if(CheckMD5Sum("ota.key.enc", ota_md5) == 1){
					char cmd[1024];
					FILE* pass_fp = NULL;
					printf("ota.key.enc md5sum pass\n");
					ota_md5_pass = 1;
					sprintf(cmd, "unzip -p %s ota.key.enc | openssl rsautl -decrypt -inkey /root/ota_key.priv", "/tmp/ota.zip");
					pass_fp = popen(cmd, "r");
					if(pass_fp != NULL){//ota decrypt key ok
						memset(ota_pass, '\0', sizeof(ota_pass));
						fgets(ota_pass, sizeof(ota_pass), pass_fp);
						if(strlen(ota_pass) > 0){
							filter_newline(ota_pass);
							//printf("ota_pass=[%s]\n", ota_pass);
							ota_key_ready = 1;
						}
					}
				}else{
					ShowText(renderer, font, "Checked File:Fail", 0, 255, 1);
					printf("ota.key.enc md5sum fail\n");
					sleep(3);
					goto exit;
				}
			}else if(!strncmp("uboot.img.enc,", buf, strlen("uboot.img.enc,"))){
				strcpy(boot1_md5, buf + strlen("uboot.img.enc,"));
				filter_newline(boot1_md5);
				if(CheckMD5Sum("uboot.img.enc", boot1_md5) == 1){
					ShowText(renderer, font, "Checked Boot1 Stage 1:Pass", 0, 255, 1);
					boot1_ready = 1;
				}else{
					ShowText(renderer, font, "Checked Boot1 Stage 1:Fail", 0, 255, 1);
					sleep(3);
					goto exit;
				}
			}else if(!strncmp("boot.img.enc,", buf, strlen("boot.img.enc,"))){
				strcpy(boot_md5, buf + strlen("boot.img.enc,"));
				filter_newline(boot_md5);
				if(CheckMD5Sum("boot.img.enc", boot_md5) == 1){
					ShowText(renderer, font, "Checked Boot Stage 1:Pass", 0, 255, 1);
					boot_ready = 1;
				}else{
					ShowText(renderer, font, "Checked Boot Stage 1:Fail", 0, 255, 1);
					sleep(3);
					goto exit;
				}
			}else if(!strncmp("rootfs.img.enc,", buf, strlen("rootfs.img.enc,"))){
				strcpy(rootfs_md5, buf + strlen("rootfs.img.enc,"));
				filter_newline(rootfs_md5);
				if(CheckMD5Sum("rootfs.img.enc", rootfs_md5) == 1){
					ShowText(renderer, font, "Checked RootFS Stage 1:Pass", 0, 255, 1);
					rootfs_ready = 1;
				}else{
					ShowText(renderer, font, "Checked RootFS Stage 1:Fail", 0, 255, 1);
					sleep(3);
					goto exit;
				}
			}

			memset(buf, '\0', sizeof(buf));
		}
		pclose(fp);
	}else{
		ShowText(renderer, font, "Checked File:Fail", 0, 255, 1);
		printf("ota.key.enc md5sum fail\n");
		sleep(3);
		goto exit;
	}

	if(system_model_pass == 1 && ota_key_ready == 1){
		char cmd[2048];
		memset(cmd, '\0', sizeof(cmd));
		if(boot1_ready == 1){
			sprintf(cmd, "unzip -p %s uboot.img.enc | openssl enc -d -aes-256-cbc -pbkdf2 -out %s -pass pass:%s", "/tmp/ota.zip", "/tmp/uboot.img", ota_pass);
			ret = system(cmd);
			if(ret == 0){
				printf("decrypt uboot.img pass[%d]\n", ret);
				ShowText(renderer, font, "Checked Boot1 Stage 2:Pass", 0, 255, 1);
				memset(cmd, '\0', sizeof(cmd));

				if(flash_type == EMMC_FLASH){
					ret = BurnBoot1("/dev/mmcblk0p1");
				} else {
					ret = BurnBoot1("/dev/rknand0p1");
				}

				if(ret == 0){
					printf("Updated Boot1:Pass\n");
					ShowText(renderer, font, "Updated Boot1:Pass", 0, 255, 1);
				}else{
					update_status = EXIT_FAILURE;
					printf("Updated Boot1:Fail\n");
					ShowText(renderer, font, "Updated Boot1:Fail", 0, 255, 1);
					sleep(3);
					goto exit;
				}
				unlink("/tmp/uboot.img");
			}else{
				printf("decrypt uboot.img fail[%d]\n", ret);
				ShowText(renderer, font, "Checked Boot1 Stage 2:Fail", 0, 255, 1);
				sleep(3);
				goto exit;
			}
		}

		if(boot_ready == 1){
			sprintf(cmd, "unzip -p %s boot.img.enc | openssl enc -d -aes-256-cbc -pbkdf2 -out %s -pass pass:%s", "/tmp/ota.zip", "/dev/null", ota_pass);
			ret = system(cmd);
			if(ret == 0){
				printf("decrypt boot.img pass[%d]\n", ret);
				ShowText(renderer, font, "Checked Boot Stage 2:Pass", 0, 255, 1);
				memset(cmd, '\0', sizeof(cmd));
				if(flash_type == EMMC_FLASH)
					sprintf(cmd, "unzip -p %s boot.img.enc | openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:%s | dd of=/dev/mmcblk0p3", "/tmp/ota.zip", ota_pass); 
				else 
					sprintf(cmd, "unzip -p %s boot.img.enc | openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:%s | dd of=/dev/rknand0p3", "/tmp/ota.zip", ota_pass); 
				ret = system(cmd);
				if(ret == 0){
					ShowText(renderer, font, "Updated Boot:Pass", 0, 255, 1);
					sleep(1);
					sync();
				}else{
					update_status = EXIT_FAILURE;
					ShowText(renderer, font, "Updated Boot:Fail", 0, 255, 1);
					sleep(3);
					goto exit;
				}
			}else{
				printf("decrypt boot.img fail[%d]\n", ret);
				ShowText(renderer, font, "Checked Boot Stage 2:Fail", 0, 255, 1);
				sleep(3);
				goto exit;
			}
		}
		if(rootfs_ready == 1){
			sprintf(cmd, "unzip -p %s rootfs.img.enc | openssl enc -d -aes-256-cbc -pbkdf2 -out %s -pass pass:%s", "/tmp/ota.zip", "/dev/null", ota_pass);
			ret = system(cmd);
			if(ret == 0){
				printf("decrypt rootfs.img pass[%d]\n", ret);
				ShowText(renderer, font, "Checked RootFS Stage 2:Pass", 0, 255, 1);
				SDL_Delay(3000);
				{//Update rootfs done, then we need update cksum.txt file for Test Mode
					struct stat s;
					int mount_ret = -1;
					umount2("/mnt", MNT_FORCE);
					if(flash_type == EMMC_FLASH)
						mount_ret = stat("/dev/mmcblk0p7", &s);
					else 
						mount_ret = stat("/dev/rknand0p7", &s);

					if(mount_ret == 0){
						FILE* cksum_fp = NULL;
						char cksum_str[1024];
						int i = 0;
						if(flash_type == EMMC_FLASH)
							ret = mount("/dev/mmcblk0p7", "/mnt", "vfat", 0, "");
						else 
							ret = mount("/dev/rknand0p7", "/mnt", "vfat", 0, "");

						if(ret == 0){
							printf("System mount 'check' partition OK\n");
						}else{
							printf("System mount 'check' partition failed\n");
						}
						memset(cksum_str, '\0', sizeof(cksum_str));
						memset(cmd, '\0', sizeof(cmd));
						sprintf(cmd, "unzip -p %s rootfs.img.enc | openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:%s | cksum | awk '{printf \"%%s\",$1}' | xargs printf '%%010d'", "tmp/ota.zip", ota_pass);
						//printf("cmd=[%s]\n", cmd);
						cksum_fp = popen(cmd, "r");
						fgets(cksum_str, sizeof(cksum_str), cksum_fp);
						if(cksum_fp != NULL) pclose(cksum_fp);
						printf("cksum_str=[%s], len=[%d]\n", cksum_str, strlen(cksum_str));
						for(i = 0; i < sizeof(cksum_str); i++){
							if(cksum_str[i] == '\n') cksum_str[i] = '\0';
						}

						FILE* cksum_fp2 = fopen("/mnt/cksum.txt", "w");
						if(cksum_fp2){
							fprintf(cksum_fp2, "%s", cksum_str);
							fflush(cksum_fp2);
							fclose(cksum_fp2);
							printf("cksum=%s\n", cksum_str);
						}
						remove("/mnt/times.txt");
						umount2("/mnt", MNT_FORCE);
					}else{
						ShowText(renderer, font, "Updated RootFS checksum:Fail", 0, 255, 1);
						SDL_Delay(3000);
					}
				}

				memset(cmd, '\0', sizeof(cmd));
				if(flash_type == EMMC_FLASH) {
					printf("dd stage emmc\n");

					sprintf(cmd, "unzip -p /tmp/ota.zip rootfs.img.enc | openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:%s | dd of=/dev/mmcblk0p4", ota_pass);
				} else {
					printf("dd stage nand\n");
					sprintf(cmd, "unzip -p /tmp/ota.zip rootfs.img.enc | openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:%s | dd of=/dev/rknand0p4", ota_pass);
				} // else
				ret = system(cmd);
				if(ret == 0){
					FILE* dd_fp = NULL;
					char dd_str[1024];

					sync();
					ShowText(renderer, font, "Updated RootFS:Pass", 0, 255, 1);
					//you cannot use dd after you dd rootfs, need  reboot directly.
					sleep(3);
					printf("dd checksum check\n");
					if(flash_type == EMMC_FLASH)
						sprintf(cmd, "dd if=/dev/mmcblk0p4 | cksum | awk '{printf \"%%s\",$1}' | xargs printf '%%010d'");
					else
						sprintf(cmd, "dd if=/dev/rknand0p4 | cksum | awk '{printf \"%%s\",$1}' | xargs printf '%%010d'");

					dd_fp = popen(cmd, "r");
					if(dd_fp){
						fgets(dd_str, sizeof(dd_str), dd_fp);
						pclose(dd_fp);
						for(i = 0; i < sizeof(dd_str); i++){
							if(dd_str[i] == '\n')  dd_str[i] = '\0';
						}
						//ShowText(renderer, font, dd_str, 0, 255, 1);
						sleep(3);
					}
				}else{
					update_status = EXIT_FAILURE;
					ShowText(renderer, font, "Updated RootFS:Fail", 0, 255, 1);
					sleep(3);
					goto exit;
				}
			}else{
				printf("decrypt rootfs.fex fail[%d]\n", ret);
				ShowText(renderer, font, "Checked RootFS Stage 2:Fail", 0, 255, 1);
				sleep(3);
				goto exit;
			}
		}
	}
show_errror:
	if(argc >= 2){
		ShowText(renderer, font, "Update Files is illegal.", 0, 255, 255, 0, 0, 1);
	}
exit:
	printf("sync data\n");
	if(update_status == EXIT_SUCCESS){
		printf("EXIT_SUCCESS\n");
		if(flash_type == EMMC_FLASH)
			mount("/dev/mmcblk0p7", "/mnt", "vfat", 0, "");
		else 
			mount("/dev/rknand0p7", "/mnt", "vfat", 0, "");

		remove("/mnt/recovery.txt");
		umount2("/mnt", MNT_FORCE);
	}

	printf("drop_caches\n");
	//system("echo 3 > /proc/sys/vm/drop_caches");
	{
		FILE* fp_drop = NULL;
		char data[10] = "3";
		fp_drop = fopen("/proc/sys/vm/drop_caches", "wb");
		if(fp_drop != NULL){
			fwrite(data, 1, 1, fp_drop);
			fclose(fp_drop);
		}
	}

	printf("sync\n");
        sync();
#if 0
	printf("SDL Quit\n");
	SDL_Delay(5000);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_CloseFont(font);
	TTF_Quit();
	SDL_Quit();
#endif
	printf("reboot system\n");
	{
		FILE* fp_reboot = NULL;
		char data[10] = "1";
		fp_reboot = fopen("/proc/a40i_reboot", "wb");
		if(fp_reboot != NULL){
			fwrite(data, 1, 1, fp_reboot);
			fclose(fp_reboot);
		}
	}
}
