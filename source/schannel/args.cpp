#include <cxxopts.hpp>
#include <codecvt>
#include <crypt.hpp>
#include <magic_enum.hpp>
#include <schannel/args.hpp>
#include <schannel/messages.hpp>
#include <schannel/proxy.hpp>

namespace Schannel {
    bool HandleFunction(std::ostream& out, const Proxy& proxy, const cxxopts::ParseResult& result) {
        switch (magic_enum::enum_cast<PROTOCOL_MESSAGE_TYPE>(result["function"].as<std::string>()).value()) {
        case PROTOCOL_MESSAGE_TYPE::CacheInfo: {
            return false;// CacheInfo();
        }
        case PROTOCOL_MESSAGE_TYPE::LookupCert:
            return false;// LookupCert();
        case PROTOCOL_MESSAGE_TYPE::LookupExternalCert: {
            return false;// return LookupExternalCert();
        }
        case PROTOCOL_MESSAGE_TYPE::PerfmonInfo: {
            DWORD flags{ 0 }; // The flags are ignored by the dispatch function
            return proxy.PerfmonInfo(flags);
        }
        case PROTOCOL_MESSAGE_TYPE::PurgeCache: {
            LUID luid;
            reinterpret_cast<LARGE_INTEGER*>(&luid)->QuadPart = result["luid"].as<long long>();
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            auto server{ converter.from_bytes(result["server"].as<std::string>()) };
            DWORD flags{ 0 };
            flags |= (result.count("client-entry")) ? static_cast<ULONG>(Schannel::PurgeEntriesType::Client) : 0;
            flags |= (result.count("server-entry")) ? static_cast<ULONG>(Schannel::PurgeEntriesType::Server) : 0;
            flags |= (result.count("clients")) ? static_cast<ULONG>(Schannel::PurgeEntriesType::ClientAll) : 0;
            flags |= (result.count("servers")) ? static_cast<ULONG>(Schannel::PurgeEntriesType::ServerAll) : 0;
            flags |= (result.count("locators")) ? static_cast<ULONG>(Schannel::PurgeEntriesType::ServerEntriesDisardLocators) : 0;
            return proxy.PurgeCache(&luid, server, flags);
        }
        case PROTOCOL_MESSAGE_TYPE::StreamSizes:
            return proxy.StreamSizes();
        default:
            out << "Unsupported function" << std::endl;
            return false;
        }
    }

    void Parse(std::ostream& out, const std::vector<std::string>& args) {
        char* command{ "schannel" };
        cxxopts::Options options{ command };

        options.add_options("Schannel Function")
            ("f,function", "Function name", cxxopts::value<std::string>())
            ;

        // Arguments for functions that require additional inputs
        options.add_options("Function arguments")
            ("server", "Server name", cxxopts::value<std::string>())
            ("luid", "Logon session", cxxopts::value<long long>())
            ("clients", "All clients flag", cxxopts::value<bool>()->default_value("false"))
            ("client-entry", "Client entry flag", cxxopts::value<bool>()->default_value("false"))
            ("locators", "Purge locators flag", cxxopts::value<bool>()->default_value("false"))
            ("servers", "All servers flag", cxxopts::value<bool>()->default_value("false"))
            ("server-entry", "Server entry flag", cxxopts::value<bool>()->default_value("false"))
            ;

        try {
            std::vector<char*> argv{ command };
            std::for_each(args.begin(), args.end(), [&argv](const std::string& arg) {
                argv.push_back(const_cast<char*>(arg.data()));
            });
            auto result{ options.parse(argv.size(), argv.data()) };
            if (result.count("function")) {
                auto lsa{ std::make_shared<Lsa>() };
                Proxy proxy{ lsa };
                HandleFunction(out, proxy, result);
            }
            else {
                out << options.help() << std::endl;
            }
        }
        catch (const std::exception& exception) {
            out << exception.what() << std::endl;
        }
    }
}