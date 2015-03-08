/*
 * ZLog.c
 *
 *  Created on: 2015年3月5日
 *      Author: Administrator
 */


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_PRINT_LEN 		1024
#define MAX_FILE_NAME_LEN 	255

#define ZLOG_FATAL 		5	// 每个严重的错误事件将会导致应用程序的退出。

#define ZLOG_ERROR 		4  	// 指出虽然发生错误事件，但仍然不影响系统的继续运行。
#define ZLOG_WARNING 	3 	// 表明会出现潜在错误的情形。
#define ZLOG_INFO  		2 	// 消息在粗粒度级别上突出强调应用程序的运行过程。
#define ZLOG_DEBUG 		1 	// 指出细粒度信息事件对调试应用程序是非常有帮助的。

#define ZLOG_ALL 		0 	// 最低等级,打开所有日志记录


static const char *levels[] = {
		"ALL",
		"DEBUG",
		"INFO",
		"WARNING",
		"ERROR",
		"FATAL",
		NULL
};

static int writed_size 	= 0;
static int limit_size 	= 10 * 1024 * 1024; // 10M
static int backups_num  = 5;

static FILE* fp = NULL;
static char log_path[MAX_FILE_NAME_LEN];
static int current_level = 1;
static int flush = 1;
static int echo = 0;

void zlog_set_echo(int echo_) {
	echo = echo_;
		
}

void zlog_set_backupfile_num(int num) {
	if (num >= 1 && num <= 100) {
		backups_num = num;
	}
}

void zlog_set_limit_size(int size) { // MB 最大100MB, 最小 2 MB
	if (size >= 2 && size <= 100) {
		limit_size = size * 1024 * 1024;
	}
}

int zlog_open(const char* filepath, const char* mode) {
	if (fp || !filepath) {
		fprintf(fp, "(fp || !filepath) true\n");
		return -1;
	}

	fp = fopen(filepath, mode);
	if (!fp) {
		printf("open log file %s failed:%m, use echo\n");
		echo = 1;
		return -1;
	}

	struct stat stat;
	if (fstat(fileno(fp), &stat) == 0) {
		writed_size = stat.st_size;
	}

	strcpy(log_path, filepath);
	return 0;
}


void zlog_close() {
	if (fp) {
		fclose(fp);
		fp = NULL;
	}
}

const char* zlog_set_level(int level) {

	if (level >= ZLOG_ALL && level <= ZLOG_FATAL)
		current_level = level;

	if (current_level <= ZLOG_DEBUG)
		flush = 1;
	else
		flush = 0;

	return levels[current_level];
}

const char* zlog_set_strlevel(const char* level) {

	for (int i = 0; levels[i] != NULL; i++) {
		if (strcmp(level, levels[i]) == 0) {
			current_level = i;
			break;
		}
	}

	if (current_level <= ZLOG_DEBUG)
		flush = 1;
	else
		flush = 0;

	return levels[current_level];
}



void zlog(int level, const char* format, ...) {

	int str_len = 0;
	char str[MAX_PRINT_LEN]="";

	time_t t = time(NULL);
	struct tm tm;

	localtime_r(&t, &tm);
	sprintf(str, "[%d/%02d/%02d %02d:%02d:%02d][%s]",
			tm.tm_year + 1990, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
			levels[level]);

	va_list args;
	va_start(args, format);

	str_len = strlen(str);
	vsnprintf(str + strlen(str), MAX_PRINT_LEN - str_len - 1, format, args);

	va_end(args);

	if (echo)
		fprintf(stdout, "%s\n", str);

	if (!fp)
		return;

//	str_len = strlen(str);
//	str_len = fwrite(str, str_len + 1, 1, fp);
	str_len = fprintf(fp, "%s\n", str);

	if (current_level <= 1 /*debug*/) {
		fflush(fp);
	}

	if (str_len > 0) {
		writed_size += str_len;

		if (writed_size >= limit_size) {
			static int round = 1;
//			char backfile[300] = "";
			char* backfile = (char*)malloc(strlen(log_path) + 20);
			sprintf(backfile, "%s_%d", log_path, round);
			rename(log_path, backfile);
			fp = freopen(log_path, "w", fp);
			writed_size = 0;
			round = round % backups_num + 1;
			free(backfile);
		}
	}
}

void zlog2(const char* filename, int line, const char* funcname, int level, const char* format, ...) {

	if (level < current_level || (!fp && !echo))
		return ;

	int str_len = 0;
	char str[MAX_PRINT_LEN]="";

	time_t t = time(NULL);
	struct tm tm;

	// 1. time
	localtime_r(&t, &tm);
	sprintf(str, "[%d/%02d/%02d %02d:%02d:%02d][%s]",
			tm.tm_year + 1990, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
			levels[level]);

	// 2. 日志所在文件，及其位置
	if (current_level <= ZLOG_DEBUG) {
		str_len = strlen(str);
//		snprintf(str + str_len, MAX_PRINT_LEN - str_len - 1, "[FILE:%s, LINE:%d, FUNC:%s]",
		snprintf(str + str_len, MAX_PRINT_LEN - str_len - 1, "[%s(%s:%d)]",
				filename, funcname, line);
	}

	// 写入 空格
	str_len = strlen(str);
	snprintf(str + str_len, MAX_PRINT_LEN - str_len - 1, " ");

	// 3. 日志信息
	va_list args;
	va_start(args, format);

	str_len = strlen(str);
	vsnprintf(str + str_len, MAX_PRINT_LEN - str_len - 1, format, args);

	va_end(args);


	if (echo) {
		fprintf(stdout, "%s\n", str);
	}

	if (!fp)
		return;

	str_len = fprintf(fp, "%s\n", str);

	if (flush) {
		fflush(fp);
	}

	if (str_len > 0) {
		writed_size += str_len;

		if (writed_size >= limit_size) {
			static int round = 1;
			char backfile[MAX_FILE_NAME_LEN] = "";
			snprintf(backfile, MAX_FILE_NAME_LEN - 1, "%s_%d", log_path, round);
			rename(log_path, backfile);
			fp = freopen(log_path, "w", fp);
			writed_size = 0;
			round = round % backups_num + 1;
		}
	}
}

