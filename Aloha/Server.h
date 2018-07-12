#ifndef __ALOHA_SERVER_H_
#define __ALOHA_SERVER_H_

#define MAX_NUMBER_OF_NODES     200
#define MAX_ITERATIONS          200

#include <omnetpp.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <list>
#include <algorithm>
#include <string>

struct packetCount                    //for internal use (not statistics): temporarily counts packets within a sequence
{
    int packetsReceivedTotal;         //number of received packets (does also count defective packets)
    int packetsReceivedGood;          //number of packets that arrived without collision
    int firstPacketIndex;             //Index of the first packet of a message that has been received
    int packetsSkipped;               //number of packets that have been skipped
    //long double avg_delay;            //average delay from triggering a node to the end of first successful reception
};

struct result_t                       //for statistics
{
    long int numberOfNodes;
    unsigned long int numberOfPackets;
    unsigned long int packetsGood;
    unsigned long int packetsTotal;
    unsigned long packetsLost;
    unsigned long int sequencesGood;    //good sequences
    unsigned long int sequencesLost;        //failed sequences
    unsigned long int sequencesTotal;       //number of sequences received in total
    unsigned long int sequencesGood2;
    unsigned long int sequencesLost2;        //type 2 nodes (submode 11 & 12)
    unsigned long int sequencesTotal2;
    double deadline_missed_counter;
    long int ACKsLost;
    long int ACKsTransmitted;               //total number of ACKs that were transmitted
    long int packetsSkipped;
    double RTNS_reliability;
    int min;
    int max;
    float mean;
    float stddev;
    double deadline;

    double delay_avg;
    double delay_min;
    double delay_max;
    double rx_time_avg;                    //time that nodes were in receiver mode
    double rx_time_min;
    double rx_time_max;
    double tx_time_avg;
    double tx_time_min;
    double tx_time_max;
    double sleep_time_avg;                 //time between transmissions in which node can power down and sleep (does not take transition times into account, i.e., wakeup and power down times)

    double packets_per_sequence_avg;        //number of transmissions per sequence on average
    double packets_skipped_avg;
};





namespace aloha {

/**
 * Aloha server; see NED file for more info.
 */
class Server : public cSimpleModule
{
  private:
    // state variables, event pointers
    bool channelBusy;                                       // true: channel is busy
    bool receivingPaket;                                    // true: packet is being received at the moment (and causes channel to be busy)
    bool sendingACK;                                        // true: ACK is currently being sent
    cMessage *endRxEvent;
    double txRate;
    cPar *pkLenBits;
    cMessage *restartEvent;
    cMessage *ACK_startEvent;
    cMessage *ACK_finishedEvent;
    cMessage *CSMA_waited_DIFS;
    cMessage *modeSwitchFinishedEvent;                           //scheduled after ack und resets is_switching_mode to false
    cPar *ACKLenBits;
    double ACK_duration;
    bool send_n_packets;                                    //true: hosts send n packets
    double packet_duration;                                 //duration of a packet
    int carrierSenseMode;
    std::string vectorOutput;
    double deadline;
    double deadline2; //RTNS mode == 7 if two node types are used
    bool fixedDeadline;

    simtime_t recvStartTime;
    enum {IDLE=0, TRANSMISSION=1, COLLISION=2};
    simsignal_t channelStateSignal;

    packetCount packetList[MAX_NUMBER_OF_NODES];            //array containing information about how many pakets were received, lost, etc. of every node
    int nodesFinishedTransmissionIDs[MAX_NUMBER_OF_NODES];     //list containing node IDs that have finished since last rxEndEvent
    int nodesFinishedTransmissionNumber;                    //number of nodes that have finished since last rxEndEvent
    int minHosts;
    int maxHosts;
    int HostStepSize;
    int minPackets;
    int maxPackets;
    int mode;
    int submode;
    bool WC_MODE;
    bool PAUSE;
    bool LONG_PAUSE;
    unsigned long int sequencesPerIteration;
    unsigned long int sequencesPerIteration_original;
    unsigned long int deadlinesPerIteration;
    unsigned long int cyclesPerIteration;
    time_t timer;
    time_t timer_next;
    struct tm time_old;
    struct tm time_new;

