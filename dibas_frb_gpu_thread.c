/*dibas_frb_gpu_thread.c
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include "hashpipe.h"
#include "dibas_frb_databuf.h"

static void *run(hashpipe_thread_args_t * args)
{
    // Local aliases to shorten access to args fields
    dibas_frb_input_databuf_t *db_in = (dibas_frb_input_databuf_t *)args->ibuf;
    dibas_frb_output_databuf_t *db_out = (dibas_frb_output_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char * status_key = args->thread_desc->skey;

    int rv,a,b,c;
    uint64_t mcnt=0;
    uint64_t pkt_loss=0;
    int curblock_in=0;
    int curblock_out=0;
	
	unsigned char frm_header[N_CHAN_PER_PACK]; //char header information from packet
	unsigned long header; // for 64 bit counter	
	int packet_number=0;
	unsigned char xx_tmp[N_CHAN_PER_PACK];
	unsigned char yy_tmp[N_CHAN_PER_PACK];
	unsigned char xx[N_CHANS_SPEC];
	unsigned char yy[N_CHANS_SPEC];
	unsigned char packet_tmp[PKTSIZE];
	unsigned long SEQ=0;
	unsigned long LAST_SEQ=0;
	unsigned long CHANNEL;
	long long n_pkt_rcv;
	bool first_pkt=1;
	double pkt_loss_rate;
/*
	I = X*X + Y*Y
	Q = X*X - Y*Y
	U = 2 * XY_real
	V = -2 * XY_img
*/
/*	struct Full_Stokes{
		short int I[N_CHANS_SPEC];
		short int Q[N_CHANS_SPEC];
		short int U[N_CHANS_SPEC];
		short int V[N_CHANS_SPEC]; 
	}*/
    while (run_threads()) {

        hashpipe_status_lock_safe(&st);
        hputi4(st.buf, "GPUBLKIN", curblock_in);
        hputs(st.buf, status_key, "waiting");
        hputi4(st.buf, "GPUBKOUT", curblock_out);
	hputi8(st.buf,"GPUMCNT",mcnt);
	hputi8(st.buf,"PKTLOSS",pkt_loss);
        hashpipe_status_unlock_safe(&st);

        // Wait for new input block to be filled
        while ((rv=dibas_frb_input_databuf_wait_filled(db_in, curblock_in)) != HASHPIPE_OK) {
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

        // Got a new data block, update status and determine how to handle it
        /*hashpipe_status_lock_safe(&st);
        hputu8(st.buf, "GPUMCNT", db_in->block[curblock_in].header.mcnt);
        hashpipe_status_unlock_safe(&st);*/

        // Wait for new output block to be free
        while ((rv=dibas_frb_output_databuf_wait_free(db_out, curblock_out)) != HASHPIPE_OK) {
            if (rv==HASHPIPE_TIMEOUT) {
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "blocked gpu out");
                hashpipe_status_unlock_safe(&st);
                continue;
            } else {
                hashpipe_error(__FUNCTION__, "error waiting for free databuf");
                pthread_exit(NULL);
                break;
            }
        }

        // Note processing status
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "processing gpu");
	hgeti8(st.buf,"NPACKETS", &n_pkt_rcv);
        hashpipe_status_unlock_safe(&st);

		
		memcpy(packet_tmp,db_in->block[curblock_in].data,PKTSIZE*sizeof(unsigned char));
		for(int j=0;j<N_CHAN_PER_PACK;j++){
			frm_header[j]=packet_tmp[j*3];
			xx_tmp[j]=packet_tmp[j*3+1];
			yy_tmp[j]=packet_tmp[j*3+2];
		}
		memcpy(&header, frm_header,8*sizeof(unsigned char));
		//header = ((unsigned long)frm_header[0])+((unsigned long)frm_header[1]<<8)+((unsigned long)frm_header[2]<<16)+((unsigned long)frm_header[3]<<24)+((unsigned long)frm_header[4]<<32)+((unsigned long)frm_header[5]<<40)+((unsigned long)frm_header[6]<<48)+((unsigned long)frm_header[7]<<56);
		CHANNEL = header & 0x3fff;
		/*for(int k=0;k<N_CHAN_PER_PACK;k++){
			xx[k+int(CHANNEL/4)] = xx_tmp[k];
			yy[k+int(CHANNEL/4)] = yy_tmp[k];
		}*/
		packet_number++;
		//printf("CHANNEL is:%ld\n",CHANNEL);
		if(CHANNEL == 15360){
			memcpy(db_out->block[curblock_out].XX,xx,N_CHANS_SPEC*sizeof(unsigned char));
			memcpy(db_out->block[curblock_out].YY,yy,N_CHANS_SPEC*sizeof(unsigned char));
			SEQ = header >> 14;
			if (first_pkt == 1){
				pkt_loss=0;
				pkt_loss_rate = 0.0;
				first_pkt = 0;
				LAST_SEQ = SEQ;}else{
			pkt_loss += SEQ - (LAST_SEQ+1);
			pkt_loss_rate = (double)pkt_loss/(double)n_pkt_rcv*100.0;
			LAST_SEQ = SEQ;}
			//printf("SEQ is: %lu\n",SEQ);
			//printf("header is: %lu\n",header);
			packet_number=0;
		//}
        //db_out->block[curblock_out].Stokes_Full=db_in->block[curblock_in].data;
		//memcpy(db_out->block[curblock_out].Stokes_Full,db_in->block[curblock_in].data,PKTSIZE*sizeof(char));
        // Mark output block as full and advance
        //dibas_frb_output_databuf_set_filled(db_out, curblock_out);
        //curblock_out = (curblock_out + 1) % db_out->header.n_block;
		}
        // Mark input block as free and advance
        dibas_frb_input_databuf_set_free(db_in, curblock_in);
        curblock_in = (curblock_in + 1) % db_in->header.n_block;
	mcnt++;
	//display sum in status
	hashpipe_status_lock_safe(&st);
	hputi8(st.buf,"PKTLOSS",pkt_loss);
	hputr8(st.buf,"LOSSRATE",pkt_loss_rate);
	hashpipe_status_unlock_safe(&st);
        /* Check for cancel */
        pthread_testcancel();
    }
    return THREAD_OK;
}

static hashpipe_thread_desc_t dibas_frb_gpu_thread = {
    name: "dibas_frb_gpu_thread",
    skey: "GPUSTAT",
    init: NULL,
    run:  run,
    ibuf_desc: {dibas_frb_input_databuf_create},
    obuf_desc: {dibas_frb_output_databuf_create}
};

static __attribute__((constructor)) void ctor()
{
  register_hashpipe_thread(&dibas_frb_gpu_thread);
}

