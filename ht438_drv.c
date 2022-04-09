/* Basic driver example to show skelton methods for several file operations.
 references: http://lxr.free-electrons.com/source/tools/include/linux/list.h#L713
 ----------------------------------------------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/hashtable.h>
#include "ht438_ioctl.h"

#define DEVICE_NAME                 "ht438_dev"  // device names of the hashtables

int retval_set;
int key;
int errno;

typedef struct ht_object_node {		//node structure of hashed objects

	ht_object_t new_ht_object;
	struct hlist_node next;		//next node in hash

} ht_object_node, *ht_object_nodep;

/* per device structure */
struct ht438_dev {

	struct cdev cdev;               //cdev structure
	char name[20];                  //device name
	ht_object_nodep new_ht_nodep;

} *ht438_devp;

static dev_t ht_dev_number;      		//Allotted device number
struct class* ht_dev_class;          		//Tie with the device model
static struct device* ht_dev_device;

static char* user_name = "DK";

module_param(user_name, charp, 0000);			//to get parameter from load.sh script to greet the user

DEFINE_HASHTABLE(ht438_tbl, 5);

/*
* Open ht438 driver
*/
int ht_driver_open(struct inode* inode, struct file* file)
{
	struct ht438_dev* ht438_devp;

	/* Get the per-device structure that contains this cdev */
	ht438_devp = container_of(inode->i_cdev, struct ht438_dev, cdev);


	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = ht438_devp;
	printk("\n%s is opening \n", ht438_devp->name);

	return 0;
}

/*
 * Write to ht438 driver
 */
ssize_t ht_driver_write(struct file* file, const char* buf,
	size_t count, loff_t* ppos)
{

	int kbuf;
	int flag_one = 0;
	int flag_two = 0;
	struct ht438_dev* ht438_devp = file->private_data;
	ht_object_nodep temp = NULL;
	ht_object_nodep nodep = NULL;
	ht_object_nodep nodepp = NULL;
	if (!(nodepp = kmalloc(sizeof(struct ht_object_node), GFP_KERNEL)))
	{
		printk("Bad Kmalloc\n");
		return -ENOMEM;
	}
	memset(nodepp, 0, sizeof(ht_object_node));

	if (copy_from_user(&nodepp->new_ht_object, buf, count))
		return -EFAULT;


	ht438_devp->new_ht_nodep = nodepp;
	nodep = ht438_devp->new_ht_nodep;

	printk("Writing DATA= %s to KEY = %d\n", nodep->new_ht_object.data, nodep->new_ht_object.key);
	if (hash_empty(ht438_tbl)){
		hash_add(ht438_tbl, &nodep->next, nodep->new_ht_object.key);
		flag_two = 1;
	}
	else
	{
		hash_for_each(ht438_tbl, kbuf, temp, next) {
			if ((temp->new_ht_object.key == nodep->new_ht_object.key) && (nodep->new_ht_object.data != NULL)) {
				hash_del(&temp->next);
				flag_one = 1;
				hash_add(ht438_tbl, &nodep->next, nodep->new_ht_object.key);
			}
			else if ((nodep->new_ht_object.data == NULL) && (temp->new_ht_object.key == nodep->new_ht_object.key)) {
				hash_del(&temp->next);
				flag_one = 1;
			}
		}
	}
	if ((flag_one == 0) && (flag_two == 0) && (nodep->new_ht_object.data != NULL)){
		hash_add(ht438_tbl, &nodep->next, nodep->new_ht_object.key);
	}

	return 0;
}

/*
 * Read to ht438 driver
 */
ssize_t ht_driver_read(struct file* file, char* buf,
	size_t count, loff_t* ppos)   //assumimg we know the key that is stored in read_key (a global variable)
{
	int kbuf;
	int dataSize = 0;
	char * dataRead = "";
	ht_object_nodep temp = NULL;
	hash_for_each(ht438_tbl, kbuf, temp, next) {
		if (temp->new_ht_object.key == key)
			dataRead = temp->new_ht_object.data;
	}

	if (dataRead != NULL) {
		dataSize = sizeof(char[4]);
		printk("\nReading DATA = %s from KEY = %d\n", dataRead, key);
		if (copy_to_user((int*)buf, (int*)&dataRead, dataSize))
		{
			printk("unable to copy to the user");
			return -EFAULT;
		}
	}

	else
	{
		printk("is invalid \n");
		errno = EINVAL;
		return -1;
	}

	return dataSize;

}

