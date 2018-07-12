/*
 * periods.h
 *
 *  Created on: 10.07.2015
 *      Author: Philip
 */

#ifndef PERIODS_H_
#define PERIODS_H_

extern int period_list_schweden[30][30];
extern double period_list_journal[75][75];
extern double new_alg_periods[101][101];
extern double deadline_list_journal[30];
extern double deadline_list_schweden[30];
extern bool host_id_carrier_sense[512];    //managed by server. True[id]: Host with ID is currently in carrier sense mode ONLY FOR mode 5.2


#endif /* PERIODS_H_ */
