#include "Query.h"
#include "eql.h"
#include "_shared/Aggregation.h"
#include "_shared/GlobalClock.h"

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <iostream>

using namespace std;

extern "C" int yylex();
extern "C" FILE* yyin;

static bool findQueryEventAttrIndex(const Query& _q, const QueryLoader& _loader, const std::string& _event, const std::string& _attr, std::pair<uint32_t, uint32_t>& _out)
{
	for (uint32_t i = 0; i < _q.events.size(); ++i)
	{
		if (_q.events[i].name == _event)
		{
			_out.first = i;
			_out.second = _loader.eventDecl(_q.events[i].type.c_str())->findAttribute(_attr.c_str());
			return _out.second != ~0u;
		}
	}
	return false;
}

bool QueryLoader::loadFile(const char* _path)
{
	yyin = fopen(_path, "r");
	if (!yyin)
		return false;

	m_NextTok = yylex();
	while (peak())
	{
		switch (peak())
		{
		case KW_TYPE:	parseType();	break;
		case KW_QUERY:	parseQuery();	break;
		default:		parseError("unexpected token");
		}
	}

	return true;
}

bool QueryLoader::storeFile(const char * _path) const
{
	ofstream o(_path, ofstream::out | ofstream::trunc);

	for (const EventDecl& decl : m_EventDecls)
	{
		o << "TYPE " << decl.name << " {" << endl;

		for (const string& attr : decl.attributes)
		{
			o << "\t" << attr;
			if (&attr != &decl.attributes.back())
				o << ",";
			o << endl;
		}
		
		o << "};" << endl << endl;
	}

	for (const Query& q : m_Queries)
	{
		o << "QUERY " << q.name << endl;

		o << "EVENT SEQ(";
		for (const auto& e : q.events)
		{
			if (e.stopEvent)
				o << '~';
			
			o << e.type;

			if (e.kleenePlus)
				o << '+';

			o << ' ' << e.name;

			if (&e != &q.events.back())
				o << ", ";
		}
		o << ')' << endl;

		if (!q.where.empty())
		{
			o << "WHERE ";

			for (const auto& p : q.where)
			{
				o << generateEventAttribute(q, p.event1, p.attr1);
				o << ' ' << PatternMatcher::operatorInfo(p.op).sign << ' ';

				if (p.attr2 == Query::DA_ZERO)
				{
					o << p.offset;
				}
				else
				{
					o << generateEventAttribute(q, p.event2, p.attr2);

					if (p.offset)
					{
						o << " + " << p.offset;
					}
				}

				if (&p != &q.where.back())
					o << " && " << endl;
			}

			o << endl;
		}

		o << "WITHIN " << q.within << "us";

		if (!q.returnName.empty())
		{
			o << endl << "RETURN " << q.returnName << '(';

			for (const auto& a : q.returnAttr)
			{
				o << generateEventAttribute(q, a.first, a.second);

				if (&a != &q.returnAttr.back())
					o << ", ";
			}

			o << ')';
		}
		o << ';' << endl << endl;
	}
	return true;
}

void QueryLoader::parseError(const char* _msg)
{
	fprintf(stderr, "error: %s\n", _msg);
	exit(1);
}

void QueryLoader::skip(int _tok)
{
	assert(_tok == m_NextTok);
	if (_tok != m_NextTok)
		parseError("unexpected token");
	m_NextTok = yylex();
}

void QueryLoader::parseType()
{
	EventDecl decl;

	skip(KW_TYPE);
	decl.name = yylval.string;
	skip(ID);
	skip('{');
	while (true)
	{
		decl.attributes.push_back(yylval.string);
		skip(ID);
		if (peak() != ',')
			break;
		skip(',');
	}
	skip('}');
	skip(';');

	m_EventDecls.push_back(decl);
}

