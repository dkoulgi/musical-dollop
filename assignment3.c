/*---------------------
* CSE438 Assignment3
* Dhruv Koulgi
* assignment3.c
----------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "ht438_ioctl.h"
#include <fcntl.h>

#define NUM_THREADS 2		//two threads for hashtable devices
#define BUFFER_SIZE 1024	//buffer size for input
#define PRIO 90			//initial priority

int set_retval;
int key;
char data[5] = {'a','b','a','b','\0'};
pthread_mutex_t w_mux = PTHREAD_MUTEX_INITIALIZER;	//mutex for data write

int ioctl_dump_args(int fd, int n, ht_object_t object_array[8])
{
	dump_arg dump;
	dump.in.n = n;
	for (int i = 0; i < 8; i++){				//dump at most 8 objects
		dump.out.object_array[i] = object_array[i];
	}
	set_retval = ioctl(fd, DUMP_IOCTL, (unsigned long)&dump);

	if (set_retval) {
		printf("ERR: ht438_dev was unable to perform dump.\n");
	}

	return dump.retval;
}

int read_key(int fd, int key)
{
	if ((set_retval = ioctl(fd, HT_438_READ_KEY, (unsigned long)&key)) != 0) {
		printf("ERR: ioctl error %d\n", set_retval);
	}
	else if (set_retval) {
		printf("ERR: ht438_dev was unable to perform read_key \n");
	}
	return 0;
}


void* tester(void* fd)
{
	int res;
	int set;

	int buf;
	int fptr = *(int*)fd;

	ht_object_tp object;
	object = (ht_object_tp)malloc(sizeof(ht_object_t));
	memset(object, 0, sizeof(ht_object_t));			//set mem of object

	object->key = key;					//set key
	strcpy(object->data, data);				//set data
	pthread_mutex_lock(&w_mux);
	sleep(1);
	res = write(fptr, object, sizeof(ht_object_t));		//driver write
	if (res < 0){
		printf("Failed to write data.\n");
	}

	pthread_mutex_unlock(&w_mux);
	sleep(1);

	set = read_key(fptr, object->key);      		//call to give input key to the read function
	if ((set = read(fptr, &buf, sizeof(int))) == -1){	//driver read
		printf("Failed to read data.\n");
	}
	else{
		printf("Data from hashtable =  %d\n ", buf);
	}

	pthread_exit(0);
}


int main(int argc, char** argv)
{
	int fd, res;
	ht_object_t object_array[8];
	int i = 0;
//	char buffer1[BUFFER_SIZE];
//	char buffer2[BUFFER_SIZE];
//	char * token;
//	char **cmd;

	fd = open("/dev/ht438_dev", O_RDWR);

/*	TOKENIZE FILES
	if (argc < 3) {
		printf("Please provide two test files.");
		return -1;
	}

	FILE *t1 = fopen(argv[1], "r");
	if (!t1) {
		printf("Could not open %s.\n", argv[1]);
		return 0;
	}
	else {
		printf("Opened %s.\n", argv[1]);
	}

	FILE *t2 = fopen(argv[2], "r");
	if (!t2) {
		printf("Could not open %s.\n", argv[2]);
		return 0;
	}
	else {
		printf("Opened %s.\n", argv[2]);
	}

	if (fd < 0) {
		printf("Could not open device file.\n");
		return 0;
	}
	else {
		printf("ht438 device is open.\n");
	}

	// READ FILE LINES
	while(fgets(buffer1, BUFFER_SIZE, t1) != NULL) {
		token = strtok(buffer1, " ");
		while(token != NULL) {
			if(strcmp(token, "w") == 0){
				printf("Writing data...\n");
			}
			if(strcmp(token, "r") == 0){
				printf("Reading data...\n");
			}
			if(strcmp(token, "s") == 0){
				printf("Sleeping...\n");
				sleep(5);
			}
			if(strcmp(token, "d") == 0){
				printf("Dumping...\n");
			}
			token = strtok(NULL, " ");
		}
		sleep(1);
	}

	while(fgets(buffer2, BUFFER_SIZE, t2) != NULL) {
		token = strtok(buffer2, " ");
		while(token != NULL) {
			if(strcmp(token, "w") == 0){
				printf("Writing data...\n");
			}
			if(strcmp(token, "r") == 0){
				printf("Reading data...\n");
			}
			if(strcmp(token, "s") == 0){
				printf("Sleeping...\n");
				sleep(5);
			}
			if(strcmp(token, "d") == 0){
				printf("Dumping...\n");
			}
			token = strtok(NULL, " ");
		}
		sleep(1);
	}

*/

	//PTHREAD INITIALIZATION
	pthread_t tid[NUM_THREADS];
	struct sched_param param[NUM_THREADS];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	for (i = 0; i < NUM_THREADS; i++){
		param[i].sched_priority = PRIO;
		pthread_attr_setschedparam(&attr, &param[i]);
		pthread_create(&tid[i], &attr, tester, (void*)&fd);
	}

	sleep(5);

	for (i = 0; i < NUM_THREADS; i++) {
		pthread_join(tid[i], NULL);
	}

	for (i = 0; i < 128; i++) {
		res = ioctl_dump_args(fd, i, object_array);
	}

	if (res < 0) {
		printf("Dump failed\n");
	}

	/* close devices */
	close(fd);
//	fclose(t1);
//	fclose(t2);
	return 0;
}
