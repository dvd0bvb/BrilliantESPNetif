/*
 * SPDX-FileCopyrightText: 2026 David Brill
 * SPDX-License-Identifier: MIT
 */

/**
 * @file NetifError.hpp
 * @brief Error codes and categories for BrilliantESPNetif operations.
 */

#pragma once

#include <system_error>
#include <type_traits>

#include "esp_err.h"

namespace brilliant::ESPNetif {

  /** @brief Library-specific network interface errors. */
  enum class Errc : int { NETIF_NEW_FAILED, NETIF_INIT_FAILED };

  /** @brief Describes errors originating in BrilliantESPNetif. */
  class ErrorCategory : public std::error_category {
    [[nodiscard]] inline auto name() const noexcept -> const char* override {
      return "BrilliantESPNetifError";
    }

    [[nodiscard]] inline auto message(int e) const -> std::string override {
      switch (static_cast<Errc>(e)) {
      case Errc::NETIF_NEW_FAILED:
        return "esp_netif_new returned nullptr";
      case Errc::NETIF_INIT_FAILED:
        return "esp_netif_init returned ESP_FAIL";
      default:
        return "unknown error code";
      }
    }
  };

  /** @brief Adapts an ESP-IDF error value for std::error_code. */
  struct NetifErrc {
    esp_err_t err;
  };

  /** @brief Describes ESP-IDF network interface error codes. */
  class NetifErrorCategory : public std::error_category {
    [[nodiscard]] inline auto name() const noexcept -> const char* override {
      return "NetifError";
    }

    [[nodiscard]] inline auto message(int e) const -> std::string override {
      return esp_err_to_name(e);
    }
  };

}  // namespace brilliant::ESPNetif

namespace std {
  template <>
  struct is_error_code_enum<brilliant::ESPNetif::Errc> : std::true_type {};

  inline auto make_error_code(brilliant::ESPNetif::NetifErrc errc) noexcept
      -> error_code {
    static brilliant::ESPNetif::NetifErrorCategory category;
    return {static_cast<int>(errc.err), category};
  }

  template <>
  struct is_error_code_enum<brilliant::ESPNetif::NetifErrc> : true_type {};

  inline auto make_error_code(brilliant::ESPNetif::Errc errc) noexcept
      -> error_code {
    static brilliant::ESPNetif::ErrorCategory category;
    return {static_cast<int>(errc), category};
  }

}  // namespace std
