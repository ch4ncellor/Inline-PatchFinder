#pragma once

#include "../Miscellaneous/Dependancies.h"

class C_Utilities
{
public:

	HANDLE TargetProcess; // for target process
	DWORD  TargetId;      // for target process

private:
	HANDLE GetProcess(const char* m_szProcessName);
public:

	bool SetupDesiredProcess(const char* m_szProcessName);
	
	struct LoadedModuleData_t
	{
		std::string m_szModuleName;
		std::string m_szModulePath;
		DWORD m_ModuleSize;
		BYTE *m_ModuleBaseAddress;
	};

	std::vector<LoadedModuleData_t> m_OutModules;

	DWORD m_dSavedExportVirtualAddress = NULL;
	bool EnumerateModulesInProcess();
}; inline C_Utilities g_Utilities;