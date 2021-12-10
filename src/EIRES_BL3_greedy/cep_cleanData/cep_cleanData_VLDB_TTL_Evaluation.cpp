//#include "../Query.h"
//#include "../EventStream.h"

//#include "../freegetopt/getopt.h"
#include <utility> 
#include <vector>
#include <algorithm>
#include <assert.h>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>


typedef long int attr_t;
//typedef long double attr_t;

using namespace std;

int main(int _argc, char* _argv[])
{

    //cout << _argv[1] << endl;
    //int numTimeSlice = stoi(string(_argv[1]));
    
    /*
	const char* deffile = "default.eql";


	int c;
	while ((c = getopt(_argc, _argv, "c:")) != -1)
	{
		switch (c)
		{
		case 'c':
			deffile = optarg;
			break;
		default:
			abort();
		}
	}



	QueryLoader def;
	if (!def.loadFile(deffile))
	{
		fprintf(stderr, "failed to load definition file %s\n", deffile);
		return 1;
	}
    */

    const int CONST_NUM_STATES = 2;

    string outfile_1 = "PM_state_1.csv";
    string outfile_2 = "PM_state_2.csv";
    string outfile_3 = "FM.csv";
    string outputfile = "CleanedOutput";
    string inputfile = "/home/bo/CEP_load_shedding/data/google_cluster/results_4_VLDB/PM_P26.dat";

    const int atsIdx = 1;
    const int av1Idx = 2;
    const int aIDIdx = 3;
    const int btsIdx = 4;
    const int bv1Idx = 5;
    const int ctsIdx = 6;
    const int cv1Idx = 7;

    const int timewindow = 9000;
    const int numTimeSlice = 1;
    const int TTLSpan = timewindow/numTimeSlice;
    cout << TTLSpan << endl;

    ifstream ifs;
    ifs.open(inputfile.c_str());
    if( !ifs.is_open())
    {
        cout << "can't open file " << inputfile << endl;
        return false;
    }


    //cout << "output file 1" << outfile_1 << endl;

    //cout << "output file 2" << outfile_2 << endl;



    /*
	vector<string> col;
	for (size_t i = 0; i < def.numEventDecls(); ++i)
	{
		for (const string& attr : def.eventDecl(i)->attributes)
		{
			if (find(col.begin(), col.end(), attr) == col.end())
				col.push_back(attr);
		}
	}

	sort(col.begin(), col.end());
	const char* definedCol[] = { "_type", "_flags", "_timestamp", "_timeoutState" };
	col.insert(col.begin(), definedCol, definedCol + sizeof(definedCol) / sizeof(definedCol[0]));
    */

    // to clean data and merge duplicate attribute values in the output full matches
    vector<map<attr_t,attr_t> > PM_state_1_contribution(numTimeSlice);
    vector<map<attr_t,attr_t> > PM_state_1_consumption(numTimeSlice);

    vector<map<attr_t,attr_t> > PM_state_2_contribution(numTimeSlice);
    vector<map<attr_t,attr_t> > PM_state_2_consumption(numTimeSlice);

    vector<map<attr_t,attr_t> > event_a_contribution(numTimeSlice);
    vector<map<attr_t,attr_t> > event_a_consumption(numTimeSlice);

    
    vector<map<attr_t,attr_t> > event_b_contribution(numTimeSlice);
    vector<map<attr_t,attr_t> > event_b_consumption(numTimeSlice);
    

    vector<map<attr_t,attr_t> > event_c_contribution(numTimeSlice);

    string line;

    attr_t av1 = 0;
    attr_t bv1 = 0;
    attr_t cv1 = 0;
    attr_t ats = 0;
    attr_t bts = 0;
    attr_t cts = 0;

    bool parseLineFlag = false;

    while(getline(ifs,line))
    {
        if(line == "[beg]")
        {
            parseLineFlag = true;
            continue;
        }
        if(line == "[end]")
            break;
        if(!parseLineFlag)
            continue;
        vector<string> match;
        stringstream lineStream(line);
        string cell;

        while(getline(lineStream,cell,','))
            match.push_back(cell);

        if(match[0] == "0")
        {
            av1 = stoi(match[av1Idx]); 
            bv1 = stoi(match[bv1Idx]); 
            cv1 = stoi(match[cv1Idx]); 

            ats = stoi(match[atsIdx]); 
            bts = stoi(match[btsIdx]); 
            cts = stoi(match[ctsIdx]); 

            //int PM2_time_slice = (bts-ats)/TTLSpan; 
            int PM2_time_slice = 0; 

            ++event_a_contribution[0][av1];
            ++event_a_consumption[0][av1];

            ++PM_state_1_contribution[0][av1];
            ++PM_state_1_consumption[0][av1];
            
            ++event_b_contribution[PM2_time_slice][bv1];
            ++event_b_consumption[PM2_time_slice][bv1];

            ++PM_state_2_contribution[PM2_time_slice][av1+bv1];
            ++PM_state_2_consumption[PM2_time_slice][av1+bv1];

            ++event_c_contribution[0][cv1];
        }
        else if(match[0] == "1")
        {
            av1 = stoi(match[av1Idx]);

            ++event_a_consumption[0][av1];
            ++PM_state_1_consumption[0][av1];
        }
        else if(match[0] == "2")
        {
            av1 = stoi(match[av1Idx]);
            bv1 = stoi(match[bv1Idx]);

            ats = stoi(match[atsIdx]); 
            bts = stoi(match[btsIdx]); 

            //int PM2_time_slice = (bts-ats)/TTLSpan; 
            int PM2_time_slice = 0; 

            ++event_a_consumption[0][av1];
            ++event_b_consumption[PM2_time_slice][bv1];

            ++PM_state_1_consumption[0][av1];
            ++PM_state_2_consumption[PM2_time_slice][av1+bv1];
        }
        else
            ;
    }




    cout << "parse finished" << endl;

    /*
	StreamEvent::setupStdIo(stdin);

	//for (const string& title : col)
	//	printf("%s%s", title.c_str(), title == col.back() ? "" : ",");
	//putchar('\n');

	StreamEvent event;
	while (event.read())
	{
		const EventDecl* decl = def.eventDecl(event.typeIndex);

	//	printf("%s,%i,", decl->name.c_str(), (int)event.flags);

	//	if (event.flags & StreamEvent::F_TIMESTAMP)
	//		printf("%llu,", event.attributes[event.attributeCount - 1]);
	//	else
	//		putchar(',');

	//	if (event.flags & StreamEvent::F_TIMEOUT)
	//		printf("%i,", (int)event.timeoutState);
	//	else
	//		putchar(',');

		for (size_t i = 1; i < col.size(); ++i)
		{
			vector<string>::const_iterator it = find(decl->attributes.begin(), decl->attributes.end(), col[i]);
			if (it != decl->attributes.end())
			{
				//printf("%llu", event.attributes[it - decl->attributes.begin()]);
                if(i==4)
                {
                    ++attr_1_contributions[(attr_t) event.attributes[it - decl->attributes.begin()]];
                }
                else if(i==5)
                {
                    ++attr_2_contributions[(attr_t) event.attributes[it - decl->attributes.begin()]];
                }
			}
			
		//	putchar(i + 1 != col.size() ? ',' : '\n');
		}
	}
    */

    std::ofstream ofs;
    ofs.open(outputfile.c_str());
    
    std::ofstream ofsPM1;
    ofsPM1.open(outfile_1.c_str());
    ofsPM1 << "av1, consumptions, contributions" << endl;



    std::ofstream ofsPM2;
    ofsPM2.open(outfile_2.c_str());
    ofsPM2 << "av1, bv1, consumptions, contributions" << endl;

    std::ofstream ofsFM;
    ofsFM.open(outfile_3.c_str());

    cout << "writing to " << outputfile << endl;

    ofs << "consumption and contribution for event A on v1" << endl;

    int i =1;
    for(auto const& it : event_a_consumption)
    {
        ofs << "time slice " << i << endl;
        for(auto const& iter : it)
            ofs << iter.first << " , " << iter.second << " , " << event_a_contribution[i-1][iter.first] << endl;
        ofs << "===============================================" << endl;
        ++i;
    }
    i = 1;

    ofs << "===============================================" << endl
        << "consumption and contribution for event B on v1" << endl;
    for(auto const& it : event_b_consumption)
    {        
        ofs << "time slice " << i << endl;
        for(auto const& iter : it)
            ofs << iter.first << " , " << iter.second << " , " << event_b_contribution[i-1][iter.first] << endl;
        ofs << "===============================================" << endl;

    }
    i =1;


    ofs << "===============================================" << endl
        << "contribution for event C on v1" << endl;
    for(auto const& it : event_c_contribution)
    {
        ofs << "time slice " << i << endl;
        for(auto const& iter : it)
            ofs << iter.first << " , " << iter.second << " , " << event_c_contribution[i-1][iter.first] << endl;
        ofs << "===============================================" << endl;
        ++i;
    }
    i=1;


    ofs << "===============================================" << endl
        << "consumption and contribution for partial match 1" << endl;
    for(auto const& it : PM_state_1_consumption)
    {
        ofs << "time slice " << i << endl;
        for(auto const& iter : it)
            ofs << iter.first << " , " << iter.second << " , " << PM_state_1_contribution[i-1][iter.first] << endl;
        ofs << "===============================================" << endl;
        ++i;

        //ofs << iter.first << " , " << iter.second << " , " << PM_state_1_contribution[iter.first] << endl;
        //ofsPM1 << iter.first << " , " << iter.second << " , " << PM_state_1_contribution[iter.first] << endl;
    }
    i=1;
    


    ofs << "===============================================" << endl
        << "consumption and contribution for partial match 2" << endl;
    for(auto const& it : PM_state_2_consumption)
    {
        ofs << "time slice " << i << endl;
        for(auto const& iter : it)
            if(iter.second !=0)
                ofs << iter.first << " , " << iter.second << " , " << PM_state_2_contribution[i-1][iter.first] << " , " <<  (long double) (PM_state_2_contribution[i-1][iter.first])/(long double) (iter.second) << endl;
            else
                ofs << iter.first << " , " << iter.second << " , " << PM_state_2_contribution[i-1][iter.first]  << endl;
        ofs << "===============================================" << endl;
        ++i;
            
        //ofs << iter.first <<  " , " << iter.second << " , " << PM_state_2_contribution[iter.first] << endl;
        //ofsPM2 << iter.first << " , " << iter.second << " , " << PM_state_2_contribution[iter.first] << endl;
    }
    ofs.close();




    string _file = string("PM_state_2_2ms_")+to_string(numTimeSlice);
    ofs.open(_file.c_str());
    attr_t Contribution = 0;
    attr_t Consumption = 0;

    vector<attr_t> PM_2_contribution_time_slice;
    vector<attr_t> PM_2_consumption_time_slice;

    for(auto & ts: PM_state_2_contribution)
    {
        attr_t CtTS = 0;
        for(auto &iter: ts)
        {
            Contribution += iter.second;
            CtTS += iter.second;
        }
        PM_2_contribution_time_slice.push_back(CtTS);
    }

    for(auto & ts: PM_state_2_consumption)
    {
        attr_t CtTS = 0;
        for(auto &iter: ts)
        {
            Consumption += iter.second;
            if(iter.first <= 10)
            CtTS += iter.second;
        }
        PM_2_consumption_time_slice.push_back(CtTS);
    }

    ofs << "ContributionTS" << endl;
    i=1;
    for(auto & it: PM_2_contribution_time_slice)
        ofs << i++ << "," << (long double)(it) / (long double)(Contribution) << endl;
    
    i=1; 
    ofs << "ConsumptionTS" << endl;
    for(auto & it: PM_2_consumption_time_slice)
        ofs << i++ << "," << (long double)it / (long double) Consumption << endl;
        
    ofs << "RationTS" << endl;
    i=1;
    for(auto const& it : PM_state_2_consumption)
    {
        for(auto const& iter : it)
            if(iter.second !=0)
                ofs << i << "," << iter.first << "," << (long double) iter.second/ (long double) Consumption << "," << (long double) PM_state_2_contribution[i-1][iter.first]/(long double) Contribution << "," <<  (long double) (PM_state_2_contribution[i-1][iter.first])/(long double) (iter.second) << endl;
        ++i;
    }
    ofs.close();


	return 0;
}


