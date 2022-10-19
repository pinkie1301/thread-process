// g++ -o s1091433 s1091433.cpp -Werror -Wall -lpthread
//./s1091433 mode randomseed
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>    /* Symbolic Constants */
#include <sys/types.h> /* Primitive System Data Types */
#include <errno.h>     /* Errors */
#include <stdio.h>     /* Input/Output */
#include <stdlib.h>    /* General Utilities */
#include <pthread.h>   /* POSIX Threads */
#include <string.h>    /* String handling */
#include <cmath>       /* Calculate sqrt*/
#include <iomanip>     /* Setw, setprecision*/
#include <time.h>      /* Cpu time*/
#include <sstream>
using namespace std;

//零件:battery, aircraft, propeller
typedef struct str_thdata
{
    int id;
    bool battery;
    bool aircraft;
    bool propeller;
} thdata;

//用來傳亂數種子
struct dis_component
{
    int seed;
};

thdata shcomp;        // share component
pthread_t tid[5];     // producer[0][1][2] + dispatcher[3][4]共五個thread
pthread_mutex_t lock; // mutex lock
int mode, sum = 0;
int n = 50;
int total[6] = {};//[0][1][2]=>producer做了幾個,[3][4][5]=>dispatcher準備了多少個零件

void *doSomeThing(void *arg);//producer做事
void *dispatcher_A(void *arg);//dispatcher A做事
void *dispatcher_B(void *arg);//dispatcher B做事

