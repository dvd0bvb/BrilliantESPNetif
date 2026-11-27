/*
 * SPDX-FileCopyrightText: 2026 David Brill
 * SPDX-License-Identifier: MIT
 */

/**
 * @file NetifRegistry.cpp
 * @brief Implements registry operations for managed ESP-NETIF instances.
 */

#include "BrilliantESPNetif/NetifRegistry.hpp"

#include <expected>
#include "esp_netif.h"

namespace brilliant::ESPNetif {

  auto NetifRegistry::instance() -> NetifRegistry& {
    static NetifRegistry instance;
    return instance;
  }

#ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
  auto NetifRegistry::emplaceDefaultWifiAp()
      -> std::expected<Netif*, std::error_code> {
    return emplace(&Netif::createDefaultWifiAp);
  }
#endif

  auto NetifRegistry::emplaceDefaultWifiSta()
      -> std::expected<Netif*, std::error_code> {
    return emplace(&Netif::createDefaultWifiSta);
  }

#ifdef CONFIG_ESP_WIFI_NAN_SYNC_ENABLE
  auto NetifRegistry::emplaceDefaultWifiNan()
      -> std::expected<Netif*, std::error_code> {
    return emplace(&Netif::createDefaultWifiNan);
  }
#endif

  auto NetifRegistry::emplaceConfig(const esp_netif_config_t& config)
      -> std::expected<Netif*, std::error_code> {
    return emplace(&Netif::create, config);
  }

  auto NetifRegistry::find(std::string_view name) -> Netif* {
    auto netif = esp_netif_get_handle_from_ifkey(name.data());
    if (!netif) {
      return nullptr;
    }

    if (auto found = std::find_if(_netifStorage.begin(), _netifStorage.end(), [netif](auto&& n) {
      return n->handle() == netif;
    }); found != _netifStorage.end()) {
      return found->get();
    }
    return nullptr;
  }

  void NetifRegistry::erase(std::string_view name) {
    auto netif = esp_netif_get_handle_from_ifkey(name.data());
    if (!netif) {
      return;
    }

    if (auto found = std::find_if(_netifStorage.begin(), _netifStorage.end(), [netif](auto&& n) {
      return n->handle() == netif;
    }); found != _netifStorage.end()) {
      _netifStorage.erase(found);
    }
  }

  void NetifRegistry::clear() { _netifStorage.clear(); }

}  // namespace brilliant::ESPNetif