std::pair<uint32_t, uint32_t> QueryLoader::parseEventAttrId(Query& _q)
{
	string eventName = yylval.string;	skip(ID);

	if (peak() == '(')
	{
		Query::Aggregation a;
		a.function = eventName;

		skip('(');
		a.source = parseEventAttrId(_q);
		skip(')');

		auto it = find(_q.aggregation.begin(), _q.aggregation.end(), a);
		if (it == _q.aggregation.end())
		{
			_q.aggregation.push_back(a);
			return make_pair(a.source.first + 1, (uint32_t)(Query::DA_LAST_ATTRIBUTE - (_q.aggregation.size() - 1)));
		}
		else
		{
			return make_pair(a.source.first + 1, (uint32_t)(Query::DA_LAST_ATTRIBUTE - (it - _q.aggregation.begin())));
		}
	}
	else
	{
		skip('.');
		string attrName = yylval.string;	skip(ID);

		for (size_t i = 0; i < _q.events.size(); ++i)
		{
			if (_q.events[i].name == eventName)
			{
				if (attrName[0] == '_')
				{
					// predefined attributes
					Query::DefinedAttributes type;
					if (attrName == "_ID")
						type = Query::DA_ID;
					else if (attrName == "_OFFSET")
						type = Query::DA_OFFSET;
					else if (attrName == "_CURRENT_TIME")
						type = Query::DA_CURRENT_TIME;
					else
						parseError("unsupported predefined event attribute");

					return make_pair((uint32_t)i, (uint32_t)type);
				}
				else
				{
					const EventDecl* decl = eventDecl(_q.events[i].type.c_str());

					auto it = find(decl->attributes.begin(), decl->attributes.end(), attrName);
					if (it != decl->attributes.end())
					{
						return make_pair((uint32_t)i, (uint32_t)(it - decl->attributes.begin()));
					}
				}

				puts(eventName.c_str());
				parseError("event attribute name not found");
			}
		}
		parseError("event name not found");
		return make_pair(0ul, 0ul);
	}
}

std::string QueryLoader::generateEventAttribute(const Query & _query, uint32_t _eventIdx, uint32_t _eventAttrIdx) const
{
	if (_eventAttrIdx > Query::DA_LAST_ATTRIBUTE)
	{
		// predefined defined attributes

		const auto& e = _query.events[_eventIdx];
		switch (_eventAttrIdx)
		{
		case Query::DA_ID:
			return e.name + "._ID";
		case Query::DA_OFFSET:
			return e.name + "._OFFSET";
		case Query::DA_CURRENT_TIME:
			return e.name + "._CURRENT_TIME";
		case Query::DA_ZERO:
			return e.name + "._ZERO";
		case Query::DA_MAX:
			return e.name + "._MAX";
		default:
			assert(!"no impl");
		}
	}
	else if (_eventAttrIdx > Query::DA_LAST_ATTRIBUTE - _query.aggregation.size())
	{
		// aggregation function

		const auto& a = _query.aggregation[Query::DA_LAST_ATTRIBUTE - _eventAttrIdx];
		return a.function + '(' + generateEventAttribute(_query, a.source.first, a.source.second) + ')';
	}
	else
	{
		const auto& e = _query.events[_eventIdx];
		const auto& decl = m_EventDecls[findEventDecl(e.type.c_str())];

		return e.name + '.' + decl.attributes[_eventAttrIdx];
	}

	return string();
}

void QueryLoader::parseQuery()
{
	Query q;

	skip(KW_QUERY);
	q.name = yylval.string;
	skip(ID);

	skip(KW_EVENT);
	parseQuerySeq(q);

	if (peak() == KW_WHERE)
	{
		parseQueryWhere(q);
	}

	parseQueryWithin(q);

	if (peak() == KW_RETURN)
	{
		parseQueryReturn(q);
	}

	skip(';');

	q.fillAttrMap();
	m_Queries.push_back(q);
}

void QueryLoader::parseQuerySeq(Query& _q)
{
	skip(KW_SEQ);
	skip('(');
	while (true)
	{
		Query::Event e;

		if(e.stopEvent = peak() == '~')
			skip('~');

		e.type = yylval.string;
		skip(ID);

		if (e.kleenePlus = peak() == '+')
			skip('+');

		e.name = yylval.string;
		skip(ID);

		if (!eventDecl(e.type.c_str()))
			parseError("event type not found during query parsing");

		_q.events.push_back(move(e));

		if (peak() != ',')
			break;
		skip(',');
	}
	skip(')');
}

