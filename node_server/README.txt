Instalacja wszystkich bibliotek potrzebnych do działania serwera:
> npm i

Uruchomienie servera:
> node index.js
W folderze node_server należy stworzyć plik config.js, w którym zamieścić chcianą konfigurację serwera.
Przykład zawartości pliku config.js:
=== Od tąd ===
const Config = {
    mqtt_user: "user",
    mqtt_pass: "password",
    mqtt_addr: "192.168.0.238",
    mqtt_port: 1883,
    http_port: 80,
    mb_addr: "192.168.0.102",
    mb_port: 502,
    mb_keepalive: 5000,
    mb_reconnectTimeout: 1000,
    mb_updateInterval: 1000,
}
module.exports = Config;
=== Do tąd ===
Po uruchomieniu, można otworzyć interfejs urzytkownika wpisując w przeglądarkę "localhost" albo "localhost:80"

Uruchomianie symulatora kamery:
> node camera_simulator.js

Konfiguracja Mosquitto:
Przy instalacji, jeśli nie odznaczyiście instalacji usługi windowsowej, to mosquitto startuje za każdym uruchomieniem komputera automatycznie.
Aby to wyłączyć należy (w folderze gdzie jest zainstalowany) wykonać:
> mosquitto uninstall
Należy stworzyć plik z hasłami (jak instalowaliście w C:/ProgramFiles to trzeba uruchomić cmd jako admin):
> mosquitto_passwd -c -b passwd.txt user password
W notatniku otworzyć plik konfituracyjny, nazwać go np. config.txt. Zawartość config.txt:
=== Od tąd ===
password_file passwd.txt
listener 1883
protocol mqtt
listener 8080
protocol websockets
=== Do tąd ===
Uruchamianie mosquitto:
> mosquitto -v -c config.txt
