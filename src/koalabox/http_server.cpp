#include <koalabox/http_server.hpp>
#include <koalabox/globals.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/paths.hpp>
#include <koalabox/util.hpp>
#include <koalabox/win_util.hpp>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <ShlObj_core.h>
#include <WinDNS.h>
#include <winhttp.h>

#pragma comment(lib, "Dnsapi.lib")
#pragma comment(lib, "Winhttp.lib")

namespace koalabox::http_server {

    // Source: https://gist.github.com/nathan-osman/5041136
    class Certificate {
    private:
        EVP_PKEY* key = nullptr;
        X509* x509 = nullptr;
        bool is_ca;
        String cert_name = koalabox::globals::get_project_name();

        void free_key() {
            EVP_PKEY_free(key);
            key = nullptr;
        }

        void free_cert() {
            X509_free(x509);
            x509 = nullptr;

            free_key();
        }

        String get_cert_string() const {
            if (not x509) {
                throw Exception("Null pointer certificate");
            }

            auto* bio = BIO_new(BIO_s_mem());
            if (not bio) {
                throw Exception("Failed to allocate bio buffer");
            }

            if (not PEM_write_bio_X509(bio, x509)) {
                BIO_free(bio);
                throw Exception("Failed to write x509 to bio buffer");
            }

            BUF_MEM* buf_mem = nullptr;
            BIO_get_mem_ptr(bio, &buf_mem);

            if (not buf_mem) {
                BIO_free(bio);
                throw Exception("Failed to get bio memory pointer");
            }

            String cert_string(buf_mem->length, '\0');

            if (not BIO_read(bio, cert_string.data(), buf_mem->length)) {
                throw Exception("Failed to copy bio buffer to string");
            }

            BIO_free(bio);

            return cert_string;
        }

        void write_ca_to_disk() {
            if (not is_ca) {
                throw Exception("Attempt to write non-CA certificate to disk");
            }

            const auto write = [&](const Path& path, const Function<int(FILE*)>& callback) {
                LOG_DEBUG("Writing CA certificate to {}", path.string())

                FILE* stream;
                if (fopen_s(&stream, path.string().c_str(), "wb")) {
                    throw util::exception("Error opening certificate file for writing");
                }

                auto key_success = callback(stream);
                fclose(stream);

                if (not key_success) {
                    throw util::exception("Error writing certificate to disk");
                }
            };

            write(paths::get_ca_key_path(), [&](FILE* stream) {
                return PEM_write_PrivateKey(stream, key, NULL, NULL, 0, NULL, NULL);
            });

            write(paths::get_ca_cert_path(), [&](FILE* stream) {
                return PEM_write_X509(stream, x509);
            });
        }

        void create_key() {
            LOG_DEBUG("Creating an RSA key")

            // Allocate memory for the EVP_PKEY structure.
            key = EVP_PKEY_new();

            if (!key) {
                throw Exception("Unable to create an EVP_PKEY structure.");
            }

            // Generate the RSA key and assign it to pkey
            RSA* rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);

            if (!EVP_PKEY_assign_RSA(key, rsa)) {
                EVP_PKEY_free(key);
                throw Exception("Unable to generate an RSA key.");
            }

            // The key has been generated at this point
            LOG_DEBUG("Successfully created an RSA key")
        }

