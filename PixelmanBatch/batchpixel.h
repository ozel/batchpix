#ifndef __batchpixel_h
#define __batchpixel_h

#include <string>
#include <vector>
#include <typeinfo>

using namespace std;

#define __sensing_nreads 10
#define __ndacs_timepix  TPX_REFLVDS+1
#define __ndacs_medipixMXR  MXR_REFLVDS+1

///////////////////////////////////////////////////////////////
// BatchParCommon
class BatchParCommon {

public:
	BatchParCommon(string parname);
	virtual ~BatchParCommon(){};

	void SetTypeName(string n) { m_typeName = n; };
	//void SetTypeHash(size_t h) { m_typeHash = h; };

	string GetParName() { return m_name; };
	string GetTypeName() { return m_typeName; };
	///size_t GetTypeHash() { return m_typeHash; };

	virtual bool IsRange() = 0;
	template<typename T> T GetMin();

private:
	string m_name;		// parameter name
	string m_typeName;  // type name
	//size_t m_typeHash;  // type identifier

};

///////////////////////////////////////////////////////////////
// One Batch parameter
template <class T> class BatchParameter : public BatchParCommon {

public:

	BatchParameter(string parname, T initval);
	virtual ~BatchParameter();

	//type_info MyType() { return typeid(this); };

	bool IsRange() { return m_isRange; };
	T GetMin() { return m_min; };
	T GetMax() { return m_max; };
	T GetStep() { return m_step; };

	void SetMin(T m) { m_min = m; };
	void SetMax(T m) { m_max = m; };
	void SetStep(T m) { m_step = m; };
	void SetIsRange(bool v) { m_isRange = v; };

private:

	bool m_isRange;		// indicates if a range/scan is requested
	T m_min, m_max, m_step;

};

///////////////////////////////////////////////////////////////
// Container for all parameters
class AllParameters {

public:
	AllParameters();
	~AllParameters();

	template<typename T> void PushParameter(string parname, T initval);

	int GetNPars () { return m_npars; };
	BatchParCommon * GetParByName(string);

	void DumpAllParameters();

	bool ParseAndModifyParameter(char * oneline);

private:

	int m_npars;
	vector<BatchParCommon * > m_parameters;
	int m_nparsScan;
	vector<BatchParCommon * > m_parametersRange;
	vector<BatchParCommon * > m_parametersFix;

};

static const char * const nameofMXRDACs[] =
{
    "MXR_IKRUM",
    "MXR_DISC",
    "MXR_PREAMP",
    "MXR_BUFFA",
    "MXR_BUFFB",
    "MXR_DELAYN",
    "MXR_THLFINE",
    "MXR_THLCOARSE",
    "MXR_THHFINE",
    "MXR_THHCOARSE",   
    "MXR_FBK",
    "MXR_GND",
    "MXR_THS",
    "MXR_BIASLVDS",
    "MXR_REFLVDS",
};

static const char * const nameofTPXDACs[] =
{
    "TPX_IKRUM",
    "TPX_DISC",
    "TPX_PREAMP",
    "TPX_BUFFA",
    "TPX_BUFFB",
    "TPX_HIST",
    "TPX_THLFINE",
    "TPX_THLCOARSE",
    "TPX_VCAS",
    "TPX_FBK",
    "TPX_GND",
    "TPX_THS",
    "TPX_BIASLVDS",
    "TPX_REFLVDS"
};


#endif
