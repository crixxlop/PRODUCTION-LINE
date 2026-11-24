#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

enum class Color { Rojo, Verde, Azul, Desconocido };

std::string colorATexto(Color color) {
    switch (color) {
        case Color::Rojo:  return "ROJO";
        case Color::Verde: return "VERDE";
        case Color::Azul:  return "AZUL";
        default:           return "DESCONOCIDO";
    }
}

struct LecturaRGB {
    unsigned long rojo  = 0;
    unsigned long verde = 0;
    unsigned long azul  = 0;

    unsigned long minimo() const { return std::min({rojo, verde, azul}); }

    bool esValida() const { return rojo > 0 && verde > 0 && azul > 0; }
};

class Caja {
public:
    using Reloj = std::chrono::system_clock;

    Caja(int numero, Color color, int lecturasUsadas, Reloj::time_point momento)
        : numero_(numero), color_(color), lecturasUsadas_(lecturasUsadas), momento_(momento) {}

    int numero() const { return numero_; }
    Color color() const { return color_; }
    int lecturasUsadas() const { return lecturasUsadas_; }

    std::string fechaHora() const {
        std::time_t t = Reloj::to_time_t(momento_);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream salida;
        salida << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return salida.str();
    }

private:
    int numero_;
    Color color_;
    int lecturasUsadas_;
    Reloj::time_point momento_;
};

class ResumenConteo {
public:
    void sumar(Color color) { ++conteo_[color]; }

    int cantidad(Color color) const {
        auto it = conteo_.find(color);
        return it == conteo_.end() ? 0 : it->second;
    }

    int total() const {
        int suma = 0;
        for (const auto& par : conteo_) suma += par.second;
        return suma;
    }

private:
    std::map<Color, int> conteo_;
};

class IFuenteLecturas {
public:
    virtual ~IFuenteLecturas() = default;

    virtual bool leerLinea(std::string& linea) = 0;
};

class FuenteArchivo : public IFuenteLecturas {
public:
    explicit FuenteArchivo(const std::string& ruta) : archivo_(ruta) {
        if (!archivo_.is_open()) {
            throw std::runtime_error("No se pudo abrir el archivo: " + ruta);
        }
    }

    bool leerLinea(std::string& linea) override {
        return static_cast<bool>(std::getline(archivo_, linea));
    }

private:
    std::ifstream archivo_;
};

class FuenteSimulada : public IFuenteLecturas {
public:
    FuenteSimulada() {
        const std::string vacio = "R:310\tV:330\tA:290";
        const std::string rojo  = "R:45\tV:190\tA:160";
        const std::string verde = "R:200\tV:70\tA:150";
        const std::string azul  = "R:180\tV:140\tA:60";

        lineas_ = {
            vacio, vacio,
            rojo, "ROJO", rojo, vacio,
            verde, verde, "VERDE", vacio, vacio,
            azul, "AZUL", azul, vacio,
            rojo, rojo, vacio,
            "R:0\tV:0\tA:0",
            verde, vacio,
            azul, azul, azul, vacio
        };
    }

    bool leerLinea(std::string& linea) override {
        if (posicion_ >= lineas_.size()) return false;
        linea = lineas_[posicion_++];
        return true;
    }

private:
    std::vector<std::string> lineas_;
    std::size_t posicion_ = 0;
};

#ifdef _WIN32
class PuertoSerieWindows : public IFuenteLecturas {
public:
    PuertoSerieWindows(const std::string& puerto, unsigned long baudios) {
        const std::string ruta = "\\\\.\\" + puerto;
        handle_ = CreateFileA(ruta.c_str(), GENERIC_READ, 0, nullptr,
                              OPEN_EXISTING, 0, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) {
            throw std::runtime_error(
                "No se pudo abrir " + puerto + ". Revisa el numero de puerto en el "
                "IDE de Arduino (Herramientas > Puerto) y CIERRA el Monitor Serie.");
        }

        DCB config{};
        config.DCBlength = sizeof(config);
        if (!GetCommState(handle_, &config)) {
            cerrar();
            throw std::runtime_error("No se pudo leer la configuracion de " + puerto);
        }
        config.BaudRate = baudios;
        config.ByteSize = 8;
        config.Parity   = NOPARITY;
        config.StopBits = ONESTOPBIT;
        config.fDtrControl = DTR_CONTROL_ENABLE;
        if (!SetCommState(handle_, &config)) {
            cerrar();
            throw std::runtime_error("No se pudo configurar " + puerto);
        }

        COMMTIMEOUTS tiempos{};
        tiempos.ReadIntervalTimeout = 50;
        tiempos.ReadTotalTimeoutConstant = 1000;
        tiempos.ReadTotalTimeoutMultiplier = 10;
        SetCommTimeouts(handle_, &tiempos);
    }

