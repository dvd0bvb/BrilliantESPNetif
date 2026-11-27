/*
 * SPDX-FileCopyrightText: 2026 David Brill
 * SPDX-License-Identifier: MIT
 */

/**
 * @file Netif.cpp
 * @brief Implements ESP-NETIF ownership, creation, and configuration.
 */

#include "BrilliantESPNetif/Netif.hpp"

#include <expected>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>

#include "BrilliantESPNetif/NetifError.hpp"
#include "esp_netif.h"
#include "esp_netif_defaults.h"
#include "esp_netif_types.h"
#include "esp_wifi_default.h"

namespace brilliant::ESPNetif {

  Netif::~Netif() { reset(); }

  Netif::Netif(Netif&& other) noexcept
      : _isDefault(other._isDefault),
        _handle(std::exchange(other._handle, nullptr)) {}

  auto Netif::operator=(Netif&& other) noexcept -> Netif& {
    reset();
    _isDefault = other._isDefault;
    _handle = std::exchange(other._handle, nullptr);
    return *this;
  }

  auto Netif::create(const esp_netif_config_t& config)
      -> std::expected<Netif, std::error_code> {
    static std::once_flag onceFlag;
    static std::error_code initEc{};
    std::call_once(onceFlag, [] {
      if (const auto ret = esp_netif_init(); ret != ESP_OK) {
        initEc = std::make_error_code(Errc::NETIF_INIT_FAILED);
      }
    });

    if (initEc) {
      return std::unexpected(initEc);
    }

    if (auto handle = esp_netif_new(&config)) {
      return Netif(false, handle);
    } else {
      return std::unexpected(std::make_error_code(Errc::NETIF_NEW_FAILED));
    }
  }

