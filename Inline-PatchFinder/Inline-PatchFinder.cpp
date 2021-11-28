#include "Miscellaneous/Dependancies.h"

int main()
{
    LOG("[+] Please enter the process ID of the desired process: ");
    int m_nProcessID = 0;
    std::cin >> m_nProcessID;

    LOG("\n\n");

    if (!g_Utilities.SetupDesiredProcess(m_nProcessID))
    {
        LOG("[!] Awaiting for desired process to open...\n");
        PAUSE_SYSTEM_CMD(false);

        if (!g_Utilities.SetupDesiredProcess(m_nProcessID))
        {
            LOG("[-] Couldn't find desired process...\n");
            PAUSE_SYSTEM_CMD(true);
        }
    }

    if (!g_Utilities.EnumerateModulesInProcess())
    {
        LOG("[-] Failed to enumerate modules in process...\n");
        PAUSE_SYSTEM_CMD(true);
    }

    for (auto& ModuleList : g_Utilities.m_OutModules)
    {
        if (ModuleList.m_szModulePath.empty() || ModuleList.m_ModuleBaseAddress <= 0x0)
            continue;

        BYTE m_ModulePEHeaders[0x10000];
        BOOL m_bRPMResult = ReadProcessMemory(g_Utilities.TargetProcess,
            reinterpret_cast<LPCVOID>(ModuleList.m_ModuleBaseAddress),
            &m_ModulePEHeaders,
            sizeof(m_ModulePEHeaders),
            NULL);

        if (!m_bRPMResult)
        {
       //     LOG("[-] Couldn't read first 0x10000 bytes of module %s...\n", ModuleList.m_szModuleName.c_str());
            continue;
        }

        PIMAGE_DOS_HEADER m_pImageDOSHeaders = reinterpret_cast<PIMAGE_DOS_HEADER>(m_ModulePEHeaders);
        if (m_pImageDOSHeaders->e_magic != IMAGE_DOS_SIGNATURE)
        {
            LOG("[-] Couldn't find IMAGE_DOS_SIGNATURE for module %s...\n", ModuleList.m_szModuleName.c_str());
            continue;
        }

        const PIMAGE_NT_HEADERS m_pImageNTHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<ULONG_PTR>(m_ModulePEHeaders) + m_pImageDOSHeaders->e_lfanew);

        if (m_pImageNTHeaders->Signature != IMAGE_NT_SIGNATURE)
        {
            LOG("[-] Couldn't find IMAGE_NT_SIGNATURE for module %s...\n", ModuleList.m_szModuleName.c_str());
            continue;
        }

        DWORD m_dSavedExportVirtualAddress = m_pImageNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        DWORD m_dSavedExportSize = m_pImageNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
      
        if (!m_dSavedExportVirtualAddress || !m_dSavedExportSize)
        {
            LOG("[-] Couldn't find export table of module %s...\n", ModuleList.m_szModuleName.c_str());
            continue;
        }

        PIMAGE_SECTION_HEADER m_pImageSectionHeader = IMAGE_FIRST_SECTION(m_pImageNTHeaders);

        DWORD m_dStartAddressOfSection = NULL;
        DWORD m_dSizeOfSection = NULL;

        // Save off our start address and size of .text section, so we can detect OOB exports later.
        for (UINT i = 0; i != m_pImageNTHeaders->FileHeader.NumberOfSections; ++i, ++m_pImageSectionHeader) {
            if (!strstr((char*)m_pImageSectionHeader->Name, ".text") ||
                !m_pImageSectionHeader->Misc.VirtualSize)
            {
                continue;
            }

            m_dStartAddressOfSection = m_pImageSectionHeader->VirtualAddress;
            m_dSizeOfSection = m_pImageSectionHeader->Misc.VirtualSize;
        }

        IMAGE_EXPORT_DIRECTORY m_pImageExportDirectory = { 0 };
        m_bRPMResult = ReadProcessMemory(g_Utilities.TargetProcess,
            reinterpret_cast<LPCVOID>(ModuleList.m_ModuleBaseAddress + m_dSavedExportVirtualAddress),
            &m_pImageExportDirectory,
            m_dSavedExportSize,
            NULL);

        if (!m_bRPMResult)
        {
            LOG("[-] Failed to read export directory of module %s with error code #%i...\n",
                ModuleList.m_szModuleName.c_str(),
                GetLastError());
            continue;
        }

        BYTE m_WholeModuleBuffer[3000000];
        m_bRPMResult = ReadProcessMemory(g_Utilities.TargetProcess,
            reinterpret_cast<LPCVOID>(ModuleList.m_ModuleBaseAddress),
            &m_WholeModuleBuffer,
            ModuleList.m_ModuleSize,
            NULL);

        if (!m_bRPMResult)
        {
            continue;
        }

        //read the file
        const HANDLE m_hFile = CreateFileA(ModuleList.m_szModulePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (!m_hFile || m_hFile == INVALID_HANDLE_VALUE)
        {
            LOG("[-] CreateFile failed with error code #%i...\n", GetLastError());
            continue;
        }

        //map the file
        const HANDLE m_hMappedFile = CreateFileMappingA(m_hFile, nullptr, PAGE_READONLY | SEC_IMAGE, 0, 0, nullptr);
        if (!m_hMappedFile || m_hMappedFile == INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hFile);
            LOG("[-] CreateFileMapping failed with error code #%i...\n", GetLastError());
            continue;
        }

        //map the sections appropriately
        ZyanU8* m_FileMap = reinterpret_cast<ZyanU8*>(MapViewOfFile(m_hMappedFile, FILE_MAP_READ, 0, 0, 0));
        if (!m_FileMap)
        {
            CloseHandle(m_hFile);
            CloseHandle(m_hMappedFile);
            LOG("[-] MapViewOfFile failed with error code #%i...\n", GetLastError());
            continue;
        }


        WORD* m_pOrdinalAddress = reinterpret_cast<WORD*>(m_pImageExportDirectory.AddressOfNameOrdinals + reinterpret_cast<uintptr_t>(&m_pImageExportDirectory) - m_dSavedExportVirtualAddress);
        DWORD* m_pNamesAddress = reinterpret_cast<DWORD*>(m_pImageExportDirectory.AddressOfNames + reinterpret_cast<uintptr_t>(&m_pImageExportDirectory) - m_dSavedExportVirtualAddress);
        DWORD* m_pFunctionAddress = reinterpret_cast<DWORD*>(m_pImageExportDirectory.AddressOfFunctions + reinterpret_cast<uintptr_t>(&m_pImageExportDirectory) - m_dSavedExportVirtualAddress);

        // Traverse through all export functions, getting all function's addresses.
        for (int i = 0; i < m_pImageExportDirectory.NumberOfNames; ++i)
        {
            // I don't yet understand how this is even possible to have exports leading outside of module bounds...
            // But it fucking happens, and not too infrequently either.
            const DWORD m_AddressFromBaseAddress = m_pFunctionAddress[m_pOrdinalAddress[i]];
            if (m_AddressFromBaseAddress >= ModuleList.m_ModuleSize)
            {
                // Too far ahead, this will cause us OOB crashes. Let's get out of here.
                continue;
            }

            // This fucking shitter isn't part of the .text section, GET EM OUTTA HERE.
            if (m_AddressFromBaseAddress < m_dStartAddressOfSection ||
                m_AddressFromBaseAddress > m_dStartAddressOfSection + m_dSizeOfSection)
            {
                continue;
            }


            bool bIsDifferent = false;
            for (int x = 0; x < 15; ++x)
            {
                if (m_FileMap[m_AddressFromBaseAddress + x] != m_WholeModuleBuffer[m_AddressFromBaseAddress + x])
                    bIsDifferent = true;
            }

            // Print and cache differences.
            if (bIsDifferent)
            {
                char* m_szExportName = reinterpret_cast<char*>(m_pNamesAddress[i] + reinterpret_cast<uintptr_t>(&m_pImageExportDirectory) - m_dSavedExportVirtualAddress);

                std::string m_szExportNameStr(m_szExportName);

                if (m_szExportNameStr.empty() || g_Utilities.HasSpecialCharacters(m_szExportNameStr.c_str()))
                {
                    // Something is very very wrong, I've only seen this for a few modules.
                    continue;
                }

                LOG("[+] Found difference at %s!%s addr:0x%X\n",
                    ModuleList.m_szModuleName.c_str(),
                    m_szExportNameStr.c_str(),
                    m_AddressFromBaseAddress);

                LOG("[+] Original Buffer: ");
                for (int x = 0; x < 15; ++x) {
                    LOG("%02X ", m_FileMap[m_AddressFromBaseAddress + x]);
                }  LOG("\n\n");

                LOG("=============================================\n\n");

                LOG("[+] Modified Buffer: ");
                for (int x = 0; x < 15; ++x) {
                    LOG("%02X ", m_WholeModuleBuffer[m_AddressFromBaseAddress + x]);
                }  LOG("\n\n");

                // Initialize decoder context
                ZydisDecoder decoder;
#if defined (_WIN64)
                ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZydisStackWidth::ZYDIS_STACK_WIDTH_64);
#else
                ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZydisStackWidth::ZYDIS_STACK_WIDTH_32);
#endif

                // Initialize formatter. Only required when you actually plan to do instruction
                // formatting ("disassembling"), like we do here
                ZydisFormatter formatter;
                ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

                // Loop over the instructions in our buffer.
                // The runtime-address (instruction pointer) is chosen arbitrary here in order to better
                // visualize relative addressing
                ZyanU64 runtime_address = m_AddressFromBaseAddress;
                ZyanUSize offset = 0;
                const ZyanUSize length = 15;
                ZydisDecodedInstruction instruction;
                while (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, m_WholeModuleBuffer + m_AddressFromBaseAddress + offset, length - offset, &instruction)))
                {
                    // Print current instruction pointer.
                    LOG("%010" "llx" "  ", runtime_address);

                    // Format & print the binary instruction structure to human readable format
                    char buffer[256];
                    ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), runtime_address);

                    LOG("%s\n", buffer);

                    offset += instruction.length;
                    runtime_address += instruction.length;
                } LOG("\n");
            }
        }

        UnmapViewOfFile(m_FileMap);
        CloseHandle(m_hFile);
        CloseHandle(m_hMappedFile);
    }

    // Cleanup, and finish off.
    {
        CloseHandle(g_Utilities.TargetProcess);
    }

    PAUSE_SYSTEM_CMD(true);
}
