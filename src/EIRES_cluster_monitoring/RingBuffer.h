#pragma once
#include <inttypes.h>
#include <assert.h>
#include <functional>

template<typename T>
class RingBuffer
{
public:
	RingBuffer(uint32_t _capacity = 1024 * 1024)
	{
		m_Capacity = _capacity;
		m_Begin = 0;
		m_End = 0;
		m_Ring = new T[m_Capacity];
	}

	~RingBuffer()
	{
		delete[] m_Ring;
	}

	void push_back(const T& _item)
	{
		if ((m_End + 1) % m_Capacity == m_Begin)
			resize();

		m_Ring[m_End] = _item;
		m_End = (m_End + 1) % m_Capacity;
	}

	bool empty() const { return m_Begin == m_End; }

	T& front() { return m_Ring[m_Begin]; }

	void pop_front()
	{
		m_Begin = (m_Begin + 1) % m_Capacity;
	}

	void iterate(std::function<void(T&)> _func)
	{
		if (m_Begin <= m_End)
		{
			for (T* it = m_Ring + m_Begin; it != m_Ring + m_End; it++)
				_func(*it);
		}
		else
		{
			for (T* it = m_Ring + m_Begin; it != m_Ring + m_Capacity; it++)
				_func(*it);
			for (T* it = m_Ring; it != m_Ring + m_End; it++)
				_func(*it);
		}
	}

	T& operator[] (uint32_t _idx)
	{
		return m_Ring[(m_Begin + _idx) % m_Capacity];
	}

protected:
	void resize()
	{
		T* ring = new T[m_Capacity * 2];
		T* it = ring;

		iterate([&](T& _item) {
			*it++ = _item;
		});
		assert(ring + m_Capacity - 1 == it);

		delete[] m_Ring;
		m_Begin = 0;
		m_End = (uint32_t)(it - ring);
		m_Capacity *= 2;
		m_Ring = ring;
	}

private:
	uint32_t	m_Capacity;
	uint32_t	m_Begin;
	uint32_t	m_End;

	T*			m_Ring;
};
