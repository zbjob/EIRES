#include <iostream> 
// #include <boost/geometry.hpp>
// #include <boost/geometry/geometries/point_xy.hpp>
// #include <boost/geometry/geometries/polygon.hpp>

// namespace bg = boost::geometry;
// typedef bg::model::polygon<bg::model::d2::point_xy<double> > poly_t;
// #include <boost/type_index.hpp>
#include <typeinfo>


using namespace std;

// struct attr_e {
// 	enum {UINT64_T, DOUBLE, POLYGON} tag;
// 	union
// 	{
// 		uint64_t i;
// 		double d;
// 	};
// 	poly_t poly;

//     attr_e(){
//         this->i = (uint64_t)1;
//     };

// 	attr_e(uint64_t i_){
// 		this->tag = attr_e::UINT64_T;
// 		this->i = i_;
// 	};
// 	attr_e(double d_){
// 		this->tag = attr_e::DOUBLE;
// 		this->d = d_;
// 	};
// 	attr_e(string s_){
// 		this->tag = attr_e::POLYGON;
// 		bg::read_wkt(s_, this->poly);
// 	};

// 	auto getValue() {
// 		struct result {
//             operator uint64_t(){
//                 return e->i;
//             }
//             operator double(){
//                 return e->d;
//             }
//             operator poly_t(){
//                 return e->poly;
//             }
//             attr_e* e;
//         };
//         return result {this};
// 	}
// };

struct Member {
    union {
        int i;
        double d;
    };
    // int (*getValue) (void*);
    Member(int _i){
        i = _i;
        // getValue = &getI();
    }
    Member(double _d){
        d = _d;
        // getValue = &getD():
    }
    enum {INT, DOUBLE} tag;
    int getI() const {
        return i;
    };
    double getD() const {
        return d;
    }

    void setI(){
        this->i = 10;
    } 
    auto get() {
        struct result {
            operator int(){
                return e->i;
            }
            operator double(){
                return e->d;
            }
            // operator poly_t(){
            //     return e->poly;
            // }
            Member* e;
        };
        return result {this};
    }
    template <typename T>
    T getVl() const {
        // cout << boost::typeindex::type_id<T>().pretty_name() << endl;
        // cout << "type info: " << *typeid(T).name() << endl;
        //    cout << "call getVL" << endl;

       if(typeid(T).name() == typeid(uint64_t).name()){
           cout << "haha" << endl;
           return this->i;
       } else {
           cout << "get d" <<endl;
           cout << this->d << endl;
           return this->d;
       }
    }
    
    template <typename T>
    T plus(T a){
        this->i += a;
        return this->i;
    }
};


// void print_e(const attr_e& s)
// {
//     switch(s.tag)
//     {
//         case attr_e::UINT64_T: 
//             std::cout << "int "<<  s.i << '\n'; break;
//         case attr_e::DOUBLE: 
//             std::cout << "float " << s.d << '\n'; break;
//     }
// }

auto foo(){
    int a = 10;
    return a;
    // float b = 1.5f;
    // return b;
}

// void print_Arr(attr_e * events){
//     for(int i=0;i<5;i++){
//         cout << "element " << i << " value: " << (uint64_t) events[i].getValue() <<endl;
//     }
// }

struct Event {
    union {
        int i;
        double d;
        char c;
    };
    enum {INT, DOUBLE, CHAR} tag;
    Event(int i_){
        this->i = i_;
    }
    Event(double d_){
        this->d = d_;
    }
    Event(char c_){
        this->c = c_;
    }
};


int main()
{
    // attr_e e = attr_e("POLYGON((0 0,0 7,4 2,2 0,0 0))");
    // attr_e e = attr_e("POLYGON((0 0,0 7,4 2,2 0,0 0))");
    // uint64_t v = e.getValue();
    // double d = e.getValue();
    // poly_t p = e.getValue();
    // double area = bg::area(p);
    // cout << "area : " << area << endl;

    // attr_e arr[5];
    // arr[0] =  attr_e((uint64_t)0);
    // arr[1] =  attr_e((uint64_t)1);
    // arr[2] =  attr_e((uint64_t)2);
    // arr[3] =  attr_e((uint64_t)3);
    // arr[4] =  attr_e((uint64_t)4);
    // print_Arr(arr);

//    const Member event = Member(1);
//     int i_ = event.getI();
//     cout << "i: " << i_ << endl;
//     double d = event.getD();
//     cout << "d: " << d << endl;
//     int db = event.getVl<int>();
//    cout << "auto: "  << db;

    Event event = {1};
    cout << "event int: " << event.i << endl;
    cout << "event double: " << event.d << endl;
    cout << "event char: " << event.c << endl;


}