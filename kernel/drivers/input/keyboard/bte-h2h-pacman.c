
int deck2A[3][2][6] = {
	{//live button
                {0,  0,  0,  0,  0,  0},
                {0,  0,  0,  0,  KEY_F1,  0}, //buf[2]
        },
	{//player1
		{KEY_RIGHT,  KEY_DOWN,   KEY_LEFT,   KEY_UP,      KEY_RIGHTSHIFT,   KEY_ENTER},//buf[4]
		{KEY_A,      KEY_S,      KEY_Q,      KEY_Z,       KEY_X,            KEY_W},    //buf[5]
	},
	{//player2
		{KEY_B,      KEY_C,      KEY_G,      KEY_D,       KEY_KP9,          KEY_KP6},//buf[8]
		{KEY_KP4,    KEY_KP5,    KEY_KP7,    KEY_KP1,     KEY_KP2,          KEY_KP8},//buf[9]
	},
};

static int bte_thread(void* arg){
	loff_t pos = 0;
	int len = 18;
	uint8_t buf[18 * 2];
	uint8_t bte_buf_tx[3] = {0xA6, 0x01, 0x00};

	int bFirstCmd = 0;
	printk("bte_thread H2H Pacman start\n");

	set_current_state(TASK_UNINTERRUPTIBLE);
	
	while(!kthread_should_stop()){
		int i = 0, ret = 0, max_tries = 30, tries = 0, cnt = 0;
		pos = 0;
		if(bte_fp){
			memset(buf, 0, sizeof(buf));
			ret = vfs_write(bte_fp, bte_buf_tx, 3, &pos);//send request information to mcu
			if(ret != 3) { msleep(10); continue; }

			for(tries = 0; cnt < len && tries < max_tries; tries++){
				ndelay(1000);
				ret = vfs_read(bte_fp, buf + cnt, sizeof(buf) - cnt, &pos);//receive information from mcu
				cnt += ret;
			}

			if(cnt > 0){
				if(buf[0]==0xA7 && buf[1]==0x10){
					if(cnt != 18){
						printk("cnt=%d=>", cnt);
						for(i = 0; i < cnt; i++)
							printk("[%02x]", buf[i]);
						printk("\n");
						continue;
					}

					else {
						if(buf[2]==0x09 && buf[3]==0x01 && buf[4]==0x00 && buf[5]==0x00 && buf[6]==0x00 && buf[7]==0x02 && \
						   buf[8]==0x00 && buf[9]==0x00 && buf[10]==0x00 && buf[11]==0x00 && buf[12]==0x00 && buf[13]==0x00 && \
						   buf[14]==0x00 && buf[15]==0x00 && buf[16]==0x00 && buf[17]==0x00){
						   /*do nothing*/
						}

						else if (buf[2]==0x01 && buf[3]==0x01 && buf[4]==0x00 && buf[5]==0x00 && buf[6]==0x00 && buf[7]==0x02 && \
                                                   buf[8]==0x00 && buf[9]==0x00 && buf[10]==0x00 && buf[11]==0x00 && buf[12]==0x00 && buf[13]==0x00 && \
                                                   buf[14]==0x00 && buf[15]==0x00 && buf[16]==0x00 && buf[17]==0x00){
						   /*do nothing*/		
						}

						else {
							/*
							printk("cnt2=%d=>", cnt);
                                                	for(i = 0; i < cnt; i++)
                                                        	printk("[%02x]", buf[i]);
                                                	printk("\n");
							*/	
						}
					}

					bte_mcu_status = 1;
					//printk("v[%02x],t[%02x],s0[%02x],s1[%02x],s2[%02x],s3[%02x],b0[%02x],b1[%02x],b2[%02x],b3[%02x]\n",buf[2],buf[3],buf[4],buf[7],buf[10],buf[13],buf[5],buf[8],buf[11],buf[14]);
					if(buf[2] == 0x00){//Volume Control Mode
						key_val = 0;
					}else if(buf[2] == 0x01){
						key_val = 1;
					}else if(buf[2] == 0x02){
						key_val = 2;
					}

					if(key_val - key_val_org){
#if defined(VOL_BAR)
						if(key_val == 0){//no sound
							bte_report_key(0x0, KEY_VOLUMEUP);
							bte_report_key(0x1, KEY_VOLUMEDOWN);
							msleep(20);
						}else if(key_val == 1){//middle sound
							bte_report_key(0x0, KEY_VOLUMEUP);
							bte_report_key(0x0, KEY_VOLUMEDOWN);
							msleep(20);
						}else if(key_val==2){//large sound
							bte_report_key(0x0, KEY_VOLUMEDOWN);
							bte_report_key(0x1, KEY_VOLUMEUP);
							msleep(20);
						}
#endif
						key_val_org = key_val;
						proc_flag = 1;
					}

					for(i = 0; i < 6; i++) // live button
						bte_report_key2A(2, buf[2], 1 << i, deck2A[0][1][i]);

					for(i = 0; i < 6; i++){
						bte_report_key2A(4, buf[4], 1 << i, deck2A[1][0][i]);
					}

					p1_start = (buf[4] & 0x20) ? 1: 0;//Start, for testmode
					p1_a = (buf[5]&0x10) ?  1 : 0;//C---light kick, for test mode
					for(i = 0; i < 6; i++){
						bte_report_key2A(5, buf[5], 1 << i, deck2A[1][1][i]);
						bte_report_key2A(8, buf[8], 1 << i, deck2A[2][0][i]);
						bte_report_key2A(9, buf[9], 1 << i, deck2A[2][1][i]);
					}
					p2_start = (buf[8] & 0x20) ? 1 : 0;
#if TESTMODE
					if(bFirstCmd == 0){//skip first time result
						bFirstCmd = 1;
					}else if(bFirstCmd == 1){
						//printk("key_val=%d, p1_start=%d, p2_start=%d, p1_a=%d, p2_kp2=%d\n", key_val, p1_start, p2_start, p1_a, p2_kp2);
						if(key_val == 1 && p1_start == 1 && p2_start == 1 && p1_a == 1){
							testmode_bootup = 1;
						}
						bFirstCmd = 2;
					}else{
						if(testmode_bootup == 1){
							if(key_val != 1 || p1_start != 1 || p2_start != 1 || p1_a != 1){
								testmode_bootup = 0;
							}
						}
					}
#endif
				}//cnt > 0 , check buf[0]==0xa7, buf[1]=0x10
			}else{//cnt < 0
				msleep(10);
			}
		}//check bte_fp
		input_sync(bte_dev);
		msleep(10);
	}//while
	printk("bte_thread stop\n");
	return 0;
}

