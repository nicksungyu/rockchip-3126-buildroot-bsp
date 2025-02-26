#include <linux/fs.h>
#include <linux/syscalls.h>	
#include <linux/ioctl.h>

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/tty.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <asm/termios.h>
#include <asm/ioctls.h>
#include <linux/serial.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/keyboard.h>

//a40i_vol_proc
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched/rt.h>
static struct input_dev *bte_dev = NULL;

#define BTE_UART_KB_TTY "/dev/ttyS1"
#define VOL_BAR 1
static struct tty_struct *local_tty = NULL;
struct file *bte_fp = NULL;
static struct task_struct *bte_tsk = NULL;
static struct task_struct *bte_sleep_tsk = NULL;

int init_uart(char* dev, struct tty_struct **tty, struct file **fp);

u8 bte_timeout = 0;
int bte_mcu_status = 9;
u8 key_val = 0; //Volume Switch
u8 key_val_org = 0;
u8 proc_flag = 1;
//extern int sign_unlock;
//gametype
int bte_gametype = -1;

u8 bte_loop=0;
u8 bte_jsx = 0x80, bte_jsy = 0x80;

static u8 p1_start = 0;
static u8 p2_start = 0;
static u8 p1_a = 0;
static u8 p2_b = 0;
//static u8 p2_kp2 = 0;
static u8 testmode_bootup = 0;
static int g_power_off = 0;
static int power_button_enable = 0;
static int power_keyevent_enable = 0; // enable by menu program when Test mode on.
static int major = -1;
static struct class *myclass = NULL;
static struct cdev mycdev;

static long drv_ioctl(struct file* filp, unsigned int cmd, unsigned long arg) {
	unsigned int *data = (unsigned int*)arg;
	printk("%s: %d, data: %d\n", __func__, cmd, *data);
	switch(cmd) {
	case 0: {
		printk("Power button enable function :%d\n", *data);
		power_button_enable = *data;
		break;
	}
	case 1: {
		printk("Power key event enable function :%d\n", *data);
		power_keyevent_enable = *data;
		break;
	}
	default: 
		break;
	}
	return 0;
}

static const struct file_operations drv_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = drv_ioctl
};

static void cleanup(int device_created){
	if(device_created) {
		device_destroy(myclass, major);
		cdev_del(&mycdev);
	} 
	if(myclass)
		class_destroy(myclass);
	if(major != -1)
		unregister_chrdev_region(major, 1);
}

static int ioctl_init(void){
	int device_created = 0;
	// cat /proc/devices 
	if(alloc_chrdev_region(&major, 0, 1, "ioctl_proc") < 0)
		goto error;
	// ls /sys/class 
	if((myclass = class_create(THIS_MODULE, "ioctl_sys")) == NULL)
		goto error;
	// ls /dev/ 
	if((device_create(myclass, NULL, major, NULL, "ioctl_dev")) == NULL)
		goto error;

	device_created = 1;
	cdev_init(&mycdev, &drv_fops);
	if( cdev_add(&mycdev, major, 1) == -1 )
		goto error;
	return 0;

error:
	printk("ioctl_error!\n");
	cleanup(device_created);
	return -1;
}

static void ioctl_exit(void){
	printk("ioctl_exit!\n");
	cleanup(1);
}

