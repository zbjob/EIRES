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

using namespace std;

int main(int _argc, char* _argv[])
{
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
    string inputfile = "../../../data/google_cluster/PMPM7-1.log";

    const int av1Idx = 2;
    const int bv2Idx = 5;
    const int cv1Idx = 7;
    const int cv2Idx = 8;
    const int atsIdx = 1;
    const int btsIdx = 4;
    const int ctsIdx = 6;
    const int timewindow = 20000;
    const int numTimeSlice = 4;

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
    map<attr_t,attr_t> PM_state_1_contribution;
    map<attr_t,attr_t> PM_state_1_consumption;

    map<tuple<attr_t,attr_t>,attr_t> PM_state_2_contribution;
    map<tuple<attr_t,attr_t>,attr_t> PM_state_2_consumption;

    map<attr_t,attr_t> event_a_contribution;
    map<attr_t,attr_t> event_a_consumption;

    
    map<attr_t,attr_t> event_b_contribution;
    map<attr_t,attr_t> event_b_consumption;
    

    map<attr_t,attr_t> event_c_contribution;
    map<attr_t,attr_t> event_c_contribution_cv2;

    string line;

    attr_t av1 = 0;
    attr_t bv2 = 0;
    attr_t cv1 = 0;
    attr_t cv2 = 0;

    while(getline(ifs,line))
    {
        vector<string> match;
        stringstream lineStream(line);
        string cell;

        while(getline(lineStream,cell,','))
            match.push_back(cell);

        if(match[0] == "0")
        {
            av1 = stoi(match[av1Idx+1]); 
            bv2 = stoi(match[bv2Idx]); 
            cv1 = stoi(match[cv1Idx]); 
            cv2 = stoi(match[cv2Idx]); 

            ++event_a_contribution[av1];
            ++event_a_consumption[av1];

            ++PM_state_1_contribution[av1];
            ++PM_state_2_consumption[make_pair(av1,bv2)];
            
            ++event_b_contribution[bv2];
            ++event_b_consumption[bv2];

            ++PM_state_2_contribution[make_pair(av1,bv2)];
            ++PM_state_2_consumption[make_pair(av1,bv2)];

            ++event_c_contribution[cv1];
            ++event_c_contribution_cv2[cv2];
        }
        else if(match[0] == "1")
        {
            av1 = stoi(match[av1Idx]);

            ++event_a_consumption[av1];
            ++PM_state_1_consumption[av1];
        }
        else if(match[0] == "2")
        {
            av1 = stoi(match[av1Idx]);
            bv2 = stoi(match[bv2Idx]);

            ++event_a_consumption[av1];
            ++event_b_consumption[bv2];

            ++PM_state_1_consumption[av1];
            ++PM_state_2_consumption[make_pair(av1,bv2)];
        }
        else
            ;
    }





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
    ofsPM2 << "av1, bv2, consumptions, contributions" << endl;

    std::ofstream ofsFM;
    ofsFM.open(outfile_3.c_str());

    cout << "writing to " << outputfile << endl;

    ofs << "consumption and contribution for event A on v1" << endl;
    for(auto const& iter : event_a_consumption)
        ofs << iter.first << " , " << iter.second << " , " << event_a_contribution[iter.first] << endl;

    ofs << "===============================================" << endl
        << "consumption and contribution for event B on v2" << endl;
    for(auto const& iter : event_b_consumption)
        ofs << iter.first << " , " << iter.second << " , " << event_b_contribution[iter.first] << endl;

    ofs << "===============================================" << endl
        << "contribution for event C on v1" << endl;
    for(auto const& iter : event_c_contribution)
        ofs << iter.first << " , " << iter.second << endl;

    ofs << "===============================================" << endl
        << "contribution for event C on v2" << endl;
    for(auto const& iter : event_c_contribution_cv2)
        ofs << iter.first << " , " << iter.second << endl;


    ofs << "===============================================" << endl
        << "consumption and contribution for partial match 1" << endl;
    for(auto const& iter : PM_state_1_consumption)
    {
        ofs << iter.first << " , " << iter.second << " , " << PM_state_1_contribution[iter.first] << endl;
        ofsPM1 << iter.first << " , " << iter.second << " , " << PM_state_1_contribution[iter.first] << endl;
    }
    


    ofs << "===============================================" << endl
        << "consumption and contribution for partial match 2" << endl;
    for(auto const& iter : PM_state_2_consumption)
    {
        ofs << get<0>(iter.first) << ","  << get<1>(iter.first) << " , " << iter.second << " , " << PM_state_2_contribution[iter.first] << endl;
        ofsPM2 << get<0>(iter.first) << ","  << get<1>(iter.first) << " , " << iter.second << " , " << PM_state_2_contribution[iter.first] << endl;
    }



    ofs.close();


	return 0;
}


