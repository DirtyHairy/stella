//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "AudioQueue.hxx"

using std::mutex;
using std::lock_guard;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioQueue::AudioQueue(uInt32 fragmentSize, uInt32 capacity, bool isStereo, StaggeredLogger::Logger logger)
  : myFragmentSize(fragmentSize),
    myIsStereo(isStereo),
    myFragmentQueue(capacity),
    myAllFragments(capacity + 2),
    mySize(0),
    myNextFragment(0),
    myIgnoreOverflows(true),
    myOverflowLogger("audio buffer overflow", logger)
{
  const uInt8 sampleSize = myIsStereo ? 2 : 1;

  myFragmentBuffer = make_unique<Int16[]>(myFragmentSize * sampleSize * (capacity + 2));

  for (uInt32 i = 0; i < capacity; ++i)
    myFragmentQueue[i] = myAllFragments[i] = myFragmentBuffer.get() + i * sampleSize * myFragmentSize;

  myAllFragments[capacity] = myFirstFragmentForEnqueue =
    myFragmentBuffer.get() + capacity * sampleSize * myFragmentSize;

  myAllFragments[capacity + 1] = myFirstFragmentForDequeue =
    myFragmentBuffer.get() + (capacity + 1) * sampleSize * myFragmentSize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 AudioQueue::capacity() const
{
  return uInt32(myFragmentQueue.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 AudioQueue::size()
{
  lock_guard<mutex> guard(myMutex);

  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioQueue::isStereo() const
{
  return myIsStereo;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 AudioQueue::fragmentSize() const
{
  return myFragmentSize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int16* AudioQueue::enqueue(Int16* fragment)
{
  lock_guard<mutex> guard(myMutex);

  Int16* newFragment;

  if (!fragment) {
    if (!myFirstFragmentForEnqueue) throw runtime_error("enqueue called empty");

    newFragment = myFirstFragmentForEnqueue;
    myFirstFragmentForEnqueue = nullptr;

    return newFragment;
  }

  const uInt16 capacity = uInt16(myFragmentQueue.size());
  const uInt16 fragmentIndex = (myNextFragment + mySize) % capacity;

  newFragment = myFragmentQueue.at(fragmentIndex);
  myFragmentQueue.at(fragmentIndex) = fragment;

  if (mySize < capacity) ++mySize;
  else {
    myNextFragment = (myNextFragment + 1) % capacity;
    if (!myIgnoreOverflows) myOverflowLogger.log();
  }

  return newFragment;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int16* AudioQueue::dequeue(Int16* fragment)
{
  lock_guard<mutex> guard(myMutex);

  if (mySize == 0) return nullptr;

  if (!fragment) {
    if (!myFirstFragmentForDequeue) throw runtime_error("dequeue called empty");

    fragment = myFirstFragmentForDequeue;
    myFirstFragmentForDequeue = nullptr;
  }

  Int16* nextFragment = myFragmentQueue.at(myNextFragment);
  myFragmentQueue.at(myNextFragment) = fragment;

  --mySize;
  myNextFragment = (myNextFragment + 1) % myFragmentQueue.size();

  return nextFragment;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioQueue::closeSink(Int16* fragment)
{
  lock_guard<mutex> guard(myMutex);

  if (myFirstFragmentForDequeue && fragment)
    throw new runtime_error("attempt to return unknown buffer on closeSink");

  if (!myFirstFragmentForDequeue)
    myFirstFragmentForDequeue = fragment;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioQueue::ignoreOverflows(bool shouldIgnoreOverflows)
{
  myIgnoreOverflows = shouldIgnoreOverflows;
}
