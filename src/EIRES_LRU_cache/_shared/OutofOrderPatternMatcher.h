#pragma once
#include "../PatternMatcher.h"
#include <list>
#include <vector>

struct Query;

class OutofOrderPatternMatcher : public PatternMatcher
{
	struct EventInfo
	{
		attr_t attribute[MAX_ATTRIBUTES];
	};


public:
	OutofOrderPatternMatcher();

	void init(const Query* _query, const std::function<void(const attr_t*)>& _revokeCallback);

	virtual uint32_t event(uint32_t _type, const attr_t* _attr);
protected:
	uint32_t ooevent(uint32_t _type, EventInfo& _event);
	uint32_t replayEvents(uint32_t _state, attr_t _timestamp, size_t _runOffset);

private:

	attr_t								m_LastEventTimestamp;
	std::vector<std::list<EventInfo> >	m_EventQueue;
	std::vector<bool>					m_StoreEvent;

};
