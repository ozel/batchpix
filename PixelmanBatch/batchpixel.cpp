/*
 * batchpixel.cpp
 *
 *  Created on: Mar 2012
 *  Authors:	John Idarraga <idarraga@cern.ch>
 *    			Martin Kuppa <mkroupa@uh.edu>
 */

#include "mpxmanagerapi.h"
#include "mpxhw.h"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
//#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include <vector>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

#include "batchpixel.h"

// prototypes
bool ParseConfFile(string, AllParameters *);
AllParameters * LoadParameterList();
void DumpHardwareItems(DEVID);
void DumpDACs(DACTYPE *, vector<double>);

int ndacs;
int endless_acq;
int toggle_acq = 1; //default is run the acq. loop
int acqcount = 0;

void handle_sigusr1(int sig){
	if(toggle_acq == 0) acqcount = 0; //reset counter
	toggle_acq = !toggle_acq;
}

int main(int argc, char ** argv) {

	if(argc < 2) {
		cout << "use: " << endl;
		cout << "     " << argv[0] << " configure run file (string) " << endl;
		return 1;
	}

	// Start a time stamp
	time_t rawtime;
	time(&rawtime);
	// second
	struct timeval tv;
	gettimeofday(&tv,NULL);

	// Get configure run file
	string confRunFile ( argv[1] );

	// Create the parameter list
	AllParameters * pars = LoadParameterList();

	// Modify parameters from the input file
	ParseConfFile ( confRunFile, pars );

	// Dump the parameters
	pars->DumpAllParameters();


	// Parameters always fixed
	BatchParameter<int> * pacqcounts = (BatchParameter<int> *) pars->GetParByName("AcqCounts");
	int acqcounts_setting = pacqcounts->GetMin();

	BatchParameter<double> * pacqtime = (BatchParameter<double> *) pars->GetParByName("AcqTime");
	double acqTime_setting = pacqtime->GetMin();
	cout << "Acq time = " << acqTime_setting << endl;
	//acqTime_setting = 0.1; //REM

	BatchParameter<string> * pconfig = (BatchParameter<string> *) pars->GetParByName("BPConfig");
	string config = pconfig->GetMin();

	BatchParameter<string> * pdataset = (BatchParameter<string> *) pars->GetParByName("Dataset");
	string dataset = pdataset->GetMin();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Scan parameters
	BatchParameter<int> * pv = (BatchParameter<int> *) pars->GetParByName("V");
	int minV = pv->GetMin();
	int maxV = pv->GetMax();
	int stepV = pv->GetStep();

	BatchParameter<int> * pthl = (BatchParameter<int> *) pars->GetParByName("THL");
	int thl_min = pthl->GetMin();
	int thl_setting = thl_min;
	int thl_max = pthl->GetMax();
	int thl_step = pthl->GetStep();

	BatchParameter<int> * pthlcoarse = (BatchParameter<int> *) pars->GetParByName("THLCoarse");
	int thlcoarse_min = pthlcoarse->GetMin();
	int thlcoarse_setting = thl_min;
	int thlcoarse_max = pthlcoarse->GetMax();
	int thlcoarse_step = pthlcoarse->GetStep();

	BatchParameter<int> * pikrum = (BatchParameter<int> *) pars->GetParByName("IKRUM");
	int ikrum_min = pikrum->GetMin();
	int ikrum_setting = ikrum_min; // for first setting
	int ikrum_max = pikrum->GetMax();
	int ikrum_step = pikrum->GetStep();


	///////////////////////////////////////////////////////////////////////////////////////////////////
	//device specific settings

	// indexes to set and read HV bias and ADCs, depend on fitpix hardware variant!
	int vset_index = 9; //MXR lite: 7;   	//mx-10: 9;
	int vread_index = 10; //MXR lite: 10; 	//mx-10: 10;
	int adcsense_index = 30; //MXR lite: 30 -> ignore; //mx-10:  11;	
	int ndacs = __ndacs_medipixMXR;
	//int ndacs = __ndacs_timepix;
	
	//add signal to pause acquisition loop
	(void) signal(SIGUSR1, handle_sigusr1);
	
	
	///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Init manager
	int control = -1;
	u32 flags = MGRINIT_NOEXHANDLING;
	control = mgrInitManager(flags, '\0');
	if(mgrIsRunning()){
		cout << "Manager running " << endl;
	}

	// Find devices
	DEVID devId;
	int count = 0;
	control = mpxCtrlGetFirstMpx(&devId, &count);

	// Check for devices
	control = mpxCtrlFindNewMpxs();

	// Seems to be necesary to do this a second time after the call to mpxCtrlFindNewMpxs();
	control = mpxCtrlGetFirstMpx(&devId, &count);
	cout << "Number of devices found : " << count << " | first devId : " << devId << endl;
	if ( count < 1 ) {
		cout << "[ERROR] No devices found." << endl;
		return 1;
	}

	// Scan devices and find information
	DevInfo * devInfo = new DevInfo;
	for(DEVID devs = devId ; devs < (DEVID)count ; devs++) {
		control = mpxCtrlGetDevInfo(devs, devInfo);
		cout << devs << ") " << devInfo->chipboardID << endl;
	}
	if ( count > 1 ) { // if there is more than 1 device make a selection
		cout << "Select one by id and hit Enter ..." << endl;
		cin >> devId;
	} else {
		devId = 0; // otherwise simply go ahead with the one and only device discovered
	}

	// sleep
	usleep(100000);

	// Load binary pixels config
	if (config.length() > 0){
		control = mpxCtrlLoadPixelsCfg(devId, config.c_str(), true);
		cout << "Load pixels config : " << config << " | ret : " << control << endl;
		if(control != 0) {
			cout << "[ERROR] Configuration can't be loaded." << endl;
			exit(1);
		}
	} else {
		cout << "No BPC file provided." << endl;
	}

	// check mode
	int mode;
	int hwtmr_mode;
	control = mpxCtrlGetHwTimer(devId, &hwtmr_mode); //auto
	if(control != 0) {
		cout << "[ERROR] HW can't be set to auto." << endl;
		exit(1);
	}
	cout << "HW timer mode = " << hwtmr_mode << endl;

	//control = mpxCtrlSetAcqMode(devId, 1); //common acq.
	//if(control != 0) {
	//	cout << "[ERROR] Acq mode can't be set." << endl;
	//	exit(1);
	//}
	control = mpxCtrlGetAcqMode(devId, &mode);
	cout << "Acq mode = " << mode << endl;



	// Settings //////////////////////////////////////////////////////////
	// Get and Set Daqs
	int chipnumber = 0;
	DACTYPE dacVals[ndacs];
	double dacValsAnalogue[ndacs];

	// Get DACs
	control = mpxCtrlGetDACs(devId, dacVals, ndacs, chipnumber);
	usleep(10000);

	// Get DACs analogue
	vector<double> dacValsAnalogueSampling;
	for ( int vii = 0; vii < __sensing_nreads ; vii++ ) {
		control = mpxCtrlGetDACsAnalog(devId, dacValsAnalogue, ndacs, chipnumber);
		for(int i = 0 ; i < ndacs ; i++) {
			if( vii == 0 ) {
				dacValsAnalogueSampling.push_back( dacValsAnalogue[i] );
			} else {
				dacValsAnalogueSampling[i] += dacValsAnalogue[i];
			}
			//cout << dacValsAnalogue[i] << " ";
		}
		//cout << endl;
		usleep(5000);
	}
	// average
	for(int i = 0 ; i < ndacs ; i++) dacValsAnalogueSampling[i] /= __sensing_nreads;

	// Dump
	DumpDACs(dacVals, dacValsAnalogueSampling);


	//dacVals[TPX_IKRUM] = ikrum_setting;
	//control = mpxCtrlSetDACs(devId, dacVals, 14, chipnumber);
	//control = mpxCtrlGetDACs(devId, dacVals_new, 14, chipnumber);
	//cout << "IKRUM set and verification = " << dacVals_new[TPX_IKRUM] << endl;

	// Settings //////////////////////////////////////////////////////////
	// Set bias voltage
	double voltage = 15.0; // start up voltage
	byte * data = new byte(sizeof(double));//(byte)voltage;
	data = (byte * ) (&voltage);
	//REM
	control = mpxCtrlSetHwInfoItem(devId, vset_index, data, sizeof(double));	 // 8 voltage
	cout << "Set voltage OK " << " | ret : " << control << endl;
	// After a voltage set one must wait for the circuit to stabilize
	sleep(1);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Dump all hardware items
	//DumpHardwareItems(devId); //--> doing it in a function doesn't work //No it doens't! --Oli


	//cout << "size of double:"<< sizeof(double) <<endl;
	
	//int datax;

	int * datasize = new(nothrow) int();
	if (datasize == NULL) {
			cout << "could not allocate memory for datasize" << endl;
	}
	
	// All information about the device
	int * hardw_info_count = new int();

	control = mpxCtrlGetHwInfoCount(devId, hardw_info_count);
	int hw_counts = *hardw_info_count;

	cout << "-- HwInfo ------------------------------ " << endl;
	cout << "Hardware info items = " << hw_counts << " | (type, datasize, count)"<< endl;
 
	// dummy reading
	for ( int index = 0 ; index < hw_counts ; index++ ) {
		HwInfoItem  infoIdummy;
		double tempdoubledummy;
		infoIdummy.data = &tempdoubledummy;
		control = mpxCtrlGetHwInfoItem ( devId, index, &infoIdummy, datasize );
		usleep(10000);
	}

	for ( int index = 0 ; index < hw_counts ; index++ ) {
		
		double tempdouble;

		HwInfoItem  * infoI = new HwInfoItem();
		// at least one reference must be stablished here
		//double * tempdouble = new double;
		infoI->data = &tempdouble;

		control = mpxCtrlGetHwInfoItem ( devId, index, infoI, datasize );
		// count = array size (number of values).  Each entry with size (*datasize)
		cout << "   " << index << ") "
				<< infoI->name << " (" << nameofType[infoI->type] << "," << *datasize << "," << infoI->count << ") | ";




		if ( infoI->type == TYPE_BOOL ) {
			bool tempbool;
			// Establish the reference
			infoI->data = &tempbool;
			// Read again
			control = mpxCtrlGetHwInfoItem ( devId, index, infoI, datasize );
			// cout
			if(tempbool) cout << "TRUE";
			else cout << "FALSE";
		}


		if ( infoI->type == TYPE_CHAR ) {

			signed char * temp = new signed char[infoI->count * (*datasize)];
			// Establish the reference
			infoI->data = temp;
			// Read again
			control = mpxCtrlGetHwInfoItem ( devId, index, infoI, datasize );
			// cout
			for(u32 i = 0 ; i < infoI->count * (*datasize) ; i++) {
				if( temp[i] == '\0' || temp[i] == 0xa || temp[i] == 0xd ) break;
				cout << temp[i];
			}

			delete [] temp;
		}

		if ( infoI->type == TYPE_UCHAR || infoI->type == TYPE_BYTE ) {
			// to be treated as 8 bits integer WARNING !
			unsigned char * temp = new unsigned char[infoI->count * (*datasize)];
			// Establish the reference
			infoI->data = temp;
			// Read again
			control = mpxCtrlGetHwInfoItem ( devId, index, infoI, datasize );
			// cout
			for(u32 i = 0 ; i < infoI->count ; i++) {
				cout << temp[i];  // FIXME ! review this case
			}

			delete [] temp;
		}

		if ( infoI->type == TYPE_DOUBLE ) {

			double * tempd = new double[infoI->count];
			// Establish the reference
			infoI->data = tempd;

			// Special cases which involve sensing.  Get an average.
			if ( index == vread_index ) { //was 9 REM

				cout << "[sampling " << __sensing_nreads << " times] ";
				double voltage_mean_read = 0.;
				int voltage_nreads = __sensing_nreads;
				for ( int vii = 0; vii < __sensing_nreads ; vii++ ) {
					mpxCtrlGetHwInfoItem(devId, index, infoI, datasize);
					//cout << "read voltage = " << tempd[0] << endl;
					voltage_mean_read += tempd[0];
					usleep(50000);
				}
				voltage_mean_read /= voltage_nreads;
				cout << voltage_mean_read;

			} else if (  index == adcsense_index ) { //was 10 REM
				vector<double> adcsenseV;

				cout << "[sampling " << __sensing_nreads << " times] ";
				for ( int vii = 0; vii < __sensing_nreads ; vii++ ) {
					mpxCtrlGetHwInfoItem(devId, index, infoI, datasize);
					// put the values in a vector
					for (u32 i = 0 ; i < infoI->count ; i++) {
						if ( vii == 0 ) { // the first sampling
							adcsenseV.push_back( tempd[i] );
						} else {
							adcsenseV[i] += tempd[i];
						}
						//cout << tempd[i] << " " ;
					}
					//cout << endl;
					usleep(50000);
				}
				// Get mean and print
				for (u32 i = 0 ; i < infoI->count ; i++) {
					adcsenseV[i] /= double(__sensing_nreads);
					cout << adcsenseV[i];
					if(i != infoI->count - 1) cout << ", ";
				}

			} else {

				// Read again
				control = mpxCtrlGetHwInfoItem ( devId, index, infoI, datasize );
				// cout
				for (u32 i = 0 ; i < infoI->count ; i++) {
					cout << tempd[i];
					if(i != infoI->count - 1) cout << ", ";
				}
			}
		}

		if ( infoI->type == TYPE_U16 ) {
			unsigned short * tempuns = new unsigned short;
			// Establish the reference
			infoI->data = tempuns;
			// Read again
			control = mpxCtrlGetHwInfoItem ( devId, index, infoI, datasize );
			// cout
			cout << *tempuns;
		}

		cout << endl;

		delete infoI;
		//delete [] datasize;
		usleep(100000);
		//usleep(1000);
	}
	
	// Set bias voltage
	string filename = "";
	char temp_ikrum[256];
	char temp_thl[256];
	char temp_setvoltage[256];
	char temp_readvoltage[256];

	ItemInfo data1;
	int datar_size = sizeof(double); //8;
	double voltage_back;
	//DACTYPE dacVals_new[__ndacs_timepix];
	DACTYPE dacVals_new[ndacs];
	int all_loopsCounter = 0;

	for ( int V_itr = minV ; V_itr <= maxV ; V_itr += stepV ) {

		voltage = (double)V_itr;

		// write voltage
		byte * data2 = new byte(sizeof(double));
		data2 = (byte * )(&voltage);
		//REM
		control = mpxCtrlSetHwInfoItem(devId, vset_index, data2, sizeof(double));	 // 8 voltage
		if ( control == 0 ) cout << "Set voltage OK = " << voltage << "V" << endl;

		sleep(3);

		// read voltage back a few times
		double voltage_mean_read = 0.;
		int voltage_nreads = __sensing_nreads;
		cout << "Voltage read [sampling " << __sensing_nreads << " times] | ";
		for ( int vii = 0; vii <= __sensing_nreads ; vii++ ) {
			data1.data = &voltage_back;
			mpxCtrlGetHwInfoItem(devId, vread_index, &data1, &datar_size); //was 9 for voltage ref
			//first reading seems worng, so we skip it			
			if (vii > 0) {
				cout << "read voltage = " << voltage_back << " | setup voltage = " << voltage << endl;
				voltage_mean_read += voltage_back;
			}			
			usleep(500000); //REM
		}
		voltage_mean_read /= voltage_nreads;
		cout << "mean = " << voltage_mean_read << endl;


		for ( int THL_itr = thl_min; THL_itr <= thl_max ; THL_itr += thl_step ) {

			thl_setting = THL_itr;

			/////////////////////////////////////////////////////////////////////////////////////
			// TPX_THLFINE
			control = mpxCtrlGetDACs(devId, dacVals, ndacs, chipnumber);
			//cout << "THLFINE read = " << dacVals[TPX_THLFINE] << endl;

			//REM dacVals[TPX_THLFINE] = thl_setting;
			dacVals[MXR_THLFINE] = thl_setting;
			control = mpxCtrlSetDACs(devId, dacVals, ndacs, chipnumber);

			control = mpxCtrlGetDACs(devId, dacVals_new, ndacs, chipnumber);
			//cout << "THLFINE set and verification = " << dacVals_new[TPX_THLFINE] << endl;
			/////////////////////////////////////////////////////////////////////////////////////

			for ( int THLcoarse_itr = thlcoarse_min; THLcoarse_itr <= thlcoarse_max ; THLcoarse_itr += thlcoarse_step ) {

				thlcoarse_setting = THLcoarse_itr;

				/////////////////////////////////////////////////////////////////////////////////////
				// TPX_THLCoarse
				control = mpxCtrlGetDACs(devId, dacVals, ndacs, chipnumber);
				//cout << "THLFINE read = " << dacVals[TPX_THLFINE] << endl;
				dacVals[MXR_THLCOARSE] = thlcoarse_setting;
				control = mpxCtrlSetDACs(devId, dacVals, ndacs, chipnumber);
				control = mpxCtrlGetDACs(devId, dacVals_new, ndacs, chipnumber);
				//cout << "THLFINE set and verification = " << dacVals_new[TPX_THLFINE] << endl;
				/////////////////////////////////////////////////////////////////////////////////////

				for ( int IKRUM_itr = ikrum_min ; IKRUM_itr <= ikrum_max ; IKRUM_itr += ikrum_step ) {

					ikrum_setting = IKRUM_itr;

					/////////////////////////////////////////////////////////////////////////////////////
					// IKRUM
					//REM changed "TPX" -> "MXR"
					//but works still with TPX because IKRUM, and THL* are mapped to the same index numbers
					control = mpxCtrlGetDACs(devId, dacVals, ndacs, chipnumber);
					//cout << "IKRUM read = " << dacVals[TPX_IKRUM] << endl;

					dacVals[MXR_IKRUM] = ikrum_setting;
					control = mpxCtrlSetDACs(devId, dacVals, ndacs, chipnumber);

					control = mpxCtrlGetDACs(devId, dacVals_new, ndacs, chipnumber);
					//cout << "IKRUM set and verification = " << dacVals_new[TPX_IKRUM] << endl;
					/////////////////////////////////////////////////////////////////////////////////////

					/////////////////////////////////////////////////////////////////////////////////////
					// Setting report
					cout << "-- " << all_loopsCounter << " -------------------------------------------------" << endl;
					cout << "THLFINE set and verification = " << dacVals_new[MXR_THLFINE] << endl;
					cout << "THLCOARSE set and verification = " << dacVals_new[MXR_THLCOARSE] << endl;
					cout << "IKRUM set and verification = " << dacVals_new[MXR_IKRUM] << endl;


					// build a name
					sprintf(temp_setvoltage,"%.1f", voltage);
					sprintf(temp_readvoltage,"%.1f", voltage_mean_read);
					sprintf(temp_ikrum,"%d",ikrum_setting);
					sprintf(temp_thl,"%d",thl_setting);

					// perform acquisition
					
					if(acqcounts_setting == 0) {
						endless_acq = 1;
						//filename = "./";
						filename = dataset;
						//filename += "/frame_";
						cout << "Start acquisition.  Write to --> " << filename << endl;
						while(endless_acq){
							if (toggle_acq) {
								cout << "Acquisition Nr." << ++acqcount << endl; 
								control = mpxCtrlPerformFrameAcq(devId, 1, acqTime_setting, FSAVE_ASCII |  FSAVE_SPARSEXY, filename.c_str());
								if ( control != 0 ) {
									cout << "[WARNING] Problems at : " << filename << " | error:" << control << endl;
									//return 0;
								}
							} else {
								//insert sleep to prevent spinning
								sleep(1);
							}
						} 					
					} else {
						filename = "./";
						filename += dataset;
						filename += "_IKRUM";
						filename += temp_ikrum;
						filename += "__set";
						filename += temp_setvoltage; filename += "V";
						filename += "__read";
						filename += temp_readvoltage; filename += "V";
						filename += "_THL_";
						filename += temp_thl;
						filename += "/frame_";
						
						cout << "Start acquisition.  Write to --> " << filename << endl;
						control = mpxCtrlPerformFrameAcq(devId, acqcounts_setting, acqTime_setting, FSAVE_SPARSEXY | FSAVE_ASCII | FSAVE_U32 , filename.c_str());
						//control = mpxCtrlPerformFrameAcq(devId, acqcounts_setting, acqTime_setting, FSAVE_ASCII | FSAVE_U32 , filename.c_str());
						if ( control != 0 ) {
							cout << "[WARNING] Problems at : " << filename << " | error:" << control << endl;
							//return 0;
						}
					}
						
					all_loopsCounter++;
				}

			}

		}

	}

	cout << "[DONE] Run finished !" << endl;

	return 0;
}


AllParameters * LoadParameterList() {

	AllParameters * pars = new AllParameters();
	pars->PushParameter("V"			,   int(100) 	);
	pars->PushParameter("THL"		,   int(300) 	);
	pars->PushParameter("THLCoarse"	,   int(7) 	);
	pars->PushParameter("IKRUM"		, 	int(3)   	);
	pars->PushParameter("AcqCounts"	,	int(10)		);
	pars->PushParameter("AcqTime"	,	double(0.1) );
	pars->PushParameter("BPConfig"	,	string("bpc.bpc")		);
	pars->PushParameter("Dataset"	, 	string("testdataset")	);

	return pars;
}


void DumpDACs(DACTYPE * dacs, vector<double> dacsAnalogue) {
	cout << "-- DACs -------------------------------- " << endl;
	for ( int i = 0 ; i < ndacs ; i++ ) {
		//cout << "  " << nameofTPXDACs[i] << " = " << dacs[i] << "\t| " << dacsAnalogue[i] << endl; 
		cout << "  " << nameofMXRDACs[i] << " = " << dacs[i] << "\t| " << dacsAnalogue[i] << endl;
	}
	cout << "---------------------------------------- " << endl;

}	///////////////////////////////////////////////////////////////////////////////////////////////////
