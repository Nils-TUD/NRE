/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

namespace nul {

/**
 * Producer with fixed items.
 */
template <typename T, unsigned SIZE>
class Producer
{
protected:
  Consumer<T, SIZE> *_consumer;
  KernelSemaphore    _sem;
  bool               _dropping;

public:
  /**
   * Put something in the buffer. Please note that this function is
   * not locked, thus only a single producer should do the access at
   * the very same time.
   */
  bool produce(T &value)
  {
    if (!_consumer || ((_consumer->_wpos + 1) % SIZE == _consumer->_rpos))
      {
        _dropping = true;
        return false;
      }
    _dropping = false;
    _consumer->_buffer[_consumer->_wpos] = value;
    _consumer->_wpos = (_consumer->_wpos + 1) % SIZE;
    MEMORY_BARRIER;
    if (_sem.up(false)) Logging::printf("  : producer issue - wake up failed\n");
    return true;
  }

  unsigned sm() { return _sem.sm(); }

  Producer(Consumer<T, SIZE> *consumer = 0, unsigned nq = 0) : _consumer(consumer), _sem(nq) {};
};

}
