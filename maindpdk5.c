#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/queue.h>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <cmdline_rdline.h>
#include <cmdline_parse.h>
#include <cmdline_socket.h>
#include <cmdline.h>
#include <rte_mbuf.h>

#include <time.h> 

#include "allHeaders.h"

//#define RUNMAINDPDK
#ifdef RUNMAINDPDK

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1
// #define MEMPOOL_F_SP_PUT         0x0

#define MBUF_CACHE_SIZE 32
#define NUM_MBUFS 511
static const char *MBUF_POOL = "MBUF_POOL";

static const char *Beforescramble = "Beforescramble";
static const char *scramble_2_BCC1 = "scramble_2_BCC1";
static const char *scramble_2_BCC2 = "scramble_2_BCC2";
static const char *BCC_2_modulation1 = "BCC_2_modulation1";
static const char *BCC_2_modulation2 = "BCC_2_modulation2";
static const char *BCC_2_modulation3 = "BCC_2_modulation3";
static const char *BCC_2_modulation4 = "BCC_2_modulation4";
static const char *BCC_2_modulation5 = "BCC_2_modulation5";
static const char *BCC_2_modulation6 = "BCC_2_modulation6";
static const char *modulation_2_CSD1 = "modulation_2_CSD1";
static const char *modulation_2_CSD2 = "modulation_2_CSD2";
static const char *modulation_2_CSD3 = "modulation_2_CSD3";
static const char *modulation_2_CSD4 = "modulation_2_CSD4";
static const char *modulation_2_CSD5 = "modulation_2_CSD5";
static const char *modulation_2_CSD6 = "modulation_2_CSD6";
static const char *AfterCSD1 = "AfterCSD1";

const unsigned APEP_LEN_DPDK = 512; 

// static int i=0; 
struct rte_ring *Ring_Beforescramble;
struct rte_ring *Ring_scramble_2_BCC1;
struct rte_ring *Ring_scramble_2_BCC2;
struct rte_ring *Ring_BCC_2_modulation1;
struct rte_ring *Ring_BCC_2_modulation2;
struct rte_ring *Ring_BCC_2_modulation3;
struct rte_ring *Ring_BCC_2_modulation4;
struct rte_ring *Ring_BCC_2_modulation5;
struct rte_ring *Ring_BCC_2_modulation6;
struct rte_ring *Ring_modulation_2_CSD1;
struct rte_ring *Ring_modulation_2_CSD2;
struct rte_ring *Ring_modulation_2_CSD3;
struct rte_ring *Ring_modulation_2_CSD4;
struct rte_ring *Ring_modulation_2_CSD5;
struct rte_ring *Ring_modulation_2_CSD6;
struct rte_ring *Ring_AfterCSD1;

struct rte_mempool *mbuf_pool;
	
volatile int quit = 0;

long int ReadData_count = 0;
long int GenDataAndScramble_DPDK_count = 0;
long int BCC_encoder_DPDK_count = 0;
long int modulate_DPDK_count = 0;
long int Data_CSD_DPDK_count = 0;
long int CSD_encode_DPDK_count = 0;
long int retrieve_Loop1_count = 0;

int N_CBPS, N_SYM, ScrLength, valid_bits;

struct timespec time1,time2,time_diff;	/** < Test the running time. >*/
int time_test_flag = 0;
struct timespec diff(struct timespec start, struct timespec end);

static int ReadData(__attribute__((unused)) struct rte_mbuf *Data_out, unsigned char* Data_in);
static int GenDataAndScramble_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int BCC_encoder_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int modulate_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int CSD_encode_dpdk (__attribute__((unused)) struct rte_mbuf *Data_In);

static int ReadData_Loop();
static int GenDataAndScramble_Loop();
static int BCC_encoder_Loop1();
static int BCC_encoder_Loop2();
static int modulate_Loop1();
static int modulate_Loop2();
static int modulate_Loop3();
static int modulate_Loop4();
static int modulate_Loop5();
static int modulate_Loop6();
static int Data_CSD_Loop1();
static int retrieve_Loop();

struct timespec diff(struct timespec start, struct timespec end)
{
    struct  timespec temp;

     if ((end.tv_nsec-start.tv_nsec)<0) {
         temp.tv_sec = end.tv_sec-start.tv_sec-1;
         temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
     } else {
         temp.tv_sec = end.tv_sec-start.tv_sec;
         temp.tv_nsec = end.tv_nsec-start.tv_nsec;
     }
     return temp;
}

