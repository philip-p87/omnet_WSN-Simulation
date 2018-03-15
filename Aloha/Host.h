#ifndef __ALOHA_HOST_H_
#define __ALOHA_HOST_H_



#include <omnetpp.h>
#include <time.h>
#include "Server.h"

namespace aloha {



/**
 * Aloha host; see NED file for more info.
 */
class Host : public cSimpleModule
{
  private:
    // parameters
    simtime_t delay_time_stamp;
    double message_delay;        //delay from triggering the node until the end of the first successfully received packet
    double tx_time;
    double rx_time;                 //time a node spent in rx mode. Used to calculate energy consumption (includes switching times)
    double sleep_time;
    double txRate;
    cPar *iaTime;
    cPar *pkLenBits;
    double packet_duration;
    cPar *ACKLenBits;
    int mode;
    int submode;
    bool WC_MODE;       //WC: Worst case mode flag (sequences start randomized)
    bool PAUSE;
    bool LONG_PAUSE;
    double deadline;

    int minHosts;
    int maxHosts;
    int HostStepSize;
    int minPackets;
    int maxPackets;
    int currentActiveNodes;
    int packetNumberMax;                    //max packets in a sequence
    bool send_n_packets;                    //true: hosts send n packets

    int packetNumber;                       //counting variable: counts packets in a sequence (includes also skipped ones (packetNumberSkipped))
    int remainingPackets;                   //(n - packetNumber) used for LONG_PAUSE
    int packetNumberSkipped;                //number of packets that have been skipped because channel was busy during cs
    int pkCounter;
    int pkRepetition;
    char pkname[40];
    bool ACK_received;
    bool receiverInitiated;

    double periodTime;
    double longestPeriodTime;               //größte Periode der N knoten (bei zb 25 Knoten ist das Periode 25)
    double intersequence_pause;             //N*Pi+Li

    double randomTimeMin2;
    double randomTimeMax2;
    double minPeriod;
    bool isGui;

    //carrier sense variables ///////////////////////////////////////////////////////
    int carrierSenseMode;
    double listen_time;
    double ACK_duration;
    double RX_TX_switching_time;
    double processing_delay;
    double cs_sensitivity;                  //receiver sensitivity: time a signal must be active for the node to detect it
    double cs_duration;                     //carrier sense time
    bool carrier_sense_done;                //mode4: finished sensing the channel
    bool channel_was_busy_during_sensing;   //mode4: channel was busy during sensing
    //bool skip_current_packet;             //mode4: channel was busy during sensing, skip packet
    bool cs_active;                         //true: node is currently doing a carrier sense
    bool channel_is_busy;                   //set by server: true: channel is used, false: channel is free

    //CSMA mode 5 variables ///////////////////////////////////////////////////////
    double CSMA_backoff_time;
    int CSMA_max_retransmissions;
    int CSMA_max_backoffs;
    int CSMA_packets_failed;                // number of packets that failed because no ACK was received
    int CSMA_backoff_counter;               // number of back-off retries
    int CSMA_wc_min;
    int CSMA_wc_max;
    int CSMA_mode;
    double CSMA_DIFS;
    simtime_t CSMA_backoff_finished_time;   //mode 2: timestamp, when backoff is done and packet should be transmitted
    simtime_t CSMA_backoff_started_time;
    bool old_channel_state;                 //buffer for inform hosts()
    simtime_t remaining_backoff_time;
    double CSMA_deadline;
    double CSMA_get_backoff();              //returns random back-off time

    //TDMA variables ///////////////////////////////////////////////////////
    double TDMA_beacon_size;
    double TDMA_beacon_rate;
    double TDMA_slot_tolerance;
    volatile double TDMA_slot_duration;
    double TDMA_beacon_duration;
    double TDMA_cycle_duration;                 //mode 8 only
    double TDMA_slot_duration_drifted;          //with clock drift
    double TDMA_cycle_duration_drifted;
    double TDMA_guard_time;
    double TDMA_cycles_per_deadline;
    double TDMA_beacon_reception_rate;          //one beacon every x seconds
    bool TDMA_ready_for_sync;                   //true: receive next beacon and then sync
    double TDMA_default_clock_drift;
    cMessage *TDMA_sync_request;                //called periodically to set TDMA_ready_for_sync
    double TDMA_beacons_received_this_sequence; //number of beacons received in this sequence (from 0 to TDMA_cycles_per_deadline); used to calc rx_time (reseted every finish_tx())
    double TDMA_beacons_missed;                 //counts the number of beacons that a node did not receive successfully (interference) when it wants to sync
    bool TDMA_desync;                           //when too many packets have been lost, the node desyncs
    double TDMA_rx_sync_overhead;                      //stores rx_time spent for listening for beacons
    double TDMA_rx_sync_overhead_timestamp;
    void TDMA_calculate_guard_time();
    bool TDMA_guard_time_automatic_flag;        //true: calc guardtime automatically (was -1)
    //double TDMA_beacon_counter;                 //used to determine whether beacon should be received or not
    //double TDMA_beacon_counter_max;             //when TDMA_beacon_counter reaches this value, a beacon is received; calculated using TDMA_beacon_reception_rate

