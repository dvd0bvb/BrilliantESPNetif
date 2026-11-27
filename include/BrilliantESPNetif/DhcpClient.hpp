/*
 * SPDX-FileCopyrightText: 2026 David Brill
 * SPDX-License-Identifier: MIT
 */

/**
 * @file DhcpClient.hpp
 * @brief RAII wrapper for ESP-NETIF DHCP client operations.
 */

#pragma once

#include <cstddef>
#include <span>
#include <system_error>

#include "BrilliantESPNetif/DhcpServer.hpp"
#include "BrilliantESPNetif/Netif.hpp"
#include "BrilliantESPNetif/NetifError.hpp"
#include "esp_netif.h"
#include "esp_netif_types.h"

namespace brilliant::ESPNetif {

  namespace detail {
    template <esp_netif_dhcp_option_id_t Option>
    struct DhcpcOptionTraits {};

    template <>
    struct DhcpcOptionTraits<ESP_NETIF_IP_REQUEST_RETRY_TIME> {
      using SetType = uint8_t;
      using GetType = uint8_t&;
    };

#if !ESP_DHCP_DISABLE_VENDOR_CLASS_IDENTIFIER
    template <>
    struct DhcpcOptionTraits<ESP_NETIF_VENDOR_SPECIFIC_INFO> {
      using SetType = std::span<std::byte>;
      using GetType = std::span<std::byte>;
    };
#endif

  }  // namespace detail

  /** @brief Controls the DHCP client associated with a network interface. */
  class DhcpClient {
  public:
    /**
     * @brief Creates a client wrapper for @p netif.
     * @note The network interface must outlive this object.
     */
    DhcpClient(Netif& netif);

    /** @brief Stops the client if this wrapper owns a netif reference. */
    ~DhcpClient();

    DhcpClient(const DhcpClient&) = delete;

    auto operator=(const DhcpClient&) -> DhcpClient& = delete;

    /** @brief Transfers the wrapper's netif reference. */
    DhcpClient(DhcpClient&& other) noexcept;

    /**
     * @brief Stops this client and transfers the netif reference from @p other.
     */
    auto operator=(DhcpClient&& other) noexcept -> DhcpClient&;

    /** @brief Starts the DHCP client. */
    auto start() -> std::error_code;

    /** @brief Stops the DHCP client. */
    void stop();

    /** @brief Reports whether the DHCP client status is started. */
    auto isRunning() -> bool;

    /** @brief Sets a supported DHCP option through ESP-NETIF. */
    template <esp_netif_dhcp_option_id_t Option>
    auto setOption(typename detail::DhcpsOptionTraits<Option>::SetType value)
        -> std::error_code {
      if constexpr (Option == ESP_NETIF_VENDOR_SPECIFIC_INFO) {
        const auto ret = esp_netif_dhcps_option(
            _netif->handle(), ESP_NETIF_OP_SET, Option, value.data(),
            static_cast<uint32_t>(value.size()));
        return std::make_error_code(NetifErrc{ret});
      } else {
        const auto ret = esp_netif_dhcps_option(
            _netif->handle(), ESP_NETIF_OP_SET, Option, &value,
            static_cast<uint32_t>(sizeof(value)));
        return std::make_error_code(NetifErrc{ret});
      }
    }

    /** @brief Reads a supported DHCP option into the supplied value. */
    template <esp_netif_dhcp_option_id_t Option>
    auto getOption(typename detail::DhcpsOptionTraits<Option>::SetType value)
        -> std::error_code {
      if constexpr (Option == ESP_NETIF_VENDOR_SPECIFIC_INFO) {
        const auto ret = esp_netif_dhcps_option(
            _netif->handle(), ESP_NETIF_OP_GET, Option, value.data(),
            static_cast<uint32_t>(value.size()));
        return std::make_error_code(NetifErrc{ret});
      } else {
        const auto ret = esp_netif_dhcps_option(
            _netif->handle(), ESP_NETIF_OP_GET, Option, &value,
            static_cast<uint32_t>(sizeof(value)));
        return std::make_error_code(NetifErrc{ret});
      }
    }

  private:
    Netif* _netif;
  };

}  // namespace brilliant::ESPNetif