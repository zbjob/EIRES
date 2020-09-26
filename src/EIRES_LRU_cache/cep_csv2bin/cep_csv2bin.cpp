#include "../Query.h"
#include "../EventStream.h"

#include "../freegetopt/getopt.h"

#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <set>
#include <assert.h>

using namespace std;

static set<uint64_t>	g_AttributeValueHash;

std::vector<std::string> explode(const std::string& str, char delimiter)
{
	std::vector<std::string> tokens;
	std::stringstream tokenStream(str);
	std::string tempStr;

	while (std::getline(tokenStream, tempStr, delimiter))
	{
		tokens.push_back(tempStr);
	}

	return tokens;
}


// some hard coded attributes generated from multiple attributes or thin air
enum GeneratedAttribute
{
	GA_ID = -1, // sequentially counting number
	GA_JOBTASK = -2, // job_id << 20 | task_index

	GA_NUM = 2
};

struct ColMapping : vector<pair<int, int> >
{
	size_t num_attr;
	size_t event_type;
	uint32_t event_type_hash;
	const EventDecl* decl;
};

static vector<ColMapping> GenerateMapping(QueryLoader& _def, const vector<string>& _header)
{
	vector<ColMapping> ret;

	for (size_t i = 0; i < _def.numEventDecls(); ++i)
	{
		ColMapping m;
		m.decl = _def.eventDecl(i);
		m.num_attr = m.decl->attributes.size();
		m.event_type = i;
		m.event_type_hash = StreamEvent::hash(m.decl->name);

		for (size_t a = 0; a < m.decl->attributes.size(); ++a)
		{
			auto it = find(_header.begin(), _header.end(), m.decl->attributes[a]);
			if (it != _header.end())
				m.push_back(make_pair((int)(it - _header.begin()), (int)a));
			else
			{
				if (m.decl->attributes[a] == "id")
				{
					m.push_back(make_pair((int)GA_ID, (int)a));
				}
				else if (m.decl->attributes[a] == "task_id")
				{
					m.push_back(make_pair((int)GA_JOBTASK, (int)a));
				}
				else
				{
					fprintf(stderr, "warning: attribute %s of event %s not found in column list\n", m.decl->attributes[a].c_str(), m.decl->name.c_str());
				}
			}
		}

		ret.push_back(m);
	}
	return ret;
}

void storeAttributeHash(uint64_t _hash, const char* _value)
{
	if (g_AttributeValueHash.find(_hash) == g_AttributeValueHash.end())
	{
		g_AttributeValueHash.insert(_hash);
		cerr << "hash: " << hex << _hash << " = " << _value << endl;
	}
}

attr_t parseAttribute(const char* _str)
{
	bool hasPoint = false;
	bool hasAlpha = false;
	for (const char* it = _str; *it; it++)
	{
		if (*it == '.')
			hasPoint = true;
		else if (*it < '0' || *it > '9')
			hasAlpha = true;
	}

	if (hasAlpha)
	{
		uint64_t buf[2];
		MurmurHash3_x64_128(_str, (int)strlen(_str), 0xBABE05u, buf);
		//storeAttributeHash(buf[0], _str);
		return buf[0];
	}
	else if (hasPoint)
	{
		double x = atof(_str);
		return (attr_t)(round(x * 100));
	}
	return atoll(_str);
}

int main(int _argc, char* _argv[])
{
	StreamEvent::setupStdIo(stdout);

	const char* deffile = "default.eql";
	string headerLine;

	vector<pair<string, string> > typeMapping;

	bool order = false;
	int c;
	while ((c = getopt(_argc, _argv, "c:a:m:so")) != -1)
	{
		switch (c)
		{
		case 'c':
			deffile = optarg;
			break;
		case 'a':
			headerLine = optarg;
			break;
		case 's':
			{
				string hole;
				getline(std::cin, hole);
			}
			break;
		case 'o':
			order = true;
			break;
		case 'm':
		{
			vector<string> arg = explode(string(optarg), '=');
			if (arg.size() == 2)
			{
				typeMapping.push_back(pair<string, string>(arg[0], arg[1]));
			}
			else
			{
				abort();
			}
			break;
		}
		default:
			abort();
		}
	}
	sort(typeMapping.begin(), typeMapping.end());

	QueryLoader def;
	if (!def.loadFile(deffile))
	{
		fprintf(stderr, "failed to load definition file %s\n", deffile);
		return 1;
	}

	if (headerLine.empty())
		getline(std::cin, headerLine);

	const vector<string> header = explode(headerLine, ',');
	vector<ColMapping> mapping = GenerateMapping(def, header);

	vector<StreamEvent> events;

	size_t typeColIdx = (size_t)(find(header.begin(), header.end(), "_type") - header.begin());
	size_t jobIdColIdx = (size_t)(find(header.begin(), header.end(), "job_id") - header.begin());
	size_t taskIdxColIdx = (size_t)(find(header.begin(), header.end(), "task_index") - header.begin());

	string line;
	uint64_t skipCounter = 0;
	attr_t idCounter = -1;
	while (getline(cin, line))
	{
		++idCounter;
		vector<string> attributeStr = explode(line, ',');

		if (attributeStr.size() <= typeColIdx)
		{
			skipCounter++;
			continue;
		}

		auto it_type_mapping = lower_bound(typeMapping.begin(), typeMapping.end(), attributeStr[typeColIdx],
			[](const pair<string, string>& _a, const string& _b) -> bool { return strcmp(_a.first.c_str(), _b.c_str()) < 0; });
		const string& typeName = it_type_mapping != typeMapping.end() && it_type_mapping->first == attributeStr[typeColIdx] ? it_type_mapping->second : attributeStr[typeColIdx];

		uint32_t declIdx = def.findEventDecl(typeName.c_str());
		if (declIdx == ~0u)
		{
			skipCounter++;
			continue;
		}

		const ColMapping& m = mapping[declIdx];

		StreamEvent o;
		o.attributeCount = (uint8_t)m.num_attr;
		o.typeIndex = (uint16_t)m.event_type;
		o.typeHash = m.event_type_hash;

		// clear all used attributes
		for (size_t i = 0; i < m.num_attr; ++i)
			o.attributes[i] = 0;

		// copy columns
		for (auto it : m)
		{
			switch (it.first)
			{
			case GA_ID:
				o.attributes[it.second] = idCounter;
				break;
			case GA_JOBTASK:
				if (jobIdColIdx < attributeStr.size() && taskIdxColIdx < attributeStr.size())
				{
					attr_t jobId = (attr_t)atoll(attributeStr[jobIdColIdx].c_str());
					attr_t taskIdx = (attr_t)atoll(attributeStr[taskIdxColIdx].c_str());
					o.attributes[it.second] = (attr_t)((jobId << 20) | taskIdx);

					if (taskIdx >= 1ull << 20)
						fprintf(stderr, "%s WARNING: task index out of bounds!!! %llu\n", _argv[0], taskIdx);
				}
				break;
			default:
				if(it.first < attributeStr.size())
					o.attributes[it.second] = parseAttribute(attributeStr[it.first].c_str());
			}
		}
		if (order)
			events.push_back(o);
		else
			o.write();
	}

	if (order)
	{
		fprintf(stderr, "%s: begin sorting of %u events\n", _argv[0], (unsigned)events.size());
		sort(events.begin(), events.end());

		for (const auto& it : events)
			it.write();
	}

	fprintf(stderr, "%s: %llu events skipped\n", _argv[0], skipCounter);
	return 0;
}
