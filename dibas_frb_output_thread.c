/*
 * dibas_frb_output_thread.c
 * 
 */

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "hashpipe.h"
#include "dibas_frb_databuf.h"
#include "filterbank.h"
#include <sys/time.h>
extern double MJD;

static void *run(hashpipe_thread_args_t * args)
{
	// Local aliases to shorten access to args fields
	// Our input buffer happens to be a dibas_frb_ouput_databuf
	dibas_frb_input_databuf_t *db = (dibas_frb_input_databuf_t *)args->ibuf;
	hashpipe_status_t st = args->st;
	const char * status_key = args->thread_desc->skey;
	int c,rv,result;
	int block_idx = 0;
	uint64_t mcnt=0;
	bool f_full_flag = 1;
	//extern double MJD;
/*
	I = X*X + Y*Y
	Q = X*X - Y*Y
	U = 2 * XY_real
	V = -2 * XY_img
*/
	//unsigned char XX[N_CHANS_SPEC];
	//unsigned char YY[N_CHANS_SPEC];
	//short int U[N_CHANS_SPEC];
	//short int V[N_CHANS_SPEC]; 
	char f_fil[256];
        char fin_fil[256], command[512];
	struct tm        *now;
	time_t           rawtime;
    	//Note: To change integration time, change FILE_SIZE_MB
        double  FILE_SIZE_MB = 5000; // MB
	double  FILE_SIZE_NOW_MB = 0;

	//f_fil = (char *)malloc(256*sizeof(char));
	//f_fil="filterbank-test20170612";

	FILE * dibas_frb_file;
	//dibas_frb_file=fopen("./dibas_frb_file.fil","w");
	//printf("open new filterbank file...");
	//fclose(dibas_frb_file);
	//f_fil = strcpy(f_fil,"filterbank-test20170612");
	/*time(&rawtime);
	now = localtime(&rawtime);
	strftime(f_fil,sizeof(f_fil), "./data_%Y-%m-%d_%H-%M-%S.fil",now);
	WriteHeader(f_fil);
	printf("file name is: %s\n",f_fil);
	printf("write header done!\n");
	dibas_frb_file=fopen(f_fil,"a+");
	printf("starting write data...\n");	*/
	/* Main loop */
	while (run_threads()) {

		hashpipe_status_lock_safe(&st);
		hputi4(st.buf, "OUTBLKIN", block_idx);
		hputi8(st.buf, "OUTMCNT",mcnt);
		hputs(st.buf, status_key, "waiting");
		hashpipe_status_unlock_safe(&st);

		// get new data
		while ((rv=dibas_frb_input_databuf_wait_filled(db, block_idx))
		!= HASHPIPE_OK) {
		if (rv==HASHPIPE_TIMEOUT) {
			hashpipe_status_lock_safe(&st);
			hputs(st.buf, status_key, "blocked");
			hashpipe_status_unlock_safe(&st);
			continue;
			} else {
				hashpipe_error(__FUNCTION__, "error waiting for filled databuf");
				pthread_exit(NULL);
				break;
			}
		}

		hashpipe_status_lock_safe(&st);
		hputs(st.buf, status_key, "processing");
		hashpipe_status_unlock_safe(&st);

        if (f_full_flag ==1){
            char    f_fil[256];
            struct tm  *now;
            time_t rawtime;

            printf("\n\nopen new filterbank file...\n\n");
            time(&rawtime);
			now = localtime(&rawtime);
//			strftime(f_fil,sizeof(f_fil), "/ldata/dibas_frb/frb_%Y-%m-%d_%H-%M-%S.fil",now);
			//strftime(f_fil,sizeof(f_fil), "/sdata/filterbank/frb_%Y-%m-%d_%H-%M-%S.fil.tmp",now);
			strftime(f_fil,sizeof(f_fil), "/ldata/dibas_frb/frb_%Y-%m-%d_%H-%M-%S.fil",now);
                        strncpy(fin_fil,f_fil,strlen(f_fil) - 4);
                        fin_fil[strlen(f_fil) - 4] = '\0';
			WriteHeader(f_fil,MJD);
			printf("file name is: %s\n",f_fil);
                        printf("final file name is: %s\n",fin_fil);
			printf("write header done!\n");
			dibas_frb_file=fopen(f_fil,"a+");
			printf("starting write data...\n");
                        strcpy(command,"mv ");
                        strcat(command,f_fil);
                        strcat(command," ");
                        strcat(command,fin_fil);
                        printf("Command: %s\n",command);	

        }

		// TODO check mcnt
		//memcpy(Full_Stokes,db->block[block_idx].Stokes_Full,PKTSIZE*sizeof(char));
		//memcpy(XX,db->block[block_idx].XX,N_CHANS_SPEC*sizeof(char));
		//memcpy(YY,db->block[block_idx].YY,N_CHANS_SPEC*sizeof(char));
		//memcpy(U,db->block[block_idx].XY_REAL,N_CHANS_SPEC*sizeof(short int));
		//memcpy(V,db->block[block_idx].XY_IMAG,N_CHANS_SPEC*sizeof(short int));
		//printf("first data from output thread is:%c\n",I[0]);

        FILE_SIZE_NOW_MB += PAGE_SIZE*N_CHANS_SPEC*N_IFs_FIL*N_BYTES_FIL/1024/1024.0;
//	printf("FILE_SIZE_NOW_MB is %lf\n",FILE_SIZE_NOW_MB);
        if (FILE_SIZE_NOW_MB >= FILE_SIZE_MB){
			f_full_flag = 1;
			FILE_SIZE_NOW_MB = 0;
        }
        else{f_full_flag = 0;}



        //printf("\ndata save:%lld\n",N_Mbytes_save);
        //printf("\nfile save:%lld\n",N_Mbytes_file);
        //printf("\ndevide?:%lld\n",N_Mbytes_save%N_Mbytes_file);


		/*if (mcnt%10==0){
			printf("open new filterbank file...");
			//fclose(dibas_frb_file);
			//f_fil = strcpy(f_fil,"filterbank-test20170612");
			time(&rawtime);
			now = localtime(&rawtime);
			strftime(f_fil,sizeof(f_fil), "./data_%Y-%m-%d_%H-%M-%S",now);
			WriteHeader(f_fil);
			dibas_frb_file=fopen(f_fil,"a");		
		}*/
		if(N_IFs_FIL==1){
			fwrite(db->block[block_idx].XX,sizeof(char),PAGE_SIZE*N_CHANS_SPEC,dibas_frb_file);
		}else if(N_IFs_FIL==2){
			fwrite(db->block[block_idx].XX,sizeof(char),PAGE_SIZE*N_CHANS_SPEC,dibas_frb_file);
			fwrite(db->block[block_idx].YY,sizeof(char),PAGE_SIZE*N_CHANS_SPEC,dibas_frb_file);
		}
		//fwrite(U,sizeof(short int),N_CHANS_SPEC,dibas_frb_file);
		//fwrite(V,sizeof(short int),N_CHANS_SPEC,dibas_frb_file);
		//char a = Full_Stokes[0];
		//fwrite(&a,sizeof(a),1,dibas_frb_file);
		//fwrite(&Full_Stokes[0],sizeof(a),1,dibas_frb_file);
		//sleep(0.1);
		/*hashpipe_status_lock_safe(&st);
		hputi4(st.buf, "OUT_FIRST", XX[0]);
		hashpipe_status_unlock_safe(&st);
		*/
		dibas_frb_input_databuf_set_free(db,block_idx);
		block_idx = (block_idx + 1) % db->header.n_block;
		mcnt++;
                printf("mcnt is: %ld\n",long(mcnt));
	        printf("FILE_SIZE_NOW_MB is %lf\n",FILE_SIZE_NOW_MB);
                if((mcnt!=0)&&(mcnt%10==0)&&(FILE_SIZE_NOW_MB==0)){
                  printf("Renaming file\n");
                  //result = rename(f_fil,fin_fil);
                  //if(result==0){
                  //printf("File renamed");
                  //} else {
                  //printf("Could not rename");
                  //}
                  printf("Command: %s\n",command);
                  system(command);
                }

		//Will exit if thread has been cancelled
		pthread_testcancel();
	}
	fclose(dibas_frb_file);
	return THREAD_OK;
}

static hashpipe_thread_desc_t dibas_frb_output_thread = {
	name: "dibas_frb_output_thread",
	skey: "OUTSTAT",
	init: NULL, 
	run:  run,
	ibuf_desc: {dibas_frb_input_databuf_create},
	obuf_desc: {NULL}
};

static __attribute__((constructor)) void ctor()
{
	register_hashpipe_thread(&dibas_frb_output_thread);
}

