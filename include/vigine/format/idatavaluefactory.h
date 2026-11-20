#pragma once

#include "vigine/format/idatavalue.h"
#include "vigine/format/vigineformat_export.h"

#include <cstdint>
#include <memory>
#include <string_view>

namespace vigine::format
{
class VIGINEFORMAT_API IDataValueFactory
{
  public:
    virtual ~IDataValueFactory();

    [[nodiscard]] virtual std::unique_ptr<IDataValue> table() const = 0;
    [[nodiscard]] virtual std::unique_ptr<IDataValue> array() const = 0;
    [[nodiscard]] virtual std::unique_ptr<IDataValue> string(std::string_view text) const = 0;
    [[nodiscard]] virtual std::unique_ptr<IDataValue> integer(std::int64_t number) const = 0;
    [[nodiscard]] virtual std::unique_ptr<IDataValue> floating(double number) const = 0;
    [[nodiscard]] virtual std::unique_ptr<IDataValue> boolean(bool flag) const = 0;
    [[nodiscard]] virtual std::unique_ptr<IDataValue> null() const = 0;
};

[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDataValueFactory> createDataValueFactory();
} // namespace vigine::format