void QueryLoader::parseQueryWhere(Query& _q)
{
	skip(KW_WHERE);

	while (true)
	{
		if (peak() == '[')
		{
			skip('[');
			string name = yylval.string;	skip(ID);
			skip(']');

			const vector<string>& attrVec = eventDecl(_q.events[0].type.c_str())->attributes;
			vector<string>::const_iterator pos = find(attrVec.begin(), attrVec.end(), name);
			if (pos == attrVec.end())
				parseError("event attribute not found");
			const uint32_t e0attrIdx = (uint32_t)(pos - attrVec.begin());

			for (uint32_t e = 1; e < _q.events.size(); ++e)
			{
				Query::Predicate p = {};
				p.event1 = 0;
				p.attr1 = e0attrIdx;
				p.op = PatternMatcher::OP_EQUAL;

				const vector<string>& attrVec = eventDecl(_q.events[e].type.c_str())->attributes;
				vector<string>::const_iterator pos = find(attrVec.begin(), attrVec.end(), name);
				if (pos == attrVec.end())
					parseError("event attribute not found");

				p.event2 = e;
				p.attr2 = (uint32_t)(pos - attrVec.begin());
				_q.where.push_back(p);
			}
		}
		else
		{
			Query::Predicate p = {};

			pair<uint32_t, uint32_t> param1 = parseEventAttrId(_q);
			p.event1 = param1.first;
			p.attr1 = param1.second;

			if (peak() < LSS || peak() > NEQ)
				parseError("unknown operator found in query");
			p.op = (PatternMatcher::Operator)(peak() - LSS);
			skip(peak());

			if (peak() == CONST_INT)
			{
				p.event2 = param1.first;
				p.attr2 = Query::DA_ZERO;
				p.offset = (attr_t)yylval.intValue;
				skip(CONST_INT);
			}
			else
			{
				pair<uint32_t, uint32_t> param2 = parseEventAttrId(_q);
				p.event2 = param2.first;
				p.attr2 = param2.second;
				p.offset = 0;
			}

			if (p.event1 > p.event2)
			{
				parseError("wrong predicate event order");
			}
			else if (p.event1 == p.event2)
			{
				if (p.attr1 < Query::DA_LAST_ATTRIBUTE - _q.aggregation.size() || p.attr1 > Query::DA_LAST_ATTRIBUTE)
					parseError("event preconditions not implemented");
			}
			_q.where.push_back(p);
		}

		if (peak() != AND)
			break;
		skip(AND);
	}
}

void QueryLoader::parseQueryWithin(Query& _q)
{
	skip(KW_WITHIN);

	_q.within = yylval.intValue;
	skip(CONST_INT);

	if (!strcmp(yylval.string, "us"))
	{
	}
	else if (!strcmp(yylval.string, "ms"))
	{
		_q.within *= 1000;
	}
	else if (!strcmp(yylval.string, "s"))
	{
		_q.within *= 1000 * 1000;
	}
	else if (!strcmp(yylval.string, "m"))
	{
		_q.within *= 60ull * 1000 * 1000;
	}
	else if (!strcmp(yylval.string, "h"))
	{
		_q.within *= 60ull * 60 * 1000 * 1000;
	}
	else if (!strcmp(yylval.string, "d"))
	{
		_q.within *= 24ull * 60 * 60 * 1000 * 1000;
	}
	skip(ID);
}

void QueryLoader::parseQueryReturn(Query& _q)
{
	skip(KW_RETURN);
	_q.returnName = yylval.string;	skip(ID);

	skip('(');
	while (true)
	{
		_q.returnAttr.push_back(parseEventAttrId(_q));

		if (peak() != ',')
			break;
		skip(',');
	}
	skip(')');
}


const Query* QueryLoader::query(const char* _name) const
{
	for (auto& it : m_Queries)
		if (!strcmp(it.name.c_str(), _name))
			return &it;
	return NULL;
}

const EventDecl* QueryLoader::eventDecl(const char* _name) const
{
	for (auto& it : m_EventDecls)
		if (!strcmp(it.name.c_str(), _name))
			return &it;
	return NULL;
}

