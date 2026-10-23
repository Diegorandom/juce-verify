// juce-verify — arnés de verificación para las celdas de JUCE del Módulo 3.
//
//   juce-verify passthrough   el contrato del host: prepareToPlay + processBlock
//                             dejan la señal intacta, y es bit-exacta
//   juce-verify zipper        ganancia sin suavizar vs suavizada: cuánto salta
//   juce-verify blocksize     la salida no depende del tamaño de bloque
//   juce-verify deadline      cuánto tarda un bloque frente a lo que dura
//   juce-verify latency       muestras de latencia, medidas con un impulso
//
// La regla del arnés: **ningún «qué se oye» se afirma sin una cifra**. Cada
// subcomando imprime lo que midió y con qué señal, para que el número se pueda
// citar en clase y repetir después.

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <vector>
#include <string>

namespace
{
constexpr double SR = 48000.0;
constexpr double F0 = 1000.0;
constexpr double AMP = 0.5;
constexpr double DUR = 1.0;

juce::AudioBuffer<float> seno (double freq = F0, double amp = AMP, double dur = DUR)
{
    const int n = (int) std::lround (SR * dur);
    juce::AudioBuffer<float> b (1, n);
    auto* d = b.getWritePointer (0);
    for (int i = 0; i < n; ++i)
        d[i] = (float) (amp * std::sin (2.0 * juce::MathConstants<double>::pi * freq * i / SR));
    return b;
}

/** Salto máximo entre muestras consecutivas: la firma medible de un chasquido. */
double saltoMaximo (const juce::AudioBuffer<float>& b, int desde = 0, int hasta = -1)
{
    const int n = b.getNumSamples();
    if (hasta < 0) hasta = n;
    double m = 0.0;
    for (int i = juce::jmax (1, desde); i < juce::jmin (hasta, n); ++i)
        m = juce::jmax (m, (double) std::abs (b.getSample (0, i) - b.getSample (0, i - 1)));
    return m;
}

double aDb (double x) { return x <= 0.0 ? -200.0 : 20.0 * std::log10 (x); }

void linea() { std::printf ("--------------------------------------------------------------\n"); }
void cabeza (const char* t) { std::printf ("\n%s\n", t); linea(); }

// ---------------------------------------------------------------- procesadores

/** El plugin mínimo de W3.3b: no hace nada, y demuestra que el host es quien llama. */
class PassThrough final : public juce::AudioProcessor
{
public:
    PassThrough() : juce::AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)) {}

    const juce::String getName() const override { return "PassThrough"; }
    void prepareToPlay (double, int) override { llamadasPrepare++; }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }

    void processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override
    {
        llamadasProcess++;
        tamanosVistos.push_back (buf.getNumSamples()); // aquí se ve que el tamaño varía
    }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    int llamadasPrepare = 0, llamadasProcess = 0;
    std::vector<int> tamanosVistos;
};

/** Ganancia con parámetro, con y sin suavizado: la celda W3.4a y W3.4b. */
class Ganancia final : public juce::AudioProcessor
{
public:
    explicit Ganancia (bool conSuavizado, double msRampa = 20.0)
        : juce::AudioProcessor (BusesProperties()
            .withInput ("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          suaviza (conSuavizado), ms (msRampa) {}

    const juce::String getName() const override { return "Ganancia"; }
    void prepareToPlay (double sr, int) override
    {
        valor = 1.0;
        actual = 1.0;
        paso = ms > 0.0 ? 1.0 / (sr * ms / 1000.0) : 1.0;
    }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }

    void processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override
    {
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            if (suaviza)
            {
                // rampa lineal hacia el objetivo: un cambio por muestra, acotado
                if (actual < valor) actual = juce::jmin (valor, actual + paso);
                else if (actual > valor) actual = juce::jmax (valor, actual - paso);
            }
            else
            {
                actual = valor; // salto instantáneo: el defecto
            }

            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
                buf.setSample (ch, i, buf.getSample (ch, i) * (float) actual);
        }
    }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    void setObjetivo (double v) { valor = v; }
    double objetivo() const { return valor; }

private:
    bool suaviza;
    double ms;
    double valor = 1.0, actual = 1.0, paso = 1.0;
};

