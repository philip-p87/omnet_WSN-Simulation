simple Server
{
    parameters:
        @display("i=device/antennatower_l");
        @signal[receiveBegin](type="long");  // increases with each new frame arriving to the server and drops to 0 if the channel becomes finally idle
        @signal[receive](type="long");  // for successful receptions (non-collisions): 1 at the start of the reception, 0 at the end of the reception
        @signal[collision](type="long"); // the number of collided frames at the beginning of the collision period
        @signal[collisionLength](type="simtime_t");  // the length of the last collision period at the end of the collision period
        @signal[channelState](type="long");

        @statistic[serverChannelState](source="channelState";title="Channel state";enum="IDLE=0,TRANSMISSION=1,COLLISION=2";record=vector);
        @statistic[receiveBegin](source="receiveBegin"; record=vector?; interpolationmode=sample-hold; title="receive begin");
        @statistic[channelUtilization](source="timeavg(receive)"; record=last; interpolationmode=linear; title="channel utilization");
        @statistic[collisionMultiplicity](source=collision; record=vector?,histogram; title="collision multiplicity");
        @statistic[collisionLength](record=vector?,histogram,mean,sum,max; title="collision length");
        @statistic[receivedFrames](source="sum(receive)"; record=last; title="received frames");
        @statistic[collidedFrames](source="sum(collision)"; record=last; title="collided frames");
        int minHosts;
        int maxHosts;
        int HostStepSize;
        int minPackets;
        int maxPackets;
        bool WC_MODE;
        bool PAUSE;
        bool LONG_PAUSE;
        bool short_period;
        int max_iteration_number;	//number of restarting transmission at random
        double RX_TX_switching_time @unit(us);
        double processing_delay @unit(us);
        double cs_sensitivity @unit(us);
        double listen_time;
        int carrierSenseMode;
        string vectorOutput;
        
        int mode;
        int submode;
        bool receiverInitiated;
        int haltOnPacketNumberSent;
        int haltOnDeadlines;
        double txRate @unit(bps);         // transmission rate
        volatile int pkLenBits @unit(b);  // packet length in bits
        volatile int ACKLenBits @unit(b);  // packet length in bits
        double deadline;
        
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
        double ExternalInterferenceActiveMin;
		double ExternalInterferenceActiveMax;
        
		//submode variables
		double submode_steps;
		double deadline_start;
		double deadline_stop;
		double RX_TX_switching_time_start;
		double RX_TX_switching_time_stop;
		double packet_length_start;	
		double packet_length_stop;
		
    gates:
        input in @directIn;
}

