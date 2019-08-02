/*
 * test_tw6874.c
 *
 *  Created on: 2016-12-9
 *      Author: eternal
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "tw6874_ioctl_cmd.h"

int main()
{
	int tw6874_1 = open("/dev/tw6874_driver_1", O_RDWR);
	int tw6874_2 = open("/dev/tw6874_driver_2", O_RDWR);
	unsigned char temp = 0;
	unsigned char i;
	if (tw6874_1 == -1)
	{
		printf("open tw6874 failed!please check Drivers\n");
		return -1;
	}
	if (tw6874_2 == -1)
	{
		printf("open tw6874 failed!please check Drivers\n");
		return -1;
	}

	for (i = 0; i < 250; i++)
	{
		temp = 0; //chn number
		ioctl(tw6874_1, CMD_CHECK_ADN_LOCK_SDI, &temp);
		printf("dev:%d chn:%d CMD_HANDLE_CHN_CHANGE ok,chn format: %d .\n", 1, 0, temp);
		//sleep(2);
		temp = 1; //chn number
		ioctl(tw6874_1, CMD_CHECK_ADN_LOCK_SDI, &temp);
		printf("dev:%d chn:%d CMD_HANDLE_CHN_CHANGE ok,chn format: %d .\n", 1, 1, temp);
		//sleep(2);
		temp = 2; //chn number
		ioctl(tw6874_1, CMD_CHECK_ADN_LOCK_SDI, &temp);
		printf("dev:%d chn:%d CMD_HANDLE_CHN_CHANGE ok,chn format: %d .\n", 1, 2, temp);
		//sleep(2);
		temp = 3; //chn number
		ioctl(tw6874_1, CMD_CHECK_ADN_LOCK_SDI, &temp);
		printf("dev:%d chn:%d CMD_HANDLE_CHN_CHANGE ok,chn format: %d .\n", 1, 3, temp);
		//sleep(2);

		temp = 0; //chn number
		ioctl(tw6874_2, CMD_CHECK_ADN_LOCK_SDI, &temp);
		printf("dev:%d chn:%d CMD_HANDLE_CHN_CHANGE ok,chn format: %d .\n", 2, 0, temp);
		//sleep(2);
		temp = 1; //chn number
		ioctl(tw6874_2, CMD_CHECK_ADN_LOCK_SDI, &temp);
		printf("dev:%d chn:%d CMD_HANDLE_CHN_CHANGE ok,chn format: %d .\n", 2, 1, temp);
	}

	close(tw6874_1);
	close(tw6874_2);
}
