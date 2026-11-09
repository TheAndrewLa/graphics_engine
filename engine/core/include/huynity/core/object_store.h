#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace huynity::core {

struct ObjectId {
    std::uint64_t slot{};
    std::uint64_t generation{};
    friend constexpr bool operator==(const ObjectId&, const ObjectId&) = default;
};

template <typename T>
    requires std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>
class ObjectStore {
public:
    [[nodiscard]] ObjectId insert(T value) {
        const std::size_t denseIndex = m_values.size();
        m_values.push_back(std::move(value));
        try {
            if (m_freeSlots.empty()) {
                const ObjectId id{m_slots.size(), 1};
                m_ids.push_back(id);
                try {
                    m_slots.push_back({id.generation, denseIndex});
                } catch (...) {
                    m_ids.pop_back();
                    throw;
                }
                return id;
            }

            const std::size_t slotIndex = m_freeSlots.back();
            const ObjectId id{slotIndex, m_slots[slotIndex].generation};
            m_ids.push_back(id);
            m_freeSlots.pop_back();
            m_slots[slotIndex].denseIndex = denseIndex;
            return id;
        } catch (...) {
            m_values.pop_back();
            throw;
        }
    }

    [[nodiscard]] bool contains(ObjectId id) const noexcept { return denseIndex(id) != invalidIndex; }

    [[nodiscard]] T* find(ObjectId id) noexcept {
        const std::size_t index = denseIndex(id);
        return index == invalidIndex ? nullptr : &m_values[index];
    }

    [[nodiscard]] const T* find(ObjectId id) const noexcept {
        const std::size_t index = denseIndex(id);
        return index == invalidIndex ? nullptr : &m_values[index];
    }

    bool remove(ObjectId id) {
        const std::size_t index = denseIndex(id);
        if (index == invalidIndex) {
            return false;
        }
        m_freeSlots.reserve(m_freeSlots.size() + 1);

        const std::size_t lastIndex = m_values.size() - 1;
        if (index != lastIndex) {
            m_values[index] = std::move(m_values[lastIndex]);
            m_ids[index] = m_ids[lastIndex];
            m_slots[m_ids[index].slot].denseIndex = index;
        }
        m_values.pop_back();
        m_ids.pop_back();

        auto& slot = m_slots[id.slot];
        ++slot.generation;
        slot.denseIndex = invalidIndex;
        m_freeSlots.push_back(id.slot);
        return true;
    }

    void reserveRemoval(std::size_t count) { m_freeSlots.reserve(m_freeSlots.size() + count); }

    [[nodiscard]] std::size_t size() const noexcept { return m_values.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_values.empty(); }
    [[nodiscard]] std::span<const T> values() const noexcept { return m_values; }
    [[nodiscard]] std::span<T> values() noexcept { return m_values; }
    [[nodiscard]] std::span<const ObjectId> ids() const noexcept { return m_ids; }

private:
    struct Slot {
        std::uint64_t generation{1};
        std::size_t denseIndex{invalidIndex};
    };

    static constexpr std::size_t invalidIndex = static_cast<std::size_t>(-1);

    [[nodiscard]] std::size_t denseIndex(ObjectId id) const noexcept {
        if (id.slot >= m_slots.size()) {
            return invalidIndex;
        }
        const auto& slot = m_slots[id.slot];
        if (slot.generation != id.generation || slot.denseIndex == invalidIndex) {
            return invalidIndex;
        }
        return slot.denseIndex;
    }

    std::vector<Slot> m_slots;
    std::vector<std::size_t> m_freeSlots;
    std::vector<T> m_values;
    std::vector<ObjectId> m_ids;
};

} // namespace huynity::core
