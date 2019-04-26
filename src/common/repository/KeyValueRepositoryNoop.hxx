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

#ifndef KEY_VALUE_REPOSITORY_NOOP_HXX
#define KEY_VALUE_REPOSITORY_NOOP_HXX

#include "KeyValueRepository.hxx"

class KeyValueRepositoryNoop : public KeyValueRepository
{
  public:

    virtual std::map<string, Variant> load() {
      return std::map<string, Variant>();
    }

    virtual void save(const std::map<string, Variant>& values) {}
};

#endif // KEY_VALUE_REPOSITORY_NOOP_HXX