/** Retardo puro, para medir latencia con un impulso. */
class Retardo final : public juce::AudioProcessor
{
public:
    explicit Retardo (int muestras) : juce::AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)), n (muestras) {}

    const juce::String getName() const override { return "Retardo"; }
    void prepareToPlay (double, int) override { buf.assign ((size_t) n, 0.0f); idx = 0; }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }

    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float in = b.getSample (0, i);
            const float out = buf[(size_t) idx];
            buf[(size_t) idx] = in;
            idx = (idx + 1) % n;
            b.setSample (0, i, out);
        }
    }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    int n;
    std::vector<float> buf;
    int idx = 0;
};

// ---------------------------------------------------------------- subcomandos

int cmdPassthrough()
{
    cabeza ("1 · passthrough — el contrato del host (celda W3.3b)");

    juce::AudioBuffer<float> original = seno();
    juce::AudioBuffer<float> trabajo (1, original.getNumSamples());
    trabajo.makeCopyOf (original);

    PassThrough p;
    juce::MidiBuffer midi;

    p.prepareToPlay (SR, 512);           // el host llama primero a prepareToPlay
    for (int pos = 0; pos < trabajo.getNumSamples();)
    {
        const int n = juce::jmin (512, trabajo.getNumSamples() - pos);
        juce::AudioBuffer<float> bloque (trabajo.getArrayOfWritePointers(), 1, pos, n);
        p.processBlock (bloque, midi);   // ...y después, bloque a bloque
        pos += n;
    }

    double maxDif = 0.0;
    for (int i = 0; i < original.getNumSamples(); ++i)
        maxDif = juce::jmax (maxDif, (double) std::abs (trabajo.getSample (0, i) - original.getSample (0, i)));

    std::printf ("  prepareToPlay llamado   : %d vez/veces\n", p.llamadasPrepare);
    std::printf ("  processBlock llamado    : %d veces\n", p.llamadasProcess);
    std::printf ("  diferencia con la entrada: %.3e  (%s)\n", maxDif,
                 maxDif == 0.0 ? "bit-exacta" : "DISTINTA");
    std::printf ("  → el «no hace nada y suena» es literal: la salida es la entrada.\n");
    return maxDif == 0.0 ? 0 : 1;
}

int cmdZipper()
{
    cabeza ("2 · zipper — ganancia sin suavizar contra suavizada (celdas W3.4a y W3.4b)");

    const double saltoDb = juce::Decibels::gainToDecibels (0.25);
    std::printf ("  señal: seno %.0f Hz, amplitud %.2f, %.1f s a %.0f Hz\n", F0, AMP, DUR, SR);
    std::printf ("  cambio de ganancia: 1.000 → 0.250  (%.2f dB) en t = 0.5 s\n", saltoDb);
    linea();

    double saltoRef = 0.0;
    {
        // referencia: el mismo seno con ganancia constante (nada que saltar)
        juce::AudioBuffer<float> b = seno();
        Ganancia g (true, 20.0);
        juce::MidiBuffer m;
        g.prepareToPlay (SR, 512);
        juce::AudioBuffer<float> t (b.getArrayOfWritePointers(), 1, 0, b.getNumSamples());
        g.processBlock (t, m);
        saltoRef = saltoMaximo (b);
        std::printf ("  referencia (ganancia constante): salto máx = %.5f  (%.1f dBFS)\n",
                     saltoRef, aDb (saltoRef));
    }

    auto correr = [&] (bool suaviza, const char* nombre) -> double
    {
        juce::AudioBuffer<float> b = seno();
        Ganancia g (suaviza, 20.0);
        juce::MidiBuffer m;
        g.prepareToPlay (SR, 512);
        const int cambio = (int) (SR * 0.5);
        double saltoEnCambio = 0.0;
        for (int pos = 0; pos < b.getNumSamples();)
        {
            const int n = juce::jmin (512, b.getNumSamples() - pos);
            if (pos >= cambio && g.objetivo() != 0.25) g.setObjetivo (0.25); // el host mueve la perilla
            juce::AudioBuffer<float> bloque (b.getArrayOfWritePointers(), 1, pos, n);
            g.processBlock (bloque, m);
            pos += n;
        }
        saltoEnCambio = saltoMaximo (b, cambio - 200, cambio + (int) (SR * 0.05));
        std::printf ("  %-22s salto máx en el cambio = %.5f  (%.1f dBFS)\n",
                     nombre, saltoEnCambio, aDb (saltoEnCambio));
        return saltoEnCambio;
    };

    const double sinS = correr (false, "sin suavizar:");
    const double conS = correr (true, "con rampa de 20 ms:");
    linea();
    std::printf ("  exceso sobre la referencia:  sin suavizar %.1f dB  ·  con rampa %.1f dB\n",
                 aDb (sinS) - aDb (saltoRef), aDb (conS) - aDb (saltoRef));
    std::printf ("  → %s\n", sinS > conS * 2.0
                 ? "el defecto SE OYE, y la rampa lo quita: la celda tiene algo que medir."
                 : "no hay diferencia audible: revisar el montaje antes de enseñarlo.");
    return 0;
}