    ~PuertoSerieWindows() override { cerrar(); }

    PuertoSerieWindows(const PuertoSerieWindows&) = delete;
    PuertoSerieWindows& operator=(const PuertoSerieWindows&) = delete;

    bool leerLinea(std::string& linea) override {
        linea.clear();
        char caracter = 0;
        DWORD leidos = 0;
        while (true) {
            if (!ReadFile(handle_, &caracter, 1, &leidos, nullptr)) {
                return false;
            }
            if (leidos == 0) continue;
            if (caracter == '\n') return true;
            if (caracter != '\r') linea += caracter;
        }
    }

private:
    void cerrar() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE handle_ = INVALID_HANDLE_VALUE;
};
#endif

class ParserLecturas {
public:
    std::optional<LecturaRGB> parsear(const std::string& linea) const {
        LecturaRGB lectura;
        const int camposLeidos = std::sscanf(linea.c_str(), " R:%lu V:%lu A:%lu",
                                             &lectura.rojo, &lectura.verde, &lectura.azul);
        if (camposLeidos != 3) return std::nullopt;
        return lectura;
    }
};

class IClasificadorColor {
public:
    virtual ~IClasificadorColor() = default;
    virtual Color clasificar(const LecturaRGB& lectura) const = 0;
};

class ClasificadorDominante : public IClasificadorColor {
public:
    Color clasificar(const LecturaRGB& l) const override {
        if (l.rojo  < l.verde && l.rojo  < l.azul)  return Color::Rojo;
        if (l.verde < l.rojo  && l.verde < l.azul)  return Color::Verde;
        if (l.azul  < l.rojo  && l.azul  < l.verde) return Color::Azul;
        return Color::Desconocido;
    }
};

struct Rango {
    unsigned long minimo = 0;
    unsigned long maximo = std::numeric_limits<unsigned long>::max();
    bool contiene(unsigned long valor) const { return valor >= minimo && valor <= maximo; }
};

struct RangoColor {
    Color color;
    Rango rojo;
    Rango verde;
    Rango azul;
};

class ClasificadorPorRangos : public IClasificadorColor {
public:
    explicit ClasificadorPorRangos(std::vector<RangoColor> rangos) : rangos_(std::move(rangos)) {}

    static ClasificadorPorRangos conRangosDelArduino() {
        const unsigned long INF = std::numeric_limits<unsigned long>::max();
        return ClasificadorPorRangos({
            {Color::Rojo,  {0, 69},    {151, INF}, {121, INF}},
            {Color::Verde, {181, INF}, {0, 149},   {121, INF}},
            {Color::Azul,  {161, INF}, {131, INF}, {0, 99}},
        });
    }

    Color clasificar(const LecturaRGB& l) const override {
        for (const RangoColor& r : rangos_) {
            if (r.rojo.contiene(l.rojo) && r.verde.contiene(l.verde) && r.azul.contiene(l.azul)) {
                return r.color;
            }
        }
        return Color::Desconocido;
    }

private:
    std::vector<RangoColor> rangos_;
};

struct DeteccionCaja {
    Color color;
    int lecturas;
};

class DetectorCajas {
public:
    DetectorCajas(std::unique_ptr<IClasificadorColor> clasificador,
                  unsigned long umbralPresencia,
                  int lecturasVaciasParaCerrar)
        : clasificador_(std::move(clasificador)),
          umbralPresencia_(umbralPresencia),
          lecturasVaciasParaCerrar_(lecturasVaciasParaCerrar) {
        if (!clasificador_) throw std::invalid_argument("El detector necesita un clasificador");
        if (lecturasVaciasParaCerrar_ < 1) lecturasVaciasParaCerrar_ = 1;
    }