    cMessage *simtime_event;
    int currentActiveNodes;                             //the number of nodes which are currently active (in this iteration)
    int currentPacketCountPerSequence;                   //the number of packets within a sequence (k <= n) (in this iteration)

    unsigned long int packetsGood;               //number of packets (each sequence contains a number of packets)
    unsigned long int packetsLost;
    unsigned long int packetsTotal;
    unsigned long int sequencesGood;                 //number of received sequences (also contains non successful sequences)
    unsigned long int sequencesLost;                 //number of non successful sequences
    unsigned long int sequencesTotal;
    unsigned long int sequencesGood2;
    unsigned long int sequencesLost2;                //type 2 nodes (submode 11 & 12)
    unsigned long int sequencesTotal2;
    long int currentCollisionNumFrames;
    long int ACKsLost;
    long int packetsSkipped;
    long int packetsSkippedTotal;
    double temp_rx_time;
    double temp_tx_time;
    double temp_delay;
    double temp_deadline;
    double temp_sleep_time;
    double temp_packets_per_sequence;
    double temp_packets_skipped;                //per sequence
    long int total_packets_transmitted;         //used for calculating the energy
    long int total_packets_skipped;
    long int total_acks_transmitted;

    //CSMA mode 5 variables ///////////////////////////////////////////////////////
    double CSMA_backoff_time;
    int CSMA_max_retransmissions;
    int CSMA_max_backoffs;
    int CSMA_wc_min;
    int CSMA_wc_max;
    int CSMA_mode;
    double CSMA_DIFS;
    bool last_state;
    double CSMA_deadline;

    //TDMA variables ///////////////////////////////////////////////////////
    double TDMA_beacon_size;
    double TDMA_beacon_rate;
    double TDMA_slot_tolerance;
    double TDMA_slot_duration;
    double TDMA_beacon_duration;
    cMessage *TDMA_sync_start;
    cMessage *TDMA_sync_finished;  //selfmessage when sync has been transmitted
    bool TDMA_sync_successful;
    bool TDMA_performing_sync;
    double TDMA_guard_time;
    double TDMA_cycles_per_deadline;
    double TDMA_current_beacon_per_deadline;    //counter from 1 to TDMA_cycles_per_deadline
    bool TDMA_restart_beacon_after_new_iteration;         //used to restart beaconing interval when a new iteration is started
    double TDMA_default_clock_drift;
    void TDMA_calculate_guard_time();           //calculate TDMA_guard_time

    //RTNS mode variables ///////////////////////////////////////////////////////
    double RTNS_reliability;
    bool RTNS_reliability_max;
    double RTNS_m;
    double RTNS_deadline2;
    cPar *RTNS_packet_length2;
    bool RTNS_use_different_node_types;
    double RTNS_node_type_ratio;
    int RTNS_node_type;
    int RTNS_get_node_type(int ID_);  //returns 1 or 2

    //submode variables ///////////////////////////////////////////////////////
    double submode_steps;
    double deadline_start;
    double deadline_stop;
    double RX_TX_switching_time_start;
    double RX_TX_switching_time_stop;
    double submode_step_current;
    double packet_length_start;
    double packet_length_stop;


    bool lastPacketGood;        //flag: true: last packet (lastID) was received successfully
                                //      false: last packet collided. Used for collision counting
    int lastID;        //contains the ID of the last sender.Used for collision counting
    char *fullPackets;          //contains the ID of all Nodes that finished the package count. Is cleared after rxEndEvent

    // statistics
    simsignal_t receiveBeginSignal;
    simsignal_t receiveSignal;
    simsignal_t collisionLengthSignal;
    simsignal_t collisionSignal;

    double listen_time;
    double RX_TX_switching_time;
    double processing_delay;
    double cs_sensitivity;
    double cs_duration;                 //Time that host sense the channel for activity


