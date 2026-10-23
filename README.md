# juce-verify

**Mide lo que un plugin de JUCE afirma que se oye.** No empaqueta nada y no necesita el Projucer: construye con CMake y ejecuta el procesamiento de audio de verdad, a través de clases `juce::AudioProcessor` reales, para responder con cifras a preguntas que normalmente se responden de memoria.

> La regla del repositorio: **ningún efecto audible se afirma sin un número medido.** Si una afirmación no se puede medir aquí, se dice que no se puede medir.

| Comando | Qué responde | Cifra de hoy |
|---|---|---|
| `passthrough` | ¿deja la señal intacta, y cuántas veces llama el host? | diferencia **0.000e+00** |
| `zipper` | ¿cuánto salta una ganancia sin suavizar? | **+14,7 dB** sobre la referencia |
| `blocksize` | ¿depende la salida del tamaño de bloque? | **0.000e+00** de 16 a 2048 |
| `deadline` | ¿cuánto del plazo consume un bloque? | **79,6 %** a 32 muestras |
| `latency` | ¿cuántas muestras de latencia? | **240** = 5,000 ms a 48 kHz |

Los números completos, con la salida cruda, están en [RESULTS.md](RESULTS.md).

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

### Lo que este repositorio no afirma

La honestidad es parte del artefacto, no una nota al pie:

- **No verifica la GUI.** Que una perilla cambie el audio sin recompilar es una comprobación en un DAW, con ojos y oídos.
- **No construye el plugin empaquetado.** No hay `.vst3` ni `.au` aquí: eso es un Projucer y un proyecto, no este arnés.
- **No reproduce un host real.** El arnés *es* el host: llama `prepareToPlay` una vez y `processBlock` N veces, como describe la documentación, pero no imita las rarezas de un DAW concreto.
- **La CI no se ha ejecutado todavía.** El flujo está escrito y compila en una máquina; la primera ejecución real será la que lo confirme.

## Uso

Requiere CMake 3.22+ y un compilador de C y C++.

```bash
cmake -S . -B build -DJUCE_DIR=/ruta/a/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
./build/juce-verify_artefacts/Release/juce-verify todo
```

`JUCE_DIR` es un parámetro a propósito: el arnés sobrevive a la próxima subida de versión de JUCE sin tocar una línea. Probado con **JUCE 8.0.16-security**.

## Por qué CMake y no el Projucer

Porque el objeto de la verificación es **el comportamiento del audio**, y el Projucer es una herramienta de empaquetado. Añadir el Projucer habría metido un paso manual —y un binario compilado— entre la medición y el resultado. Con CMake el bucle es `editar → construir → medir` y se puede automatizar entero.

## Licencia

Sin decidir todavía. Si vas a usarlo, abre un *issue*.
