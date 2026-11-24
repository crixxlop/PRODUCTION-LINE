#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <windows.h> 
using namespace std;


class ModuloLectorSerial {
private:
    string puertoCom;
    unsigned long baudRate;
    bool conectado;
    string bufferInterno;
    HANDLE hSerial; 

public:
    ModuloLectorSerial(const string& puerto, unsigned long baudios = 115200)
        : puertoCom(puerto), baudRate(baudios), conectado(false), hSerial(INVALID_HANDLE_VALUE) {}

    ~ModuloLectorSerial() {
        desconectar();
    }

    bool conectar() {
        
        string puertoFormateado = "\\\\.\\" + puertoCom;
        
        hSerial = CreateFileA(puertoFormateado.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hSerial == INVALID_HANDLE_VALUE) return false;

        // Configuración de los parámetros del puerto COM
        DCB dcbParams = { 0 };
        dcbParams.DCBlength = sizeof(dcbParams);
        if (!GetCommState(hSerial, &dcbParams)) { CloseHandle(hSerial); hSerial = INVALID_HANDLE_VALUE; return false; }

        dcbParams.BaudRate = baudRate;
        dcbParams.ByteSize = 8;
        dcbParams.StopBits = ONESTOPBIT;
        dcbParams.Parity = NOPARITY;
        if (!SetCommState(hSerial, &dcbParams)) { CloseHandle(hSerial); hSerial = INVALID_HANDLE_VALUE; return false; }

        
        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = 10;
        timeouts.ReadTotalTimeoutConstant = 10;
        timeouts.ReadTotalTimeoutMultiplier = 1;
        SetCommTimeouts(hSerial, &timeouts);
        
        conectado = true;
        return true;
    }

    void desconectar() {
        if (conectado && hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
            conectado = false;
        }
    }

    
    string leerBuffer() {
        if (!conectado) return "";
        
        char tempBuffer[128];
        DWORD bytesLeidos = 0; 

        ReadFile(hSerial, tempBuffer, sizeof(tempBuffer) - 1, &bytesLeidos, NULL);

        if (bytesLeidos > 0) {
                        bufferInterno.append(tempBuffer, bytesLeidos);
        }
        return bufferInterno;
    }

    
    string recibirTrama() {
        leerBuffer();
        size_t pos = bufferInterno.find('\n');
        if (pos != string::npos) {
            string trama = bufferInterno.substr(0, pos);
            if (!trama.empty() && trama.back() == '\r') {
                trama.pop_back(); 
            }
            bufferInterno.erase(0, pos + 1);
            return trama;
        }
        return ""; 
    }

    bool estaConectado() const { return conectado; }
};


class ContadoresEstado {
private:
    int totalRojas = 0;
    int totalAzules = 0;
    int totalAmarillas = 0;
    int totalDesconocidas = 0;

public:
    void incrementar(const string& color) {
        if (color == "ROJO") {
            ++totalRojas;
            return;
        }

        if (color == "AZUL") {
            ++totalAzules;
            return;
        }

        if (color == "AMARILLO") {
            ++totalAmarillas;
            return;
        }

        ++totalDesconocidas;
    }

    int getTotalRojas() const {
        return totalRojas;
    }

    int getTotalAzules() const {
        return totalAzules;
    }

    int getTotalAmarillas() const {
        return totalAmarillas;
    }

    int getTotalDesconocidas() const {
        return totalDesconocidas;
    }

    int obtenerTotal() const {
        return getTotalRojas() + getTotalAzules() + getTotalAmarillas();
    }
};


class InterfazUsuario {
public:
    void mostrarEstado(const string& estado) {
        cout << "[SISTEMA BANDA] Estado actual: " << estado << endl;
    }

    void mostrarContadores(const ContadoresEstado& contadores) {
       
        cout << " Cajas Rojas     : " << contadores.getTotalRojas() << endl;
        cout << " Cajas Azules    : " << contadores.getTotalAzules() << endl;
        cout << " Cajas Amarillas : " << contadores.getTotalAmarillas() << endl;
        if (contadores.getTotalDesconocidas() > 0) {
            cout << " Sin clasificar  : " << contadores.getTotalDesconocidas() << endl;
        }
        cout << "----------------------------------------" << endl;
        cout << " Total Procesado : " << contadores.obtenerTotal() << endl;

        if (contadores.obtenerTotal() > 0) {
            int total = contadores.obtenerTotal();
            cout << " Porcentaje      : Rojas " << contadores.getTotalRojas() * 100 / total
                 << "% | Azules " << contadores.getTotalAzules() * 100 / total
                 << "% | Amarillas " << contadores.getTotalAmarillas() * 100 / total << "%" << endl;
        }
        
    }
};


class LogicaClasificacion {
private:
    shared_ptr<ContadoresEstado> contadores;
    shared_ptr<InterfazUsuario> ui;

public:
    LogicaClasificacion(shared_ptr<ContadoresEstado> c, shared_ptr<InterfazUsuario> u)
        : contadores(c), ui(u) {}

    void interpretarTrama(const string& trama) {
        if (trama.empty()) {
            return;
        }

        const string prefijoEstado = "ESTADO:";
        const string prefijoColor = "COLOR:";

        if (trama.compare(0, prefijoEstado.size(), prefijoEstado) == 0) {
            ui->mostrarEstado(trama.substr(prefijoEstado.size()));
            return;
        }

        if (trama.compare(0, prefijoColor.size(), prefijoColor) == 0) {
            clasificarProducto(trama.substr(prefijoColor.size()));
            return;
        }

        cout << "[AVISO] Mensaje desconocido: " << trama << endl;
    }

    void clasificarProducto(const string& color) {
        contadores->incrementar(color);
        ui->mostrarContadores(*contadores);
    }
};


int main() {
    string puerto;
    cout << "Ingrese el puerto COM de Arduino (Ej: COM3): ";
    cin >> puerto;

    
    ModuloLectorSerial lectorSerial(puerto, 115200);

    if (!lectorSerial.conectar()) {
        cerr << "Error: No se pudo abrir el puerto " << puerto << ". Verifique que el Arduino este conectado y el puerto no este en uso." << endl;
        return 1;
    }

    cout << "Conectado correctamente al puerto " << puerto << " a 115200 baudios." << endl;
    cout << "Esperando datos del Arduino... (cierre la ventana para salir)" << endl;
    cout << "======== SISTEMA BANDA CLASIFICADORA ========" << endl;
   

    auto contadores = make_shared<ContadoresEstado>();
    auto ui = make_shared<InterfazUsuario>();
    LogicaClasificacion clasificador(contadores, ui);


    while (lectorSerial.estaConectado()) {
        string trama = lectorSerial.recibirTrama();
        
        if (!trama.empty()) {
            clasificador.interpretarTrama(trama);
        }

      
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    return 0;
}
