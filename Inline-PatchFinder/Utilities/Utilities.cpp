#include "Utilities.h"
#include <WinTrust.h>

HANDLE C_Utilities::GetProcess(int m_nProcessID)
{
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 entry = {0};
	entry.dwSize = sizeof(entry);

	do 
	{
		if (m_nProcessID == entry.th32ProcessID)
		{
			TargetId = entry.th32ProcessID;
			CloseHandle(handle);
			TargetProcess = OpenProcess(PROCESS_ALL_ACCESS, false, TargetId);
			return TargetProcess;
		}
	} while (Process32NextW(handle, &entry));

	return {};
}

bool C_Utilities::EnumerateModulesInProcess()
{
	HANDLE hmodule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, TargetId);
	MODULEENTRY32 mEntry;
	mEntry.dwSize = sizeof(mEntry);

	static bool m_bIsInitialProcessModule = false;

	do
	{
		_bstr_t szEntryExePath(mEntry.szModule);
		std::string m_szEntryExePath = std::string(szEntryExePath);

		WCHAR szModName[MAX_PATH];
		if (K32GetModuleFileNameExW(this->TargetProcess, mEntry.hModule, szModName, sizeof(szModName) / sizeof(WCHAR)))
		{
			std::wstring ws(szModName);
			std::string _str(ws.begin(), ws.end());

			if (strstr(_str.c_str(), "WINDOWS"))
				// VERY stupid and ghetto way to check if this module was a core windows one.
			{
				this->m_OutModules.push_back({ m_szEntryExePath, _str, mEntry.modBaseSize, mEntry.modBaseAddr });
			}

		}
	} while (Module32NextW(hmodule, &mEntry));

	CloseHandle(hmodule);

	return this->m_OutModules.size() != 0;
}


bool C_Utilities::SetupDesiredProcess(int m_nProcessID)
{
	const HANDLE m_hProcessHandle = this->GetProcess(m_nProcessID);
	return m_hProcessHandle && m_hProcessHandle != INVALID_HANDLE_VALUE;
}

