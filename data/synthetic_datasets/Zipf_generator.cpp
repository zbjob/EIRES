#include "NormalDistGen.h"
#include <chrono>
#include <fstream>
#include <string>

using namespace std;
using namespace std::chrono;

void NormalDistGen::run_zipf(string file)
{
    m_GenThread = thread ([=] () { 
        readZipf(file);

        ofstream ofs;
        ofs.open(outputFile.c_str());

        default_random_engine generator;
        uniform_int_distribution<int> distribution(1,4); //control the frequency of each type of events
        uniform_int_distribution<int> IDgen(1,100);  // the range of ID
        //uniform_int_distribution<int> IDgen(1,10);  // the range of ID
        
        uint64_t timeCnt = 1;
        
        for(uint64_t cnt = 1; cnt <= eventCnt; ++cnt)
        {
            int dice_roll = distribution(generator);  // generates number in the range [1,4]

            NormalEvent Event;

            Event.name = "A";
            Event.v1 = attr_t (zipfVec[cnt-1]);
            Event.v2 = 0; 
            Event.ArrivalQTime = cnt; 
            Event.ID = (uint64_t) IDgen(generator);


            if(dice_roll == 1 ) {
                Event.name = "A";
            }
            else if(dice_roll == 2){
                Event.name = "B";
            }
            else if(dice_roll == 3){
                Event.name = "C";
            }
            else
            {
                Event.name = "D";
            }


            ofs << Event.name << "," << Event.ArrivalQTime << "," << Event.ID << "," << uint64_t (Event.v1) << "," <<  Event.v2 <<  endl;
        } //for
        ofs.close();
        m_StopThread = true;


    });
  
}

bool NormalDistGen::readZipf(string file)
{
    ifstream ifs;
    ifs.open(file.c_str());
    if( !ifs.is_open())
    {
        cout << "can't open file " << file << endl;
        return false;
    }

    string line;
    while(getline(ifs,line))
    {
        uint64_t vale = uint64_t(stoll(line));
        zipfVec.push_back(vale);
    }
    return true;
}


bool NormalDistGen::stop()
{
    //if(m_StopThread) 
        m_GenThread.join();

    return m_StopThread;
}

int main()
{
    int eventCnt = 500000;
        string outputFile = "Zipf_StreamLog_prefetch_500K.csv";
        string zipfFile = "zipf_1-01.csv";
        NormalDistGen StreamGenerator(eventCnt, outputFile);
        StreamGenerator.run_zipf(zipfFile);
        StreamGenerator.stop();
    return 0;
}
