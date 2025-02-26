/*
 * Simple I2C example
 *
 * Copyright 2017 Joel Stanley <joel@jms.id.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>
static inline __s32 i2c_smbus_access(int file, char read_write, __u8 command,
                                     int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;
	return ioctl(file,I2C_SMBUS,&args);
}


static inline  __s32 i2c_smbus_read_i2c_block_data(int file, __u8 command, __u8 length,
                     __u8 *values)
 {
     union i2c_smbus_data data;
     int i, err;
  
//     if (length > I2C_SMBUS_BLOCK_MAX)
//         length = I2C_SMBUS_BLOCK_MAX;
     data.block[0] = length;
  
     err = i2c_smbus_access(file, I2C_SMBUS_READ, command,
                    length == 32 ? I2C_SMBUS_I2C_BLOCK_BROKEN :
                 I2C_SMBUS_I2C_BLOCK_DATA, &data);
     if (err < 0)
         return err;
  
     for (i = 1; i <= data.block[0]; i++)
         values[i-1] = data.block[i];
     return data.block[0];
 }

static inline __s32 i2c_smbus_write_i2c_block_data(int file, __u8 command, __u8 length,
                      const __u8 *values)
 {
     union i2c_smbus_data data;
     int i;
     if (length > I2C_SMBUS_BLOCK_MAX)
         length = I2C_SMBUS_BLOCK_MAX;
     for (i = 1; i <= length; i++)
         data.block[i] = values[i-1];
     data.block[0] = length;
     return i2c_smbus_access(file, I2C_SMBUS_WRITE, command,
                 I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
 }

static inline __s32 i2c_smbus_read_byte_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
	                     I2C_SMBUS_BYTE_DATA,&data))
		return -1;
	else
		return 0x0FF & data.byte;
}

static inline __s32 i2c_smbus_write_byte_data(int file, __u8 command,
                                              __u8 value)
{
        union i2c_smbus_data data;
        data.byte = value;
        return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
                                I2C_SMBUS_BYTE_DATA, &data);
}

#define TOKEN_FILE "/game/docs/token.bin"
#define TOKEN_SIZE 28
int main(int argc, char **argv){
	uint8_t data, addr = 0x50, reg = 0x0d;
	const char *path = "/dev/i2c-0";
	char mode = 0;
	int file = 0, rc = 0, token_ret = 0;
	uint8_t fingerprint[TOKEN_SIZE];

	if (argc == 1){
		printf("./fingerprint r/w/t/e\n");
		printf("./fingerprint r : read fingerprint\n");
		printf("./fingerprint w : write fingerprint\n");
		printf("./fingerprint t : check fingerprint\n");
		printf("./fingerprint e : erase fingerprint\n");
		printf("exit code:\n");
		printf("10: can not find i2c channel 0 device node\n");
		printf("20: can not create or read %s\n", TOKEN_FILE);
		printf("30: the %s content is not %d bytes\n", TOKEN_FILE, TOKEN_SIZE);
		printf("40: read or write eeprom fail\n");
		printf("50: can not find eeprom\n");
		printf("60: write %s not success\n", TOKEN_FILE);
		printf("70: eeprom crc fail\n");
		return -1;
	}

	if (argc > 1)
		mode = argv[1][0];

	memset(fingerprint, 0x0, sizeof(fingerprint));

	file = open(path, O_RDWR);
	if (file < 0){
		printf("can not find /dev/i2c-0\n");
		return 10;
	}

	rc = ioctl(file, I2C_SLAVE, addr);
	if(rc < 0){
		printf("can't locate address %d\n", rc);
		return 80;
	}

	int id = i2c_smbus_read_byte_data(file, 0x0);
	// sleep(10);
	if (id != -1){
		if(mode == 'r'){//read fingerprint from eeprm and write into TOKEN_FILE
			rc = ioctl(file, I2C_SLAVE, addr);
			if(rc < 0){
				printf("can not find eeprom\n");
				token_ret = 50;
			}else{
				if(i2c_smbus_read_i2c_block_data(file, 0x00, 8, fingerprint) < 0){
					token_ret = 40;
				}
				usleep(10000);
				rc = ioctl(file, I2C_SLAVE, addr);
				if(i2c_smbus_read_i2c_block_data(file, 0x08, 8, (fingerprint + 8)) < 0){
					token_ret = 40;
				}
				usleep(10000);
				rc = ioctl(file, I2C_SLAVE, addr);
				if(i2c_smbus_read_i2c_block_data(file, 0x10, 8, (fingerprint + 16)) < 0){
					token_ret = 40;
				}
				usleep(10000);
				rc = ioctl(file, I2C_SLAVE, addr);
				if(i2c_smbus_read_i2c_block_data(file, 0x18, 4, (fingerprint + 24)) < 0){
					token_ret = 40;
				}
				if(token_ret == 0){
					FILE* fp = fopen(TOKEN_FILE, "wb");
					if(fp != NULL){
						int ret = fwrite(fingerprint, 1, TOKEN_SIZE, fp);
						if(ret != TOKEN_SIZE){
							printf("write %s fail\n", TOKEN_FILE);
							token_ret = 60;
						}
						fclose(fp);
						system("sync;sync");
					}else{
						printf("can not open %s file\n", TOKEN_FILE);
						token_ret = 20;
					}
				}
			}
		}else if(mode == 'w'){//read TOKENF_SIZE and write it into eeprom
			FILE* fp = fopen(TOKEN_FILE, "rb");
			if(fp != NULL){
				int ret = fread(fingerprint, 1, TOKEN_SIZE, fp);
				if(ret == TOKEN_SIZE){
					for(int i = 0; i < TOKEN_SIZE; i++){
						printf("%02x,", fingerprint[i]);
					}
					rc = ioctl(file, I2C_SLAVE, addr);
					if(rc < 0){
						printf("can not find eeprom\n");
						token_ret = 50;
					}else{
						if(i2c_smbus_write_i2c_block_data(file, 0x00, 8, fingerprint)){
							token_ret = 40;
						}
						usleep(10000);
						rc = ioctl(file, I2C_SLAVE, addr);
						if(i2c_smbus_write_i2c_block_data(file, 0x08, 8, (fingerprint + 8))){
							token_ret = 40;
						}
						usleep(10000);
						rc = ioctl(file, I2C_SLAVE, addr);
						if(i2c_smbus_write_i2c_block_data(file, 0x10, 8, (fingerprint + 16))){
							token_ret = 40;
						}
						usleep(10000);
						rc = ioctl(file, I2C_SLAVE, addr);
						if(i2c_smbus_write_i2c_block_data(file, 0x18, 4, (fingerprint + 24))){
							token_ret = 40;
						}
					}
					printf("\n");
				}else{
					printf("the file content is not %d bytes\n", TOKEN_SIZE);
					token_ret = 30;
				}
				fclose(fp);
			}else{
				printf("can not open %s file\n", TOKEN_FILE);
				token_ret = 20;
			}
		}else if(mode == 't'){//read TOKENF_SIZE and write it into eeprom
			rc = ioctl(file, I2C_SLAVE, addr);
			if(rc < 0){
				printf("can not find eeprom\n");
				token_ret = 50;
			}else{
				if(i2c_smbus_read_i2c_block_data(file, 0x00, 8, fingerprint) < 0){
					token_ret = 40;
				}
				usleep(10000);
				rc = ioctl(file, I2C_SLAVE, addr);
				if(i2c_smbus_read_i2c_block_data(file, 0x08, 8, (fingerprint + 8)) < 0){
					token_ret = 40;
				}
				usleep(10000);
				rc = ioctl(file, I2C_SLAVE, addr);
				if(i2c_smbus_read_i2c_block_data(file, 0x10, 8, (fingerprint + 16)) < 0){
					token_ret = 40;
				}
				usleep(10000);
				rc = ioctl(file, I2C_SLAVE, addr);
				if(i2c_smbus_read_i2c_block_data(file, 0x18, 4, (fingerprint + 24)) < 0){
					token_ret = 40;
				}
				//Check CRC status
				if(token_ret == 0){
					uLong eep_crc = 0;
					unsigned char tmp1[5];
					uLong cal_crc = crc32(0L, Z_NULL, 0);
					cal_crc = crc32(cal_crc, fingerprint, TOKEN_SIZE - 4);
					printf("CRC32 is 0x%x\n", cal_crc);
					sprintf((char*)tmp1, "%02x%02x%02x%02x", fingerprint[27], fingerprint[26], fingerprint[25], fingerprint[24]);
					printf("rom CRC is[%s]\n", tmp1);
					sscanf((char*)tmp1, "%lx", &eep_crc);
					if(cal_crc == eep_crc){
						printf("eeprom crc is correct\n");
					}else{
						printf("eeprom crc is wrong\n");
						token_ret = 70;
					}
				}
			}
		}else if(mode == 'e'){//erase eeprom
			char cmd[1024];
			printf("ClearFingerprint\n");
			remove("/game/docs/network.ini");
			remove("/game/docs/token.bin");
			for(int i = 0; i <= 31; i++){
				sprintf(cmd, "i2cset -y 0 0x50 %d 0xff", i);
				system(cmd);
			}
		}
	}else{
		token_ret = 50;
	}
	close(file);
	return token_ret;
}

