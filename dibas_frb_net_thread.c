/*
 * dibas_frb_net_thread.c
 *
 *  
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include "hashpipe.h"
#include "dibas_frb_databuf.h"

double MJD;

//defining a struct of type hashpipe_udp_params as defined in hashpipe_udp.h
static struct hashpipe_udp_params params;

static int init(hashpipe_thread_args_t * args)
{
        hashpipe_status_t st = args->st;
        //strcpy(params.bindhost,"127.0.0.1");
        //selecting a port to listen to
        //params.bindport = 5009;
        strcpy(params.bindhost,"10.0.1.38");
        //selecting a port to listen to
        params.bindport = 10000;
        params.packet_size = 0;
        hashpipe_udp_init(&params);
        hashpipe_status_lock_safe(&st);
        hputi8(st.buf, "NPACKETS", 0);
        hputi8(st.buf, "NBYTES", 0);
        hashpipe_status_unlock_safe(&st);
        return 0;

}

double UTC2JD(double year, double month, double day){
        double jd;
        double a;
        a = floor((14-month)/12);
        year = year+4800-a;
        month = month+12*a-3;
        jd = day + floor((153*month+2)/5)+365*year+floor(year/4)-floor(year/100)+floor(year/400)-32045;
        return jd;
}

/*double UTC2MJD(double year,double month, double day){
	double mjd;
	day = day + month*30;
	mjd = int(365.25*(year-1)) - 678576 - int(0.01*(year-1))+ day;
	return mjd;
	
}*/
/* MJulDat():

	""" calculates the modified Julian day from the computer time signal """

	global tFill

	usec = time.time()-int(time.time())
	jy, m, d, h, m, s, wd, day, sz = time.gmtime()
	jy = jy - 1
	ja = int(0.01 * jy)
	day = day + h/24.0 + m/1440.0 + (s+usec)/86400.0
	mjd = int(365.25*jy) - 678576 - ja + int(0.25*ja) + day
	#if tFill.TIMESYS == "UTC":
	#    if tFill.TAIUTC != None:
	#        mjd -= (tFill.TAIUTC + tFill.GPSTAI) / 86400.0

	return mjd*/