uint32_t QueryLoader::findEventDecl(const char* _name) const
{
	for (uint32_t i = 0; i < m_EventDecls.size(); ++i)
		if (!strcmp(_name, m_EventDecls[i].name.c_str()))
			return i;
	return ~0u;
}

void QueryLoader::print()
{
    cout << " EventDecl: " << endl;
    for(auto &&e: m_EventDecls)
    {
        cout << "name : " << e.name << endl; 
        cout << "attributes : " << flush;
        for(auto a : e.attributes)
            cout << a << ",";
        cout << endl;
    }

    cout << "print queries " << endl;
    for(auto && q: m_Queries)
        q.print();
}

PatternMatcher::AggregationFunction * QueryLoader::aggregationFunction(const std::string & _name) const
{
	if (_name == "avg")
		return new AggregationAvg();
	if (_name == "count")
		return new AggregationCount();

	fprintf(stderr, "aggregation function %s unknown\n", _name.c_str());
	return new AggregationAvg();
}

void Query::print()
{
    cout << "query name: " << name << endl;
    cout << "query events: " << endl;
    int i = 0;
    for(auto && e : events)
    {
        cout << i << " , " << e.name << e.type << endl;
        ++i;
    }

    i = 0;

    cout << "query where predicates: " << endl;
    for(auto && p : where)
        cout << p.event1 << "." << p.attr1 << "---- " << p.op << "---- " <<  p.event2 << "." << p.attr2 << endl;

    cout << "query attrMap: " << endl;
    for(auto && a : attrMap)
        cout << a.first << " , " << a.second << endl;

    cout << "time window: " << within << endl;

    cout << "return attribute: " << endl;
    for(auto && r:returnAttr)
        cout << r.first << "," << r.second <<endl;

}

void Query::insertEvent(Event _e, size_t _idx)
{
	events.insert(events.begin() + _idx, move(_e));

	for (Predicate& p : where)
	{
		if (p.event1 >= _idx)
			p.event1++;
		if (p.event2 >= _idx)
			p.event2++;
	}
	for (auto& r : returnAttr)
	{
		if (r.first >= _idx)
			r.first++;
	}
	for (Aggregation& a : aggregation)
	{
		if (a.source.first >= _idx)
			a.source.first++;
	}

	attrMap.clear();
}

void Query::generateCopyList(const std::vector<std::pair<uint32_t, uint32_t> >& _attrList, std::vector<uint32_t>& _copyList) const
{
	for (const auto& it : _attrList)
	{
		const auto pos = find(attrMap.begin(), attrMap.end(), it);
		if (pos != attrMap.end())
		{
			_copyList.push_back((uint32_t)(pos - attrMap.begin()));
		}
		else
		{
			assert(!"attribute not found");
			_copyList.push_back(0);
		}
	}
}

void Query::fillAttrMap(size_t _reservedSlots)
{
	attrMap.clear();

	// timestamp
	attrMap.push_back(pair<uint32_t, uint32_t>(0, 0));

	// add reserved slots
	attrMap.resize(_reservedSlots + 1);

	// attributes used for kleene sorting
	for (uint32_t i = 0; i < events.size(); ++i)
	{
		pair<uint32_t, uint32_t> item(i, Query::DA_OFFSET);
		if (events[i].kleenePlus && find(attrMap.begin(), attrMap.end(), item) == attrMap.end())
			attrMap.push_back(item);
	}

	// aggregationn source attributes
	for (const auto& it : aggregation)
	{
		if (find(attrMap.begin(), attrMap.end(), it.source) == attrMap.end())
			attrMap.push_back(it.source);
	}

	// return attributes
	for (const auto& it : returnAttr)
	{
		if (find(attrMap.begin(), attrMap.end(), it) == attrMap.end())
			attrMap.push_back(it);
	}

	// match attributes
	for (const auto& it : where)
	{
		pair<uint32_t, uint32_t> item(it.event1, it.attr1);
		if (find(attrMap.begin(), attrMap.end(), item) == attrMap.end())
			attrMap.push_back(item);
	}

	std::sort(attrMap.begin(), attrMap.end());
}

