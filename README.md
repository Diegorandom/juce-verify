# juce-verify

Arnés de verificación de las afirmaciones de audio de las celdas de JUCE del
Módulo 3. Construido con **CMake, sin Projucer**: verifica el comportamiento del
audio, no el empaquetado.

## Uso

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   # JUCE de /Applications/JUCE-8.0.16-security
cmake --build build -j 8
./build/juce-verify_artefacts/Release/juce-verify todo
```

La ruta de JUCE es un parámetro, para que el arnés sobreviva a la próxima subida
de versión:

```bash
cmake -S . -B build -DJUCE_DIR=/ruta/a/JUCE
```

## Subcomandos

| Comando | Qué mide | Celda |
|---|---|---|
| `passthrough` | que la señal sale igual que entra (bit-exacto) y cuántas veces llama el host | W3.3b |
| `zipper` | salto máximo entre muestras, con y sin suavizado, contra una referencia | W3.4a · W3.4b |
| `blocksize` | que la salida no depende del tamaño de bloque (16 → 2048) | W3.3b |
| `deadline` | peor bloque frente a su propio plazo: limpio vs 1 MB tocado por bloque | W3.4b |
| `latency` | latencia en muestras, medida con un impulso | W3.4b |

## Resultados de referencia (2026-10-23, JUCE 8.0.16-security)

- **passthrough**: diferencia 0.000e+00 · 1× `prepareToPlay` · 94× `processBlock` para 1 s a 48 kHz en bloques de 512
- **zipper**: referencia −23,7 dBFS · sin suavizar −9,0 dBFS (**+14,7 dB**) · rampa 20 ms −23,7 dBFS (**0,0 dB**)
- **blocksize**: diferencia máxima 0.000e+00 entre bloques de 16 y 2048
- **deadline**: 1 MB por bloque → 79,6 % del plazo a 32 muestras, 0,7 % a 1024 · limpio 5,0 % y 0,2 %
- **latency**: pico en la muestra 240 → **5,000 ms** a 48 kHz

## Qué NO verifica

- La interacción con la GUI (necesita un DAW y ojos).
- El plugin empaquetado (.vst3/.au): eso es P1 y el Projucer.
- Las rarezas de un host real (llamadas fuera de orden, cambios de tamaño a mitad de reproducción).

## Un aviso sobre medir tiempo real

La primera versión de `deadline` asignaba el tamaño del bloque (~cientos de bytes)
y el resultado era **ruido**: la señal quedaba por debajo del ruido del
planificador. Con 1 MB tocado por bloque —que fuerza faltas de página— el
contraste sí es monótono y legible. Una transgresión de tiempo real no se
demuestra con un cronómetro sobre una asignación pequeña.
