idf_component_register(
  SRCS
    src/Netif.cpp 
    src/DhcpServer.cpp 
    src/DhcpClient.cpp
    src/NetifRegistry.cpp
  INCLUDE_DIRS
    "include"
  REQUIRES 
    esp_netif
    esp_wifi
    # TODO eventually depend on EventLoop wrapper
)