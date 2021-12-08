#include "../Query.h"
#include "../PatternMatcher.h"
#include "../EventStream.h"
#include "BatchWorker.h"

#include "../freegetopt/getopt.h"

#include <thread>
#include <vector>
#include <memory>
#include <assert.h>

using namespace std;

int main(int _argc, char* _argv[])
{
	const char* deffile = "default.eql";
	const char* queryName = 0;

	int c;
	while ((c = getopt(_argc, _argv, "c:q:")) != -1)
	{
		switch (c)
		{
		case 'c':
			deffile = optarg;
			break;
		case 'q':
			queryName = optarg;
			break;
		default:
			abort();
		}
	}


	// set input/output stream to binary
	StreamEvent::setupStdIo();

	// load eql file
	QueryLoader def;
	if (!def.loadFile(deffile))
	{
		fprintf(stderr, "failed to load definition file %s\n", deffile);
		return false;
	}

	// select query of eql file
	const Query* query = !queryName ? def.query((size_t)0) : def.query(queryName);
	if (!query)
	{
		fprintf(stderr, "query not found");
		return false;
	}

	// add one thread for IO
	unsigned int numThreads = std::thread::hardware_concurrency() + 1;

	BatchWorker* worker = new BatchWorker[numThreads];
	for (unsigned int i = 0; i < numThreads; ++i)
	{
		QueryLoader::Callbacks cb;
		// TODO: setup callbacks for matching events here

		unique_ptr<PatternMatcher> matcher(new PatternMatcher);
		def.setupPatternMatcher(query, *matcher, cb);

		// TODO: modify pattern matcher to reflect the new type of query
		
		worker[i].setPatternMatcher(move(matcher));
		worker[i].start();
	}

	// wait for all threads to finish
	delete[] worker;
	return 0;
}
