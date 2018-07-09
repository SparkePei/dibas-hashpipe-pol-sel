#!/bin/bash
#./send_packet.py &
#hashpipe -p ./dibas_frb_hashpipe -I 0   dibas_frb_net_thread -c 10 dibas_frb_gpu_thread -c 11   dibas_frb_output_thread -c 12 
hashpipe -p ./dibas_frb_hashpipe -I 0  -c 7 dibas_frb_net_thread -c 9 dibas_frb_output_thread 

