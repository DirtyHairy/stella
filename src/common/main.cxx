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

#include <cstdlib>

#include "bspf.hxx"
#include "MediaFactory.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "PropsSet.hxx"
#include "Sound.hxx"
#include "Settings.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"
#include "PNGLibrary.hxx"
#include "System.hxx"
#include "TIASurface.hxx"

#include "ThreadDebugging.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

#ifdef CHEATCODE_SUPPORT
  #include "CheatManager.hxx"
#endif

/**
  Parse the commandline arguments and store into the appropriate hashmap.

  Keys without a corresponding value are assumed to be boolean, and set to true.
  Some keys are used only by the main function; these are placed in localOpts.
  The rest are needed globally, and are placed in globalOpts.
*/
void parseCommandLine(int ac, char* av[],
    Settings::Options& globalOpts, Settings::Options& localOpts);

/**
  Checks the commandline for special settings that are used by various ports
  to use a specific 'base directory'.

  This needs to be done separately, before either an OSystem or Settings
  object can be created, since they both depend on each other, and a
  variable basedir implies a different location for the settings file.

  This function will call OSystem::overrideBaseDir() when either of the
  applicable arguments are found, and then remove them from the argument
  list.
*/
void checkForCustomBaseDir(Settings::Options& options);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void parseCommandLine(int ac, char* av[],
    Settings::Options& globalOpts, Settings::Options& localOpts)
{
  localOpts["ROMFILE"] = "";  // make sure we have an entry for this

  for(int i = 1; i < ac; ++i)
  {
    string key = av[i];
    if(key[0] == '-')
    {
      key = key.substr(1);

      // Certain options are used only in the main function
      if(key == "help" || key == "listrominfo" || key == "rominfo" || key == "takesnapshot")
      {
        localOpts[key] = true;
        continue;
      }
      // Take care of arguments without an option that are needed globally
      if(key == "debug" || key == "holdselect" || key == "holdreset")
      {
        globalOpts[key] = true;
        continue;
      }
      // Some ports have the ability to override the base directory where all
      // configuration files are stored; we check for those next
      if(key == "baseinappdir")
      {
        localOpts[key] = true;
        continue;
      }

      if(++i >= ac)
      {
        cerr << "Missing argument for '" << key << "'" << endl;
        continue;
      }
      if(key == "basedir" || key == "break")
        localOpts[key] = av[i];
      else
        globalOpts[key] = av[i];
    }
    else
      localOpts["ROMFILE"] = key;
  }

#if 0
  cout << "Global opts:" << endl;
  for(const auto& x: globalOpts)
    cout << " -> " << x.first << ": " << x.second << endl;
  cout << "Local opts:" << endl;
  for(const auto& x: localOpts)
    cout << " -> " << x.first << ": " << x.second << endl;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void checkForCustomBaseDir(Settings::Options& options)
{
  // If both of these are activated, the 'base in app dir' takes precedence
  auto it = options.find("baseinappdir");
  if(it != options.end())
    OSystem::overrideBaseDirWithApp();
  else
  {
    it = options.find("basedir");
    if(it != options.end())
      OSystem::overrideBaseDir(it->second.toString());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(BSPF_MACOS)
int stellaMain(int ac, char* av[])
#else
int main(int ac, char* av[])
#endif
{
  SET_MAIN_THREAD;

  std::ios_base::sync_with_stdio(false);

  unique_ptr<OSystem> theOSystem;

  auto Cleanup = [&theOSystem]() {
    if(theOSystem)
    {
      theOSystem->logMessage("Cleanup from main", 2);
      theOSystem->saveConfig();
      theOSystem.reset();     // Force delete of object
    }
    MediaFactory::cleanUp();  // Finish any remaining cleanup

    return 0;
  };

  // Parse the commandline arguments
  // They are placed in different maps depending on whether they're used
  // locally or globally
  Settings::Options globalOpts, localOpts;
  parseCommandLine(ac, av, globalOpts, localOpts);

  // Check for custom base directory; some ports make use of this
  checkForCustomBaseDir(localOpts);

  // Create the parent OSystem object and initialize settings
  theOSystem = MediaFactory::createOSystem();
  theOSystem->loadConfig(globalOpts);

  // Create the full OSystem after the settings, since settings are
  // probably needed for defaults
  theOSystem->logMessage("Creating the OSystem ...", 2);
  if(!theOSystem->create())
  {
    theOSystem->logMessage("ERROR: Couldn't create OSystem", 0);
    return Cleanup();
  }

  // Check to see if the user requested info about a specific ROM,
  // or the list of internal ROMs
  // If so, show the information and immediately exit
  string romfile = localOpts["ROMFILE"].toString();
  if(localOpts["listrominfo"].toBool())
  {
    theOSystem->logMessage("Showing output from 'listrominfo' ...", 2);
    theOSystem->propSet().print();
    return Cleanup();
  }
  else if(localOpts["rominfo"].toBool())
  {
    theOSystem->logMessage("Showing output from 'rominfo' ...", 2);
    FilesystemNode romnode(romfile);
    theOSystem->logMessage(theOSystem->getROMInfo(romnode), 0);

    return Cleanup();
  }
  else if(localOpts["help"].toBool())
  {
    theOSystem->logMessage("Displaying usage", 2);
    theOSystem->settings().usage();
    return Cleanup();
  }

  //// Main loop ////
  // First we check if a ROM is specified on the commandline.  If so, and if
  //   the ROM actually exists, use it to create a new console.
  // Next we check if a directory is specified on the commandline.  If so,
  //   open the rom launcher in that directory.
  // If not, use the built-in ROM launcher.  In this case, we enter 'launcher'
  //   mode and let the main event loop take care of opening a new console/ROM.
  FilesystemNode romnode(romfile);
  if(romfile == "" || romnode.isDirectory())
  {
    theOSystem->logMessage("Attempting to use ROM launcher ...", 2);
    bool launcherOpened = romfile != "" ?
      theOSystem->createLauncher(romnode.getPath()) : theOSystem->createLauncher();
    if(!launcherOpened)
    {
      theOSystem->logMessage("Launcher could not be started, showing usage", 2);
      theOSystem->settings().usage();
      return Cleanup();
    }
  }
  else
  {
    try
    {
      const string& result = theOSystem->createConsole(romnode);
      if(result != EmptyString)
        return Cleanup();

#if 0
      TODO: Fix this to use functionality from OSystem::mainLoop
      if(localOpts["takesnapshot"].toBool())
      {
        for(int i = 0; i < 30; ++i)  theOSystem->frameBuffer().update();
//        theOSystem->frameBuffer().tiaSurface().saveSnapShot();
        theOSystem->png().takeSnapshot();
        return Cleanup();
      }
#endif
    }
    catch(const runtime_error& e)
    {
      theOSystem->logMessage(e.what(), 0);
      return Cleanup();
    }

#ifdef DEBUGGER_SUPPORT
    // Set up any breakpoint that was on the command line
    if(localOpts["break"].toString() != "")
    {
      Debugger& dbg = theOSystem->debugger();
      uInt16 bp = uInt16(dbg.stringToValue(localOpts["break"].toString()));
      dbg.setBreakPoint(bp, true);
    }
#endif
  }

  // Start the main loop, and don't exit until the user issues a QUIT command
  theOSystem->logMessage("Starting main loop ...", 2);
  theOSystem->mainLoop();
  theOSystem->logMessage("Finished main loop ...", 2);

  // Cleanup time ...
  return Cleanup();
}
