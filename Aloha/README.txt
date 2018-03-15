Simulation Code for Journal Paper "Accounting for Reliability in Unacknowledged Time-Constrained WSNs" by Philip Parsch and Alejandro Masrur, TU Chemnitz, Germany (contact: philip.parsch@cs.tu-chemnitz.de)

Please note that OMNeT++ Version 4.6 is need to run this code. All configuration is done in omnetpp.ini. Results are outputted as a .csv-file and can be processed by Matlab or Excel.

Configuration (in omnetpp.ini) for the graphs:
CSMA is mode 5
TDMA is mode 11
proposed is mode 7
ACK-based is mode 10

-Fig.10a: (worst-case has been calculated and not simulated)
minHosts = 2
maxHosts = 150
submode = 0

-Fig.10b:
minHosts = 75 or 150
maxHosts = 75 or 150
submode = 5

-Fig.11a & c:
minHosts = 2
maxHosts = 150
submode = 0

-Fig.11b:
minHosts = 75
maxHosts = 75
submode = 4

-Fig.11d:
minHosts = 75
maxHosts = 75
submode = 4

-Fig.12a & b:
minHosts = 75
maxHosts = 75
submode = 0
ExternalInterferenceEnable = true

all other parameters can be left as is.