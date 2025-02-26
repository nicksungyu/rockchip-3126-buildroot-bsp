#include <iostream>
#include <iomanip>
#include <usbpp.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>   //ifreq
#include <stdio.h>
#include <time.h>
#include <iwlib.h>
using namespace std;
#define TEST_SSID "TA_WifiMachine_Tester"
#define WIFI_CND_MAX 128
void PrintStarPWD(SDL_Renderer* renderer, TTF_Font* font);
static int GetPWDKey(SDL_Renderer* renderer, TTF_Font* font, int timeout, int device_index);
void PrintWiFiDeviceInfo(SDL_Renderer* renderer, TTF_Font* font, int device_index);
void PrintWiFiTop5(SDL_Renderer* renderer, TTF_Font* font);
void PrintTastemaker_OfficeAvg(SDL_Renderer* renderer, TTF_Font* font);
void WiFiTestMode(SDL_Renderer* renderer, TTF_Font* font);
void TestFingerprint();

int fingerprint_status = 0;
struct wifi_data{
	int vid;
	int pid;
	char name[256];
	char driver_version_path[256];
};
struct wifi_ssid_data{
	char name[1024];
	int signal;
};
struct wifi_ssid_data wifi_ssid_list[WIFI_CND_MAX];
int wifi_ssid_list_cnt = 0;
struct wifi_data wifi_usb_device_list[4] = {
	{ 0x0bda, 0x8179, "RealTek RTL8188EU", "/sys/module/8188eu/version" }, //RTL8188EU
	{ 0x0bda, 0xf179, "RealTek RTL8188FU", "/sys/module/8188fu/version" }, //RTL8188FU
	{ 0x0bda, 0x0179, "RealTek RTL8188ETV", "/sys/module/8188eu/version" }, //RTL8188FU
	{ 0x8065, 0x6000, "iCOMM-SEMI SSV6X5X", "/etc/ssv6x5x_driver_version" }, //SSV6X5X
};

int GetGatewayAndIface(void);
void bubble_sort(void){
	int i = 0, j = 0;
	struct wifi_ssid_data tmp;

	if(wifi_ssid_list_cnt == 0) return;
	for(i = 1; i < (wifi_ssid_list_cnt + 1); i++){
		for(j = 1; j < (wifi_ssid_list_cnt + 1); j++){
			if(wifi_ssid_list[j].signal < wifi_ssid_list[i].signal){
				strcpy(tmp.name, wifi_ssid_list[j].name);
				tmp.signal = wifi_ssid_list[j].signal;
				strcpy(wifi_ssid_list[j].name, wifi_ssid_list[i].name);
				wifi_ssid_list[j].signal = wifi_ssid_list[i].signal;
				strcpy(wifi_ssid_list[i].name, tmp.name);
				wifi_ssid_list[i].signal = tmp.signal;
			}
		}
	}
}

void print_ssid_list(void){
	int i = 0;
	printf("print_ssid_list [%d]\n", wifi_ssid_list_cnt);
	if(wifi_ssid_list_cnt == 0) return;
	for(i = 1; i < (wifi_ssid_list_cnt + 1); i++){
		printf("SSID[%s], Signal[%d]\n", wifi_ssid_list[i].name, wifi_ssid_list[i].signal);
	}
}

char wlan0_mac[256];
void GetWLanMac(const char* iface){
	int fd;
	struct ifreq ifr;
	memset(wlan0_mac, 0, sizeof(wlan0_mac));
	if( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("open socket error\n");
		return;
	}
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, iface, IFNAMSIZ-1);
	if( ioctl(fd, SIOCGIFHWADDR, &ifr) < 0){
		printf("get hardware address error\n");
		return;
	}
	close(fd);
	strcpy(wlan0_mac, ifr.ifr_hwaddr.sa_data);
	printf("Mac : %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n" , wlan0_mac[0], wlan0_mac[1], wlan0_mac[2], wlan0_mac[3], wlan0_mac[4], wlan0_mac[5]);
	
}

// fail : return 0, success : return 1
char driver_version[256];
int readProcString(int index){
	char* fileDat = NULL;
	FILE* fp = NULL;
	long lsize = 0;

	memset(driver_version, '\0', sizeof(driver_version));
	fp = fopen(wifi_usb_device_list[index].driver_version_path, "r");
	if(fp){
		char* s = fgets(driver_version, 256, fp);
		if(s){
			char * n = strtok(driver_version, "\n");
			if(n){
				driver_version[strlen(n) + 1] = '\0';
			}
		}
		fclose(fp);
		return 1;
	}
	return 0;
}

int readWPAStatus(const char* wpa_cmd){
	char msg[4096];
	FILE* fp = popen(wpa_cmd, "r");
	int ret = 0;
	SDL_Delay(4000);
	if(fp){
		while(fgets(msg, sizeof(msg), fp)){
			printf("%s", msg);
			if(!strncmp(msg, "wpa_state=COMPLETED", strlen("wpa_state=COMPLETED"))){
				ret = 1;
			}else if(!strncmp(msg, "wpa_state=4WAY_HANDSHAKE", strlen("wpa_state=4WAY_HANDSHAKE"))){
				ret = 2;//password fail
			}else if(!strncmp(msg, "wpa_state=SCANNING", strlen("wpa_state=SCANNING"))){
				ret = 3;
//			}else if(!strncmp(msg, "wpa_state=DISCONNECTED", strlen("wpa_state=DISCONNECTED"))){
//				ret = 3;//connect fail
			}
		}
		pclose(fp);
	}
	return ret;
}

