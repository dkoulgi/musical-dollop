#include <linux/ioctl.h>
#include"ht_object.h"

#define MAGIC_NUMBER 				'K'
#define HT_IOCTL_BASE				0x00



#define	HT_438_READ_KEY							_IOWR(MAGIC_NUMBER, HT_IOCTL_BASE+1, int /*struct _HT_438_READ_KEY*/)
#define DUMP_IOCTL								_IOWR(MAGIC_NUMBER, HT_IOCTL_BASE+2, struct dump_arg)


//DEFINITIONS FOR IOCTL

/*typedef struct _HT_438_READ_KEY
{
	int RetVal;
	struct
	{
		int key;
	}in;
}HT_438_READ_KEY, *PHT_438_READ_KEY;
*/

typedef struct dump_arg
{
	int retval;
	struct
	{
		int n;
	}in;
	struct
	{
		ht_object_t object_array[8];
	}out;
} dump_arg, *dump_argp;
