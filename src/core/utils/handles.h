#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <type_traits>

namespace HandleDefaults {

/* Default internal data type used in handles. **MUST** be an integer type. */
using StorageType = uint32_t;

/* Dictates how wide the generation bit section should be. */
static constexpr size_t GenerationBits = std::numeric_limits<StorageType>::digits / 2;

}; // namespace HandleDefaults

/**
 * @brief Handle Metadata Class
 *
 * Provides basic configuration for handles via template
 * specialization.
 *
 * @code
 * template <>
 * struct HandleMeta<AudioSourceTag> {
 *     using StorageType = HandleDefaults::StorageType;
 *     static constexpr size_t IndexBits = std::bit_width(AudioLimits::MAX_VOICES + 1);
 *     static constexpr size_t GenerationBits = std::numeric_limits<StorageType>::digits - IndexBits;
 * };
 * @endcode
 *
 * @tparam Tag Which handle tag to configure.
 */
template <typename Tag>
struct HandleMeta {
    using StorageType = HandleDefaults::StorageType;
    static constexpr size_t GenerationBits = HandleDefaults::GenerationBits;
};

struct HandleCaster;

template <typename Tag>
struct Handle {
public:
    using tag = Tag;

    using StorageType = HandleMeta<Tag>::StorageType;
    static_assert(std::is_unsigned_v<StorageType> &&
                  std::is_integral_v<StorageType> &&
                  std::numeric_limits<StorageType>::is_integer);

public:
    constexpr Handle() = default;

    [[nodiscard]] static constexpr Handle<Tag> from_index_gen(size_t index, size_t gen) noexcept {
        if (index > INDEX_MAX || index == INDEX_INVALID) {
            return Handle<Tag>{};
        }

        auto converted_idx = static_cast<StorageType>(index) + 1;
        auto value = static_cast<StorageType>(converted_idx << GENERATION_BITS);
        value |= gen & GENERATION_MASK;
        return Handle<Tag>{value};
    }

    [[nodiscard]] static constexpr Handle from_index(size_t index) noexcept {
        return from_index_gen(index, 0);
    }

public:
    [[nodiscard]] constexpr size_t index() const noexcept {
        if (!valid()) {
            return INDEX_INVALID;
        }
        return ((m_value & INDEX_MASK) >> GENERATION_BITS) - 1;
    }

    [[nodiscard]] constexpr size_t gen() const noexcept {
        return (m_value & GENERATION_MASK);
    }

    [[nodiscard]] constexpr StorageType value() const noexcept {
        return m_value;
    }

    [[nodiscard]] constexpr bool valid() const noexcept {
        return (m_value & INDEX_MASK) != 0;
    }

public:
    [[nodiscard]] constexpr Handle inc_gen() const {
        StorageType new_value = (m_value + 1) & GENERATION_MASK;
        new_value |= m_value & INDEX_MASK;
        return Handle{new_value};
    }

    [[nodiscard]] constexpr Handle dec_gen() const {
        StorageType new_value = (m_value - 1) & GENERATION_MASK;
        new_value |= m_value & INDEX_MASK;
        return Handle{new_value};
    }

public:
    /* Conversion operators */

    /* Provides implicit conversion to ErasedHandle, aka Handle<void> */
    [[nodiscard]] constexpr operator Handle<void>() const noexcept
        requires(!std::is_void_v<Tag>)
    {
        return Handle<void>{m_value};
    }

    [[nodiscard]] constexpr explicit operator StorageType() const noexcept {
        return value();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return valid();
    }

public:
    /* Comparison operators */
    friend constexpr bool operator==(const Handle &a, const Handle &b) noexcept {
        return a.m_value == b.m_value;
    }

public:
    /* Split point between upper and lower bits */
    static constexpr StorageType GENERATION_BITS =
        HandleMeta<Tag>::GenerationBits;

    /* Upper bits stores index */
    static constexpr auto INDEX_MASK = static_cast<StorageType>(
        ~StorageType{0} << GENERATION_BITS);

    /* Lower bits stores generation */
    static constexpr auto GENERATION_MASK = static_cast<StorageType>(
        ~INDEX_MASK << 0);

    static constexpr auto INDEX_INVALID = std::numeric_limits<size_t>::max();

    /* Maximum accepted index (not count!) */
    static constexpr auto INDEX_MAX = static_cast<size_t>(INDEX_MASK >> GENERATION_BITS) - 1;

    /* Maximum accepted generation (not count!) */
    static constexpr auto GENERATION_MAX = static_cast<size_t>(GENERATION_MASK);

private:
    constexpr explicit Handle(StorageType value)
        : m_value(value) {}

    StorageType m_value;
    friend HandleCaster;
};

template struct Handle<void>;
using ErasedHandle = Handle<void>;

/* template specialization */
template <typename T>
struct is_handle : std::false_type {};
template <typename Tag>
struct is_handle<Handle<Tag>> : std::true_type {};
template <typename T>
inline constexpr bool is_handle_v = is_handle<T>::value;

/* HandleCaster has permission for casting two handle types */
struct HandleCaster {
    template <typename To, typename Tag>
    [[nodiscard]] static constexpr To cast(Handle<Tag> from) noexcept {
        return To{from.m_value};
    }
};

/**
 * @brief Provides explicit casting between handle types.
 *
 * @code
 *
 * Handle<UselessHandle> a;
 * auto b = handle_cast<TextureHandle>(b);
 *
 * @endcode
 *
 * @param from Handle to cast from
 * @return The newly casted handle type.
 */
template <typename To, typename Tag>
    requires is_handle_v<To>
[[nodiscard]] constexpr To handle_cast(Handle<Tag> from) noexcept {
    static_assert(std::is_same_v<typename To::StorageType,
        typename Handle<Tag>::StorageType>);
    static_assert(To::GENERATION_BITS == Handle<Tag>::GENERATION_BITS);
    return HandleCaster::cast<To>(from);
}

template <typename Tag>
struct std::formatter<Handle<Tag>> : std::formatter<std::string_view> {
    auto format(const Handle<Tag> &h, format_context &ctx) const {
        std::string s;
        if (h.valid()) {
            s = std::format("({}:{})", h.index(), h.gen());
        } else {
            s = "(null)";
        }
        return formatter<std::string_view>::format(s, ctx);
    }
};