    std::optional<DeteccionCaja> procesar(const LecturaRGB& lectura) {
        const bool hayObjeto = lectura.minimo() < umbralPresencia_;

        if (hayObjeto) {
            ++votos_[clasificador_->clasificar(lectura)];
            ++lecturasCaja_;
            vaciasSeguidas_ = 0;
            return std::nullopt;
        }

        if (lecturasCaja_ == 0) return std::nullopt;

        ++vaciasSeguidas_;
        if (vaciasSeguidas_ < lecturasVaciasParaCerrar_) return std::nullopt;

        return cerrarCaja();
    }

    std::optional<DeteccionCaja> finalizar() {
        if (lecturasCaja_ == 0) return std::nullopt;
        return cerrarCaja();
    }

private:
    DeteccionCaja cerrarCaja() {
        DeteccionCaja deteccion{colorGanador(), lecturasCaja_};
        votos_.clear();
        lecturasCaja_ = 0;
        vaciasSeguidas_ = 0;
        return deteccion;
    }

    Color colorGanador() const {
        Color ganador = Color::Desconocido;
        int maxVotos = 0;
        for (const auto& par : votos_) {
            if (par.first != Color::Desconocido && par.second > maxVotos) {
                ganador = par.first;
                maxVotos = par.second;
            }
        }
        return ganador;
    }

    std::unique_ptr<IClasificadorColor> clasificador_;
    unsigned long umbralPresencia_;
    int lecturasVaciasParaCerrar_;

    std::map<Color, int> votos_;
    int lecturasCaja_ = 0;
    int vaciasSeguidas_ = 0;
};

class IRepositorioCajas {
public:
    virtual ~IRepositorioCajas() = default;
    virtual void guardar(const Caja& caja) = 0;
    virtual int cantidadGuardada() const = 0;
};

class RepositorioCSV : public IRepositorioCajas {
public:
    explicit RepositorioCSV(const std::string& ruta) {
        const int filasPrevias = contarFilas(ruta);
        archivo_.open(ruta, std::ios::app);
        if (!archivo_.is_open()) {
            throw std::runtime_error("No se pudo abrir o crear el archivo " + ruta);
        }
        if (filasPrevias < 0) {
            archivo_ << "numero,color,fecha_hora,lecturas\n";
            cantidad_ = 0;
        } else {
            cantidad_ = filasPrevias > 0 ? filasPrevias - 1 : 0;
        }
    }

    void guardar(const Caja& caja) override {
        archivo_ << caja.numero() << ','
                 << colorATexto(caja.color()) << ','
                 << caja.fechaHora() << ','
                 << caja.lecturasUsadas() << '\n';
        archivo_.flush();
        ++cantidad_;
    }

    int cantidadGuardada() const override { return cantidad_; }

private:
    static int contarFilas(const std::string& ruta) {
        std::ifstream lectura(ruta);
        if (!lectura.is_open()) return -1;
        int filas = 0;
        std::string linea;
        while (std::getline(lectura, linea)) {
            if (!linea.empty()) ++filas;
        }
        return filas == 0 ? -1 : filas;
    }

    std::ofstream archivo_;
    int cantidad_ = 0;
};

class RepositorioMemoria : public IRepositorioCajas {
public:
    void guardar(const Caja& caja) override { cajas_.push_back(caja); }
    int cantidadGuardada() const override { return static_cast<int>(cajas_.size()); }
    const std::vector<Caja>& cajas() const { return cajas_; }

private:
    std::vector<Caja> cajas_;
};

class IObservadorCajas {
public:
    virtual ~IObservadorCajas() = default;
    virtual void alRegistrarCaja(const Caja& caja, const ResumenConteo& resumen) = 0;
};

class ObservadorConsola : public IObservadorCajas {
public:
    explicit ObservadorConsola(std::ostream& salida = std::cout) : salida_(salida) {}