int readUDHCPCStatus(const char* udhcpc_cmd){
	char msg[4096];
	FILE* fp = popen(udhcpc_cmd, "r");
	int ret = 0;
	if(fp){
		while(fgets(msg, sizeof(msg), fp)){
			printf("%s", msg);
			if(!strncmp(msg, "no lease, ", strlen("no lease, "))){
				printf("Can not get IP\n");
				ret = 1;
			}else if(!strncmp(msg, "deleting routers", strlen("deleting routers"))){
				printf("Got IP\n");
				return 2;
			} // else if
		}
		pclose(fp);
	}
	return ret;
}

int ping_transmitted = 0, ping_received = 0, ping_loss = 0;
float ping_min = 0, ping_avg = 0, ping_max = 0;
int readPingStatus(const char* ping_cmd){
	char msg[4096];
	FILE* fp = popen(ping_cmd, "r");
	int ret = 0;
	if(fp){
		while(fgets(msg, sizeof(msg), fp)){
			printf("%s", msg);
			if(strstr(msg, "packets transmitted")){
				sscanf(msg, "%d packets transmitted, %d packets received, %d packet loss", &ping_transmitted, &ping_received, &ping_loss);
				printf("transmitted[%d], received[%d], loss[%d]\n", ping_transmitted, ping_received, ping_loss);
			}else if(strstr(msg, "round-trip min/avg/max")){
				sscanf(msg, "round-trip min/avg/max = %f/%f/%f ms", &ping_min, &ping_avg, &ping_max);
				printf("latency min[%f]/avg[%f]/max[%f]\n", ping_min, ping_avg, ping_max);
			}
		}
		pclose(fp);
		return 1;
	}
	return 0;
}

/*
	fail return 0, match target string return 1.
*/
int ScanWiFiSSIDandFind(char* iface, const char* target){
	wireless_scan_head head;
	wireless_scan *result;
	iwrange	range;
	iwstats stats;
	int sock = -1, quality = -1, has_range, ret = 0, re_scan_times = 0;
	double dbm = -100.0;
ret_find:
	sock = -1; quality = -1; has_range = 0; ret = 0; dbm = -100.0;
	wifi_ssid_list_cnt = 0;
	sock = iw_sockets_open();
	memset(wifi_ssid_list, '\0', sizeof(struct wifi_ssid_data) * WIFI_CND_MAX);
	if(sock != -1){
		if(iw_get_range_info(sock, iface, &range) < 0){
			printf("Error during iw_get_range_info.\n");
			return -2;
		}
		if(iw_scan(sock, iface, range.we_version_compiled, &head) < 0){
			printf("Error during iw_scan\n");
			return -3;
		}
		result = head.result;
		while (NULL != result){//wireless_tools_rtl8188eu->iwlib.c iw_print_stats()
			if(!strcmp(result->b.essid, "")){
				result = result->next;
				continue;
			}
			if((result->stats.qual.level != 0) || (result->stats.qual.updated & (IW_QUAL_DBM | IW_QUAL_RCPI))){
				if(result->stats.qual.updated & IW_QUAL_RCPI){
					if(!(result->stats.qual.updated & IW_QUAL_LEVEL_INVALID)){
						dbm = (result->stats.qual.level / 2.0) - 110.0;
					}
				}else{
					if((result->stats.qual.updated & IW_QUAL_DBM) || (result->stats.qual.level > range.max_qual.level)){
						if(!(result->stats.qual.updated & IW_QUAL_LEVEL_INVALID)){
							if(result->stats.qual.level >= 64)
								dbm = result->stats.qual.level - 0x100;
							else
								dbm = result->stats.qual.level;
						}
					}else{
						if(!(result->stats.qual.updated & IW_QUAL_LEVEL_INVALID)){
							dbm = result->stats.qual.level; //range->max_qual.level
						}
					}
				}
			}
			if(dbm >= -50.0)
				quality = 100.0;
			else if(dbm >= -80.0)
				quality = (unsigned int) ( 24.0 + ((dbm + 80.0) * 26.0) / 10.0);
			else if(dbm >= -90.0)
				quality = (unsigned int) ( ((dbm + 90.0) * 26.0) / 10.0 );
			else
				quality = 0;
			printf("%s, %f, %d, %d, key[%d][%d][%d]\n", result->b.essid, dbm, quality, result->has_stats, result->b.has_key, result->b.key_size, result->b.key_flags);

			if(wifi_ssid_list_cnt < WIFI_CND_MAX){
				wifi_ssid_list_cnt++;
				strcpy(wifi_ssid_list[wifi_ssid_list_cnt].name, result->b.essid);
				wifi_ssid_list[wifi_ssid_list_cnt].signal = quality;
			}
			if(!strcmp(result->b.essid, target)){
				ret = 1;
			}
			result = result->next;
		}
		iw_sockets_close(sock);
		bubble_sort();
		print_ssid_list();
	}

	if(ret == 0 && re_scan_times <= 3){
		re_scan_times++;
		goto ret_find;
	}

	if(ret == 1) printf("Found SSID [%s]\n", target);
	else printf("No found SSID[%s]\n", target);
	return ret;
}

