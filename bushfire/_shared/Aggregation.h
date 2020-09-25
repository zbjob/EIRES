#pragma once
#include "../query/PatternMatcher.h"

class AggregationAvg : public PatternMatcher::AggregationFunction
{
public:
	virtual void clear()
	{
		m_Sum = attr_e((attr_t)0);
		m_Count = 0;
	}

	virtual attr_e push(attr_e _attr)
	{
		m_Sum = m_Sum + _attr.i;
		attr_e r = m_Sum / attr_e((attr_t)m_Count);
		m_Count++;
		return r;
	}

	virtual void pop(attr_e _attr)
	{
		m_Sum = m_Sum - _attr;
		m_Count--;
	}

private:
	attr_e		m_Sum;
	unsigned	m_Count;
};

class AggregationCount : public PatternMatcher::AggregationFunction
{
public:
	virtual void clear()
	{
		m_Count = 0;
	}

	virtual attr_e push(attr_e _attr)
	{
		return (int64_t)++m_Count;
	}

	virtual void pop(attr_e _attr)
	{
		m_Count--;
	}

private:
	unsigned	m_Count;
};