static int InitData(unsigned char** p_databits)
{
	FILE *fp=fopen("send_din_dec.txt","rt");
	unsigned char *databits=(unsigned char*)malloc(APEP_LEN_DPDK*sizeof(unsigned char));
	*p_databits = databits;
	if(databits == NULL){
		printf("error");
		return 0;
	}
	unsigned int datatmp=0;
	for(int i=0;i<APEP_LEN_DPDK;i++){
	    fscanf(fp,"%ud",&datatmp);
	    databits[i]=datatmp&0x000000FF;
	}
	//memcpy(rte_pktmbuf_mtod(Data,unsigned char *), databits, APEP_LEN_DPDK);
	fclose(fp);
	return 0;
}

static int ReadData(__attribute__((unused)) struct rte_mbuf *Data_out, unsigned char* Data_in) 
{
	//printf("Data->buflen = %d\n",Data_out->buf_len);
	//printf("ReadData_count = %d\n", ReadData_count++);
	if(time_test_flag==0){
		time_test_flag = 1;
		clock_gettime(CLOCK_REALTIME, &time1); //CLOCK_REALTIME:系统实时时间   
		//CLOCK_MONOTONIC:从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
		//CLOCK_PROCESS_CPUTIME_ID:本进程到当前代码系统CPU花费的时间
		//CLOCK_THREAD_CPUTIME_ID:本线程到当前代码系统CPU花费的时间
	}
	rte_memcpy(rte_pktmbuf_mtod(Data_out,unsigned char *), Data_in, APEP_LEN_DPDK);
	return 0;
}


static int GenDataAndScramble_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In) 
{
	//printf("GenDataAndScramble_DPDK_count = %ld\n", GenDataAndScramble_DPDK_count++);
	
	unsigned char *databits = rte_pktmbuf_mtod(Data_In, unsigned char *);
	unsigned char *data_scramble = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, MBUF_CACHE_SIZE/2*1024);
	GenDataAndScramble(data_scramble, ScrLength, databits, valid_bits);	

	return 0;
}

static int BCC_encoder_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In)
{
	//printf("BCC_encoder_DPDK_count = %ld\n", BCC_encoder_DPDK_count++);

	int CodeLength = N_SYM*N_CBPS/N_STS;
	unsigned char *data_scramble = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, MBUF_CACHE_SIZE/2*1024);
	unsigned char* BCCencodeout = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, 0);
	BCC_encoder_OPT(data_scramble, ScrLength, N_SYM, &BCCencodeout, CodeLength);

	return 0;
}

static int modulate_DPDK(__attribute__((unused)) struct rte_mbuf *Data_In)
{
	//printf("modulate_DPDK_count = %ld\n", modulate_DPDK_count++);

	unsigned char* BCCencodeout = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, 0);
	unsigned char *stream_interweave_dataout = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, MBUF_CACHE_SIZE/2*1024);
	//complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, MBUF_CACHE_SIZE/2*1024);
	complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, 0);

	modulate_mapping(BCCencodeout, &stream_interweave_dataout, &subcar_map_data);

	return 0;
}	

static int CSD_encode_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In)
{
	int i;
	//CSD_encode_DPDK_count++;
	//printf("CSD_encode_DPDK_count = %ld\n", CSD_encode_DPDK_count++);
	//complex32 *csd_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, 0);
	//complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, MBUF_CACHE_SIZE/2*1024);
	complex32 *csd_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, MBUF_CACHE_SIZE/2*1024);
	complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, 0);
	//Data_CSD(&subcar_map_data, N_SYM, &csd_data);
	
	for(i=0;i<N_STS;i++){
		__Data_CSD_aux(&subcar_map_data, N_SYM, &csd_data,i);
	}
	return 0;
}

static int retrieve_Loop(){
	void *Data_In_CSD=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_AfterCSD1, &Data_In_CSD) >= 0)
		{
			//printf("retrieve_Loop1 = %ld\n", retrieve_Loop1_count++);
			rte_mempool_put(((struct rte_mbuf *)Data_In_CSD)->pool, Data_In_CSD);
		}
		else 
		{
			continue;
		}
	}
	return 0;	
}


