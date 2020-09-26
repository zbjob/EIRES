#include "../Query.h"
#include "../EventStream.h"

#include "../freegetopt/getopt.h"

#include <vector>
#include <algorithm>
#include <assert.h>

using namespace std;

int main(int _argc, char* _argv[])
{
	const char* deffile = "default.eql";

	int c;
	while ((c = getopt(_argc, _argv, "c:")) != -1)
	{
		switch (c)
		{
		case 'c':
			deffile = optarg;
			break;
		default:
			abort();
		}
	}

	QueryLoader def;
	if (!def.loadFile(deffile))
	{
		fprintf(stderr, "failed to load definition file %s\n", deffile);
		return 1;
	}

	vector<string> col;
	for (size_t i = 0; i < def.numEventDecls(); ++i)
	{
		for (const string& attr : def.eventDecl(i)->attributes)
		{
			if (find(col.begin(), col.end(), attr) == col.end())
				col.push_back(attr);
		}
	}

	sort(col.begin(), col.end());
	const char* definedCol[] = { "_type", "_flags", "_timestamp", "_timeoutState" };
	col.insert(col.begin(), definedCol, definedCol + sizeof(definedCol) / sizeof(definedCol[0]));

	StreamEvent::setupStdIo(stdin);

	for (const string& title : col)
		printf("%s%s", title.c_str(), title == col.back() ? "" : ",");
	putchar('\n');

	StreamEvent event;
	while (event.read())
	{
		const EventDecl* decl = def.eventDecl(event.typeIndex);

		printf("%s,%i,", decl->name.c_str(), (int)event.flags);

		if (event.flags & StreamEvent::F_TIMESTAMP)
			printf("%llu,", event.attributes[event.attributeCount - 1]);
		else
			putchar(',');

		if (event.flags & StreamEvent::F_TIMEOUT)
			printf("%i,", (int)event.timeoutState);
		else
			putchar(',');

		for (size_t i = 1; i < col.size(); ++i)
		{
			vector<string>::const_iterator it = find(decl->attributes.begin(), decl->attributes.end(), col[i]);
			if (it != decl->attributes.end())
			{
				printf("%llu", event.attributes[it - decl->attributes.begin()]);
			}
			
			putchar(i + 1 != col.size() ? ',' : '\n');
		}
	}

	return 0;
}