        Certificate(
            bool is_ca,
            const Certificate& issuer,
            const Map<int, String> ext_map // See https://superuser.com/a/1248085
        ) {
            create_key();

            this->is_ca = is_ca;

            LOG_DEBUG("Creating an x509 certificate")

            // Allocate memory for the X509 structure.
            x509 = X509_new();

            if (not x509) {
                throw Exception("Unable to create an X509 structure.");
            }

            X509_set_version(x509, 2);

            // Create random serial
            // Source: https://github.com/pashapovalov/Webinos-Platform-buildhive/blob/386178ed83198c7b802aa894f579899edf199525/webinos/common/manager/certificate_manager/src/openssl_wrapper.cpp#L166-L176
            ASN1_INTEGER* serial = ASN1_INTEGER_new();
            BIGNUM* big_num = BN_new();
            //64 bits of randomness?
            BN_pseudo_rand(big_num, 120, 0, 0);
            BN_to_ASN1_INTEGER(big_num, serial);
            BN_free(big_num);
            X509_set_serialNumber(x509, serial);

            // This certificate is valid from now until 25 year from now.
            X509_gmtime_adj(X509_get_notBefore(x509), 0);
            X509_gmtime_adj(X509_get_notAfter(x509), 25 * 365 * 24 * 60 * 60);

            // Set the public key for our certificate.
            X509_set_pubkey(x509, key);

            // We want to copy the subject name to the issuer name.
            X509_NAME* subject_name = X509_get_subject_name(x509);

            const auto add_entry = [&](const String& field, const String& data) {
                X509_NAME_add_entry_by_txt(
                    subject_name, // name
                    field.c_str(), // field
                    MBSTRING_ASC, // type
                    (unsigned char*) data.c_str(), // bytes
                    -1, // len
                    -1, // loc
                    0 // set
                );
            };

            // Set the country code and common name.
            const String project_name = koalabox::globals::get_project_name();
            add_entry("O", "acidicoala");
            add_entry("CN", cert_name + " " + (is_ca ? "Authority" : "Server"));

            for (const auto& [nid, data]: ext_map) {
                X509V3_CTX ctx;
                X509V3_set_ctx_nodb(&ctx)

                X509V3_set_ctx(&ctx, issuer.x509, x509, NULL, NULL, 0);
                auto* ext = X509V3_EXT_conf_nid(NULL, &ctx, nid, data.c_str());
                if (not X509V3_EXT_conf_nid(NULL, &ctx, nid, data.c_str())) {
                    continue;
                }

                X509_add_ext(x509, ext, -1);
                X509_EXTENSION_free(ext);
            }

            // Now set the issuer name.
            X509_set_issuer_name(x509, X509_get_subject_name(issuer.x509));

            // Actually sign the certificate with our key.
            if (!X509_sign(x509, issuer.key, EVP_sha256())) {
                free_cert();
                throw Exception("Error signing certificate.");
            }

            LOG_DEBUG("Successfully created an x509 certificate")
        }

    public:
        /**
         * Constructs a self-signed certification authority
         */
        Certificate() : Certificate(true, *this, {
                { NID_basic_constraints,        "critical, CA:TRUE" },
                { NID_subject_key_identifier,   "hash" },
                { NID_authority_key_identifier, "keyid:always, issuer:always" },
                { NID_key_usage,                "critical, cRLSign, digitalSignature, keyCertSign" },
            }
        ) {
            // We want to cache created CA certificates
            write_ca_to_disk();
        }

        /**
         * Constructs a server certificate signed by the provided authority
         */
        Certificate(
            const String& server_host,
            const Certificate& issuer
        ) : Certificate(false, issuer, {
                { NID_basic_constraints,        "critical, CA:FALSE" },
                { NID_subject_key_identifier,   "hash" },
                { NID_authority_key_identifier, "keyid:always, issuer:always" },
                { NID_key_usage,                "critical, nonRepudiation, digitalSignature, keyEncipherment, keyAgreement" },
                { NID_ext_key_usage,            "critical, serverAuth" },
                { NID_subject_alt_name,         "DNS:" + server_host }
            }
        ) {}

        /**
         * Constructs an existing CA certificate
         */
        Certificate(EVP_PKEY* key, X509* x509) : key{ key }, x509{ x509 }, is_ca{ true } {}

        ~Certificate() {
            LOG_DEBUG("Freeing an x509 certificate")

            free_cert();
        }

        EVP_PKEY* get_key() const {
            return key;
        }

        X509* get_x509() const {
            return x509;
        }

        void add_to_system_store(const String& store_name) const {
            const auto cert_string = get_cert_string();
            LOG_TRACE("Adding cert to system store:\n{}", cert_string)

            Vector<BYTE> binary_buffer(32 * 1024); // 32KB should be more than enough
            DWORD binary_size = binary_buffer.size();
            if (not CryptStringToBinaryA(
                cert_string.c_str(),
                0,
                CRYPT_STRING_BASE64_ANY,
                binary_buffer.data(),
                &binary_size,
                NULL,
                NULL
            )) {
                throw Exception("Error converting base64 certificate to binary");
            }

            // Will return true even if the cert is already installed
            if (not CertAddEncodedCertificateToSystemStoreA(
                store_name.c_str(),
                binary_buffer.data(),
                binary_size
            )) {
                throw util::exception(
                    "Error adding a certificate to the system '{}' store",
                    store_name
                );
            }

            LOG_DEBUG(
                "Successfully added a certificate to the system '{}' store (or it was already present)",
                store_name
            )
        }

