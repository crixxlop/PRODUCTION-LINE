#ifndef CLASES_BANDA_TRANSPORTADORA_H
#define CLASES_BANDA_TRANSPORTADORA_H

#include <string>
#include <ctime>

// LineaProduccion
class LineaProduccion {
private:
    int idLinea;
    std::string nombreLinea;
    bool estadoLinea;

public:
    void consultarEstado() {
    }
};

// Interruptor
class Interruptor {
private:
    bool estado;

public:
    void conmutar() {
    }

    void notificarCambio() {
    }
};

// BandaTransportadora
class BandaTransportadora {
private:
    float velocidad;
    bool estado;

public:
    void encender() {
    }

    void apagar() {
    }

    void ajustarVelocidad(float velocidad) {
    }
};

// Actuador
class Actuador {
private:
    int posicion;

public:
    void desviar(std::string destinoContenedorSalida) {
    }

    void reset() {
    }
};

// Producto
class Producto {
private:
    std::string color;
    int idProducto;

public:
    void moverse() {
    }

    void obtenerColor() {
    }
};

// CajaRoja
class CajaRoja {
private:
    std::string colorHex;

public:
};

// CajaAzul
class CajaAzul {
private:
    std::string colorHex;

public:
};

// CajaAmarilla
class CajaAmarilla {
private:
    std::string colorHex;

public:
};

// Controlador
class Controlador {
private:
    bool estadoSistema;

public:
    void iniciarSistema() {
    }
};

// Sensor
class Sensor {
private:
    std::string colorDetectado;

public:
    void calibrar() {
    }

    std::string detectarColor() {
        return "";
    }
};

// Clasificador
class Clasificador {
private:
    int idServo;
    std::string tipoClasificacion;
    int angulo;

public:
    void activarBrazo(std::string color) {
    }
};

// RegistroClasificacion
class RegistroClasificacion {
private:
    time_t timestamp;
    Producto producto;

public:
    void guardar() {
    }
};

// ContadorCajas
class ContadorCajas {
private:
    int totalRojas;
    int totalAzules;
    int totalAmarillas;

public:
    void incrementar(std::string color) {
    }

    int obtenerTotal() {
        return 0;
    }
};

#endif
