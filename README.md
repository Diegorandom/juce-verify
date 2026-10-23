# juce-verify

**Mide lo que un plugin de JUCE afirma que se oye.** No empaqueta nada y no necesita el Projucer: construye con CMake y ejecuta el procesamiento de audio de verdad, a través de clases `juce::AudioProcessor` reales, para responder con cifras a preguntas que normalmente se responden de memoria.

> La regla del repositorio: **ningún efecto audible se afirma sin un número medido.** Si una afirmación no se puede medir aquí, se dice que no se puede medir.

Escribí esto para un curso de plugins: cada comando verifica la afirmación de audio de una celda concreta, y la columna «celda» de la tabla es la razón de que cada medición exista.

## Subcomandos

| Comando | Qué mide | Celda del curso |
|---|---|---|
| `passthrough` | que la señal sale igual que entra (bit-exacto) y cuántas veces llama el host | W3.3b · *introducción a Projucer* |
| `zipper` | salto máximo entre muestras, con y sin suavizado, contra una referencia | W3.4a · W3.4b |
| `blocksize` | que la salida no depende del tamaño de bloque (16 → 2048) | W3.3b |
| `deadline` | peor bloque frente a su propio plazo: limpio vs 1 MB tocado por bloque | W3.4b · *procesamiento y tiempo real* |
| `latency` | latencia en muestras, medida con un impulso | W3.4b |

`todo` ejecuta los cinco seguidos y devuelve código de salida distinto de cero si alguna de las comprobaciones clave falla.

## Resultados de referencia

JUCE 8.0.16-security · 48 kHz · seno de 1 kHz · macOS. La salida cruda está en [RESULTS.md](RESULTS.md).

| Medida | Resultado |
|---|---|
| **passthrough** | diferencia **0.000e+00** · 1× `prepareToPlay` · **94×** `processBlock` para 1 s en bloques de 512 |
| **zipper** | referencia **−23,7 dBFS** · sin suavizar **−9,0 dBFS** (**+14,7 dB**) · rampa de 20 ms **−23,7 dBFS** (**0,0 dB**) |
| **blocksize** | diferencia máxima **0.000e+00** entre bloques de 16 y de 2048 muestras |
| **deadline** | 1 MB por bloque: **79,6 %** del plazo a 32 muestras, 14,3 % a 64, 5,5 % a 128, 3,0 % a 256, 1,1 % a 512, **0,7 %** a 1024 · limpio: 5,0 % · 2,0 % · 1,8 % · 0,6 % · 0,1 % · 0,2 % |
| **latency** | pico del impulso en la muestra **240** → **5,000 ms** a 48 kHz |

## Uso

Requiere CMake 3.22+ y un compilador de C y C++.

```bash
cmake -S . -B build -DJUCE_DIR=/ruta/a/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
./build/juce-verify_artefacts/Release/juce-verify todo
```

`JUCE_DIR` es un parámetro a propósito: el arnés sobrevive a la próxima subida de versión de JUCE sin tocar una línea. Probado con **JUCE 8.0.16-security**.

## Cómo se construyó esto

Este repositorio es el artefacto de un **flujo de trabajo agéntico**, y la parte interesante no es que el código compile: es el bucle.

```
decisión humana  →  el agente mide  →  la medición contradice al agente
                 →  el agente corrige y vuelve a medir  →  la cifra se publica
```

El humano fijó las cuatro decisiones que no son técnicas: qué escribir, dónde, cómo se verifica en el aula y **cuánto había que verificar**. El agente construyó el entorno, escribió el arnés, lo ejecutó y —esto es lo que se puede auditar— **se equivocó tres veces y lo detectó**.

### Las tres correcciones, en orden

**1 · Simulé con `float` cuando el código declara `double`.** Verificando un defecto de fase de un generador de ondas, la simulación con 32 bits daba *cero cents* de error y el documento afirmaba *+11,82*. Estuve a punto de publicar «el documento está mal». El código declara `double`; con `double` da exactamente 146 muestras y +11,82 cents. **El error era mi instrumento, no el objeto medido.** Lección que quedó escrita en el arnés: el tipo declarado es parte del hecho.

**2 · La primera medición del plazo era ruido.** El comando `deadline` asignaba un vector del tamaño del bloque —cientos de bytes— y medía 2,12 % del plazo contra 3,01 %, con casos en los que asignar salía *más rápido* que no asignar. La señal estaba por debajo del ruido del planificador. Se cambió a **1 MB por bloque, tocado**, que fuerza faltas de página: entonces el contraste se volvió monótono. **Una transgresión de tiempo real no se demuestra con un cronómetro sobre una asignación pequeña.**

**3 · Un hallazgo que no estaba buscado.** Al arreglar lo anterior, la tabla mostró algo que no estaba en el diseño: **el plazo se rompe antes con bloques pequeños**. 79,6 % del plazo a 32 muestras, 0,7 % a 1024 — el mismo código. La razón es que el plazo *es* la duración del bloque, así que el margen se reparte entre más bloques. Eso no salió de leer documentación: salió de medir.

## Lo que este repositorio no afirma

La honestidad es parte del artefacto, no una nota al pie:

- **No verifica la GUI.** Que una perilla cambie el audio sin recompilar es una comprobación en un DAW, con ojos y oídos.
- **No construye el plugin empaquetado.** No hay `.vst3` ni `.au` aquí: eso es un Projucer y un proyecto, no este arnés.
- **No reproduce un host real.** El arnés *es* el host: llama `prepareToPlay` una vez y `processBlock` N veces, como describe la documentación, pero no imita las rarezas de un DAW concreto.
- **No mide el rendimiento absoluto de una máquina**, sino la proporción del plazo consumida. Los porcentajes de `deadline` dependen del hardware y sirven para comparar, no como cifra universal.
- **La CI no se ha ejecutado todavía.** El flujo está escrito y compila en una máquina; la primera ejecución real será la que lo confirme.

## Por qué CMake y no el Projucer

Porque el objeto de la verificación es **el comportamiento del audio**, y el Projucer es una herramienta de empaquetado. Añadir el Projucer habría metido un paso manual —y un binario compilado— entre la medición y el resultado. Con CMake el bucle es `editar → construir → medir` y se puede automatizar entero.

## Licencia

Sin decidir todavía. Si vas a usarlo, abre un *issue*.