/*
 * ioctl for ht438 driver
 */
long ht_driver_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	int count = 0;
	ht_object_nodep temp = NULL;
	void* iobufp = NULL;

	if (!(iobufp = kmalloc(_IOC_SIZE(ioctl_num), GFP_KERNEL)))
	{
		printk("ht_dev_error: unable to allocate io buffer memory for ioctl \n");
		return -ENOMEM;
	}
	if (copy_from_user(iobufp, (unsigned long*)(ioctl_param), _IOC_SIZE(ioctl_num)))
	{
		printk("ht_dev_error: unable to copy data from user space\n");
		kfree(iobufp);
		return -ENOMEM;
	}

	switch (ioctl_num)
	{
	//read key from user
	case HT_438_READ_KEY:
		key = *((int*)iobufp);
		break;

	//dump 8 objects from the bucket
	case DUMP_IOCTL:
		printk("\n DUMP_IOCTL call\n");
		((dump_argp)iobufp)->retval = 0;
		hlist_for_each_entry(temp, &ht438_tbl[((dump_argp)iobufp)->in.n], next)
		{
			if (count < 8)
			{
				printk("BUCKET %d:   ioctl_dump KEY = %d   ioctl_dump DATA = %s\n", ((dump_argp)iobufp)->in.n, temp->new_ht_object.key, temp->new_ht_object.data);
				((dump_argp)iobufp)->out.object_array[count] = temp->new_ht_object;
				count++;
			}
		}
		((dump_argp)iobufp)->in.n = count;
		if (copy_to_user((unsigned long*)(ioctl_param), (unsigned long*)iobufp, _IOC_SIZE(ioctl_num)))
		{
			printk(" in dump_ioctl unable to copy data to user space\n");
			kfree(iobufp);

		}
		break;
		kfree(iobufp);
	}
	return 0;
}


/*
 * Release ht438 driver
 */
int ht_driver_release(struct inode* inode, struct file* file)
{
	struct ht438_dev* ht438_devp = file->private_data;
	printk("\n%s is closing\n", ht438_devp->name);
	return 0;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations ht_fops = {
	.owner = THIS_MODULE,           /* Owner */
	.open = ht_driver_open,        /* Open method */
	.release = ht_driver_release,     /* Release method */
	.write = ht_driver_write,       /* Write method */
	.read = ht_driver_read,        /* Read method */
    .unlocked_ioctl = ht_driver_ioctl,
};

/*
 * Driver Initialization
 */
int __init ht_driver_init(void)
{
	int ret;
	int time_since_boot;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&ht_dev_number, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "Can't register device 0\n"); return -1;
	}
	if (alloc_chrdev_region(&ht_dev_number, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "Can't register device 1\n"); return -1;
	}

	/* Populate sysfs entries */
	ht_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
	ht438_devp = kmalloc(sizeof(struct ht438_dev), GFP_KERNEL);

	if (!ht438_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	/* Request I/O region */
	sprintf(ht438_devp->name, DEVICE_NAME);

	/* Connect the file operations with the cdev */
	cdev_init(&ht438_devp->cdev, &ht_fops);
	ht438_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&ht438_devp->cdev, (ht_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	ht_dev_device = device_create(ht_dev_class, NULL, MKDEV(MAJOR(ht_dev_number), 0), NULL, DEVICE_NAME);
	time_since_boot = (jiffies - INITIAL_JIFFIES) / HZ;//since on some systems jiffies is a very huge uninitialized value at boot and saved.

	printk("ht438 driver initialized.\n");

	return 0;
}
/* Driver Exit */
void __exit ht_driver_exit(void)
{
	int kbuf;
	ht_object_nodep temp = NULL;
	hash_for_each(ht438_tbl, kbuf, temp, next) {
		hash_del(&temp->next);
	}
	unregister_chrdev_region((ht_dev_number), 1);

	/* Destroy device */
	device_destroy(ht_dev_class, MKDEV(MAJOR(ht_dev_number), 0));
	cdev_del(&ht438_devp->cdev);
	kfree(ht438_devp);

	/* Destroy driver_class */
	class_destroy(ht_dev_class);

	printk("ht438 driver removed.\n");
}

module_init(ht_driver_init);
module_exit(ht_driver_exit);
MODULE_LICENSE("GPL v2");