    //RTNS mode variables ///////////////////////////////////////////////////////
    double RTNS_reliability;
    bool RTNS_reliability_max;
    double RTNS_m;
    double RTNS_deadline2;
    cPar *RTNS_packet_length2;
    bool RTNS_use_different_node_types;
    double RTNS_node_type_ratio;
    double RTNS_tmin1, RTNS_tmin2, RTNS_tmax1, RTNS_tmax2;
    int RTNS_node_type;        //type of this node (to distinguish between different node types if RTNS_use_different_node_types == true)
    double data_duration2;
    double RTNS_get_random_pause();
    int RTNS_get_node_type();  //returns 1 or 2


    //submode variables ///////////////////////////////////////////////////////
    double submode_steps;
    double deadline_start;
    double deadline_stop;
    double RX_TX_switching_time_start;
    double RX_TX_switching_time_stop;
    double packet_length_start;
    double packet_length_stop;


    bool ClockDriftEnabled;
    double ClockDriftRangePercent;
    bool ClockDriftPlotMode;
    int ClockDriftPlotStepNumber;
    int ClockDriftPlotCurrentStepNumber;                // counter for current iteration, used to calculate the size of clockdrift
    double ClockDriftPlotDriftRange_temp;
    double Clock_drift_current_value;                   //current clock drift that has been (randomnly) selected between [0,ClockDriftRangePercent]
    double periodTime_original;                         //period time without any drift etc. The clock drift modifies periodTime
    double Clock_drift_calculate_period(double period); //takes period and calculates drift
    void Clock_drift_generate_new_random_drift();       //shuffles new random values for clock drift
    //double periodTimeDrifted;

    bool ExternalInterferenceEnable;
    double ExternalInterferenceDutyCycle;
    bool ExternalInterferencePlotMode;
    int ExternalInterferencePlotStepNumber;
    double external_interference_current_duty_cycle;
    int external_interference_current_step_number;

    // state variables, event pointers etc
    Server *server;
    cMessage *endTxEvent;
    cMessage *endCarrierSenseEvent;
    cMessage *ACKTimeoutEvent;                  //no ACK was received
    bool ACK_is_used;                           //true if scheme uses an ACK
    bool carrier_sense_is_used;                 //true if schemes uses carrier sensing
    bool deadline_is_used;
    bool transmission_successful;               // true: packet was successfully delivered, false: sequence ended without success -> used for counting successful and failed packets
    enum {IDLE=0, TRANSMIT_END=1, WAITED_PERIOD=2} state;
    simsignal_t stateSignal;


    void calculatePeriods(int maxPaketNumber, int numberOfTransmittingNodes);
    void sendPacket(char *name);
    bool check_and_transmit();  //checks for deadline etc. and then transmits a packet: returns false if transmitting failed
    double max(double a, double b);
    void reset_variables();
    void calc_sequence_stats();         //rx_time, tx_time, delay, etc
    void finish_tx();   //ends sequence

  public:
    Host();
    virtual ~Host();

    void activate();                    //will activate the node: starts sending
    void change_transmission_scheme(int packetNumberMax_, int currentActiveNodes_, double RX_TX_switching_time_, double deadline_, double packet_duration_);  //changes the transmission scheme to next iteration. Called by Server node
    void next_plot_step(int iteration_number);

    void stop_transmission();
    void start_at_random();    //triggers transmission cycle at random start time

    void packet_handler(cMessage *msg);
    void ack_was_lost(bool only_ack_was_lost);  //only_ack_was_lost = true: message has been received successfully, but ACK got lost
                                                //false: message was corrupt

    void update_channel_state(bool channel_busy);  //used by server to inform host, whether channel is currently busy or not
    void carrier_sense_callback();  //called if host subscribed to carrier sense list and

    void update_plot_step_number(bool external_interference, int stepnumber);

    void TDMA_synchronize();

    void wakeup();  //for receiver-initiated topology


  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    simtime_t getNextInterSequenceTime();
};

}; //namespace

#endif

