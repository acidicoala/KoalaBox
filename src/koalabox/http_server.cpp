#include <koalabox/globals.hpp>
#include <koalabox/http_server.hpp>
#include <koalabox/paths.hpp>
#include <koalabox/util.hpp>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <koalabox/logger.hpp>

namespace koalabox::http_server {

    // Source: https://gist.github.com/nathan-osman/5041136

    class Key {
    private:
        EVP_PKEY* value = nullptr;
    public:
        Key() {
            LOG_DEBUG("Creating an RSA key")

            // Allocate memory for the EVP_PKEY structure.
            value = EVP_PKEY_new();

            if (!value) {
                throw Exception("Unable to create an EVP_PKEY structure.");
            }

            // Generate the RSA key and assign it to pkey
            RSA* rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);

            if (!EVP_PKEY_assign_RSA(value, rsa)) {
                EVP_PKEY_free(value);
                throw Exception("Unable to generate an RSA key.");
            }

            // The key has been generated at this point
            LOG_DEBUG("Successfully created an RSA key")
        }

        ~Key() {
            LOG_DEBUG("Freeing an RSA key")

            EVP_PKEY_free(value);
        }

        EVP_PKEY* operator*() const {
            return value;
        }
    };

    class Certificate {
    private:
        X509* x509;
    public:
        Certificate(const Key& key, const String& common_name) {
            LOG_DEBUG("Creating an x509 certificate")

            // Allocate memory for the X509 structure.
            x509 = X509_new();

            if (!x509) {
                throw Exception("Unable to create an X509 structure.");
            }

            ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

            // This certificate is valid from now until 25 year from now.
            X509_gmtime_adj(X509_get_notBefore(x509), 0);
            X509_gmtime_adj(X509_get_notAfter(x509), 25 * 365 * 24 * 60 * 60);

            // Set the public key for our certificate.
            X509_set_pubkey(x509, *key);

            // We want to copy the subject name to the issuer name.
            X509_NAME* name = X509_get_subject_name(x509);

            const auto add_entry = [&](const String& field, const String& data) {
                X509_NAME_add_entry_by_txt(
                    name, field.c_str(), MBSTRING_ASC, (unsigned char*) data.c_str(), -1, -1, 0
                );
            };

            // Set the country code and common name.
            add_entry("C", "AU");
            add_entry("S", "Eucalyptus forest");
            add_entry("O", koalabox::globals::get_project_name());
            add_entry("CN", common_name);

            const auto add_ext = [&](int nid, const String& data) {
                X509_EXTENSION* ext;
                X509V3_CTX ctx;
                // This sets the context of the extensions. No configuration database
                X509V3_set_ctx_nodb(&ctx)

                // Issuer and subject certs: both the target since it is self-signed, no request and no CRL
                X509V3_set_ctx(&ctx, x509, x509, NULL, NULL, 0);
                ext = X509V3_EXT_conf_nid(NULL, &ctx, nid, data.c_str());
                if (not X509V3_EXT_conf_nid(NULL, &ctx, nid, data.c_str())) {
                    return;
                }

                X509_add_ext(x509, ext, -1);
                X509_EXTENSION_free(ext);
            };

            add_ext(NID_basic_constraints, "critical,CA:TRUE");
            add_ext(NID_key_usage, "critical,keyCertSign,cRLSign");
            add_ext(NID_subject_key_identifier, "hash");
            // TODO: NID_subject_key_identifier?

            // Now set the issuer name.
            X509_set_issuer_name(x509, name);

            // Actually sign the certificate with our key.
            if (!X509_sign(x509, *key, EVP_sha1())) {
                X509_free(x509);
                throw Exception("Error signing certificate.");
            }

            LOG_DEBUG("Successfully created an x509 certificate")
        }

        ~Certificate() {
            LOG_DEBUG("Freeing an x509 certificate")

            X509_free(x509);
        }

        X509* operator*() const {
            return x509;
        }
    };

    void write_ca_to_disk(const String& ca_name, const Key& key, const Certificate& cert) {
        const auto write = [&](const String& extension, const Function<int(FILE*)>& callback) {
            const auto file_name = ca_name + "." + extension;
            const auto file_path = (koalabox::paths::get_self_path() / file_name).string();

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
            return PEM_write_PrivateKey(stream, *key, NULL, NULL, 0, NULL, NULL);
        });

        write("crt", [&](FILE* stream) {
            return PEM_write_X509(stream, *cert);
        });
    }

    void start(
        const String& host,
        unsigned int port,
        const Map<String, httplib::Server::Handler>& pattern_handlers
    ) noexcept {
        std::thread([=]() {
            try {
                Key ca_key;
                Certificate ca_cert(ca_key, koalabox::globals::get_project_name());

                write_ca_to_disk(koalabox::globals::get_project_name(), ca_key, ca_cert);

                X509* cert; // TODO
                EVP_PKEY* key; // TODO

                httplib::SSLServer server(*ca_cert, *ca_key);

                for (const auto& [pattern, handler]: pattern_handlers) {
                    server.Get(pattern, handler);
                }

                if (not server.listen(host, port)) {
                    throw Exception("server.listen returned false");
                }
            } catch (const Exception& e) {
                koalabox::util::panic("Error starting http server: {}", e.what());
            }
        }).detach();
    }

}