    void alRegistrarCaja(const Caja& caja, const ResumenConteo& resumen) override {
        salida_ << "[" << caja.fechaHora() << "] Caja #" << caja.numero()
                << ": " << colorATexto(caja.color())
                << "   | Rojas: "  << resumen.cantidad(Color::Rojo)
                << "  Verdes: "    << resumen.cantidad(Color::Verde)
                << "  Azules: "    << resumen.cantidad(Color::Azul)
                << "  Total sesion: " << resumen.total() << '\n';
    }

private:
    std::ostream& salida_;
};

class RegistroCajas {
public:
    explicit RegistroCajas(std::unique_ptr<IRepositorioCajas> repositorio)
        : repositorio_(std::move(repositorio)) {
        if (!repositorio_) throw std::invalid_argument("El registro necesita un repositorio");
    }

    void suscribir(std::shared_ptr<IObservadorCajas> observador) {
        if (observador) observadores_.push_back(std::move(observador));
    }

    Caja registrar(const DeteccionCaja& deteccion) {
        const int numero = repositorio_->cantidadGuardada() + 1;
        cajas_.emplace_back(numero, deteccion.color, deteccion.lecturas, Caja::Reloj::now());
        const Caja& caja = cajas_.back();

        resumen_.sumar(caja.color());
        repositorio_->guardar(caja);
        notificar(caja);
        return caja;
    }

    const ResumenConteo& resumen() const { return resumen_; }
    const std::vector<Caja>& cajas() const { return cajas_; }

private:
    void notificar(const Caja& caja) {
        for (const auto& observador : observadores_) {
            observador->alRegistrarCaja(caja, resumen_);
        }
    }

    std::unique_ptr<IRepositorioCajas> repositorio_;
    std::vector<std::shared_ptr<IObservadorCajas>> observadores_;
    std::vector<Caja> cajas_;
    ResumenConteo resumen_;
};

enum class TipoFuente { Simulada, Archivo, Serial };
enum class TipoClasificador { Dominante, Rangos };

struct Configuracion {
    TipoFuente fuente = TipoFuente::Simulada;
    std::string puerto = "COM3";
    unsigned long baudios = 9600;
    std::string archivoEntrada;
    std::string archivoCSV = "registro_cajas.csv";
    TipoClasificador clasificador = TipoClasificador::Dominante;
    unsigned long umbralPresencia = 150;
    int lecturasVaciasParaCerrar = 1;
    bool mostrarAyuda = false;

    static Configuracion desdeArgumentos(int argc, char* argv[]) {
        Configuracion c;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            auto valor = [&]() -> std::string {
                if (i + 1 >= argc) throw std::invalid_argument("Falta un valor despues de " + arg);
                return argv[++i];
            };

            if (arg == "--ayuda" || arg == "-h") c.mostrarAyuda = true;
            else if (arg == "--simulado")    c.fuente = TipoFuente::Simulada;
            else if (arg == "--serial")    { c.fuente = TipoFuente::Serial;  c.puerto = valor(); }
            else if (arg == "--archivo")   { c.fuente = TipoFuente::Archivo; c.archivoEntrada = valor(); }
            else if (arg == "--baudios")     c.baudios = std::stoul(valor());
            else if (arg == "--csv")         c.archivoCSV = valor();
            else if (arg == "--umbral")      c.umbralPresencia = std::stoul(valor());
            else if (arg == "--vacias")      c.lecturasVaciasParaCerrar = std::stoi(valor());
            else if (arg == "--clasificador") {
                const std::string tipo = valor();
                if (tipo == "dominante")   c.clasificador = TipoClasificador::Dominante;
                else if (tipo == "rangos") c.clasificador = TipoClasificador::Rangos;
                else throw std::invalid_argument("Clasificador desconocido: " + tipo);
            }
            else throw std::invalid_argument("Opcion desconocida: " + arg + " (usa --ayuda)");
        }
        return c;
    }
};

class FabricaFuentes {
public:
    static std::unique_ptr<IFuenteLecturas> crear(const Configuracion& config) {
        switch (config.fuente) {
            case TipoFuente::Archivo:
                return std::make_unique<FuenteArchivo>(config.archivoEntrada);
            case TipoFuente::Serial:
#ifdef _WIN32
                return std::make_unique<PuertoSerieWindows>(config.puerto, config.baudios);
#else
                throw std::runtime_error("El puerto serie solo esta implementado para Windows");
#endif
            case TipoFuente::Simulada:
            default:
                return std::make_unique<FuenteSimulada>();
        }
    }
};

