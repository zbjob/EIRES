#include "../EventStream.h"
#include <iostream>
#include <map>
#include <list>
#include <algorithm>

using namespace std;

static bool compareEvent(const StreamEvent& _a, const StreamEvent& _b)
{
	for (size_t i = 0; i < _a.attributeCount && i < _b.attributeCount; ++i)
	{
		if (_a.attributes[i] < _b.attributes[i])
			return true;
		else if (_a.attributes[i] > _b.attributes[i])
			return false;
	}
	return false;
}

int main(int _argc, char* _argv[])
{
	StreamEvent::setupStdIo();

	bool removeRevokes = false;

	if (_argc > 1 && !strcmp(_argv[1], "-d"))
		removeRevokes = true;

	list<StreamEvent> queue;

	StreamEvent event;
	while(event.read())
	{
		if (removeRevokes && (event.flags & StreamEvent::F_REVOKE))
		{
			event.flags &= ~StreamEvent::F_REVOKE;
			for (auto it = queue.rbegin(); it != queue.rend(); ++it)
			{
				if (*it == event)
				{
					queue.erase(next(it).base());
					break;
				}
			}
		}
		else
		{
			if (queue.empty() || queue.back().attributes[0] < event.attributes[0])
			{
				queue.push_back(event);
			}
			else
			{
				for (auto it = queue.rbegin(); it != queue.rend(); ++it)
				{
					if (compareEvent(*it, event))
					{
						queue.insert(it.base(), event);
						break;
					}
				}
			}

			if (queue.size() > 1000000)
			{
				queue.front().write();
				queue.pop_front();
			}
		}
	}

	for (auto& it : queue)
		it.write();

	return 0;
}