int CheckWiFiVidPid(void){
	USB::Busses buslist;
	USB::Bus *bus;
	list<USB::Bus *>::const_iterator biter;
	USB::Device *device;
	list<USB::Device *>::const_iterator diter;
	for(biter = buslist.begin(); biter != buslist.end();biter++){
		bus = *biter;
		for(diter = bus->begin(); diter != bus->end(); diter++){
			device = *diter;
			//printf("device list have %d\n", sizeof(wifi_usb_device_list) / sizeof(wifi_usb_device_list[0]));
			for(int i = 0; i < sizeof(wifi_usb_device_list) / sizeof(wifi_usb_device_list[0]); i++){
				if(device->idVendor() == wifi_usb_device_list[i].vid &&
					device->idProduct() == wifi_usb_device_list[i].pid){
					printf("Found WiFi USB [%s]\n", wifi_usb_device_list[i].name);
					GetWLanMac("wlan0");
					if(readProcString(i) == 1){
						printf("driver version [%s]\n", driver_version);
					}
					return i;
				}
			}
		}
	}
	return 0;
}

int GetTmpSSID(char* fn){//have bug, tha mac data can not get correct data
	FILE* fp = NULL;
	char ssid[1024];
	char secure[1024];
	char secure_type[1024];
	int signal;
	char mac[1024];
	fp = fopen(fn, "r");
	if(fp){
		do{
			memset(ssid, '\0', sizeof(ssid));
			memset(secure, '\0', sizeof(secure));
			memset(secure_type, '\0', sizeof(secure_type));
			memset(mac, '\0', sizeof(mac));
			fscanf(fp, "%s,%s,%s,%d%,%s", ssid, secure, secure_type, &signal, mac);
			printf("%s,%s,%s,%d\%,%s\n", ssid, secure, secure_type, signal, mac);
		}while(!feof(fp));
		fclose(fp);
		return 1;
	}
	return 0;
}

static int GetAnyKey(void* render, int timeout){
	SDL_Event event;
	ClearKey();
	Uint32 start_time = SDL_GetTicks();
	while(1){
		if(SDL_PollEvent(&event) != 0){
			switch(event.type){
				case SDL_KEYDOWN:
					break;
				case SDL_KEYUP:
					return 1;
					break;
			}
		}
		if(((SDL_GetTicks() - start_time) / 1000.0) > timeout){
			return 1;
		}
		SDL_Delay(5);
	}
	return 0;
}

char wifi_password[11] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
int password_cnt = 0;
int wifi_test_status = 0; //0:Scanning WiFi SSID
Uint32 password_start_time = 0;
int password_idle_time = 30;
int scanning_got = 0;
static int GetPWDKey(SDL_Renderer* renderer, TTF_Font* font, int timeout, int device_index){
	SDL_Event event;
	ClearKey();
	password_start_time = SDL_GetTicks();
	while(1){
        PrintWiFiDeviceInfo(renderer, font, device_index);
        PrintWiFiTop5(renderer,font);
		PrintStarPWD(renderer, font);
		SDL_RenderPresent(renderer);
		if(SDL_PollEvent(&event)){
			printf("SDL_PollEvent %d\n", event.type);
			switch(event.type){
				case SDL_KEYDOWN:
					printf("SDL_KEY_DOWN\n");
					break;
				case SDL_KEYUP:
					printf("SDL_KEYUP\n");
					switch(event.key.keysym.sym){
						case SDLK_RETURN:
							wifi_test_status = 2;
							return 1;
						case SDLK_x: //p1, shoot
							strcpy(wifi_password, "TA$wifi135");//'a'
							// strcpy(wifi_password, "86872623");//'a'
							break;
						case SDLK_z:
						case SDLK_s:
							strcpy(wifi_password, "~My[E2a[");//'a'
							break;
						case SDLK_F1:
							// strcpy(wifi_password, "btewan6660106");//'a'
							strcpy(wifi_password, "86872623");//'a'
							break;
						/*	
						case SDLK_x: //p1, pass
							strcpy(wifi_password, "7XpYqF'R");//'b'
							break;
						
						case SDLK_z: //p1, turbo
							strcpy(wifi_password, "y]sQ}!dK");//'c'
							break;
						*/	
						case SDLK_KP_5: //p2, shoot
							strcpy(wifi_password, "vJApQS~8");//'d'
							break;
						case SDLK_KP_2: //p2, pass
							strcpy(wifi_password, "{dsYk=:c");//'e'
							break;
						case SDLK_KP_1: //p2, turbo
							strcpy(wifi_password, "'NZ!ZFR^");//'f'
							break;
						case SDLK_4:   //p3, shoot
							strcpy(wifi_password, "zp&g}#D>");//'g'
							break;
						case SDLK_o:   //p3, pass
							strcpy(wifi_password, "*!NqN>X$");//'h'
							break;
						case SDLK_u:   //p3, turbo
							strcpy(wifi_password, "F'4n4%p)");//'i'
							break;
						case SDLK_9:   //p4, shoot
							strcpy(wifi_password, "*C]#5eK@");//'j'
							break;
						case SDLK_m:   //p4, pass
							strcpy(wifi_password, "rL7}5*6q");//'k'
							break;
						case SDLK_v:   //p4, turbo
							strcpy(wifi_password, "W/wqa#k3");//'l'
							break;
					}
					password_cnt++;
					printf("%d, cnt=%d, [%s]\n", event.key.keysym.sym, password_cnt, wifi_password);
					if(password_cnt >= 1){
						PrintWiFiDeviceInfo(renderer, font, device_index);
				        PrintWiFiTop5(renderer,font);
						PrintStarPWD(renderer, font);
						SDL_RenderPresent(renderer);
						return 0;
					}
					break;
				default:
					printf("SDL_PollEvent default %d\n", event.type);
					break;
			}
		}

		if(((SDL_GetTicks() - password_start_time) / 1000.0) > timeout){
			return 1;
		}
		SDL_Delay(5);
	}
	return 0;
}