class FabricaClasificadores {
public:
    static std::unique_ptr<IClasificadorColor> crear(const Configuracion& config) {
        if (config.clasificador == TipoClasificador::Rangos) {
            return std::make_unique<ClasificadorPorRangos>(ClasificadorPorRangos::conRangosDelArduino());
        }
        return std::make_unique<ClasificadorDominante>();
    }
};

class SistemaConteo {
public:
    SistemaConteo(std::unique_ptr<IFuenteLecturas> fuente,
                  ParserLecturas parser,
                  DetectorCajas detector,
                  RegistroCajas& registro)
        : fuente_(std::move(fuente)),
          parser_(parser),
          detector_(std::move(detector)),
          registro_(registro) {
        if (!fuente_) throw std::invalid_argument("El sistema necesita una fuente de lecturas");
    }

    void ejecutar() {
        std::string linea;
        while (fuente_->leerLinea(linea)) {
            procesarLinea(linea);
        }
        if (auto caja = detector_.finalizar()) {
            registro_.registrar(*caja);
        }
    }

private:
    void procesarLinea(const std::string& linea) {
        const auto lectura = parser_.parsear(linea);
        if (!lectura || !lectura->esValida()) return;

        if (auto caja = detector_.procesar(*lectura)) {
            registro_.registrar(*caja);
        }
    }

    std::unique_ptr<IFuenteLecturas> fuente_;
    ParserLecturas parser_;
    DetectorCajas detector_;
    RegistroCajas& registro_;
};

void mostrarAyuda() {
    std::cout <<
        "Uso: conteo_cajas [opciones]\n\n"
        "Fuente de lecturas (elige una):\n"
        "  --simulado              datos de prueba inventados (por defecto)\n"
        "  --serial COM3           lee del Arduino por el puerto COM3\n"
        "  --archivo ruta.txt      lee lineas desde un archivo de texto\n\n"
        "Otras opciones:\n"
        "  --baudios 9600          velocidad del puerto (igual que Serial.begin)\n"
        "  --csv registro.csv      archivo donde se guarda el registro\n"
        "  --umbral 150            menor que esto = hay una caja delante\n"
        "  --vacias 1              lecturas vacias seguidas para cerrar una caja\n"
        "  --clasificador dominante|rangos\n"
        "  --ayuda                 muestra esta ayuda\n";
}

void mostrarResumenFinal(const ResumenConteo& resumen, const std::string& archivoCSV) {
    std::cout << "\n===== RESUMEN DE ESTA SESION =====\n"
              << "Rojas:        " << resumen.cantidad(Color::Rojo) << '\n'
              << "Verdes:       " << resumen.cantidad(Color::Verde) << '\n'
              << "Azules:       " << resumen.cantidad(Color::Azul) << '\n'
              << "Desconocidas: " << resumen.cantidad(Color::Desconocido) << '\n'
              << "TOTAL:        " << resumen.total() << '\n'
              << "Registro guardado en: " << archivoCSV << '\n';
}

int main(int argc, char* argv[]) {
    try {
        const Configuracion config = Configuracion::desdeArgumentos(argc, argv);
        if (config.mostrarAyuda) {
            mostrarAyuda();
            return 0;
        }

        auto fuente       = FabricaFuentes::crear(config);
        auto clasificador = FabricaClasificadores::crear(config);
        auto repositorio  = std::make_unique<RepositorioCSV>(config.archivoCSV);

        DetectorCajas detector(std::move(clasificador),
                               config.umbralPresencia,
                               config.lecturasVaciasParaCerrar);
        RegistroCajas registro(std::move(repositorio));
        registro.suscribir(std::make_shared<ObservadorConsola>());

        SistemaConteo sistema(std::move(fuente), ParserLecturas{}, std::move(detector), registro);

        if (config.fuente == TipoFuente::Serial) {
            std::cout << "Escuchando " << config.puerto << "... (Ctrl+C para salir)\n";
        }
        sistema.ejecutar();

        mostrarResumenFinal(registro.resumen(), config.archivoCSV);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
