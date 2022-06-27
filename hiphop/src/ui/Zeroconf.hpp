/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2022 Luciano Iam <oss@lucianoiam.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ZEROCONF_HPP
#define ZEROCONF_HPP

#if DISTRHO_OS_LINUX
# include <cstdio>
# include <signal.h>
# include <spawn.h>
#elif DISTRHO_OS_MAC
# include <dns_sd.h>
# include <arpa/inet.h>
#elif DISTRHO_OS_WINDOWS
# include <codecvt>
# include <cstring>
# include <locale>
# include <libloaderapi.h>
# include <windns.h>
# include <winerror.h>
#endif

#include "src/DistrhoDefines.h"

#if DISTRHO_OS_LINUX
extern char **environ;
#endif

START_NAMESPACE_DISTRHO

#if DISTRHO_OS_WINDOWS
# define LOAD_DNSAPI_DLL() LoadLibrary("dnsapi.dll")
class Zeroconf;
struct DnsApiHelper
{
    Zeroconf* weakThis;
    DNS_SERVICE_INSTANCE* instance;
};
#endif

class Zeroconf
{
public:
    Zeroconf()
        : fPublished(false)
#if DISTRHO_OS_LINUX
        , fPid(0)
#elif DISTRHO_OS_MAC
        , fService(nullptr)
#elif DISTRHO_OS_WINDOWS
        , fHelper(nullptr)
#endif
    {}

    ~Zeroconf()
    {
        unpublish();
    }

    bool isPublished() const noexcept
    {
        return fPublished;
    }

    void publish(const char* name, const char* type, int port, const char* txt) noexcept
    {
        unpublish();

#if DISTRHO_OS_LINUX
        const char* const bin = "avahi-publish";
        char sport[10];
        std::sprintf(sport, "%d", port);
        const char *argv[] = {bin, "-s", name, type, sport, txt, nullptr};
        const int status = posix_spawnp(&fPid, bin, nullptr/*file_actions*/, nullptr/*attrp*/,
                                        const_cast<char* const*>(argv), environ);
        if (status == 0) {
            fPublished = true;
        } else {
            d_stderr2("Zeroconf : failed publish()");
        }
#elif DISTRHO_OS_MAC
        char txtRecord[127];
        snprintf(txtRecord + 1, sizeof(txtRecord) - 1, "%s", txt);
        txtRecord[0] = static_cast<char>(strlen(txt));
        const uint16_t txtLen = 1 + static_cast<uint16_t>(txtRecord[0]);

        DNSServiceErrorType err = DNSServiceRegister(&fService, 0/*flags*/, kDNSServiceInterfaceIndexAny,
            name, type, nullptr/*domain*/, nullptr/*host*/, htons(port), txtLen, txtRecord,
            nullptr/*callBack*/, nullptr/*context*/);
        if (err == kDNSServiceErr_NoError) {
            fPublished = true;
        } else {
            d_stderr2("Zeroconf : failed publish()");
        }
#elif DISTRHO_OS_WINDOWS
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
        HMODULE dnsapi = LOAD_DNSAPI_DLL();
        if (dnsapi == nullptr) {
            return;
        }

        typedef PDNS_SERVICE_INSTANCE (*PFN_DnsServiceConstructInstance)(PCWSTR pServiceName, PCWSTR pHostName,
                PIP4_ADDRESS pIp4, PIP6_ADDRESS pIp6, WORD wPort, WORD wPriority, WORD wWeight, DWORD dwPropertiesCount,
                PCWSTR *keys, PCWSTR *values);
        const PFN_DnsServiceConstructInstance pDnsServiceConstructInstance =
            reinterpret_cast<PFN_DnsServiceConstructInstance>(GetProcAddress(dnsapi, "DnsServiceConstructInstance"));      

        typedef DWORD (*PFN_DnsServiceRegister)(PDNS_SERVICE_REGISTER_REQUEST pRequest, PDNS_SERVICE_CANCEL pCancel);
        const PFN_DnsServiceRegister pDnsServiceRegister =
            reinterpret_cast<PFN_DnsServiceRegister>(GetProcAddress(dnsapi, "DnsServiceRegister"));

        if ((pDnsServiceConstructInstance == nullptr) || (pDnsServiceRegister == nullptr)) {
            FreeLibrary(dnsapi);
            return;
        }

        char hostname[128];
        DWORD size = sizeof(hostname);
        GetComputerNameEx(ComputerNameDnsHostname, hostname, &size);
        std::strcat(hostname, ".local");

        char service[128];
        std::strcpy(service, name);
        std::strcat(service, ".");
        std::strcat(service, type);
        std::strcat(service, ".local");

        std::wstring_convert<std::codecvt_utf8<wchar_t>> wconv;

        // Helper outlives Zeroconf instance because the DNS API is asynchronous
        fHelper = new DnsApiHelper();
        fHelper->weakThis = this;
        fHelper->instance = pDnsServiceConstructInstance(wconv.from_bytes(service).c_str(),
                wconv.from_bytes(hostname).c_str(), nullptr, nullptr, static_cast<WORD>(port),
                0, 0, 0, nullptr, nullptr);
        
        if (fHelper->instance != nullptr) {
            std::memset(&fCancel, 0, sizeof(fCancel));
            std::memset(&fRequest, 0, sizeof(fRequest));
            fRequest.Version = DNS_QUERY_REQUEST_VERSION1;
            fRequest.pServiceInstance = fHelper->instance;
            fRequest.pRegisterCompletionCallback = dnsServiceRegisterComplete;
            fRequest.pQueryContext = fHelper;
            pDnsServiceRegister(&fRequest, &fCancel);
        }

        FreeLibrary(dnsapi);
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic pop
# endif
#endif
    }