static void *run(hashpipe_thread_args_t * args){

    	dibas_frb_input_databuf_t *db  = (dibas_frb_input_databuf_t *)args->obuf;
    	hashpipe_status_t st = args->st;
    	const char * status_key = args->thread_desc->skey;

    	/* Main loop */
    	int i, rv,input,n;
    	uint64_t mcnt = 0;
    	int block_idx = 0;
	unsigned char frm_header[N_CHAN_PER_PACK]; //char header information from packet
        unsigned long header; // for 64 bit counter     
        unsigned char xx_tmp[N_CHAN_PER_PACK];
        unsigned char yy_tmp[N_CHAN_PER_PACK];
        unsigned char xx[N_CHANS_SPEC];
        unsigned char yy[N_CHANS_SPEC];
        unsigned char packet_tmp[PKTSIZE];
        unsigned long SEQ=0;
        unsigned long LAST_SEQ=0;
        unsigned long CHANNEL;
        unsigned long n_pkt_rcv,pkt_loss;
        bool first_pkt=1;
        //bool first_cache=1;
        double pkt_loss_rate;

	double Year, Month, Day;
	double jd;
	//double MJD;
	time_t timep;
	struct tm *p;
	struct timeval currenttime;

    	unsigned char *frm_data;
    	frm_data = (unsigned char *)malloc(PKTSIZE*sizeof(unsigned char));
	//unsigned char *data;
	//frm_data = (unsigned char *)malloc(PKTSIZE*sizeof(unsigned char));

    	uint64_t npackets = 0; //number of received packets
    	uint64_t nbytes = 0;  //number of received bytes
 

    	while (run_threads()){
		// get the local time when start a new block
        	time(&timep);
        	p=gmtime(&timep);
        	Year=p->tm_year+1900;
        	Month=p->tm_mon+1;
        	Day=p->tm_mday;
        	jd = UTC2JD(Year, Month, Day);
        	MJD=jd+(double)((p->tm_hour-12)/24.0)
        	                       +(double)(p->tm_min/1440.0)
        	                       +(double)(p->tm_sec/86400.0)
        	                       +(double)(currenttime.tv_usec/86400.0/1000000.0)
        	                        -(double)2400000.5;
        	printf("MJD time of packets is %lf\n",MJD);
		// update the status of net thread
        	hashpipe_status_lock_safe(&st);
        	hputs(st.buf, status_key, "waiting");
        	hputi4(st.buf, "NETBKOUT", block_idx);
		hputi8(st.buf,"NETMCNT",mcnt);
        	hashpipe_status_unlock_safe(&st);
 
        	// Wait for data
        	/* Wait for new block to be free, then clear it
        	 * if necessary and fill its header with new values.
        	 */
        	while ((rv=dibas_frb_input_databuf_wait_free(db, block_idx)) 
        	        != HASHPIPE_OK) {
        	    if (rv==HASHPIPE_TIMEOUT) {
        	        hashpipe_status_lock_safe(&st);
        	        hputs(st.buf, status_key, "blocked");
        	        hashpipe_status_unlock_safe(&st);
        	        continue;
        	    } else {
        	        hashpipe_error(__FUNCTION__, "error waiting for free databuf");
        	        pthread_exit(NULL);
        	        break;
        	    }
        	}

        	hashpipe_status_lock_safe(&st);
        	hputs(st.buf, status_key, "receiving");
        	hashpipe_status_unlock_safe(&st);
		for(int i=0;i<PAGE_SIZE*N_PACKETS_PER_SPEC;i){
		        n = recvfrom(params.sock,frm_data,PKTSIZE*sizeof(unsigned char),0,0,0);
			if(n>0){
				i++;
				npackets++;
				nbytes += n;

				for(int j=0;j<N_CHAN_PER_PACK;j++){
		                        frm_header[j]=frm_data[j*3];
        		                xx_tmp[j]=frm_data[j*3+1];
		                        yy_tmp[j]=frm_data[j*3+2];
					//printf("yy_tmp[%i] is: %u\n",j,xx_tmp[j]);
		                }
				memcpy(&header, frm_header,8*sizeof(unsigned char));
        	        	CHANNEL = header & 0x3fff;
				SEQ = header >> 10;
				//printf("SEQ is: %lu\n",SEQ);

				if(N_IFs_FIL==1){
        	        		for(int k=0;k<N_CHAN_PER_PACK;k++){
						//do (x+y)/2 to save single polarization
        	                		//xx[k+int(CHANNEL/4)] = (xx_tmp[k]+yy_tmp[k])/2;
        	                		xx[k+int(CHANNEL/4)] = (xx_tmp[k]/2+yy_tmp[k]/2);
						//printf("xx[k+int(CHANNEL/4)] is: %u\n",xx[k+int(CHANNEL/4)]);	
        		        	}
				}
				else if(N_IFs_FIL==2){
        	        		for(int k=0;k<N_CHAN_PER_PACK;k++){
						//save 2 polarizations
        	                		xx[k+int(CHANNEL/4)] = xx_tmp[k];
		                	        yy[k+int(CHANNEL/4)] = yy_tmp[k];
				}

        	                //if(first_pkt == 1){
				// neglect the packets loss if it is the first cache
				if(mcnt == 0){
        	                        pkt_loss=0;
        	                        LAST_SEQ = SEQ;}
				else{
        		                pkt_loss += SEQ - (LAST_SEQ+1);
					//i += (SEQ - LAST_SEQ);
        	        	        pkt_loss_rate = (double)pkt_loss/(double)npackets*100.0;
					//printf("seq is : %lu\n",SEQ);
					//printf("last seq is : %lu\n",LAST_SEQ);
        	                	LAST_SEQ = SEQ;}


		                if(CHANNEL == 15360){
					if(N_IFs_FIL==1){
						memcpy(db->block[block_idx].XX+(npackets%PAGE_SIZE)*N_CHANS_SPEC,xx,N_CHANS_SPEC*sizeof(unsigned char));
					}else if(N_IFs_FIL==2){
						memcpy(db->block[block_idx].XX+(npackets%PAGE_SIZE)*N_CHANS_SPEC,xx,N_CHANS_SPEC*sizeof(unsigned char));
						memcpy(db->block[block_idx].YY+(npackets%PAGE_SIZE)*N_CHANS_SPEC,yy,N_CHANS_SPEC*sizeof(unsigned char));
					}
				}
				//	printf("XX[2200] is: %u\n", db->block[block_idx].XX[(npackets%PAGE_SIZE)*N_CHANS_SPEC+2200]);	
      				hashpipe_status_lock_safe(&st);
        			hputi8(st.buf, "NPACKETS", npackets);
			        hputi8(st.buf, "NBYTES", nbytes);
				hputi8(st.buf,"PKTLOSS",pkt_loss);
			      	hputr8(st.buf,"LOSSRATE",pkt_loss_rate);
			      	hashpipe_status_unlock_safe(&st);
		
				}else{continue;}
			}

		}
	
		// Mark block as full	
		if(dibas_frb_input_databuf_set_filled(db, block_idx) != HASHPIPE_OK) {
			hashpipe_error(__FUNCTION__, "error waiting for databuf filled call");
		        pthread_exit(NULL);
		}
		db->block[block_idx].header.mcnt = mcnt;
        	block_idx = (block_idx + 1) % db->header.n_block;
		mcnt++;
        	/* Will exit if thread has been cancelled */
        	pthread_testcancel();
    	}
    	// Thread success!
	return THREAD_OK;
}

static hashpipe_thread_desc_t dibas_frb_net_thread = {
    name: "dibas_frb_net_thread",
    skey: "NETSTAT",
    init: init,
    run:  run,
    ibuf_desc: {NULL},
    obuf_desc: {dibas_frb_input_databuf_create}
};

static __attribute__((constructor)) void ctor()
{
  register_hashpipe_thread(&dibas_frb_net_thread);
}
