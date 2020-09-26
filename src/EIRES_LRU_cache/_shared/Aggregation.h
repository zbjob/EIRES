#pragma once
#include "../PatternMatcher.h"

class AggregationAvg : public PatternMatcher::AggregationFunction
{
public:
	virtual void clear()
	{
		m_Sum = 0;
		m_Count = 0;
	}

	virtual attr_t push(attr_t _attr)
	{
		return (m_Sum += _attr) / ++m_Count;
	}

	virtual void pop(attr_t _attr)
	{
		m_Sum -= _attr;
		m_Count--;
	}

private:
	attr_t		m_Sum;
	unsigned	m_Count;
};

class AggregationCount : public PatternMatcher::AggregationFunction
{
public:
	virtual void clear()
	{
		m_Count = 0;
	}

	virtual attr_t push(attr_t _attr)
	{
		return ++m_Count;
	}

	virtual void pop(attr_t _attr)
	{
		m_Count--;
	}

private:
	unsigned	m_Count;
};
