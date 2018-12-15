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

static int read_statistic(char* manufacturer, char* product, char* serial)
{
	struct file *f;
	char buf[4096];
	
	int i;
	for (i=0; i<4096; i++)
		buf[i] = 0;
	
	f = filp_open("/var/log/statistic.log", O_RDONLY, 0);
	char* connection_count_char = "0";
	
	if(!IS_ERR(f))
	{
		mm_segment_t fs;
		fs = get_fs();
		set_fs(get_ds());
		int ret = vfs_read(f, buf, 4096, &f->f_pos);
		set_fs(fs);
		filp_close(f, NULL);
		printk("\n---///USB_KERNEL_MODULE///--- Stat file readed:\n\t%s", buf);
		
		char* buf_string = kmalloc(strlen(buf)*sizeof(char), GFP_KERNEL);
		strcpy(buf_string, buf);
		
		char* line;
		while(buf_string != NULL)
		{
			line = strsep(&buf_string, "\n");

			if (line != NULL && strlen(line) >= 1)
			{
				char* token_ccount = strsep(&line, "\t");
				char* token_manuf = strsep(&line, "\t");
				char* token_product = strsep(&line, "\t");
				char* token_serial = strsep(&line, "\t");
				
				if (strcmp(token_product, product) == 0 && strcmp(token_serial, serial) == 0)
					connection_count_char = token_ccount;
			}
		}
		printk("\n---///USB_KERNEL_MODULE///---Stat file handled.");
		set_fs(fs);
	}
	char* connection_count_char_term = kmalloc((strlen(connection_count_char)+1)*sizeof(char), GFP_KERNEL);
	strcpy(connection_count_char_term, connection_count_char);
	strcat(connection_count_char_term, "\0");
	int connection_count;
	sscanf(connection_count_char_term, "%d", &connection_count);
	connection_count++;


	return connection_count;
}

static void write_statistic(char* manufacturer, char* product, char* serial, int connection_count)
{
	struct file *f; 

	f = filp_open("/var/log/statistic.log", O_APPEND | O_CREAT | O_WRONLY, 0);

	if (!IS_ERR(f))
	{	
		//struct tineval now;
		//struct tm tm_val;
		//do_gettimeofday(&now);
		//time_to_tm(now.tv_sec, 0, &tm_val);
		struct timeval time;
		unsigned long local_time;
		struct rtc_time tm;

		do_gettimeofday(&time);
		local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
		rtc_time_to_tm(local_time, &tm);
	
		char new_sl[1024];

		sprintf(new_sl, "%d\t%s\t%s\t%s\t%04d-%02d-%02d %02d:%02d:%02d\n", connection_count, manufacturer, product, serial, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		mm_segment_t fs_w;
		fs_w = get_fs();
		set_fs(get_ds());
		int ret = vfs_write(f, new_sl, strlen(new_sl), &f->f_pos);
		set_fs(fs_w);
		filp_close(f, NULL);
		printk("\n---///USB_KERNEL_MODULE///--- Stat file writed with new line.");

		set_fs(fs_w);
	}

	return 0;
}

static int probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	printk("\n---///USB_KERNEL_MODULE///--- Some usb connected");

	//No data handle
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

	int connection_count = read_statistic(manufacturer, product, serial);

	write_statistic(manufacturer, product, serial, connection_count);

	return 0;
}

static void disconnect(struct usb_interface *intf)
{
	struct usb_device *device = interface_to_usbdev(intf);
	printk("---///USB_KERNEL_MODULE///--- Some usb disconnected.");
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
 {.driver_info = 42},
 { }
};

MODULE_DEVICE_TABLE(usb, empty_usb_table);
MODULE_LICENSE("GPL");

static struct usb_driver statistic_usb_driver = 
{
	.name = "usb_kernel_module",
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
	printk("Stat grabber activated.\n");
	err = usb_register(&statistic_usb_driver);
	return err;
}

static void my_cleanup_module(void)
{
	printk("Stat grabber deactivated.\n");
	usb_deregister(&statistic_usb_driver);
}

module_init(my_init_module);
module_exit(my_cleanup_module);
			

