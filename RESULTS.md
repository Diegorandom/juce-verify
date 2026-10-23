# Salida real del arnés

Capturada el 2026-10-23 en macOS 26.7.1, CMake 4.4.3, JUCE 8.0.16-security.

```text

1 · passthrough — el contrato del host (celda W3.3b)
--------------------------------------------------------------
  prepareToPlay llamado   : 1 vez/veces
  processBlock llamado    : 94 veces
  diferencia con la entrada: 0.000e+00  (bit-exacta)
  → el «no hace nada y suena» es literal: la salida es la entrada.

2 · zipper — ganancia sin suavizar contra suavizada (celdas W3.4a y W3.4b)
--------------------------------------------------------------
  señal: seno 1000 Hz, amplitud 0.50, 1.0 s a 48000 Hz
  cambio de ganancia: 1.000 → 0.250  (-12.04 dB) en t = 0.5 s
--------------------------------------------------------------
  referencia (ganancia constante): salto máx = 0.06526  (-23.7 dBFS)
  sin suavizar:          salto máx en el cambio = 0.35369  (-9.0 dBFS)
  con rampa de 20 ms:    salto máx en el cambio = 0.06526  (-23.7 dBFS)
--------------------------------------------------------------
  exceso sobre la referencia:  sin suavizar 14.7 dB  ·  con rampa 0.0 dB
  → el defecto SE OYE, y la rampa lo quita: la celda tiene algo que medir.

3 · blocksize — la salida no depende del tamaño de bloque (celda W3.3b)
--------------------------------------------------------------
  bloque    16 muestras  →  salida calculada
  bloque    32 muestras  →  salida calculada
  bloque    64 muestras  →  salida calculada
  bloque   128 muestras  →  salida calculada
  bloque   256 muestras  →  salida calculada
  bloque   512 muestras  →  salida calculada
  bloque  1024 muestras  →  salida calculada
  bloque  2048 muestras  →  salida calculada
--------------------------------------------------------------
  diferencia máxima contra el bloque de 16: 0.000e+00
  → independiente del tamaño: el código no asume un bloque fijo.

4 · deadline — cuánto tarda un bloque frente a lo que dura (celda W3.4b)
--------------------------------------------------------------
  Dos procesadores con el MISMO trabajo de audio. La diferencia es lo que
  hacen además: uno no toca el heap, el otro asigna un vector por bloque.
--------------------------------------------------------------
  bloque   dura (µs)      limpio (x)  asignando (x)
  32       666.7                1.58%         80.05%
  64       1333.3               2.18%         10.88%
  128      2666.7               1.05%          5.34%
  256      5333.3               0.13%          2.30%
  512      10666.7              0.28%          1.05%
  1024     21333.3              0.10%          0.63%
--------------------------------------------------------------
  se mide el PEOR bloque, no el promedio: el chasquido lo produce el peor.
  el plazo de un bloque es su propia duración: pasado el 100 % no hay
  lentitud, hay silencio o chasquido.

5 · latency — muestras de latencia, medidas con un impulso
--------------------------------------------------------------
  impulso en la muestra 0  →  pico en la muestra 240
  latencia = 240 muestras = 5.000 ms a 48000 Hz
  → coincide con la cifra que se dice en clase.

```

## Cómo se reproduce

```bash
cmake -S . -B build -DJUCE_DIR=/ruta/a/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
./build/juce-verify_artefacts/Release/juce-verify todo
```
