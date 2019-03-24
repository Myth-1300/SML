// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "Mod.h"
#include "DLLMain.h"
#include "Config.h"
#include "DetoredEvents.h"
#include <string>
#include <iostream>
#include <filesystem>
#include <Windows.h>
#include <tuple>

// Main DLL for loading mod DLLs
void ModLoaderEntry() {

	// Allocate console
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);

	log("Attached SatisfactoryModLoader to Satisfactory");

	readConfig(); // read the config file

	if (!LOADCONSOLE) { // destroy the console if stated by the config file
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	}

	// get application path
	char p[MAX_PATH];
	GetModuleFileNameA(NULL, p, MAX_PATH);

	// split the path
	std::string appPath(p);
	size_t pos = appPath.find_last_of('\\');
	std::string path = appPath.substr(0, pos) + "\\mods";
	std::string pathExact = path + "\\";

	log("Looking for mods in: " + path);

	// iterate through the directory to find mods
	for (const auto & entry : std::experimental::filesystem::directory_iterator(path)) {
		if (entry.path().extension().string() == ".dll") { // check if the file has a .dll extension
			std::string file = pathExact + entry.path().filename().string();
			std::wstring stemp = std::wstring(file.begin(), file.end());

			log("Attempting to load mod: " + file);

			LPCWSTR sw = stemp.c_str();

			HMODULE dll = LoadLibrary(sw); // load the dll

			FARPROC valid = GetProcAddress(dll, "isMod");
			if (!valid) { //check validity of the mod dll
				FreeLibrary(dll);
				continue;
			}

			std::tuple<bool, std::string> modName = getFieldValue<std::string>(dll, "ModName");
			std::tuple<bool, std::string> modVersion = getFieldValue<std::string>(dll, "ModVersion");
			std::tuple<bool, std::string> modDescription = getFieldValue<std::string>(dll, "ModDescription");
			std::tuple<bool, std::string> modAuthors = getFieldValue<std::string>(dll, "ModAuthors");
			
			if (!std::get<0>(modName) || !std::get<0>(modVersion) || !std::get<0>(modDescription) || !std::get<0>(modAuthors)) {
				FreeLibrary(dll);
				continue;
			}

			//check if the mod has already been loaded
			bool isDuplicate = false;
			for (Mod existingMod : ModList) {
				if (existingMod.name == std::get<1>(modName)) {
					log("Skipping duplicate mod " + existingMod.name);
					FreeLibrary(dll);
					isDuplicate = true;
					break;
				}
			}

			if (isDuplicate) {
				continue;
			}

			// if valid, initalize a mod struct and add it to the modlist
			Mod mod = {
				sw,
				dll,
				std::get<1>(modName),
				std::get<1>(modVersion),
				std::get<1>(modDescription),
				std::get<1>(modAuthors),
			};

			ModList.push_back(mod);

			log("[Name] " + mod.name, false);
			log(" [Version] " + mod.version, false, false);
			log(" [Description] " + mod.description, false, false);
			log(" [Authors] " + mod.authors, true, false);
		}
	}

	size_t listSize = ModList.size();
	log("Loaded " + std::to_string(listSize), false);
	if (listSize > 1) {
		log(" mods", true, false);
	}
	else {
		log(" mod", true, false);
	}

	// run test event
	run_event(Event::Test);

	// assign test event
	hook_event(HookEvent::OnPickupFoliage, UFGFoliageLibrary_CheckInventorySpaceAndGetStacks);

	log("SatisfactoryModLoader Initialization complete. Launching Satisfactory...");
}

template<typename T>
void log(T msg, bool endLine, bool showHeader) {
	if (showHeader) {
		std::cout << "[SML] ";
	}
	std::cout << msg;
	if (endLine) {
		std::cout << std::endl;
	}
}

template <typename O>
std::tuple<bool, O> getFunctionValue(HMODULE module, const char* procName) {
	FARPROC proc = GetProcAddress(module, procName);
	if (!proc) {
		return std::make_tuple(false, "");
	}

	typedef O TFUNC();
	TFUNC* f = (TFUNC*)proc;

	return std::make_tuple(true, f());
}

FARPROC getFunction(HMODULE module, const char* procName) {
	FARPROC proc = GetProcAddress(module, procName);
	if (!proc) {
		return NULL;
	}

	return proc;
}

template <typename O>
std::tuple<bool, O> getFieldValue(HMODULE module, const char* procName) {
	FARPROC proc = getFunction(module, procName);
	if (!proc) {
		return std::make_tuple(false, "");
	}

	typedef O (funcN);
	funcN* n1 = (funcN*)proc;
	return std::make_tuple(true, *n1);
}

void runEvent(Event event) {
	for (Mod mod : ModList) {
		FARPROC func = getFunction(mod.fileModule, "GetTickEvent");

		typedef FUNC GETFUNC(Event);
		GETFUNC* f = (GETFUNC*)func;
		FUNC returnFunc = (*f)(event);
		if (returnFunc != NULL) {
			returnFunc();
		}
	}
}

void runPreInit() {
	std::cout << "[SML|Event] Pre Initializing!" << std::endl;
	return;
}

void runPostInit() {
	std::cout << "[SML|Event] Pre Initializing!" << std::endl;
	return;
}