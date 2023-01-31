#include <koalabox/globals.hpp>
#include <koalabox/http_server.hpp>
#include <koalabox/paths.hpp>
#include <koalabox/util.hpp>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <koalabox/logger.hpp>

namespace koalabox::http_server {

    // Source: https://gist.github.com/nathan-osman/5041136
    class Certificate {
    private:
        EVP_PKEY* key = nullptr;
        X509* x509 = nullptr;
        String cert_name;

        void free_key() {
            EVP_PKEY_free(key);
            key = nullptr;
        }

        void free_cert() {
            X509_free(x509);
            x509 = nullptr;

            free_key();
        }

        String get_cert_string() {
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
            const String& common_name,
            const Certificate& issuer,
            const Map<int, String> ext_map // See https://superuser.com/a/1248085
        ) {
            create_key();

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
            add_entry("CN", koalabox::globals::get_project_name() + " " + common_name);

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
        Certificate() : Certificate("Authority", *this, {
                { NID_basic_constraints,        "critical, CA:TRUE" },
                { NID_subject_key_identifier,   "hash" },
                { NID_authority_key_identifier, "keyid:always, issuer:always" },
                { NID_key_usage,                "critical, cRLSign, digitalSignature, keyCertSign" },
            }
        ) {}

        /**
         * Constructs a server certificate signed by the provided authority
         */
        Certificate(
            const String& server_host,
            const Certificate& issuer
        ) : Certificate("Server", issuer, {
                { NID_basic_constraints,        "critical, CA:FALSE" },
                { NID_subject_key_identifier,   "hash" },
                { NID_authority_key_identifier, "keyid:always, issuer:always" },
                { NID_key_usage,                "critical, nonRepudiation, digitalSignature, keyEncipherment, keyAgreement" },
                { NID_ext_key_usage,            "critical, serverAuth" },
                { NID_subject_alt_name,         "DNS:" + server_host }
            }
        ) {}

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

        void write_to_disk(const String& file_name) {
            const auto write = [&](const String& extension, const Function<int(FILE*)>& callback) {
                const auto full_file_name = file_name + "." + extension;
                const auto file_path = (koalabox::paths::get_self_path() / full_file_name).string();

                LOG_DEBUG("Writing {}", file_path)

                FILE* stream;
                if (fopen_s(&stream, file_path.c_str(), "wb")) {
                    throw koalabox::util::exception("Unable to open {} for writing", file_path);
                }

                auto key_success = callback(stream);
                fclose(stream);

                if (not key_success) {
                    throw koalabox::util::exception("Unable to write {} to disk", file_path);
                }
            };

            write("key", [&](FILE* stream) {
                return PEM_write_PrivateKey(stream, get_key(), NULL, NULL, 0, NULL, NULL);
            });

            write("crt", [&](FILE* stream) {
                return PEM_write_X509(stream, get_x509());
            });
        }

        void add_to_system_store(const String& store_name) {
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
                "Successfully added a certificate to the system '{}' store",
                store_name
            )
        }
    };

    void start(
        const String& local_host,
        unsigned int port,
        const String& server_host,
        const Map<String, httplib::Server::Handler>& pattern_handlers
    ) noexcept {
        std::thread([=]() {
            try {
                const auto project_name = koalabox::globals::get_project_name();

                Certificate ca_cert;
                ca_cert.write_to_disk(project_name + ".ca");
                ca_cert.add_to_system_store("ROOT");

                Certificate server_cert(server_host, ca_cert);
                server_cert.write_to_disk(project_name + ".server");

                httplib::SSLServer server(server_cert.get_x509(), server_cert.get_key());

                // TODO: Fallback handlers for other GET paths, and all POST, DELETE, etc. paths

                for (const auto& [pattern, handler]: pattern_handlers) {
                    server.Get(pattern, handler);
                }

                if (not server.listen(local_host, port)) {
                    throw Exception("server.listen returned false");
                }
            } catch (const Exception& e) {
                koalabox::util::panic("Error starting http server: {}", e.what());
            }
        }).detach();
    }

}
