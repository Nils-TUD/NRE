/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <service/Service.h>
#include <service/Consumer.h>
#include <stream/Serial.h>
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>

using namespace nul;

class LogService;

static void receiver(void *s);

static UserSm *serial_sm;
static LogService *log;
static cpu_t next_cpu = 1;
static size_t cpu_count;

class LogSessionData : public SessionData {
public:
	LogSessionData(Service *s,capsel_t caps,Pt::portal_func func)
		: SessionData(s,caps,func), _id(_next_id++), _bufpos(0), _buf(),
		  _ec(receiver,Atomic::xadd(&next_cpu,+1) % cpu_count), _sc(), _cons() {
		_ec.set_tls<capsel_t>(Ec::TLS_PARAM,caps);
	}
	virtual ~LogSessionData() {
		delete _cons;
		delete _sc;
	}

	virtual void invalidate() {
		_cons->stop();
	}

	int id() const {
		return _id;
	}
	const char *buffer() const {
		return _buf;
	}
	size_t size() const {
		return _bufpos;
	}

	Consumer<char> *cons() {
		return _cons;
	}

	bool put(char c) {
		if(_bufpos < sizeof(_buf))
			_buf[_bufpos++] = c;
		return c == '\n' || _bufpos == sizeof(_buf);
	}
	void flush();

protected:
	virtual void set_ds(DataSpace *ds) {
		SessionData::set_ds(ds);
		_cons = new Consumer<char>(ds);
		_sc = new Sc(&_ec,Qpd());
	}

private:
	int _id;
	size_t _bufpos;
	char _buf[80];
	GlobalEc _ec;
	Sc *_sc;
	Consumer<char> *_cons;
	static int _next_id;
};

class LogService : public Service {
public:
	LogService() : Service("log",0) {
	}

private:
	virtual SessionData *create_session(capsel_t caps,Pt::portal_func func) {
		return new LogSessionData(this,caps,func);
	}
};

int LogSessionData::_next_id = 0;

void LogSessionData::flush() {
	ScopedLock<UserSm> guard(serial_sm);
	Serial::get().writef("[%d] ",_id);
	for(size_t i = 0; i < _bufpos; ++i)
		Serial::get().write(_buf[i]);
	_bufpos = 0;
}

static void receiver(void *) {
	capsel_t caps = Ec::current()->get_tls<capsel_t>(Ec::TLS_PARAM);
	while(1) {
		ScopedLock<RCULock> guard(&RCU::lock());
		LogSessionData *sess = log->get_session<LogSessionData>(caps);
		Consumer<char> *cons = sess->cons();
		char *c = cons->get();
		if(c == 0)
			return;
		if(sess->put(*c))
			sess->flush();
		cons->next();
	}
}

int main() {
	cpu_count = Hip::get().cpu_online_count();
	serial_sm = new UserSm();
	Serial::get().init(false);

	log = new LogService();
	const Hip& hip = Hip::get();
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			log->provide_on(it->id());
	}
	log->reg();
	log->wait();
	return 0;
}
