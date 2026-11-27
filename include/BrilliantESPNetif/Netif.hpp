/*
 * SPDX-FileCopyrightText: 2026 David Brill
 * SPDX-License-Identifier: MIT
 */

/**
 * @file Netif.hpp
 * @brief C++ ownership and configuration interface for ESP-NETIF instances.
 */

#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <system_error>

#include "esp_netif_ip_addr.h"
#include "esp_netif_types.h"
#include "sdkconfig.h"

namespace brilliant::ESPNetif {

  /** @brief Selects a link-local or global IPv6 address. */
  enum class Ip6Scope { LOCAL, GLOBAL };

  /** @brief Size of a hardware MAC address in bytes. */
  inline constexpr std::size_t MAC_ADDRESS_LEN{6};

  /** @brief Owns and configures one ESP-NETIF interface. */
  class Netif {
  public:
    Netif() = delete;

    /** @brief Releases the owned interface. */
    ~Netif();

    /** @brief Copying an interface owner is not supported. */
    Netif(const Netif&) = delete;

    /** @brief Copying an interface owner is not supported. */
    auto operator=(const Netif&) -> Netif& = delete;

    /** @brief Transfers ownership of an interface. */
    Netif(Netif&& other) noexcept;

    /** @brief Releases this interface and transfers ownership from @p other. */
    auto operator=(Netif&& other) noexcept -> Netif&;

    /**
     * @brief Creates an interface from an ESP-NETIF configuration.
     * @param config ESP-NETIF configuration.
     * @return The owning interface, or the ESP-IDF error.
     */
    static auto create(const esp_netif_config_t& config)
        -> std::expected<Netif, std::error_code>;

    /** @brief Creates and attaches the default Wi-Fi access-point interface. */
    static auto createDefaultWifiAp() -> std::expected<Netif, std::error_code>;

#ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
    /** @brief Creates and attaches the default Wi-Fi station interface. */
    static auto createDefaultWifiSta() -> std::expected<Netif, std::error_code>;
#endif

#ifdef CONFIG_ESP_WIFI_NAN_SYNC_ENABLE
    /** @brief Creates and attaches the default Wi-Fi NAN interface. */
    static auto createDefaultWifiNan() -> std::expected<Netif, std::error_code>;
#endif

    /** @brief Returns the borrowed ESP-IDF interface handle. */
    auto handle() -> esp_netif_t*;

    /** @brief Attaches an I/O driver to this interface. */
    auto attach(esp_netif_iodriver_handle driver) -> std::error_code;

    /** @brief Reports whether the interface is up. */
    auto isUp() -> bool;

    /** @brief Gets the interface MTU. */
    auto getMtu() -> std::expected<uint16_t, std::error_code>;

    /** @brief Sets the interface MTU. */
    auto setMtu(uint16_t) -> std::error_code;

#ifdef CONFIG_LWIP_DHCPS_REPORT_CLIENT_HOSTNAME
    /** @brief Gets the interface hostname as a non-owning view. */
    auto getHostname() -> std::expected<std::string_view, std::error_code>;

    /** @brief Sets the interface hostname. */
    auto setHostname(std::string_view hostname) -> std::error_code;
#endif

    /** @brief Gets the interface IPv4 address, netmask, and gateway. */
    auto getIpInfo() -> std::expected<esp_netif_ip_info_t, std::error_code>;

    /**
     * @brief Sets the interface IPv4 address, netmask, and gateway.
     * @note Stop DHCP client/server before changing IP information.
     */
    auto setIpInfo(const esp_netif_ip_info_t& info) -> std::error_code;

#if CONFIG_LWIP_IPV6
    /** @brief Gets a preferred IPv6 address in the requested scope. */
    auto getIp6(Ip6Scope scope)
        -> std::expected<esp_ip6_addr_t, std::error_code>;

    /**
     * @brief Copies interface IPv6 addresses into a span.
     * @param container Span with room for at least the configured address
     *        limit.
     * @param onlyPreferred If true, copies only preferred addresses.
     * @return Number of copied addresses, or an error.
     */
    auto getAllIp6(std::span<esp_ip6_addr_t> container,
                   bool onlyPreferred = false)
        -> std::expected<int, std::error_code>;

    /** @brief Adds an IPv6 address to the interface. */
    auto addIp6(const esp_ip6_addr_t& ip, bool preferred = true)
        -> std::error_code;

    /** @brief Removes an IPv6 address from the interface. */
    auto removeIp6(const esp_ip6_addr_t& ip) -> std::error_code;
#endif

    /** @brief Gets the interface MAC address. */
    auto getMacAddress()
        -> std::expected<std::array<uint8_t, MAC_ADDRESS_LEN>, std::error_code>;

    /** @brief Sets the interface MAC address. */
    auto setMacAddress(const std::array<uint8_t, MAC_ADDRESS_LEN>& mac)
        -> std::error_code;

  private:
    Netif(bool isDefault, esp_netif_t* handle);

    void reset() noexcept;

    bool _isDefault;
    esp_netif_t* _handle;
  };

}  // namespace brilliant::ESPNetif
