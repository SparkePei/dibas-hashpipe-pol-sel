# dibas_frb_hashpipe

This code used to receive the data from one Gb Ethernet and write the data to disc in filterbank format.

There are three threads can run in parallel. dibas_frb_net_thread will collect the packets from one Gb Ethernet. dibas_frb_gpu_thread will re-order the data and convert it a type appropriate for processing on the GPUs, we reserved this function for the future real-time FRBs detection, and just processed data format conversion in here. dibas_frb_output_thread can write the data to disc in filterbank format.

Before you run this code, you need to install Hashpipe first, you can download it [from here](http://astro.berkeley.edu/~davidm/hashpipe.git). This code wrote by David Macmahon from UC Berkeley.

You can monitor the status of packets receiving, data formatting and storage from a ruby code which wrote by David Macmahon. But first you need install rb-Hashpipe [from here](https://github.com/david-macmahon/rb-hashpipe.git).

The installation of dibas_frb_hashpipe is very easy, you just need to do make and make install. If everything is working, you can run dibas_frb_init.sh to collect data, fig. 1 is shown this code running.
![hashpipe print](dibas-hashpipe.png)
> fig. 1 dibas-hashpipe run-time monitor 
