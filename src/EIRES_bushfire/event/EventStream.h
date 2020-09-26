#pragma once
// #include "../query/Query.h"
#include "../_shared/MurmurHash3.h"

#include <inttypes.h>
#include <stdio.h>
#include <fcntl.h>
#include <string>

#ifdef _WIN32
#include <io.h>
#endif

#include <iostream>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

namespace bg = boost::geometry;
typedef bg::model::polygon<bg::model::d2::point_xy<double>> poly_t;
using namespace std;
typedef int64_t attr_t;

struct attr_e
{
	enum
	{
		INT64_T,
		DOUBLE,
		POLYGON
	} tag;
	union
	{
		attr_t i;
		double d;
	};
	poly_t poly;

	attr_e()
	{
		this->i = (attr_t)0;
	};

	attr_e(attr_t i_)
	{
		this->tag = attr_e::INT64_T;
		this->i = i_;
	};
	attr_e(double d_)
	{
		this->tag = attr_e::DOUBLE;
		this->d = d_;
	};
	attr_e(std::string s_)
	{
		// cout << s_ << endl;
		this->tag = attr_e::POLYGON;
		bg::read_wkt(s_, this->poly);
	};

	// auto getValue() {
	// 	struct result {
	//         operator INT64_T(){
	//             return e->i;
	//         }
	//         operator double(){
	//             return e->d;
	//         }
	//         operator poly_t(){
	//             return e->poly;
	//         }
	//         attr_e* e;
	//     };
	//     return result {this};
	// }

	// template <typename T>
	// T getValue() const {
	// 	std::cout << "type name: " << typeid(poly_t).name() << std::endl;
	// 	if(typeid(T).name() == typeid(INT64_T).name())
	// 		return this->i;
	// 	else if (typeid(T).name() == typeid(double).name())
	// 		return this->d;
	// 	else //if(typeid(T).name() == typeid(poly_t).name())
	// 		return this->poly;
	// }

	template <typename T>
	T plus(T a)
	{
		if (typeid(T).name() == typeid(INT64_T).name())
		{
			this->i += a;
			return this->i;
		}
		else if (typeid(T).name() == typeid(double).name())
		{
			this->d += a;
			return this->d;
		}
		return NULL;
	};

	void *operator new(size_t size)
	{
		void *p = malloc(size);
		return p;
	};

	attr_e operator+(attr_e &a)
	{

		switch (a.tag)
		{
		case attr_e::INT64_T:
			return attr_e(this->i + a.i);
		case attr_e::DOUBLE:
			return attr_e(this->d + a.d);
		default:
			return a;
		}
	}

	attr_e operator+(const attr_e &a)
	{

		switch (a.tag)
		{
		case attr_e::INT64_T:
			return attr_e(this->i + a.i);
		case attr_e::DOUBLE:
			return attr_e(this->d + a.d);
		default:
			return a;
		}
	}

	attr_e operator/(const attr_e &a)
	{
		switch (this->tag)
		{
		case attr_e::INT64_T:
			return attr_e(this->i / a.i);
		case attr_e::DOUBLE:
			return attr_e(this->d / a.d);
		default:
			return a;
		}
	}

	attr_e operator-(const attr_e &a)
	{
		switch (this->tag)
		{
		case attr_e::INT64_T:
			return attr_e(this->i - a.i);
		case attr_e::DOUBLE:
			return attr_e(this->d - a.d);
		default:
			return a;
		}
	}

	bool operator<(const attr_e &_e) const
	{
		switch (this->tag)
		{
		case attr_e::INT64_T:
			return i < _e.i;
			break;
		case attr_e::DOUBLE:
			return d < _e.d;
			break;
		default:
			return false;
		}
	}

	bool operator>(const attr_e &_e) const
	{
		switch (this->tag)
		{
		case attr_e::INT64_T:
			return i > _e.i;
		case attr_e::DOUBLE:
			return d > _e.d;
		default:
			return false;
			break;
		}
	}

	bool operator>=(const attr_e &_e) const
	{
		switch (this->tag)
		{
		case attr_e::INT64_T:
			return i >= _e.i;
		case attr_e::DOUBLE:
			return d >= _e.d;
		default:
			return false;
			break;
		}
	}

	bool operator<=(const attr_e &_e) const
	{
		switch (this->tag)
		{
		case attr_e::INT64_T:
			return i <= _e.i;
		case attr_e::DOUBLE:
			return d <= _e.d;
		default:
			return false;
			break;
		}
	}

	bool operator==(const attr_e &_e) const
	{
		switch (this->tag)
		{
		case attr_e::DOUBLE:
			return d == _e.d;
		case attr_e::POLYGON:
		{
			std::deque<poly_t> outputInter = attr_e::intersect(*this, _e);
			if (outputInter.size() > 0)
				return true;
			return false;
		}
			// return bg::within(_e.poly,this->poly);
		default:
			return i == _e.i;
			break;
		}
	}