    void unpublish() noexcept
    {
#if DISTRHO_OS_LINUX
        if (fPid != 0) {
            kill(fPid, SIGTERM);
            fPid = 0;
        }
#elif DISTRHO_OS_MAC
        if (fService != nullptr) {
            DNSServiceRefDeallocate(fService);
            fService = nullptr;
        }
#elif DISTRHO_OS_WINDOWS
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
        HMODULE dnsapi = LOAD_DNSAPI_DLL();
        if (dnsapi == nullptr) {
            return;
        }

        if (fHelper != nullptr) {
            if (fPublished) {
                // Avoid callback to deleted this, helper instance is deleted later.
                fHelper->weakThis = nullptr;

                typedef (*PFN_DnsServiceDeRegister)(PDNS_SERVICE_REGISTER_REQUEST pRequest, PDNS_SERVICE_CANCEL pCancel);
                const PFN_DnsServiceDeRegister pDnsServiceDeRegister =
                    reinterpret_cast<PFN_DnsServiceDeRegister>(GetProcAddress(dnsapi, "DnsServiceDeRegister"));
                pDnsServiceDeRegister(&fRequest, nullptr);
            } else {
                typedef (*PFN_DnsServiceRegisterCancel)(PDNS_SERVICE_CANCEL pCancelHandle);
                const PFN_DnsServiceRegisterCancel pDnsServiceRegisterCancel =
                    reinterpret_cast<PFN_DnsServiceRegisterCancel>(GetProcAddress(dnsapi, "DnsServiceRegisterCancel"));
                pDnsServiceRegisterCancel(&fCancel);

                typedef (*PFN_DnsServiceFreeInstance)(PDNS_SERVICE_INSTANCE pInstance);
                const PFN_DnsServiceFreeInstance pDnsServiceFreeInstance =
                    reinterpret_cast<PFN_DnsServiceFreeInstance>(GetProcAddress(dnsapi, "DnsServiceFreeInstance"));
                pDnsServiceFreeInstance(fHelper->instance);

                delete fHelper;
            }

            fHelper = nullptr;
        }

        FreeLibrary(dnsapi);
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic pop
# endif
#endif
    }

#if DISTRHO_OS_WINDOWS
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
    static void dnsServiceRegisterComplete(DWORD status, PVOID pQueryContext, PDNS_SERVICE_INSTANCE /*pInstance*/)
    {
        DnsApiHelper* helper = static_cast<DnsApiHelper*>(pQueryContext);

        if (helper->weakThis == nullptr) {
            HMODULE dnsapi = LOAD_DNSAPI_DLL();
            
            typedef (*PFN_DnsServiceFreeInstance)(PDNS_SERVICE_INSTANCE pInstance);
            const PFN_DnsServiceFreeInstance pDnsServiceFreeInstance =
                reinterpret_cast<PFN_DnsServiceFreeInstance>(GetProcAddress(dnsapi, "DnsServiceFreeInstance"));
            pDnsServiceFreeInstance(helper->instance);
            
            FreeLibrary(dnsapi);
            delete helper;

            return;
        }

        if (status == ERROR_SUCCESS) {
            helper->weakThis->fPublished = true;
            helper->weakThis = nullptr;
        } else {
            d_stderr2("Zeroconf : failed publish()");
        }
    }
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic pop
# endif
#endif

private:
    bool fPublished;
#if DISTRHO_OS_LINUX
    pid_t fPid;
#elif DISTRHO_OS_MAC
    DNSServiceRef fService;
#elif DISTRHO_OS_WINDOWS
    DnsApiHelper* fHelper;
    DNS_SERVICE_REGISTER_REQUEST fRequest;
    DNS_SERVICE_CANCEL fCancel;
#endif

};

END_NAMESPACE_DISTRHO

#endif // ZEROCONF_HPP
