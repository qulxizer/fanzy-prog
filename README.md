# fanzy-prog

## Configuration

The Wi-Fi access point is implemented in `components/wifi_access_point` and is
enabled by default with the SSID `fanzy-prog`. Configure it with:

```text
idf.py menuconfig
Fanzy Wi-Fi access point
```

Enable the access point and set its SSID and password before building. The
password may be empty for an open network, or must contain at least eight
characters for WPA2. Credentials are supplied through `sdkconfig` and are not
stored in source code.
