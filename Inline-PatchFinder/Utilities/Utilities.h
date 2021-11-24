#pragma once

#include "../Miscellaneous/Dependancies.h"

class C_Utilities
{
public:

	HANDLE TargetProcess; // for target process
	DWORD  TargetId;      // for target process
private:
	HANDLE GetProcess(int m_nProcessID);
public:

	bool SetupDesiredProcess(int m_nProcessID);
	
	bool HasSpecialCharacters(const char* str)
	{
		return strlen(str) < 4 || str[strspn(str, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_")] != 0;
	}

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