char* wpa_supplicant_conf[2048];
#define WPA_SUPPLICANT_CONF_PATH "/tmp/wpa_supplicant.conf"
char wpa_supplicant_text[128][1024] = {
	"ctrl_interface=/var/run/wpa_supplicant\n",
	"update_config=1\n",
	"network={\n",
	"ssid=\"STANLEY2\"\n",
	"psk=\"86872623\"\n",
	"}\n",
};

int CreateWPASupplicantConf(const char* ssid, char* pwd){
	int i = 0;
	FILE* fp = NULL;
	fp = fopen(WPA_SUPPLICANT_CONF_PATH, "w");
	if(fp){
		sprintf(wpa_supplicant_text[3], "ssid=\"%s\"\n", ssid);
		sprintf(wpa_supplicant_text[4], "psk=\"%s\"\n", pwd);
		for(i = 0; i < 6; i++){
			fwrite(wpa_supplicant_text[i], strlen(wpa_supplicant_text[i]), 1, fp);
		}
		fclose(fp);
		return 1;
	}
	return 0;
}

int scanning = 1;
char wifi_list[6][1024];
int wifi_list_signal[6], signal_small = 0;
int wifi_refresh_count[7] = {0, 0, 0, 0, 0, 0, 0};
static int ScanSSID(void){
	static int count = 0;
	char iface[256];
	int wifi_id = 0, i = 0;

	printf("start scan ssid function\n");
	if(count == 6){
		printf("ScanSSID count=%d, skip ScanWiFi\n", count);
		return 6;
	}
	sprintf(iface, "%s", "wlan0");
	wifi_id = CheckWiFiVidPid();
	scanning_got = ScanWiFiSSIDandFind(iface, TEST_SSID);
	memset(wifi_list, ' ', sizeof(wifi_list));
	if(wifi_ssid_list_cnt >= 1){
		for(i = 1; i <= 5; i++){//Only print top 5
			sprintf(wifi_list[i], "%s (%d%%)", wifi_ssid_list[i].name, wifi_ssid_list[i].signal);
			wifi_list_signal[i] = wifi_ssid_list[i].signal;
		}
	}
	count++;
	if(count > 6) count = 6;
	if(scanning_got == 1){
		for(i = 1; i <= 5; i++){
			if(strstr(wifi_list[i], TEST_SSID)){
				wifi_refresh_count[count] = wifi_list_signal[i];
				break;
			}
		}
	}
	switch(wifi_id){
		case 0:
		case 2:
			printf("reload 8188eu wifi driver\n");
			system("/sbin/rmmod 8188eu");
			system("/sbin/insmod /8188eu.ko");
			break;
		case 1:
			printf("reload 8188fu wifi driver\n");
			system("/sbin/rmmod 8188fu");
			system("/sbin/insmod /8188fu.ko");
			break;
	}
	system("/sbin/ifconfig wlan0 down; /sbin/ifconfig wlan0 up");
	return count;
}

int get_fingerprint = 0;
static int FingerThread(void* render){
	char msg[256];
	int ret = 0;
	get_fingerprint = 1;
	printf("FingerThread: fingerprint fail, try to get new one\n");
	//sprintf(msg, "cd /game; LD_LIBRARY_PATH=/game/lib /game/OnlineHub -init -connection=1 -ssid=%s -wifi_pw=%s -secure=wpa2 -ssid_mac=any -render=false -auto_factory=key", TEST_SSID, wifi_password);
	sprintf(msg, "cd /game; LD_LIBRARY_PATH=/game/lib /game/OnlineHub -render=false -auto_factory=key");
	ret = system(msg);
	printf("OnlineHub exit code=%d\n", ret);
	if(ret == 50 || ret == 0){
		system("/sbin/fingerprint w");
		if(ret == 0){
			printf("a new token.bin is generated, because no token was found\n");
		}else if(ret == 50){
			printf("50:the token.bin was found in eeprom and the server confirmed it valid\n");
		}
		get_fingerprint = 0;
	}else{
		switch(ret){
			case 51:
				printf("51:the token.bin was found and the server said it was invalid\n");
				break;
			case 52:
				printf("52:the token.bin was not found but server refused to create one (or it could not be contacted)\n");
				break;
			default:
				printf("%d:fingerprint not write\n", ret);
				break;
		}
		get_fingerprint = 2;
	}
	printf("FingerThread: Exit.\n");
	return 0;
}

void PrintStarPWD(SDL_Renderer* renderer, TTF_Font* font){
	printf("%s\n", __func__);
	int index;
	char star_pwd[128], msg[128];
	memset(star_pwd, '\0', sizeof(star_pwd));
	//TTF_Font* font25 = TTF_OpenFont(MENU_FONT, 25);
	sprintf(msg, "PASS. Please input password(%02d):", password_idle_time - (SDL_GetTicks() - password_start_time) / 1000);
	ShowText(renderer, font, msg, 0, -75, 0, 255, 0, 0);
	for(index = 0; index < password_cnt; index++){
		star_pwd[index] = '*';
	}
	printf("star[%s] %s\n", star_pwd, msg);
	ShowText(renderer, font, star_pwd, 0, -50, 0);
	//TTF_CloseFont(font25);
}