static int Data_CSD_Loop1() 
{
	void *Data_In_CSD=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_modulation_2_CSD1, &Data_In_CSD) >= 0 || 
			rte_ring_dequeue(Ring_modulation_2_CSD2, &Data_In_CSD) >= 0 ||
			rte_ring_dequeue(Ring_modulation_2_CSD3, &Data_In_CSD) >= 0 ||
			rte_ring_dequeue(Ring_modulation_2_CSD4, &Data_In_CSD) >= 0 ||
			rte_ring_dequeue(Ring_modulation_2_CSD5, &Data_In_CSD) >= 0 ||
			rte_ring_dequeue(Ring_modulation_2_CSD6, &Data_In_CSD) >= 0 )
		{
			CSD_encode_DPDK_count++;
			CSD_encode_DPDK(Data_In_CSD);
			if(CSD_encode_DPDK_count >= 10000)
			{
				quit = 1;

				clock_gettime(CLOCK_REALTIME, &time2);
				time_diff = diff(time1,time2);
				printf("CSD_encode_DPDK_count = %ld\n", CSD_encode_DPDK_count);
				printf("Start time # %.24s %ld Nanoseconds \n",ctime(&time1.tv_sec), time1.tv_nsec);
				printf("Stop time # %.24s %ld Nanoseconds \n",ctime(&time2.tv_sec), time2.tv_nsec);
				printf("Running time # %ld.%ld Seconds \n",time_diff.tv_sec, time_diff.tv_nsec);
				//printf("%.24s %ld Nanoseconds \n", ctime(&ts.tv_sec), ts.tv_nsec); 
			}
			//rte_ring_enqueue(Ring_AfterCSD1, Data_In_CSD);	
			rte_mempool_put(((struct rte_mbuf *)Data_In_CSD)->pool, Data_In_CSD);
		}
		else 
		{
			continue;
		}
	
	}
	return 0;
}

static int modulate_Loop1() 
{
	void *Data_In_modulate=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_BCC_2_modulation1, &Data_In_modulate) >= 0)
		{
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD1, Data_In_modulate);
		}
		else if (rte_ring_dequeue(Ring_scramble_2_BCC1, &Data_In_modulate) >= 0){
			BCC_encoder_DPDK(Data_In_modulate);
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD1, Data_In_modulate);
		}
		else 
		{	
			continue;
		}
	}	
	return 0;
}

static int modulate_Loop2() 
{
	void *Data_In_modulate=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_BCC_2_modulation2, &Data_In_modulate) >= 0)
		{
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD2, Data_In_modulate);
		}
		else if (rte_ring_dequeue(Ring_scramble_2_BCC1, &Data_In_modulate) >= 0){
			BCC_encoder_DPDK(Data_In_modulate);
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD2, Data_In_modulate);
		}
		else 
		{	
			continue;
		}
	}	
	return 0;
}

static int modulate_Loop3() 
{
	void *Data_In_modulate=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_BCC_2_modulation3, &Data_In_modulate) >= 0)
		{
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD3, Data_In_modulate);
		}
		else if (rte_ring_dequeue(Ring_scramble_2_BCC1, &Data_In_modulate) >= 0){
			BCC_encoder_DPDK(Data_In_modulate);
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD3, Data_In_modulate);
		}
		else 
		{	
			continue;
		}
	}	
	return 0;
}

static int modulate_Loop4() 
{
	void *Data_In_modulate=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_BCC_2_modulation4, &Data_In_modulate) >= 0)
		{
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD4, Data_In_modulate);
		}
		else if (rte_ring_dequeue(Ring_scramble_2_BCC2, &Data_In_modulate) >= 0){
			BCC_encoder_DPDK(Data_In_modulate);
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD4, Data_In_modulate);
		}
		else 
		{	
			continue;
		}
	}	
	return 0;
}

static int modulate_Loop5() 
{
	void *Data_In_modulate=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_BCC_2_modulation5, &Data_In_modulate) >= 0)
		{
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD5, Data_In_modulate);
		}
		else if (rte_ring_dequeue(Ring_scramble_2_BCC2, &Data_In_modulate) >= 0){
			BCC_encoder_DPDK(Data_In_modulate);
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD5, Data_In_modulate);
		}
		else 
		{	
			continue;
		}
	}	
	return 0;
}
static int modulate_Loop6() 
{
	void *Data_In_modulate=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_BCC_2_modulation6, &Data_In_modulate) >= 0)
		{
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD6, Data_In_modulate);
		}
		else if (rte_ring_dequeue(Ring_scramble_2_BCC2, &Data_In_modulate) >= 0){
			BCC_encoder_DPDK(Data_In_modulate);
			modulate_DPDK(Data_In_modulate);
			rte_ring_enqueue(Ring_modulation_2_CSD6, Data_In_modulate);
		}
		else 
		{	
			continue;
		}
	}	
	return 0;
}