bool QueryLoader::setupPatternMatcher(const Query* _query, PatternMatcher& _matcher, const Callbacks& _functions) const
{
	// find event attributes to store in run
	const uint32_t numEvents = (uint32_t)_query->events.size();
	//_matcher.addState(numEvents * 2 + 1, 0, PatternMatcher::ST_REJECT); // create last timeout event
	_matcher.addState(numEvents + 1, 0, PatternMatcher::ST_REJECT); // create last timeout event

	uint32_t lastEvent = 0;
	for (uint32_t i = 0; i < numEvents; ++i)
		if (!_query->events[i].stopEvent)
			lastEvent = i;

    // add states
	for (uint32_t i = 0; i < numEvents; ++i)
	{
		// create state
		PatternMatcher::StateType type;
		uint32_t numSlots = 0;

		if (i == lastEvent)
		{
			type = PatternMatcher::ST_ACCEPT;
		}
		else if (_query->events[i].stopEvent)
		{
			type = PatternMatcher::ST_REJECT;
		}
		else
		{
			type = PatternMatcher::ST_NORMAL;

			for (const auto& it : _query->attrMap)
				if (it.first <= i)
					numSlots++;
		}

		uint32_t kleeneAttr = 0;
		if (_query->events[i].kleenePlus)
			kleeneAttr = (uint32_t)(find(_query->attrMap.begin(), _query->attrMap.end(), make_pair(i, (uint32_t)Query::DA_OFFSET)) - _query->attrMap.begin());

		_matcher.addState(i + 1,G_numTimeslice, G_numCluster, numSlots, type, kleeneAttr);
        //cout << "[QueryLoader::setupPatternMatcher ] state " <<  i+1 << "--" << G_numTimeslice << " || " << G_numCluster << endl;
        //cout << _matcher.m_States[i+1].buffers.size() << endl;

		if (_functions.insertEvent[type])
			_matcher.setCallback(i+1, PatternMatcher::CT_INSERT, bind(_functions.insertEvent[type], i + 1, placeholders::_1));

		if (_functions.timeoutEvent)
			_matcher.setCallback(i+1, PatternMatcher::CT_TIMEOUT, bind(_functions.timeoutEvent, i + 1, placeholders::_1));

	}

    _matcher.setStates2States();



	// add sequential transitions
	uint32_t srcState = 0;
	for (uint32_t i = 0; i < numEvents; ++i)
	{
		uint32_t dstState = i + 1;

		const uint32_t eventType = findEventDecl(_query->events[i].type.c_str());
		_matcher.addTransition(srcState, dstState, eventType);

		for (uint32_t m = 0; m < _query->attrMap.size(); ++m)
		{
			if (_query->attrMap[m].first == i)
				_matcher.addActionCopy(_query->attrMap[m].second, m);
		}

		for (const auto& it : _query->where)
		{
			if (it.event2 != i)
				continue;

			if (it.event1 != it.event2)
			{
				uint32_t runAttrIdx = (uint32_t)(find(_query->attrMap.begin(), _query->attrMap.end(), pair<uint32_t, uint32_t>(it.event1, it.attr1)) - _query->attrMap.begin());
				assert(runAttrIdx < _query->attrMap.size());
				_matcher.addCondition(runAttrIdx, it.attr2, it.op, it.offset);
			}
			else
			{
				_matcher.addCondition(it.attr1, it.attr2, it.op, it.offset, true);
			}
		}
		
		if (!_query->events[i].stopEvent)
			srcState = i + 1;
	}

    _matcher.setStates2Transitions();

	// setup aggregation functions
	for (size_t i = 0; i < _query->aggregation.size(); ++i)
	{
		const Query::Aggregation& a = _query->aggregation[i];
		uint32_t state = a.source.first + 1;
		uint32_t srcAttr = (uint32_t)(find(_query->attrMap.begin(), _query->attrMap.end(), a.source) - _query->attrMap.begin());
		uint32_t dstAttr = Query::DA_LAST_ATTRIBUTE - (uint32_t)i;

		auto* function = aggregationFunction(a.function);

		_matcher.addAggregation(state, function, srcAttr, dstAttr);
	}

	_matcher.setTimeout(_query->within);


    

	return true;
}
