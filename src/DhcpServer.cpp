/*
 * SPDX-FileCopyrightText: 2026 David Brill
 * SPDX-License-Identifier: MIT
 */

/**
 * @file DhcpServer.cpp
 * @brief Implements DHCP server lifecycle, status, and lease lookup.
 */

#include "BrilliantESPNetif/DhcpServer.hpp"

#include <algorithm>
#include <limits>
#include <system_error>
#include <utility>
#include <vector>

#include "esp_netif.h"
#include "esp_netif_types.h"

#include "esp_log.h"

namespace brilliant::ESPNetif {

  DhcpServer::DhcpServer(Netif& netif) : _netif(&netif) {}

  DhcpServer::~DhcpServer() { stop(); }

  DhcpServer::DhcpServer(DhcpServer&& other) noexcept
      : _netif(std::exchange(other._netif, nullptr)),
        _captivePortalUri(std::move(other._captivePortalUri)) {}

  auto DhcpServer::operator=(DhcpServer&& other) noexcept -> DhcpServer& {
    stop();
    _netif = std::exchange(other._netif, nullptr);
    _captivePortalUri = std::move(other._captivePortalUri);
    return *this;
  }

  auto DhcpServer::start() -> std::error_code {
    ESP_LOGI("dhcps", "start() _netif: %x handle: %x", _netif, _netif ? _netif->handle() : nullptr);
    if (const auto ret = esp_netif_dhcps_start(_netif->handle());
        // already started is ok
        ret != ESP_OK && ret != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
      return std::make_error_code(NetifErrc{ret});
    }
    return std::error_code{};
  }

  auto DhcpServer::isRunning() -> bool {
    ESP_LOGI("dhcps", "isRunning() _netif: %x handle: %x", _netif, _netif ? _netif->handle() : nullptr);
    if (!_netif) {
      return false;
    }

    esp_netif_dhcp_status_t status{};
    // return esp_netif_dhcps_get_status(_netif->handle(), &status) == ESP_OK &&
    //        status == ESP_NETIF_DHCP_STARTED;
    const auto ret = esp_netif_dhcps_get_status(_netif->handle(), &status);

    ESP_LOGI("dhcps", "ret %d status %d", ret, (int)status);
    return status == ESP_NETIF_DHCP_STARTED;
  }

  auto DhcpServer::lookupLeases(std::span<esp_netif_pair_mac_ip_t> pairs)
      -> std::error_code {
    if (pairs.size() >
        static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      return std::make_error_code(NetifErrc{ESP_ERR_ESP_NETIF_INVALID_PARAMS});
    }
    if (pairs.empty()) {
      return {};
    }
    if (!_netif) {
      return std::make_error_code(NetifErrc{ESP_ERR_ESP_NETIF_INVALID_PARAMS});
    }
    if (const auto ret = esp_netif_dhcps_get_clients_by_mac(
            _netif->handle(), static_cast<int>(pairs.size()), pairs.data());
        ret != ESP_OK) {
      return std::make_error_code(NetifErrc{ret});
    }
    return {};
  }

  void DhcpServer::stop() {
    if (_netif) {
      esp_netif_dhcps_stop(_netif->handle());
    }
  }

}  // namespace brilliant::ESPNetif