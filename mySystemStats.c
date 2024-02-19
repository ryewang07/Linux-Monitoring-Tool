#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <utmp.h>
#include <unistd.h>
#include <ctype.h>
typedef struct MemoryStorage { //Define struct for just the memory info
    float physUsedGB;
    float totalGB;
    float virtUsedGB;
    float virtTotalGB;
}  MemStor;

void showRunningParameters(int samples, int tdelay) { //Prints the samples and tdelay
    struct rusage selfUsage; //Struct to help obtain the self memory utilization 
    printf("Nbr of samples: %d -- every %d secs\n", samples, tdelay);
    if (getrusage(RUSAGE_SELF, &selfUsage) == 0) {
        printf("Memory usage: %ld kilobytes\n", selfUsage.ru_maxrss);
    }
    else{
        printf("error\n"); //In case of error
        exit(1);
    }
    printf("----------------------------------------\n");
}
void createMemoryInfo(MemStor* memInfoArray, int i) { //Only generates a singular instance of the memory used at the time
    FILE *memInfoFile = fopen("/proc/meminfo", "r");
    //Error Check
    if (memInfoFile == NULL) {
        printf("Error opening /proc/meminfo");
        exit(1);
    }

    char checkLine[100];
        long totalMem, freeMem, totalSwap, freeSwap;

        while (fgets(checkLine, sizeof(checkLine), memInfoFile)) { //Obtain the necessary info
            if (strncmp(checkLine, "MemTotal:", 9) == 0) {
                sscanf(checkLine, "MemTotal: %ld kB", &totalMem);
            } else if (strncmp(checkLine, "MemFree:", 8) == 0) {
                sscanf(checkLine, "MemFree: %ld kB", &freeMem);
            } else if (strncmp(checkLine, "SwapTotal:", 10) == 0) {
                sscanf(checkLine, "SwapTotal: %ld kB", &totalSwap);
            } else if (strncmp(checkLine, "SwapFree:", 9) == 0) {
                sscanf(checkLine, "SwapFree: %ld kB", &freeSwap);
            }
        }

        long physMemUsed = totalMem - freeMem;
        long virtMemUsed = totalMem + totalSwap - freeMem - freeSwap; //Convert properly for the right info

        // Convert from KB to GB
        memInfoArray[i].physUsedGB = physMemUsed / 1024.0 / 1024.0; //Store the data
        memInfoArray[i].totalGB = totalMem / 1024.0 / 1024.0;
        memInfoArray[i].virtUsedGB = virtMemUsed / 1024.0 / 1024.0;
        memInfoArray[i].virtTotalGB = (totalMem + totalSwap) / 1024.0 / 1024.0;

    fclose(memInfoFile);
}

void showMemoryInfo(MemStor* memInfoArray, int printsamples, int samples) { // Prints memory info
    //printsamples is the amount+1 of samples wanting to print, samples will be the total
    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
    printf("----------------------------------------\n");
    for(int i = 0; i< printsamples; i++) {
        printf("%.2f GB / %.2f GB  -- %.2f GB / %.2f GB\n", 
        memInfoArray[i].physUsedGB, memInfoArray[i].totalGB, memInfoArray[i].virtUsedGB, memInfoArray[i].virtTotalGB); //Print the necessary samples
    }
    for(int i = 0; i< samples - printsamples ; i++) { //Create new lines for the remaining samples
        printf("\n");
    }
    
    printf("---------------------------------------\n");
}
void showMemoryInfoSEQ(MemStor* memInfoArray, int printsamples, int samples) { //Same function as above (Prints memory info) but for --sequential
    //printsamples means the INDEX at which i want the memory to be called, different from the previous function
    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
    printf("---------------------------------------\n");
    for(int i = 0; i< printsamples; i++) {
        printf("\n");
    } //Spaces above the wanted print of the memory
    printf("%.2f GB / %.2f GB  -- %.2f GB / %.2f GB\n", 
        memInfoArray[printsamples].physUsedGB, memInfoArray[printsamples].totalGB, memInfoArray[printsamples].virtUsedGB, memInfoArray[printsamples].virtTotalGB);
     
    for(int i = 0; i< samples - printsamples -1; i++) {
        printf("\n");
    } //Spaces below the wanted print of the memory
    
    
    printf("---------------------------------------\n");
}