int cmdBlockSize()
{
    cabeza ("3 · blocksize — la salida no depende del tamaño de bloque (celda W3.3b)");

    std::vector<int> tamanos { 16, 32, 64, 128, 256, 512, 1024, 2048 };
    juce::AudioBuffer<float> ref;
    double peor = 0.0;

    for (int tam : tamanos)
    {
        juce::AudioBuffer<float> b = seno (F0, AMP, 0.25);
        Ganancia g (true, 20.0);
        juce::MidiBuffer m;
        g.prepareToPlay (SR, tam);
        for (int pos = 0; pos < b.getNumSamples();)
        {
            const int n = juce::jmin (tam, b.getNumSamples() - pos);
            juce::AudioBuffer<float> bloque (b.getArrayOfWritePointers(), 1, pos, n);
            g.processBlock (bloque, m);
            pos += n;
        }
        if (ref.getNumSamples() == 0) ref.makeCopyOf (b);
        else
        {
            double d = 0.0;
            for (int i = 0; i < b.getNumSamples(); ++i)
                d = juce::jmax (d, (double) std::abs (b.getSample (0, i) - ref.getSample (0, i)));
            peor = juce::jmax (peor, d);
        }
        std::printf ("  bloque %5d muestras  →  salida calculada\n", tam);
    }
    linea();
    std::printf ("  diferencia máxima contra el bloque de 16: %.3e\n", peor);
    std::printf ("  → %s\n", peor < 1.0e-7
                 ? "independiente del tamaño: el código no asume un bloque fijo."
                 : "OJO: la salida cambia con el tamaño de bloque.");
    return 0;
}

