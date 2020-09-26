#pragma once
#include "../PatternMatcher.h"
#include "../Query.h"

#include <deque>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <assert.h>

class PredicateMiner
{
	enum Op {
		O_LESS,
		O_EQUAL,
		O_GREATER,
		O_MAX
	};

	struct EventInfo
	{
		size_t					numAttributes;
		std::vector<uint8_t>	eventMapping;
	};

	struct EventItem
	{
		uint32_t	type;
		uint64_t	id;
		uint64_t	nextIdByType;
		attr_t		attributes[PatternMatcher::MAX_ATTRIBUTES];
	};

	struct PredicateInfo
	{
		uint32_t	eventId;
		uint32_t	numAttr;
		uint32_t	attrIdx[2];
		PatternMatcher::Operator op[2];
	};

	struct MiningTask
	{
		size_t				listIdx;
		const EventItem*	event;
		size_t				sliceBegin;
		size_t				sliceEnd;
		std::atomic<size_t>	current;
		MiningTask*			next;
	};

	struct AppearanceInfo
	{
		struct Todo
		{
			uint32_t attr1 : 7;
			uint32_t attr2 : 7;
			uint32_t attr1_mapped : 7;
			uint32_t attr2_mapped : 7;
			uint32_t op1 : 2;
			uint32_t op2 : 2;
		};

		enum {
			MAX_ATTRIBUTES = PatternMatcher::MAX_ATTRIBUTES - Query::DA_COUNT,
			NUM_ATTRIBUTES = (MAX_ATTRIBUTES - 1) * MAX_ATTRIBUTES / 2 + MAX_ATTRIBUTES
		};

		AppearanceInfo() : todo_count(0)
		{
			memset(attribute, 0, sizeof(attribute));
		}

		AppearanceInfo(const AppearanceInfo& _o)
		{
			memcpy(attribute, _o.attribute, sizeof(attribute));
			todo_count = _o.todo_count.load();
			todo = _o.todo;
		}

		bool contains(size_t _attribute, Op _op) const { return contains(_attribute, O_LESS, _attribute, _op); }
		void insert(size_t _attribute, Op _op) { insert(_attribute, O_LESS, _attribute, _op); }

		bool contains(size_t _att1, Op _op1, size_t _att2, Op _op2) const
		{
			size_t idx = _att2 * (_att2 + 1) / 2 + _att1;
			uint16_t bit = 1 << (_op1 * 4 + _op2);
			assert(_att1 <= _att2 && idx < NUM_ATTRIBUTES);
			return (attribute[idx] & bit) != 0;
		}

		void insert(size_t _att1, Op _op1, size_t _att2, Op _op2)
		{
			size_t idx = _att2 * (_att2 + 1) / 2 + _att1;
			uint16_t bit = 1 << (_op1 * 4 + _op2);
			assert(_att1 <= _att2 && idx < NUM_ATTRIBUTES);
			attribute[idx] |= bit;
		}

		uint16_t opmask(size_t _att1, size_t _att2) const
		{
			size_t idx = _att2 * (_att2 + 1) / 2 + _att1;
			assert(_att1 <= _att2 && idx < NUM_ATTRIBUTES);
			return attribute[idx];
		}

		// operators are <, = and >
		uint16_t attribute[NUM_ATTRIBUTES];

		std::vector<Todo>	todo;
		std::atomic<size_t>	todo_count;
	};

	class SliceInfo
	{
	public:
		SliceInfo()
		{
			clear();
		}

		attr_t		min;
		attr_t		max;
		uint64_t	filter[8];

		void insert(attr_t _value)
		{
			if (_value < min) min = _value;
			if (_value > max) max = _value;

			setbit(_value);
			setbit(_value ^ (_value >> 32));
			setbit(_value + (_value >> 16) + (_value >> 32) + (_value >> 48));
		}