/*a40i_vol_proc*/
static int a40i_vol_proc_show(struct seq_file *m, void *v) {
	proc_flag = 0;
	if(bte_mcu_status == 9)
		seq_printf(m, "%d\n", bte_mcu_status);
	else
		seq_printf(m, "%d\n", key_val);
	return 0;
}
static int a40i_vol_proc_open(struct inode *inode, struct  file *file) {
	return single_open(file, a40i_vol_proc_show, NULL);
}
static const struct file_operations a40i_vol_proc_fops = {
	.owner = THIS_MODULE,
	.open = a40i_vol_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

ssize_t reboot_write_proc(struct file *flip, const char* buf, size_t count, loff_t *offp){
        char msg[128];
        int ret_ignore = 0;
        printk("reboot proc\n");
        memset(msg, '\0', sizeof(msg));
        ret_ignore = copy_from_user(msg, buf, count);
        g_power_off = 2;
        return count;
}

struct file_operations reboot_fops = {
        write: reboot_write_proc
};

#define TESTGAMETYPE 1
#if TESTGAMETYPE
ssize_t testgametype_write_proc(struct file *flip, const char* buf, size_t count, loff_t *offp){
	char msg[128];
	int ret_ignore = 0;
	memset(msg, '\0', sizeof(msg));
	ret_ignore = copy_from_user(msg, buf, count);
	bte_gametype = simple_strtol(msg, NULL, 10);
//	proc_flag = 1;
	return count;
}

struct file_operations testgametype_fops = {
	write: testgametype_write_proc
};
#endif


#define TEST 1
#if TEST
ssize_t test_write_proc(struct file *flip, const char* buf, size_t count, loff_t *offp){
	char msg[128];
	int ret_ignore = 0;
	memset(msg, '\0', sizeof(msg));
	ret_ignore = copy_from_user(msg, buf, count);
	key_val = simple_strtol(msg, NULL, 10);
	proc_flag = 1;
	return count;
}

struct file_operations test_fops = {
	write: test_write_proc
};
#endif

ssize_t unlock_write_proc(struct file *flip, const char* buf, size_t count, loff_t *offp){
	int ret;
	unsigned long long res;
	ret = kstrtoull_from_user(buf, count, 10, &res);
	if(ret){
		return ret;
	}else{
		printk("unlock got %llu\n", res);
		//sign_unlock = res;
		*offp = count;
		return count;
	}
}

struct file_operations unlock_fops = {
	write: unlock_write_proc
};

#define TESTMODE 1
#if TESTMODE
static int a40i_testmode_proc_show(struct seq_file *m, void *v) {
	int tmp = 0;
	if(testmode_bootup == 1 && key_val == 1 && p1_start == 1 && p2_start == 1 && p1_a == 1 && p2_b == 0){ // factory mode 
		tmp = 1;
	} else if(testmode_bootup == 1 && key_val == 1 && p1_start == 1 && p2_start == 1 && p1_a == 0 && p2_b == 1) { // wifi mode
		tmp = 2;
	} else if(testmode_bootup == 1 && key_val == 1 && p1_start == 1 && p2_start == 1 && p1_a == 1 && p2_b == 1) { // headset mode
		tmp = 6;
	} // else if
	seq_printf(m, "%d\n", tmp);
	return 0;
}
static int a40i_testmode_proc_open(struct inode *inode, struct  file *file) {
	return single_open(file, a40i_testmode_proc_show, NULL);
}
static const struct file_operations a40i_testmode_proc_fops = {
	.owner = THIS_MODULE,
	.open = a40i_testmode_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static inline void bte_report_key(uint8_t value, int key){
	input_report_key(bte_dev, key, (value > 0) ? 1:0);
}

static inline void bte_report_key2A(uint8_t index, uint8_t value, uint8_t mask, int key){
        static uint8_t buf[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	uint8_t temp = (value & mask);
	bool send_event = true;
        if((buf[index] & mask) != (temp) && key != 0){
		if ( key == KEY_A || key == KEY_Q ) {
			if(!power_keyevent_enable)
				send_event = false;
		} // if
	
		if ( send_event ){
                	input_report_key(bte_dev, key, (temp > 0) ? 1:0);
                	if(temp){
                        	buf[index] |= mask;
                	}else{
                        	buf[index] &= ~mask;
                	}
		}
        }
}

static inline void bte_report_key2B(uint8_t index, uint8_t value, uint8_t mask, int key){
        static uint8_t buf[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	uint8_t temp = (value & mask);
        if((buf[index] & mask) != (temp)){
                input_report_key(bte_dev, key, (temp > 0) ? 1:0);
                if(temp){
                        buf[index] |= mask;
                }else{
                        buf[index] &= ~mask;
                }
        }
}

#include <linux/kmod.h>
static inline int run_shell(char* cmd){
	char* argv[6], *envp[6];
	int result = -1;
	argv[0] = "/bin/bash";
	argv[1] = "-c";
	argv[2] = cmd;
	argv[3] = NULL;
	envp[0] = "HOME=/";
	envp[1] = "TERM=linux";
	envp[2] = "PATH=/sbin:/usr/sbin:/bin:/usr/bin";
	envp[3] = "NULL";
	result = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC); //do not use UMH_KILLABLE
	return result;
}

static inline int run_power_off(void){
	int result=-1;
	char* argv1[] = { "/usr/bin/killall", "-15", "ash", NULL};
	char* argv2[] = { "/usr/bin/killall", "-9", "OnlineHub", NULL};
	char* argv3[] = { "/usr/bin/kill_game.sh", NULL};
	char* argv4[] = { "/usr/bin/killall", "-9", "menu", NULL};
	char* argv5[] = { "/usr/bin/killall", "-9", "hp_spk_switch", NULL};
	char* argv5_1[] = { "/usr/bin/hp_spk_mute", NULL};
	char* argv6[] = { "/usr/bin/killall", "-15", "Downloader", NULL};
	char* argv7[] = { "/bin/umount", "/game/docs", NULL};
	char* argv8[] = { "/bin/sleep", "3", NULL};
	char* argv9[] = { "/usr/bin/show_power_off.sh", NULL};
	char* argv10[] = { "/bin/sleep", "3", NULL};
	char* envp[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL};
	result = call_usermodehelper(argv1[0], argv1, envp, UMH_WAIT_PROC);
	result = call_usermodehelper(argv2[0], argv2, envp, UMH_WAIT_PROC);
	result = call_usermodehelper(argv3[0], argv3, envp, UMH_WAIT_PROC);
	result = call_usermodehelper(argv4[0], argv4, envp, UMH_WAIT_PROC);
	result = call_usermodehelper(argv5[0], argv5, envp, UMH_WAIT_PROC);
	result = call_usermodehelper(argv5_1[0], argv5_1, envp, UMH_WAIT_PROC);
	result = call_usermodehelper(argv6[0], argv6, envp, UMH_WAIT_PROC);
	result = call_usermodehelper(argv7[0], argv7, envp, UMH_WAIT_PROC);
	result = call_usermodehelper(argv8[0], argv8, envp, UMH_WAIT_PROC);
	result = call_usermodehelper(argv9[0], argv9, envp, UMH_WAIT_PROC);
	result = call_usermodehelper(argv10[0], argv10, envp, UMH_WAIT_PROC);
	return result;
}

static int bte_sleep_thread(void* arg){
	printk("bte_sleep_thread starting\n");
	while(!kthread_should_stop()){
		run_shell("/bin/sync;");
		run_power_off();
		msleep(1000);
		run_shell("/bin/sync;/bin/sync");
		g_power_off = 1;
		break;
	} // while

	printk("bte_sleep_thread exit\n");
	return 0;
} 


#if defined GAME_PACMANIA
	#include "bte-pacmania.c"
#else
	printk("Missing bte driver file\n");
#endif

long tty_ioctl(struct file *f, unsigned op, unsigned long param){
	if (f->f_op->unlocked_ioctl) {
		return f->f_op->unlocked_ioctl(f, op, param);
	}
	return 0;
}

int init_uart(char* dev, struct tty_struct **tty, struct file **fp){
	struct termios newtio;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	*fp = filp_open(dev, O_RDWR | O_NOCTTY, 0);
	if( IS_ERR(*fp) ){
		printk("Can not open serial port\n");
		return -1;
	}else printk("init_uart fp=[%p]\n", *fp);

	tty_ioctl(*fp, TCGETS, (unsigned long)&newtio);

	newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = 0;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 0; //inter-character timer unused
	newtio.c_cc[VMIN]  = 0; //0.5 seconds read timeout
	*tty = (struct tty_struct *)((struct file*)(*fp)->private_data);

	tty_ioctl(*fp, TCSETS, (unsigned long)&newtio);

	set_fs(oldfs);
	return 0;
}

int init_kb(void){
	int err = 0, i = 0;

	bte_dev = input_allocate_device();
	if(!bte_dev){
		printk("bte: not enought memory for input device\n");
		err = -ENOMEM;
		return err;
	}
	bte_dev->name = "bte-key";
	bte_dev->phys = "bte/input0";
	bte_dev->id.bustype = BUS_HOST;
	bte_dev->id.vendor = 0x0001;
	bte_dev->id.product = 0x0001;
	bte_dev->id.version = 0x0100;

	bte_dev->evbit[0] = (BIT_MASK(EV_KEY) | BIT_MASK(EV_MSC));
	bte_dev->mscbit[0] = (BIT_MASK(MSC_SCAN));

	for(i = 1; i <= 248; i++){ //you could check uapi/linux/input.h
		input_set_capability(bte_dev, EV_KEY, i);
	}

        err = input_register_device(bte_dev);
	if(err){
		input_free_device(bte_dev);
                printk("register bte-key failed\n");
                return -1;
        }

	return 0;
}

static int __init bte_uart_kb_init(void){
	int ret = 0;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	printk("bte_uart_kb_init %s\n", BTE_UART_KB_TTY);

	ret = init_uart(BTE_UART_KB_TTY, &local_tty, &bte_fp);
	if(ret < 0){
		printk("Can not initialize %s device\n", BTE_UART_KB_TTY);
		return 0;
	}else{
		printk("init [%s] uart successed[%p]\n", BTE_UART_KB_TTY, bte_fp);
	}

	ret = init_kb();
	if(ret < 0){
		printk("Can not create bte keyboard\n");
		return 0;
	}else{
		printk("init bte keyboard successed\n");
	}

	ret = ioctl_init();
	if(ret < 0){
		printk("ioctl init failed!\n");
		return 0;
	} else {
		printk("ioctl init successed!\n");
	} 

	bte_tsk = kthread_create(bte_thread, NULL, "bte_thread");
	sched_setscheduler(bte_tsk, SCHED_FIFO, &param);
	wake_up_process(bte_tsk);

	printk("create a40i_vol_proc\n");
	proc_create("a40i_vol_proc", 0, NULL, &a40i_vol_proc_fops);/*a40i_vol_proc create*/
	proc_create("a40i_reboot", 0, NULL, &reboot_fops);
#if TESTGAMETYPE
	printk("create a40i_gametype\n");
	proc_create("a40i_gametype", 0, NULL, &testgametype_fops);
#endif

#if TEST
	proc_create("a40i_vol_test", 0, NULL, &test_fops);
#endif

#if TESTMODE
	proc_create("a40i_testmode", 0, NULL, &a40i_testmode_proc_fops);
#endif
	proc_create("unlock", 0, NULL, &unlock_fops);
	return 0;
}

static void __exit bte_uart_kb_exit(void){
	kthread_stop(bte_tsk);

	ioctl_exit();
	if(bte_fp)
		filp_close(bte_fp, NULL);
	input_unregister_device(bte_dev);
	input_free_device(bte_dev);
#if TESTGAMETYPE
	remove_proc_entry("a40i_gametype", NULL);
#endif
#if TEST
	remove_proc_entry("a40i_vol_test", NULL);
#endif
#if TESTMODE
	remove_proc_entry("a40i_testmode", NULL);
#endif
	remove_proc_entry("unlock", NULL);
	printk("bte_uart_kb_exit\n");
	return;
}

module_init(bte_uart_kb_init);
module_exit(bte_uart_kb_exit);
MODULE_AUTHOR("<stanley@bte.com.tw");
MODULE_DESCRIPTION("BTE UART Keyboard Driver");
MODULE_LICENSE("GPL");
