
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

//#include "VMFace.h"

#include <libdevcore/Common.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/SHA3.h>

#include "Instruction.h"
//#include <libethcore/BlockHeader.h>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/Common.h>
#include <libethcore/EVMSchedule.h>
//#include <libethcore/LogEntry.h>

#include <boost/optional.hpp>
#include <evmc/evmc.hpp>
#include <functional>
#include <set>

namespace dev
{
namespace eth
{

/////////////////////////////////
inline evmc::address EthToEvmC(Address const& _addr)
{
    return reinterpret_cast<evmc_address const&>(_addr);
}

inline evmc_uint256be EthToEvmC(h256 const& _h)
{
    return reinterpret_cast<evmc_uint256be const&>(_h);
}

inline u256 EthFromEvmC(evmc_uint256be const& _n)
{
    return fromBigEndian<u256>(_n.bytes);
}

inline Address EthFromEvmC(evmc::address const& _addr)
{
    return reinterpret_cast<Address const&>(_addr);
}

inline u256 u256FromEvmC(evmc_uint256be const& _n)
{
    return fromBigEndian<u256>(_n.bytes);
}

/////////////////////////////////
class PithyEVMC;

using PVMPtr = std::shared_ptr<PithyEVMC>;

/////////////////////////////////
// PithyEVMC

class PithyEVMC : public evmc::VM
{
public:
    PithyEVMC(evmc_vm* _vm);

    static PVMPtr createVm();

    evmc::result exec(evmc::Host& host, const bool fCreate, const uint64_t gas, const evmc::address& destination, const evmc::address& sender,
                      const evmc::uint256be& value, const bytes& data, const bytes& code);
};

} // namespace eth
} // namespace dev
