typedef unsigned long size_t;

extern "C" {
void* malloc(size_t size);
void free(void* p);
}

static char buffer[4096];
static size_t pos = 0;

void* malloc(size_t size) {
	void* res;
	if(pos + size >= sizeof(buffer))
		return 0;
	
	res = buffer + pos;
	pos += size;
	return res;
}

void free(void*) {
	// do nothing for now
}

