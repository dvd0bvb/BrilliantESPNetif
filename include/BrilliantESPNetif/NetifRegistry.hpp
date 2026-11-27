/*
 * SPDX-FileCopyrightText: 2026 David Brill
 * SPDX-License-Identifier: MIT
 */

/**
 * @file NetifRegistry.hpp
 * @brief Registry for named, owned ESP-NETIF interfaces.
 */

#pragma once

#include <concepts>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

#include "BrilliantESPNetif/Netif.hpp"
#include "esp_netif_types.h"

namespace brilliant::ESPNetif {

  inline constexpr std::string_view DEFAULT_WIFI_STA_NAME = "WIFI_STA_DEF";
  inline constexpr std::string_view DEFAULT_WIFI_AP_NAME = "WIFI_AP_DEF";
  inline constexpr std::string_view DEFAULT_WIFI_NAN_NAME = "WIFI_NAN_DEF";
  inline constexpr std::string_view DEFAULT_ETH_NAME = "ETH_DEF";
  // TODO: david - BR0, BR1, PPP

  /** @brief Owns and looks up network interfaces created through the registry. */
  class NetifRegistry {
  public:
    /** @brief Returns the process-wide registry instance. */
    static auto instance() -> NetifRegistry&;

    /** @brief Destroys all interfaces owned by this registry. */
    ~NetifRegistry() = default;

    NetifRegistry(const NetifRegistry&) = delete;

    auto operator=(const NetifRegistry&) -> NetifRegistry& = delete;

    NetifRegistry(NetifRegistry&&) = default;

    auto operator=(NetifRegistry&&) -> NetifRegistry& = default;

    /** @brief Creates an interface and adds it to the registry. */
    template <class Fn, class... Args>
      requires std::same_as<std::invoke_result_t<Fn, Args...>,
                            std::expected<Netif, std::error_code>>
    auto emplace(Fn fn, Args&&... args)
        -> std::expected<Netif*, std::error_code> {
      if (auto result = fn(std::forward<Args>(args)...)) {
        auto& item = _netifStorage.emplace_back(
            std::make_unique<Netif>(std::move(*result)));
        return item.get();
      } else {
        return std::unexpected(result.error());
      }
    }

#ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
    /** @brief Creates and registers the default Wi-Fi access-point interface. */
    auto emplaceDefaultWifiAp() -> std::expected<Netif*, std::error_code>;
#endif

    /** @brief Creates and registers the default Wi-Fi station interface. */
    auto emplaceDefaultWifiSta() -> std::expected<Netif*, std::error_code>;

#ifdef CONFIG_ESP_WIFI_NAN_SYNC_ENABLE
    /** @brief Creates and registers the default Wi-Fi NAN interface. */
    auto emplaceDefaultWifiNan() -> std::expected<Netif*, std::error_code>;
#endif

    /** @brief Creates and registers an interface from @p config. */
    auto emplaceConfig(const esp_netif_config_t& config)
        -> std::expected<Netif*, std::error_code>;

    /** @brief Finds an interface by its ESP-NETIF interface key. */
    auto find(std::string_view name) -> Netif*;

    /** @brief Removes and destroys the registered interface with the given key. */
    void erase(std::string_view name);

    /** @brief Removes and destroys all registered interfaces. */
    void clear();

  private:
    NetifRegistry() = default;

    std::vector<std::unique_ptr<Netif>>
        _netifStorage;  // use unique_ptr for pointer/reference stability
  };

}  // namespace brilliant::ESPNetif