  auto Netif::createDefaultWifiAp() -> std::expected<Netif, std::error_code> {
    const esp_netif_config_t config = ESP_NETIF_DEFAULT_WIFI_AP();
    auto result = create(config);
    if (!result) {
      return result;
    }
    result->_isDefault = true;

    if (auto ret = esp_netif_attach_wifi_ap(result->handle()); ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    if (auto ret = esp_wifi_set_default_wifi_ap_handlers(); ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    return result;
  }

#ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
  auto Netif::createDefaultWifiSta() -> std::expected<Netif, std::error_code> {
    const esp_netif_config_t config = ESP_NETIF_DEFAULT_WIFI_STA();
    auto result = create(config);
    if (!result) {
      return result;
    }
    result->_isDefault = true;

    if (auto ret = esp_netif_attach_wifi_station(result->handle());
        ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    if (auto ret = esp_wifi_set_default_wifi_sta_handlers(); ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    return result;
  }
#endif

#ifdef CONFIG_ESP_WIFI_NAN_SYNC_ENABLE
  auto Netif::createDefaultWifiNan() -> std::expected<Netif, std::error_code> {
    const esp_netif_config_t config = ESP_NETIF_DEFAULT_WIFI_NAN();
    auto result = create(config);
    if (!result) {
      return result;
    }
    result->_isDefault = true;

    if (auto ret = esp_netif_attach_wifi_nan(result->handle()); ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    if (auto ret = esp_wifi_set_default_wifi_nan_handlers(); ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    return result;
  }
#endif

  auto Netif::handle() -> esp_netif_t* { return _handle; }

  auto Netif::attach(esp_netif_iodriver_handle driverHandle)
      -> std::error_code {
    const auto ret = esp_netif_attach(_handle, driverHandle);
    return std::make_error_code(NetifErrc{ret});
  }

  auto Netif::isUp() -> bool { return esp_netif_is_netif_up(_handle); }

  auto Netif::getMtu() -> std::expected<uint16_t, std::error_code> {
    uint16_t mtu{};
    if (const auto ret = esp_netif_get_mtu(_handle, &mtu); ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    return mtu;
  }

  auto Netif::setMtu(uint16_t mtu) -> std::error_code {
    return std::make_error_code(NetifErrc{esp_netif_set_mtu(_handle, mtu)});
  }

#ifdef CONFIG_LWIP_DHCPS_REPORT_CLIENT_HOSTNAME
  auto Netif::getHostname()
      -> std::expected<std::string_view, std::error_code> {
    const char* hostname{};
    if (const auto ret = esp_netif_get_hostname(_handle, &hostname);
        ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    if (hostname == nullptr) {
      return std::unexpected(
          std::make_error_code(NetifErrc{ESP_ERR_ESP_NETIF_IF_NOT_READY}));
    }
    return std::string_view{hostname};
  }

  auto Netif::setHostname(std::string_view hostname) -> std::error_code {
    const std::string hostnameString{hostname};
    return std::make_error_code(
        NetifErrc{esp_netif_set_hostname(_handle, hostnameString.c_str())});
  }
#endif

  auto Netif::getIpInfo()
      -> std::expected<esp_netif_ip_info_t, std::error_code> {
    esp_netif_ip_info_t info{};
    if (const auto ret = esp_netif_get_ip_info(_handle, &info); ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    return info;
  }

  auto Netif::setIpInfo(const esp_netif_ip_info_t& info) -> std::error_code {
    return std::make_error_code(
        NetifErrc{esp_netif_set_ip_info(_handle, &info)});
  }

#if CONFIG_LWIP_IPV6
  auto Netif::getIp6(Ip6Scope scope)
      -> std::expected<esp_ip6_addr_t, std::error_code> {
    esp_ip6_addr_t address{};
    esp_err_t ret{};
    switch (scope) {
    case Ip6Scope::LOCAL:
      ret = esp_netif_get_ip6_linklocal(_handle, &address);
      break;
    case Ip6Scope::GLOBAL:
      ret = esp_netif_get_ip6_global(_handle, &address);
      break;
    default:
      return std::unexpected(
          std::make_error_code(NetifErrc{ESP_ERR_ESP_NETIF_INVALID_PARAMS}));
    }
    if (ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    return address;
  }

  auto Netif::getAllIp6(std::span<esp_ip6_addr_t> container, bool onlyPreferred)
      -> std::expected<int, std::error_code> {
    if (container.size() < CONFIG_LWIP_IPV6_NUM_ADDRESSES) {
      return std::unexpected(
          std::make_error_code(NetifErrc{ESP_ERR_ESP_NETIF_INVALID_PARAMS}));
    }

    if (onlyPreferred) {
      return esp_netif_get_all_preferred_ip6(_handle, container.data());
    } else {
      return esp_netif_get_all_ip6(_handle, container.data());
    }
  }

  auto Netif::addIp6(const esp_ip6_addr_t& ip, bool preferred)
      -> std::error_code {
    return std::make_error_code(
        NetifErrc{esp_netif_add_ip6_address(_handle, ip, preferred)});
  }

  auto Netif::removeIp6(const esp_ip6_addr_t& ip) -> std::error_code {
    return std::make_error_code(
        NetifErrc{esp_netif_remove_ip6_address(_handle, &ip)});
  }
#endif

  auto Netif::getMacAddress()
      -> std::expected<std::array<uint8_t, MAC_ADDRESS_LEN>, std::error_code> {
    std::array<uint8_t, MAC_ADDRESS_LEN> mac{};
    if (const auto ret = esp_netif_get_mac(_handle, mac.data());
        ret != ESP_OK) {
      return std::unexpected(std::make_error_code(NetifErrc{ret}));
    }
    return mac;
  }

  auto Netif::setMacAddress(const std::array<uint8_t, MAC_ADDRESS_LEN>& mac)
      -> std::error_code {
    auto mutableMac = mac;
    return std::make_error_code(
        NetifErrc{esp_netif_set_mac(_handle, mutableMac.data())});
  }

  Netif::Netif(bool isDefault, esp_netif_t* handle)
      : _isDefault(isDefault), _handle(handle) {}

  void Netif::reset() noexcept {
    if (_handle) {
      if (_isDefault) {
        esp_netif_destroy_default_wifi(_handle);
      } else {
        esp_netif_destroy(_handle);
      }
    }
  }

}  // namespace brilliant::ESPNetif