/*
 * SPDX-FileCopyrightText: 2026 David Brill
 * SPDX-License-Identifier: MIT
 */

/**
 * @file DhcpServer.hpp
 * @brief RAII wrapper for ESP-NETIF DHCP server operations.
 */

#pragma once

#include <expected>
#include <optional>
#include <span>
#include <string_view>

#include "BrilliantESPNetif/Netif.hpp"
#include "BrilliantESPNetif/NetifError.hpp"
#include "dhcpserver/dhcpserver.h"
#include "esp_netif.h"
#include "esp_netif_types.h"

namespace brilliant::ESPNetif {

  namespace detail {

    template <esp_netif_dhcp_option_id_t Option>
    struct DhcpsOptionTraits {};

    template <>
    struct DhcpsOptionTraits<ESP_NETIF_IP_ADDRESS_LEASE_TIME> {
      using SetType = uint32_t;
      using GetType = uint32_t;
    };

    template <>
    struct DhcpsOptionTraits<ESP_NETIF_SUBNET_MASK> {
      using SetType = dhcps_lease_t&;
      using GetType = dhcps_lease_t;
    };

    template <>
    struct DhcpsOptionTraits<ESP_NETIF_REQUESTED_IP_ADDRESS>
        : DhcpsOptionTraits<ESP_NETIF_SUBNET_MASK> {};

    template <>
    struct DhcpsOptionTraits<ESP_NETIF_ROUTER_SOLICITATION_ADDRESS> {
      using SetType = dhcps_offer_t&;
      using GetType = dhcps_offer_t;
    };

    template <>
    struct DhcpsOptionTraits<ESP_NETIF_DOMAIN_NAME_SERVER> {
      using SetType = dhcps_offer_t&;
      using GetType = dhcps_offer_t;
    };

    template <>
    struct DhcpsOptionTraits<ESP_NETIF_CAPTIVEPORTAL_URI> {
      using SetType = std::string_view;
    };
  }  // namespace detail

  /** @brief Controls the DHCP server associated with a network interface. */
  class DhcpServer {
  public:
    /**
     * @brief Creates a server wrapper for @p _netif.
     * @note The network interface must outlive this object.
     */
    DhcpServer(Netif& _netif);

    /** @brief Stops the server if this wrapper owns a netif reference. */
    ~DhcpServer();

    DhcpServer(const DhcpServer&) = delete;

    auto operator= (const DhcpServer&) -> DhcpServer& = delete;

    /** @brief Transfers the wrapper's netif reference and URI storage. */
    DhcpServer(DhcpServer&& other) noexcept;

    /** @brief Stops this server and transfers state from @p other. */
    auto operator= (DhcpServer&& other) noexcept -> DhcpServer&;

    /** @brief Starts the DHCP server. */
    auto start() -> std::error_code;

    /** @brief Stops the DHCP server. */
    void stop();

    /** @brief Reports whether the DHCP server status is started. */
    auto isRunning() -> bool;

    /**
     * @brief Looks up lease addresses for the MAC addresses in @p pairs.
     * @param pairs Each pair supplies a MAC address and receives its lease IP.
     */
    auto lookupLeases(std::span<esp_netif_pair_mac_ip_t> pairs)
        -> std::error_code;

    /** @brief Sets a supported DHCP server option. */
    template <esp_netif_dhcp_option_id_t Option>
    auto setOption(typename detail::DhcpsOptionTraits<Option>::SetType value)
        -> std::error_code {
      if constexpr (Option == ESP_NETIF_CAPTIVEPORTAL_URI) {
        _captivePortalUri->assign(value.c_str(), value.size());
        const auto ret = esp_netif_dhcps_option(
            _netif->handle(), ESP_NETIF_OP_SET, Option,
            _captivePortalUri->data(),
            static_cast<uint32_t>(_captivePortalUri->size()));
        return std::make_error_code(NetifErrc{ret});
      } else {
        const auto ret = esp_netif_dhcps_option(
            _netif->handle(), ESP_NETIF_OP_SET, Option, &value,
            static_cast<uint32_t>(sizeof(value)));
        return std::make_error_code(NetifErrc{ret});
      }
    }

    /** @brief Gets a supported DHCP server option. */
    template <esp_netif_dhcp_option_id_t Option>
    auto getOption()
        -> std::expected<typename detail::DhcpsOptionTraits<Option>::GetType,
                         std::error_code> {
      typename detail::DhcpsOptionTraits<Option>::GetType value{};
      if (const auto ret = esp_netif_dhcps_option(
              _netif->handle(), ESP_NETIF_OP_GET, Option, &value,
              static_cast<uint32_t>(sizeof(value)));
          ret != ESP_OK) {
        return std::unexpected(std::make_error_code(NetifErrc{ret}));
      }
      return value;
    }

  private:
    Netif* _netif;

    std::optional<std::string> _captivePortalUri;
  };

}  // namespace brilliant::ESPNetif
