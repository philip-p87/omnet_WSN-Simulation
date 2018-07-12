#include "Host.h"
#include "Server.h"
#include "periods.h"




namespace aloha {

Define_Module(Host);

Host::Host()
{
}


Host::~Host()
{
    cancelAndDelete(endTxEvent);
    cancelAndDelete(endCarrierSenseEvent);
    cancelAndDelete(ACKTimeoutEvent);
    cancelAndDelete(TDMA_sync_request);
}


void Host::initialize()
{
    cModule *server_m;
    stateSignal = registerSignal("state");
    server_m = simulation.getModuleByPath("server");
    if (!server_m) error("server not found");
    server = check_and_cast<Server *>(server_m);

    //get variables
    txRate = par("txRate");
    pkLenBits = &par("pkLenBits");
    ACKLenBits = &par("ACKLenBits");
    mode = par("mode");
    submode = par("submode");
    WC_MODE = par("WC_MODE");           //worst case mode flag
    PAUSE = par("PAUSE");
    LONG_PAUSE = par("LONG_PAUSE");
    minHosts = par("minHosts");
    maxHosts = par("maxHosts");
    HostStepSize = par("HostStepSize");
    minPackets = par("minPackets");
    maxPackets = par("maxPackets");
    listen_time = par("listen_time");
    RX_TX_switching_time = par("RX_TX_switching_time");
    RX_TX_switching_time /= 1000000;    //convert to µs
    processing_delay = par("processing_delay");
    processing_delay /= 1000000;    //convert to µs
    cs_sensitivity = par("cs_sensitivity");
    cs_sensitivity /= 1000000;    //convert to µs
    packet_duration = pkLenBits->doubleValue() / txRate;
    ACK_duration = ACKLenBits->longValue() / txRate;
    receiverInitiated = par("receiverInitiated");
    carrierSenseMode = par("carrierSenseMode");
    deadline = par("deadline");
    deadline /= 1000;
    RTNS_deadline1 = deadline;
    fixedDeadline = par("fixedDeadline");

    CSMA_backoff_time = par("CSMA_backoff_time");
    CSMA_backoff_time /= 1000000;    //convert to µs
    CSMA_max_retransmissions = par("CSMA_max_retransmissions");
    CSMA_max_backoffs = par("CSMA_max_backoffs");
    CSMA_backoff_counter = 0;
    CSMA_packets_failed = 0;
    CSMA_wc_min = par("CSMA_wc_min");
    CSMA_wc_max = par("CSMA_wc_max");
    CSMA_mode = par("CSMA_mode");
    CSMA_DIFS = par("CSMA_DIFS");
    CSMA_DIFS /= 1000000;
    CSMA_deadline = par("CSMA_deadline");
    CSMA_deadline /= 1000;

    TDMA_beacon_size = par("TDMA_beacon_size");
    if(TDMA_beacon_size == -1) TDMA_beacon_size = pkLenBits->doubleValue();
    TDMA_beacon_rate = par("TDMA_beacon_rate");
    TDMA_beacon_duration = TDMA_beacon_size / txRate;
    TDMA_slot_tolerance = par("TDMA_slot_tolerance");
    TDMA_slot_tolerance /= 100;
    TDMA_slot_duration = par("TDMA_slot_duration");
    TDMA_guard_time = par("TDMA_guard_time");
    if(TDMA_guard_time < 0) TDMA_guard_time_automatic_flag = true;
    else {TDMA_guard_time_automatic_flag = false; TDMA_guard_time /= 1e6;} //conversion to microseconds
    TDMA_cycles_per_deadline = par("TDMA_cycles_per_deadline");
    TDMA_beacon_reception_rate = par("TDMA_beacon_reception_rate");
    TDMA_default_clock_drift = par("TDMA_default_clock_drift");
    TDMA_default_clock_drift /= 1e6;

    RTNS_reliability = par("RTNS_reliability");
    RTNS_reliability_max = par("RTNS_reliability_max");
    RTNS_m = par("RTNS_m");
    RTNS_deadline2 = par("RTNS_deadline2");
    RTNS_packet_length2 = &par("RTNS_packet_length2");
    RTNS_use_different_node_types = par("RTNS_use_different_node_types");
    RTNS_node_type_ratio = par("RTNS_node_type_ratio");
    RTNS_node_type_ratio *= 100;
    data_duration2 = RTNS_packet_length2->doubleValue()/ txRate;

    //external interference and clock drift
    ClockDriftEnabled = par("ClockDriftEnabled");
    ClockDriftRangePercent = par("ClockDriftRangePercent");
    ClockDriftRangePercent /= 100;// conversion to percent
    ClockDriftPlotMode = par("ClockDriftPlotMode");
    ClockDriftPlotStepNumber = par("ClockDriftPlotStepNumber");
    //ClockDriftPlotStepNumber++;
    ExternalInterferenceEnable = par("ExternalInterferenceEnable");
    ExternalInterferenceDutyCycle = par("ExternalInterferenceDutyCycle");
    ExternalInterferencePlotMode = par("ExternalInterferencePlotMode");
    ExternalInterferencePlotStepNumber = par("ExternalInterferencePlotStepNumber");
    external_interference_current_step_number = 0;

    //submode variables
    submode_steps = par("submode_steps");
    deadline_start = par("deadline_start");
    deadline_start /= 1000;
    deadline_stop = par("deadline_stop");
    deadline_stop /= 1000;
    RX_TX_switching_time_start = par("RX_TX_switching_time_start");
    RX_TX_switching_time_start /= 1e6;
    RX_TX_switching_time_stop = par("RX_TX_switching_time_stop");
    RX_TX_switching_time_stop /= 1e6;
    packet_length_start = par("packet_length_start");
    packet_length_stop = par("packet_length_stop");
    submode_step_current = 0;


    if(ClockDriftEnabled == false) ClockDriftPlotMode = false;
    if(ExternalInterferenceEnable == false) ExternalInterferencePlotMode = false;
    if(ClockDriftPlotMode == true) ExternalInterferencePlotMode = false; //both modes are exclusive
    ClockDriftPlotCurrentStepNumber = 0;

    //validate input values
    if(minHosts == -1)
        minHosts = maxHosts;
    if(minPackets > minHosts)        //prevent sending more than n packets
        minPackets = minHosts;
    if(minPackets > maxHosts)
        minPackets = maxHosts;
    if(minHosts > maxHosts)         //for user convenience: automatically sets maxHosts, when only minHosts is set
        maxHosts = minHosts;
    if(minPackets > maxPackets)
        maxPackets = minPackets;

    currentActiveNodes = minHosts;
    if(minPackets == -1)
        packetNumberMax = currentActiveNodes;
    else
        packetNumberMax = minPackets;

    //all modes that use ACKs
    if(mode == 4 || mode == 5 || mode == 6 || mode == 9 || mode == 10 || mode == 11)
        ACK_is_used = true;
    else
        ACK_is_used = false;


    //submode configuration
    if(submode == 4)//increase RX_TX_switching_time
    {
        RX_TX_switching_time = RX_TX_switching_time_start;
        if(RX_TX_switching_time == 0)
            RX_TX_switching_time = 1e-9;
    }
    else if(submode == 5)//increase deadline
    {
        deadline = deadline_start;
    }
    else if(submode == 6)//increase packet length
    {
        packet_duration = packet_length_start / txRate;
    }
    else if(submode == 11 || submode == 12)
    {
        //if(mode != 7)
        //{
        //    EV << "submode not compatible with current mode" << endl;
        //    scheduleAt(simTime()-1000, endTxEvent);//create some error
        //}
        RTNS_use_different_node_types = true;
        receiverInitiated = false;

        RTNS_node_type = RTNS_get_node_type();
        if(RTNS_node_type == 2)
        {
            if(submode == 11)
                packet_duration = packet_length_start/txRate;
            else if(submode == 12)
                deadline = deadline_start;
        }
    }


    //all modes that use carrier sensing
    if(mode == 4 || mode == 5 || mode == 6 || mode == 10)
    {
        cs_duration = 2 * cs_sensitivity +  RX_TX_switching_time + processing_delay;
        carrier_sense_is_used = true;
    }
    else
        carrier_sense_is_used = false;

    //all modes that use deadlines
    if(mode == 7 || mode == 9  || mode == 10)       //RTNS based modes
    {
        deadline_is_used = true;
    }
    else if(mode == 5 && CSMA_deadline > 0)         //CSMA if deadline is enabled
    {
        deadline = CSMA_deadline;
        deadline_is_used = true;
    }
    else
        deadline_is_used = false;

    //overwrite some values for mode 4 (paper reliable CSMA (DSD2016))
    if(mode == 4 || mode == 6)
    {
        if(minPackets == -1)
            packetNumberMax = 2 * currentActiveNodes - 1;
    }
    else if(mode == 5) //CSMA
    {
        receiverInitiated = true; //not implemented yet
        LONG_PAUSE = false;
        packetNumberMax = CSMA_max_retransmissions + CSMA_max_backoffs;
    }
    else if(mode == 1 || mode == 2 || mode == 3 || mode == 7 || mode == 9)
    {
        if(submode == 1 && ExternalInterferenceEnable == false && ClockDriftEnabled == false)
        {
            packetNumberMax = 1;
        }
        else if(submode == 1 && (ExternalInterferenceEnable == true || ClockDriftEnabled == true))
        {
            packetNumberMax = maxPackets;
        }
    }

    if(mode == 7 || mode == 9) //RTNS
    {
        packetNumberMax = maxPackets;
    }
    else if (mode == 8) //TDMA
    {
        packetNumberMax = 1;
    }

    else if(mode==10) //RTCSA
    {
        LONG_PAUSE = false;
        packetNumberMax = maxPackets;
    }
    else if(mode == 11) //TDMA with ACK
    {
        packetNumberMax = TDMA_cycles_per_deadline;
        receiverInitiated = false;   //temporary, will also support other non receiver-initiated mode later
        TDMA_ready_for_sync = true;  //default value (first ever beacon has to be received)
        TDMA_beacons_missed = 0;
        TDMA_rx_sync_overhead_timestamp = simTime().dbl();
        if(TDMA_guard_time_automatic_flag)
            TDMA_calculate_guard_time();
    }


    //initialize variables
    calculatePeriods(packetNumberMax, currentActiveNodes);
    reset_variables();
    state = IDLE;
    endCarrierSenseEvent = new cMessage("endCarrierSense");
    endTxEvent = new cMessage("send/endTx");
    ACKTimeoutEvent = new cMessage("ACKTimeoutEvent");
    TDMA_sync_request = new cMessage("syncRequest");

    //GUI
    isGui = ev.isGUI();
    //debug
    isGui = false;

    if (isGui)
        getDisplayString().setTagArg("t",2,"#808000");

    if(receiverInitiated == true)
    {
        //do nothing, server does activating
    }

    //start transmission at random time
    else if(getIndex() < currentActiveNodes)
    {
        start_at_random();
    }

    //debug
    //std::cout << "host " << this->getIndex() << " n:" << currentActiveNodes << "  k:" << packetNumberMax << "  deadline:" << deadline << "  RX_TX_switching_time:" << RX_TX_switching_time << std::endl;
}

void Host::start_at_random()
{
    Enter_Method("start_at_random()");

    //RTNS, RTNS with ACK, RTCSA
    if(mode == 7 || mode == 9 || mode == 10)
    {
        //generate random starting period
        periodTime = RTNS_get_random_pause();
        periodTime = Clock_drift_calculate_period(periodTime);

        delay_time_stamp = simTime();

        scheduleAt(simTime()+periodTime, endTxEvent);   //start right away without additional shuffling
    }
    //CSMA
    else if(mode == 5)
    {
        double shuffling_time = 0;

        //generate random starting period
        CSMA_packets_failed = 1;    //for better start shuffling
        periodTime = CSMA_get_backoff();
        periodTime = Clock_drift_calculate_period(periodTime);
        shuffling_time = uniform(0, deadline/20); //shuffling
        CSMA_packets_failed = 0;

        if(receiverInitiated)
            delay_time_stamp = simTime();           //actually add initial delay if receiver initiated
        else
            delay_time_stamp = simTime()+periodTime;//do not count initial back off as delay

        delay_time_stamp = simTime()+shuffling_time; //do not dad shuffling time to delay, but initial backoff

        EV<< "node: "<< this->getIndex() << "  random start: " << periodTime << endl;
        scheduleAt(simTime()+periodTime+shuffling_time, endTxEvent);
    }


    else if (mode == 1 || mode == 2 || mode == 3 || mode == 4)
    {
        double delay_temp = uniform(0,100*packet_duration);//periodTime_original);
        //double delay_temp = uniform(0,periodTime_original/2);
        //double delay_temp = uniform(0,deadline/2);
        delay_time_stamp = simTime() + delay_temp;
        scheduleAt(simTime() + delay_temp, endTxEvent);
        EV<< "node: "<< this->getIndex() << " random start delay: " << delay_temp <<  endl;
    }

    //all other options
    else if(mode != 8 && mode != 11) //TDMA is started manually, not automatically on start-up
    {
        double delay_temp = uniform(0,100*packet_duration);
        delay_time_stamp = simTime() + delay_temp;
        scheduleAt(simTime() + delay_temp, endTxEvent);
    }

}

void Host::activate()
{
    Enter_Method("activate()");

    reset_variables();
    state = IDLE;

    //stop all events before restarting
    this->cancelEvent(endTxEvent);
    this->cancelEvent(endCarrierSenseEvent);
    this->cancelEvent(ACKTimeoutEvent);

    EV<< "node: "<< this->getIndex() << " activate() called" << endl;

    start_at_random();
}

void Host::change_transmission_scheme(int packetNumberMax_, int currentActiveNodes_, double RX_TX_switching_time_, double deadline_, double packet_duration_, double submode_step_current_)
{
    Enter_Method("change_transmission_scheme()");

    EV << "node: "<< this->getIndex() << " change_transmission_scheme() called with maxPaketNumber: " << packetNumberMax_
                << "  numberOfTransmittingNodes: " << currentActiveNodes_
                << "  RX_TX_switching_time_:" << RX_TX_switching_time_*1e6 << " us"
                << "  deadline_:" << deadline_*1e3 << " ms"
                << "  packet_duration_:" << packet_duration_*1e6 << " us" << endl;

    //reset & update variables
    reset_variables();
    state = IDLE;
    TDMA_ready_for_sync = true;
    TDMA_beacons_missed = 0;
    TDMA_desync = false;
    packetNumberMax = packetNumberMax_;
    currentActiveNodes = currentActiveNodes_;
    RX_TX_switching_time = RX_TX_switching_time_;
    if(submode != 12)
        deadline = deadline_;
    if(submode != 11)
        packet_duration = packet_duration_;
    submode_step_current = submode_step_current_;
    if(carrier_sense_is_used)
        cs_duration = 2 * cs_sensitivity +  RX_TX_switching_time + processing_delay;
    if(TDMA_guard_time_automatic_flag)
        TDMA_calculate_guard_time();

    if (mode == 4 || mode == 6)
    {
        packetNumberMax = currentActiveNodes;
    }
    else if(mode == 5) //CSMA
    {
        packetNumberMax = CSMA_max_retransmissions + CSMA_max_backoffs;
    }

    if(ClockDriftPlotMode == true)
        ClockDriftPlotCurrentStepNumber++;

    calculatePeriods(packetNumberMax_, currentActiveNodes_);
}


void Host::stop_transmission()
{
    Enter_Method("stop_transmission()");
    cancelEvent(endTxEvent);
    cancelEvent(endCarrierSenseEvent);
    cancelEvent(TDMA_sync_request);
    cancelEvent(ACKTimeoutEvent);

    //reset variables
    reset_variables();
    state = IDLE;

    if (isGui)
    {
        getDisplayString().setTagArg("i",1,"");
        getDisplayString().setTagArg("t",0,"");
    }

    EV << "node: "<< this->getIndex() << " stop_transmission() called" << endl;
    //EV << "node: "<< this->getIndex() << " currentActiveNodes: " << currentActiveNodes << endl;
}

void Host::wakeup()
{
    Enter_Method("wakeup()");
    EV << "node: "<< this->getIndex() << " wakeup() called" << endl;

    //cancel all "old" activity from previous cycle and do a fresh restart
    stop_transmission();
    activate();

    //debug
    if(mode == 7 || mode == 9 || mode == 10)
    {
        EV << "node: "<< this->getIndex() << " tmin: " << RTNS_tmin1 << "  tmax: " << RTNS_tmax1 << endl;
    }
}


void Host::handleMessage(cMessage *msg)
{
    char buf[32];

    if(msg->isSelfMessage() == true) //message is a self-message
    {
        if(msg->isName("send/endTx") == true)
        {
            //packet message
            packet_handler(msg);
            return;
        }
///////////////////////////////////////////////////////////////////////
//No ACK has been received -> timeout
///////////////////////////////////////////////////////////////////////
        else if(msg->isName("ACKTimeoutEvent") == true)
        {
            EV << "node: " << this->getIndex() << "  did not receive ACK" << endl;
            if(mode == 5)
            {
                CSMA_packets_failed++;
                if(CSMA_packets_failed > CSMA_max_retransmissions)
                {
                    EV << "node: " << this->getIndex() << "  max transmissions reached -> sleep " << endl;
                    finish_tx();
                }
                else
                {
                    //schedule new packet
                    EV << "node: " << this->getIndex() << " CSMA_packets_failed++ (" << CSMA_packets_failed << "/" << CSMA_max_retransmissions << ")" << endl;
                    periodTime = CSMA_get_backoff();
                    periodTime = Clock_drift_calculate_period(periodTime);
                    scheduleAt(simTime() + periodTime, endTxEvent);

                    EV << "node: " << this->getIndex() << " scheduled next packet in (periodTime): " << periodTime <<endl;
                }
            }
            else if(packetNumber >= packetNumberMax)
            {
                EV << "node: " << this->getIndex() << "  transmitted all " << packetNumber << " packets  -> sleep" << endl;
                finish_tx();
            }
            else if(mode == 4 || mode == 6) //paper mode (reliable csma)
            {
                //mode 4: wait for one period (at this point, the node has already waited for tsen + tset + tdata + tproc + tset + tack)
                scheduleAt(simTime() + periodTime - cs_duration - 2*RX_TX_switching_time - packet_duration - processing_delay - ACK_duration, endTxEvent);
                EV << "node: " << this->getIndex() << " schedule next packet in (periodTime): " << periodTime <<endl;
            }
            else if(mode == 9)  //RTNS with ACK
            {
                periodTime = RTNS_get_random_pause();
                periodTime = Clock_drift_calculate_period(periodTime);
                scheduleAt(simTime() + periodTime - RX_TX_switching_time - packet_duration - processing_delay - ACK_duration, endTxEvent);
            }
            else if(mode == 10) //RTCSA (RTNS with ACK and cs)
            {
                periodTime = RTNS_get_random_pause();
                periodTime = Clock_drift_calculate_period(periodTime);
                scheduleAt(simTime() + periodTime - cs_duration - RX_TX_switching_time - packet_duration - RX_TX_switching_time - processing_delay - ACK_duration, endTxEvent);
            }
            else if(mode == 11) //TDMA with ACK
            {
                //No ACK has been received -> retransmit
                //calculate time to next slot in next cycle
                //                        time to next cycle within deadline         substract time to beginning of slot
                double time_to_next_cycle = deadline/TDMA_cycles_per_deadline - (packet_duration + RX_TX_switching_time + processing_delay + ACK_duration) - 1e-9;

                //debug
                //EV << "node: " << this->getIndex() << "  remaining slots in this cycle: " << currentActiveNodes - 1 - this->getIndex() << endl;
                EV << "node: " << this->getIndex() << "  time_to_next_cycle: " << time_to_next_cycle*1000 << "ms" << endl;
                EV << "node: " << this->getIndex() << "  time_to_next_cycle time_stamp: " << simTime() + time_to_next_cycle << endl;

                //schedule packet
                scheduleAt(simTime() + time_to_next_cycle, endTxEvent);
            }

            //reset some variables
            if(carrier_sense_is_used)
            {
                carrier_sense_done = false;
                channel_was_busy_during_sensing = false;
            }
        }

///////////////////////////////////////////////////////////////////////
//Carrier sensing has been completed
///////////////////////////////////////////////////////////////////////
        else if (msg->isName("endCarrierSense") == true)
        {

            if(carrierSenseMode == 1)
            {
                server->carrier_sense_subscribe(false, this->getIndex());
            }
            else
            {
                if(channel_was_busy_during_sensing == false)    //do not sense again, if channel has already been detected as busy
                    channel_was_busy_during_sensing = server->carrier_sense_simple();
            }

            EV << "node: " << this->getIndex() << " finished carrier sensing, channel was busy: " << channel_was_busy_during_sensing << endl;

            if(channel_was_busy_during_sensing == true)
            {
                //reset some variables
                carrier_sense_done = false;
                packetNumberSkipped++;
                packetNumber++;

                if(mode == 5)//CSMA
                {
                    CSMA_backoff_counter++;
                    if(CSMA_backoff_counter > CSMA_max_backoffs)    //back-off limit reached?
                    {
                        EV << "node: " << this->getIndex() << "  max back-off retries reached -> go back to sleep" << endl;
                        //end sequence
                        finish_tx();
                        return;
                    }

                    //schedule new packet
                    EV << "node: " << this->getIndex() << " CSMA_backoff_counter++ (" << CSMA_backoff_counter << "/" << CSMA_max_backoffs << ")" << endl;
                    periodTime = CSMA_get_backoff();
                    periodTime = Clock_drift_calculate_period(periodTime);
                    scheduleAt(simTime() + periodTime, endTxEvent);
                }
                else if(mode == 4 || mode == 6) //paper mode (reliable csma)
                {
                    //mode 4: wait for one period (at this point, the node has already waited for tsen + tset)
                    scheduleAt(simTime() + periodTime - cs_duration - RX_TX_switching_time, endTxEvent);
                }
                else if(mode == 10) //RTCSA
                {
                    periodTime = RTNS_get_random_pause();
                    periodTime = Clock_drift_calculate_period(periodTime);
                    scheduleAt(simTime() + periodTime - cs_duration - RX_TX_switching_time, endTxEvent);
                }

                //check if maximum retransmission numbers have been reached
                if(packetNumber >= packetNumberMax)
                {
                    EV << "node: " << this->getIndex() << "  transmitted all " << packetNumber << " packets" << endl;
                    finish_tx();
                }
                else
                {
                    EV << "node: " << this->getIndex() << " transmission failed -> back off: " << periodTime << endl;
                    EV << "node: " << this->getIndex() << "  packetNumber: " << packetNumber << "  packetNumberSkipped: " << packetNumberSkipped << endl;
                }
            }
            else
            {
                // carrier sense has been completed -> wait for RX_TX_switching_time and send packet
                scheduleAt(simTime() + RX_TX_switching_time, endTxEvent);
                carrier_sense_done = true;
            }

            cs_active = false;

        }//end: CS completed

///////////////////////////////////////////////////////////////////////
//set beacon/sync flag (mode == 11)
///////////////////////////////////////////////////////////////////////
        else if (msg->isName("syncRequest"))
        {
            TDMA_ready_for_sync = true;
            EV << "node: " << this->getIndex() << "  syncRequest: node is listening for sync" << endl;
            TDMA_rx_sync_overhead_timestamp = simTime().dbl();
            //TODO
            //node starts listening here for next beacon -> save timestamp for calculating rx_time
        }
    }//end: self-message

///////////////////////////////////////////////////////////////////////
//received ACK message
///////////////////////////////////////////////////////////////////////
    else if (msg->isName("ACK"))
    {
        bool corrupt_flag = msg->par("corrupt");
        delete msg;

        EV << "node: " << this->getIndex() << "  received ACK, state: ";
        if(corrupt_flag) EV << "corrupt" << endl;
        else EV << "ok" << endl;


        //ignore ACK if corrupt (do nothing here, a timeout handler will handle this event)
        if(corrupt_flag)
            return;


        // update network graphics
        if (isGui)
        {
            getDisplayString().setTagArg("i",1,"white");
            sprintf(buf, "Received ACK");
            getDisplayString().setTagArg("t",0,buf);
        }



        transmission_successful = true;
        finish_tx();
    }

///////////////////////////////////////////////////////////////////////
//received beacon (mode == 11) (Note: this is called at the end of beacon packet)
///////////////////////////////////////////////////////////////////////
    else if (msg->isName("beacon"))
    {
        bool corrupt_flag = msg->par("corrupt");
        int number = msg->par("number");    //used in case multiple beacons are sent within a deadline (TDMA_cycles_per_deadline)
        delete msg;
        double next_sync_request;           //for TDMA: time after which node listens for beacon

        //time to receive beacon?
        if(TDMA_ready_for_sync)  //receive and process beacon
        {
            TDMA_beacons_received_this_sequence++;  //used for calc rx_time (reseted every finish_tx())
            //TDMA_rx_sync_overhead += simTime().dbl() - TDMA_rx_sync_overhead_timestamp + RX_TX_switching_time; //time node was listening for beacon

            //console output
            //EV << "node: " << this->getIndex() << "  received beacon, state: ";
            //if(corrupt_flag) EV << "corrupt";
            //else EV << "ok";
            //EV << "   number: " << number << endl;

            //ignore if corrupt (do nothing here, a timeout handler will handle this event)
            if(corrupt_flag)
            {
                //beacon is corrupted -> try to receive next one
                TDMA_beacons_missed++; //count number of failed syncs
                EV << "node: " << this->getIndex() << " beacon corrupted -> reschedule wakeup to listen for next beacon (missed beacons=" << TDMA_beacons_missed << ")" << endl;

                //check if maximum number of failed syncs is exceeded
                //TODO if yes, then increase listening time
                //is node already desynchronized (too many beacons in a row could not be received?
                double TDMA_max_beacons_lost_before_desync = (TDMA_cycles_per_deadline+1);  //TODO change number?
                if(TDMA_beacons_missed >= TDMA_max_beacons_lost_before_desync)
                {
                    TDMA_desync = true;
                    EV << "node: " << this->getIndex() << "  desync!!!" << endl;

                    //alternative solution
                    next_sync_request = (deadline/TDMA_cycles_per_deadline - TDMA_beacon_duration - RX_TX_switching_time - TDMA_guard_time/2);
                    double delta = (next_sync_request) * (TDMA_beacons_missed - TDMA_max_beacons_lost_before_desync) * TDMA_default_clock_drift;
                    TDMA_rx_sync_overhead += TDMA_beacon_duration + 2*RX_TX_switching_time + TDMA_guard_time/2 + delta;
                    EV << "extra_time: " << delta*1000 << " ms" << endl;

                    /*next_sync_request = (deadline/TDMA_cycles_per_deadline - TDMA_beacon_duration - RX_TX_switching_time - TDMA_guard_time/2);
                    double delta = next_sync_request * (TDMA_beacons_missed - TDMA_max_beacons_lost_before_desync) * TDMA_default_clock_drift;
                    EV << "extra_time: " << delta*1000 << " ms" << endl;
                    next_sync_request -= delta;
                    if(next_sync_request <= 0)
                        next_sync_request = 1e-9;*/
                }
                else
                    TDMA_rx_sync_overhead += TDMA_beacon_duration + 2*RX_TX_switching_time + TDMA_guard_time/2;
                /*else
                    next_sync_request = (deadline/TDMA_cycles_per_deadline - TDMA_beacon_duration - RX_TX_switching_time - TDMA_guard_time/2);
                //TODO apply clock drift
                scheduleAt(simTime() + next_sync_request, TDMA_sync_request);*/
            }//end if(corrupt_flag)
            else    //received beacon successfully
            {
                EV << "node: " << this->getIndex() << " resync ok" << endl;
                TDMA_ready_for_sync = false;
                TDMA_beacons_missed = 0;    //reset failed sync attempts
                TDMA_desync = false;
                TDMA_rx_sync_overhead += TDMA_beacon_duration + 2*RX_TX_switching_time + TDMA_guard_time/2;

                //schedule next sync request
                next_sync_request = (floor(TDMA_beacon_reception_rate/deadline)-1) *deadline;               //next beacon should start at beginning of cycle BEFORE TDMA_beacon_reception_rate is reached
                                                                                                            //(otherwise clock drift can already lead to packet loss)
                next_sync_request -= TDMA_beacon_duration - RX_TX_switching_time - TDMA_guard_time/2;       //node starts listening for beacon in the middle of guard time interval before beacon starts (due to possible drifts)
                //TODO apply clock drift
                scheduleAt(simTime()+next_sync_request, TDMA_sync_request);
            }
        }
        else    //ignore beacon
        {
            EV << "node: " << this->getIndex() << "  beacon ignored!" << endl;
        }

        //TODO move calculation to resync event !!!!!
        //TODO schedule packet after resync and all subsequent packets (1 every deadline) after packet has been successfully transmitted (take care if this happens on first, second, etc. cycle in he deadline)
        if(number == 1) //first beacon of cycle -> schedule packet
        {
            //set timestamp for delay calculation
            delay_time_stamp = simTime() - TDMA_guard_time - TDMA_beacon_duration - RX_TX_switching_time;

            //schedule packet
            //TODO implement clock drift
            double TDMA_time_until_slot_begins = RX_TX_switching_time + TDMA_guard_time + TDMA_slot_duration * this->getIndex();
            EV << "node: " << this->getIndex() << "  TDMA_time_until_slot_begins: " << TDMA_time_until_slot_begins*1000 << " ms" << endl;
            scheduleAt(simTime()+TDMA_time_until_slot_begins, endTxEvent);
        }
        else
        {
            //debug remove
            double TDMA_time_until_slot_begins = RX_TX_switching_time + TDMA_guard_time + TDMA_slot_duration * this->getIndex();
            EV << "node: " << this->getIndex() << "  TDMA_time_until_slot_begins: " << TDMA_time_until_slot_begins*1000 << " ms" << endl;
            EV << "node: " << this->getIndex() << "  timestamp: " << simTime() + TDMA_time_until_slot_begins << endl;
        }
    }//end: else if (msg->isName("beacon"))
}

void Host::packet_handler(cMessage *msg)
{
////////////////////////////////////////////////////////////////
////////// start carrier sense, mode 4/5/6/10
////////////////////////////////////////////////////////////////
    if(carrier_sense_is_used && carrier_sense_done == false)
    {
        EV << "node: " << this->getIndex() << " start with carrier sensing" << endl;

        if(carrierSenseMode == 1)
        {
            server->carrier_sense_subscribe(false, this->getIndex());
        }
        else
        {
            channel_was_busy_during_sensing = server->carrier_sense_simple();
        }

        cs_active = true;
        scheduleAt(simTime() + Clock_drift_calculate_period(cs_duration), endCarrierSenseEvent);

        return;
    }


////////////////////////////////////////////////////////////////
////////// start sending
////////////////////////////////////////////////////////////////
    if (state==IDLE)
    {
        EV << "node: "<< this->getIndex() << " transmit start" << endl;

        //reset some variables
        pkRepetition = 1;
        ACK_received = false;
        host_id_carrier_sense[getIndex()] = false;
        //cs_active = false;

        //check if transmission is still within deadline
        if(check_and_transmit() == false)
        {
            return; //deadline has passed -> end sequence
        }

        //schedule next event: message end
        state = TRANSMIT_END;
        this->cancelEvent(endTxEvent);
        scheduleAt(simTime() + packet_duration, endTxEvent); //at this point, cs_duration and RX_TX_set have passed (if mode 4/5/6)
    }//end if(state==IDLE)

////////////////////////////////////////////////////////////////
////////// finished sending -> stop or wait a period
////////////////////////////////////////////////////////////////
    //packet has been transmitted (in state==IDLE), send end of packet (endTxEvent)
    else if (state==TRANSMIT_END)
    {

        //schedule ACK timeout
        if(ACK_is_used && channel_was_busy_during_sensing == false)
            scheduleAt(simTime() + RX_TX_switching_time + processing_delay + ACK_duration + 1e-9, ACKTimeoutEvent); //1e-9 is a negligible short time to ensure safe ACK detection

        ////////////////////////////////////////////////////////////////
        //finished transmitting all packets within a sequence
        ////////////////////////////////////////////////////////////////
        if(packetNumber >= packetNumberMax && ACK_is_used == false)
        {
            EV << "node: " << this->getIndex() << "  transmitted all " << packetNumber << " packets" << endl;
            finish_tx();
        }


        ////////////////////////////////////////////////////////////////
        //more packets to come -> wait for a period to transmit next packet
        ////////////////////////////////////////////////////////////////
        else
        {
            state = WAITED_PERIOD;
            carrier_sense_done = false;
            channel_was_busy_during_sensing = false;
            //cs_active = false;

            if(ACK_is_used == true)
            {
                //do nothing -> scheduling is done at ACK and CS events
            }
            else //no ack modes
            {
                if(mode == 7) //RTNS
                {
                    periodTime = RTNS_get_random_pause();
                    periodTime = Clock_drift_calculate_period(periodTime);
                    scheduleAt(simTime() + periodTime - packet_duration, endTxEvent); //period time begins at the beginning of a packet, therefore we must substract one packet length (is already added elsewhere)
                }
                else
                //TODO all other modes
                //SIES, etc
                    scheduleAt(simTime() + periodTime - packet_duration, endTxEvent);
            }

        }
    }//end if(state==TRANSMIT_END)


////////////////////////////////////////////////////////////////
////////// waited a period -> transmit next packet
////////////////////////////////////////////////////////////////
    //transmit next packet (we waited for one period)
    else if(state == WAITED_PERIOD)
    {
        //EV << "node: "<< this->getIndex() << " state == WAIT_PERIOD" << endl;

        //check for deadline and then transmit. If transmit fails (deadline passed), nodes finishes sequence
        if(check_and_transmit() == false)
        {
            return;
        }

        //cs_active = false;
        host_id_carrier_sense[getIndex()] = false;  //needed?
        state = TRANSMIT_END;

        //schedule next event: message end
        scheduleAt(simTime() + packet_duration, endTxEvent);
    } //end if(state==WAIT_PERIOD)

    else
    {
        error("invalid state");
    }
}

//end sequence and schedule new one
void Host::finish_tx()
{
    this->cancelEvent(endTxEvent);
    this->cancelEvent(ACKTimeoutEvent);
    this->cancelEvent(endCarrierSenseEvent);


    //schedule next message or go to sleep
    if(receiverInitiated == false && mode != 8 && mode != 11) //do nothing if receiverInitiated == true or for TDMA, server does activating
    {
        scheduleAt(getNextInterSequenceTime(), endTxEvent);
    }
    else
        EV << "node: " << this->getIndex() << " going to sleep." << endl;

    //inform server about transmission and its state (only ack-based protocols)
    if(ACK_is_used)
    {
        calc_sequence_stats();
        server->notify_about_transmission(this->getIndex(), transmission_successful, rx_time, tx_time);
        /*if(transmission_successful)
            server->notify_about_transmission(this->getIndex(), transmission_successful);
        else
        {
            //calc_sequence_stats();
            server->notify_about_transmission(this->getIndex(), transmission_successful, rx_time, tx_time);
        }*/
    }

    //reset variables
    state = IDLE;
    reset_variables();


    // update network graphics
    if (isGui)
    {
        getDisplayString().setTagArg("i",1,"");
        getDisplayString().setTagArg("t",0,"");
    }
}


//pause time after sequence
simtime_t Host::getNextInterSequenceTime()
{
    simtime_t  t = 0;
    double random_time = 0; //random offset to archive some shuffling

    //calculate random offset ////////////////////////////////////////
    //if (mode == 4 || mode == 6)
    //    random_time = 0; //uniform(0, packet_duration * packetNumberMax);
    if (mode == 5)
    {
        //generate random starting period
        random_time = CSMA_get_backoff();
        periodTime = Clock_drift_calculate_period(random_time);
        delay_time_stamp = simTime() + random_time;
        return periodTime;
    }
    else if(mode == 7 || mode == 9 || mode == 10) //RTNS based modes
    {
        //since we schedule the first packet of the next sequence here, we have to wait for the remaining time until the old deadline has passed
        //calculate remaining time to deadline
        random_time = (deadline - (simTime().dbl() - delay_time_stamp.dbl()));
        //EV << "node: " << this->getIndex() << " time passed since sequence start: " <<  simTime().dbl() - delay_time_stamp.dbl() << endl;

        //old
        /*if(RTNS_node_type == 1)
            buf = (deadline - (simTime().dbl() - delay_time_stamp.dbl()));
        else
            buf = (RTNS_deadline2 - (simTime().dbl() - delay_time_stamp.dbl()));*/


        EV << "node: " << this->getIndex() << " added last sequence pause time of: " <<  random_time << endl;

        //add some more time for shuffling
        //if(submode != 11 && submode != 12) //do not add this random time for submodes //TODO check if that is needed
            random_time += uniform(0, RTNS_tmin1/10);
    }
    else if(mode == 8) //TDMA
    {
        t = TDMA_cycle_duration_drifted - packet_duration + simTime();
        return t;
    }
    else if(mode == 1 || mode == 2 || mode == 4)
    {
        //wait until the end deadline
        simtime_t remaining_time = simTime() - delay_time_stamp;
        random_time = remaining_time.dbl() + deadline; //waiting time equals to reamining time to deadline and full deadline (inter-sequnce pause)
    }
    else if(mode == 3 && submode != 3) //SIES Journal
    {
        simtime_t remaining_time = simTime() - delay_time_stamp;
        //int random_number = (int)uniform(0,currentActiveNodes+0.9999);
        //double a;

        //temp solution: wait until the end of deadline and then wait a short random time
        random_time = remaining_time.dbl() + uniform(0, 100*packet_duration);

        //zufällig ein vielfaches der periode bestimmen. Falls sequenz dann zu ende ist, kann wieder zufällig gewartet werden.
        //random_time = (int)uniform(0, maxHosts-packetNumberMax-1)*periodTime;
        //random_time = uniform(0, periodTime);
    }

    else //all other modes
        random_time = uniform(0, packet_duration * packetNumberMax);


    //Pause ////////////////////////////////////////
    if(PAUSE == false || mode == 7 || mode == 9 || mode == 5 || mode == 10) //CSMA and RTNS
        t = simTime();
    else if(LONG_PAUSE == true && (mode == 4 || mode == 6))
    {
        //wait for remaining periods and inter-sequence pause
        remainingPackets = packetNumberMax - packetNumber;
        double remaining_time = remainingPackets * periodTime;
        //t = simTime() + remaining_time + intersequence_pause;
        t = simTime() + remaining_time + intersequence_pause;
        random_time = uniform(0, remaining_time);
    }
    else //LONG_PAUSE == false -> no added pause to gain high network load
    {
        t = simTime() + intersequence_pause;
    }

    //RTNS based modes
    if(mode == 7 || mode == 9 || mode == 10)
    {
        periodTime = RTNS_get_random_pause();
        random_time += periodTime;
    }
    //clock drift : re-shuffle period times after each sequence
    if(ClockDriftEnabled == true)
    {
        //calculate new drift
        Clock_drift_current_value = uniform(-ClockDriftPlotDriftRange_temp, +ClockDriftPlotDriftRange_temp);

        //calculate new periods with drift
        random_time = Clock_drift_calculate_period(random_time);

        EV << "node: " << getIndex() << " period_time_original: " << periodTime_original << "  period_time: " << periodTime << endl;
    }

    //set time stamp accordingly
    delay_time_stamp = t + random_time;

    return t + random_time;
}


void Host::sendPacket(char *name)
{
    bool SKIP;  //indicates whether this message was skipped
    //bool ACK;   //true: request ACK from Server, false: no ACK (used for transmit ACK every k packets)
    char buf[64];

    packetNumber++;

    // update network graphics
    if (isGui)
    {
        for(int i = 0; i<40; i++)
            pkname[i] = 0;
        sprintf(pkname,"pk-id%d #%d r%d", getIndex(), pkCounter++, pkRepetition);

        getDisplayString().setTagArg("i",1,"yellow");
        if((mode == 4 || mode == 5 || mode == 6) && channel_was_busy_during_sensing)
            sprintf(buf, "SKIP %i / %i", packetNumber+1, packetNumberMax);
        else
            sprintf(buf, "TRANSMIT %i / %i", packetNumber+1, packetNumberMax);
        getDisplayString().setTagArg("t",0,buf);
    }

    //if TDMA is desynched, do not send a packet
    if(mode == 11 && TDMA_desync)
    {
        EV << "node: " << this->getIndex() << " did not send packet because desync! " << endl;
        return;
    }


    EV << "node: "<< this->getIndex() << " sendPacket() called with: " << name << " #(" << packetNumber << "/" << packetNumberMax << ")";
    if(ACK_is_used)
        EV << " ACK: true";
    if(carrier_sense_is_used)
        EV << ", SKIP: "<< SKIP;
    EV << endl;

    //calculate delay and other statistics
    calc_sequence_stats();

    //send packet
    cPacket *pk = new cPacket(name);
    pk->addPar("id");
    pk->par("id").setLongValue(this->getIndex());
    pk->addPar("repetition");
    pk->par("repetition").setLongValue(pkRepetition);
    pk->addPar("delay");
    pk->par("delay").setDoubleValue(message_delay);
    pk->addPar("rx_time");
    pk->par("rx_time").setDoubleValue(rx_time);
    pk->addPar("tx_time");
    pk->par("tx_time").setDoubleValue(tx_time);
    pk->addPar("sleep_time");
    pk->par("sleep_time").setDoubleValue(sleep_time);
    pk->addPar("deadline");
    pk->par("deadline").setDoubleValue(deadline);
    pk->addPar("packets_per_sequence");
    pk->par("packets_per_sequence").setDoubleValue(packetNumber-packetNumberSkipped);
    pk->addPar("packets_skipped");
    pk->par("packets_skipped").setDoubleValue(packetNumberSkipped);
    pk->setBitLength(pkLenBits->longValue());
    pk->addPar("type");
    pk->par("type").setLongValue(RTNS_node_type);

    sendDirect(pk, 0, packet_duration, server->gate("in"));
}


bool Host::check_and_transmit()
{
    char packet_name[64];

    //check if packet is within deadline
    if(deadline_is_used)
    {
        double time_to_deadline = delay_time_stamp.dbl() - simTime().dbl() + deadline;
        EV << "node: " << this->getIndex() << " time_to_deadline: " << time_to_deadline*1000 << "ms" << endl;
        if(time_to_deadline <= 0)
        {
            //do not send packet but finish
            EV << "node: " << this->getIndex() << " deadline missed! " << endl;
            finish_tx();
            return false;
        }
    }

    //create name for packet
    switch(state)
    {
    case IDLE: sprintf(packet_name, "state==IDLE"); break;
    case TRANSMIT_END: sprintf(packet_name, "state==TRANSMIT_END"); break;
    case WAITED_PERIOD: sprintf(packet_name, "state==WAITED_PERIOD"); break;
    default:sprintf(packet_name, "state==unknown");
    }

    //transmit packet
    sendPacket(packet_name);

    return true;
}




void Host::calculatePeriods(int maxPaketNumber, int numberOfTransmittingNodes)
{
    double l_max = packet_duration;

    //calculate periods
    if(this->getIndex() < currentActiveNodes)
    {
        if(mode == 1)   //SIES (DEEP analytic)
        {
            minPeriod = ((numberOfTransmittingNodes-1) * (numberOfTransmittingNodes-2)*2*l_max) + 2*l_max;      //smallest period
            periodTime_original = minPeriod + (this->getIndex() * 2 * l_max);                           //period = smallest period + index*2*packet_duration
            longestPeriodTime = minPeriod + (numberOfTransmittingNodes-1)*2*l_max;
            intersequence_pause = (numberOfTransmittingNodes-1)*longestPeriodTime + l_max;
            if(fixedDeadline == false)
                deadline = intersequence_pause;
            EV << "node " << getIndex() << "   periodTime_original: " << periodTime_original*1000 << " ms" << endl;
            EV << "node " << getIndex() << "   intersequence_pause: " << intersequence_pause*1000 << " ms" << endl;
        }
        else if(mode == 2)   //Schweden
        {
            periodTime_original = period_list_schweden[currentActiveNodes-2][this->getIndex()] * l_max;
            intersequence_pause = (period_list_schweden[currentActiveNodes-2][currentActiveNodes-1] * (currentActiveNodes-1) +1) * l_max;
            if(fixedDeadline == false)
                deadline = deadline_list_schweden[currentActiveNodes-2] / 1000; //convert to seconds
            EV << "periodTime: " << periodTime_original*1000 << " ms  intersequence_pause: " << intersequence_pause*1000 << " ms" << endl;
            EV << "periodTime: " << periodTime_original/l_max << "[lmax]   intersequence_pause: " << intersequence_pause/l_max << "[lmax]" << endl;

            if(submode == 3)
            {
                minPeriod = ((numberOfTransmittingNodes-1) * (numberOfTransmittingNodes-2)*2*l_max) + 2*l_max;      //smallest period
                longestPeriodTime = minPeriod + (numberOfTransmittingNodes-1)*2*l_max;
                intersequence_pause = longestPeriodTime + l_max;
            }

        }
        else if(mode == 3)            //Journal (DEEP heuristic): SIES with different deadlines and packet lengths
        {

            //temp
            if(submode == 3) //use the same inter-sequence pause as for mode == 1
            {
                //calculate inter-sequence pause (which is (n-1) times the longest period)
                minPeriod = ((numberOfTransmittingNodes-1) * (numberOfTransmittingNodes-2)*2*l_max) + 2*l_max;      //smallest period
                longestPeriodTime = minPeriod + (numberOfTransmittingNodes-1)*2*l_max;
                intersequence_pause = (numberOfTransmittingNodes-1)*longestPeriodTime + l_max;
            }
            else
                intersequence_pause = 0;

            periodTime_original = period_list_journal[currentActiveNodes-2][this->getIndex()] /22 * l_max;
            if(fixedDeadline == false)
                deadline = deadline_list_journal[currentActiveNodes-2] / 1000; //convert to seconds
            EV << "deadline_list_journal["<< currentActiveNodes-2 << "]: " << deadline << endl;
            //intersequence_pause = (period_list_journal[currentActiveNodes-2][0] * (currentActiveNodes-1) +2) / 3 * l_max;
            //intersequence_pause = periodTime_original; //modified transmission scheme: inter-sequence pause is just one period
            EV << "periodTime: " << periodTime_original*1000 << " ms   intersequence_pause: " << intersequence_pause*1000 << " ms" << endl;
            EV << "periodTime: " << periodTime_original/l_max << "[lmax]   intersequence_pause: " << intersequence_pause/l_max << "[lmax]" << endl;
        }
        else if (mode == 4) //(bi-DEEP) neuer Alg für DSD paper: bidirectional, ACK, carrier-sense, 100% reliable
        {
            double pmin1, pmin2, pmin, pmax;
            double delta = RX_TX_switching_time + cs_sensitivity; //tset + t_bar
            pmin1 = ((2*currentActiveNodes - 2)*(currentActiveNodes - 1)*2 + 2) * delta;
            pmin2 = cs_duration + packet_duration + 2*RX_TX_switching_time + ACK_duration + RX_TX_switching_time + (RX_TX_switching_time + 2*cs_sensitivity); // == L + tset + tsen
            pmin = max(pmin1, pmin2);
            //pmin = pmin2; // debug
            periodTime_original = pmin + this->getIndex() * 2 * delta; //in s
            pmax = pmin + (currentActiveNodes-1) * 2 * delta;
            intersequence_pause = (2*currentActiveNodes - 2) * pmax + pmin2;
            if(fixedDeadline == false)
                deadline = intersequence_pause;


            EV << "periodTime: " << periodTime_original << "   intersequence_pause: " << intersequence_pause << endl;
            EV << "periodTime: " << periodTime_original/l_max*3 << "[lmax]   intersequence_pause: " << intersequence_pause/l_max*3 << "[lmax]" << endl;
        }
        else if(mode == 5) //CSMA
        {
            //values are calculated before being used
            periodTime_original = -1;
            intersequence_pause = -1;
        }
        else if(mode == 6)            //mode 4 with optimized periods ---> obsolete
        {
            periodTime_original = new_alg_periods[currentActiveNodes-2][this->getIndex()] / 1000000;
            intersequence_pause = (new_alg_periods[currentActiveNodes-2][0] / 1000000 * (2* currentActiveNodes  - 2)) + packet_duration + 2*RX_TX_switching_time + ACK_duration;
            EV << "periodTime: " << periodTime_original << "   intersequence_pause: " << intersequence_pause << endl;
            EV << "periodTime: " << periodTime_original/l_max*3 << "[lmax]   intersequence_pause: " << intersequence_pause/l_max*3 << "[lmax]" << endl;
        }
        else if(mode == 7 || mode == 9) //RTNS: random periods, k packets
        {
            //int n_1, n_2; //the number of nodes of type 1 and 2; n_1 + n_2 = numberOfTransmittingNodes
            double buf;
            double m = RTNS_m;

            if(mode == 9)
                l_max = packet_duration + RX_TX_switching_time + ACK_duration + RX_TX_switching_time;

            RTNS_node_type = RTNS_get_node_type();

            if(submode == 11 && RTNS_node_type == 2)
            {
                double packet_length_temp = floor(packet_length_start + (packet_length_stop-packet_length_start)/(submode_steps-1)*submode_step_current);
                l_max = packet_length_temp / txRate;
                packet_duration = l_max;
                EV << "node " << getIndex() << " new packet length: " << packet_length_temp << endl;
            }
            if(submode == 12 && RTNS_node_type == 2)
            {
                deadline = deadline_start + (deadline_stop-deadline_start)/(submode_steps-1)*submode_step_current;

                double buffer = floor(deadline / RTNS_deadline1);
                if(buffer < 1) buffer = 1;
                m = buffer;

                //TODO check
                ////////////////////////////////////////////////begin algorithm
                //calculate m with algorithm 1 of RTNS Journal
                double tmax1,tmin1,tmax2,tmin2,m12, m21,m2,p2_neu,p2_alt,delta_coll;
                RTNS_calculate_node_numbers();
                tmax1 = (RTNS_deadline1 - packet_duration) / (double)maxPaketNumber;
                tmin1 = tmax1 / 2;
                tmax2 = (deadline - packet_duration) / (double)maxPaketNumber;

                p2_alt = 0;
                m2=1;
                while(1)
                {
                    //increase m2 and see if system is still feasible
                    m2 = m2 + 1; //line 11 (here m2 is m2)
                    tmin2 = tmax2 - m2 * tmin1; //line 12
                    m12 = ceil((tmax2-tmin2) / tmin1); //line 13

                    //calculate reliability with Equ. 23
                    delta_coll = 2*packet_duration* (RTNS_n1*m2 + (RTNS_n2-1)*1); //Equ. 22
                    p2_neu = 1 - pow((delta_coll/(tmax2-tmin2)), (double)maxPaketNumber);

                    //check if system is still feasible
                    if (tmin2 < (tmax2/2))
                        break;
                    else if (tmin2 < tmin1)
                        break;
                    else if (p2_alt > p2_neu)
                        break;

                   //checks were successful -> next iteration
                   p2_alt = p2_neu;
                }
                //restore last valid values for m2 and tmin2
                m2 = m2 - 1;
                tmin2 = tmax2 - m2 * tmin1;
                m21 = ceil((tmax2-tmin2) / tmin1);
                m12 = ceil((tmax1-tmin1) / tmin2);

                //calculate reliability with Equ. 23
                delta_coll = 2*packet_duration* (RTNS_n1*m2 + (RTNS_n2-1)*1);
                p2_neu = 1 - pow((delta_coll/(tmax2-tmin2)), (double)maxPaketNumber);


                //sprintf("d2 = %f;  m2 = %f;  tmin2 = %f;  tmax2 = %f;  m12 = %f;  m21 = %f;  p2_neu = %f\n", deadline*1000, m2, tmin2*1000, tmax2*1000, m12, m21, p2_neu*100);

                //debug output
                //if(getIndex() == 15)
                //    std::cout << "d2: " << deadline*1000 << "   m2: " << m2 << "   tmin2: " << tmin2 << "   tmax2: " << tmax2 << "   m12: " << m12 << "   m21: " << m21 << "   p2_neu: " << p2_neu << std::endl;
                ////////////////////////////////////////////////end algorithm

                //set variables
                RTNS_tmax1 = tmax2;
                RTNS_tmin1 = tmin2;
                RTNS_reliability = p2_neu;

                EV << "node " << getIndex() << "   new deadline: " << deadline << "   new m: " << m << endl;
                //std::cout << "node " << getIndex() << "   new deadline: " << deadline << "   new m: " << m << std::endl;


                //debug output
                //if(getIndex() == 15)
                //{
                //    std::cout << "node " << getIndex() << "   new deadline: " << deadline << "   new m: " << m << std::endl;
                //}
            }
            else
            {
                RTNS_tmax1 = (deadline - l_max)/(double)maxPaketNumber;
                buf = pow(1-RTNS_reliability, (1/(double)maxPaketNumber)); //k root of 1-p
                RTNS_tmin1 = RTNS_tmax1 - (2*m* ((double)numberOfTransmittingNodes-1) * l_max) / buf;

                //maximize reliability
                if(RTNS_reliability_max == true)
                {
                    RTNS_tmax1 = (deadline - l_max)/(double)maxPaketNumber;
                    RTNS_tmin1 = RTNS_tmax1/2;
                    buf = (2*RTNS_m*((double)numberOfTransmittingNodes-1)*l_max) / (RTNS_tmax1-RTNS_tmin1);
                    RTNS_reliability = 1 - pow(buf, maxPaketNumber);
                    //EV << "RTNS_reliability: " << RTNS_reliability << endl;
                }

                if(RTNS_tmin1 < (RTNS_tmax1/2))
                {
                    EV << "Error: reliability not possible anymore" << std::endl;
                    RTNS_tmin1 = -1; RTNS_tmax1=-1;
                }

                //Verify found intervals
                if( (RTNS_tmin1 >= (RTNS_tmax1 - l_max)) || (RTNS_tmin1 < (RTNS_tmax1/(m+1))) )
                {
                    EV << "node " << getIndex() << " period invalid!!!" << endl;
                }

                intersequence_pause = 0;
            }

            EV<< "node: "<< this->getIndex() << " i am type: " << RTNS_node_type << " with d: " << deadline << "  and l: " << packet_duration << endl;
            EV << "node " << getIndex() << " : tmax: " << RTNS_tmax1*1000 << "   tmin: "<< RTNS_tmin1*1000 << endl;
        }//end else if(mode == 7) RTNS mode



        else if(mode == 10) //RTCSA17
        {
            //only 1 node type supported
            RTNS_node_type = 1;
            double buf;
            double L = packet_duration + RX_TX_switching_time + ACK_duration;
            double tcca = cs_duration + RX_TX_switching_time; //t_sen + t_set
            double d_tot = tcca + L + cs_sensitivity; //delta total

            RTNS_tmax1 = (deadline - L - tcca)/(double)maxPaketNumber;
            buf = pow(1-RTNS_reliability, (1/(double)maxPaketNumber)); //k root of 1-p
            RTNS_tmin1 = RTNS_tmax1 - (((double)numberOfTransmittingNodes-1) * d_tot) / buf;

            //RTNS_tmax2 = RTNS_tmax1;
            //RTNS_tmin2 = RTNS_tmin1;


            //maximize reliability
            if(RTNS_reliability_max == true)
            {
                RTNS_tmin1 = RTNS_tmax1/2;
                buf = (2*RTNS_m*((double)numberOfTransmittingNodes-1)*packet_duration) / (RTNS_tmax1-RTNS_tmin1);
                RTNS_reliability = 1 - pow(buf, maxPaketNumber);
                //EV << "RTNS_reliability: " << RTNS_reliability << endl;
            }

            //check if valid
            if(RTNS_tmin1 < (RTNS_tmax1/2))
            {
                EV << "Error: reliability not possible anymore" << std::endl;
                RTNS_tmin1 = -1; RTNS_tmax1=-1;
                //RTNS_tmin2 = -1; RTNS_tmax2=-1;
            }
        }



        else if(mode == 8) //TDMA
        {
            /*if(TDMA_slot_duration == -1)
            {
                TDMA_slot_duration = packet_duration;// TDMA_beacon_size / txRate + RX_TX_switching_time;
            }*/
            TDMA_slot_duration = packet_duration;
            TDMA_slot_duration *= (1+TDMA_slot_tolerance);
            TDMA_beacon_duration *= (1+TDMA_slot_tolerance);
            TDMA_cycle_duration = TDMA_slot_duration * (currentActiveNodes) +  2*RX_TX_switching_time + TDMA_beacon_duration;
            periodTime_original = TDMA_cycle_duration; //total cycle duration

        }
        else if(mode == 11) //TDMA with ACK
        {
            TDMA_slot_duration = packet_duration + 2*RX_TX_switching_time + processing_delay + ACK_duration + TDMA_guard_time;
        }
    }



    //clock drift
    Clock_drift_generate_new_random_drift();
    //multiply drift with periods
    periodTime = Clock_drift_calculate_period(periodTime_original);
}

double Host::max(double a, double b)
{
    if(a>b)
        return a;
    else
        return b;
}

double Host::min(double a, double b)
{
    if(a<b)
        return a;
    else
        return b;
}

void Host::ack_was_lost(bool only_ack_was_lost)
{
    Enter_Method("ack_was_lost()");

    EV << "ack_was_lost() called: host id: " << this->getIndex() << endl;

    cs_active = false;

    if(only_ack_was_lost)
    {
        //only ACK was lost, message has been received
    }
    else
    {
        //message was lost, i.e. no ACK was sent
    }
    CSMA_packets_failed++;
    EV << "node: " << this->getIndex() << "  CSMA_packets_failed++ (" << CSMA_packets_failed << "/" << CSMA_max_retransmissions << ")" << endl;
}

void Host::reset_variables()
{
    //reset variables
    pkCounter = 0;
    packetNumber = 0;
    packetNumberSkipped = 0;
    pkRepetition = 0;
    carrier_sense_done = false;
    channel_was_busy_during_sensing = false;
    CSMA_backoff_counter = 0;
    CSMA_packets_failed = 0;
    TDMA_beacons_received_this_sequence = 0;
    TDMA_rx_sync_overhead = 0;
    message_delay = 0;
    cs_active = false;
    host_id_carrier_sense[getIndex()] = false;
    old_channel_state = false;
    channel_is_busy = false;
    transmission_successful = false;
    rx_time = 0;
    tx_time = 0;
    sleep_time = 0;
    //ClockDriftPlotCurrentStepNumber = 0;
}

void Host::next_plot_step(int iteration_number)
{
    Enter_Method("next_plot_step()");

    ClockDriftPlotCurrentStepNumber++;
}


//used for CSMA (mode 5)
void Host::update_channel_state(bool channel_busy)
{
    Enter_Method("update_channel_state()");

    if(channel_busy == old_channel_state)  //cancel, if called multiple times with same channel state
    {
        EV << "node: " << this->getIndex() << "  called with same channel state -> skip update_channel_state    channel_busy= " << channel_busy << endl;
        return;
    }
    else
    {
        EV << "node: "<< this->getIndex() << " called update_channel_state() with channel_busy= " << channel_busy << endl;
    }
    channel_is_busy = channel_busy;

    if(cs_active == true && carrier_sense_done == false && (mode == 5 && CSMA_mode == 2))
    {
        if(channel_busy == true) //channel busy during carrier sensing, pause backoff time
        {
            remaining_backoff_time = CSMA_backoff_finished_time - simTime();
            EV << "node: " << this->getIndex() << " pause carrier sensing -> cancelEvent(endCarrierSenseEvent)" << endl;
            cancelEvent(endCarrierSenseEvent);
        }
        else                        //channel is free again, resume backoff time
        {
            CSMA_backoff_finished_time = simTime() + remaining_backoff_time;
            EV << "node: " << this->getIndex() << " resume carrier sensing. Estimated duration: " << remaining_backoff_time
                            << " finish time: " << CSMA_backoff_finished_time << endl;
            scheduleAt(CSMA_backoff_finished_time, endCarrierSenseEvent);
        }
    }

    old_channel_state = channel_busy;
}

void Host::carrier_sense_callback()
{
    EV << "node: " << this->getIndex() << " carrier_sense_callback() called" << endl;
    channel_was_busy_during_sensing = true;
}

//calculates new double value with drift. Drift value must have already been calculated when calling this function
double Host::Clock_drift_calculate_period(double period)
{
    double periodTime_buf;
    if(ClockDriftEnabled == false)
        return period;

    if(Clock_drift_current_value < 0)
        periodTime_buf = period * (1-Clock_drift_current_value);
    else
        periodTime_buf = period * (1 + Clock_drift_current_value);

    return periodTime_buf;
}

double Host::RTNS_get_random_pause()
{
    double random_time_buf = uniform(RTNS_tmin1, RTNS_tmax1);


    /*if(ClockDriftEnabled)
    {
        //clock drift has been calculated yet
        if(Clock_drift_current_value < 0)
            random_time_buf = random_time_buf * (1-Clock_drift_current_value);
        else
            random_time_buf = random_time_buf * (1 + Clock_drift_current_value);
    }*/



    /*//debug remove
    //limit to only 4 possible period choices
    int rand_num = uniform(1, 4.9999);
    random_time_buf = 0.01 * rand_num;*/

    return random_time_buf;
}

int Host::RTNS_get_node_type()
{
    //calculate node type
    double percent_current = (100/(double)currentActiveNodes)*((double)this->getIndex()+1);

    if((percent_current <= RTNS_node_type_ratio || ((int)RTNS_node_type_ratio == 100)) || RTNS_use_different_node_types == false)
        RTNS_node_type = 1;
    else
        RTNS_node_type = 2;

    return RTNS_node_type;
}

int Host::RTNS_get_node_type(int i)
{
    //calculate node type
    double percent_current = (100/(double)currentActiveNodes)*((double)i+1);

    if((percent_current <= RTNS_node_type_ratio || ((int)RTNS_node_type_ratio == 100)) || RTNS_use_different_node_types == false)
        RTNS_node_type = 1;
    else
        RTNS_node_type = 2;

    return RTNS_node_type;
}

void Host::update_plot_step_number(bool external_interference, int stepnumber)
{
    Enter_Method("update_plot_step_number()");

    external_interference_current_step_number = stepnumber;
    external_interference_current_duty_cycle = stepnumber * ExternalInterferenceDutyCycle / ExternalInterferencePlotStepNumber;
}

//mode == 8 only
//is called at the end of a sync beacon -> first slot starts now after RX TX switching time plus remaining tolerance
//resets and synchronizes time
void Host::TDMA_synchronize()
{
    Enter_Method("TDMA_synchronize()");

    double TDMA_time_until_slot_begins;


    //calculate new drift
    Clock_drift_generate_new_random_drift();
    TDMA_slot_duration_drifted = Clock_drift_calculate_period(TDMA_slot_duration);
    TDMA_cycle_duration_drifted = Clock_drift_calculate_period(TDMA_cycle_duration);

    //debug
    //TDMA_slot_duration_drifted = TDMA_slot_duration;
    //TDMA_cycle_duration_drifted = TDMA_cycle_duration_drifted;


    simtime_t time_until_finished_sending = 0;

    if(state != IDLE) //check if there is currently a packet being sent (later check if it ends after own slot time starts -> if so, then skip the slot)
    {
        EV << "node " << this->getIndex() << "  currently node IDLE when sync happened" << endl;
        time_until_finished_sending = endTxEvent->getArrivalTime() - simTime(); //get arrival time of the packet that is currently sent (0 if no packet is sent at the moment)
        state = IDLE; //if we were sending
    }

    //cancel all packets
    cancelEvent(endTxEvent);

    //calculate new times
    TDMA_time_until_slot_begins = TDMA_slot_duration_drifted * this->getIndex() + TDMA_beacon_duration * (TDMA_slot_tolerance) + RX_TX_switching_time; //note: slot 1 starts at the end of sync, i.e., now

    //debug
    EV << "node " << this->getIndex() << "  time_until_finished_sending.dbl() = " << time_until_finished_sending.dbl() << endl;
    EV << "node " << this->getIndex() << "  TDMA_slot_duration_drifted = " << TDMA_slot_duration_drifted << endl;
    EV << "node " << this->getIndex() << "  TDMA_cycle_duration_drifted = " << TDMA_cycle_duration_drifted << endl;
    EV << endl;

    //check if next slot time is feasible
    if(TDMA_time_until_slot_begins >= time_until_finished_sending.dbl())
    {
        scheduleAt(simTime() + TDMA_time_until_slot_begins, endTxEvent);
    }
    else
    {
        EV << "node " << this->getIndex() << "  skipped cycle" << endl;
        //do nothing (slot has been skipped -> wait for next cycle)
    }
}

void Host::Clock_drift_generate_new_random_drift()
{
    //clock drift
    if(ClockDriftEnabled == true)
    {
        if(ClockDriftPlotMode == true)
            ClockDriftPlotDriftRange_temp = (ClockDriftRangePercent / ClockDriftPlotStepNumber) * ClockDriftPlotCurrentStepNumber;
        else
            ClockDriftPlotDriftRange_temp = ClockDriftRangePercent;

        //calculate current drift
        Clock_drift_current_value = uniform(-ClockDriftPlotDriftRange_temp, +ClockDriftPlotDriftRange_temp);
    }
    else
    {
        periodTime = periodTime_original;
        Clock_drift_current_value = 0;
    }
}


double Host::CSMA_get_backoff()
{
    double periodTime_;
    int pow_result = pow(2, (CSMA_packets_failed + CSMA_backoff_counter));
    double CSMA_contention_window = pow_result * CSMA_wc_min;
    if (CSMA_contention_window > CSMA_wc_max)   //limit contention window to CSMA_wc_max
        CSMA_contention_window = CSMA_wc_max;
    int rand = (int)uniform((double)CSMA_wc_min, CSMA_contention_window-1e-9); //integer conversion to round down, rand number between [0,WC-1]
    if (rand > CSMA_contention_window)  rand = CSMA_contention_window;
    periodTime_ = rand * CSMA_backoff_time;

    return periodTime_;
}

void Host::calc_sequence_stats()
{
    //calculate delays
    simtime_t delay_time_buf = simTime() - delay_time_stamp; //total time since wakeUp
    delay_time_buf += packet_duration;
    message_delay = delay_time_buf.dbl();
    if(ACK_is_used)
        message_delay += RX_TX_switching_time + processing_delay + ACK_duration;

    //debug remove
    //EV << "node " << this->getIndex() << "  delay: " << message_delay*1000 << endl;

    //calculate receive and transmit times
    switch(mode)
    {
    //transmit-only modes
    case 1: //SIES
    case 2: //Schweden
    case 3: //SIES Journal
    case 7: //RTNS
        tx_time = packetNumberMax * packet_duration;
        rx_time = 0;
        sleep_time = deadline - tx_time;
        break;
    //modes with cs and ACK
    case 4:  //reliable CSMA (DSD16)
    case 5:  //CSMA 802.15.4
    case 6:  //mode 4 with different back-off times
    case 10: //RTCSA17
        tx_time = (packetNumber-packetNumberSkipped) * packet_duration;
        rx_time = packetNumber*(cs_duration) + (packetNumber-packetNumberSkipped)*(2*RX_TX_switching_time + processing_delay + ACK_duration); //assumption: if a packet is skipped, only cs_duration is consumed
        break;
    //modes without cs, but with ACK
    case 9://RTNS with ACK
        tx_time = packetNumber * packet_duration;
        rx_time = packetNumber*(RX_TX_switching_time + processing_delay + ACK_duration + RX_TX_switching_time);
        break;
    case 8:
        tx_time = 0; //2beacons+safety interval + data+ack(falls genutzt)
        rx_time = 0; //TODO
        break;
    case 11:  //TDMA with ACK
        tx_time = packetNumber * packet_duration;
        rx_time = packetNumber * (RX_TX_switching_time + processing_delay + ACK_duration + RX_TX_switching_time);  //calc for ACK only (no overhead)
        //rx_time += TDMA_beacons_received_this_sequence * (TDMA_guard_time/2  + TDMA_beacon_duration + 2*RX_TX_switching_time); //sync overhead
        rx_time += TDMA_rx_sync_overhead; //sync overhead (advanced version that account that node has increase listening interval for beacons)
        if(rx_time > deadline)
            rx_time = deadline;
        EV << "node " << this->getIndex() << "  added " << TDMA_beacons_received_this_sequence << " beacon(s) to rx_time" << endl;
        break;
    default:
        rx_time = -1;
        tx_time = -1;
        break;
    }
    if(tx_time < 0)
    {
        EV << "tx/rx_time error" << endl;
    }

    //calculate sleep times
    if(deadline_is_used || mode == 11)
        sleep_time = deadline - tx_time - rx_time;
    else if(mode == 2 || mode == 3) ;//nothing
    else
        sleep_time = message_delay - tx_time - rx_time;
}

void Host::TDMA_calculate_guard_time()
{
    if(mode != 11)
        return;
    double time_per_cycle = deadline / TDMA_cycles_per_deadline;
    time_per_cycle = time_per_cycle - (TDMA_beacon_duration + 2*RX_TX_switching_time) - maxHosts*(packet_duration + 2*RX_TX_switching_time + ACK_duration);  //substract one beacon and all packet slots (without guard time intervals)
    TDMA_guard_time = time_per_cycle / (maxHosts+1);
    //EV << "guard time:" << TDMA_guard_time*1e6 << " us" << endl;
}

void Host::RTNS_calculate_node_numbers()
{
    RTNS_n1 = 0;
    RTNS_n2 = 0;

    for(int i=0; i< currentActiveNodes; i++)
    {
        if(RTNS_get_node_type(i) == 1)
            RTNS_n1++;
        else
            RTNS_n2++;
    }
}

}; //namespace
