network Aloha
{
    parameters:
        int minHosts;
        int maxHosts;  // number of hosts
        int HostStepSize;
        int minPackets;
        int maxPackets;
        int mode;		//mode of operation: 1 normal single packet; 2=random multiple packets; 3=our algorithm
        int submode;
        bool receiverInitiated;
        double txRate @unit(bps);  // transmission rate
        volatile int pkLenBits @unit(b);  // packet length in bits
        volatile int ACKLenBits @unit(b);  // packet length in bits
        @display("bgi=background/terrain,s;bgb=718,342");
        int haltOnPacketNumberSent;
        int haltOnDeadlines;
        bool WC_MODE;
        bool PAUSE; 
        bool LONG_PAUSE;
        int max_iteration_number;
        bool shortPeriod;
        double deadline;
        
        bool ClockDriftEnabled;
        double ClockDriftRangePercent;
        int ClockDriftPlotStepNumber;
        bool ClockDriftPlotMode;
        
        bool ExternalInterferenceEnable;
        double ExternalInterferenceDutyCycle;
        int ExternalInterferencePlotStepNumber;
        bool ExternalInterferencePlotMode;
        double ExternalInterferenceActiveMin;
    	double ExternalInterferenceActiveMax;
        
        double RX_TX_switching_time @unit(us);
        double processing_delay @unit(us);
        double cs_sensitivity @unit(us);
        double listen_time;
        int carrierSenseMode;
        
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

		//submode variables
		double submode_steps;
		double deadline_start;
		double deadline_stop;
		double RX_TX_switching_time_start;
		double RX_TX_switching_time_stop;
		double packet_length_start;
		double packet_length_stop;

    submodules:
        server: Server {
            mode = mode;
            submode = submode;
            receiverInitiated = receiverInitiated;
            WC_MODE = WC_MODE;
            PAUSE = PAUSE;
            LONG_PAUSE = LONG_PAUSE;
            minHosts = minHosts;
            maxHosts = maxHosts;
            HostStepSize = HostStepSize;
            minPackets = minPackets;
            maxPackets = maxPackets;
            short_period = shortPeriod;
            haltOnPacketNumberSent = haltOnPacketNumberSent;
            haltOnDeadlines = haltOnDeadlines;
            txRate = txRate;
            pkLenBits = pkLenBits;
            max_iteration_number = max_iteration_number;
            RX_TX_switching_time = RX_TX_switching_time;
            processing_delay = processing_delay;
            cs_sensitivity = cs_sensitivity;
            listen_time = listen_time;
            ACKLenBits = ACKLenBits;
            carrierSenseMode = carrierSenseMode;
            deadline = deadline;
            
            CSMA_backoff_time = CSMA_backoff_time;
            CSMA_max_retransmissions = CSMA_max_retransmissions;
            CSMA_max_backoffs = CSMA_max_backoffs;
            CSMA_wc_min = CSMA_wc_min;
            CSMA_wc_max = CSMA_wc_max;
            CSMA_mode = CSMA_mode;
            CSMA_DIFS = CSMA_DIFS;
            CSMA_deadline = CSMA_deadline;
            
            TDMA_beacon_size = TDMA_beacon_size;
            TDMA_beacon_rate = TDMA_beacon_rate;
            TDMA_slot_tolerance = TDMA_slot_tolerance;
			TDMA_slot_duration = TDMA_slot_duration;
			TDMA_guard_time = TDMA_guard_time;
			TDMA_cycles_per_deadline = TDMA_cycles_per_deadline;
			TDMA_default_clock_drift = TDMA_default_clock_drift;
            
            ClockDriftEnabled = ClockDriftEnabled;
        	ClockDriftRangePercent = ClockDriftRangePercent;
        	ClockDriftPlotMode = ClockDriftPlotMode;
        	ClockDriftPlotStepNumber = ClockDriftPlotStepNumber;
        	
        	ExternalInterferenceEnable = ExternalInterferenceEnable;
        	ExternalInterferenceDutyCycle = ExternalInterferenceDutyCycle;
        	ExternalInterferencePlotMode = ExternalInterferencePlotMode;
        	ExternalInterferencePlotStepNumber = ExternalInterferencePlotStepNumber;
        	ExternalInterferenceActiveMin = ExternalInterferenceActiveMin;
        	ExternalInterferenceActiveMax = ExternalInterferenceActiveMax;
        	
        	RTNS_reliability = RTNS_reliability;
        	RTNS_reliability_max = RTNS_reliability_max;
        	RTNS_m = RTNS_m;
        	RTNS_deadline2 = RTNS_deadline2;
        	RTNS_packet_length2	= RTNS_packet_length2;
        	RTNS_use_different_node_types = RTNS_use_different_node_types;
        	RTNS_node_type_ratio = RTNS_node_type_ratio;
        	
			//submode variables
    		submode_steps = submode_steps;
    		deadline_start = deadline_start;
    		deadline_stop = deadline_stop;
    		RX_TX_switching_time_start = RX_TX_switching_time_start;
    		RX_TX_switching_time_stop = RX_TX_switching_time_stop;
			packet_length_start = packet_length_start;
			packet_length_stop = packet_length_stop;

            @display("p=357,65");
        }
        host[maxHosts]: Host {
            txRate = txRate;
            pkLenBits = pkLenBits;
            mode = mode;
            submode = submode;
            receiverInitiated = receiverInitiated;
            WC_MODE = WC_MODE;
            PAUSE = PAUSE;
            LONG_PAUSE = LONG_PAUSE;
            short_period = shortPeriod;
            minHosts = minHosts;
            maxHosts = maxHosts;
            HostStepSize = HostStepSize;
            minPackets = minPackets;
            maxPackets = maxPackets;
            RX_TX_switching_time = RX_TX_switching_time;
            processing_delay = processing_delay;
            cs_sensitivity = cs_sensitivity;
            listen_time = listen_time;
            ACKLenBits = ACKLenBits;
            carrierSenseMode = carrierSenseMode;
            deadline = deadline;
            
            CSMA_backoff_time = CSMA_backoff_time;
            CSMA_max_retransmissions = CSMA_max_retransmissions;
            CSMA_max_backoffs = CSMA_max_backoffs;
            CSMA_wc_min = CSMA_wc_min;
            CSMA_wc_max = CSMA_wc_max;
            CSMA_mode = CSMA_mode;
            CSMA_DIFS = CSMA_DIFS;
            CSMA_deadline = CSMA_deadline;
            
            TDMA_beacon_size = TDMA_beacon_size;
            TDMA_beacon_rate = TDMA_beacon_rate;
            TDMA_slot_tolerance = TDMA_slot_tolerance;
			TDMA_slot_duration = TDMA_slot_duration;
			TDMA_guard_time = TDMA_guard_time;
			TDMA_cycles_per_deadline = TDMA_cycles_per_deadline;
			TDMA_beacon_reception_rate = TDMA_beacon_reception_rate;
			TDMA_default_clock_drift = TDMA_default_clock_drift;
            
            ClockDriftEnabled = ClockDriftEnabled;
        	ClockDriftRangePercent = ClockDriftRangePercent;
        	ClockDriftPlotMode = ClockDriftPlotMode;
        	ClockDriftPlotStepNumber = ClockDriftPlotStepNumber;
        	
        	ExternalInterferenceEnable = ExternalInterferenceEnable;
        	ExternalInterferenceDutyCycle = ExternalInterferenceDutyCycle;
        	ExternalInterferencePlotMode = ExternalInterferencePlotMode;
        	ExternalInterferencePlotStepNumber = ExternalInterferencePlotStepNumber;
        	
        	RTNS_reliability = RTNS_reliability;
        	RTNS_reliability_max = RTNS_reliability_max;
        	RTNS_m = RTNS_m;
        	RTNS_deadline2 = RTNS_deadline2;
        	RTNS_packet_length2	= RTNS_packet_length2;
        	RTNS_use_different_node_types = RTNS_use_different_node_types;
        	RTNS_node_type_ratio = RTNS_node_type_ratio;
        	
			//submode variables
    		submode_steps = submode_steps;
    		deadline_start = deadline_start;
    		deadline_stop = deadline_stop;
    		RX_TX_switching_time_start = RX_TX_switching_time_start;
    		RX_TX_switching_time_stop = RX_TX_switching_time_stop;  
			packet_length_start = packet_length_start;
			packet_length_stop = packet_length_stop;      	
        }
}

