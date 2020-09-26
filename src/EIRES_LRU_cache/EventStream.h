#pragma once
#include "Query.h"
#include "_shared/MurmurHash3.h"

#include <inttypes.h>
#include <stdio.h>
#include <fcntl.h>

#ifdef _WIN32
# include <io.h>
#endif

struct StreamEvent
{
	enum Flags
	{
		F_TIMEOUT		= 0x1,
		F_TIMESTAMP		= 0x2,
		F_REVOKE		= 0x4,
	};

	StreamEvent() : head(0), offset(0)
	{}

	union {
		struct {
			uint16_t	typeIndex;
			uint16_t	typeHash;
			uint8_t		attributeCount;
			uint8_t		flags;
			uint8_t		timeoutState;
			uint8_t		reserved;
		};
		
		uint64_t		head;
	};

	uint64_t			attributes[EventDecl::MAX_ATTRIBUTES];
	mutable uint64_t	offset;

	bool operator < (const StreamEvent& _o) const { return attributes[0] < _o.attributes[0]; }

	bool operator == (const StreamEvent& _o) const {
		if (head == _o.head)
		{
			for (size_t i = 0; i < attributeCount; ++i)
				if (attributes[i] != _o.attributes[i])
					return false;
			return true;
		}
		return false;
	}

	static void setupStdIo(FILE* _stream = NULL)
	{
		if (_stream)
		{
#ifdef _WIN32
			_setmode(_fileno(_stream), _O_BINARY);
#endif
		}
		else
		{
#ifdef _WIN32
			_setmode(_fileno(stdin), _O_BINARY);
			_setmode(_fileno(stdout), _O_BINARY);
#else
			freopen(NULL, "rb", stdin);
			freopen(NULL, "wb", stdout);
#endif
		}
	}

	static uint16_t hash(const std::string& _string)
	{
		uint32_t out;
		MurmurHash3_x86_32(_string.c_str(), (int)_string.length(), 0xDEADBEEFu, &out);
		return (uint16_t)out;
	}

	bool read(FILE* _file = stdin)
	{
		if (fread(&head, sizeof(head), 1, _file) == 1)
		{
			if (fread(attributes, sizeof(uint64_t), attributeCount, _file) == attributeCount)
			{
				offset += sizeof(uint64_t) * attributeCount + sizeof(head);
				return true;
			}
		}
		return false;
	}

	bool write(FILE* _file = stdout) const
	{
		offset += sizeof(uint64_t) * (1 + attributeCount);

		return	fwrite(&head, sizeof(head), 1, _file) == 1 &&
				fwrite(attributes, sizeof(uint64_t) * attributeCount, 1, _file) == 1;
	}

}; 