int cmdDeadline()
{
    cabeza ("4 · deadline — cuánto tarda un bloque frente a lo que dura (celda W3.4b)");

    std::printf ("  Dos procesadores con el MISMO trabajo de audio. La diferencia es lo que\n");
    std::printf ("  hacen además: uno no toca el heap, el otro asigna un vector por bloque.\n");
    linea();
    std::printf ("  %-8s %-11s %14s %14s\n", "bloque", "dura (µs)", "limpio (x)", "asignando (x)");

    for (int tam : { 32, 64, 128, 256, 512, 1024 })
    {
        juce::AudioBuffer<float> b = seno (F0, AMP, 0.5);
        juce::MidiBuffer m;

        auto medir = [&] (bool asigna) -> double
        {
            Ganancia g (true, 20.0);
            g.prepareToPlay (SR, tam);
            std::vector<float> bufferTemporal (asigna ? (size_t) tam : 0, 0.0f);
            double peor = 0.0;

            for (int r = 0; r < 120; ++r)
            {
                for (int pos = 0; pos < b.getNumSamples();)
                {
                    const int n = juce::jmin (tam, b.getNumSamples() - pos);
                    juce::AudioBuffer<float> bloque (b.getArrayOfWritePointers(), 1, pos, n);

                    auto t0 = std::chrono::steady_clock::now();
                    if (asigna)
                    {
                        // el pecado de la regla 1: memoria en el hilo de audio.
                        // 1 MB por bloque y TOCADO: fuerza faltas de página, que es
                        // lo que de verdad rompe el plazo (no la aritmética).
                        const size_t grande = 256u * 1024u;
                        std::vector<float> tmp (grande, 0.0f);
                        for (size_t i = 0; i < grande; i += 512) tmp[i] = (float) i;
                        for (int i = 0; i < n; ++i) bloque.setSample (0, i, bloque.getSample (0, i) * (0.5f + tmp[(size_t) i] * 0.0f));
                    }
                    else
                    {
                        // en su sitio, sin reservar: lo correcto
                        for (int i = 0; i < n; ++i) bloque.setSample (0, i, bloque.getSample (0, i) * 0.5f);
                    }
                    g.processBlock (bloque, m);
                    auto t1 = std::chrono::steady_clock::now();

                    peor = juce::jmax (peor, std::chrono::duration<double, std::micro> (t1 - t0).count());
                    pos += n;
                }
            }
            juce::ignoreUnused (bufferTemporal);
            return peor;
        };

        const double limpio = medir (false);
        const double asigna = medir (true);
        const double dura = tam / SR * 1.0e6;
        std::printf ("  %-8d %-11.1f %13.2f%% %13.2f%%\n", tam, dura,
                     100.0 * limpio / dura, 100.0 * asigna / dura);
    }
    linea();
    std::printf ("  se mide el PEOR bloque, no el promedio: el chasquido lo produce el peor.\n");
    std::printf ("  el plazo de un bloque es su propia duración: pasado el 100 %% no hay\n");
    std::printf ("  lentitud, hay silencio o chasquido.\n");
    return 0;
}

int cmdLatency()
{
    cabeza ("5 · latency — muestras de latencia, medidas con un impulso");

    const int esperado = 240; // 5 ms a 48 kHz — la cifra que se cita en clase
    juce::AudioBuffer<float> b (1, 1024);
    b.clear();
    b.setSample (0, 0, 1.0f);

    Retardo r (esperado);
    juce::MidiBuffer m;
    r.prepareToPlay (SR, 512);
    juce::AudioBuffer<float> t (b.getArrayOfWritePointers(), 1, 0, b.getNumSamples());
    r.processBlock (t, m);

    int pico = 0;
    float maxv = 0.0f;
    for (int i = 0; i < b.getNumSamples(); ++i)
        if (std::abs (b.getSample (0, i)) > maxv) { maxv = std::abs (b.getSample (0, i)); pico = i; }

    std::printf ("  impulso en la muestra 0  →  pico en la muestra %d\n", pico);
    std::printf ("  latencia = %d muestras = %.3f ms a %.0f Hz\n", pico, pico / SR * 1000.0, SR);
    std::printf ("  → %s\n", pico == esperado
                 ? "coincide con la cifra que se dice en clase."
                 : "NO coincide: revisar antes de citarla.");
    return pico == esperado ? 0 : 1;
}

void ayuda()
{
    std::printf ("\njuce-verify — arnés de verificación de las celdas de JUCE\n\n");
    std::printf ("  passthrough   el contrato del host deja la señal intacta\n");
    std::printf ("  zipper        ganancia sin suavizar contra suavizada\n");
    std::printf ("  blocksize     independencia del tamaño de bloque\n");
    std::printf ("  deadline      tiempo de un bloque contra su duración\n");
    std::printf ("  latency       latencia medida con un impulso\n");
    std::printf ("  todo          los cinco seguidos\n\n");
}
} // namespace

int main (int argc, char** argv)
{
    const std::string cmd = argc > 1 ? argv[1] : "todo";

    if (cmd == "passthrough") return cmdPassthrough();
    if (cmd == "zipper")      return cmdZipper();
    if (cmd == "blocksize")   return cmdBlockSize();
    if (cmd == "deadline")    return cmdDeadline();
    if (cmd == "latency")     return cmdLatency();
    if (cmd == "todo")
    {
        int rc = 0;
        rc |= cmdPassthrough();
        rc |= cmdZipper();
        rc |= cmdBlockSize();
        rc |= cmdDeadline();
        rc |= cmdLatency();
        std::printf ("\n");
        return rc;
    }
    ayuda();
    return 2;
}
