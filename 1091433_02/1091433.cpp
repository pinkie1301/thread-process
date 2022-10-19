//g++ -o 1091433 1091433.cpp -Wall -Werror -lpthread
//./1091433 filename.txt
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include<cmath>			/* Calculate sqrt*/
#include<iomanip>		/* Setw, setprecision*/
#include<time.h>		/* Cpu time*/
#include<sstream>

using namespace std;
void *print_message_function(void *ptr);
/* struct to hold data to be passed to a thread
this shows how multiple data items can be passed to a thread */
typedef struct str_thdata
{
	double Avg_cos;//average cosine
	string docnum;//document number
	vector<string> message;//data's own word
	vector<string> mes_vec;//word dictionary
	vector<int>time;//total appear times of one word
} thdata;

thdata data[50];//Data to be passed to threads
int datacount = 0;

int main(int argc, char *argv[])
{
	//計算程式開始時間
	clock_t main_start, main_end;//time variable
	double main_cputime;
	main_start = clock();//start calculate

	vector<string>wd;
	vector<int>wd_num;

	// READ FILE
	ifstream fin1;
	string line[100000];
	int linecount = 0;
	fin1.open(argv[1]);
	//開檔
	if(!fin1.is_open()){
		cout << "Open File Failed\n";
	}
	//讀檔
	while (!fin1.eof())
	{
		getline(fin1, line[linecount]);
		linecount++;
	}
	// END READ FILE

	//divide each word from a line
	for(int i = 0; i < linecount; i++)
	{
		stringstream ss;
		string ss_str;
		ss << line[i];
		ss >> ss_str;
		if (atoi(ss_str.c_str()) > 0)
		{
			//如果整個字串都是數字,沒有空白或是字母就歸類為文件編號
			data[datacount].docnum = ss_str;
		}
		else
		{
			string temp;
			for(size_t end = 0; end <= line[i].length(); end++){
				if((65 <= line[i][end] && line[i][end] <= 90) || ( 97 <= line[i][end] && line[i][end]  <= 122)){
					//pushback to temp if it is a letter or number
					temp += line[i][end];
				}
				else{
					if(temp != " " && !temp.empty()){
						//encounter a none letter word -> store previous vector into data as a word
						data[datacount].message.push_back(temp);
						bool match = false;
						//create a word dictionary
						if(wd.size()==0){
							wd.push_back(temp);
							wd_num.push_back(0);
						}
						for(size_t j = 0; j <= wd.size(); j++){
							if(wd[j] == temp){
								match = true;
								break;
							}
						}
						//如果原本的wd沒有任何匹配的字,代表這是新的字,所以可以加進wd裡面
						if(!match){
							wd.push_back(temp);
							wd_num.push_back(0);
						}
						temp.clear();//clear temp for next loop
					}
				}
			}
			datacount++;//how many files
		}
	}
	for(int i = 0; i < 50; i++){
		//計算每個單字出現多少次
		data[i].mes_vec = wd;
		data[i].time = wd_num;
		for(size_t j = 0; j < data[i].message.size(); j++){
			for(size_t k = 0; k < wd.size(); k++){
				if(data[i].message[j] == wd[k]){
					//有跟wd字典裡match到就加次數
					data[i].time[k] += 1;
					break;
				}
			}
		}
	}
	pthread_t thread[datacount];
	for(int i = 0;i < datacount; i++){
		//create thread
		pthread_create(&thread[i], NULL, print_message_function, (void*)&data[i]);
		cout << "[Main thread]: create TID:" << thread[i]/*get tid*/ << ", DocID:" << data[i].docnum << "\n";
	}
	/* Main block now waits for threads to terminate, before it exits
	If main block exits, both threads exit, even if the threads have not
	finished their work */
	for(int i = 0; i < datacount; i++){
		//回收thread
		pthread_join(thread[i], NULL);
	}

	//KeyDocID
	//判斷哪個文件的avg_cos最高並且文件id最小
	double max = 0;
	int min = 99999;
	string str_min;
	for(int i = 0; i < datacount; i++){
		if(data[i].Avg_cos == max && atoi(data[i].docnum.c_str()) < min){
			max = data[i].Avg_cos;
			min = atoi(data[i].docnum.c_str());
			str_min = data[i].docnum;
		}
		else if(data[i].Avg_cos > max){
			max = data[i].Avg_cos;
			min = atoi(data[i].docnum.c_str());
			str_min = data[i].docnum;
		}
	}
	cout << "[Main thread] KeyDocID:" << str_min << " Highest Average Cosine: " << max << "\n";

	//獲取程式結束時間
	main_end = clock();//end calculate
	//用cpu time公式算出結果
	main_cputime = ((double)(main_end - main_start)) / CLOCKS_PER_SEC;//the cpu time
	cout.unsetf(ios::fixed);
	cout << "[Main thread] CPU time:" << main_cputime*1000000 << "ms" << endl;
	/* exit */
	exit(0);
} /* main() */
/**
 * print_message_function is used as the start routine for the threads used
 * it accepts a void pointer
 **/
void *print_message_function(void *ptr)
{
	//獲取thread開始時間
	clock_t th_beg, th_end;
	double th_cputime;
	th_beg = clock();

	thdata *d;
	pthread_t tid = pthread_self();//thread id
	vector<double>Cos;//存跟其他文件的詞頻向量

	d = (thdata *)ptr; /* type cast to a pointer to thdata */
	/* do the work */
	//負責計算的主文件ID編號，以及它的詞頻向量
	cout << "[TID=" << tid << "]DocID:" << d->docnum << "[" << d->time[0];
	for(size_t i = 1; i < d->time.size(); i++)
		cout << ',' << d->time[i];
	cout << "]\n";
	//印出是哪兩個文件在計算
	double c_sum = 0;
	for(int i = 0; i < datacount; i++){
		double Vs = 0, Vx = 0, Vs_Vx = 0, c = 0;
		if(data[i].docnum != d->docnum){
			//公式
			for(size_t j = 0; j < data[i].time.size(); j++){
				Vs += d->time[j]*d->time[j];
				Vx += data[i].time[j]*data[i].time[j];
				Vs_Vx += d->time[j]*data[i].time[j];
			}
			c = Vs_Vx/(sqrt(Vs)*sqrt(Vx));
			//它們Cosine similarity
			cout << "[TID=" << tid << "]cosine(" << d->docnum << "," << data[i].docnum << ")=" << setprecision(4) << fixed << c << endl;
			Cos.push_back(c);
			c_sum+=c;
		}
	}
	d->Avg_cos = c_sum/(datacount - 1);//avg_cos值
	ptr = d;
	//輸出內容
	cout << "[TID=" << tid << "]Avg_cosine:" << d->Avg_cos << endl;
	th_end = clock();
	th_cputime = ((double)(th_end-th_beg))/CLOCKS_PER_SEC * 1000000;
	cout.unsetf(ios::fixed);
	cout << "[TID=" << tid << "] CPU time: " << th_cputime << "ms" << endl;
	pthread_exit(0); /* exit */
} /* print_message_function ( void *ptr ) */