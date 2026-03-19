# Laboratorio 2 de Sistemas Embebidos - 2026-1
Este laboratorio consistía en crear un videojuego en una matrix led 8x8. Con un microcontrolador ESP32

## El juego: Stack 🏰
El objetivo de The Stack es construir una torre de luz sobre una matriz de 8x8 alineando bloques móviles mediante un único botón. Cada vez que presionas el botón, el bloque actual se fija sobre el anterior; si no logras una alineación perfecta, los LEDs sobrantes se recortan, reduciendo el tamaño de tu siguiente pieza y aumentando la dificultad. El reto crece con la velocidad en cada nivel, y ganas si logras apilar bloques hasta el octavo piso para desbloquear el trofeo final, pero el juego termina si fallas una alineación por completo y te quedas sin base. El botón se utiliza únicamente para fijar el bloque ya que el juego tiene reinicio automático en caso de perder. Además, por cada vez que pierdas, la velocidad de cada nivel aumenta así que la clave está en la concentración y la reacción. 

## Herramientas y Hardware
* **Microcontrolador:** ESP32 (AZ-Delivery Dev Kit C V4)
* **Framework:** ESP-IDF
* **Entorno de desarrollo:** VSCode + PlatformIO
* **Lenguaje:** C

## Estructura del Repositorio
* `src/`: Archivos de código fuente (.c).
* `include/`: Cabeceras (.h).
* `platformio.ini`: Configuración del entorno y placa.

## Branch de Pruebas
> En la rama `pruebas-matriz` se encuentra el código experimental para verificar el funcionamiento de la matriz con sus respectivos gráficos. 