#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SIGUSR3 SIGWINCH

int main()
{
    // initialize
    int P,Q;
    int R;
    int signal_id[R];
    //read the input
    scanf("%d %d",&P,&Q);
    scanf("%d",&R);
    for(int i = 0;i < R;i++)
    {
        scanf("%d",&signal_id[i]);
    }
    char P_string[1024],Q_string[1024] = {0};
    sprintf(P_string,"%d",P);
    sprintf(Q_string,"%d",Q);
    pid_t pid;
    if((pid = fork()) == 0)
    {
        execlp("./hw3","./hw3",P_string,Q_string,"3","0",NULL);
        exit(0);
    }
    /*for(int i = 0;i < R;i++)
    {
        sleep(5);
        kill(pid,signo); // send signal
        read(); // read ACK message
    }
    read() // read the content of arr*/
    waitpid(pid,NULL,0);// close pipe
    return 0;
}