float returnCPUUsage(long* lastTotalCPUCount, long* lastIdle, int firstSampleTested ) {
    FILE *cpuStatFile = fopen("/proc/stat", "r");
    //Error Check
    if (cpuStatFile == NULL) {
        printf("Error opening /proc/stat");
        exit(1);
    }

    char checkLine[100];
    if(firstSampleTested == 0) { //This is have a starting total cpu usage of 0.0 as a reference point for future calculations
        return 0.0;
    }
    else{
    // Read the initial values from /proc/stat
        while (fgets(checkLine, sizeof(checkLine), cpuStatFile)) {
            if (strncmp(checkLine, "cpu ", 4) == 0) {
                long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
                long totalCPU;
                sscanf(checkLine, "cpu %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
                    &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice);

                totalCPU = user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;

                // Calculate total CPU usage
                float totalCpuUsage = (1.0 - (float)(idle - *lastIdle) / (totalCPU - *lastTotalCPUCount)) * 100.0;

                *lastTotalCPUCount = totalCPU;
                
                *lastIdle = idle;

                // Close the file
                fclose(cpuStatFile);

                return totalCpuUsage;
            }
        }
    }
}

void showCPUUsage(float totalCpuUsage) { //Prints the CPU usage calculated from the function above
    int numCores = sysconf(_SC_NPROCESSORS_ONLN); //To obtain the cores
    printf("Number of cores: %d\n", numCores);
    printf("Total CPU use = %.2f%%\n", totalCpuUsage);
    printf("---------------------------------------\n");;
}

void showUserUsage(){  //Prints the Session Users
    struct utmp *userList;
    setutent(); //Open utmp file and point it at beginning

    printf("### Sessions/users ###\n");
    while((userList = getutent()) != NULL) { //Read all the users
        if(userList->ut_type == USER_PROCESS) {
            printf(" %s\t%s (%s)\n", userList->ut_user, userList->ut_line, userList->ut_host);
        }
    }

    endutent(); //Close utmp file
    printf("---------------------------------------\n");

    
}
void showSystemInfo() { //Prints the System Information
    struct utsname systemInfo; //For System Info
    if (uname(&systemInfo) == 0) { //Retrieve system information and check if its successful.
        printf("### System Information ###\n");
        printf("System Name = %s\n", systemInfo.sysname);
        printf("Machine Name = %s\n", systemInfo.nodename);
        printf("Version = %s\n", systemInfo.version);
        printf("Release = %s\n", systemInfo.release);
        printf("Architecture = %s\n", systemInfo.machine);
        
        }
    struct sysinfo systemInfoTime; //For System uptime to find previous reboot.
        if (sysinfo(&systemInfoTime) == 0){
            //Perform calculations since systemInfoTime.uptime gives the amount of time of last reboot in seconds
            int seconds = systemInfoTime.uptime % 60;
            int minutes =  (systemInfoTime.uptime / 60 ) % 60;
            int hours = (systemInfoTime.uptime / 3600) % 24;
            int days = (systemInfoTime.uptime / 86400);
            printf("System running since last reboot: %d days %02d:%02d:%02d (%02d:%02d:%02d) \n", days, hours, minutes, seconds, (days*24 + hours), minutes, seconds);
            printf("---------------------------------------\n");
        }

}
void printAll(int samples, int tdelay, int userFlag, int systemFlag, int sequentialFlag) { //Combines all previous print statements and prints accordingly depending on user inputing flags
    //if any of the flags = 0: indicates that they are not on (off), while 1 means they are on.
    MemStor* memInfoArray = (MemStor*)malloc(samples * sizeof(MemStor)); //Create the memoryinfo when printing it all
    //Error Check
    if(memInfoArray == NULL) {
        printf("error\n");
        exit(1);
    }
    long lastTotalCPUCount = 0; //Initzalize for CPU calculation
    long lastIdle = 0; //Initzalize for CPU calculation
    int firstSampleTested = 0; //Initzalize for CPU calculation
    for (int i = 0; i<samples; i++) {
        //Every if statement is to make sure the right output is displayed
        if(sequentialFlag == 1) {
            printf("iteration: %d\n", i);
        }
        if(sequentialFlag == 0) {
            system("clear"); //This is what implements the potential "refreshing" display if --sequential is not called
        }
        
        showRunningParameters(samples,tdelay); //Always printed so no condition

        if(systemFlag == 1 && sequentialFlag == 0) {
            createMemoryInfo(memInfoArray, i); 
            showMemoryInfo(memInfoArray, i+1, samples); 
        }
        if(systemFlag == 1 && sequentialFlag == 1) {
            createMemoryInfo(memInfoArray, i); 
            showMemoryInfoSEQ(memInfoArray, i, samples); 
        }
        if(userFlag == 1) {
            showUserUsage(); 
        }
        if(systemFlag == 1) {
            if(i > 0) {
                firstSampleTested = 1; //We will have 1 sample of 0.0 total cpu usage by here.
            }
            float totalCpuUsage = returnCPUUsage(&lastTotalCPUCount, &lastIdle, firstSampleTested);
            showCPUUsage(totalCpuUsage);
        }
    
        sleep(tdelay); //To help implement the potential refresh
       
    }
    showSystemInfo(); //Always display the system info at the end shown in the demo
    free(memInfoArray); //Free the array 

    
}

