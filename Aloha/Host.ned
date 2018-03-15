simple Host
{
    parameters:
        @signal[state](type="long");
        @statistic[radioState](source="state";title="Radio state";enum="IDLE=0,TRANSMIT=1";record=vector);
        double txRate @unit(bps);         // transmission rate
        volatile int pkLenBits @unit(b);  // packet length in bits
        int mode;
        int submode;
        bool receiverInitiated;
        bool WC_MODE;
        bool PAUSE;
        bool LONG_PAUSE;
        bool short_period;
        
        int minHosts;
        int maxHosts;
        int HostStepSize;
        int minPackets;
        int maxPackets;
        double listen_time;
        int carrierSenseMode;
        double RX_TX_switching_time @unit(us);
        double processing_delay @unit(us);
        double cs_sensitivity @unit(us);
        volatile int ACKLenBits @unit(b);  // packet length in bits
        double deadline;
        //int haltOnPacketNumberSent;		  //Stopp nach sovielen Paketen
        
        double CSMA_backoff_time @unit(us);
		int CSMA_max_retransmissions;
		int CSMA_max_backoffs;
		int CSMA_wc_min;
		int CSMA_wc_max;
		int CSMA_mode;
		double CSMA_DIFS;
		double CSMA_deadline;
		
		double TDMA_beacon_size;
		double TDMA_beacon_rate;
		double TDMA_slot_tolerance;
		double TDMA_slot_duration;
		double TDMA_guard_time;
		double TDMA_cycles_per_deadline;
		double TDMA_beacon_reception_rate;
		double TDMA_default_clock_drift;
		
		double RTNS_reliability;
		bool RTNS_reliability_max;
		double RTNS_m;							
        double RTNS_deadline2;				
        int RTNS_packet_length2	@unit(b);			
        bool RTNS_use_different_node_types;
        double RTNS_node_type_ratio;	
		
		bool ClockDriftEnabled;
        double ClockDriftRangePercent;
        bool ClockDriftPlotMode;
        int ClockDriftPlotStepNumber;
        
        bool ExternalInterferenceEnable;
        double ExternalInterferenceDutyCycle;
        bool ExternalInterferencePlotMode;
        int ExternalInterferencePlotStepNumber;
        
		//submode variables
		double submode_steps;
		double deadline_start;
		double deadline_stop;
		double RX_TX_switching_time_start;
		double RX_TX_switching_time_stop;
		double packet_length_start;
		double packet_length_stop;
		
        @display("i=device/pc_s");

    gates:        
        input in @directIn;
}