static int BCC_encoder_Loop1() 
{

	void *Data_In_BCC=NULL;
	int dis_count = 0;
	int dis_count2 = 0;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_scramble_2_BCC1, &Data_In_BCC) >= 0)
		{
			dis_count++;
			if(dis_count == 4){
				dis_count = 1;
			}
			BCC_encoder_DPDK(Data_In_BCC);
			if(dis_count == 1)
				rte_ring_enqueue(Ring_BCC_2_modulation1, Data_In_BCC);
			else if(dis_count == 2)
				rte_ring_enqueue(Ring_BCC_2_modulation2, Data_In_BCC);
			else if(dis_count == 3)
				rte_ring_enqueue(Ring_BCC_2_modulation3, Data_In_BCC);
			else
				rte_ring_enqueue(Ring_BCC_2_modulation1, Data_In_BCC);
		}
		else if (rte_ring_dequeue(Ring_Beforescramble, &Data_In_BCC) >= 0){
			GenDataAndScramble_DPDK(Data_In_BCC);
			BCC_encoder_DPDK(Data_In_BCC);
			if(dis_count2 == 4){
				dis_count2 = 1;
			}
			if(dis_count2 == 1)
				rte_ring_enqueue(Ring_BCC_2_modulation4, Data_In_BCC);
			else if(dis_count2 == 2)
				rte_ring_enqueue(Ring_BCC_2_modulation5, Data_In_BCC);
			else if(dis_count2 == 3)
				rte_ring_enqueue(Ring_BCC_2_modulation6, Data_In_BCC);
			else
				rte_ring_enqueue(Ring_BCC_2_modulation4, Data_In_BCC);
		}
		else 
		{	
			continue;
		}
	
	}
	return 0;
}

static int BCC_encoder_Loop2() 
{

	void *Data_In_BCC=NULL;
	int dis_count = 0;
	int dis_count2 = 0;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_scramble_2_BCC2, &Data_In_BCC) >= 0)
		{
			dis_count++;
			if(dis_count == 4){
				dis_count = 1;
			}
			BCC_encoder_DPDK(Data_In_BCC);
			if(dis_count == 1)
				rte_ring_enqueue(Ring_BCC_2_modulation4, Data_In_BCC);
			else if(dis_count == 2)
				rte_ring_enqueue(Ring_BCC_2_modulation5, Data_In_BCC);
			else if(dis_count == 3)
				rte_ring_enqueue(Ring_BCC_2_modulation6, Data_In_BCC);
			else
				rte_ring_enqueue(Ring_BCC_2_modulation4, Data_In_BCC);
		}
		else if (rte_ring_dequeue(Ring_Beforescramble, &Data_In_BCC) >= 0){
			GenDataAndScramble_DPDK(Data_In_BCC);
			BCC_encoder_DPDK(Data_In_BCC);
			if(dis_count2 == 4){
				dis_count2 = 1;
			}
			if(dis_count2 == 1)
				rte_ring_enqueue(Ring_BCC_2_modulation4, Data_In_BCC);
			else if(dis_count2 == 2)
				rte_ring_enqueue(Ring_BCC_2_modulation5, Data_In_BCC);
			else if(dis_count2 == 3)
				rte_ring_enqueue(Ring_BCC_2_modulation6, Data_In_BCC);
			else
				rte_ring_enqueue(Ring_BCC_2_modulation4, Data_In_BCC);
		}
		else 
		{	
			continue;
		}
	
	}
	return 0;
}

static int GenDataAndScramble_Loop() 
{
	void *Data_In_Scramble=NULL;
	int dis_count = 0;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_Beforescramble, &Data_In_Scramble) >= 0)
		{
			dis_count++;
			if(dis_count == 3){
				dis_count = 1;
			}
			GenDataAndScramble_DPDK(Data_In_Scramble);
			if(dis_count == 1)
				rte_ring_enqueue(Ring_scramble_2_BCC1, Data_In_Scramble);
			if(dis_count == 2)
				rte_ring_enqueue(Ring_scramble_2_BCC2, Data_In_Scramble);
		}
		else 
		{	
			continue;
		}
	
	}
	return 0;
}

static int ReadData_Loop() 
{
	struct rte_mbuf *Data =NULL;
	unsigned char* Data_in =NULL;
	InitData(&Data_in);
	while (!quit){
		Data = rte_pktmbuf_alloc(mbuf_pool);
		if (Data != NULL)
		{
			ReadData(Data, Data_in);
			rte_ring_enqueue(Ring_Beforescramble, Data);
		}
		else 
		{
			continue;
		}
	
	}
	return 0;
}