	bool operator!=(const attr_e &_e) const
	{
		switch (this->tag)
		{
		case attr_e::INT64_T:
			return i != _e.i;
		case attr_e::DOUBLE:
			return d != _e.d;
		default:
			return false;
			break;
		}
	}

	static std::deque<poly_t> intersect(const attr_e &a, const attr_e &b)
	{
		std::deque<poly_t> output;
		bg::intersection(a.poly, b.poly, output);
		return output;

		// poly_t green, blue;

		// boost::geometry::read_wkt("POLYGON ((-121.1949094667507 39.10744767599169, -121.179449350117 39.0984544700691, -121.1678611142817 39.0984544700691, -121.1833197524842 39.10744767599169, -121.1949094667507 39.10744767599169))"
		// 	, green);

		// boost::geometry::read_wkt("POLYGON ((-121.1949094667507 39.10744767599169, -121.179449350117 39.0984544700691, -121.1678611142817 39.0984544700691, -121.1833197524842 39.10744767599169, -121.1949094667507 39.10744767599169))"
		// 	, blue);

		// std::deque<poly_t> output;
		// boost::geometry::intersection(green, blue, output);
		// return output;
	}
};

namespace std
{
	template <>
	struct hash<attr_e>
	{
		std::size_t operator()(const attr_e &k) const
		{
			using std::hash;
			using std::size_t;
			using std::string;

			// Compute individual hash values for first,
			// second and third and combine them using XOR
			// and bit shifting:
			switch (k.tag)
			{
			case attr_e::INT64_T:
				return (hash<attr_t>()(k.i) ^ (hash<int>()(k.tag) << 1));
			case attr_e::DOUBLE:
				return ((hash<double>()(k.d) << 1) >> 1) ^ (hash<int>()(k.tag) << 1);
			default:
				return ((hash<attr_t>()(k.i) ^ (hash<double>()(k.d) << 1)) >> 1) ^ (hash<int>()(k.tag) << 1);
			}

			return ((hash<attr_t>()(k.i) ^ (hash<double>()(k.d) << 1)) >> 1) ^ (hash<int>()(k.tag) << 1);
		}
	};
} // namespace std

struct StreamEvent
{
	enum Flags
	{
		F_TIMEOUT = 0x1,
		F_TIMESTAMP = 0x2,
		F_REVOKE = 0x4,
	};

	StreamEvent() : head(0), offset(0)
	{
	}

	union
	{
		struct
		{
			uint16_t typeIndex;
			uint16_t typeHash;
			uint8_t attributeCount;
			uint8_t flags;
			uint8_t timeoutState;
			uint8_t reserved;
		};

		uint64_t head;
	};

	attr_e attributes[32]; //attributes[EventDecl::MAX_ATTRIBUTES];
	mutable uint64_t offset;

	bool operator<(const StreamEvent &_o) const
	{
		bool less = false;
		switch (attributes[0].tag)
		{
		case attr_e::INT64_T:
			less = attributes[0].i < _o.attributes[0].i;
			break;
		case attr_e::DOUBLE:
			less = attributes[0].d < _o.attributes[0].d;
			break;
		default:
			less = less;
		}
		return less;
	}

	bool operator==(const StreamEvent &_o) const
	{
		bool equal = true;
		if (head == _o.head)
		{
			for (size_t i = 0; i < attributeCount; ++i)
			{
				switch (attributes[i].tag)
				{
				case attr_e::INT64_T:
					equal = attributes[i].i == _o.attributes[i].i;
					break;
				case attr_e::DOUBLE:
					equal = attributes[i].d == _o.attributes[i].d;
					break;
				default:
					equal = true;
				}
				if (equal == false)
					return equal;
			}
			return true;
		}
		return false;
	}

	static void setupStdIo(FILE *_stream = NULL)
	{
		if (_stream)
		{
#ifdef _WIN32
			_setmode(_fileno(_stream), _O_BINARY);
#endif
		}
		else
		{
#ifdef _WIN32
			_setmode(_fileno(stdin), _O_BINARY);
			_setmode(_fileno(stdout), _O_BINARY);
#else
			freopen(NULL, "rb", stdin);
			freopen(NULL, "wb", stdout);
#endif
		}
	}

	static uint16_t hash(const std::string &_string)
	{
		uint32_t out;
		MurmurHash3_x86_32(_string.c_str(), (int)_string.length(), 0xDEADBEEFu, &out);
		return (uint16_t)out;
	}

	bool read(FILE *_file = stdin)
	{
		if (fread(&head, sizeof(head), 1, _file) == 1)
		{
			if (fread(attributes, sizeof(uint64_t), attributeCount, _file) == attributeCount)
			{
				offset += sizeof(uint64_t) * attributeCount + sizeof(head);
				return true;
			}
		}
		return false;
	}

	bool write(FILE *_file = stdout) const
	{
		offset += sizeof(uint64_t) * (1 + attributeCount);

		return fwrite(&head, sizeof(head), 1, _file) == 1 &&
			   fwrite(attributes, sizeof(uint64_t) * attributeCount, 1, _file) == 1;
	}
};


