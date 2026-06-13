![License](https://img.shields.io/badge/License-MIT-lightgrey)
![Platform](https://img.shields.io/badge/ESP32--S3-black)
![Arduino](https://img.shields.io/badge/Arduino-IDE-black)
![MQTT](https://img.shields.io/badge/MQTT-enabled-black)

# CPU‑MONITOR‑OpenNextion
## 🖥️ Vista del panel en Open Nextion

![CPU-MONITOR en Open Nextion](images/cpu-monitor.jpg)


CPU‑MONITOR es un proyecto en Arduino diseñado para mostrar en una pantalla Open Nextion los datos internos del NestDisk, publicados previamente en Home Assistant mediante MQTT. El objetivo es ofrecer un panel visual autónomo, fluido y moderno que permita consultar en tiempo real el estado del sistema.

El proyecto nació durante la revisión del NestDisk, cuando aprendimos a acceder a sus métricas internas y a publicarlas en Home Assistant usando MQTT. A partir de ahí, visualizar esos datos en una pantalla externa fue solo cuestión de elegir el dispositivo adecuado. Tras varias pruebas con WebScreen y JavaScript, las pantallas Open Nextion demostraron ser la opción más potente y flexible.

## 🧩 Características principales

- Visualización en tiempo real de carga de CPU, temperatura, memoria y otros datos del NestDisk.
- Comunicación mediante MQTT para integrarse con Home Assistant.
- Interfaz gráfica basada en LVGL para animaciones y widgets fluidos.
- Código escrito en Arduino y organizado en módulos comentados dentro del `.ino`.
- Compatible con pantallas Open Nextion basadas en ESP32‑S3.

## 📁 Contenido del repositorio

- `CPU-MONITOR.ino` — Archivo principal del proyecto, con comentarios que describen los módulos internos.
- `config/` — Parámetros de conexión MQTT y ajustes opcionales.
- `README.md` — Este documento.

## 🧩 Requisitos

- Pantalla Open Nextion con ESP32‑S3.
- Arduino IDE funcionando en un entorno compatible (recomendado: Debian o Linux).
- Broker MQTT accesible (por ejemplo, el de Home Assistant).
- NestDisk configurado para publicar métricas del sistema.

## 🚀 Instalación y uso

1. Clona este repositorio en tu entorno de desarrollo.
2. Abre `CPU-MONITOR.ino` en Arduino IDE.
3. Ajusta los parámetros MQTT en la sección correspondiente del código.
4. Compila y sube el firmware a la pantalla Open Nextion.
5. Asegúrate de que el NestDisk está publicando datos en los topics esperados.
6. Disfruta del panel de monitorización en tiempo real.

## 🛠️ Estructura del código

El archivo `.ino` está organizado en módulos claramente comentados, que explican la función de cada bloque:

- Inicialización del hardware
- Conexión WiFi
- Suscripción MQTT
- Procesamiento de mensajes
- Actualización de widgets LVGL
- Bucle principal optimizado para la pantalla

## 📦 Licencia

Proyecto publicado bajo licencia MIT. Puedes usarlo, modificarlo y adaptarlo libremente.

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Si mejoras el código, encuentras un bug o quieres añadir nuevas métricas, no dudes en abrir un *pull request*.