int
main(int argc, char **argv)
{
	const unsigned flags = 0;
	const unsigned ring_size = 512;
	const unsigned pool_size = 256;
	const unsigned pool_cache = 32;
	const unsigned priv_data_sz = 0;
	int ret;
	// 运行一次得到preamble和HeLTF.
	generatePreambleAndHeLTF_csd();
	// 运行一次得到比特干扰码表。
	Creatnewchart();
	// 运行一次得到BCC编码表。
	init_BCCencode_table();
	// 运行一次得到生成导频的分流交织表
	initial_streamwave_table();
	// 运行一次得到CSD表。
	//initcsdTableForHeLTF();
	// 初始化函数，计算OFDM符号个数，字节长度
	//int N_CBPS, N_SYM, ScrLength, valid_bits;
   	GenInit(&N_CBPS, &N_SYM, &ScrLength, &valid_bits);
	///////////////////////////////////////////////////////////////////////////////////
	//unsigned lcore_id;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
		Ring_Beforescramble = rte_ring_create(Beforescramble , ring_size, rte_socket_id(), flags);
		Ring_scramble_2_BCC1 = rte_ring_create(scramble_2_BCC1, ring_size, rte_socket_id(), flags);
		Ring_scramble_2_BCC2 = rte_ring_create(scramble_2_BCC2, ring_size, rte_socket_id(), flags);
		Ring_BCC_2_modulation1 = rte_ring_create(BCC_2_modulation1, ring_size, rte_socket_id(), flags);
		Ring_BCC_2_modulation2 = rte_ring_create(BCC_2_modulation2, ring_size, rte_socket_id(), flags);
		Ring_BCC_2_modulation3 = rte_ring_create(BCC_2_modulation3, ring_size, rte_socket_id(), flags);
		Ring_BCC_2_modulation4 = rte_ring_create(BCC_2_modulation4, ring_size, rte_socket_id(), flags);
		Ring_BCC_2_modulation5 = rte_ring_create(BCC_2_modulation5, ring_size, rte_socket_id(), flags);
		Ring_BCC_2_modulation6 = rte_ring_create(BCC_2_modulation6, ring_size, rte_socket_id(), flags);
		Ring_modulation_2_CSD1 = rte_ring_create(modulation_2_CSD1, ring_size, rte_socket_id(), flags);
		Ring_modulation_2_CSD2 = rte_ring_create(modulation_2_CSD2, ring_size, rte_socket_id(), flags);
		Ring_modulation_2_CSD3 = rte_ring_create(modulation_2_CSD3, ring_size, rte_socket_id(), flags);
		Ring_modulation_2_CSD4 = rte_ring_create(modulation_2_CSD4, ring_size, rte_socket_id(), flags);
		Ring_modulation_2_CSD5 = rte_ring_create(modulation_2_CSD5, ring_size, rte_socket_id(), flags);
		Ring_modulation_2_CSD6 = rte_ring_create(modulation_2_CSD6, ring_size, rte_socket_id(), flags);
		Ring_AfterCSD1 = rte_ring_create(AfterCSD1, ring_size, rte_socket_id(), flags);
	
	if (Ring_Beforescramble == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_scramble_2_BCC1 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting receiving ring\n");
	if (Ring_scramble_2_BCC2 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting receiving ring\n");
	if (Ring_BCC_2_modulation1 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_BCC_2_modulation2 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_BCC_2_modulation3 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_BCC_2_modulation4 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_BCC_2_modulation5 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_BCC_2_modulation6 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_modulation_2_CSD1 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting receiving ring\n");
	if (Ring_modulation_2_CSD2 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting receiving ring\n");
	if (Ring_modulation_2_CSD3 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting receiving ring\n");
	if (Ring_modulation_2_CSD4 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting receiving ring\n");
	if (Ring_modulation_2_CSD5 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting receiving ring\n");
	if (Ring_modulation_2_CSD6 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting receiving ring\n");
	if (Ring_AfterCSD1 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");

	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE*16, rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	RTE_LOG(INFO, APP, "Finished Process Init.\n");

	rte_eal_remote_launch(ReadData_Loop, NULL,1);
	rte_eal_remote_launch(GenDataAndScramble_Loop, NULL, 2);
	rte_eal_remote_launch(BCC_encoder_Loop1, NULL, 3);
	rte_eal_remote_launch(BCC_encoder_Loop2, NULL,4);
	rte_eal_remote_launch(modulate_Loop1, NULL, 5);
	rte_eal_remote_launch(modulate_Loop2, NULL,6);
	rte_eal_remote_launch(modulate_Loop3, NULL,7);
	rte_eal_remote_launch(modulate_Loop4, NULL,8);
	rte_eal_remote_launch(modulate_Loop5, NULL,9);
	rte_eal_remote_launch(modulate_Loop6, NULL,10);
	rte_eal_remote_launch(Data_CSD_Loop1, NULL,11);
	//rte_eal_remote_launch(retrieve_Loop, NULL,12);
	rte_eal_mp_wait_lcore();
	return 0;
}
#endif // RUN