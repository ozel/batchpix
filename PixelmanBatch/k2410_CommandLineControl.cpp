/*
 * batchpixel.cpp
 *
 *  Created on: Mar 2012
 *  Authors:	John Idarraga <idarraga@cern.ch>
 *    			Martin Kuppa <mkroupa@uh.edu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include <string.h>
#include <dlfcn.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <stdlib.h>

#include <sys/time.h>

int open_port(const char *);
void terminate_string(char *);
long GetTimeDiff(struct timeval, struct timeval);

int main(int argc, char ** argv) {

	if(argc < 3) {
	  cout << "use: " << endl;
	  cout << "     " << argv[0] << " voltage port(/dev/ttyUSB0)" << endl;
	  exit(1);
	}

	// Input parameters /////////////////////////////////
	double voltage = atoi(argv[1]);
	string serialPort(argv[2]);
	// Input parameters /////////////////////////////////
	cout << "-- User Config ---------------------------" << endl;
	cout << "voltage  = " << voltage << endl;
	cout << "serial   = " << serialPort << endl;
	cout << "------------------------------------------" << endl;

	int fd = open_port( serialPort.c_str() );

	vector<string> startup;
	startup.push_back("*rst\r");
	startup.push_back("*rst\r");
	startup.push_back(":sour:func volt\r");
	startup.push_back(":sour:volt:mode fixed\r");
	startup.push_back(":sour:volt:range 200\r");
	startup.push_back(":sour:volt:level 0\r"); // set 0 V
	startup.push_back(":sens:curr:prot 100E-6\r");
	startup.push_back(":sens:func \"curr\"\r");
	startup.push_back(":sens:curr:range 10E-6\r");
	startup.push_back(":form:elem curr\r");
	startup.push_back(":output on\r");
	startup.push_back(":read?\r");

	// start up
	vector<string>::iterator itr = startup.begin();
	int n = 0;
	for( ; itr != startup.end() ; itr++) {
	  
	  printf("%s \n",(*itr).c_str());
	  n = write(fd, (*itr).c_str(), (*itr).size());
	  if (n < 0) {
	    fputs("write() of n bytes failed!\n", stderr);
	  }
	  usleep(500000); // 500ms
	}

	// sleep
	usleep(100000);

	string tempC = "";
        char tV[256];
	double lastVoltage;

	// Ramp down the voltage
	cout << "Ramp up ..." << endl;
	double step = voltage / 10.;
	for (double voltage_s = 0 ; voltage_s <= voltage ; voltage_s += step) {
	  
	  // start the string
	  tempC = ":sour:volt:level ";
	  // sprintf the voltage (float)
	  sprintf(tV,"%.1f", voltage_s);
	  // append voltage and CR
	  tempC.append(tV);
	  tempC.append("\r");
	  
	  cout << tempC << endl;
	  n = write(fd, tempC.c_str(), tempC.size());

	  // keep track of last voltage sent
	  lastVoltage = voltage;
	  if (n < 0) {
	    fputs("write() of n bytes failed!\n", stderr);
	  }
	  
	  usleep(100000); // 100ms
	  
	}
	
	// Stabilization //////////////////////////////////
	// Let the new set stabilize
	usleep(500000); // 500ms
	
	// Stay here until hit exit
	string exitcommand;

	while(exitcommand != string("exit")) {
	  cout << "Type \"exit\" to ramp down the voltage and turn off the keithley" << endl;
	  cin >> exitcommand;
	}

	// Ramp down the voltage
	cout << "Ramp down ..." << endl;
	for (float voltage = lastVoltage ; voltage >= 0. ; voltage-=1.0) {
	  
	  // start the string
	  tempC = ":sour:volt:level ";
	  // sprintf the voltage (float)
	  sprintf(tV,"%.1f", voltage);
	  // append voltage and CR
	  tempC.append(tV);
	  tempC.append("\r");
	  
	  cout << tempC << endl;
	  n = write(fd, tempC.c_str(), tempC.size());
	  
	  usleep(100000); // 100ms
	  
	}
	
	// Set keithley output to off
	cout << "Output off ..." << endl;
	tempC = ":outp off \r";
	n = write(fd, tempC.c_str(), tempC.size());
	
	return 0;
}

void terminate_string(char * t) {

	bool foundE = false;
	int itr = 0;
	while (!foundE) {
		if( t[itr] == 'E' ) {
			foundE = true;
			*(t+itr+4) = '\0';
		}
		itr++;
	}

}

/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */

int open_port(const char * port) {
	// File descriptor for the port
	int fd;

	cout << "Openning port : " << port << endl;

	// open
	fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);

	// get current options for the port
	struct termios options;
	tcgetattr(fd, &options);

	// set 19200 baud
	cfsetispeed(&options, B19200);
	cfsetospeed(&options, B19200);

	// set 8N1
	options.c_cflag &= ~PARENB; // no parity bit
	options.c_cflag &= ~CSTOPB; // 1 stop bit
	options.c_cflag &= ~CSIZE;  // no bit mask for data bits
	options.c_cflag |= CS8;

	// raw output
	//options.c_oflag &= ~OPOST;

	// disable hardware flow control
	options.c_cflag &= ~CRTSCTS;

	// Set the new options for the port...
	tcsetattr(fd, TCSANOW, &options);

	if (fd == -1) {
		/*
		 * Could not open the port.
		 */
		perror("open_port: Unable to open port - ");
	}
	else
		fcntl(fd, F_SETFL, 0);

	return (fd);
}

long GetTimeDiff(struct timeval start, struct timeval end){

	long mtime, seconds, useconds;

	// find the time difference
	seconds = end.tv_sec - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	mtime = useconds + seconds*1E6;
	//mtime = ((seconds)*1000 + useconds/1000.0) + 0.5;
	//cout << "time : " << mtime << " milliseconds" << endl;

	return mtime;
}