char wifi_ip[INET_ADDRSTRLEN], wifi_mask[INET_ADDRSTRLEN], wifi_gw[INET_ADDRSTRLEN];
void GetIPInfo(const char* iface){
	int fd_arp;      /* socket fd for receive packets */
	u_char *ptr;
	struct ifreq ifr; /* ifr structure */
	struct sockaddr_in *sin_ptr;

	strcpy(ifr.ifr_name, iface);

	if ((fd_arp = socket(AF_INET, SOCK_PACKET, htons(0x0806))) < 0) {
		printf("socket create fail\n");
	}

	if(fd_arp){
		//get ip address of interface
		if(ioctl(fd_arp, SIOCGIFADDR, &ifr) < 0) {
			memset(wifi_ip, '\0', sizeof(wifi_ip));
			printf("SIOCGIFADDR failed\n");
		}else{
			sin_ptr = (struct sockaddr_in *)&ifr.ifr_addr;
			sprintf(wifi_ip, "%s", inet_ntoa(sin_ptr->sin_addr));
		}
		//get network mask of my interface
		if(ioctl(fd_arp, SIOCGIFNETMASK, &ifr) < 0){
			memset(wifi_mask, '\0', sizeof(wifi_mask));
			printf("SIOCGIFNETMASK failed\n");
		}else{
			sin_ptr = (struct sockaddr_in *)&ifr.ifr_addr;
			sprintf(wifi_mask, "%s", inet_ntoa(sin_ptr->sin_addr));
		}
		//get mac address of the interface
		//if(ioctl(fd_arp, SIOCGIFHWADDR, &ifr) < 0){
		//	ptr = NULL;
		//}else{
		//	ptr = (u_char *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
		//}
		//printf( "request netmask %s\n", inet_ntoa(mymask));
		//printf( "request IP %s\n", inet_ntoa(myip));
		close(fd_arp);
	}
}

void TestFingerprint() {
	int fg_ret = 0;
	fg_ret = system("/sbin/fingerprint t > /dev/null");
	if(fg_ret == 0)
		fingerprint_status = 1;
	else 
		fingerprint_status = 0;
}

void PrintWiFiDeviceInfo(SDL_Renderer* renderer, TTF_Font* font, int device_index){
	char wifi_desc[1024], driver_ver[1024], mac_data[1024];
	char msg[2048];
	sprintf(wifi_desc, "WiFi Device : %s[0x%04x:0x%04x]", wifi_usb_device_list[device_index].name, wifi_usb_device_list[device_index].vid, wifi_usb_device_list[device_index].pid);
	sprintf(driver_ver, "WiFi Driver Version: %s", driver_version);
	sprintf(mac_data, "WiFi Device MAC : %.2X:%.2X:%.2X:%.2X:%.2X:%.2X", wlan0_mac[0], wlan0_mac[1], wlan0_mac[2], wlan0_mac[3], wlan0_mac[4], wlan0_mac[5]);
	ShowText(renderer, font, wifi_desc,  0, -425, 0);
	ShowText(renderer, font, driver_ver, 0, -400, 0);
	ShowText(renderer, font, mac_data,  0, -375, 0);
	
	if(fingerprint_status == 1){
		sprintf(msg, "Fingerprint: OK");
		ShowText(renderer, font, msg, 0, -350, 0, 255, 0, 0);
	}else{
		sprintf(msg, "Fingerprint: Not Ready");
		ShowText(renderer, font, msg, 0, -350, 255, 0, 0, 0);
	}
}

void PrintWiFiTop5(SDL_Renderer* renderer, TTF_Font* font){
	int i = 0;
	char msg[256];
	memset(wifi_list, ' ', sizeof(wifi_list));
	if(wifi_ssid_list_cnt >= 1){
		for(i = 1; i <= 5; i++){//Only print top 5
			sprintf(wifi_list[i], "%s (%d%%)", wifi_ssid_list[i].name, wifi_ssid_list[i].signal);
			wifi_list_signal[i] = wifi_ssid_list[i].signal;
		}
	}
	sprintf(msg, "%s", "WiFi AP Signal Top 5");
	ShowText(renderer, font, msg,  0, -300, 0);
	sprintf(msg, "%s", "==========================================================");
	ShowText(renderer, font, msg,  0, -275, 0);
	for(i = 1; i <= 5; i++){
		if(strstr(wifi_list[i], TEST_SSID)){
			if(wifi_list_signal[i] < 70){
				ShowText(renderer, font, wifi_list[i], 0, -275 + i * 25, 255, 0, 0, 0);
			}else{
				ShowText(renderer, font, wifi_list[i], 0, -275 + i * 25, 0, 255, 0, 0);
			}
		}else{
			ShowText(renderer, font, wifi_list[i], 0, -275 + i * 25, 0);
		}
	}
	sprintf(msg, "%s", "===========================================================");
	ShowText(renderer, font, msg,  0, -125, 0);
}