int helpCheckInteger(char *possibleInt) { //Returns 0 if string is not an integer, otherwise returns 1.
    //Helper function to assist in making sure the argument is an integer for argv, i.e to combat the 3--junk  that atol wouldn't help with
    int i = 0;
    while(possibleInt[i] != '\0') {
        if(isdigit(possibleInt[i]) == 0) {
            return 0;
        }
        i++;
    }
    return 1;
}

int main(int argc, char **argv) { //main function is used for parsing arguments.
    //0 means the flag is not called, 1 means it is.
    int sequentialFlag = 0;
    int systemFlag = 0;
    int userFlag = 0;
    int sample = 10;
    int tdelay = 1;

    for (int i = 1; i < argc; i++) { //Parse through arguments to check for the flags user inputed.
        if (strcmp(argv[i], "--system") == 0) {
            systemFlag = 1;
        } else if (strcmp(argv[i], "--user") == 0) {
            userFlag = 1;
        } else if (strcmp(argv[i], "--sequential") == 0) {
            sequentialFlag = 1;
        } else if (strncmp(argv[i], "--samples=", 10) == 0 && helpCheckInteger(argv[i] + 10)) {
            sample = atoi(argv[i] + 10); //Skip the --samples= and check if its a number following the "="
        } else if (strncmp(argv[i], "--tdelay=", 9) == 0 && helpCheckInteger(argv[i] + 9)) {
            tdelay = atoi(argv[i] + 9); //Skip the --tdelay= and check if its a number following the "="
        }
          else if (i + 1 < argc && helpCheckInteger(argv[i]) == 1 &&  helpCheckInteger(argv[i + 1]) == 1) { // Check for two consecutive numbers after a flag
            //i + 1 < argc, incase they insert the two numbers at the end
            //helpCheckInteger(argv[i]) == 1 &&  helpCheckInteger(argv[i + 1]) == 1 to make sure they're both integers
            sample = atoi(argv[i]);
            tdelay = atoi(argv[i + 1]);
            i++;  // Skip the next argument
    
          }
        else{
            printf("Invalid Argument %s\n", argv[i]);
            return 1;
        }
    }

    if ((userFlag == 0 && systemFlag == 0)) { 
        //If both user and system flag are 0 that means the user DID NOT input --system --user, making them automatically on given our description of the assignment
        systemFlag = 1;
        userFlag = 1;
    }
    printAll(sample, tdelay, userFlag, systemFlag, sequentialFlag); //print with the respective flags user inputed.

    return 0;
}