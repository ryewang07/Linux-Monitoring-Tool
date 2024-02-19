# Linux-Monitoring-Tool
A monitoring tool for Linux devices.

how to run 

Compile the code with: gcc mySystemStats.c -o mySystemStats
Then call: ./mySystemStats  flags        See the list of flags below:
Flags:
	--user: reports the user usage(how many users are logged in)
 	--system: reports the system info(ram, cpu)
--sequential: reports the information sequentially without a “refresh”
--samples=N: where N (an integer) indicates how many times the statistics will be printed,                 
Note: if no integer is present after the = sign, it will be assumed to be 0 and invalid 
integers after the = sign will result in an error. Default is 10
--tdelay=T: where T indicates how frequently each sample will be printed in seconds, 
	Note: if no integer is present after the = sign, it will be assumed to be 0 and invalid 
	integers after the = sign will result in an error.  Default is 1
a b: where a and b both represent an integer, specifically samples, tdelay respectively. 
Note it must be written as a b (i.e 10 1 indicating 10 samples with 1 second delay between each) otherwise it will result in an invalid argument (i.e 10 –users 2 is not allowed) 
