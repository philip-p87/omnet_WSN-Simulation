#include "Server.h"
#include "Host.h"
#include "periods.h"

#include <iostream>
#include <fstream>


namespace aloha {

Define_Module(Server);

Server::Server()
{
    endRxEvent = NULL;
    hopCountStats = new cLongHistogram();
}

Server::~Server()
{
    cancelAndDelete(endRxEvent);
    cancelAndDelete(restartEvent);
    cancelAndDelete(ACK_finishedEvent);
    cancelAndDelete(ACK_startEvent);
    cancelAndDelete(CSMA_waited_DIFS);
    cancelAndDelete(ExternalInterferenceEvent);
    cancelAndDelete(TDMA_sync_finished);
    cancelAndDelete(TDMA_sync_start);
    cancelAndDelete(simtime_event);
    cancelAndDelete(modeSwitchFinishedEvent);
    free(packetList);

    delete delay;
    delete tx_time;
    delete rx_time;
    delete packets_per_sequence;
    delete packets_skipped;
    delete sleep_time;
}

void Server::initialize()
{
    gate("in")->setDeliverOnReceptionStart(true);

    /*channelStateSignal = registerSignal("channelState");
    receiveBeginSignal = registerSignal("receiveBegin");
    receiveSignal = registerSignal("receive");
    collisionSignal = registerSignal("collision");
    collisionLengthSignal = registerSignal("collisionLength");*/

    vectorOutput = par("vectorOutput").str();
    EV << "vector output: " << vectorOutput.c_str() << endl;
    txRate = par("txRate");
    ACKLenBits = &par("ACKLenBits");
    mode = par("mode");
    submode = par("submode");
    pkLenBits = &par("pkLenBits");
    WC_MODE = par("WC_MODE");
    PAUSE = par("PAUSE");
    LONG_PAUSE = par("LONG_PAUSE");
    minHosts = par("minHosts");
    maxHosts = par("maxHosts");
    HostStepSize = par("HostStepSize");
    minPackets = par("minPackets");
    maxPackets = par("maxPackets");
    haltOnPacketNumberSent = par("haltOnPacketNumberSent");
    haltOnDeadlines = par("haltOnDeadlines");
    RX_TX_switching_time = par("RX_TX_switching_time");
    RX_TX_switching_time /= 1000000;
    processing_delay = par("processing_delay");
    processing_delay /= 1000000;    //convert to 탎
    cs_sensitivity = par("cs_sensitivity");
    cs_sensitivity /= 1000000;    //convert to 탎
    listen_time = par("listen_time");
    receiverInitiated = par("receiverInitiated");
    carrierSenseMode = par("carrierSenseMode");
    deadline = par("deadline");
    deadline /= 1000;

    CSMA_backoff_time = par("CSMA_backoff_time");
    CSMA_backoff_time /= 1000000;    //convert to 탎
    CSMA_max_retransmissions = par("CSMA_max_retransmissions");
    CSMA_max_backoffs = par("CSMA_max_backoffs");
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
    TDMA_guard_time /= 1e6; //conversion to microseconds
    TDMA_cycles_per_deadline = par("TDMA_cycles_per_deadline");
    TDMA_restart_beacon_after_new_iteration = false;
    TDMA_default_clock_drift = par("TDMA_default_clock_drift");
    TDMA_default_clock_drift /= 1e6;

    RTNS_reliability = par("RTNS_reliability");
    RTNS_reliability_max = par("RTNS_reliability_max");
    RTNS_m = par("RTNS_m");
    RTNS_deadline2 = par("RTNS_deadline2");
    RTNS_packet_length2 = &par("RTNS_packet_length2");
    RTNS_use_different_node_types = par("RTNS_use_different_node_types");
    RTNS_node_type_ratio = par("RTNS_node_type_ratio");

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
    //EV << "RX_TX_switching_time_start:" << RX_TX_switching_time_start << "    RX_TX_switching_time_stop:" << RX_TX_switching_time_stop << endl;

    packet_duration = pkLenBits->doubleValue() / txRate;
    ACK_duration = ACKLenBits->longValue() / txRate;
    sendingACK = false;
    channelBusy = false;
    ackScheduled = false;

    //external interference and clock drift
    ClockDriftEnabled = par("ClockDriftEnabled");
    ClockDriftRangePercent = par("ClockDriftRangePercent");
    ClockDriftRangePercent /= 100;                                                  // conversion to percent
    ClockDriftPlotMode = par("ClockDriftPlotMode");
    ClockDriftPlotStepNumber = par("ClockDriftPlotStepNumber");
    ClockDriftPlotStepNumber++;
    ExternalInterferenceEnable = par("ExternalInterferenceEnable");
    ExternalInterferenceDutyCycle = par("ExternalInterferenceDutyCycle");
    ExternalInterferenceDutyCycle /= 100;
    ExternalInterferencePlotMode = par("ExternalInterferencePlotMode");
    ExternalInterferencePlotStepNumber = par("ExternalInterferencePlotStepNumber");
    ExternalInterferenceActiveMin = par("ExternalInterferenceActiveMin");
    ExternalInterferenceActiveMax = par("ExternalInterferenceActiveMax");
    if(ClockDriftEnabled == false) ClockDriftPlotMode = false;
    if(ExternalInterferenceEnable == false) ExternalInterferencePlotMode = false;
    if(ClockDriftPlotMode == true) ExternalInterferencePlotMode = false; //both modes are exclusive
    if(ClockDriftEnabled || ExternalInterferenceEnable) submode = 0;
    deadline_missed_counter = 0;
    ClockDriftPlotCurrentStepNumber = 0;
    ExternalInterferencePlotCurrentStepNumber = 0;
    WC_MODE = false;

    //ExternalInterference_active_min = packet_duration/10;
    //ExternalInterference_active_max = 5 * packet_duration;
    if(ExternalInterferenceActiveMin == -1)
    {
        //custom values
        ExternalInterference_active_min = packet_duration/10;
        ExternalInterference_active_max = 5 * packet_duration;
    }
    else
    {
        ExternalInterference_active_min = ExternalInterferenceActiveMin*8/txRate;//12byte = minimum packet length with 0byte payload
        ExternalInterference_active_max = ExternalInterferenceActiveMax*8/txRate;//64 byte payload + 12 byte overhead
    }

    //process input values
    if(minHosts == -1)
        minHosts = maxHosts;
    if(minHosts > maxHosts)
            maxHosts = minHosts;
    if(minPackets > maxHosts)
            minPackets = maxHosts;
    if(minPackets > maxPackets)
            maxPackets = minPackets;
    currentActiveNodes = minHosts;
    if(minPackets == -1)
        currentPacketCountPerSequence = minHosts; //for mode 1 2 3 send n packets
    else
        currentPacketCountPerSequence = minPackets;

    //events
    endRxEvent = new cMessage("end-reception");
    restartEvent = new cMessage("restartEvent");
    ACK_finishedEvent = new cMessage("ACK_finished");
    ACK_startEvent = new cMessage("ACK_start");
    CSMA_waited_DIFS = new cMessage("CSMA_waited_DIFS");
    ExternalInterferenceEvent = new cMessage("ExternalInterferenceEvent");
    TDMA_sync_finished = new cMessage("TDMA_sync_finished");
    TDMA_sync_start = new cMessage("TDMA_sync_start");
    simtime_event = new cMessage("simtime_event");
    modeSwitchFinishedEvent = new cMessage("modeSwitchFinishedEvent");

    //storage containers
    tx_time = new cDoubleHistogram();
    rx_time = new cDoubleHistogram();
    delay = new cDoubleHistogram();
    packets_per_sequence = new cDoubleHistogram();
    packets_skipped = new cDoubleHistogram();
    sleep_time = new cDoubleHistogram();

    //initialize storage
    for(int i = 0; i< MAX_ITERATIONS; i++)
    {
        statsAll[i].numberOfNodes = 0;
        statsAll[i].numberOfPackets = 0;
        statsAll[i].packetsGood = 0;
        statsAll[i].packetsTotal = 0;
        statsAll[i].packetsLost = 0;
        statsAll[i].sequencesGood = 0;    //good sequences
        statsAll[i].sequencesLost = 0;        //failed sequences
        statsAll[i].sequencesTotal = 0;      //number of sequences received in total
        statsAll[i].deadline_missed_counter = 0;
        statsAll[i].ACKsLost = 0;
        statsAll[i].ACKsTransmitted = 0;               //total number of ACKs that were transmitted
        statsAll[i].packetsSkipped = 0;
        statsAll[i].RTNS_reliability = 0;
        statsAll[i].min = 1000;
        statsAll[i].max = 0;
        statsAll[i].mean = 0;
        statsAll[i].stddev = 0;

        statsAll[i].delay_avg = 0;
        statsAll[i].delay_min = 0;
        statsAll[i].delay_max = 0;
        statsAll[i].rx_time_avg = 0;                    //time that nodes were in receiver mode
        statsAll[i]. rx_time_min = 0;
        statsAll[i].rx_time_max = 0;
        statsAll[i].tx_time_avg = 0;
        statsAll[i].tx_time_min = 0;
        statsAll[i].tx_time_max = 0;
        statsAll[i].sleep_time_avg = 0;                 //time between transmissions in which node can power down and sleep (does not take transition times into account, i.e., wakeup and power down times)

        statsAll[i].packets_per_sequence_avg = 0;        //number of transmissions per sequence on average
        statsAll[i].packets_skipped_avg = 0;
    }
    storage_index = 0;
    storage_iteration_number = 0;

    //overwrite values for individual modes////////////////////////////////////////
    if(mode == 4 || mode == 5 || mode == 6) //CSMA variants
    {
        receiverInitiated = true; //not yet implemented

        currentPacketCountPerSequence = currentActiveNodes;
        //currentPacketCountPerSequence = 2* currentActiveNodes - 1; //wrong!!!!!! -> server will only receive n packets

        if(mode == 5)
        {
            LONG_PAUSE = false;
            currentPacketCountPerSequence = CSMA_max_retransmissions;
        }

    }
    else if(mode == 10) //RTCSA
    {
        if(submode != 1)
        {
            currentPacketCountPerSequence = maxPackets;
            minPackets = maxPackets;
        }
    }
    else if (mode == 1 || mode == 2 || mode == 3 || mode == 7 || mode == 9) //SIES, schweden , sies journal, rtns
    {

        if(mode == 7 || mode == 9) //RTNS
        {
            if(submode != 1)
                currentPacketCountPerSequence = maxPackets;
            minPackets = maxPackets;
        }
        else if(submode == 1 && ExternalInterferenceEnable == false && ClockDriftEnabled==false)
        {
            currentPacketCountPerSequence = 1;
            maxPackets = currentActiveNodes;
        }
        else if(submode == 1 && ClockDriftEnabled==true)
        {
            currentPacketCountPerSequence = maxPackets;
        }
    }
    else if(mode == 8)   //TDMA no-ACK
    {
        maxPackets = 1;
        currentPacketCountPerSequence = 1;
        scheduleAt(simTime() + 0.1, TDMA_sync_start);
    }
    else if(mode == 11) //TDMA with ACK
    {
        currentPacketCountPerSequence = TDMA_cycles_per_deadline;
        receiverInitiated = false;   //for now, will also support other non receiver-initiated mode later
        scheduleAt(simTime() + TDMA_guard_time + RX_TX_switching_time, TDMA_sync_start);
        if(haltOnDeadlines > 0)
            haltOnPacketNumberSent = haltOnDeadlines * currentActiveNodes;
        if(TDMA_guard_time < 0)
            TDMA_calculate_guard_time();
    }


    //submode configuration
    if(submode == 4)//increase RX_TX_switching_time
    {
        RX_TX_switching_time = RX_TX_switching_time_start;
        if(RX_TX_switching_time == 0)
            RX_TX_switching_time = 1e-9;    //add very short time to prevent that order of events gets messed up
    }
    else if(submode == 5)//increase deadline
    {
        deadline = deadline_start;
    }
    else if(submode == 6)//increase packet length
    {
        packet_duration = packet_length_start / txRate;
    }


    //all modes that use ACKs
    if(mode == 4 || mode == 5 || mode == 6 || mode == 9 || mode == 10 || mode == 11)
        ACK_is_used = true;
    else
        ACK_is_used = false;

    //all modes that use carrier sensing
    if(mode == 4 || mode == 5 || mode == 6 || mode == 10)
    {
        carrier_sense_is_used = true;
        cs_duration = 2 * cs_sensitivity +  RX_TX_switching_time + processing_delay;
    }
    else
        carrier_sense_is_used = false;

    //all modes that use deadlines
    if(mode == 7 || mode == 9  || mode == 10)       //RTNS based modes
    {
        deadline_is_used = true;
    }
    else
        deadline_is_used = false;



    //receiver initiated setup for high traffic bursts
    if(receiverInitiated == true && mode != 8) //no TDMA
    {
        //calculate restart Interval
        calculate_restart_interval();
        restartEvent_counter = 0;
        scheduleAt(simTime()+0.01, restartEvent);
    }


    this->initVariables();


    //External Interference
    if(ExternalInterferenceEnable) // && (ExternalInterferenceDutyCycle > 0))// || ExternalInterferencePlotMode))
    {
        if(mode == 3 && submode == 1)
            currentPacketCountPerSequence = maxPackets;

        //ExternalInterferenceEvent_is_active = true;
        //ExternalInterference_active_time = uniform(ExternalInterference_active_min, ExternalInterference_active_max);
        //scheduleAt(simTime() + ExternalInterference_active_time, ExternalInterferenceEvent);
        if(ExternalInterferencePlotMode == false)
        {
            ExternalInterferenceCurrentDutyCycle = ExternalInterferenceDutyCycle;
            //random start
            scheduleAt(simTime() + uniform(packet_duration, 100*packet_duration), ExternalInterferenceEvent);
        }
        else
        {
            ExternalInterferenceCurrentDutyCycle = 0;
        }

        ExternalInterference_active_time = packet_duration; //should not matter, since duty cycle is off after start
    }

    //GUI//////////////////////////////////////////////////////////
    isGui = ev.isGUI();
    //debug
    isGui = false;

    if (isGui)
        getDisplayString().setTagArg("i2",0,"x_off");

    //output simulation setting to console to prevent running with wrong parameters
    if(ExternalInterferenceEnable)
    {
        if(ExternalInterferencePlotMode == false)
            std::cout << "Interference enabled d=" << ExternalInterferenceDutyCycle*100 << "%" << std::endl;
        else
            std::cout << "Interference Plot mode enable (0-" << ExternalInterferenceDutyCycle*100 << "% in " << ExternalInterferencePlotStepNumber << " steps): n=" << currentActiveNodes
            << " k=" << currentPacketCountPerSequence << std::endl;
    }
    std::cout << "submode = " << submode << std::endl;
    //TODO other outputs
    std::cout << "-------------------------------------------------------" << std::endl;

    //test debug
    //std::cout << "Server n:" << currentActiveNodes << "  k:" << currentPacketCountPerSequence << "  deadline:" << deadline << "  RX_TX_switching_time:" << RX_TX_switching_time << std::endl;
}


void Server::handleMessage(cMessage *msg)
{
    if(msg == ExternalInterferenceEvent)
    {
        if(ExternalInterferenceCurrentDutyCycle == 100)
        {
            channelBusy = true;
            ExternalInterferenceEvent_is_active = true;
            return;
        }
        //////////////////////////////////////
        //stop interference
        //////////////////////////////////////
        if(ExternalInterferenceEvent_is_active == true)
        {
            ExternalInterferenceEvent_is_active = false;
            //EV << "ExternalInterferenceCurrentDutyCycle: " << ExternalInterferenceCurrentDutyCycle << endl;
            ExternalInterference_idle_time = ExternalInterference_active_time * (1 / ExternalInterferenceCurrentDutyCycle - 1);
            EV << "Interference OFF (for " << ExternalInterference_idle_time*1000 << " ms)" << endl;

            if(!sendingACK && !receivingPaket)
                channelBusy = false;

            //cancelEvent(ExternalInterferenceEvent);
            scheduleAt(simTime() + ExternalInterference_idle_time, ExternalInterferenceEvent);
        }
        //////////////////////////////////////
        //start interference
        //////////////////////////////////////
        else
        {
            channelBusy = true;
            ExternalInterferenceEvent_is_active = true;
            ExternalInterference_active_time = uniform(ExternalInterference_active_min, ExternalInterference_active_max);
            //cancelEvent(ExternalInterferenceEvent);
            scheduleAt(simTime() + ExternalInterference_active_time, ExternalInterferenceEvent);
            EV << "Interference ON" << endl;
        }
    }

    //////////////////////////////////////
    //NOT used anymore
    if(msg == CSMA_waited_DIFS)
    {
        EV << "Server: msg == CSMA_waited_DIFS" << endl;
        //inform_hosts_about_channel_state(false, false);
        Host *host;
        for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
        {
            if (iter()->isName("host")) // if iter() is in the same vector as this module
            {
                host = check_and_cast<Host *>(iter());
                if(host->getIndex() < currentActiveNodes)
                {
                    host->update_channel_state(false);
                }
            }
        }
    }

    ////////////////////////////////////////////////
    //server finished switching mode from TX (sending ACK) to RX after RX_TX_switching_time
    ////////////////////////////////////////////////
    if(msg == modeSwitchFinishedEvent)
    {
        is_switching_mode = false;
    }



    ////////////////////////////////////////////////
    ////TDMA synchronizing - start of beacon
    ////////////////////////////////////////////////
    if(msg == TDMA_sync_start) //start to send sync message (just listen for interference at this time)
    {
        TDMA_performing_sync = true;

        if(channelBusy == true) //channel is blocked -> sync message will be corrupted (TODO count the packet that currently blocks the channel as lost)
        {
            //EV << "Server TDMA_sync_start: beacon corrupted due to busy channel";
            TDMA_sync_successful = false;
        }

        //whenever a packet is received and TDMA_performing_sync == true -> set TDMA_sync_successful = false; (done in later code,i.e., receive-function)

        //schedule beacon end after TDMA_beacon_duration (with tolerance)
        //scheduleAt(simTime() + TDMA_beacon_duration * (1+TDMA_slot_tolerance) + RX_TX_switching_time, TDMA_sync_finished);
        scheduleAt(simTime() + TDMA_beacon_duration, TDMA_sync_finished);

    }

    ////////////////////////////////////////////////
    ////TDMA synchronizing - end of beacon
    ////////////////////////////////////////////////
    if(msg == TDMA_sync_finished) //sending sync is complete -> inform hosts
    {
        double TDMA_time_to_next_sync;

        if(channelBusy == true)
            TDMA_sync_successful = false;

        if(mode == 8)//TODO: veraltet, sollte geupdatet werden
        {
            //inform hosts, if sync was successful
            if(TDMA_sync_successful == true)
            {
                EV << "server: beacon successful";
                //send sync beacon
                Host *host;
                for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
                {
                    if (iter()->isName("host")) // if iter() is in the same vector as this module
                    {
                        host = check_and_cast<Host *>(iter());
                        if(host->getIndex() < currentActiveNodes)
                        {
                            host->TDMA_synchronize();
                        }
                    }
                }
            }

            //convert beacon rate from seconds to multiples of cycle durations
            TDMA_slot_duration = packet_duration;// + RX_TX_switching_time;
            TDMA_slot_duration *= (1+TDMA_slot_tolerance);
            double TDMA_beacon_slot_duration = (TDMA_beacon_size / txRate) * (1+TDMA_slot_tolerance) + 2*RX_TX_switching_time;
            double TDMA_remaining_beacon_slot_time = TDMA_beacon_slot_duration - TDMA_beacon_size / txRate;
            double TDMA_cycle_duration_full = TDMA_slot_duration * (double) currentActiveNodes + TDMA_beacon_slot_duration;
            double TDMA_cycle_duration_without_beacon = TDMA_slot_duration * (double) currentActiveNodes;
            //double TDMA_cycle_number = floor((TDMA_beacon_rate + TDMA_beacon_slot_duration) / (TDMA_slot_duration * (double) currentActiveNodes));
            double TDMA_cycle_number = (TDMA_beacon_rate - TDMA_cycle_duration_without_beacon - TDMA_slot_tolerance*(TDMA_beacon_size / txRate)  + 2*RX_TX_switching_time)
                                        / TDMA_cycle_duration_full +1;


            if(TDMA_cycle_number < 1) //beacon rate is less than one cycle --> only during debug
            {
                //send beacon at beginning of slot -> gets interrupted -> useless
                //instead send at end of cycle
                EV << "beacon rate too small -> set beacon rate to 1 every 1 cycle" << endl;
                TDMA_time_to_next_sync = TDMA_cycle_duration_without_beacon + TDMA_remaining_beacon_slot_time;
            }
            else if(floor(TDMA_cycle_number) == 1) //one sync every cycle
            {
                TDMA_time_to_next_sync = TDMA_cycle_duration_without_beacon + TDMA_remaining_beacon_slot_time;
            }
            else
            {
                TDMA_time_to_next_sync = (floor(TDMA_cycle_number) - 1) * TDMA_cycle_duration_full + TDMA_cycle_duration_without_beacon + TDMA_remaining_beacon_slot_time;
            }

            EV <<"server: " << "TDMA_time_to_next_sync " << TDMA_time_to_next_sync << endl;
            EV <<"server: " << "TDMA_cycle_number " << TDMA_cycle_number << endl;

            //schedule next beacon
            //scheduleAt(simTime() + 1.9847925, TDMA_sync_start);
            scheduleAt(simTime() + TDMA_time_to_next_sync, TDMA_sync_start);
        }//endif (mode == 8)


        else if(mode == 11)
        {
            EV << "Server: send beacon #" << TDMA_current_beacon_per_deadline;
            if(TDMA_sync_successful == false)
                EV << " (corrupt)"<< endl;

            sendBeacon(TDMA_current_beacon_per_deadline);
            TDMA_current_beacon_per_deadline++; //count beacon number within cycle
            if(TDMA_current_beacon_per_deadline > TDMA_cycles_per_deadline)
            {
                TDMA_current_beacon_per_deadline = 1;
                //EV << "TDMA cycle finished -> process data" << endl;
                //deadline has finished (after sending "TDMA_cycles_per_deadline" cycles) -> evaluate results
            }

            //schedule next beacon
            TDMA_time_to_next_sync = deadline / TDMA_cycles_per_deadline - TDMA_beacon_duration;   //TODO move to beginning of server.cc; TODO calculate properly
            scheduleAt(simTime() + TDMA_time_to_next_sync, TDMA_sync_start);
        }

        else
            EV << "Error!!" << endl;

        //reset flags
        TDMA_performing_sync = false;
        TDMA_sync_successful = true;
    }



    ///////////////////////////////////////////////////////////////////////////////
    //restart (wakeup) event
    ///////////////////////////////////////////////////////////////////////////////
    if(msg == restartEvent)
    {
        if(next_iteration_flag)
        {
            EV << "Server: restart" << endl;
            this->start_next_iteration();
            restartEvent_counter = 0;
            return;
        }


        Host *host;
        //change transmission scheme
        for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
        {
            if (iter()->isName("host")) // if iter() is in the same vector as this module
            {
                host = check_and_cast<Host *>(iter());
                if(host->getIndex() < currentActiveNodes)
                {
                    host->wakeup();
                    //host->stop_transmission();
                    //host->activate();
                }
            }
        }

        restartEvent_counter++;
        scheduleAt(simTime()+restartInterval, restartEvent);  //TODO dynamic restart interval
        return;
    }


    ///////////////////////////////////////////////////////////////////////////////
    //Start sending ACK
    ///////////////////////////////////////////////////////////////////////////////
    if(msg == ACK_startEvent)
    {
        //is_switching_mode = false;
        ackScheduled = false;
        sendingACK = true;
        ACK_was_lost = false;
        ACK_host_id = senderId;
        carrier_sense_update();

        EV << "Server: " << " start sending ACK to node: " << ACK_host_id << endl;

        //schedule end of ACK
        total_acks_transmitted++;
        scheduleAt(simTime() + ACK_duration, ACK_finishedEvent);
    }




    ///////////////////////////////////////////////////////////////////////////////
    //finished sending ACK
    ///////////////////////////////////////////////////////////////////////////////
    if(msg == ACK_finishedEvent)
    {
        char name[10] = {"ACK"};
        if(ACK_was_lost == true || channelBusy == true)
            ACK_was_lost = true;
        sendACKPacket(name, ACK_host_id);

        //sequencesGood++;

        char buf[32];
        if (isGui)
        {
            sprintf(buf, "Message complete. ID %i\n", ACK_host_id);
            bubble(buf);
        }
        EV << "Server: " << " finished sending ACK to node: " << ACK_host_id << endl;


        //EV << "Server: successfully sent ACK to node: "<< ACK_host_id << endl;
                //EV << "channelBusy: " << channelBusy << endl;

        sendingACK = false;

        //server now switches back to receive mode in which he is not able to receive any packets
        is_switching_mode = true;
        scheduleAt(simTime()+RX_TX_switching_time+processing_delay, modeSwitchFinishedEvent);

        //reset counters - remove??
        packetList[ACK_host_id].packetsReceivedTotal = 0;
        packetList[ACK_host_id].packetsReceivedGood = 0;
        packetList[ACK_host_id].firstPacketIndex = 1000;
        packetList[ACK_host_id].packetsSkipped = 0;

        return;
    }



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//finished receiving, evaluate data
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (msg==endRxEvent)
    {
        receivingPaket = false;
        if(ExternalInterferenceEvent_is_active == false)
            channelBusy = false;
        packetsLost += currentCollisionNumFrames;

        //TODO check this solution
        if(ExternalInterferenceEvent_is_active == true)
        {
            if(lastPacketGood == true)
                packetsLost++;      //count this packet as lost
            lastPacketGood = false;
        }

        //some debug output
        if(currentCollisionNumFrames == 0)  currentCollisionNumFrames = 1; //only for better understanding (==0 if only a single packet is received)
        EV << "Server: finished receiving packet(s), number: " << currentCollisionNumFrames << ", state: ";
        if(lastPacketGood) EV << "good (ID=" << lastID << ")" << endl;
        else EV << "corrupt" << endl;
        currentCollisionNumFrames = 0;

        //packet was received, when ACK was transmitted -> ACK is lost
        if(sendingACK == true)
        {
            ACK_was_lost = true;
        }

        ////////////////////////////////
        //successful reception
        ////////////////////////////////
        else if(lastPacketGood == true)
        {
            //we received a packet -> increment the counter
            packetList[lastID].packetsReceivedGood++;
            packetsGood++;

            //note delay, rx_time, tx_time, etc. is saved in ack function for all ack-based schemes

            //set the firstPacketIndex
            if(packetList[lastID].firstPacketIndex > packetList[lastID].packetsReceivedTotal)
            {
                packetList[lastID].firstPacketIndex = packetList[lastID].packetsReceivedTotal;
                hopCountStats->collect(packetList[lastID].firstPacketIndex);

                //take delay of first packet (do not regard the remaining packets of a sequence that will be received later)
                if(ACK_is_used == false)
                {
                    delay->collect(temp_delay);
                    rx_time->collect(temp_rx_time);
                    tx_time->collect(temp_tx_time);
                    packets_per_sequence->collect(temp_packets_per_sequence);
                    packets_skipped->collect(temp_packets_skipped);
                    sleep_time->collect(temp_sleep_time);
                }
            }
        }


        ////////////////////////////////
        //send ACK if requested
        ////////////////////////////////
        if(ACK_is_used == true)
        {
            if(lastPacketGood == true)
            {
                ackScheduled = true;
                cancelEvent(ACK_startEvent);
                scheduleAt(simTime() + RX_TX_switching_time + processing_delay, ACK_startEvent);
                //is_switching_mode = true;
                EV << "Server: schedule ACK_startEvent" << endl;
            }
            else    //(multiple) packets were lost -> send no ACK, but notify host
            {
                EV << "Server: multiple packets collided -> do not send ACK" << endl;
            }
        }

//        ////////////////////////////////
//        //(multiple) packets were lost -> send no ACK, but notify host
//        ////////////////////////////////
//        else if(ACK_is_used == true)
//        {
//            EV << "Server: multiple packets collided -> do not send ACK" << endl;
//        }


        //old counting mechanisms for non-ack-based transmission schemes
        //received one or multiple (collided) packets ///////////////////////////////////////////////////////////////////
        //Process those packets only that are the last one of a sequence
        if(nodesFinishedTransmissionNumber > 0 && ACK_is_used == false)
        {
            for(int i=0; i < nodesFinishedTransmissionNumber; i++)
            {
                //check if all packets of a sequence were lost
                if(packetList[nodesFinishedTransmission[i]].packetsReceivedGood == 0)
                {
                    //all n packets of a sequence have been lost
                    sequencesLost++;
                    EV << "Server: Sequence loss! ID: " << nodesFinishedTransmission[i] << endl;      //debug
                }
                else
                {
                    EV << "Server: debug: sequencesGood++" << endl;
                    sequencesGood++;
                }

                //reset counters
                packetList[nodesFinishedTransmission[i]].packetsReceivedTotal = 0;
                packetList[nodesFinishedTransmission[i]].packetsReceivedGood = 0;
                packetList[nodesFinishedTransmission[i]].firstPacketIndex = 1000;
                packetList[nodesFinishedTransmission[i]].packetsSkipped = 0;

                sequencesTotal++;
                EV << "Server: debug: sequencesTotal++" << endl;
                EV << "Server: debug: sequencesLost: " << sequencesLost << "  sequencesGood: " << sequencesGood << "  sequencesTotal: " << sequencesTotal << endl;
                EV << "Server: debug: packetsLost: " << packetsLost << "  packetsGood: " << packetsGood << "  packetsTotal: " << packetsTotal << endl;

                char buf[32];
                sprintf(buf, "Message complete (pos2). ID%i Received %i/%i\n", nodesFinishedTransmission[i],
                                            packetList[nodesFinishedTransmission[i]].packetsReceivedGood, packetList[nodesFinishedTransmission[i]].packetsReceivedTotal);
                EV << buf;
                if (isGui)
                {
                    bubble(buf);
                }

            }//end for

            nodesFinishedTransmissionNumber = 0;
        }//end if(nodesFinishedTransmissionNumber > 0)

        lastPacketGood = true;

        //check whether it is time to switch to next iteration
        if(sequencesTotal >= haltOnPacketNumberSent && ACK_is_used == false) //for ack-based modes this is checked in notify_about_transmission()
        {
            if(receiverInitiated)
                next_iteration_flag = true;
            else
            {
                this->start_next_iteration();
            }
            return;
        }

        // update network graphics
        if (isGui)
        {
            getDisplayString().setTagArg("i2",0,"x_off");
            getDisplayString().setTagArg("t",0,"");
        }
    } //if (msg==endRxEvent)



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//start receiving //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if(msg->isSelfMessage() == false)
    {
        //read data from packet
        repetition = msg->par("repetition");
        senderId = msg->par("id");
        temp_delay = msg->par("delay");
        temp_rx_time = msg->par("rx_time");
        temp_tx_time = msg->par("tx_time");
        temp_sleep_time = msg->par("sleep_time");
        temp_packets_per_sequence = msg->par("packets_per_sequence");
        temp_packets_skipped = msg->par("packets_skipped");
        temp_deadline = msg->par("deadline"); //deadline of that packet; for mode == 7 RTNS
        cPacket *pkt = check_and_cast<cPacket *>(msg);
        ASSERT(pkt->isReceptionStart());
        simtime_t endReceptionTime = simTime() + pkt->getDuration();
        carrier_sense_update(); //let subscribed nodes (i.e., those which perform carrier sensing at the moment) know about blocked channel

        EV << "Server: start receiving packet from node: " << senderId << endl;

        //RTNS deadline (note: other modes with deadline are processed in host.cc)
        deadline_missed = false;
        if(mode == 7 && ClockDriftEnabled == true && temp_delay > temp_deadline)
        {
            deadline_missed = true;
            EV << "SERVER: a node missed a deadline" << endl;
            deadline_missed_counter++;
            EV << "deadline_missed: message delay: " << temp_delay << endl;
        }

        if((mode == 8 || mode == 11) && TDMA_performing_sync == true)
            TDMA_sync_successful = false;   //packet reception is started while beacon is transmitted

        EV << "Server: start receiving, channelBusy:" << channelBusy << "  sendingACK:" << sendingACK << " deadline_missed:" << deadline_missed << " ackScheduled:" << ackScheduled << " is_switching_mode:" << is_switching_mode << endl;

        //normal operation: received just 1 packet, no collision
        if (channelBusy == false && sendingACK == false && deadline_missed == false && ackScheduled == false && is_switching_mode == false)
        {

            //recall handleMessage with message end endRxEvent
            //EV << "receive: normal operation so far" << endl;
            scheduleAt(endReceptionTime, endRxEvent);

            if (isGui)
            {
                getDisplayString().setTagArg("i2",0,"x_yellow");
                getDisplayString().setTagArg("t",0,"RECEIVE");
                getDisplayString().setTagArg("t",2,"#808000");

                //show packet number
                if(packetList[senderId].packetsReceivedTotal < currentPacketCountPerSequence)
                {
                    char buf[32];
                    sprintf(buf, "ID %i  Repetition %i", senderId, repetition);
                    bubble(buf);
                }
            }
        }
        //received multiple packets (collision)
        else
        {
            if (currentCollisionNumFrames == 0 && sendingACK == false && ExternalInterferenceEvent_is_active == false && deadline_missed == false)
                currentCollisionNumFrames = 2;
            else if (sendingACK == true) //new packet arrives during ACK transmission -> both packet and ACK are lost
            {
                ACKsLost++;
                currentCollisionNumFrames++;
                ACK_was_lost = true;
            }
            else // if deadline_missed == true
            {
                currentCollisionNumFrames++;
            }

            lastPacketGood = false;

            //if last packet would finish after endRxEvent, shift event to this time instead
            if (endReceptionTime > endRxEvent->getArrivalTime())
            {
                //EV << "collision: reschedule" << endl;
                cancelEvent(endRxEvent);
                scheduleAt(endReceptionTime, endRxEvent);
            }

            // update network graphics
            char buf[32];
            sprintf(buf, "Collision! (%ld frames)", currentCollisionNumFrames);
            EV << buf << endl;
            if (isGui && deadline_missed == false)
            {
                getDisplayString().setTagArg("i2",0,"x_red");
                getDisplayString().setTagArg("t",0,"COLLISION");
                getDisplayString().setTagArg("t",2,"#800000");
                bubble(buf);
            }
            else if(isGui)
            {
                getDisplayString().setTagArg("i2",0,"x_red");
                getDisplayString().setTagArg("t",0,"Over deadline");
                getDisplayString().setTagArg("t",2,"#800000");
                bubble(buf);
            }


        }//end else: received multiple packets (collision)

        //old packet counting mechanisms for non-ack-based transmission schemes
        //if all packets have been received of a node, increase counter and save ID
        if(ACK_is_used == false)
        {
            packetList[senderId].packetsReceivedTotal++;
            if(packetList[senderId].packetsReceivedTotal >= currentPacketCountPerSequence)
            {
                nodesFinishedTransmission[nodesFinishedTransmissionNumber] = senderId;
                nodesFinishedTransmissionNumber++;
            }
        }

        //setting and resetting some variables
        packetsTotal++;
        receivingPaket = true;
        channelBusy = true;
        ackScheduled = false;
        cancelEvent(ACK_startEvent);
        delete pkt;
    }//end else: start receiving

    lastID = senderId;
}

void Server::finish()
{
    double collisionPackets[MAX_ITERATIONS+2];    //packets within a seaquence
    double collisionSequences[MAX_ITERATIONS+2];    //sequences

    std::ofstream myfile;
    std::string file_output = vectorOutput.substr(1,vectorOutput.size()-2); //remove quotation marks
    myfile.open (file_output.c_str(), std::ios::trunc);

    if(myfile.is_open() == false)
    {
        EV << "cannot open output log file!" << endl;
        std::cout << "cannot open output log file!: "<< vectorOutput << endl;
    }


    myfile << "mode: " << mode;

    if (PAUSE == false)
        myfile << " - no pause";
    else
    {
        myfile << " - pause";
        if (LONG_PAUSE == true)
            myfile << " long";
        else
            myfile << " short";
    }
    if(ClockDriftPlotMode == true)
    {
        myfile << " ClockDriftPlotMode:1   ClockDriftPlotDriftRange: " << ClockDriftRangePercent
                << "   ClockDriftPlotStepNumber: " << ClockDriftPlotStepNumber  << "\n";
        myfile << "number of nodes: " << statsAll[0].numberOfNodes << "   number of packets: " << statsAll[0].numberOfPackets;
    }
    else if(ExternalInterferencePlotMode)
    {
        myfile << "ExternalInterferencePlotMode:1   Duty Cycle: " << ExternalInterferenceDutyCycle << "   Step number: " <<
                ExternalInterferencePlotStepNumber << "\n";
        myfile << "number of nodes: " << statsAll[0].numberOfNodes << "   number of packets: " << statsAll[0].numberOfPackets;
    }
    else
    {
        myfile << " \n";
    }


    //////////////calculate collision/success rates////////////////////////////////////////////////////////////////////////////////////////
    int upper_bound;
    if(ClockDriftPlotMode == true)
        upper_bound = ClockDriftPlotStepNumber;
    else if(ExternalInterferencePlotMode == true)
        upper_bound = ExternalInterferencePlotStepNumber;
    else
        upper_bound = MAX_NUMBER_OF_NODES;
    for(int i=0; i<=upper_bound; i++)
    {
        if(statsAll[i].numberOfNodes <= 0)
            break;

        if(statsAll[i].packetsTotal != 0)
            collisionPackets[i] = (double)statsAll[i].packetsLost / (double)statsAll[i].packetsTotal  * 100; //in percent
        else
            collisionPackets[i] = -1;
        if(statsAll[i].sequencesTotal != 0)
            collisionSequences[i] = (double)statsAll[i].sequencesLost / (double)statsAll[i].sequencesTotal * 100 ;  //in percent
        else
            collisionSequences[i] = -1;
    }



    myfile << "haltOnPacketNumberSent: " << haltOnPacketNumberSent << "   packet length[탎]:" << packet_duration*1e6 << "  number of packets:" << currentPacketCountPerSequence;
    if(deadline_is_used)
        myfile << "   deadline[ms]: " << deadline*1e3 << "   use different node types: " << RTNS_use_different_node_types;
    myfile << endl;

/////////////////////////////////////// ClockDriftPlotMode ////////////////////////////////////////////////////////////////////////////////////
    if(ClockDriftPlotMode == true)
    {

        myfile << "Clock Drift; sequence loss [%]; packet loss [%]; avg delay [ms]; receive time [ms]; transmit time [ms]; sleep time[ms]; deadline missed counter; deadline missed [%]\n";

        //if(ClockDriftPlotCurrentStepNumber > 100) ClockDriftPlotCurrentStepNumber = 100;
        for (int i = 0; i<= ClockDriftPlotStepNumber; i++) //ClockDriftPlotStepNumber
        {
            double current_drift = i * (100 * ClockDriftRangePercent / ClockDriftPlotStepNumber);
            double deadline_missed_percent = (double)statsAll[i].deadline_missed_counter / (double)statsAll[i].packetsGood * 100;


            myfile << current_drift << ";" << collisionSequences[i] << ";" << collisionPackets[i] << ";"
                    << statsAll[i].delay_avg*1000 << ";" << statsAll[i].rx_time_avg*1000 << ";" << statsAll[i].tx_time_avg*1000 << ";" << statsAll[i].sleep_time_avg*1000
                    << ";" << (double)statsAll[i].deadline_missed_counter << ";" << deadline_missed_percent << ";" << endl;


            EV << "mode: " << mode << "  WC_MODE: " << WC_MODE << "  PAUSE: " << PAUSE <<endl;
            EV << "ClockDriftMode enabled: ClockDriftPlotDriftRange: " << ClockDriftRangePercent*100 << "  ClockDriftPlotStepNumber: "
                    << ClockDriftPlotStepNumber << "  current step:" << i << "   current drift: " << current_drift << endl;

            EV << "number of nodes: " << statsAll[i].numberOfNodes << endl;
            if(mode != 0) EV << "number of max packets: " << statsAll[i].numberOfPackets << endl;
            if(mode != 0) EV << "Packets total: " << statsAll[i].packetsTotal << "   lost: " << statsAll[i].packetsLost
                    << "  loss in percent: " << collisionPackets[i] << "\n";
            EV << "Sequences total: " << statsAll[i].sequencesTotal << "   lost: " << statsAll[i].sequencesLost
                    << "  loss in percent: " << collisionSequences[i] << "\n";
            EV << "avg delay [ms]: " << statsAll[i].delay_avg*1000 << endl;
            EV << "avg delay [ms]: " << statsAll[i].delay_avg*1000 << endl;
            EV << "avg receive time per sequence[ms]: " << statsAll[i].rx_time_avg*1000 << endl;
            EV << "avg transmit time per sequence [ms]: " << statsAll[i].tx_time_avg*1000 << endl;
            EV << "----------------------------------------------------------" << endl << endl;
        }
    }
/////////////////////////////////////// ExternalInterferencePlotMode ////////////////////////////////////////////////////////////////////////////////////
    else if(ExternalInterferencePlotMode == true)
    {

        myfile << "duty cycle; sequence loss [%]; packet loss [%]; avg delay [ms]; receive time [ms]; transmit time [ms]; sleep time[ms]; sent packets per sequence; skipped packets per sequence;\n";

        //if(ClockDriftPlotCurrentStepNumber > 100) ClockDriftPlotCurrentStepNumber = 100;
        for (int i = 0; i<= ExternalInterferencePlotStepNumber; i++) //ClockDriftPlotStepNumber
        {
            double current_duty_cycle = i * (100 * ExternalInterferenceDutyCycle / ExternalInterferencePlotStepNumber);

            myfile << current_duty_cycle << ";" << collisionSequences[i] << ";" << collisionPackets[i] << ";" << statsAll[i].delay_avg*1000 << ";"
                   << statsAll[i].rx_time_avg*1000 << ";" << statsAll[i].tx_time_avg*1000 << ";" << statsAll[i].sleep_time_avg*1000
                   << ";" << statsAll[i].packets_per_sequence_avg << ";" << statsAll[i].packets_skipped_avg << ";" << endl;


            EV << "mode: " << mode << "  WC_MODE: " << WC_MODE << "  PAUSE: " << PAUSE <<endl;
            EV << "ExternalInterferencePlotMode enabled:   duty cycle: " << ClockDriftRangePercent*100 << "%    Step Number: "
                    << ExternalInterferencePlotStepNumber << "    current step:" << i << "     current duty cycle: " << current_duty_cycle << "% \n";

            EV << "number of nodes: " << statsAll[i].numberOfNodes << endl;
            if(mode != 0) EV << "number of max packets: " << statsAll[i].numberOfPackets << endl;
            if(mode != 0) EV << "Packets total: " << statsAll[i].packetsTotal << "   lost: " << statsAll[i].packetsLost
                    << "  loss in percent: " << collisionPackets[i] << "\n";
            EV << "Sequences total: " << statsAll[i].sequencesTotal << "   lost: " << statsAll[i].sequencesLost
                    << "  loss in percent: " << collisionSequences[i] << "\n";
            EV << "avg delay [ms]: " << statsAll[i].delay_avg*1000 << endl;
            EV << "avg receive time per sequence[ms]: " << statsAll[i].rx_time_avg*1000 << endl;
            EV << "avg transmit time per sequence [ms]: " << statsAll[i].tx_time_avg*1000 << endl;
            EV << "avg delay [ms]: " << statsAll[i].delay_avg*1000 << endl;
            EV << "----------------------------------------------------------" << endl << endl;
        }
    }
/////////////////////////////////////// submodes ////////////////////////////////////////////////////////////////////////////////////
    else if(submode == 1)//increase packet numbers
    {
        EV << "submode:" << submode << "   #nodes:" << currentActiveNodes << "  deadline:" << deadline << endl;
        EV << "----------------------------------------------------------" << endl << endl;
        myfile << "submode:" << submode << "   nodes:" << currentActiveNodes << "  deadline:" << deadline << endl;
        myfile << "packet number; sequence loss [%]; avg delay [ms];";
        if(mode == 7 || mode == 9 || mode == 10)
            myfile << "RTNS_reliability";
        myfile << "\n";
        for(int i=0; i<(int)submode_steps; i++)
        {
            //stop outputting when last entry is over (number == 0)
            if(statsAll[i].numberOfNodes == 0)
                break;

            //file output
            myfile << statsAll[i].numberOfPackets << ";" << collisionSequences[i] << ";" << (double)statsAll[i].delay_avg*1000;
            if(mode == 7 || mode == 9 || mode == 10)
                myfile << ";" << statsAll[i].RTNS_reliability;
            myfile << endl;

            //console output
            if(mode != 0) EV << "number of max packets: " << statsAll[i].numberOfPackets << endl;
            EV << "Sequences total: " << statsAll[i].sequencesTotal << "   lost: " << statsAll[i].sequencesLost
                    << "  loss in percent: " << collisionSequences[i] << "\n";
            EV << "avg delay [ms]: " << statsAll[i].delay_avg*1000 << endl;
            EV << "avg packets per sequence: " << statsAll[i].packets_per_sequence_avg << endl;
            EV << "----------------------------------------------------------" << endl << endl;
        }
    }
    else if(submode == 4)//increase RX_TX_switching_time
    {
        EV << "submode:" << submode << "   #nodes:" << currentActiveNodes << "  #packets:" << currentPacketCountPerSequence << "  deadline:" << deadline << "  steps:" << submode_steps << endl;
        EV << "----------------------------------------------------------" << endl << endl;
        myfile << "submode:" << submode << "   nodes:" << currentActiveNodes << "  packets:" << currentPacketCountPerSequence << "  deadline:" << deadline << endl;
        myfile << "RX_TX_switching_time[us]; sequence loss [%]; avg delay [ms]; rx time [ms]; tx time [ms]; sleep time[ms];";
        if(mode == 7 || mode == 9 || mode == 10)
            myfile << "RTNS_reliability";
        myfile << "\n";
        for(int i=0; i<(int)submode_steps; i++)
        {
            //stop outputting when last entry is over (number == 0)
            if(statsAll[i].numberOfNodes == 0)
                break;

            double temp = RX_TX_switching_time_start + (RX_TX_switching_time_stop-RX_TX_switching_time_start)/(submode_steps-1)*i;
            temp *= 1e6;

            //file output
            myfile << temp << ";" << collisionSequences[i] << ";" << (double)statsAll[i].delay_avg*1000 << ";" << statsAll[i].rx_time_avg*1000 << ";" << statsAll[i].tx_time_avg*1000 << ";" << statsAll[i].sleep_time_avg*1000;
            if(mode == 7 || mode == 9 || mode == 10)
                myfile << ";" << statsAll[i].RTNS_reliability;
            myfile << endl;

            //console output
            if(mode != 0) EV << "RX_TX_switching_time: " << temp << " us" << endl;
            EV << "Sequences total: " << statsAll[i].sequencesTotal << "   lost: " << statsAll[i].sequencesLost
                    << "  loss in percent: " << collisionSequences[i] << "\n";
            EV << "avg delay [ms]: " << statsAll[i].delay_avg*1000 << endl;
            EV << "avg packets per sequence: " << statsAll[i].packets_per_sequence_avg << endl;
            EV << "----------------------------------------------------------" << endl << endl;
        }
    }
    else if(submode == 5)//increase deadline
    {
        EV << "submode:" << submode << "   #nodes:" << currentActiveNodes << "  #packets:" << currentPacketCountPerSequence << "  steps:" << submode_steps << endl;
        EV << "----------------------------------------------------------" << endl << endl;
        myfile << "submode:" << submode << "   nodes:" << currentActiveNodes << "  packets:" << currentPacketCountPerSequence << "  deadline:" << deadline << endl;
        myfile << "deadline [ms]; sequence loss [%]; avg delay [ms]; rx time [ms]; tx time [ms]; sleep time[ms];";
        if(mode == 7 || mode == 9 || mode == 10)
            myfile << "RTNS_reliability";
        myfile << "\n";
        for(int i=0; i<(int)submode_steps; i++)
        {
            //stop outputting when last entry is over (number == 0)
            if(statsAll[i].numberOfNodes == 0)
                break;

            double temp = deadline_start + (deadline_stop-deadline_start)/(submode_steps-1)*i;
            temp *= 1e3;

            //file output
            myfile << temp << ";" << collisionSequences[i] << ";" << (double)statsAll[i].delay_avg*1000 <<  ";" << statsAll[i].rx_time_avg*1000 << ";" << statsAll[i].tx_time_avg*1000 << ";" << statsAll[i].sleep_time_avg*1000;
            if(mode == 7 || mode == 9 || mode == 10)
                myfile << ";" << statsAll[i].RTNS_reliability;
            myfile << endl;

            //console output
            if(mode != 0) EV << "deadline: " << temp << " ms" << endl;
            EV << "Sequences total: " << statsAll[i].sequencesTotal << "   lost: " << statsAll[i].sequencesLost
                    << "  loss in percent: " << collisionSequences[i] << "\n";
            EV << "avg delay [ms]: " << statsAll[i].delay_avg*1000 << endl;
            EV << "avg packets per sequence: " << statsAll[i].packets_per_sequence_avg << endl;
            EV << "----------------------------------------------------------" << endl << endl;
        }
    }
    else if(submode == 6)//increase packet length
    {
        EV << "submode:" << submode << "   #nodes:" << currentActiveNodes << "  #packets:" << currentPacketCountPerSequence << "  deadline:" << deadline << "  steps:" << submode_steps << endl;
        EV << "----------------------------------------------------------" << endl << endl;
        myfile << "submode:" << submode << "   nodes:" << currentActiveNodes << "  packets:" << currentPacketCountPerSequence << "  deadline:" << deadline << endl;
        myfile << "packet length [bits]; sequence loss [%]; avg delay [ms]; rx time [ms]; tx time [ms]; sleep time[ms];";
        if(mode == 7 || mode == 9 || mode == 10)
            myfile << "RTNS_reliability";
        myfile << "\n";
        for(int i=0; i<(int)submode_steps; i++)
        {
            //stop outputting when last entry is over (number == 0)
            if(statsAll[i].numberOfNodes == 0)
                break;

            double temp = packet_length_start + (packet_length_stop-packet_length_start)/(submode_steps-1)*i;
            temp = floor(temp);

            //file output
            myfile << temp << ";" << collisionSequences[i] << ";" << (double)statsAll[i].delay_avg*1000 <<  ";" << statsAll[i].rx_time_avg*1000 << ";" << statsAll[i].tx_time_avg*1000 << ";" << statsAll[i].sleep_time_avg*1000;
            if(mode == 7 || mode == 9 || mode == 10)
                myfile << ";" << statsAll[i].RTNS_reliability;
            myfile << endl;

            //console output
            if(mode != 0) EV << "packet length: " << temp << " bits" << endl;
            EV << "Sequences total: " << statsAll[i].sequencesTotal << "   lost: " << statsAll[i].sequencesLost
                    << "  loss in percent: " << collisionSequences[i] << "\n";
            EV << "avg delay [ms]: " << statsAll[i].delay_avg*1000 << endl;
            EV << "avg packets per sequence: " << statsAll[i].packets_per_sequence_avg << endl;
            EV << "----------------------------------------------------------" << endl << endl;
        }
    }
/////////////////////////////////////// Normal operation ////////////////////////////////////////////////////////////////////////////////////
    else
    {
        myfile << "Number of Nodes; Numbers of packets; sequence loss [%]; packet loss [%]; avg delay [ms]; min delay [ms]; max delay [ms]; receive time [ms]; transmit time [ms]; sleep time[ms]; sent packets per sequence; skipped packets per sequence; ";
        if(mode == 7 || mode == 9 || mode == 10)
            myfile << "RTNS_reliability";
        myfile << "\n";

        //output all results to console
        for (int i = 0; i <= maxHosts; i++)
        {

            //stop outputting when last entry is over (number == 0)
            if(statsAll[i].numberOfNodes == 0)
                break;


            myfile << statsAll[i].numberOfNodes << ";" << statsAll[i].numberOfPackets << ";" << collisionSequences[i] << ";" << collisionPackets[i]
                   << ";" << (double)statsAll[i].delay_avg*1000 << ";" << (double)statsAll[i].delay_min*1000 << ";" << (double)statsAll[i].delay_max*1000 << ";" << statsAll[i].rx_time_avg*1000 << ";" << statsAll[i].tx_time_avg*1000
                   << ";" << statsAll[i].sleep_time_avg*1000 << ";" << statsAll[i].packets_per_sequence_avg << ";" << statsAll[i].packets_skipped_avg;
                   //(double)statsAll[i].avg_delay * 1000 << ";" << statsAll[i].packetsSkipped << ";" << mean << ";" <<"\n";
            if(mode == 7 || mode == 9 || mode == 10)
                myfile << ";" << statsAll[i].RTNS_reliability;

            myfile << endl;



            //output value to console
            EV << "mode: " << mode << "  WC_MODE: " << WC_MODE << "  PAUSE: " << PAUSE <<endl;
            //EV << "Clock Drift; sequence loss [%]; packet loss [%]; avg delay [ms]; \n";

            EV << "number of nodes: " << statsAll[i].numberOfNodes << endl;
            if(mode != 0) EV << "number of max packets: " << statsAll[i].numberOfPackets << endl;
            if(mode != 0) EV << "Packets total: " << statsAll[i].packetsTotal << "   lost: " << statsAll[i].packetsLost
                    << "  loss in percent: " << collisionPackets[i] << "\n";
            EV << "Sequences total: " << statsAll[i].sequencesTotal << "   lost: " << statsAll[i].sequencesLost
                    << "  loss in percent: " << collisionSequences[i] << "\n";
            EV << "ACKsLost: " << statsAll[i].ACKsLost << endl;
            EV << "packets skipped total: " << statsAll[i].packetsSkipped << endl;
            EV << "avg receive time per sequence[ms]: " << statsAll[i].rx_time_avg*1000 << endl;
            EV << "avg transmit time per sequence [ms]: " << statsAll[i].tx_time_avg*1000 << endl;
            EV << "avg delay [ms]: " << statsAll[i].delay_avg*1000 << endl;
            EV << "avg packets per sequence: " << statsAll[i].packets_per_sequence_avg << endl;

            EV << "debug: statsAll[i].packetsGood: " << statsAll[i].packetsGood << endl;
            EV << "debug: statsAll[i].sequencesGood: " << statsAll[i].sequencesGood << endl;
            EV << "debug: statsAll[i].packetsLost: " << statsAll[i].packetsLost << endl;
            EV << "debug: statsAll[i].sequencesLost: " << statsAll[i].sequencesLost << endl;
            EV << "debug: statsAll[i].sequencesTotal: " << statsAll[i].sequencesTotal << endl;
            EV << "----------------------------------------------------------" << endl << endl;

        } //end: output all results to console: for (int i = 0; i< 101; i++)
    }

    myfile.close();
    delete hopCountStats;
}


//increase number of nodes or number of packets for simulation
void Server::start_next_iteration()
{
    EV << "server: start_next_iteration() called" << endl;

    Enter_Method_Silent();

    //save all data
    statsAll[storage_index].min = (int)hopCountStats->getMin();
    statsAll[storage_index].max = (int)hopCountStats->getMax();
    statsAll[storage_index].mean = (float)hopCountStats->getMean();
    statsAll[storage_index].stddev = (float)hopCountStats->getStddev();
    statsAll[storage_index].packetsGood = packetsGood;
    statsAll[storage_index].packetsLost = packetsLost;
    statsAll[storage_index].packetsTotal = packetsTotal;
    statsAll[storage_index].sequencesGood = sequencesGood;
    statsAll[storage_index].sequencesLost = sequencesLost;
    statsAll[storage_index].sequencesTotal = sequencesTotal;
    statsAll[storage_index].numberOfPackets = currentPacketCountPerSequence;
    statsAll[storage_index].numberOfNodes = currentActiveNodes;
    statsAll[storage_index].ACKsLost = ACKsLost;
    statsAll[storage_index].packetsSkipped = packetsSkipped;
    statsAll[storage_index].deadline_missed_counter = deadline_missed_counter;
    statsAll[storage_index].delay_avg = delay->getMean();
    statsAll[storage_index].delay_min = delay->getMin();
    statsAll[storage_index].delay_max = delay->getMax();
    statsAll[storage_index].rx_time_avg = rx_time->getMean();
    statsAll[storage_index].rx_time_min = rx_time->getMin();
    statsAll[storage_index].rx_time_max = rx_time->getMax();
    statsAll[storage_index].tx_time_avg = tx_time->getMean();
    statsAll[storage_index].tx_time_min = tx_time->getMin();
    statsAll[storage_index].tx_time_max = tx_time->getMax();
    statsAll[storage_index].packets_per_sequence_avg = packets_per_sequence->getMean();
    statsAll[storage_index].packets_skipped_avg = packets_skipped->getMean();
    statsAll[storage_index].sleep_time_avg = sleep_time->getMean();

    //RTNS bidirectional
    if(mode == 7 || mode == 9 || mode == 10)
    {
        double data_duration_ = packet_duration;
        double RTNS_tmax1, RTNS_tmin1, buf;

        //maximize reliability
        if(RTNS_reliability_max == true)
        {
            if(mode == 7)
            {
                RTNS_tmax1 = (deadline - data_duration_)/(double)currentPacketCountPerSequence;
                RTNS_tmin1 = RTNS_tmax1/2;
                buf = (2*RTNS_m*((double)currentActiveNodes-1)*data_duration_) / (RTNS_tmax1-RTNS_tmin1);
                RTNS_reliability = 1 - pow(buf, currentPacketCountPerSequence);
            }
            else if(mode == 9)
            {
                data_duration_ = packet_duration + RX_TX_switching_time + ACK_duration + RX_TX_switching_time;
                RTNS_tmax1 = (deadline - data_duration_)/(double)currentPacketCountPerSequence;
                RTNS_tmin1 = RTNS_tmax1/2;
                buf = (2*RTNS_m*((double)currentActiveNodes-1)*data_duration_) / (RTNS_tmax1-RTNS_tmin1);
                RTNS_reliability = 1 - pow(buf, currentPacketCountPerSequence);
            }
            else if(mode == 10)
            {
                RTNS_tmax1 = (deadline - cs_duration - RX_TX_switching_time - data_duration_ - RX_TX_switching_time - ACK_duration)/(double)currentPacketCountPerSequence;
                RTNS_tmin1 = RTNS_tmax1/2;
                double delta_tot = cs_duration + RX_TX_switching_time + data_duration_ + RX_TX_switching_time + ACK_duration - cs_sensitivity;
                buf = (((double)currentActiveNodes-1)*delta_tot) / (RTNS_tmax1-RTNS_tmin1);
                RTNS_reliability = 1 - pow(buf, currentPacketCountPerSequence);
            }
            statsAll[storage_index].RTNS_reliability = RTNS_reliability;
        }
        else
        {
            statsAll[storage_index].RTNS_reliability = RTNS_reliability;
        }
    }

    //debug
    //std::cout << "start_next_iteration() called with storage_index:" << storage_index << "  deadline:" << deadline << "  RTNS_reliability:" << RTNS_reliability << std::endl;

    storage_index++;


    initVariables();
    //cancel events
    cancelEvent(ACK_startEvent);
    cancelEvent(ACK_finishedEvent);
    ackScheduled = false;

    ////////////////////////
    ////////////////////////
    //Configure settings for next iterations
    ////////////////////////

    /////////////////////////////////////////////////////////////
    //Clock drift
    /////////////////////////////////////////////////////////////
    if(ClockDriftPlotMode == true)
    {
        ClockDriftPlotCurrentStepNumber++;

        if(ClockDriftPlotCurrentStepNumber >= ClockDriftPlotStepNumber+1)
        {
            this->endSimulation(); //will call finish()
            return;
        }

        EV << "clock drift iterator: " << ClockDriftPlotCurrentStepNumber << "   progress: " << ceil((double)ClockDriftPlotCurrentStepNumber/((double)ClockDriftPlotStepNumber+1)*100) << "%" << endl;
        std::cout << "clock drift iterator: " << ClockDriftPlotCurrentStepNumber << "   progress: " << ceil((double)ClockDriftPlotCurrentStepNumber/((double)ClockDriftPlotStepNumber+1)*100) << "%" << std::endl;

    }



    /////////////////////////////////////////////////////////////
    //external interference
    /////////////////////////////////////////////////////////////
    else if(ExternalInterferencePlotMode == true)
    {
        //check if max iteration number has been reached
        ExternalInterferencePlotCurrentStepNumber++;
        if(ExternalInterferencePlotCurrentStepNumber >= ExternalInterferencePlotStepNumber+1)
        {
            this->endSimulation(); //will call finish()
            return;
        }

        //increase duty cycle
        ExternalInterferenceCurrentDutyCycle = (ExternalInterferenceDutyCycle / ExternalInterferencePlotStepNumber) * ExternalInterferencePlotCurrentStepNumber;

        //inform hosts about next iteration
        Host *host;
        for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
        {
            if (iter()->isName("host")) // if iter() is in the same vector as this module
            {
                host = check_and_cast<Host *>(iter());
                if(host->getIndex() < currentActiveNodes)
                {
                    host->update_plot_step_number(true, ExternalInterferencePlotCurrentStepNumber);
                    //host->activate();
                }
            }
        }
        //restart events
        cancelEvent(ExternalInterferenceEvent);
        ExternalInterferenceEvent_is_active = false;
        channelBusy = false;
        scheduleAt(simTime() + ExternalInterference_active_time, ExternalInterferenceEvent);

        //output for better visuability
        EV << "external interference level: " << ExternalInterferenceCurrentDutyCycle << "   progress: " << ceil((double)ExternalInterferencePlotCurrentStepNumber/((double)ExternalInterferencePlotStepNumber+1)*100) << "%" << endl;
        std::cout << "external interference level: " << ExternalInterferenceCurrentDutyCycle << "   progress: " << ceil((double)ExternalInterferencePlotCurrentStepNumber/((double)ExternalInterferencePlotStepNumber+1)*100) << "%" << std::endl;
    }// end external interference


    /////////////////////////////////////////////////////////////
    //submodes
    /////////////////////////////////////////////////////////////
    else if (submode == 1)  //increase packet numbers k
    {
        //check if max iteration number has been reached
        currentPacketCountPerSequence++;
        if(currentPacketCountPerSequence > maxPackets)
            this->endSimulation(); //will call finish()

        std::cout << "currentPacketCountPerSequence: " << currentPacketCountPerSequence << " currentActiveNodes: " << currentActiveNodes << std::endl;
    }
    else if (submode == 4) //increase RX_TX_switching_time
    {
        //check if max iteration number has been reached
        submode_step_current++;
        if(submode_step_current >= submode_steps)
            this->endSimulation(); //will call finish()

        //calculate new RX_TX_switching_time
        double delta = (RX_TX_switching_time_stop - RX_TX_switching_time_start)/(submode_steps-1);
        RX_TX_switching_time = RX_TX_switching_time_start + delta*submode_step_current;
        cs_duration = 2 * cs_sensitivity +  RX_TX_switching_time + processing_delay;
        TDMA_calculate_guard_time();
        std::cout << "submode_step_current: " << submode_step_current << " RX_TX_switching_time: " << RX_TX_switching_time*1e6 << std::endl;
    }
    else if(submode == 5)//increase deadline
    {
        //check if max iteration number has been reached
        submode_step_current++;
        if(submode_step_current >= submode_steps)
            this->endSimulation(); //will call finish()

        //calculate new deadline
        double delta = (deadline_stop - deadline_start)/(submode_steps-1);
        deadline = deadline_start + delta*submode_step_current;
        calculate_restart_interval(); //has to be updated due to dependencies
        TDMA_calculate_guard_time();
        std::cout << "submode_step_current: " << submode_step_current << " deadline: " << deadline*1e3 << std::endl;
    }
    else if(submode == 6)//increase packet length
    {
        //check if max iteration number has been reached
        submode_step_current++;
        if(submode_step_current >= submode_steps)
            this->endSimulation(); //will call finish()

        //calculate new packet length
        double temp = packet_length_start + (packet_length_stop-packet_length_start)/(submode_steps-1)*submode_step_current;
        temp = floor(temp); //round down
        packet_duration = temp/txRate;
        TDMA_calculate_guard_time();
        std::cout << "submode_step_current: " << submode_step_current << " packet length: " << temp << " bits"  << std::endl;
        std::cout << "packet_duration: " << packet_duration*1e6 << std::endl;
    }

    /////////////////////////////////////////////////////////////
    //normal mode (no clock drift & external interference)
    /////////////////////////////////////////////////////////////
    else if(mode == 5) //CSMA
    {
        if(currentActiveNodes >= maxHosts)
                this->endSimulation(); //will call finish()

        if (CSMA_max_retransmissions == -1)
            currentPacketCountPerSequence = currentActiveNodes;
        else
            currentPacketCountPerSequence = CSMA_max_retransmissions;

        if(currentActiveNodes < HostStepSize)
           currentActiveNodes = HostStepSize;
        else
           currentActiveNodes = currentActiveNodes + HostStepSize;

        std::cout << "currentPacketCountPerSequence: " << currentPacketCountPerSequence << " currentActiveNodes: " << currentActiveNodes << std::endl;
    }
    else if(mode == 11) //TDMA with ACK
    {
        if(currentActiveNodes >= maxHosts)
            this->endSimulation(); //will call finish()
        if(currentActiveNodes < HostStepSize)
           currentActiveNodes = HostStepSize;
        else
           currentActiveNodes = currentActiveNodes + HostStepSize;
        std::cout << "currentPacketCountPerSequence: " << currentPacketCountPerSequence << " currentActiveNodes: " << currentActiveNodes << std::endl;
    }
    else
    {
        if(currentActiveNodes >= maxHosts && currentPacketCountPerSequence >= maxPackets)
                this->endSimulation(); //will call finish()

        //finished iterating packets? If so, then increment node number
        if((currentPacketCountPerSequence >= maxPackets) || (currentPacketCountPerSequence >= currentActiveNodes))
        {
            if(currentActiveNodes < HostStepSize)
                currentActiveNodes = HostStepSize;
            else
                currentActiveNodes = currentActiveNodes + HostStepSize;

            EV << "increasing hosts: " << currentActiveNodes << endl;
            if(minPackets == -1)
                currentPacketCountPerSequence = currentActiveNodes;
            else
                currentPacketCountPerSequence = minPackets;
        }
        else
        {
            currentPacketCountPerSequence++;
            EV << "increasing currentPacketCountPerSequence: " << currentPacketCountPerSequence << endl;
        }
        EV << "currentPacketCountPerSequence: " << currentPacketCountPerSequence << " currentActiveNodes: " << currentActiveNodes << endl;
        std::cout << "currentPacketCountPerSequence: " << currentPacketCountPerSequence << " currentActiveNodes: " << currentActiveNodes << std::endl;
    }





    //change transmission scheme
    Host *host;
    for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
    {
        if (iter()->isName("host")) // if iter() is in the same vector as this module
        {
            host = check_and_cast<Host *>(iter());
            if(host->getIndex() < currentActiveNodes)
            {
                host->stop_transmission();
                host->change_transmission_scheme(currentPacketCountPerSequence, currentActiveNodes, RX_TX_switching_time, deadline, packet_duration);
            }
        }
    }

    //EV << "currentPacketCountPerSequence: " << currentPacketCountPerSequence << " currentActiveNodes: " << currentActiveNodes << endl;
    //std::cout << "currentPacketCountPerSequence: " << currentPacketCountPerSequence << " currentActiveNodes: " << currentActiveNodes << std::endl;


    if(receiverInitiated == true)// && mode != 8) //no TDMA
    {
        cancelEvent(restartEvent);
        scheduleAt(simTime()+10, restartEvent);
    }
    else if(mode == 8) //TDMA
    {
        cancelEvent(TDMA_sync_start);
        cancelEvent(TDMA_sync_finished);
        scheduleAt(simTime() + 0.1, TDMA_sync_start);
    }
    else if (mode == 11)
    {
        //do nothing here, activation is done when beacon is received

        if(haltOnDeadlines > 0)
            haltOnPacketNumberSent = haltOnDeadlines * currentActiveNodes;
    }
    else
    {
        //activate hosts
        Host *host;
        //change transmission scheme
        for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
        {
            if (iter()->isName("host")) // if iter() is in the same vector as this module
            {
                host = check_and_cast<Host *>(iter());
                if(host->getIndex() < currentActiveNodes)
                {
                    host->activate();
                }
            }
        }
    }

}


void Server::initVariables()
{
    //initialize packetList
    for(int i=0; i<currentActiveNodes+1; i++)
    {
        packetList[i].packetsReceivedTotal = 0;
        packetList[i].packetsReceivedGood = 0;
        packetList[i].firstPacketIndex = 1000;
        packetList[i].packetsSkipped = 0;

        nodesFinishedTransmission[i] = 1000;
    }

    lastID = 0;
    lastPacketGood = true;
    nodesFinishedTransmissionNumber = 0;
    sequencesGood = 0;
    sequencesLost = 0;
    sequencesTotal = 0;
    packetsGood = 0;
    packetsLost = 0;
    packetsTotal = 0;
    currentCollisionNumFrames = 0;
    ACKsLost = 0;
    packetsSkipped = 0;
    packetsSkippedTotal = 0;
    total_packets_transmitted = 0;
    total_packets_skipped = 0;
    total_acks_transmitted = 0;
    deadline_missed_counter = 0;
    last_state = false;
    receivingPaket = false;
    next_iteration_flag = false;
    is_switching_mode = false;
    TDMA_sync_successful = true;
    TDMA_current_beacon_per_deadline = 1;

    //reset storage containers
    rx_time->clearResult();
    tx_time->clearResult();
    delay->clearResult();
    packets_per_sequence->clearResult();
    packets_skipped->clearResult();
    sleep_time->clearResult();

    ExternalInterferenceEvent_is_active = false;
}



void Server::sendACKPacket(char *name, int host_id)
{
    Host *host;
    cPacket *pk = new cPacket(name);
    pk->setBitLength(ACKLenBits->longValue());
    pk->addPar("corrupt");
    pk->par("corrupt").setBoolValue(ACK_was_lost);

    //send packet
    for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
    {
        if (iter()->isName("host"))
        {
            host = check_and_cast<Host *>(iter());
            if(host->getIndex() == host_id)
            {
                sendDirect(pk, 0, 0, host->gate("in"));
                break;
            }
        }
    }

}

void Server::sendBeacon(int number = 1)
{
    Host *host;
    cPacket *pk;

    //send packet
    for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
    {
        if (iter()->isName("host"))
        {
            host = check_and_cast<Host *>(iter());
            if(host->getIndex() >= currentActiveNodes)  //beacon only to active nodes
                continue;

            pk = new cPacket("beacon");
            pk->setBitLength(TDMA_beacon_size);
            pk->addPar("corrupt");
            pk->par("corrupt").setBoolValue(!TDMA_sync_successful);
            pk->addPar("number");
            pk->par("number").setLongValue(number);

            sendDirect(pk, 0, 0, host->gate("in"));
        }
    }
}


//OBSOLETE -> do not use
//used to inform hosts if channel is busy or not
//for persistent CSMA mode (nodes will then pause their backoff time)
void Server::inform_hosts_about_channel_state(bool channel_busy, bool wait_DIFS = false)
{
    wait_DIFS = false;  //debug


    //mode 5.2 only
    if(mode != 5 || CSMA_mode != 2)
        return;

    if(last_state != channel_busy)
    {
        Host *host;
        for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
        {
            if (iter()->isName("host")) // if iter() is in the same vector as this module
            {
                host = check_and_cast<Host *>(iter());
                int index = host->getIndex();
                if(index < currentActiveNodes)// && host_id_carrier_sense[index] == true)  //inform only hosts that are currently in carrier sense mode
                {
                    host->update_channel_state(channel_busy);
                }
            }
        }
    }
    else
    {
        // do nothing
    }
    last_state = channel_busy;



    /*else if (first_called == false || last_state == channel_busy)
    {
        EV << "Server skip inform_hosts_about_channel_state: state " << channel_busy << endl;
        return; //skip channel state has not changed since last function call to reduce overhead
    }*/

    /*if(channel_busy == true && wait_DIFS == true)
    {
        cancelEvent(CSMA_waited_DIFS);
        scheduleAt(simTime() + CSMA_DIFS, CSMA_waited_DIFS);
    }
    else
    {
        Host *host;
        for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
        {
            if (iter()->isName("host")) // if iter() is in the same vector as this module
            {
                host = check_and_cast<Host *>(iter());
                if(host->getIndex() < currentActiveNodes)
                {
                    host->update_channel_state(channel_busy);
                }
            }
        }
    }
    if(wait_DIFS == false)
    {
        cancelEvent(CSMA_waited_DIFS);
    }*/

}

//hosts will only be informed if channel is busy during their sensing interval (i.e., the time they are subscribed)
void Server::carrier_sense_update()
{
    //if (state == true)
    //    return;

    //exit if function is not enabled
    if(carrierSenseMode !=1)
        return;

    //EV << "Server: carrier_sense_update() called" << endl;

    Host *host;

    for (SubmoduleIterator iter(getParentModule()); !iter.end(); iter++)
    {
        if (iter()->isName("host")) // if iter() is in the same vector as this module
        {
            host = check_and_cast<Host *>(iter());
            int id = host->getIndex();

            //iterate through subscriber list
           //for(std::list<int>::iterator it = carrier_sense_subscriber_list.begin(); it != carrier_sense_subscriber_list.end(); ++it)
           //{
               //if(host->getIndex() == *it)
            if(std::find(carrier_sense_subscriber_list.begin(), carrier_sense_subscriber_list.end(), id) != carrier_sense_subscriber_list.end())
               {
                   //inform about blocked channel
                   host->carrier_sense_callback();
                   //clear informed host (to avoid multiple notifications; host will subscribe anew next cs interval)
                   carrier_sense_subscriber_list.remove(id);
                   packetsSkipped++;
                   break;
               }
           //}
        }
    }


    //inform all subscribed hosts (those which are doing carrier sensing) that the channel is busy

}

void Server::carrier_sense_subscribe(bool subscribe, int id)
{
    if(subscribe == true)
    {
        //add id to list
        carrier_sense_subscriber_list.push_back(id);
        EV << "Server: id " << id << " subscribed" << endl;
    }
    else
    {
        if(carrier_sense_subscriber_list.empty() == false)
        {
            carrier_sense_subscriber_list.remove(id);
            EV << "Server: id " << id << " unsubscribed" << endl;
             //iterate through list and find id and clear entry
            /*for(std::list<int>::iterator it = carrier_sense_subscriber_list.begin(); it != carrier_sense_subscriber_list.end(); ++it)
            {
                if(*it == id)
                {
                    carrier_sense_subscriber_list.erase(it);
                    EV << "Server: id " << id << " unsubscribed" << endl;
                    break;
                }
            }*/
        }
    }

    //debug: print list
    if(carrier_sense_subscriber_list.empty() == false)
    {
        EV << "Server: id list ";
        for(std::list<int>::iterator it = carrier_sense_subscriber_list.begin(); it != carrier_sense_subscriber_list.end(); ++it)
        {
            EV << id << "  ";
        }
        EV << endl;
    }
    //else
        //EV << "Server: list empty" << endl;

}

//true: channel is busy
bool Server::carrier_sense_simple()
{
    return channelBusy;
}


// is called by host when it finishes its sequence (when it successfully received an ACK) -> sends all recorded data to the sink
// for ack-based modes only
void Server::notify_about_transmission(int ID, bool success, double rx_time_, double tx_time_)
{
    sequencesTotal++;
    if(success == false)
    {
        //EV << "Server: ID " << ID << "  transmission lost!!" << endl;
        sequencesLost++;
        //rx_time->collect(rx_time_);
        //tx_time->collect(tx_time_);
    }
    else
    {
        sequencesGood++;
        //rx_time->collect(temp_rx_time);
        //tx_time->collect(temp_tx_time);
    }
    rx_time->collect(rx_time_);
    tx_time->collect(tx_time_);

    EV << "Server: notify_about_transmission(ID " << ID << ", success " << success << ") called" << endl;
    EV << "debug: sequencesGood: "<< sequencesGood << "  sequencesLost: " << sequencesLost << endl;

    //collect data from last (successful) transmission
    //could be passed as an argument in the future
    delay->collect(temp_delay);
    packets_per_sequence->collect(temp_packets_per_sequence);
    packets_skipped->collect(temp_packets_skipped);
    sleep_time->collect(temp_sleep_time);

    //switch to next iteration if necessary
    if(sequencesTotal >= haltOnPacketNumberSent) // && receiveBeginSignal == false) // && receiverInitiated == false)
    {
        if(receiverInitiated)
            next_iteration_flag = true;
        else
        {
            this->start_next_iteration();
        }

    }

    //EV << "temp_delay: " << temp_delay << "    temp_rx_time: " << temp_rx_time << "    temp_tx_time: " << temp_tx_time << endl;
}


void Server::calculate_restart_interval()
{
    switch(mode)
    {
    case 5: if(deadline_is_used)
                restartInterval = 2 * deadline;
            else
                restartInterval = 10; //TODO calculate this
    case 7:
    case 9:
    case 10:
        restartInterval = 2 * deadline;
        break;
    case 11:
        restartInterval = 2 * deadline; //factor 2 due to possible clock drifts (maximum clock drift is 100%)
    default:
        restartInterval = 10;   //TODO for other modes
    }
}

void Server::TDMA_calculate_guard_time()
{
    if(mode != 11)
        return;
    double time_per_cycle = deadline / TDMA_cycles_per_deadline;
    time_per_cycle = time_per_cycle - (TDMA_beacon_duration + 2*RX_TX_switching_time) - maxHosts*(packet_duration + 2*RX_TX_switching_time + ACK_duration);  //substract one beacon and all packet slots (without guard time intervals)
    TDMA_guard_time = time_per_cycle / (maxHosts+1);
    EV << "guard time:" << TDMA_guard_time*1e6 << " us" << endl;
}

}; //namespace
