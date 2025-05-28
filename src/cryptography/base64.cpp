#include <neo/cryptography/base64.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <stdexcept>

namespace neo::cryptography
{
    std::string Base64::Encode(const io::ByteSpan& data)
    {
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO* bmem = BIO_new(BIO_s_mem());
        b64 = BIO_push(b64, bmem);
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(b64, data.Data(), static_cast<int>(data.Size()));
        BIO_flush(b64);
        
        BUF_MEM* bptr;
        BIO_get_mem_ptr(b64, &bptr);
        
        std::string result(bptr->data, bptr->length);
        BIO_free_all(b64);
        
        return result;
    }
    
    std::string Base64::Encode(const std::string& data)
    {
        return Encode(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }
    
    io::ByteVector Base64::Decode(const std::string& data)
    {
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO* bmem = BIO_new_mem_buf(data.c_str(), static_cast<int>(data.length()));
        bmem = BIO_push(b64, bmem);
        BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);
        
        io::ByteVector result(data.length());
        int decodedSize = BIO_read(bmem, result.Data(), static_cast<int>(result.Size()));
        BIO_free_all(bmem);
        
        if (decodedSize < 0)
            throw std::runtime_error("Failed to decode base64");
        
        result.Resize(decodedSize);
        return result;
    }
    
    std::string Base64::DecodeToString(const std::string& data)
    {
        auto bytes = Decode(data);
        return std::string(reinterpret_cast<const char*>(bytes.Data()), bytes.Size());
    }
}
