
#include "PithyEvmc.h"

#include "../devcommon/util.h"
#include "../libdevcore/RLP.h"

//#include <libdevcore/Log.h>
//#include <libevm/VMFactory.h>

#include <evmc/loader.h>
#include <libaleth-interpreter/interpreter.h>

namespace dev
{
namespace eth
{

/////////////////////////////////
inline evmc::address toEvmC(Address const& _addr)
{
    return reinterpret_cast<evmc_address const&>(_addr);
}

inline evmc_uint256be toEvmC(h256 const& _h)
{
    return reinterpret_cast<evmc_uint256be const&>(_h);
}

inline u256 fromEvmC(evmc_uint256be const& _n)
{
    return fromBigEndian<u256>(_n.bytes);
}

inline Address fromEvmC(evmc::address const& _addr)
{
    return reinterpret_cast<Address const&>(_addr);
}

inline std::string toString(const evmc_address& _a)
{
    return fromEvmC(_a).hex();
}

inline std::string toString(const evmc_uint256be& _n)
{
    return u256_to_hex(fromEvmC(_n));
}

inline std::string toString(const uint8_t* _p, const std::size_t _size)
{
    std::string s;
    for (std::size_t i = 0; i < _size; i++)
    {
        char buf[12] = {};
        sprintf(buf, "%2.2x", _p[i]);
        s += buf;
    }
    return s;
}

PithyEVMC::PithyEVMC(evmc_vm* _vm)
  : evmc::VM(_vm)
{
}

PVMPtr PithyEVMC::createVm()
{
    return std::shared_ptr<PithyEVMC>(new PithyEVMC(evmc_create_aleth_interpreter()));
}

evmc::result PithyEVMC::exec(evmc::Host& host, const bool fCreate, const uint64_t gas, const evmc::address& destination, const evmc::address& sender,
                             const evmc::uint256be& value, const bytes& data, const bytes& code)
{
    evmc_message msg = {
        .kind = EVMC_CALL,
        .flags = 0,
        .depth = 0,
        .gas = gas,
        .destination = destination,
        .sender = sender,
        .input_data = data.data(),
        .input_size = data.size(),
        .value = value,
        .create2_salt = toEvmC(0x0_cppui256)
    };

    return execute(host, EVMC_MAX_REVISION, msg, code.data(), code.size());
}

} // namespace eth
} // namespace dev
