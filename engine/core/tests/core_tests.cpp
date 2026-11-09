#include "huynity/core/object_store.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace core = huynity::core;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

void identityAndStaleIds() {
    core::ObjectStore<int> store;
    CHECK(store.empty());
    const auto first = store.insert(10);
    const auto second = store.insert(20);
    CHECK(first != second);
    CHECK(store.size() == 2);
    CHECK(store.contains(first));
    CHECK(store.find(first) != nullptr && *store.find(first) == 10);

    const core::ObjectId wrongGeneration{first.slot, first.generation + 1};
    const core::ObjectId wrongSlot{first.slot + 100, first.generation};
    CHECK(!store.contains(wrongGeneration));
    CHECK(store.find(wrongGeneration) == nullptr);
    CHECK(!store.remove(wrongSlot));
    CHECK(store.size() == 2);

    CHECK(store.remove(first));
    CHECK(!store.contains(first));
    CHECK(store.find(first) == nullptr);
    CHECK(!store.remove(first));
    CHECK(store.contains(second));
}

void swapRemoveAndSlotReuse() {
    core::ObjectStore<std::string> store;
    const auto first = store.insert("first");
    const auto middle = store.insert("middle");
    const auto last = store.insert("last");
    CHECK(store.remove(middle));
    CHECK(store.size() == 2);
    CHECK(store.ids()[0] == first);
    CHECK(store.ids()[1] == last);
    CHECK(store.values()[1] == "last");
    CHECK(store.find(last) != nullptr && *store.find(last) == "last");
    CHECK(!store.contains(middle));

    const auto replacement = store.insert("replacement");
    CHECK(replacement.slot == middle.slot);
    CHECK(replacement.generation == middle.generation + 1);
    CHECK(replacement != middle);
    CHECK(!store.contains(middle));
    CHECK(store.find(replacement) != nullptr && *store.find(replacement) == "replacement");
    CHECK(store.find(first) != nullptr && *store.find(first) == "first");
    CHECK(store.find(last) != nullptr && *store.find(last) == "last");
}

void denseStorage() {
    core::ObjectStore<std::uint32_t> store;
    for (std::uint32_t value = 0; value < 100; ++value) {
        CHECK(store.insert(value).slot == value);
    }
    for (std::uint32_t value = 0; value < 100; value += 2) {
        CHECK(store.remove(store.ids()[0]));
    }
    CHECK(store.size() == 50);
    for (std::size_t i = 0; i < store.size(); ++i) {
        CHECK(store.find(store.ids()[i]) == &store.values()[i]);
    }
}

void lastRemovalAndMoveOnlyValues() {
    core::ObjectStore<std::unique_ptr<int>> store;
    const auto first = store.insert(std::make_unique<int>(1));
    const auto second = store.insert(std::make_unique<int>(2));
    CHECK(store.remove(first));
    CHECK(store.find(second) != nullptr && *store.find(second) != nullptr && **store.find(second) == 2);
    CHECK(store.remove(second));
    CHECK(store.empty());
    const auto replacement = store.insert(std::make_unique<int>(3));
    CHECK(store.remove(replacement));
    CHECK(store.empty());
}

int main() {
    try {
        identityAndStaleIds();
        swapRemoveAndSlotReuse();
        denseStorage();
        lastRemovalAndMoveOnlyValues();
        std::cout << "storage: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
