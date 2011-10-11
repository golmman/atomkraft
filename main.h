/*
 * main.h
 *
 *  Created on: 25.07.2011
 *      Author: golmman
 * 
 * Note to myself
 * --------------
 * 
 * How to compile with eclipse cdt on windows xp?
 * 
 * First get the pthread libraries from http://sourceware.org/pthreads-win32/:
 * Put the three *.h files into the include directories, 
 * download libpthreadGCE2.a (MinGW) and pthreadVCE2.lib (MSVC) rename them 
 * to libpthread.a and pthread.lib respectively put them in the lib directories.
 * Download pthreadGCE2.dll and pthreadVCE2.dll and put them 
 * into the WINDOWS\system32 directory.
 * To run the atomkraft.exe you will need mingwm10.dll and pthreadGCE2.dll if 
 * built with MinGW and msvcp100.dll, msvcr100.dll, pthreadVCE2.dll if built with MSVC.
 * 
 * using MinGW toolchain:
 * 
 * Project > Properties > C/C++ Build > Settings > GCC C++ Compiler > Preprocessor > Defined symbols:
 * -DNDEBUG -DOLD_LOCKS -DUSE_PREFETCH (see types.h for description)
 * 
 * Project > Properties > C/C++ Build > Settings > GCC C++ Compiler > Miscellaneous > other flags:
 * -msse -fno-rtti -march=pentium4
 * 
 * Project > Properties > C/C++ Build > Settings > MinGW C++ Linker > Libraries > Libraries:
 * pthread, ws2_32, mswsock, advapi32
 * 
 * using MSVC:
 * 
 * First install the Visual C++ 2010 Redistributable Package (x86 or x64)
 * 
 * Create a project from existing code in MSVC Express. Don't use the
 * -msse -fno-rtti -march=pentium4 options but all the others.
 * Set some optimization flags if necessary. Build once, now you know it is working so
 * copy the atomkraft.sln created by the ide and paste them to the home directory 
 * of the atomkraft workspace. Create a new build configuration and set
 * Project > Properties > C/C++ Build > Tool Chain Editor > Current Builder to
 * something different than the internal builder. Now go to
 * Project > Properties > C/C++ Build and set the Build command to
 * msbuild ..\src\atomkraft.sln /p:configuration="Release" /p:OutDir="..\MSVC\\" /target:Clean;Build
 * or
 * msbuild ..\src\atomkraft.sln /p:configuration="Release64" /p:Platform="x64" /p:OutDir="..\MSVC64\\" /target:Clean;Build
 * for 64 bit built.
 * In the same view switch to the tab Behavior and delete the text field
 * Build (incremental build) which is usually set to "all". If the program is now built
 * it creates the atomkraft.exe in src\Release
 *   
 *   
 *   
 */



#ifndef MAIN_H_
#define MAIN_H_

// choose one version
// it effects the functionality and the console output



//#define UCI_VERSION		// intended to run on a common gui like arena, uci output
//#define FICS_VERSION		// fics client version, minimal output
//#define TEST_VERSION		// testing and debug version, maximal output, reduced uci output
//#define BOOK_VERSION		// automated book creation version
#define ANALYZE_VERSION	// position analysis tool, mainly used for manual book creation


//#define SWEN_VERSION



#endif /* MAIN_H_ */



