int main(int argc, char *argv[])
{
    stringstream ss;
    string in1, in2;
    ss << argv[1];
    ss >> in1;
    ss << argv[2];
    ss >> in2;
    //判斷輸入合不合法
    if (argv[3] != NULL)
    {
        printf("%s", "You enter too much parameter!\n");
        return 1;
    }
    if (in1 != "0" && in1 != "1")
    {
        printf("%s", "Out of range!\n");
        return 1;
    }
    if (atoi(in2.c_str()) < 0 || atoi(in2.c_str()) > 100)
    {
        printf("%s", "Out of range!\n");
        return 1;
    }

    int i;
    int err, err2, err3;
    thdata data[3]; //每個producer的零件
    dis_component dis;//dispatcher
    mode = atoi(argv[1]);
    dis.seed = atoi(argv[2]); //亂數種子

    //初始化狀態全設為false,代表table上沒東西
    shcomp.aircraft = false;
    shcomp.battery = false;
    shcomp.propeller = false;
    for (i = 0; i < 3; i++)
    {
        data[i].aircraft = false;
        data[i].battery = false;
        data[i].propeller = false;
    }

    //初始化mutex
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    // create thread
    i = 0;
    while (i < 3)
    {
        data[i].id = i + 1;
        err = pthread_create(&(tid[i]), NULL, &doSomeThing, (void *)&data[i]);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        i++;
    }

    //mode 0
    err2 = pthread_create(&(tid[3]), NULL, &dispatcher_A, (void *)&dis);
    if (err2 != 0)
        printf("\ncan't create thread :[%s]", strerror(err));

    //mode 1
    //多出一個dispatcher B
    if(mode == 1){
        err3 = pthread_create(&(tid[4]), NULL, &dispatcher_B, (void *)&dis);
        if (err3 != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
    }

    // join thread
    for (i = 0; i < 5; i++)
    {
        pthread_join(tid[i], NULL);
    }
    // destroy mutex
    pthread_mutex_destroy(&lock);
    
    printf("\nDispatcher create:\n\tAircraft :%i\tBattery :%i\tPropeller :%i\nTotal create:\n\tProducer1 :%i\tProducer2 :%i\tProducer3 :%i\n"
    ,total[3], total[4], total[5], total[0], total[1], total[2]);
    return 0;
}

void *doSomeThing(void *data)
{
    //"producer"會一直執行直到做出來的機器達到50個
    while (sum < n)
    {
        thdata *d;
        d = (thdata *)data;
        if (d->id == 1)
            d->aircraft = true; //升級過的會自備aircraft

        pthread_mutex_lock(&lock);
        /*critical section*/

        //因為sum有可能在進入while跟mutex之間被改變,因此還要再判斷一次確認到底做了幾個產品出來
        if (sum >= n){
            pthread_mutex_unlock(&lock);
            return NULL;
        }

        //如果table上有零件是自己缺的就拿走
        if ((shcomp.aircraft && !d->aircraft) || (shcomp.battery && !d->battery) || (shcomp.propeller && !d->propeller))
        {
            cout << "Producer " << d->id << " ";
            if (d->id == 1)
                cout << "(aircraft)";
            cout << ": get ";
            //判斷要拿走table上的哪個零件
            //拿走 =>table = false
            //    =>自己 = true
            if (shcomp.aircraft && !d->aircraft)
            {
                cout << "aircraft\n";
                shcomp.aircraft = false;
                d->aircraft = true;
            }
            else if (shcomp.battery && !d->battery)
            {
                cout << "battery\n";
                shcomp.battery = false;
                d->battery = true;
            }
            else if (shcomp.propeller && !d->propeller)
            {
                cout << "propeller\n";
                shcomp.propeller = false;
                d->propeller = true;
            }
        }

        //如果拿完了一次零件,又剛好湊滿了三個零件就可以組成一台機器,sum++
        if (d->aircraft && d->battery && d->propeller)
        {
            sum++;
            if (sum > n)
                break;
            if (d->id == 1)
                printf("Producer 1 (aircraft): OK, %i drone(s)\n", sum);
            else
                printf("Producer %i: OK, %i drone(s)\n", d->id, sum);
            //組完了一台,記錄下來,零件消耗完需要歸零重算
            total[d->id - 1]++;
            d->aircraft = false;
            d->battery = false;
            d->propeller = false;
        }
        /*----------------*/
        pthread_mutex_unlock(&lock);
        data = d;
    }
    return NULL;
}

void *dispatcher_A(void *arg)
{
    dis_component *d;
    d = (dis_component *)arg;
    srand((int)d->seed); //設定亂數種子

    //"dispatcher"會一直執行直到做出來的機器達到50個
    while (sum < n)
    {
        int r;
        string name;

        pthread_mutex_lock(&lock);
        /*critical section*/
        if (sum >= n){
            pthread_mutex_unlock(&lock);
            return NULL;
        }
        //判斷mode是多少
        if(mode == 0){
            r = rand() % 3;
            name = "Dispatcher";
        }
        else if(mode == 1){
            r = rand() % 2;
            name = "Dispatcher A";
        }

        //看亂數r決定要丟出哪個零件,並且要紀錄
        switch (r)
        { 
        case 0:
            if (!shcomp.aircraft)
            {
                shcomp.aircraft = true;
                total[3]++;
                cout << name << ": aircraft\n";
            }
            break;
        case 1:
            if (!shcomp.battery)
            {
                shcomp.battery = true;
                total[4]++;
                cout << name << ": battery\n";
            }
        case 2:
            if (!shcomp.propeller && mode == 0)
            {
                shcomp.propeller = true;
                total[5]++;
                cout << name << ": propeller\n";
            }
            break;
        default:
            break;
        }
        /*----------------*/
        pthread_mutex_unlock(&lock);
    }
    arg = d;
    return NULL;
}

void *dispatcher_B(void *arg)
{
    dis_component *d;
    d = (dis_component *)arg;
    srand((int)d->seed); //設定亂數種子

    //"dispatcher"會一直執行直到做出來的機器達到50個
    while (sum < n)
    {
        int r = rand() % 2;
        pthread_mutex_lock(&lock);
        /*critical section*/
        if (sum >= n){
            pthread_mutex_unlock(&lock);
            return NULL;
        }

        //看亂數r決定要丟出哪個零件
        switch (r)
        { 
        case 0:
            if (!shcomp.aircraft)
            {
                shcomp.aircraft = true;
                total[3]++;
                cout << "Dispatcher B: aircraft\n";
            }
            break;
        case 1:
            if (!shcomp.propeller)
            {
                shcomp.propeller = true;
                total[5]++;
                cout << "Dispatcher B: propeller\n";
            }
            break;
        default:
            break;
        }
        /*----------------*/
        pthread_mutex_unlock(&lock);
    }
    arg = d;
    return NULL;
}