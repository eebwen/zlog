/*
 * test.c
 *
 *  Created on: 2015��3��5��
 *      Author: Administrator
 */


#include "../ZLog.h"
#include <stdio.h>

int main() {
	int ret = zlog_open("./aaa.log", "a");
	zlog_set_echo(0);
//	const char* level = zlog_set_level(ZLOG_FATAL);
//	const char* level = zlog_set_strlevel("INFO");
	const char* level = zlog_set_strlevel("DEBUG");
	printf("level = %s\n", level);
	if (ret) {
		printf("open log filed\n");
	}

	while(1) {
		LOGA("hello %s", "world");
		LOGD("hello %s", "world");
		LOGI("hello %s", "world");
		LOGW("hello %s", "world");
		LOGE("hello %s", "world");
		LOGF("hello %s", "world");
	}
	return 0;
}