void PrintTastemaker_OfficeAvg(SDL_Renderer* renderer, TTF_Font* font){
	char msg[2048];
	sprintf(msg, "FAIL. %s:1st[%d], 2nd[%d], 3rd[%d], 4th[%d], 5th[%d], 6th[%d]", TEST_SSID, wifi_refresh_count[1], wifi_refresh_count[2], wifi_refresh_count[3], wifi_refresh_count[4], wifi_refresh_count[5], wifi_refresh_count[6]);
	ShowText(renderer, font, msg,  0, -100, 255, 0, 0, 0);
}

void RunScanWifi(SDL_Renderer* renderer, TTF_Font* font, int device_index){
	ScanSSID();
	PrintWiFiDeviceInfo(renderer, font, device_index);
	PrintWiFiTop5(renderer,font);
	printf("1st[%d], 2nd[%d], 3rd[%d], 4th[%d], 5th[%d], 6th[%d]\n", wifi_refresh_count[1], wifi_refresh_count[2], wifi_refresh_count[3], wifi_refresh_count[4], wifi_refresh_count[5], wifi_refresh_count[6]);
}

void ClearFingerprint(void){
	char cmd[1024];
	printf("ClearFingerprint\n");
	remove("/game/docs/network.ini");
	remove("/game/docs/token.bin");
	for(int i = 0; i <= 31; i++){
		sprintf(cmd, "i2cset -y 0 0x50 %d 0xff", i);
		system(cmd);
	}
}

