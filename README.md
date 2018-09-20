# espidf-ble-adv-scan-wifi
MH-ET Live ESP32 Minikit (ESP-WROOM-32)を用いて BLE の advertising packet の送受信を行うと共に、WiFiでアクセスポイントとしてビーコンを送信しつつ、他のアクセスポイントからのビーコン情報も収集する。
収集した BLE および WiFiの RSSI測定データを(コンソールに)出力する。

Platform IDE for Visual Studio Code を開発環境とし、フレームワークに ESP-IDFを利用した ESP32 プログラム。

もう少し詳しい説明が[薫染庵 途上日誌](https://kunsen.net/)にある。


BLE central としての advertising packet の収集は一定時間行った後に、休止時間をプログラムでは含めているが、プログラムを変更すれば連続的に情報を収集することができるはず。

WiFiにはチャンネルが複数あり、一般的なWiFiクライアントでは一時に1つのチャンネルしか監視できないので、すべてのチャンネルの情報を収集するには順次調べる必要がある。ESP-IDFの関数で自動的にすべてのチャンネルの情報が収集されるが、出力はまとめて行われる。


# ファイル
各ファイルの主な役割は以下の通り。

* main.c
    * BLEとWiFiの情報を収集するためのメインプログラム


* ble_adv_scan.c
    * BLE のアドバタイジングパケットの送信を行うと共にスキャンを行う
    * 次のコードを基にしている
	https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/gatt_client/main/gattc_demo.c
	https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/gatt_server/main/gatts_demo.c

* wifi_scan.c
    * WiFi のアクセスポイントとしてビーコンを送信すると共に、スキャンし、RSSI などの情報を収集する
    * 次のコードを基にしている
	https://github.com/espressif/esp-idf/blob/master/examples/wifi/scan/main/scan.c
