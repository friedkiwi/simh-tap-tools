#pragma once

#include "Error.h"

#include <utility>
#include <variant>

namespace tap {

template <typename T>
class Result {
public:
    static Result ok(T value)
    {
        return Result(std::move(value));
    }

    static Result fail(Error error)
    {
        return Result(std::move(error));
    }

    bool hasValue() const
    {
        return std::holds_alternative<T>(value_);
    }

    explicit operator bool() const
    {
        return hasValue();
    }

    const T& value() const
    {
        return std::get<T>(value_);
    }

    T& value()
    {
        return std::get<T>(value_);
    }

    const Error& error() const
    {
        return std::get<Error>(value_);
    }

private:
    explicit Result(T value)
        : value_(std::move(value))
    {
    }

    explicit Result(Error error)
        : value_(std::move(error))
    {
    }

    std::variant<T, Error> value_;
};

template <>
class Result<void> {
public:
    static Result ok()
    {
        return Result();
    }

    static Result fail(Error error)
    {
        return Result(std::move(error));
    }

    bool hasValue() const
    {
        return !error_.has_value;
    }

    explicit operator bool() const
    {
        return hasValue();
    }

    const Error& error() const
    {
        return error_.error;
    }

private:
    struct ErrorStorage {
        bool has_value = false;
        Error error;
    };

    Result() = default;

    explicit Result(Error error)
        : error_{true, std::move(error)}
    {
    }

    ErrorStorage error_;
};

} // namespace tap
