#include <neo/wallets/nep6/scrypt_parameters.h>

namespace neo::wallets::nep6
{
    ScryptParameters::ScryptParameters()
        : n_(16384), r_(8), p_(8)
    {
    }

    ScryptParameters::ScryptParameters(uint32_t n, uint32_t r, uint32_t p)
        : n_(n), r_(r), p_(p)
    {
    }

    uint32_t ScryptParameters::GetN() const
    {
        return n_;
    }

    void ScryptParameters::SetN(uint32_t n)
    {
        n_ = n;
    }

    uint32_t ScryptParameters::GetR() const
    {
        return r_;
    }

    void ScryptParameters::SetR(uint32_t r)
    {
        r_ = r;
    }

    uint32_t ScryptParameters::GetP() const
    {
        return p_;
    }

    void ScryptParameters::SetP(uint32_t p)
    {
        p_ = p;
    }

    ScryptParameters ScryptParameters::Default()
    {
        return ScryptParameters(16384, 8, 8);
    }

    nlohmann::json ScryptParameters::ToJson() const
    {
        nlohmann::json json;
        json["n"] = n_;
        json["r"] = r_;
        json["p"] = p_;
        return json;
    }

    void ScryptParameters::FromJson(const nlohmann::json& json)
    {
        if (json.contains("n"))
            n_ = json["n"].get<uint32_t>();
        
        if (json.contains("r"))
            r_ = json["r"].get<uint32_t>();
        
        if (json.contains("p"))
            p_ = json["p"].get<uint32_t>();
    }
}