    bool ClockDriftEnabled;
    double ClockDriftRangePercent;
    bool ClockDriftPlotMode;
    int ClockDriftPlotStepNumber;
    int ClockDriftPlotCurrentStepNumber;           // counter for current iteration, used to calculate the size of clockdrift
    double ClockDriftPlotDriftRange_temp;
    double Clock_drift_current_value;             //current clock drift that has been (randomnly) selected between [0,ClockDriftRangePercent]
    double periodTime_original;                     //period time without any drift etc. The clock drift modifies periodTime
    bool deadline_missed;
    double deadline_missed_counter;                 //used for mode 7 only (all other modes implement this in host)

    //external interference
    bool ExternalInterferenceEnable;
    double ExternalInterferenceDutyCycle;
    double ExternalInterferenceCurrentDutyCycle;
    bool ExternalInterferencePlotMode;
    int ExternalInterferencePlotStepNumber;
    int ExternalInterferencePlotCurrentStepNumber; //iteration counter
    cMessage *ExternalInterferenceEvent;
    double ExternalInterference_active_time;    //used for getting the right duty cycle
    double ExternalInterference_idle_time;
    double ExternalInterference_active_min;     //min max times for shuffling active time
    double ExternalInterference_active_max;
    bool ExternalInterferenceEvent_is_active;   //true: channel is distorted
    double ExternalInterferenceActiveMin;
    double ExternalInterferenceActiveMax;


    //recording container
    cLongHistogram *hopCountStats;      //counts received packet IDs --> todo can probably be removed
    result_t statsAll[MAX_ITERATIONS+2];            //hopCountStats of all iterations
    int storage_index;
    int storage_iteration_number;
    cDoubleHistogram *rx_time;           //receive times (receiver active of hosts)
    cDoubleHistogram *tx_time;           //transmit times of hosts
    cDoubleHistogram *delay;             //message delay until successful reception
    cDoubleHistogram *packets_per_sequence;
    cDoubleHistogram *packets_skipped;
    cDoubleHistogram *sleep_time;


    unsigned int repetition, senderId, ACK_host_id;
    bool ACK_was_lost;
    bool ACK_is_used;                           //true if scheme uses an ACK
    bool carrier_sense_is_used;                 //true if schemes uses carrier sensing
    bool deadline_is_used;                      //for RTNS-based modes only
    bool ackScheduled;  //avoid scheduling acks multiple times (happens for lmax shorter than t_set)
    bool is_switching_mode;                     //true if server switches mode, e.g., from rx to tx, during RX_TX_switching_time
    bool receiverInitiated;
    unsigned long int restartEvent_counter;  //counts number of restart events to limit simulation iterations
    double restartInterval;         //restart interval in seconds. Used to dynamically adjust the restart
    bool next_iteration_flag;       //flag that will cause start of new iteration at next restartEvent
    bool isGui;

    //NEW
    bool nodes_successful_list[MAX_NUMBER_OF_NODES+1];  //list to count successful and failed receptions
    //TODO
    //node i informs server when it starts sending (-> sequences_total++; clear nodes_successful_list[i]=false;)
    //if packet is received: sequences_received_good++ if nodes_successful_list[i]==false; nodes_successful_list[i]=true;



    void initVariables();
    void sendACKPacket(char *name, int host_id);
    void sendBeacon(int number);  //for TDMA (mode == 11 only) "number" = #of beacon within deadline
    void inform_hosts_about_channel_state(bool channel_busy, bool wait_DIFS); // for persistent CSMA only

    std::list<int> carrier_sense_subscriber_list;
    void carrier_sense_update();    //used to update all subscribed hosts about changed channel state
    void calculate_restart_interval();  //calculates restart interval for receiver-initiated event

  public:
    Server();
    virtual ~Server();

    void start_next_iteration();
    void carrier_sense_subscribe(bool subscribe, int id); //for Hosts: subscribe/de-subscribe for (carrier sense) channel updates
    bool carrier_sense_simple();    //will return current channel state (carrier sensing of just a single time point, not an interval) true: channel busy

    //notify server that node with ID has finished its sequence with "success" -> used to count successful and failed sequences (ack-based modes only)
    void notify_about_transmission(int ID, bool success, double rx_time_ = 0, double tx_time_ = 0);

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();
};

}; //namespace

#endif