        /**
         * Reads a CA certificate from disk
         */
        static Certificate read_from_disk() {
            if (exists(paths::get_ca_key_path()) && exists(paths::get_ca_cert_path())) {
                try {
                    const auto read = [&](
                        const Path& path,
                        const Function<void*(FILE*)>& callback
                    ) {
                        LOG_DEBUG("Reading CA certificate from {}", path.string())

                        FILE* stream;
                        if (fopen_s(&stream, path.string().c_str(), "rb")) {
                            throw util::exception("Error opening certificate file for reading");
                        }

                        auto* result = callback(stream);
                        fclose(stream);

                        if (not result) {
                            throw util::exception("Error reading certificate from disk");
                        }

                        return result;
                    };

                    auto* key = (EVP_PKEY*) read(paths::get_ca_key_path(), [](FILE* stream) {
                        return PEM_read_PrivateKey(stream, nullptr, nullptr, nullptr);
                    });

                    auto* cert = (X509*) read(paths::get_ca_cert_path(), [](FILE* stream) {
                        return PEM_read_X509(stream, nullptr, nullptr, nullptr);
                    });

                    return Certificate(key, cert);
                } catch (const Exception& e) {
                    LOG_ERROR("Error reading CA certificate from disk: {}", e.what())
                }
            }

            LOG_DEBUG("No valid CA certificate has been found on disk. Creating a new one.")

            // We have to return constructor invocation,
            // otherwise the underlying key & cert will be freed by the destructor.
            return Certificate();
        }
    };

    class Hosts {
    private:
        Vector<String> lines;
        const Path hosts_path = get_system32_path() / "drivers" / "etc" / "hosts";

        static Path get_system32_path() {
            PWSTR buffer = nullptr;
            const auto result = SHGetKnownFolderPath(
                FOLDERID_System, KF_FLAG_DEFAULT, NULL, &buffer
            );

            if (result != S_OK) {
                throw util::exception("SHGetKnownFolderPath error: {}", result);
            }

            const auto path = Path(buffer);

            CoTaskMemFree(buffer);

            return path;
        }

    public:
        Hosts() {
            try {
                LOG_DEBUG("Reading hosts file from '{}'", hosts_path.string())

                std::ifstream input_stream(hosts_path);

                String line;
                while (std::getline(input_stream, line)) {
                    lines.push_back(line);
                }

                LOG_DEBUG("Read {} lines from hosts file", lines.size())
            } catch (const Exception& e) {
                throw util::exception("Error reading hosts file: {}", e.what());
            }
        }

        void remove(const String& hostname) {
            LOG_DEBUG("Removing hostname '{}'", hostname)

            lines.erase(
                std::remove_if(lines.begin(), lines.end(),
                    [&](const auto& line) {
                        auto trimmed_line = line;
                        const auto comment_index = line.find('#');
                        if (comment_index != std::string::npos) {
                            trimmed_line = trimmed_line.substr(0, comment_index);
                        }

                        // Far from perfect, but good enough for most cases.
                        return trimmed_line < contains > hostname;
                    }
                ),
                lines.end()
            );
        }

        void add(const String& hostname, const String& ip) {
            LOG_DEBUG("Adding hostname '{}' with ip '{}'", hostname, ip)

            const auto line = fmt::format(
                "{}    {} # {}", ip, hostname, globals::get_project_name()
            );

            lines.push_back(line);
        }

        void save() {
            try {
                LOG_DEBUG("Saving changes to the hosts file")

                std::ofstream output_stream;
                output_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                output_stream.open(hosts_path);

                for (const auto& line: lines) {
                    output_stream << line << std::endl;
                }
            } catch (const Exception& e) {
                throw util::exception("Error saving hosts file: {}", e.what());
            }
        }
    };

    namespace port_proxy {
        void execute_ps_command(const String& command) {
            const auto ps_command = fmt::format("powershell {}", command);
            // TODO: Replace with CreateProcess
            const auto result = WinExec(ps_command.c_str(), SW_HIDE);
            LOG_DEBUG("PowerShell returned result {} for command: {}", result, command)
        }

        void remove(unsigned int listen_port, String listen_address) {
            execute_ps_command(
                fmt::format(
                    "netsh interface portproxy delete v4tov4 "
                    "listenport={} listenaddress={}",
                    listen_port, listen_address
                )
            );
        }

        void add(
            unsigned int listen_port,
            const String& listen_address,
            unsigned int connect_port,
            const String& connect_address
        ) {
            execute_ps_command(
                fmt::format(
                    "netsh interface portproxy add v4tov4 "
                    "listenport={} listenaddress={} connectport={} connectaddress={}",
                    listen_port, listen_address, connect_port, connect_address
                )
            );
        }
    }

    std::optional<String> dns::get_real_ip(const String& hostname) {
        PDNS_RECORD dns_record;
        const auto dns_status = DnsQuery_A(
            hostname.c_str(),
            DNS_TYPE_A,
            DNS_QUERY_NO_HOSTS_FILE,
            NULL,
            &dns_record,
            NULL
        );

        if (dns_status != 0) {
            return std::nullopt;
        }

        in_addr addr{
            .S_un = {
                .S_addr = dns_record->Data.A.IpAddress
            }
        };

        String ip_str(17, '\0');
        inet_ntop(
            AF_INET,
            &addr,
            ip_str.data(),
            ip_str.size()
        );

        // Trim
        ip_str = String(ip_str.c_str());

        DnsRecordListFree(dns_record, DnsFreeRecordList);

        return ip_str;
    }

    std::optional<String> dns::get_canonical_name(const String& hostname) {
        PDNS_RECORD dns_record;
        const auto dns_status = DnsQuery(
            util::to_wstring(hostname).c_str(),
            DNS_TYPE_CNAME,
            DNS_QUERY_NO_HOSTS_FILE,
            NULL,
            &dns_record,
            NULL
        );

        if (dns_status != 0) {
            return std::nullopt;
        }

        const auto cname = util::to_string(dns_record->Data.CNAME.pNameHost);

        DnsRecordListFree(dns_record, DnsFreeRecordList);

        return cname;
    }

    namespace system_proxy {
        struct ProxyInfo {
            String scheme;
            String host;
            int port;
        };

        // Key is target scheme
        std::optional<Map<String, ProxyInfo>> get_info() {
            Map<String, ProxyInfo> proxy_info_map;

            WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxy_config;
            const auto success = WinHttpGetIEProxyConfigForCurrentUser(&proxy_config);
            if (not success) {
                LOG_ERROR(
                    "Error getting default proxy configuration. Last error: {}",
                    win_util::get_last_error()
                )

                return std::nullopt;
            }

            if (not proxy_config.lpszProxy) {
                return std::nullopt;
            }

            const auto proxy_str = util::to_string(proxy_config.lpszProxy);

            const std::regex regex(R"(((\w+)=)?(\w+://)?([^\s:]+)(:(\d+))?[\s;]?)");
            auto it = std::sregex_iterator(proxy_str.begin(), proxy_str.end(), regex);
            for (; it != std::sregex_iterator(); it++) {
                try {
                    const auto match = *it;

                    LOG_TRACE("Detected system proxy server: {}", match.str())

                    auto target_scheme = match.str(2);

                    // TODO: Extract to utils
                    std::transform(
                        target_scheme.begin(),
                        target_scheme.end(),
                        target_scheme.begin(),
                        [](unsigned char c) { return std::tolower(c); }
                    );

                    const auto port_str = match.str(6);

                    proxy_info_map[target_scheme] = {
                        .scheme = match.str(3),
                        .host = match.str(4),
                        .port = port_str.empty() ? -1 : std::stoi(match.str(6))
                    };
                } catch (const Exception& e) {
                    LOG_ERROR("Error parsing system proxy server: {}", proxy_str)
                }
            }

            return proxy_info_map;
        }
    }

    bool make_original_request(
        const String& bypass_server_host,
        httplib::Request req,
        httplib::Response& res
    ) {
        static auto static_req_id = 0;
        static_req_id++;

        const auto req_id = static_req_id;

        try {

            req.path = req.target;

            LOG_DEBUG(
                "[{}] Making original request to: https://{}{}",
                req_id, bypass_server_host, req.target
            )

            httplib::SSLClient client(bypass_server_host);
            client.enable_server_certificate_verification(false);

            req.headers.erase("LOCAL_ADDR");
            req.headers.erase("LOCAL_PORT");
            req.headers.erase("REMOTE_ADDR");
            req.headers.erase("REMOTE_PORT");

            const auto system_proxy_map_opt = system_proxy::get_info();
            if (system_proxy_map_opt) {
                const auto system_proxy_map = *system_proxy_map_opt;

                std::optional<system_proxy::ProxyInfo> proxy_info_opt = std::nullopt;
                if (system_proxy_map.contains("https")) {
                    proxy_info_opt = system_proxy_map.at("https");
                } else if (system_proxy_map.contains("")) {
                    proxy_info_opt = system_proxy_map.at("");
                }

                if (proxy_info_opt) {
                    const auto proxy_info = *proxy_info_opt;

                    /*LOG_DEBUG(
                        "[{}] Setting proxy for request: {}{}:{}",
                        req_id, proxy_info.scheme, proxy_info.host, proxy_info.port
                    )*/

                    // FIXME: Setting proxy breaks requests :/
                    // client.set_proxy(proxy_info.host, proxy_info.port);
                }
            }

            const auto result = client.send(req);

            if (result.error() != httplib::Error::Success) {
                throw util::exception(
                    "Error sending request to original server. Code: {}. Message: {}",
                    (int) result.error(), httplib::to_string(result.error())
                );
            }

            res = *result;

            LOG_DEBUG("[{}] Generic server handler success: {}", req_id, res.status)

            return res.status >= 200 && res.status < 300;
        } catch (const Exception& e) {
            LOG_ERROR("[{}] Generic server handler error: {}", req_id, e.what())

            res.status = 503;

            return false;
        }
    }

    void start_proxy_server(
        const String& bypass_server_host,
        const String& original_server_host,
        unsigned int original_server_port,
        const String& proxy_server_host,
        unsigned int proxy_server_port,
        const String& port_proxy_ip,
        const Vector<std::pair<String, httplib::Server::Handler>>& pattern_handlers
    ) noexcept {
        std::thread([=]() {
            try {
                if (!IsUserAnAdmin()) {
                    LOG_WARN(
                        "Program is not running as administrator. "
                        "Automatic configuration will be restricted."
                    )
                    //throw util::exception("Program is not running as administrator");
                }

                const auto ca_cert = Certificate::read_from_disk();
                ca_cert.add_to_system_store("ROOT");

                Certificate server_cert(original_server_host, ca_cert);

                if (IsUserAnAdmin()) {
                    // FIXME: Doesn't work with EOS SDK DLL. It queries DNS using DnsQueryEx with
                    // DNS_QUERY_NO_LOCAL_NAME and DNS_QUERY_NO_HOSTS_FILE flags
                    // which bypass the hosts file.

                    Hosts hosts;
                    hosts.remove(original_server_host);
                    hosts.add(original_server_host, port_proxy_ip);
                    hosts.save();

                    port_proxy::add(
                        original_server_port,
                        port_proxy_ip,
                        proxy_server_port,
                        proxy_server_host
                    );
                }

                httplib::SSLServer server(server_cert.get_x509(), server_cert.get_key());

                server.set_logger([](const auto& req, const auto& res) {
                    LOG_TRACE(
                        "HTTP Server {}:{} {} '{}'. Response code: {}, body:\n{}",
                        req.local_addr, req.local_port, req.method, req.target, res.status, res.body
                    )
                });

                const auto generic_handler = [&](
                    const httplib::Request& req,
                    httplib::Response& res
                ) {
                    make_original_request(bypass_server_host, req, res);
                };

                for (const auto& [pattern, handler]: pattern_handlers) {
                    server.Get(pattern, handler);
                }

                // Fallback handlers must always come after custom handlers
                const auto generic_pattern = ".*";
                server.Get(generic_pattern, generic_handler);
                server.Delete(generic_pattern, generic_handler);
                server.Options(generic_pattern, generic_handler);
                server.Patch(generic_pattern, generic_handler);
                server.Post(generic_pattern, generic_handler);
                server.Put(generic_pattern, generic_handler);

                LOG_INFO(
                    "Starting a proxy HTTPS server for hostname '{}' on {}:{}",
                    original_server_host, proxy_server_host, proxy_server_port
                )

                if (not server.listen(proxy_server_host, proxy_server_port)) {
                    throw Exception("server.listen returned false");
                }
            } catch (const Exception& e) {
                koalabox::util::panic("Error starting http server: {}", e.what());
            }
        }).detach();
    }
}