void WiFiTestMode(SDL_Renderer* renderer, TTF_Font* font){
	int ret_musicthread = 0, ret_pwdthread = 0, ret_fingerthread = 0;
	int device_index = 0, i = 0, wpa_status = 0;
	int avg_2nd_3rd = 0, wifi_password_fail_reconn_times = 0;;
	SDL_Thread* musicthread = NULL, *pwdthread = NULL, *fingerthread = NULL;
	TTF_Font* font50 = TTF_OpenFont(MENU_FONT, 50);
	TTF_Font* font25 = TTF_OpenFont(MENU_FONT, 25);
	printf("Enter WiFi Test Mode\n");
	musicthread = SDL_CreateThread(TestMusicThread, "TestMusicThread", (void*)renderer);

	//ClearFingerprint();

	printf("killall wpa_supplicant udhcp\n");
	system("/usr/bin/killall -9 wpa_supplicant udhcpc; /bin/sleep 3");
	printf("ifconfig wlan0 down and up\n");
	system("/sbin/ifconfig wlan0 down; /sbin/ifconfig wlan0 up");
	printf("FillScreen start\n");
	FillScreen(renderer, 0, 0, 0, 1);
	TestFingerprint();

	if((device_index = CheckWiFiVidPid()) != 0){ // Got USB Wireless Device
		int i = 0;
		char msg[256];
		bool low_40 = false;
		int avg_all = 0;
		PrintWiFiDeviceInfo(renderer, font, device_index);
		SDL_RenderPresent(renderer);
		SDL_Delay(1000);
		for(int i = 1; i <= 6; i++){
			RunScanWifi(renderer, font, device_index);
			SDL_RenderPresent(renderer);
			if(scanning_got == 1 && wifi_refresh_count[i] >= 70){
				printf("%dst:%s >=70%%, Pass\n", i, TEST_SSID);
				goto password_input;
			}else if(scanning_got == 1 && wifi_refresh_count[i] < 40){
				printf("%dst:%s <40%%, Failed\n", i, TEST_SSID);
				low_40 = true;
			}else{
				printf("%dst:%s <70%%, Fail\n", i, TEST_SSID);
			}
			if(i==6){
				if(!low_40){
					printf("6th will calculate average.\n");
					for(int j = 1; j <= 6 ; j++) avg_all = avg_all + wifi_refresh_count[j];
					avg_all = avg_all / 6;
					printf("average=%d\n", avg_all);
					if(avg_all >= 50)
						goto password_input;
				}else
					printf("Lower than 40 in any round, failed.\n");
			}
			SDL_Delay(2000);
		}
		RunScanWifi(renderer, font, device_index);
		PrintTastemaker_OfficeAvg(renderer, font);
		SDL_RenderPresent(renderer);
		SDL_Delay(5000);
		//loop forever
		while(1) {SDL_Delay(5000);}
password_input:
		printf("password_input=%d\n", i);
		int ret = GetPWDKey(renderer, font, password_idle_time, device_index);
		if(ret == 1) goto password_timeout;
		SDL_Delay(10);
		goto password_done;
password_timeout:
		PrintWiFiDeviceInfo(renderer, font, device_index);
		sprintf(msg, "%s %s", TEST_SSID, "Timeout");
		ShowText(renderer,font50, msg, 0, -50, 255, 0, 0, 1);
		while(1) {SDL_Delay(5000);}
password_done:
		printf("pwssword done\n");
		CreateWPASupplicantConf(TEST_SSID, wifi_password);
wifi_password_fail_reconn:
		system("/usr/bin/killall wpa_supplicant");
		system("/usr/sbin/wpa_supplicant -Dwext -iwlan0 -c/tmp/wpa_supplicant.conf -dd -B 2>&1 > /tmp/wpa_supplicant.log");
wifi_reconn:
		printf("wpa_cli status\n");
		wpa_status = readWPAStatus("/usr/sbin/wpa_cli -i wlan0 status");
		if(wpa_status == 3) goto wifi_reconn;
		
		if(wpa_status == 1){//Connected WiFi AP
			PrintWiFiDeviceInfo(renderer, font, device_index);
			sprintf(msg, "%s %s", TEST_SSID, "Connected(OK)");
			ShowText(renderer, font, msg, 0, -50, 0, 255, 0, 1);
			goto wifi_connected_ok;
		}else if(wpa_status == 2){//Password Fail
			if(wifi_password_fail_reconn_times <= 3){
				wifi_password_fail_reconn_times++;
				goto wifi_password_fail_reconn;
			}
			PrintWiFiDeviceInfo(renderer, font, device_index);
			sprintf(msg, "%s %s", TEST_SSID, "Password Incorrect");
			ShowText(renderer,font50, msg, 0, -50, 255, 0, 0, 1);
			while(1) {SDL_Delay(5000);}
		}else{//Connecting WiFi AP
			static int dot = 0;
			static Uint32 connect_start = SDL_GetTicks();
			char connecting_msg[256] = "Tastemaker_Office Connecting";
			strncat(connecting_msg, ".....", dot);
			ShowText(renderer, font, connecting_msg,  0, -300, 1);
			dot++;
			if(dot > 3) dot = 0;
			if( (SDL_GetTicks() - connect_start) / 1000 >= 30){
				printf("connecting > 30 sec\n");
				sprintf(connecting_msg, "%s %s", TEST_SSID, "Connect Fail");
				ShowText(renderer, font50, connecting_msg, 0, -300, 255, 0, 0, 1);
			}else{
				printf("%d\n", (SDL_GetTicks() - connect_start) / 1000);
			}
			SDL_Delay(500);
		}
		goto wifi_reconn;
wifi_connected_ok:
		//start dhcp
		system("/usr/bin/killall -9 udhcpc");
		ret = readUDHCPCStatus("/sbin/udhcpc -i wlan0 -b -t 10 -q -n");
		if(ret == 2){//got ip back
			wifi_test_status = 6;
		}else{
			sprintf(msg, "%s", "Can not get IP from DHCP server");
			ShowText(renderer, font50, msg, 0, -300, 255, 0, 0, 1);
			GetAnyKey(renderer, 30);
			goto wifi_test_exit;
		}
		SDL_Delay(2000);
		//get ip & gateway
		GetIPInfo("wlan0");
		GetGatewayAndIface();
		printf("ip=[%s], netmask=[%s], gw=[%s]\n", wifi_ip, wifi_mask, wifi_gw);
		//show IP/mask/gateway and start to ping
		sprintf(msg, "IP : [%s]", wifi_ip);
		ShowText(renderer, font25, msg, 0, -250, 0, 255, 0, 0);
		sprintf(msg, "Mask : [%s]", wifi_mask);
		ShowText(renderer, font25, msg, 0, -200, 0, 255, 0, 0);
		sprintf(msg, "Gateway : [%s]", wifi_gw);
		ShowText(renderer, font25, msg, 0, -150, 0, 255, 0, 1);
		sprintf(msg, "/bin/ping -c 5 %s", wifi_gw);

                readPingStatus(msg);
		sprintf(msg, "IP : [%s]", wifi_ip);
		ShowText(renderer, font25, msg, 0, -250, 0, 255, 0, 0);
		sprintf(msg, "Mask : [%s]", wifi_mask);
		ShowText(renderer, font25, msg, 0, -200, 0, 255, 0, 0);
		sprintf(msg, "Gateway : [%s]", wifi_gw);
		ShowText(renderer, font25, msg, 0, -150, 0, 255, 0, 0);
		sprintf(msg, "/bin/ping -c 5 %s", wifi_gw);
		sprintf(msg, "%d packets transmitted, %d packets received, %d%% packet loss", ping_transmitted, ping_received, ping_loss);
		if(ping_loss > 0){
			ShowText(renderer, font25, msg, 0, -100, 255, 0, 0, 0);
		}else{
			ShowText(renderer, font25, msg, 0, -100, 0, 255, 0, 0);
		}
		sprintf(msg, "Packets Latency min[%.03f]/avg[%.03f]/max[%.03f] ms", ping_min, ping_avg, ping_max);
		if(ping_avg > 500.0){
			ShowText(renderer, font25, msg, 0, -50, 255, 0, 0, 0);
		}else{
			ShowText(renderer, font25, msg, 0, -50, 0, 255, 0, 0);
		}
		sprintf(msg, "Press Any Key to continue...");
		ShowText(renderer, font25, msg, 0, -25, 1);
		GetAnyKey(renderer, 30);

		//Checking Token
		ret = system("/sbin/fingerprint t");
		if(ret == 0){
			goto wifi_test_exit;
		}else if(ret == 50){ //Can not find EEPROM
			printf("Can not find EEPROM\n");
			sprintf(msg, "Can not find EEPROM, Press Any Key to continue...");
			ShowText(renderer, font25, msg, 0, -300, 255, 0, 0, 1);
			GetAnyKey(renderer, 30);
			goto wifi_test_exit;
		}else if(ret == 40){ //EEPROM read error
			printf("EEPROM read error\n");
			sprintf(msg, "EEPROM read error, Press Any Key to continue...");
			ShowText(renderer, font25, msg, 0, -300, 255, 0, 0, 1);
			GetAnyKey(renderer, 30);
			goto wifi_test_exit;
		}
		get_fingerprint = 1;
		sprintf(msg, "Getting Fingerprint");
		ShowText(renderer, font25, msg, 0, -350, 0, 255, 0, 1);
		fingerthread = SDL_CreateThread(FingerThread, "FingerThread", (void*)renderer);
getting_fingerprint:
		if(get_fingerprint == 1){
			static int i = 0;
			char msg2[256];
			sprintf(msg2, "Getting Fingerprint");
			strncat(msg2, "...........", i);
			i++;
			if(i >= 5) i = 0;
			ShowText(renderer, font25, msg2, 0, -350, 0, 255, 0, 1);
			usleep(500000);
			goto getting_fingerprint;
		}else if(get_fingerprint == 2){
			sprintf(msg, "Fingerprint no update, Press Any Key to continue...");
			ShowText(renderer, font25, msg, 0, -350, 255, 0, 0, 1);
			SDL_WaitThread(fingerthread, &ret_fingerthread);
			GetAnyKey(renderer, 30);
		}else{
			SDL_WaitThread(fingerthread, &ret_fingerthread);
			sprintf(msg, "Fingerprint OK, Press Any Key to continue...");
			ShowText(renderer, font25, msg, 0, -350, 0, 255, 0, 1);
			GetAnyKey(renderer, 30);
		}
	}else{ // No USB Wireless Device
		char msg[1024];
		sprintf(msg, "%s", "Can not found USB WiFi Device");
		ShowTextBottom(renderer, font50, msg, 255, 0, 0, 1);
		GetAnyKey(renderer, 10);
	}
wifi_test_exit:
	TTF_CloseFont(font50);
	TTF_CloseFont(font25);
	printf("WiFi Test Mode done\n");
	printf("wait for music thread\n");
	SDL_WaitThread(musicthread, &ret_musicthread);
	printf("Exit WiFi Test Mode\n");
	system("/usr/bin/killall -9 wpa_supplicant udhcpc; /bin/sleep 3");
	system("sync;sync;sync");
}

