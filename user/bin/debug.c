#include "user.h"

static char str[] = "hello, ELF";

// debug 系统调用会打印信息，
// 应该大概屏幕上会持续打印 debug 信息
// 确保用户程序一直在执行
int main()
{
	str[0] = 'A';
	int pid = fork();
	if (pid == 0) {
		while (1) {
			sleep(500);
			debug(0, 0, str, 0, 0, 0);
			str[0] = 'B';
			exec("/cat", NULL);
		}
	} else {
		while (1) {
			sleep(2000);
			getpid();

			str[0] = 'A';
		}
	}
}