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
}
module.exports = Config;
=== Do tąd ===
Po uruchomieniu, można otworzyć interfejs urzytkownika wpisując w przeglądarkę "localhost" albo "localhost:80"

Uruchomianie symulatora kamery:
> node camera_simulator.js

