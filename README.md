[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) <a href="https://www.buymeacoffee.com/dvd0bvb" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height:20px !important;width: 80px !important;" ></a>

# BrilliantESPNetif

BrilliantESPNetif is a modern C++ wrapper around ESP-IDF's `esp_netif` component. It provides RAII-managed network interfaces, helpers for common interface configuration and address operations, and DHCP client/server wrappers. Operations that can fail report errors using `std::error_code` or `std::expected`.

The component also includes a registry for storing and finding interfaces by name. It is intended for ESP-IDF applications that want a C++ interface to `esp_netif` while retaining access to the underlying ESP-IDF handles and APIs.