		bool contains(attr_t _value) const
		{
			bool set = _value <= max && _value >= min;
			set &= getbit(_value);
			set &= getbit(_value ^ (_value >> 32));
			set &= getbit(_value + (_value >> 16) + (_value >> 32) + (_value >> 48));
			return set;
		}

		void clear()
		{
			min = INT64_MAX;
			max = INT64_MIN;
			for (auto& it : filter)
				it = 0;
		}

	private:
		inline bool getbit(uint64_t _idx) const
		{
			_idx %= 64 * 8;
			return (filter[_idx / 64] & (1ull << (_idx % 64))) != 0;
		}

		inline void setbit(uint64_t _idx)
		{
			_idx %= 64 * 8;
			filter[_idx / 64] |= 1ull << _idx % 64;
		}
	};

	struct EventTypeSlice
	{
		EventTypeSlice()
		{
			clear();
		}

		uint64_t	firstEventId;
		uint64_t	lastEventId;
		SliceInfo	attribute[AppearanceInfo::MAX_ATTRIBUTES];

		void clear()
		{
			firstEventId = -1;
			lastEventId = 0;
			for (SliceInfo& s : attribute)
				s.clear();
		}

		bool empty() const
		{
			return firstEventId == -1;
		}
	};

public:
	PredicateMiner(const QueryLoader& _queryLoader, const Query& _query);
	~PredicateMiner();

	void initList(size_t _listIdx, size_t _eventType);
	void initWorkerThreads(size_t _numThreads);

	void addEvent(uint32_t _type, const attr_t* _attributes);
	void addMatch(uint32_t _event, uint32_t _eventEnd, size_t _listIdx);
	void flushMatch();

	void removeTimeouts(attr_t _timestamp);

	std::vector<PredicateInfo> generateResult(size_t _posList, size_t _negList) const;
	void printResult(size_t _posList, size_t _negList) const;

	Query buildPredicateQuery(const QueryLoader& _queryLoader, const Query& _src, uint32_t _afterEventIdx, const PredicateInfo& _predicate) const;

private:
	static bool compareAttributes(attr_t _a1, attr_t _a2, Op _op)
	{
		switch (_op)
		{
		case O_LESS: return _a1 < _a2;
		case O_EQUAL: return _a1 == _a2;
		case O_GREATER: return _a1 > _a2;
		default:;
		}
		return false;
	}

	bool processTask();
	void processTask(const MiningTask* _task, size_t _slice);
	void taskThread();

	bool searchSingleAttribute(const AppearanceInfo::Todo& _item, const EventItem& _event, const EventTypeSlice& _slice) const;
	bool searchMultiAttribute(const AppearanceInfo::Todo& _item, const EventItem& _event, const EventTypeSlice& _slice) const;

	void loadEventMapping(const QueryLoader& _queryLoader);
	EventItem& eventById(uint64_t _id) { return m_EventBuffer[_id - m_EventBuffer.front().id]; }

	attr_t							m_EventTimeout;
	std::vector<uint8_t>			m_NumAttributes;

	EventTypeSlice& slice(attr_t _slice, size_t _event) { return m_Slice[_event + (_slice % m_NumSlices) * m_EventMapping.size()]; }
	uint64_t						m_CurrentSlice;
	uint32_t						m_NumSlices;
	attr_t							m_SliceTime;
	std::vector<EventTypeSlice>		m_Slice;

	std::vector<EventInfo>			m_EventMapping;
	std::deque<EventItem>			m_EventBuffer;

	std::vector<AppearanceInfo>		m_Appearance[8];

	std::atomic<MiningTask*>		m_TaskList;
	std::atomic<int>				m_TaskToFetch;
	std::atomic<int>				m_TaskToFinish;
	std::vector<std::thread>		m_TaskThread;
	std::mutex						m_TaskMutex;
	std::condition_variable			m_TaskCondition;
	volatile bool					m_ExitTaskThreads;
};
