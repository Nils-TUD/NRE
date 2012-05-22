#include <signal.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc,char *argv[]) {
	sigset_t sigs;

	sigemptyset(&sigs);
	sigaddset(&sigs,SIGINT);
	sigprocmask(SIG_BLOCK,&sigs,0);

	if(argc > 1) {
		execvp(argv[1],argv + 1);
		perror("execv");
	}
	else {
		fprintf(stderr,"Usage: %s <command> [args...]\n",argv[0]);
	}
	return 1;
}
