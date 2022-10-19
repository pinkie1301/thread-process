/*
compile: gcc -o 1091433 1091433.c -lrt
exec: ./1091433
*/
#include <sys/shm.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>

struct SHARE{
    int pid_p, pid_c;//pid of parent and child
    int hit1, hit2;//cordinate of planted bomb
    int turn;//turn 0 => parent
             //turn 1 => child
    int bomb_c, bomb_p;//count how many bomb has been planted
    int win;//init => 0 | parent win => 1 | child win => 2
};

int nextby(int x1, int y1, int x2, int y2){
    //計算兩點座標是否相連
    if(x1 == x2){
        if(abs(y1-y2) == 1){
            return 1;
        }
    }
    else if(y1 == y2){
        if(abs(x1 - x2) == 1){
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int Pseed,Cseed,mode;
    int cor_1[2] = {}, cor_2[2] = {}, hit[2] = {};//cor => 船的位置| hit => 砲彈位置
    int p1_is_hit = 1, p2_is_hit = 1;//紀錄船身有無被轟炸過（共兩位置）, 1 is not hit, 0 is hit
    int count = 0;
    // scanf("%d %d %d",&Pseed,&Cseed,&mode);
    Pseed = atoi(argv[1]);
    Cseed = atoi(argv[2]);
    mode = atoi(argv[3]);
    printf(">prog1 %d %d %d\n", Pseed, Cseed, mode);
    pid_t pid;
    /*-----------------------------------*/
    
    //create share memory
    //映射記憶體
    //把一個 struct p share memory 裡面
    int fd = shm_open("posixsm", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 0x400000);
    struct SHARE *p = mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    p->turn = 0;
    p->bomb_p = 0;
    p->bomb_c = 0;
    p->win =  0;
    //fork
    pid = fork();
    if(pid < 0){
        printf("Fork error.\n");
        exit(1);
    }
    else if(pid == 0){//child
        p->pid_c = getpid();
        srand(Cseed);
        //use rand() to generate a boat
        while((cor_1[0] == cor_2[0]) && (cor_1[1] == cor_2[1]) || (!nextby(cor_1[0], cor_1[1], cor_2[0], cor_2[1]))){
            cor_1[0] = rand() % 4; cor_1[1] = rand() % 4;//first 船座標
            cor_2[0] = rand() % 4; cor_2[1] = rand() % 4;//second 船座標
        }
        int temp = 0;
        while(1){
            if(temp == 0 && p->turn == 1){
                printf("[%d Child]: Random Seed %d\n", getpid(), Cseed);
                temp++;
                p->turn = 0;
            }
            if(temp == 1 && p->turn == 1){
                printf("[%d Child]: The gunboat: (%d,%d)(%d,%d)\n", getpid(), cor_1[0], cor_1[1], cor_2[0], cor_2[1]);
                temp++;
                p->turn = 0;
                break;
            }
        }
        while(1){//無限迴圈,讓它可以一直判斷是誰的turn
            if(p->turn == 1){
                //判斷有無被打中
                //注意打中後如果再次打中視為miss,不會扣掉擊中次數
                if(((p->hit1 == cor_1[0])&&(p->hit2 == cor_1[1])) || (p->hit1 == cor_2[0])&&(p->hit2 == cor_2[1])){
                    if((p1_is_hit == 1) && (p->hit1 == cor_1[0])&&(p->hit2 == cor_1[1])){
                            if(p1_is_hit == 1){
                                p1_is_hit--;
                                printf("[%d Child]: hit", getpid());
                            }
                            else{
                                printf("[%d Child]: missed", getpid());
                            }
                        }
                        else if((p->hit1 == cor_2[0])&&(p->hit2 == cor_2[1])){
                            if(p2_is_hit == 1){
                                p2_is_hit--;
                                printf("[%d Child]: hit", getpid());
                            }
                            else{
                                printf("[%d Child]: missed", getpid());
                            }
                        }

                    if((p1_is_hit == 0) && (p2_is_hit == 0)){//all hit => sink
                        printf(" and sinking\n");
                        // break;
                    }
                    else{
                        printf("\n");
                    }
                }
                else{
                    printf("[%d Child]: missed\n", getpid());
                }

                if(p1_is_hit == 0 && p2_is_hit == 0){
                    // printf("[%d Child]: %d wins with %d bombs\n", getpid(), p->pid_p, p->bomb_p);
                    p->turn = 0;
                    p->win = 1;//parent win
                    break;
                }
                //隨機生成座標投擲炸彈
                hit[0] = rand() % 4;hit[1] = rand() % 4;//choose a random place to hit
                p->hit1 = hit[0];
                p->hit2 = hit[1];
                p->bomb_c++ ;
                printf("[%d Child]: bombing (%d,%d)\n", getpid(), p->hit1, p->hit2);
                
                //注意 child 做完需要把 turn 改成 parent 的 0,這樣 parent 的 while 才能判斷到
                p->turn = 0;
            }
        }
    }
    else if(pid > 0){//parent
    //parent 跟 child 基本上大同小異, 只差在變數名稱不同
            p->pid_p = getpid();
            srand(Pseed);
            while((cor_1[0] == cor_2[0]) && (cor_1[1] == cor_2[1]) || (!nextby(cor_1[0], cor_1[1], cor_2[0], cor_2[1]))){
                cor_1[0] = rand() % 4; cor_1[1] = rand() % 4;//first 船座標
                cor_2[0] = rand() % 4; cor_2[1] = rand() % 4;//second 船座標
            }
            int temp = 0;
            while(1){
                if(temp == 0 && p->turn == 0){
                    printf("[%d Parent]: Random Seed %d\n", getpid(), Pseed);
                    temp++;
                    p->turn = 1;
                }
                if(temp == 1 && p->turn == 0){
                    printf("[%d Parent]: The gunboat: (%d,%d)(%d,%d)\n", getpid(), cor_1[0], cor_1[1], cor_2[0], cor_2[1]);
                    temp++;
                    p->turn = 1;
                    break;
                }
            }
            while(1){
                if(p->turn == 0){
                    if(p->win == 1){//parent win
                        printf("[%d Parent]: %d wins with %d bombs\n", getpid(), p->pid_p, p->bomb_p);
                        break;
                    }
                    else if(p->win == 2){//child win
                        printf("[%d Parent]: %d wins with %d bombs\n", getpid(), p->pid_c, p->bomb_c);
                        break;
                    }
                    else if(p->win == 0){
                        if(count != 0){
                            //判斷有無打中
                            if(((p->hit1 == cor_1[0])&&(p->hit2 == cor_1[1])) || (p->hit1 == cor_2[0])&&(p->hit2 == cor_2[1])){
                                if((p->hit1 == cor_1[0])&&(p->hit2 == cor_1[1])){
                                    if(p1_is_hit == 1){
                                        p1_is_hit--;
                                        printf("[%d Parent]: hit", getpid());
                                    }
                                    else{
                                        printf("[%d Parent]: missed", getpid());
                                    }
                                }
                                else if((p->hit1 == cor_2[0])&&(p->hit2 == cor_2[1])){
                                    if(p2_is_hit == 1){
                                        p2_is_hit--;
                                        printf("[%d Parent]: hit", getpid());
                                    }
                                    else{
                                        printf("[%d Parent]: missed", getpid());
                                    }
                                }

                                if((p1_is_hit == 0) && p2_is_hit == 0){//all hit => sink
                                    printf(" and sinking\n");
                                    // break;
                                }
                                else{
                                    printf("\n");
                                }
                            }
                            else{
                                printf("[%d Parent]: missed\n", getpid());
                            }
                    
                            if((p1_is_hit == 0) && p2_is_hit == 0){
                                //printf("[%d Parent]: %d wins with %d bombs\n", getpid(), p->pid_c, p->bomb_c);
                                p->turn = 0;
                                p->win = 2;
                                continue;
                            }
                        }
                    }
                    hit[0] = rand() % 4;hit[1] = rand() % 4;//choose a random place to hit
                    p->hit1 = hit[0];
                    p->hit2 = hit[1];
                    p->bomb_p++;
                    printf("[%d Parent]: bombing (%d,%d)\n", getpid(), p->hit1, p->hit2);
                    p->turn = 1;
                    count++;
                }
            }
        }
    //最後解除映射記憶體
    int r;
    r = munmap(p, 0x400000);

    r = shm_unlink("posixsm");

    return 0;

}