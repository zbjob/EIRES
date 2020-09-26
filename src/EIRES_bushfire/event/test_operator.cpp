#include <iostream>
#include <unordered_map>


#include "EventStream.h"

using namespace std;


struct widget {
    int i;
};

struct doodad {
    double f;
};
typedef int64_t acceptCounter_t;


struct exp {
        enum {INT,STRING,FLOAT} tag;
        int get_entity_as_widget(){
            return 1;
        };
        double get_entity_as_doodad(){
            return 1.5;
        }
        auto getExp(){
            struct result {
                operator int(){
                    return exp->get_entity_as_widget();
                }
                operator double(){
                    return exp->get_entity_as_doodad();
                }
                
                exp* exp;
            };
            return result {this};
        };
        // int getExp();
    // double getExp();
};


int main(){
    // struct exp e;
    // int w;
    // w = e.getExp();
    // cout << " w down int : " << (double) e.getExp() << endl;
    // double d = e.getExp();
    // cout << " d " << d << endl;
    attr_e a = attr_e((int64_t)1);
    attr_e b = attr_e("POLYGON((0 0,0 45,45 0,0 0))");
    attr_e c = a + b;

    // attr_e b = attr_e(1.0);
    // bool r = a == b;
    // cout << "compare : " << r << endl;
    cout << "result: " << c.tag << endl;
    return 0;
}