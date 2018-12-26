#include <linux/module.h>
#include <linux/usb.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/rtc.h>
#include <asm/segment.h>
#include <asm/uaccess.h>


#define BUF_SIZE 4096


static int read_statistic(char* manufacturer, char* product, char* serial)
{	
	char buf[BUF_SIZE] = {'\0'};
	
	int connection_count = 0;
	
	struct file* f = filp_open("var/log/statistic.log", O_RDONLY, 0);
	if(IS_ERR(f))
	{
		return connection_count;
	}

	int ret = kernel_read(f, &f->f_pos, buf, BUF_SIZE);

	filp_close(f, NULL);
	printk("\n>USB STAT KERNEL MODULE< : Stat file readed.");
	
	if (ret == 0) {
		return 0;
	}

	char* buf_string = buf;		
	while(buf_string != NULL)
	{
		char* line = strsep(&buf_string, "\n");

		if (line != NULL && strlen(line) >= 1)
		{
			char* token_ccount = strsep(&line, "\t");
			char* token_manuf = strsep(&line, "\t");
			char* token_product = strsep(&line, "\t");
			char* token_serial = strsep(&line, "\t");
			
			if (strcmp(token_manuf, manufacturer) == 0 && \
				strcmp(token_product, product) == 0 && \
				strcmp(token_serial, serial) == 0) {
			
				sscanf(token_ccount, "%d", &connection_count);
			}
		}
	}
	printk("\n>USB STAT KERNEL MODULE< : Stat file handled.");
	
	return connection_count;
}

static void write_statistic(char* manufacturer, char* product, char* serial, int connection_count)
{
	struct file* f = filp_open("var/log/statistic.log", O_APPEND | O_CREAT | O_WRONLY, 0);

	if (IS_ERR(f))
	{
		return;
	}
	
	struct timeval time;
	do_gettimeofday(&time);
	
	unsigned long local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
	
	struct rtc_time tm;
	rtc_time_to_tm(local_time, &tm);

	char new_sl[1024];

	sprintf(new_sl, "%d\t%s\t%s\t%s\t%04d-%02d-%02d %02d:%02d:%02d\n",
		connection_count, manufacturer, product, serial, tm.tm_year + 1900,
		tm.tm_mon + 1, tm.tm_mday, tm.tm_hour+3, tm.tm_min, tm.tm_sec);
		
	kernel_write(f, new_sl, strlen(new_sl), &f->f_pos);

	filp_close(f, NULL);
	
	printk("\n>USB STAT KERNEL MODULE< : Stat file writed with new line.");
}

static int probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	printk("\n>USB STAT KERNEL MODULE< : Detect new connected usb.");

	char* manufacturer;
	if (dev->manufacturer == NULL)
		manufacturer = "no data";
	else
		manufacturer = dev->manufacturer;
	char* product;
	if (dev->product == NULL)
		product = "no data";
	else
		product = dev->product;
	char* serial;
	if (dev->serial == NULL)
		serial = "no data";
	else
		serial = dev->serial;

	int connection_count = read_statistic(manufacturer, product, serial) + 1;

	write_statistic(manufacturer, product, serial, connection_count);

	return -1;
}
static void disconnect(struct usb_interface *intf)
{
	printk(">USB STAT KERNEL MODULE< : Some usb disconnected.");
}

static int driver_suspend(struct usb_interface *intf, pm_message_t message)
{ return 0; }

static int driver_resume(struct usb_interface *intf)
{ return 0; }

static int driver_pre_reset(struct usb_interface *intf)
{ return 0; }

static int driver_post_reset(struct usb_interface *intf)
{ return 0; }

static struct usb_device_id empty_usb_table[] = {
 {.driver_info = 1},
 { }
};

MODULE_DEVICE_TABLE(usb, empty_usb_table);
MODULE_LICENSE("GPL");

static struct usb_driver statistic_usb_driver = 
{
	.name = "usb_stat_km",
	.probe = probe,
	.disconnect = disconnect,
	.suspend = driver_suspend,
	.resume = driver_resume,
	.pre_reset = driver_pre_reset,
	.post_reset = driver_post_reset,
	.id_table = empty_usb_table,
	.supports_autosuspend = 1,
	.soft_unbind = 1,
};

static int my_init_module(void)
{
	int err;
	printk(">USB STAT KERNEL MODULE< : Stat grabber activated.\n");
	err = usb_register(&statistic_usb_driver);
	return err;
}

static void my_cleanup_module(void)
{
	printk(">USB STAT KERNEL MODULE< : Stat grabber deactivated.\n");
	usb_deregister(&statistic_usb_driver);
}

module_init(my_init_module);
module_exit(my_cleanup_module);
			

