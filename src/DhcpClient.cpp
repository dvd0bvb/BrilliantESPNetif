/*
 * SPDX-FileCopyrightText: 2026 David Brill
 * SPDX-License-Identifier: MIT
 */

/**
 * @file DhcpClient.cpp
 * @brief Implements DHCP client lifecycle and status operations.
 */

#include "BrilliantESPNetif/DhcpClient.hpp"

#include <system_error>
#include <utility>

#include "BrilliantESPNetif/NetifError.hpp"
#include "esp_netif.h"
#include "esp_netif_types.h"

namespace brilliant::ESPNetif {

  DhcpClient::DhcpClient(Netif& netif) : _netif(&netif) {}

  DhcpClient::~DhcpClient() { stop(); }

  DhcpClient::DhcpClient(DhcpClient&& other) noexcept
      : _netif(std::exchange(other._netif, nullptr)) {}

  auto DhcpClient::operator=(DhcpClient&& other) noexcept -> DhcpClient& {
    _netif = std::exchange(other._netif, nullptr);
    return *this;
  }

  auto DhcpClient::start() -> std::error_code {
    if (const auto ret = esp_netif_dhcpc_start(_netif->handle());
        ret != ESP_OK && ret != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
      return std::make_error_code(NetifErrc{ret});
    }
    return std::error_code{};
  }

  auto DhcpClient::isRunning() -> bool {
    if (!_netif) {
      return false;
    }

    esp_netif_dhcp_status_t status{};
    return esp_netif_dhcpc_get_status(_netif->handle(), &status) == ESP_OK &&
           status == ESP_NETIF_DHCP_STARTED;
  }

  void DhcpClient::stop() {
    if (_netif) {
      esp_netif_dhcpc_stop(_netif->handle());
    }
  }

}  // namespace brilliant::ESPNetif