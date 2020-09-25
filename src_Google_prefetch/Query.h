#pragma once
#include <vector>
#include <string>
#include <string.h>
#include "PatternMatcher.h"

struct EventDecl
{
    enum {
        MAX_ATTRIBUTES = 32
    };

    std::string	name;
    std::vector<std::string> attributes;

    uint32_t findAttribute(const char* _name) const
    {
        for (uint32_t i = 0; i < attributes.size(); ++i)
            if (!strcmp(_name, attributes[i].c_str()))
                return i;
        return ~0u;
    }
};

struct Query
{
    struct Predicate
    {
        uint32_t event1, event2;
        uint32_t attr1, attr2;
        PatternMatcher::Operator op;
        attr_t	offset;
    };

    struct AttrMapping
    {
        uint32_t event;
        uint32_t attr;
        uint32_t index;
    };

    struct Aggregation
    {
        std::string	function;
        std::pair<uint32_t, uint32_t> source;

        bool operator == (const Aggregation& _o) const
        {
            return function == _o.function && source == _o.source;
        }
    };

    struct Event
    {
        std::string	type;
        std::string	name;
        bool		stopEvent;
        bool		kleenePlus;
    };

    enum DefinedAttributes
    {
        DA_ID			= PatternMatcher::MAX_ATTRIBUTES - 1,	// sequential id of event in stream
        DA_OFFSET		= PatternMatcher::MAX_ATTRIBUTES - 2,	// file offset of event in event stream
        DA_CURRENT_TIME	= PatternMatcher::MAX_ATTRIBUTES - 3,	// local timestamp in us
        DA_ZERO			= PatternMatcher::MAX_ATTRIBUTES - 4,	// always zero to support constant number checks
        DA_MAX			= PatternMatcher::MAX_ATTRIBUTES - 5,	// always max attr_t for initialisation
        DA_FULL_MATCH_TIME			= PatternMatcher::MAX_ATTRIBUTES - 6,	

        DA_COUNT = 6,
        DA_LAST_ATTRIBUTE = PatternMatcher::MAX_ATTRIBUTES - DA_COUNT - 1
    };

    std::string name;
    std::vector<Event> events; // list of type and instance name of sequential events
    std::vector<Predicate> where;
    uint64_t within;

    std::string returnName;
    std::vector<std::pair<uint32_t,uint32_t> > returnAttr; // event and attr index for select

    std::vector<Aggregation> aggregation;

    std::vector<std::pair<uint32_t, uint32_t> > attrMap; // mapping of eventIdx and AttributeIdx to RunAttrIdx

    void insertEvent(Event _e, size_t _idx);
    void generateCopyList(const std::vector<std::pair<uint32_t, uint32_t> >& _attrList, std::vector<uint32_t>& _copyList) const;
    void fillAttrMap(size_t _reservedSlots = 0);
};

class QueryLoader
{
    public:
        bool loadFile(const char* _path);
        bool storeFile(const char* _path) const;

        void clear()											{ m_EventDecls.clear(); m_Queries.clear(); }
        void addEventDecl(const EventDecl& _decl)				{ m_EventDecls.push_back(_decl); }
        void addQuery(const Query& _q)							{ m_Queries.push_back(_q); }

        size_t numQueries() const								{ return m_Queries.size(); }
        const Query* query(size_t _idx) const					{ return &m_Queries[_idx]; }
        const Query* query(const char* _name) const;

        size_t numEventDecls() const							{ return m_EventDecls.size(); }
        uint32_t timeupdateEvent() const						{ return (uint32_t)numEventDecls(); }
        const EventDecl* eventDecl(size_t _idx) const			{ return &m_EventDecls[_idx]; }
        const EventDecl* eventDecl(const char* _name) const;
        uint32_t findEventDecl(const char* _name) const;

        PatternMatcher::AggregationFunction* aggregationFunction(const std::string& _name) const;

        struct Callbacks
        {
            std::function<void(uint32_t, const attr_t*)>	insertEvent[PatternMatcher::ST_MAX];
            std::function<void(uint32_t, const attr_t*)>	timeoutEvent;
        };

        bool setupPatternMatcher(const Query* _query, PatternMatcher& _matcher, const Callbacks& _functions) const;

    protected:
        int peak() const { return m_NextTok; }
        void skip(int _tok);
        void parseError(const char* _msg);
        void parseType();
        void parseQuery();
        void parseQuerySeq(Query& _q);
        void parseQueryWhere(Query& _q);
        void parseQueryWithin(Query& _q);
        void parseQueryReturn(Query& _q);
        std::pair<uint32_t, uint32_t> parseEventAttrId(Query& _q);

        std::string generateEventAttribute(const Query& _query, uint32_t _eventIdx, uint32_t _eventAttrIdx) const;

    private:
        std::vector<EventDecl>	m_EventDecls;
        std::vector<Query>		m_Queries;

        int	m_NextTok;

};
