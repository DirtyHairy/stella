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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef SETTINGS_DB_HXX
#define SETTINGS_DB_HXX

#include "bspf.hxx"
#include "SqliteDatabase.hxx"
#include "repository/KeyValueRepository.hxx"
#include "repository/CompositeKeyValueRepository.hxx"

class SettingsDb
{
  public:

    SettingsDb(const string& databaseDirectory, const string& databaseName);

    void initialize();

    KeyValueRepository& settingsRepository() const { return *mySettingsRepository; }

    CompositeKeyValueRepository& propertyRepository() const { return *myPropertyRepository; }

    const string& databaseFileName() const { return myDatabaseName; }

  private:

    string myDatabaseDirectory;
    string myDatabaseName;

    shared_ptr<SqliteDatabase> myDb;
    unique_ptr<KeyValueRepositoryAtomic> mySettingsRepository;
    unique_ptr<KeyValueRepositoryAtomic> myPropertyRepositoryHost;
    unique_ptr<CompositeKeyValueRepository> myPropertyRepository;
};

#endif // SETTINGS_DB_HXX
