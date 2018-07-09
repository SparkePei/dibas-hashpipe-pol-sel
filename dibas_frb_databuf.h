#include <stdint.h>
#include <stdio.h>
#include "hashpipe.h"
#include "hashpipe_databuf.h"

#define CACHE_ALIGNMENT         256
#define N_INPUT_BLOCKS          3 
#define N_OUTPUT_BLOCKS         3
#define PAGE_SIZE		131072

#define N_CHAN_PER_PACK			256		//number of channels per packet
#define N_PACKETS_PER_SPEC		16		//number of peakets per spectrum
#define N_BYTES_PACK			1		// number of bytes per datapoint in packets 
#define N_POLS_PACK			2		//number of polarizations in packets
#define N_IFs_FIL			1		//number of Ifs in filterbank file
#define N_BYTES_FIL			1		// number of bytes in fil
#define N_BYTES_COUNTER			256		// number bytes of counter
#define N_CHANS_SPEC			N_CHAN_PER_PACK * N_PACKETS_PER_SPEC
#define DATA_SIZE_PACK			N_CHAN_PER_PACK * N_POLS_PACK * N_BYTES_PACK
#define PKTSIZE				N_CHAN_PER_PACK * N_POLS_PACK * N_BYTES_PACK + N_BYTES_COUNTER  //768 bytes

//extern double MJD; // MJD time
// Used to pad after hashpipe_databuf_t to maintain cache alignment
typedef uint8_t hashpipe_databuf_cache_alignment[
  CACHE_ALIGNMENT - (sizeof(hashpipe_databuf_t)%CACHE_ALIGNMENT)
];

/* INPUT BUFFER STRUCTURES
  */
typedef struct dibas_frb_input_block_header {
   uint64_t mcnt;                    // mcount of first PACK
} dibas_frb_input_block_header_t;

typedef uint8_t dibas_frb_input_header_cache_alignment[
   CACHE_ALIGNMENT - (sizeof(dibas_frb_input_block_header_t)%CACHE_ALIGNMENT)
];

typedef struct dibas_frb_input_block {
   dibas_frb_input_block_header_t header;
   dibas_frb_input_header_cache_alignment padding; // Maintain cache alignment
   unsigned char XX[PAGE_SIZE*N_CHANS_SPEC*sizeof(unsigned char)];//65536*4096Bytes
   unsigned char YY[PAGE_SIZE*N_CHANS_SPEC*sizeof(unsigned char)];//65536*4096Bytes
} dibas_frb_input_block_t;

typedef struct dibas_frb_input_databuf {
   hashpipe_databuf_t header;
   hashpipe_databuf_cache_alignment padding; // Maintain cache alignment
   dibas_frb_input_block_t block[N_INPUT_BLOCKS];
} dibas_frb_input_databuf_t;


/*
  * OUTPUT BUFFER STRUCTURES
  */
typedef struct dibas_frb_output_block_header {
   uint64_t mcnt;
} dibas_frb_output_block_header_t;

typedef uint8_t dibas_frb_output_header_cache_alignment[
   CACHE_ALIGNMENT - (sizeof(dibas_frb_output_block_header_t)%CACHE_ALIGNMENT)
];

typedef struct dibas_frb_output_block {
   dibas_frb_output_block_header_t header;
   dibas_frb_output_header_cache_alignment padding; // Maintain cache alignment
//   uint64_t sum;
//   unsigned char XX[N_CHANS_SPEC]; //4096 bytes
//   unsigned char YY[N_CHANS_SPEC]; //4096 bytes
   //short int XY_REAL[N_CHANS_SPEC];
   //short int XY_IMAG[N_CHANS_SPEC];

} dibas_frb_output_block_t;

typedef struct dibas_frb_output_databuf {
   hashpipe_databuf_t header;
   hashpipe_databuf_cache_alignment padding; // Maintain cache alignment
   dibas_frb_output_block_t block[N_OUTPUT_BLOCKS];
} dibas_frb_output_databuf_t;

/*
 * INPUT BUFFER FUNCTIONS
 */
hashpipe_databuf_t *dibas_frb_input_databuf_create(int instance_id, int databuf_id);

static inline dibas_frb_input_databuf_t *dibas_frb_input_databuf_attach(int instance_id, int databuf_id)
{
    return (dibas_frb_input_databuf_t *)hashpipe_databuf_attach(instance_id, databuf_id);
}

static inline int dibas_frb_input_databuf_detach(dibas_frb_input_databuf_t *d)
{
    return hashpipe_databuf_detach((hashpipe_databuf_t *)d);
}

static inline void dibas_frb_input_databuf_clear(dibas_frb_input_databuf_t *d)
{
    hashpipe_databuf_clear((hashpipe_databuf_t *)d);
}

static inline int dibas_frb_input_databuf_block_status(dibas_frb_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_block_status((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_input_databuf_total_status(dibas_frb_input_databuf_t *d)
{
    return hashpipe_databuf_total_status((hashpipe_databuf_t *)d);
}

static inline int dibas_frb_input_databuf_wait_free(dibas_frb_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_input_databuf_busywait_free(dibas_frb_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_input_databuf_wait_filled(dibas_frb_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_input_databuf_busywait_filled(dibas_frb_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_input_databuf_set_free(dibas_frb_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_free((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_input_databuf_set_filled(dibas_frb_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_filled((hashpipe_databuf_t *)d, block_id);
}

/*
 * OUTPUT BUFFER FUNCTIONS
 */

hashpipe_databuf_t *dibas_frb_output_databuf_create(int instance_id, int databuf_id);

static inline void dibas_frb_output_databuf_clear(dibas_frb_output_databuf_t *d)
{
    hashpipe_databuf_clear((hashpipe_databuf_t *)d);
}

static inline dibas_frb_output_databuf_t *dibas_frb_output_databuf_attach(int instance_id, int databuf_id)
{
    return (dibas_frb_output_databuf_t *)hashpipe_databuf_attach(instance_id, databuf_id);
}

static inline int dibas_frb_output_databuf_detach(dibas_frb_output_databuf_t *d)
{
    return hashpipe_databuf_detach((hashpipe_databuf_t *)d);
}

static inline int dibas_frb_output_databuf_block_status(dibas_frb_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_block_status((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_output_databuf_total_status(dibas_frb_output_databuf_t *d)
{
    return hashpipe_databuf_total_status((hashpipe_databuf_t *)d);
}

static inline int dibas_frb_output_databuf_wait_free(dibas_frb_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_output_databuf_busywait_free(dibas_frb_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_free((hashpipe_databuf_t *)d, block_id);
}
static inline int dibas_frb_output_databuf_wait_filled(dibas_frb_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_output_databuf_busywait_filled(dibas_frb_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_output_databuf_set_free(dibas_frb_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_free((hashpipe_databuf_t *)d, block_id);
}

static inline int dibas_frb_output_databuf_set_filled(dibas_frb_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_filled((hashpipe_databuf_t *)d, block_id);
}