#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>

#define BUFFER_SIZE 4096

int GetGatewayAndIface(void){
	int     received_bytes = 0, msg_len = 0, route_attribute_len = 0;
	int     sock = -1, msgseq = 0;
	struct  nlmsghdr *nlh, *nlmsg;
	struct  rtmsg *route_entry;
	// This struct contain route attributes (route type)
	struct  rtattr *route_attribute;
	char    interface[IF_NAMESIZE];
	char    msgbuf[BUFFER_SIZE], buffer[BUFFER_SIZE];
	char    *ptr = buffer;
	struct timeval tv;

	if ((sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
        	printf("socket failed");
		return EXIT_FAILURE;
	}

	memset(msgbuf, 0, sizeof(msgbuf));
	memset(wifi_gw, 0, sizeof(wifi_gw));
	memset(interface, 0, sizeof(interface));
	memset(buffer, 0, sizeof(buffer));

	/* point the header and the msg structure pointers into the buffer */
	nlmsg = (struct nlmsghdr *)msgbuf;

	/* Fill in the nlmsg header*/
	nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	nlmsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .
	nlmsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
	nlmsg->nlmsg_seq = msgseq++; // Sequence of the message packet.
	nlmsg->nlmsg_pid = getpid(); // PID of process sending the request.

	/* 1 Sec Timeout to avoid stall */
	tv.tv_sec = 1;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
	/* send msg */
	if(send(sock, nlmsg, nlmsg->nlmsg_len, 0) < 0){
		printf("send failed");
		return EXIT_FAILURE;
	}

	/* receive response */
	do{
		received_bytes = recv(sock, ptr, sizeof(buffer) - msg_len, 0);
		if (received_bytes < 0) {
			printf("Error in recv");
			return EXIT_FAILURE;
		}

		nlh = (struct nlmsghdr *) ptr;

		/* Check if the header is valid */
		if((NLMSG_OK(nlmsg, received_bytes) == 0) || (nlmsg->nlmsg_type == NLMSG_ERROR)){
			printf("Error in received packet");
			return EXIT_FAILURE;
		}

		/* If we received all data break */
		if (nlh->nlmsg_type == NLMSG_DONE)
			break;
		else {
			ptr += received_bytes;
			msg_len += received_bytes;
		}

		/* Break if its not a multi part message */
		if ((nlmsg->nlmsg_flags & NLM_F_MULTI) == 0)
			break;
	}while ((nlmsg->nlmsg_seq != msgseq) || (nlmsg->nlmsg_pid != getpid()));

	/* parse response */
	for ( ; NLMSG_OK(nlh, received_bytes); nlh = NLMSG_NEXT(nlh, received_bytes)){
		/* Get the route data */
		route_entry = (struct rtmsg *) NLMSG_DATA(nlh);

		/* We are just interested in main routing table */
		if (route_entry->rtm_table != RT_TABLE_MAIN)
			continue;

		route_attribute = (struct rtattr *) RTM_RTA(route_entry);
		route_attribute_len = RTM_PAYLOAD(nlh);

		/* Loop through all attributes */
		for ( ; RTA_OK(route_attribute, route_attribute_len);
			route_attribute = RTA_NEXT(route_attribute, route_attribute_len)){
			switch(route_attribute->rta_type) {
				case RTA_OIF:
					if_indextoname(*(int *)RTA_DATA(route_attribute), interface);
					break;
				case RTA_GATEWAY:
					inet_ntop(AF_INET, RTA_DATA(route_attribute), wifi_gw, sizeof(wifi_gw));
					break;
				default:
					break;
			}
		}

		if((*wifi_gw) && (*interface)){
			//printf("Gateway %s for interface %s\n", wifi_gw, interface);
			break;
		}
	}

	close(sock);
	return 0;
}
